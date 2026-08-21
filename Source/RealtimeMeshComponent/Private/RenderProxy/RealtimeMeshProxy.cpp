// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#include "RenderProxy/RealtimeMeshProxy.h"

#include "RealtimeMeshComponentModule.h"
#include "Core/RealtimeMeshFuture.h"
#include "Data/RealtimeMeshShared.h"
#include "Mesh/RealtimeMeshNaniteResourcesInterface.h"
#include "RenderProxy/RealtimeMeshLODProxy.h"
#include "RenderProxy/RealtimeMeshBufferSetProxy.h"
#include "RenderProxy/RealtimeMeshSectionProxy.h"
#include "RenderProxy/RealtimeMeshVertexFactory.h"

namespace RealtimeMesh
{
	FRealtimeMeshProxy::FRealtimeMeshProxy(const FRealtimeMeshContextRef& InContext)
		: Context(InContext)
		, ActiveLODMask(false, REALTIME_MESH_MAX_LODS)
		, ScreenPercentageNextLODMask(false, REALTIME_MESH_MAX_LODS)
		, ActiveStaticLODMask(false, REALTIME_MESH_MAX_LODS)
		, ActiveDynamicLODMask(false, REALTIME_MESH_MAX_LODS)
#if UE_ENABLE_DEBUG_DRAWING
		, CollisionTraceFlag(CTF_UseSimpleAndComplex)
#endif
	{
	}

	FRealtimeMeshProxy::~FRealtimeMeshProxy()
	{
		// Versions are destroyed on the render thread when the last scene proxy
		// referencing them is torn down. Resource releases for RHI children fan
		// out from here through the LOD/BS TCowPtr destructors.
		check(IsInRenderingThread());
	}

	ERHIFeatureLevel::Type FRealtimeMeshProxy::GetRHIFeatureLevel() const
	{
		// NOTE (IDIOM-008): reports the max RHI feature level rather than the
		// scene's actual feature level, which is not threaded through to the proxy
		// here. Adequate for current callers; a per-scene value would be needed for
		// correct behavior on a mixed/lower-feature-level scene.
		return GMaxRHIFeatureLevel;
	}

	TRange<float> FRealtimeMeshProxy::GetScreenSizeRangeForLOD(const FRealtimeMeshLODKey& LODKey) const
	{
		// PROXY-F21: ranges are immutable after publish and precomputed by
		// UpdatedCachedState — read the cached flat array instead of pinning shared
		// ptrs and rescanning the mask per LOD per view per frame.
		const int32 LODIndex = LODKey.Index();
		if (ScreenSizeRanges.IsValidIndex(LODIndex))
		{
			return ScreenSizeRanges[LODIndex];
		}
		return TRange<float>(0.0f, TNumericLimits<float>::Max());
	}

	TRange<float> FRealtimeMeshProxy::ComputeScreenSizeRangeForLOD(int32 LODIndex) const
	{
		const auto GetLODByIndex = [&](int32 Index) -> FRealtimeMeshLODProxyConstPtr
		{
			return LODs.IsValidIndex(Index) ? LODs[Index].ToSharedPtrConst() : FRealtimeMeshLODProxyConstPtr();
		};

		// Special case for LOD 0 as there's no higher lod to get the max screen size from
		if (LODIndex == 0)
		{
			const auto LOD = GetLODByIndex(LODIndex);
			return TRange<float>(LOD.IsValid() ? LOD->GetScreenSize() : 0.0f, TNumericLimits<float>::Max());
		}

		// Find previous active lod
		const int32 NextActive = ScreenPercentageNextLODMask.FindFrom(true, REALTIME_MESH_MAX_LOD_INDEX - (LODIndex - 1));

		// If there is no valid lod higher than us, then we just use max value for the upper end
		if (NextActive == INDEX_NONE)
		{
			const auto LOD = GetLODByIndex(LODIndex);
			return TRange<float>(LOD.IsValid() ? LOD->GetScreenSize() : 0.0f, TNumericLimits<float>::Max());
		}

		const auto LowerLOD = GetLODByIndex(LODIndex);
		const auto UpperLOD = GetLODByIndex(REALTIME_MESH_MAX_LOD_INDEX - NextActive);
		return TRange<float>(LowerLOD.IsValid() ? LowerLOD->GetScreenSize() : 0.0f,
			UpperLOD.IsValid() ? UpperLOD->GetScreenSize() : TNumericLimits<float>::Max());
	}

	void FRealtimeMeshProxy::SetNaniteResources_RT(FRealtimeMeshNaniteResourcesPtr&& InNaniteResources)
	{
		check(IsInRenderingThread());

		if (InNaniteResources.IsValid())
		{
			UE_LOG(LogRealtimeMesh, Verbose, TEXT("SetNaniteResources_RT: Setting new Nanite resources (HasValidData: %s)"),
				InNaniteResources->HasValidData() ? TEXT("true") : TEXT("false"));

			if (!InNaniteResources->HasValidData())
			{
				UE_LOG(LogRealtimeMesh, Warning, TEXT("SetNaniteResources_RT: Incoming Nanite resources have no valid data"));
			}

			// Shared so multiple proxy versions / scene proxies can pin the same
			// underlying resources; release happens via the custom deleter once
			// every reference drops.
			NaniteResources = MakeShareable(InNaniteResources.Release(), [](FRealtimeMeshNaniteResources* Resources)
			{
				if (Resources)
				{
					UE_LOG(LogRealtimeMesh, VeryVerbose, TEXT("Shared Nanite resources deleter called"));
					FRealtimeMeshNaniteResourcesDeleter::Destroy(Resources);
				}
			});
		}
		else
		{
			UE_LOG(LogRealtimeMesh, Verbose, TEXT("SetNaniteResources_RT: Clearing Nanite resources"));
			NaniteResources.Reset();
		}
	}

	void FRealtimeMeshProxy::SetNaniteResources_RT(const TSharedPtr<FRealtimeMeshNaniteResources>& InNaniteResources)
	{
		check(IsInRenderingThread());

		// Nanite resource-sharing: the instance arrives already initialized/registered and already wrapped
		// in a shared pointer with its custom deleter, so this is a plain refcount bump — no MakeShareable,
		// no re-registration. Multiple proxy versions / scene proxies pin the same underlying registration.
		NaniteResources = InNaniteResources;
	}

	void FRealtimeMeshProxy::ClearNaniteResources_RT()
	{
		NaniteResources.Reset();
	}

	void FRealtimeMeshProxy::SetDistanceField(FRealtimeMeshDistanceField&& InDistanceField)
	{
		check(IsInRenderingThread());

		DistanceField = MakeShared<const FDistanceFieldVolumeData>(InDistanceField.MoveToRenderingData());
	}

	void FRealtimeMeshProxy::SetDistanceField(const TSharedRef<const FRealtimeMeshDistanceField>& InDistanceField)
	{
		check(IsInRenderingThread());

		// API-L8: the snapshot is shared/immutable so we copy (not move) into rendering data.
		DistanceField = MakeShared<const FDistanceFieldVolumeData>(InDistanceField->CreateRenderingData());
	}

	void FRealtimeMeshProxy::SetCardRepresentation(FRealtimeMeshCardRepresentation&& InCardRepresentation)
	{
		check(IsInRenderingThread());

		CardRepresentation = MakeShared<const FCardRepresentationData>(InCardRepresentation.MoveToRenderingData());
	}

	void FRealtimeMeshProxy::SetCardRepresentation(const TSharedRef<const FRealtimeMeshCardRepresentation>& InCardRepresentation)
	{
		check(IsInRenderingThread());

		// API-L8: the snapshot is shared/immutable so we copy (not move) into rendering data.
		CardRepresentation = MakeShared<const FCardRepresentationData>(InCardRepresentation->CreateRenderingData());
	}

	FRealtimeMeshLODProxyConstPtr FRealtimeMeshProxy::GetLOD(FRealtimeMeshLODKey LODKey) const
	{
		return LODs.IsValidIndex(LODKey) ? LODs[LODKey].ToSharedPtrConst() : FRealtimeMeshLODProxyConstPtr();
	}

	FRealtimeMeshLODProxy* FRealtimeMeshProxy::FindWorkspaceLOD(FRealtimeMeshLODKey LODKey)
	{
		if (!LODs.IsValidIndex(LODKey))
		{
			return nullptr;
		}
		TCowPtr<FRealtimeMeshLODProxy>& Slot = LODs[LODKey];
		if (!Slot.IsValid())
		{
			return nullptr;
		}
		// .Write() clones the LOD if it's still shared with an older published
		// version, then rebinds Slot to the draft-private copy.
		FRealtimeMeshLODProxy& LOD = Slot.Write();
		TouchedLODIndices.Add(LODKey.Index());
		return &LOD;
	}

	FRealtimeMeshBufferSetProxy* FRealtimeMeshProxy::FindUniqueBufferSetForInPlace(const FRealtimeMeshBufferSetKey& BufferSetKey)
	{
		const FRealtimeMeshLODKey LODKey = BufferSetKey.LOD();
		if (!LODs.IsValidIndex(LODKey))
		{
			return nullptr;
		}

		TCowPtr<FRealtimeMeshLODProxy>& Slot = LODs[LODKey];

		// If the LOD node is shared with a published snapshot, every buffer set under it is
		// shared too — an in-place write would corrupt that snapshot. Bail.
		if (!Slot.IsValid() || !Slot.IsUnique())
		{
			return nullptr;
		}

		// Unique: return the existing LOD without ever cloning (GetUniqueUnchecked, not
		// Write(), so a concurrent GT holder appearing after IsUnique() can't cause a
		// clone-and-rebind of this published slot). The buffer set node is then checked
		// for uniqueness in turn.
		return Slot.GetUniqueUnchecked()->FindUniqueBufferSetForInPlace(BufferSetKey);
	}

	void FRealtimeMeshProxy::AddLODIfNotExists(const FRealtimeMeshLODKey& LODKey)
	{
		check(IsInRenderingThread());

		if (!LODs.IsValidIndex(LODKey))
		{
			LODs.SetNum(LODKey.Index() + 1);
		}
		else if (LODs[LODKey].IsValid())
		{
			// Don't wipe an already-populated LOD proxy — "IfNotExists" means leave it be.
			return;
		}

		LODs[LODKey] = TCowPtr<FRealtimeMeshLODProxy>(Context->CreateLODProxy(LODKey));
		TouchedLODIndices.Add(LODKey.Index());
	}

	void FRealtimeMeshProxy::RemoveLOD(const FRealtimeMeshLODKey& LODKey)
	{
		check(IsInRenderingThread());

		if (LODs.IsValidIndex(LODKey))
		{
			LODs[LODKey].Reset();
			// Removing a LOD invalidates LOD indices in the touched set (especially
			// after the trim loop below), so just reset; the next UpdatedCachedState
			// will run on whatever's left.
			TouchedLODIndices.Reset();

			for (int32 Index = LODs.Num() - 1; Index >= 0; Index--)
			{
				if (!LODs[Index].IsValid())
				{
					LODs.SetNum(Index);
				}
				else
				{
					break;
				}
			}
		}
	}

#if UE_ENABLE_DEBUG_DRAWING
	void FRealtimeMeshProxy::SetCollisionRenderData(const FKAggregateGeom& InAggGeom, ECollisionTraceFlag InCollisionTraceFlag, const FCollisionResponseContainer& InCollisionResponse)
	{
		bHasCollisionData = true;
		CachedAggGeom = InAggGeom;
		CollisionTraceFlag = InCollisionTraceFlag;
		CollisionResponse = InCollisionResponse;
	}
#endif

	void FRealtimeMeshProxy::UpdatedCachedState(FRHICommandListBase& RHICmdList)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(FRealtimeMeshProxy::UpdatedCachedState);

		// Refresh per-LOD cached state for touched LODs first. Untouched LODs
		// retain whatever cached state they had when they were cloned from the
		// previous version — by construction nothing in them changed.
		for (const int32 LODIndex : TouchedLODIndices)
		{
			if (LODs.IsValidIndex(LODIndex) && LODs[LODIndex].IsValid())
			{
				// .Write() is a no-op clone here since FindWorkspaceLOD already
				// ensured the slot is draft-private. UpdateCachedState is a
				// mutating method, so we need the mutable reference.
				LODs[LODIndex].Write().UpdateCachedState(RHICmdList);
			}
		}

		// Aggregate LOD draw masks into the proxy's masks.
		DrawMask = FRealtimeMeshDrawMask();
		ActiveLODMask = FRealtimeMeshLODMask(false, REALTIME_MESH_MAX_LODS);
		ScreenPercentageNextLODMask = FRealtimeMeshLODMask(false, REALTIME_MESH_MAX_LODS);
		ActiveStaticLODMask = FRealtimeMeshLODMask(false, REALTIME_MESH_MAX_LODS);
		ActiveDynamicLODMask = FRealtimeMeshLODMask(false, REALTIME_MESH_MAX_LODS);

		bool bHasInvalidStaticRayTracingSection = false;
		for (int32 LODIndex = 0; LODIndex < LODs.Num(); LODIndex++)
		{
			if (!LODs[LODIndex].IsValid())
			{
				continue;
			}
			const FRealtimeMeshLODProxy& LOD = LODs[LODIndex].Read();

			const auto LODDrawMask = LOD.GetDrawMask();
			DrawMask |= LODDrawMask;

			// if a lod has ray tracing data after a lod that doesn't we have to use dynamic ray tracing for the entire mesh
			if (bHasInvalidStaticRayTracingSection && LODDrawMask.CanRenderInStaticRayTracing())
			{
				DrawMask.SetFlag(ERealtimeMeshDrawMask::DynamicRayTracing);
			}
			bHasInvalidStaticRayTracingSection |= !LODDrawMask.CanRenderInStaticRayTracing();

			if (LODDrawMask.HasAnyFlags())
			{
				ActiveLODMask[LODIndex] = true;
				ScreenPercentageNextLODMask[REALTIME_MESH_MAX_LOD_INDEX - LODIndex] = true;
				ActiveStaticLODMask[LODIndex] = LODDrawMask.ShouldRenderStaticPath();
				ActiveDynamicLODMask[LODIndex] = LODDrawMask.ShouldRenderDynamicPath();
			}
		}

		check(!DrawMask.HasAnyFlags() || ActiveLODMask.CountSetBits() > 0);

		// Flatten the active-LOD masks into linear index lists so scene proxies
		// can walk them with a tight for-loop at draw time.
		ActiveLODIndices.Reset();
		ActiveStaticLODIndices.Reset();
		ActiveDynamicLODIndices.Reset();
		for (TConstSetBitIterator<TFixedAllocator<1>> It(ActiveLODMask); It; ++It)
		{
			if (LODs.IsValidIndex(It.GetIndex()) && LODs[It.GetIndex()].IsValid())
			{
				ActiveLODIndices.Add(It.GetIndex());
			}
		}
		for (TConstSetBitIterator<TFixedAllocator<1>> It(ActiveStaticLODMask); It; ++It)
		{
			if (LODs.IsValidIndex(It.GetIndex()) && LODs[It.GetIndex()].IsValid())
			{
				ActiveStaticLODIndices.Add(It.GetIndex());
			}
		}
		for (TConstSetBitIterator<TFixedAllocator<1>> It(ActiveDynamicLODMask); It; ++It)
		{
			if (LODs.IsValidIndex(It.GetIndex()) && LODs[It.GetIndex()].IsValid())
			{
				ActiveDynamicLODIndices.Add(It.GetIndex());
			}
		}

		// PROXY-F21: precompute the per-LOD screen-size ranges now that the LOD screen
		// sizes and ScreenPercentageNextLODMask are final. Per-frame LOD helpers then
		// index ScreenSizeRanges instead of recomputing (pinning shared ptrs + mask
		// rescan) per LOD per view per frame.
		ScreenSizeRanges.Reset();
		ScreenSizeRanges.SetNum(LODs.Num());
		for (int32 LODIndex = 0; LODIndex < LODs.Num(); LODIndex++)
		{
			ScreenSizeRanges[LODIndex] = ComputeScreenSizeRangeForLOD(LODIndex);
		}

		TouchedLODIndices.Reset();
	}

	void FRealtimeMeshProxy::Reset()
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(FRealtimeMeshProxy::Reset);

		LODs.Empty();
		TouchedLODIndices.Reset();

		NaniteResources.Reset();
		DistanceField.Reset();
		CardRepresentation.Reset();
		DrawMask = FRealtimeMeshDrawMask();
		ActiveLODMask.SetRange(0, REALTIME_MESH_MAX_LODS, false);
		ScreenPercentageNextLODMask.SetRange(0, REALTIME_MESH_MAX_LODS, false);
		ActiveStaticLODMask.SetRange(0, REALTIME_MESH_MAX_LODS, false);
		ActiveDynamicLODMask.SetRange(0, REALTIME_MESH_MAX_LODS, false);
		ActiveLODIndices.Reset();
		ActiveStaticLODIndices.Reset();
		ActiveDynamicLODIndices.Reset();
		ScreenSizeRanges.Reset();
	}

	TSharedRef<FRealtimeMeshProxy> FRealtimeMeshProxy::Clone() const
	{
		// Build a fresh proxy that initially shares every LOD TCowPtr with this
		// one. Lazy COW: the clone only diverges from us along whatever paths the
		// next batch of tasks actually mutates.
		const TSharedRef<FRealtimeMeshProxy> Cloned = MakeShareable(new FRealtimeMeshProxy(Context), FRealtimeMeshRenderThreadDeleter<FRealtimeMeshProxy>());
		Cloned->LODs = LODs;
		Cloned->DrawMask = DrawMask;
		Cloned->ActiveLODMask = ActiveLODMask;
		Cloned->ScreenPercentageNextLODMask = ScreenPercentageNextLODMask;
		Cloned->ActiveStaticLODMask = ActiveStaticLODMask;
		Cloned->ActiveDynamicLODMask = ActiveDynamicLODMask;
		Cloned->ActiveLODIndices = ActiveLODIndices;
		Cloned->ActiveStaticLODIndices = ActiveStaticLODIndices;
		Cloned->ActiveDynamicLODIndices = ActiveDynamicLODIndices;
		Cloned->ScreenSizeRanges = ScreenSizeRanges;
		Cloned->DistanceField = DistanceField;
		Cloned->CardRepresentation = CardRepresentation;
		Cloned->NaniteResources = NaniteResources;
		// PROXY-F17: atomic load/store — the GT may be writing this flag on the
		// published proxy (SetHasNaniteData_GT) while this RT clone reads it.
		Cloned->bHasNaniteData.store(bHasNaniteData.load(std::memory_order_relaxed), std::memory_order_relaxed);
#if UE_ENABLE_DEBUG_DRAWING
		Cloned->bHasCollisionData = bHasCollisionData;
		Cloned->CollisionTraceFlag = CollisionTraceFlag;
		Cloned->CollisionResponse = CollisionResponse;
		Cloned->CachedAggGeom = CachedAggGeom;
#endif
		// TouchedLODIndices intentionally left empty — that's draft scratch.
		return Cloned;
	}
}
