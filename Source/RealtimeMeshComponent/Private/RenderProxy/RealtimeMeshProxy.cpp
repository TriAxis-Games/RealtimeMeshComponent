// Copyright (c) 2015-2025 TriAxis Games, L.L.C. All Rights Reserved.

#include "RenderProxy/RealtimeMeshProxy.h"

#include "RealtimeMeshComponentModule.h"
#include "RealtimeMeshSceneViewExtension.h"
#include "Data/RealtimeMeshShared.h"
#include "RenderProxy/RealtimeMeshLODProxy.h"

namespace RealtimeMesh
{
	FRealtimeMeshProxy::FRealtimeMeshProxy(const FRealtimeMeshSharedResourcesRef& InSharedResources)
		: SharedResources(InSharedResources)
		, ActiveLODMask(false, REALTIME_MESH_MAX_LODS)
		, ScreenPercentageNextLODMask(false, REALTIME_MESH_MAX_LODS)
		, ActiveStaticLODMask(false, REALTIME_MESH_MAX_LODS)
		, ActiveDynamicLODMask(false, REALTIME_MESH_MAX_LODS)
		, ReferencingHandle(MakeShared<uint8>(0xFF))
#if UE_ENABLE_DEBUG_DRAWING
		, CollisionTraceFlag(CTF_UseSimpleAndComplex)
#endif
	{
	}

	FRealtimeMeshProxy::~FRealtimeMeshProxy()
	{
		// The mesh proxy can only be safely destroyed from the rendering thread.
		// This is so that all the resources can be safely freed correctly.
		check(IsInRenderingThread());
		Reset();

		while (!CommandQueue.IsEmpty())
		{
			auto Entry = CommandQueue.Dequeue();
			Entry->ThreadState->FinalizeRenderThread(ERealtimeMeshProxyUpdateStatus::NoProxy);
		}	
	}

	ERHIFeatureLevel::Type FRealtimeMeshProxy::GetRHIFeatureLevel() const
	{
		// TODO: Probably shouldn't assume this as max all the time??
		return GMaxRHIFeatureLevel;
	}

	TRange<float> FRealtimeMeshProxy::GetScreenSizeRangeForLOD(const FRealtimeMeshLODKey& LODKey) const
	{
		const int32 LODIndex = LODKey.Index();

		// Special case for LOD 0 as there's no higher lod to get the max screen size from
		if (LODIndex == 0)
		{
			return TRange<float>(GetLOD(LODIndex)->GetScreenSize(), TNumericLimits<float>::Max());
		}
		
		// Find previous active lod
#if RMC_ENGINE_ABOVE_5_3
		const int32 NextActive = ScreenPercentageNextLODMask.FindFrom(true, REALTIME_MESH_MAX_LOD_INDEX - (LODIndex - 1));
#else
		int32 NextActive = INDEX_NONE;

		for (int32 Index = REALTIME_MESH_MAX_LOD_INDEX - (LODIndex - 1); Index < REALTIME_MESH_MAX_LODS; Index++)
		{
			if (ScreenPercentageNextLODMask[Index])
			{
				NextActive = Index;
				break;
			}
		}		
#endif
		
		// If there is no valid lod higher than us, then we just use max value for the upper end
		if (NextActive == INDEX_NONE)
		{
			return TRange<float>(GetLOD(LODIndex)->GetScreenSize(), TNumericLimits<float>::Max());
		}		
		
		return TRange<float>(GetLOD(LODIndex)->GetScreenSize(), GetLOD(REALTIME_MESH_MAX_LOD_INDEX - NextActive)->GetScreenSize());
	}

	void FRealtimeMeshProxy::SetDistanceField(FRealtimeMeshDistanceField&& InDistanceField)
	{
		check(IsInRenderingThread());
		
		DistanceField = MakeUnique<FDistanceFieldVolumeData>(InDistanceField.MoveToRenderingData());
	}

	bool FRealtimeMeshProxy::HasDistanceFieldData() const
	{
#if RMC_ENGINE_ABOVE_5_2
		return DistanceField.IsValid() && DistanceField->IsValid();
#else
		return DistanceField.IsValid();
#endif
	}

	void FRealtimeMeshProxy::SetNaniteResources(const TSharedPtr<IRealtimeMeshNaniteResources>& InNaniteResources)
	{
		check(IsInRenderingThread());

		NaniteResources = InNaniteResources;
	}

	bool FRealtimeMeshProxy::HasNaniteResources() const
	{
		return NaniteResources.IsValid();
	}

	void FRealtimeMeshProxy::SetCardRepresentation(FRealtimeMeshCardRepresentation&& InCardRepresentation)
	{
		check(IsInRenderingThread());
		
		CardRepresentation = MakeUnique<FCardRepresentationData>(InCardRepresentation.MoveToRenderingData());
	}

	FRealtimeMeshLODProxyPtr FRealtimeMeshProxy::GetLOD(FRealtimeMeshLODKey LODKey) const
	{
		return LODs.IsValidIndex(LODKey) ? LODs[LODKey] : FRealtimeMeshLODProxyPtr();
	}

	void FRealtimeMeshProxy::AddLODIfNotExists(const FRealtimeMeshLODKey& LODKey)
	{
		check(IsInRenderingThread());

		if (!LODs.IsValidIndex(LODKey))
		{
			LODs.SetNum(LODKey.Index() + 1);
		}

		LODs[LODKey] = SharedResources->CreateLODProxy(LODKey);
	}

	void FRealtimeMeshProxy::RemoveLOD(const FRealtimeMeshLODKey& LODKey)
	{
		check(IsInRenderingThread());

		if (LODs.IsValidIndex(LODKey))
		{
			LODs[LODKey].Reset();

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
			
		//bOwnerIsNull = ParentBaseComponent->GetOwner() == nullptr;
	}
#endif

	void FRealtimeMeshProxy::EnqueueCommandBatch(TArray<FRealtimeMeshProxyUpdateBuilder::TaskFunctionType>&& InTasks, const TSharedPtr<FRealtimeMeshCommandBatchIntermediateFuture>& ThreadState)
	{
		CommandQueue.Enqueue(FCommandBatch { MoveTemp(InTasks), ThreadState });
	}

	void FRealtimeMeshProxy::ProcessCommands(FRHICommandListBase& RHICmdList)
	{
		FScopeLock Lock(&CommandQueueLock);
		
		bool bHadAnyUpdates = false;
		while (!CommandQueue.IsEmpty())
		{
			auto Entry = CommandQueue.Dequeue();
			for (const auto& Task : Entry->Tasks)
			{
				Task(RHICmdList, *this);
			}
			Entry->ThreadState->FinalizeRenderThread(ERealtimeMeshProxyUpdateStatus::Updated);
			bHadAnyUpdates = true;
		}

		if (bHadAnyUpdates)
		{
			UpdatedCachedState(RHICmdList);
		}
	}

	void FRealtimeMeshProxy::UpdatedCachedState(FRHICommandListBase& RHICmdList)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(FRealtimeMeshProxy::UpdatedCachedState);
		
		// Handle all LOD updates next
		for (const auto& LOD : LODs)
		{
			LOD->UpdateCachedState(RHICmdList);
		}
		
		DrawMask = FRealtimeMeshDrawMask();
		ActiveLODMask = FRealtimeMeshLODMask(false, REALTIME_MESH_MAX_LODS);
		ScreenPercentageNextLODMask = FRealtimeMeshLODMask(false, REALTIME_MESH_MAX_LODS);
		ActiveStaticLODMask = FRealtimeMeshLODMask(false, REALTIME_MESH_MAX_LODS);
		ActiveDynamicLODMask = FRealtimeMeshLODMask(false, REALTIME_MESH_MAX_LODS);

		bool bHasInvalidStaticRayTracingSection = false;
		for (int32 LODIndex = 0; LODIndex < LODs.Num(); LODIndex++)
		{
			const auto& LOD = LODs[LODIndex];

			const auto LODDrawMask = LOD->GetDrawMask();
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
				ActiveStaticLODMask[LODIndex] = LOD->GetDrawMask().ShouldRenderStaticPath();
				ActiveDynamicLODMask[LODIndex] = LOD->GetDrawMask().ShouldRenderDynamicPath();
			}
		}
		
		check(!DrawMask.HasAnyFlags() || ActiveLODMask.CountSetBits() > 0);
	}

	void FRealtimeMeshProxy::Reset()
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(FRealtimeMeshProxy::Reset);
		
		LODs.Empty();

		DrawMask = FRealtimeMeshDrawMask();
		ActiveLODMask.SetRange(0, REALTIME_MESH_MAX_LODS, false);
		ScreenPercentageNextLODMask.SetRange(0, REALTIME_MESH_MAX_LODS, false);
		ActiveStaticLODMask.SetRange(0, REALTIME_MESH_MAX_LODS, false);
		ActiveDynamicLODMask.SetRange(0, REALTIME_MESH_MAX_LODS, false);
	}
}
