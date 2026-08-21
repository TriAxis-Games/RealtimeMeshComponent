// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#include "Data/RealtimeMeshData.h"

#include "RealtimeMesh.h"
#include "RealtimeMeshSceneViewExtension.h"
#include "RealtimeMeshSubsystem.h"
#include "Core/RealtimeMeshFuture.h"
#include "Data/RealtimeMeshLOD.h"
#include "Data/RealtimeMeshBufferSet.h"
#include "Data/RealtimeMeshUpdateBuilder.h"
#include "Mesh/RealtimeMeshNaniteResourcesInterface.h"
#include "RenderProxy/RealtimeMeshProxy.h"
#include "RenderProxy/RealtimeMeshProxyCommandBatch.h"
#include "RenderProxy/RealtimeMeshLODProxy.h"
#include "RenderProxy/RealtimeMeshBufferSetProxy.h"
#include "Logging/MessageLog.h"

#define LOCTEXT_NAMESPACE "RealtimeMesh"

namespace RealtimeMesh
{
	// DUP-009: shared body for the material-slot forwarders below. Each forwarder took a
	// GC scope guard, pinned the owning UObject, checked IsValid, and forwarded the call,
	// returning a per-forwarder default when the mesh was invalid. The guard's lifetime
	// (spanning GetOwningMesh + IsValid + the forwarded call) and each forwarder's early-out
	// default are preserved exactly. File-local so no member is added to the frozen header.
	namespace
	{
		template <typename ResultType, typename FunctionType>
		ResultType WithOwningMesh(const FRealtimeMeshContextRef& Context, ResultType DefaultValue, FunctionType&& Func)
		{
			FGCScopeGuard GCGuard;
			if (URealtimeMesh* Mesh = Context->GetOwningMesh(); IsValid(Mesh))
			{
				return Func(Mesh);
			}
			return DefaultValue;
		}

		template <typename FunctionType>
		void WithOwningMesh(const FRealtimeMeshContextRef& Context, FunctionType&& Func)
		{
			FGCScopeGuard GCGuard;
			if (URealtimeMesh* Mesh = Context->GetOwningMesh(); IsValid(Mesh))
			{
				Func(Mesh);
			}
		}
	}

	FRealtimeMesh::FRealtimeMesh(const FRealtimeMeshContextRef& InContext)
		: Context(InContext)
		, CollisionUpdateVersionCounter(0)
	{
	}

	FRealtimeMesh::~FRealtimeMesh()
	{
	}

	int32 FRealtimeMesh::GetNumLODs(const FRealtimeMeshLockContext& LockContext) const
	{
		return LODs.Num();
	}

	FRealtimeMeshLODPtr FRealtimeMesh::GetLOD(const FRealtimeMeshLockContext& LockContext, FRealtimeMeshLODKey LODKey) const
	{
		return LODs.IsValidIndex(LODKey) ? LODs[LODKey] : FRealtimeMeshLODPtr();
	}

	FRealtimeMeshSectionGroupPtr FRealtimeMesh::GetSectionGroup(const FRealtimeMeshLockContext& LockContext, FRealtimeMeshBufferSetKey SectionGroupKey) const
	{
		if (const FRealtimeMeshLODPtr LOD = GetLOD(LockContext, SectionGroupKey.LOD()))
		{
			return LOD->GetSectionGroup(LockContext, SectionGroupKey);
		}
		return nullptr;
	}

	FRealtimeMeshSectionPtr FRealtimeMesh::GetSection(const FRealtimeMeshLockContext& LockContext, FRealtimeMeshSectionKey SectionKey) const
	{
		if (const FRealtimeMeshLODPtr LOD = GetLOD(LockContext, SectionKey.LOD()))
		{
			if (const FRealtimeMeshSectionGroupPtr SectionGroup = LOD->GetSectionGroup(LockContext, SectionKey.SectionGroup()))
			{
				return SectionGroup->GetSection(LockContext, SectionKey);
			}
		}
		return nullptr;
	}

	TOptional<FBoxSphereBounds3f> FRealtimeMesh::GetLocalBounds(const FRealtimeMeshLockContext& LockContext) const
	{
		return Bounds.Get();
	}

	TFuture<ERealtimeMeshCollisionUpdateResult> FRealtimeMesh::UpdateCollision(FRealtimeMeshCollisionInfo&& InCollisionData, int32 NewCollisionKey)
	{
		// Complex (trimesh) geometry is always applied to the body setup, so it always needs
		// its cook. Simple geometry (convex hulls) is only applied when NOT using
		// complex-as-simple collision — ApplyCollisionUpdate skips CopySimpleGeometryToBodySetup
		// under CTF_UseComplexAsSimple, dropping the simple geometry entirely. So when
		// bUseComplexAsSimpleCollision is set, cooking the convex hulls is pure waste; skip it.
		TArray<int32> MeshesNeedingCook = InCollisionData.ComplexGeometry.GetMeshIDsNeedingCook();
		TArray<int32> ConvexObjectsNeedingCook;
		if (!InCollisionData.Configuration.bUseComplexAsSimpleCollision)
		{
			ConvexObjectsNeedingCook = InCollisionData.SimpleGeometry.GetMeshIDsNeedingCook();
		}
		const bool bNeedsCookAnything = MeshesNeedingCook.Num() > 0 || ConvexObjectsNeedingCook.Num() > 0;

		// Cook all meshes/convex's that need to be cooked.
		if (bNeedsCookAnything)
		{
			ParallelForTemplate(MeshesNeedingCook.Num() + ConvexObjectsNeedingCook.Num(), [&InCollisionData, &MeshesNeedingCook, &ConvexObjectsNeedingCook](int32 Index)
			{
				if (Index < MeshesNeedingCook.Num())
				{
					URealtimeMeshCollisionTools::CookComplexMesh(InCollisionData.ComplexGeometry.GetByIndex(MeshesNeedingCook[Index]));
				}
				else
				{
					URealtimeMeshCollisionTools::CookConvexHull(InCollisionData.SimpleGeometry.ConvexHulls.GetByIndex(ConvexObjectsNeedingCook[Index - MeshesNeedingCook.Num()]));
				}
			});
		}

		return DoOnGameThread([ThisWeak = this->AsWeak(), CollisionData = MoveTemp(InCollisionData), NewCollisionKey]() mutable
		{
			check(IsInGameThread());

			auto Pinned = ThisWeak.Pin();

			if (!Pinned)
			{
				return ERealtimeMeshCollisionUpdateResult::Ignored;
			}

			URealtimeMesh* Mesh = Pinned->GetContext()->GetOwningMesh();
			if (!IsValid(Mesh) || Mesh->CurrentCollisionVersion >= NewCollisionKey)
			{
				return ERealtimeMeshCollisionUpdateResult::Ignored;
			}

			return Mesh->ApplyCollisionUpdate(MoveTemp(CollisionData), NewCollisionKey);
		});
	}

	void FRealtimeMesh::MarkForEndOfFrameUpdate() const
	{
		FRealtimeMeshEndOfFrameUpdateManager::Get().MarkComponentForUpdate(ConstCastWeakPtr<FRealtimeMesh>(this->AsWeak()));
	}

	void FRealtimeMesh::MarkBoundsDirtyIfNotOverridden(FRealtimeMeshUpdateContext& UpdateContext)
	{
		Bounds.ClearCachedValue();
		if (!Bounds.HasUserSetBounds())
		{
			UpdateContext.GetState().bNeedsBoundsUpdate = true;
		}
	}

	void FRealtimeMesh::InitializeLODs(FRealtimeMeshUpdateContext& UpdateContext, const TFixedLODArray<FRealtimeMeshLODConfig>& InLODConfigs)
	{		
		if (InLODConfigs.Num() == 0)
		{
			FMessageLog("RealtimeMesh").Error(LOCTEXT("RealtimeMeshLODCountError", "RealtimeMesh must have at least one LOD"));
			return;
		}

		if (auto ProxyBuilder = UpdateContext.GetProxyBuilder())
		{
			ProxyBuilder->AddMeshTask([](FRHICommandListBase& RHICmdList, FRealtimeMeshProxy& Proxy)
			{
				Proxy.Reset();
			}, true /* Always need to dirty render state with this */);
		}

		check(InLODConfigs.Num() > 0);
		LODs.Empty(InLODConfigs.Num());
		for (int32 Index = 0; Index < InLODConfigs.Num(); Index++)
		{
			LODs.Add(CreateLOD(Index));

			if (auto ProxyBuilder = UpdateContext.GetProxyBuilder())
			{
				ProxyBuilder->AddMeshTask([LODIndex = Index](FRHICommandListBase& RHICmdList, FRealtimeMeshProxy& Proxy)
				{
					Proxy.AddLODIfNotExists(LODIndex);
				});
			}
			LODs[Index]->Initialize(UpdateContext, InLODConfigs[Index]);
		}
	}

	void FRealtimeMesh::AddLOD(FRealtimeMeshUpdateContext& UpdateContext, const FRealtimeMeshLODConfig& LODConfig, FRealtimeMeshLODKey* OutLODKey)
	{
		if (LODs.Num() >= REALTIME_MESH_MAX_LODS)
		{
			FMessageLog("RealtimeMesh").Error(FText::Format(LOCTEXT("RealtimeMeshLODCountError", "RealtimeMesh must have at most {0} LODs"),
															FText::AsNumber(REALTIME_MESH_MAX_LODS)));
			if (OutLODKey)
			{
				*OutLODKey = FRealtimeMeshLODKey();
			}
			return;
		}

		const int32 NewLODIndex = LODs.Num();
		const auto NewLOD = CreateLOD(NewLODIndex);
		LODs.Add(NewLOD);

		if (auto ProxyBuilder = UpdateContext.GetProxyBuilder())
		{
			ProxyBuilder->AddMeshTask([NewLODIndex](FRHICommandListBase& RHICmdList, FRealtimeMeshProxy& Proxy)
			{
				Proxy.AddLODIfNotExists(NewLODIndex);
			}, true /* Always need to dirty render state with this */);
		}
		NewLOD->Initialize(UpdateContext, LODConfig);

		if (OutLODKey)
		{
			*OutLODKey = NewLODIndex;
		}
	}

	void FRealtimeMesh::RemoveTrailingLOD(FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshLODKey* OutNewLastLODKey)
	{
		if (LODs.Num() < 2)
		{
			FMessageLog("RealtimeMesh").Error(LOCTEXT("RealtimeMeshLODCountError", "RealtimeMesh must have at least one LOD"));
			if (OutNewLastLODKey)
			{
				*OutNewLastLODKey = FRealtimeMeshLODKey(LODs.Num() - 1);
			}
			return;
		}

		const int32 RemovedLODIndex = LODs.Num() - 1;
		LODs.RemoveAt(RemovedLODIndex);

		if (auto ProxyBuilder = UpdateContext.GetProxyBuilder())
		{
			ProxyBuilder->AddMeshTask([RemovedLODIndex](FRHICommandListBase& RHICmdList, FRealtimeMeshProxy& Proxy)
			{
				Proxy.RemoveLOD(RemovedLODIndex);
			}, true /* Always need to dirty render state with this */);
		}

		if (OutNewLastLODKey)
		{
			*OutNewLastLODKey = LODs.Num() - 1;
		}
	}

	void FRealtimeMesh::SetNaniteResources(FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshNaniteResourcesPtr&& InNaniteResources)
	{
		// Create the update data for the GPU
		if (auto ProxyBuilder = UpdateContext.GetProxyBuilder())
		{
			FGCScopeGuard GCGuard;
			
			InNaniteResources->InitResources(Context->GetOwningMesh());
			
			ProxyBuilder->SetHasNaniteData(InNaniteResources.IsValid() && InNaniteResources->HasValidData());
			
			ProxyBuilder->AddMeshTask([NaniteResources = MoveTemp(InNaniteResources)](FRHICommandListBase& RHICmdList, FRealtimeMeshProxy& Proxy) mutable
			{
				Proxy.SetNaniteResources_RT(MoveTemp(NaniteResources));
			}, true);
		}
	}

	void FRealtimeMesh::ClearNaniteResources(FRealtimeMeshUpdateContext& UpdateContext)
	{		
		// Create the update data for the GPU
		if (auto ProxyBuilder = UpdateContext.GetProxyBuilder())
		{
			ProxyBuilder->SetHasNaniteData(false);
			
			ProxyBuilder->AddMeshTask([](FRHICommandListBase& RHICmdList, FRealtimeMeshProxy& Proxy) mutable
			{
				Proxy.ClearNaniteResources_RT();
			}, true);
		}
	}


	void FRealtimeMesh::SetDistanceField(FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshDistanceField&& InDistanceField)
	{		
		UpdateContext.GetState().bDistanceFieldDirty = true;
		// Create the update data for the GPU
		if (auto ProxyBuilder = UpdateContext.GetProxyBuilder())
		{
			ProxyBuilder->AddMeshTask([DistanceField = MoveTemp(InDistanceField)](FRHICommandListBase& RHICmdList, FRealtimeMeshProxy& Proxy) mutable
			{
				Proxy.SetDistanceField(MoveTemp(DistanceField));
			}, true);
		}
	}

	void FRealtimeMesh::ClearDistanceField(FRealtimeMeshUpdateContext& UpdateContext)
	{
		UpdateContext.GetState().bDistanceFieldDirty = true;
		// Create the update data for the GPU
		if (auto ProxyBuilder = UpdateContext.GetProxyBuilder())
		{
			ProxyBuilder->AddMeshTask([](FRHICommandListBase& RHICmdList, FRealtimeMeshProxy& Proxy) mutable
			{
				Proxy.ClearDistanceFieldData();
			}, true);
		}
	}

	void FRealtimeMesh::SetCardRepresentation(FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshCardRepresentation&& InCardRepresentation)
	{		
		UpdateContext.GetState().bCardRepresentationDirty = true;
		// Create the update data for the GPU
		if (auto ProxyBuilder = UpdateContext.GetProxyBuilder())
		{
			ProxyBuilder->AddMeshTask([CardRepresentation = MoveTemp(InCardRepresentation)](FRHICommandListBase& RHICmdList, FRealtimeMeshProxy& Proxy) mutable
			{
				Proxy.SetCardRepresentation(MoveTemp(CardRepresentation));
			}, true);
		}
	}

	void FRealtimeMesh::ClearCardRepresentation(FRealtimeMeshUpdateContext& UpdateContext)
	{		
		UpdateContext.GetState().bCardRepresentationDirty = true;
		// Create the update data for the GPU
		if (auto ProxyBuilder = UpdateContext.GetProxyBuilder())
		{
			ProxyBuilder->AddMeshTask([](FRHICommandListBase& RHICmdList, FRealtimeMeshProxy& Proxy) mutable
			{
				Proxy.ClearCardRepresentation();
			}, true);
		}
	}

	void FRealtimeMesh::SetupMaterialSlot(FRealtimeMeshUpdateContext& UpdateContext, int32 MaterialSlot, FName SlotName, UMaterialInterface* InMaterial)
	{
		WithOwningMesh(Context, [&](URealtimeMesh* Mesh)
		{
			Mesh->SetupMaterialSlot(MaterialSlot, SlotName, InMaterial);
		});
	}

	int32 FRealtimeMesh::GetMaterialIndex(const FRealtimeMeshLockContext& LockContext, FName MaterialSlotName) const
	{
		return WithOwningMesh<int32>(Context, INDEX_NONE, [&](URealtimeMesh* Mesh)
		{
			return Mesh->GetMaterialIndex(MaterialSlotName);
		});
	}

	FName FRealtimeMesh::GetMaterialSlotName(const FRealtimeMeshLockContext& LockContext, int32 Index) const
	{
		return WithOwningMesh<FName>(Context, NAME_None, [&](URealtimeMesh* Mesh)
		{
			return Mesh->GetMaterialSlotName(Index);
		});
	}

	bool FRealtimeMesh::IsMaterialSlotNameValid(const FRealtimeMeshLockContext& LockContext, FName MaterialSlotName) const
	{
		return WithOwningMesh<bool>(Context, false, [&](URealtimeMesh* Mesh)
		{
			return Mesh->IsMaterialSlotNameValid(MaterialSlotName);
		});
	}

	FRealtimeMeshMaterialSlot FRealtimeMesh::GetMaterialSlot(const FRealtimeMeshLockContext& LockContext, int32 SlotIndex) const
	{
		return WithOwningMesh<FRealtimeMeshMaterialSlot>(Context, FRealtimeMeshMaterialSlot(), [&](URealtimeMesh* Mesh)
		{
			return Mesh->GetMaterialSlot(SlotIndex);
		});
	}

	int32 FRealtimeMesh::GetNumMaterials(const FRealtimeMeshLockContext& LockContext) const
	{
		return WithOwningMesh<int32>(Context, 0, [&](URealtimeMesh* Mesh)
		{
			return Mesh->GetNumMaterials();
		});
	}

	TArray<FName> FRealtimeMesh::GetMaterialSlotNames(const FRealtimeMeshLockContext& LockContext) const
	{
		return WithOwningMesh<TArray<FName>>(Context, {}, [&](URealtimeMesh* Mesh)
		{
			return Mesh->GetMaterialSlotNames();
		});
	}

	TArray<FRealtimeMeshMaterialSlot> FRealtimeMesh::GetMaterialSlots(const FRealtimeMeshLockContext& LockContext) const
	{
		return WithOwningMesh<TArray<FRealtimeMeshMaterialSlot>>(Context, {}, [&](URealtimeMesh* Mesh)
		{
			return Mesh->GetMaterialSlots();
		});
	}

	UMaterialInterface* FRealtimeMesh::GetMaterial(const FRealtimeMeshLockContext& LockContext, int32 SlotIndex) const
	{
		return WithOwningMesh<UMaterialInterface*>(Context, nullptr, [&](URealtimeMesh* Mesh)
		{
			return Mesh->GetMaterial(SlotIndex);
		});
	}

	bool FRealtimeMesh::HasRenderProxy(const FRealtimeMeshLockContext& LockContext) const
	{
		FScopeLock Lock(&RenderProxyLock);
		return RenderProxy.IsValid();
	}

	FRealtimeMeshProxyPtr FRealtimeMesh::GetRenderProxy(bool bCreateIfNotExists) const
	{
		{
			FScopeLock Lock(&RenderProxyLock);
			if (RenderProxy.IsValid() || !bCreateIfNotExists || !FApp::CanEverRender())
			{
				return RenderProxy;
			}
		}

		return CreateRenderProxy();
	}

	bool FRealtimeMesh::ApplyAndPublish_RenderThread(
		FRHICommandListBase& RHICmdList,
		TArray<FRealtimeMeshProxyUpdateBuilder::TaskFunctionType>& InTasks)
	{
		check(IsInRenderingThread());

		// Take a stable ref to the current published version. We don't hold the
		// lock during the (potentially expensive) clone/apply/finalize work — RT
		// commands serialize naturally, so two batches won't race here, but we
		// still need the lock for the GT/RT publish swap.
		FRealtimeMeshProxyPtr Current;
		{
			FScopeLock Lock(&RenderProxyLock);
			Current = RenderProxy;
		}

		if (!Current.IsValid())
		{
			return false;
		}

		// Lazy-COW: clone shares every LOD with the current published version
		// until a task COWs along the path it touches.
		const TSharedRef<FRealtimeMeshProxy> Draft = Current->Clone();
		for (const auto& Task : InTasks)
		{
			Task(RHICmdList, *Draft);
		}
		Draft->UpdatedCachedState(RHICmdList);

		// Atomic publish — scene proxies created after this point capture the
		// new version; older scene proxies keep their captured reference and
		// don't see this update at all (which is the intended behavior).
		{
			FScopeLock Lock(&RenderProxyLock);
			RenderProxy = Draft;
		}

		return true;
	}

	ERealtimeMeshInPlaceApplyResult FRealtimeMesh::ApplyInPlace_RenderThread(
		FRHICommandListBase& RHICmdList,
		TArray<FRealtimeMeshInPlaceStreamUpdate>& InUpdates)
	{
		check(IsInRenderingThread());

		FRealtimeMeshProxyPtr Current;
		{
			FScopeLock Lock(&RenderProxyLock);
			Current = RenderProxy;
		}

		if (!Current.IsValid())
		{
			return ERealtimeMeshInPlaceApplyResult::NoProxy;
		}

		// Pass 1: resolve every target and confirm it's uniquely owned AND eligible. If any
		// check fails we must not mutate anything in place — fall through to the publish
		// path so the whole batch is applied consistently.
		TArray<FRealtimeMeshBufferSetProxy*, TInlineAllocator<8>> Targets;
		Targets.Reserve(InUpdates.Num());
		bool bAllInPlace = true;
		for (const FRealtimeMeshInPlaceStreamUpdate& Update : InUpdates)
		{
			FRealtimeMeshBufferSetProxy* BufferSet = Current->FindUniqueBufferSetForInPlace(Update.BufferSetKey);
			if (!BufferSet || !BufferSet->CanUpdateStreamInPlace(Update.UpdateData, Update.ElementOffset, Update.NumElements))
			{
				bAllInPlace = false;
				break;
			}
			Targets.Add(BufferSet);
		}

		if (bAllInPlace)
		{
			// Pass 2: overwrite each stream's GPU contents in place. No clone, no publish,
			// no vertex-factory reinit — the current published version (which the live scene
			// proxy holds) is updated directly.
			for (int32 Index = 0; Index < InUpdates.Num(); ++Index)
			{
				const FRealtimeMeshInPlaceStreamUpdate& Update = InUpdates[Index];
				Targets[Index]->UpdateStreamInPlace(RHICmdList, Update.UpdateData, Update.ElementOffset, Update.NumElements);
			}
			return ERealtimeMeshInPlaceApplyResult::AppliedInPlace;
		}

		// Fallback: a live snapshot still shares one of the target nodes, or a target is no
		// longer eligible (e.g. a Static buffer set or a changed element count). Clone,
		// apply reallocating updates to the draft, and publish so the result is correct.
		const TSharedRef<FRealtimeMeshProxy> Draft = Current->Clone();
		for (const FRealtimeMeshInPlaceStreamUpdate& Update : InUpdates)
		{
			if (FRealtimeMeshLODProxy* LOD = Draft->FindWorkspaceLOD(Update.BufferSetKey.LOD()))
			{
				if (FRealtimeMeshBufferSetProxy* BufferSet = LOD->FindMutableBufferSet(Update.BufferSetKey))
				{
					BufferSet->CreateOrUpdateStream(RHICmdList, Update.UpdateData);
				}
			}
		}
		Draft->UpdatedCachedState(RHICmdList);

		{
			FScopeLock Lock(&RenderProxyLock);
			RenderProxy = Draft;
		}

		return ERealtimeMeshInPlaceApplyResult::FellBackPublished;
	}

	void FRealtimeMesh::ResetInternal(FRealtimeMeshUpdateContext& UpdateContext, bool bRemoveRenderProxy)
	{
		FRealtimeMeshScopeGuardWriteCheck LockCheck(Context);

		ClearNaniteResources(UpdateContext);
		
		bool bHadProxy;
		{
			FScopeLock Lock(&RenderProxyLock);
			bHadProxy = RenderProxy.IsValid();
			if (bHadProxy && bRemoveRenderProxy)
			{
				RenderProxy.Reset();
			}
		}
		if (bHadProxy && !bRemoveRenderProxy)
		{
			if (auto ProxyBuilder = UpdateContext.GetProxyBuilder())
			{
				ProxyBuilder->AddMeshTask([](FRHICommandListBase& RHICmdList, FRealtimeMeshProxy& Proxy)
				{
					Proxy.Reset();
				}, true /* Always need to dirty render state with this */);
			}
		}

		Config = FRealtimeMeshConfig();
		LODs.Empty();
		Bounds.Reset();

		UpdateContext.GetState().bNeedsBoundsUpdate = true;
		UpdateContext.GetState().bNeedsCollisionUpdate = true;
		UpdateContext.GetState().bNeedsRenderProxyUpdate = true;
	}

	bool FRealtimeMesh::Serialize(FArchive& Ar, URealtimeMesh* Owner)
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(Context->GetGuard());

		int32 NumLODs = LODs.Num();
		Ar << NumLODs;

		if (Ar.IsLoading())
		{
			LODs.Empty(NumLODs);
			for (int32 Index = 0; Index < NumLODs; Index++)
			{
				LODs.Add(CreateLOD(Index));
				LODs[Index]->Serialize(Ar);
			}
		}
		else
		{
			for (TFixedLODArray<FRealtimeMeshLODRef>::TConstIterator It(LODs); It; ++It)
			{
				(*It)->Serialize(Ar);
			}
		}

		Ar << Config;
		Ar << Bounds;

		if (Ar.CustomVer(FRealtimeMeshVersion::GUID) < FRealtimeMeshVersion::CollisionUpdateFlowRestructure)
		{
			FRealtimeMeshSimpleGeometry SimpleGeom;
			FRealtimeMeshCollisionConfiguration CollisionConfig;
			Ar << SimpleGeom;
			Ar << CollisionConfig;
		}

		// NOTE (GUARD-006): this RenderProxy read is intentionally left unlocked,
		// unlike the rest of this file's locking discipline. Serialize runs at
		// load time before the proxy is concurrently reachable, so no read lock is
		// taken here by design.
		if (Ar.IsLoading() && RenderProxy)
		{
			// ReSharper disable once CppExpressionWithoutSideEffects
			CreateRenderProxy(true);
			//MarkRenderStateDirty(true);
		}

		return true;
	}

	void FRealtimeMesh::InitializeProxy(FRealtimeMeshUpdateContext& UpdateContext) const
	{
		if (auto ProxyBuilder = UpdateContext.GetProxyBuilder())
		{
			// Update existing LODs
			for (int32 Index = 0; Index < LODs.Num(); Index++)
			{
				ProxyBuilder->AddMeshTask([Index](FRHICommandListBase& RHICmdList, FRealtimeMeshProxy& Proxy)
				{
					Proxy.AddLODIfNotExists(Index);
				}, true /* Always need to dirty render state with this */);
				LODs[Index]->InitializeProxy(UpdateContext);
			}
		}
	}

	void FRealtimeMesh::FinalizeUpdate(FRealtimeMeshUpdateContext& UpdateContext)
	{
		// Only descend into LODs the update actually touched. FinalizeUpdate work is driven
		// entirely by the bounds/stream dirty trees; an untouched LOD (and everything under
		// it) can neither recompute bounds nor propagate a bounds change upward, so skipping
		// it is behavior-preserving. A stream-only edit doesn't flag the bounds tree, so the
		// stream tree must be consulted as well.
		FRealtimeMeshUpdateState& State = UpdateContext.GetState();
		for (const auto& LOD : LODs)
		{
			const FRealtimeMeshLODKey LODKey = LOD->GetKey(UpdateContext);
			if (State.BoundsDirtyTree.IsDirty(LODKey) || State.StreamRangeDirtyTree.IsDirty(LODKey) || State.StreamDirtyTree.HasAnyDirtyStreams(LODKey))
			{
				LOD->FinalizeUpdate(UpdateContext);
			}
		}

		// Update bounds
		if (UpdateContext.GetState().bNeedsBoundsUpdate && !Bounds.HasUserSetBounds())
		{
			// DUP-008: shared accumulate-hull helper (RealtimeMeshShared.h).
			const TOptional<FBoxSphereBounds3f> NewBounds = AccumulateBounds(LODs,
				[&UpdateContext](const FRealtimeMeshLODRef& LOD) { return LOD->GetLocalBounds(UpdateContext); });
			Bounds.SetComputedBounds(NewBounds.IsSet() ?
				*NewBounds :
				FBoxSphereBounds3f(FSphere3f(FVector3f::ZeroVector, 1.0f)));
		}
	}

	FRealtimeMeshProxyRef FRealtimeMesh::CreateRenderProxy(bool bForceRecreate) const
	{
		check(FApp::CanEverRender());

		FRealtimeMeshScopeGuardWrite ScopeGuard(Context);

		// Capture the proxy ref under RenderProxyLock. The final return must use this
		// local rather than re-reading RenderProxy unlocked at the end of the function:
		// Commit() below enqueues an RT command that reassigns RenderProxy under the same
		// lock, and a non-atomic TSharedPtr read racing that write is UB.
		bool bReusedExisting = false;
		FRealtimeMeshProxyPtr CapturedProxy;
		{
			FScopeLock Lock(&RenderProxyLock);
			if (!bForceRecreate && RenderProxy.IsValid())
			{
				bReusedExisting = true;
			}
			else
			{
				// Empty bootstrap version. Subsequent Commit batches clone this and
				// fill it in; readers prior to the first commit see a renderable-
				// but-empty mesh.
				RenderProxy = MakeShareable(new FRealtimeMeshProxy(Context), FRealtimeMeshRenderThreadDeleter<FRealtimeMeshProxy>());
			}
			CapturedProxy = RenderProxy;
		}

		// An existing proxy was reused without recreation: no new bootstrap was published,
		// so skip re-initialization/commit and return the captured ref directly.
		if (bReusedExisting)
		{
			return CapturedProxy.ToSharedRef();
		}

		FRealtimeMeshUpdateContext UpdateContext(Context);
		InitializeProxy(UpdateContext);
		auto ProxyBuilder = UpdateContext.GetProxyBuilder();
		// NOTE: do NOT ClearProxyRecreate here. In the old controller-style
		// proxy this was safe because the scene proxy held a stable mutable
		// controller that ProcessCommands populated before the first draw. In
		// the version-captured model the very first scene proxy to grab this
		// freshly-returned bootstrap will capture an *empty* version (the
		// InitializeProxy tasks are queued as an RT command that hasn't run
		// yet). Letting the broadcast fire after the publish completes drives
		// MarkRenderStateDirty, which the engine handles by recreating the
		// scene proxy against the now-populated version. The first scene proxy
		// renders empty for one frame before being replaced — acceptable.
		ProxyBuilder->Commit(this->AsShared());

		// Return the ref captured under RenderProxyLock above — NOT a fresh unlocked read
		// of RenderProxy, which Commit() may have reassigned on the render thread.
		return CapturedProxy.ToSharedRef();
	}


	FRealtimeMeshSectionRef FRealtimeMesh::CreateSection(const FRealtimeMeshSectionKey& InKey) const
	{
		return MakeShared<FRealtimeMeshSection>(Context, InKey);
	}

	FRealtimeMeshSectionGroupRef FRealtimeMesh::CreateSectionGroup(const FRealtimeMeshBufferSetKey& InKey) const
	{
		return MakeShared<FRealtimeMeshBufferSet>(Context, InKey);
	}

	FRealtimeMeshLODRef FRealtimeMesh::CreateLOD(const FRealtimeMeshLODKey& InKey) const
	{
		return MakeShared<FRealtimeMeshLOD>(Context, InKey);
	}

	FRealtimeMeshUpdateStateRef FRealtimeMesh::CreateUpdateState() const
	{
		return MakeShared<FRealtimeMeshUpdateState>();
	}
}

#undef LOCTEXT_NAMESPACE
