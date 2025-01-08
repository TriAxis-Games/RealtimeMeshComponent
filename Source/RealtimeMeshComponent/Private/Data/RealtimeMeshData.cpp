// Copyright (c) 2015-2025 TriAxis Games, L.L.C. All Rights Reserved.

#include "Data/RealtimeMeshData.h"

#include "RealtimeMesh.h"
#include "RealtimeMeshSceneViewExtension.h"
#include "RealtimeMeshSubsystem.h"
#include "Core/RealtimeMeshFuture.h"
#include "Data/RealtimeMeshLOD.h"
#include "Data/RealtimeMeshSectionGroup.h"
#include "Data/RealtimeMeshUpdateBuilder.h"
#include "Mesh/RealtimeMeshNaniteResourcesInterface.h"
#include "RenderProxy/RealtimeMeshProxy.h"
#include "RenderProxy/RealtimeMeshProxyCommandBatch.h"
#if RMC_ENGINE_ABOVE_5_2
#include "Logging/MessageLog.h"
#endif

#define LOCTEXT_NAMESPACE "RealtimeMesh"

namespace RealtimeMesh
{
	FRealtimeMesh::FRealtimeMesh(const FRealtimeMeshSharedResourcesRef& InSharedResources)
		: SharedResources(InSharedResources)
		, CollisionUpdateVersionCounter(0)
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

	FRealtimeMeshSectionGroupPtr FRealtimeMesh::GetSectionGroup(const FRealtimeMeshLockContext& LockContext, FRealtimeMeshSectionGroupKey SectionGroupKey) const
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
		// TODO: We can skip cook based on simpleascomplex or complexassimple
		TArray<int32> MeshesNeedingCook = InCollisionData.ComplexGeometry.GetMeshIDsNeedingCook();
		TArray<int32> ConvexObjectsNeedingCook = InCollisionData.SimpleGeometry.GetMeshIDsNeedingCook();
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

			URealtimeMesh* Mesh = Pinned->GetSharedResources()->GetOwningMesh();
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

	auto FRealtimeMesh::InitializeLODs(FRealtimeMeshUpdateContext& UpdateContext, const TFixedLODArray<FRealtimeMeshLODConfig>& InLODConfigs) -> void
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
			LODs.Add(SharedResources->CreateLOD(Index));

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
		const auto NewLOD = SharedResources->CreateLOD(NewLODIndex);
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

	void FRealtimeMesh::SetNaniteResources(FRealtimeMeshUpdateContext& UpdateContext, const TSharedRef<IRealtimeMeshNaniteResources>& InNaniteResources)
	{		
		// Create the update data for the GPU
		if (auto ProxyBuilder = UpdateContext.GetProxyBuilder())
		{
			InNaniteResources->InitResources(SharedResources->GetOwningMesh());
			
			ProxyBuilder->AddMeshTask([InNaniteResources](FRHICommandListBase& RHICmdList, FRealtimeMeshProxy& Proxy) mutable
			{
				Proxy.SetNaniteResources(InNaniteResources);
			}, true);
		}
	}

	void FRealtimeMesh::ClearNaniteResources(FRealtimeMeshUpdateContext& UpdateContext)
	{		
		// Create the update data for the GPU
		if (auto ProxyBuilder = UpdateContext.GetProxyBuilder())
		{
			ProxyBuilder->AddMeshTask([](FRHICommandListBase& RHICmdList, FRealtimeMeshProxy& Proxy) mutable
			{
				Proxy.SetNaniteResources(nullptr);
			}, true);
		}
	}

	void FRealtimeMesh::SetDistanceField(FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshDistanceField&& InDistanceField)
	{		
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
		// Create the update data for the GPU
		if (auto ProxyBuilder = UpdateContext.GetProxyBuilder())
		{
			ProxyBuilder->AddMeshTask([](FRHICommandListBase& RHICmdList, FRealtimeMeshProxy& Proxy) mutable
			{
				Proxy.SetDistanceField(FRealtimeMeshDistanceField());
			}, true);
		}
	}

	void FRealtimeMesh::SetCardRepresentation(FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshCardRepresentation&& InCardRepresentation)
	{		
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
		FGCScopeGuard GCGuard;
		if (URealtimeMesh* Mesh = SharedResources->GetOwningMesh(); IsValid(Mesh))
		{
			Mesh->SetupMaterialSlot(MaterialSlot, SlotName, InMaterial);
		}
	}

	int32 FRealtimeMesh::GetMaterialIndex(const FRealtimeMeshLockContext& LockContext, FName MaterialSlotName) const
	{
		FGCScopeGuard GCGuard;
		if (URealtimeMesh* Mesh = SharedResources->GetOwningMesh(); IsValid(Mesh))
		{
			return Mesh->GetMaterialIndex(MaterialSlotName);
		}
		return INDEX_NONE;
	}

	FName FRealtimeMesh::GetMaterialSlotName(const FRealtimeMeshLockContext& LockContext, int32 Index) const
	{
		FGCScopeGuard GCGuard;
		if (URealtimeMesh* Mesh = SharedResources->GetOwningMesh(); IsValid(Mesh))
		{
			return Mesh->GetMaterialSlotName(Index);
		}
		return NAME_None;
	}

	bool FRealtimeMesh::IsMaterialSlotNameValid(const FRealtimeMeshLockContext& LockContext, FName MaterialSlotName) const
	{
		FGCScopeGuard GCGuard;
		if (URealtimeMesh* Mesh = SharedResources->GetOwningMesh(); IsValid(Mesh))
		{
			return Mesh->IsMaterialSlotNameValid(MaterialSlotName);
		}
		return false;
	}

	FRealtimeMeshMaterialSlot FRealtimeMesh::GetMaterialSlot(const FRealtimeMeshLockContext& LockContext, int32 SlotIndex) const
	{
		FGCScopeGuard GCGuard;
		if (URealtimeMesh* Mesh = SharedResources->GetOwningMesh(); IsValid(Mesh))
		{
			return Mesh->GetMaterialSlot(SlotIndex);
		}
		return FRealtimeMeshMaterialSlot();
	}

	int32 FRealtimeMesh::GetNumMaterials(const FRealtimeMeshLockContext& LockContext) const
	{
		FGCScopeGuard GCGuard;
		if (URealtimeMesh* Mesh = SharedResources->GetOwningMesh(); IsValid(Mesh))
		{
			return Mesh->GetNumMaterials();
		}
		return 0;
	}

	TArray<FName> FRealtimeMesh::GetMaterialSlotNames(const FRealtimeMeshLockContext& LockContext) const
	{
		FGCScopeGuard GCGuard;
		if (URealtimeMesh* Mesh = SharedResources->GetOwningMesh(); IsValid(Mesh))
		{
			return Mesh->GetMaterialSlotNames();
		}
		return {};
	}

	TArray<FRealtimeMeshMaterialSlot> FRealtimeMesh::GetMaterialSlots(const FRealtimeMeshLockContext& LockContext) const
	{
		FGCScopeGuard GCGuard;
		if (URealtimeMesh* Mesh = SharedResources->GetOwningMesh(); IsValid(Mesh))
		{
			return Mesh->GetMaterialSlots();
		}
		return {};
	}

	UMaterialInterface* FRealtimeMesh::GetMaterial(const FRealtimeMeshLockContext& LockContext, int32 SlotIndex) const
	{
		FGCScopeGuard GCGuard;
		if (URealtimeMesh* Mesh = SharedResources->GetOwningMesh(); IsValid(Mesh))
		{
			return Mesh->GetMaterial(SlotIndex);
		}
		return nullptr;
	}

	bool FRealtimeMesh::HasRenderProxy(const FRealtimeMeshLockContext& LockContext) const
	{
		FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources->GetGuard());
		return RenderProxy.IsValid();
	}

	FRealtimeMeshProxyPtr FRealtimeMesh::GetRenderProxy(bool bCreateIfNotExists) const
	{
		FRealtimeMeshScopeGuardReadWrite ScopeGuard(SharedResources->GetGuard(), ERealtimeMeshGuardLockType::Read);
		if (RenderProxy.IsValid() || !bCreateIfNotExists || !FApp::CanEverRender())
		{
			return RenderProxy;
		}
		ScopeGuard.Unlock();

		// We hold this lock the entire time we initialize proxy so any proxy calls get delayed until after we grab the starting state

		return CreateRenderProxy();
	}

	void FRealtimeMesh::Reset(FRealtimeMeshUpdateContext& UpdateContext, bool bRemoveRenderProxy)
	{
		FRealtimeMeshScopeGuardWriteCheck LockCheck(SharedResources);

		if (RenderProxy)
		{
			if (bRemoveRenderProxy)
			{
				RenderProxy.Reset();
			}
			else
			{
				if (auto ProxyBuilder = UpdateContext.GetProxyBuilder())
				{
					ProxyBuilder->AddMeshTask([](FRHICommandListBase& RHICmdList, FRealtimeMeshProxy& Proxy)
					{
						Proxy.Reset();
					}, true /* Always need to dirty render state with this */);
				}
			}
		}

		Config = FRealtimeMeshConfig();
		LODs.Empty();
		Bounds.Reset();

		UpdateContext.GetState().bNeedsBoundsUpdate = true;
		UpdateContext.GetState().bNeedsCollisionUpdate = true;
		UpdateContext.GetState().bNeedsRenderProxyUpdate = true;

		/*// TODO: Best way to handle this?
		MarkRenderStateDirty(true, INDEX_NONE);

		SharedResources->BroadcastMeshConfigChanged();
		SharedResources->BroadcastMeshBoundsChanged();*/
	}

	bool FRealtimeMesh::Serialize(FArchive& Ar, URealtimeMesh* Owner)
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources->GetGuard());

		int32 NumLODs = LODs.Num();
		Ar << NumLODs;

		if (Ar.IsLoading())
		{
			LODs.Empty(NumLODs);
			for (int32 Index = 0; Index < NumLODs; Index++)
			{
				LODs.Add(SharedResources->CreateLOD(Index));
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
		for (const auto& LOD : LODs)
		{
			LOD->FinalizeUpdate(UpdateContext);
		}
		
		// Update bounds
		if (UpdateContext.GetState().bNeedsBoundsUpdate && !Bounds.HasUserSetBounds())
		{
			TOptional<FBoxSphereBounds3f> NewBounds;
			for (const auto& LOD : LODs)
			{
				auto SectionGroupBounds = LOD->GetLocalBounds(UpdateContext);
				if (SectionGroupBounds.IsSet())
				{
					if (!NewBounds.IsSet())
					{
						NewBounds = *SectionGroupBounds;
						continue;
					}
					NewBounds = *NewBounds + *SectionGroupBounds;
				}
			}
			Bounds.SetComputedBounds(NewBounds.IsSet() ?
				*NewBounds :
				FBoxSphereBounds3f(FSphere3f(FVector3f::ZeroVector, 1.0f)));
		}
	}

	FRealtimeMeshProxyRef FRealtimeMesh::CreateRenderProxy(bool bForceRecreate) const
	{
		check(FApp::CanEverRender());
		if (bForceRecreate || !RenderProxy.IsValid())
		{
			FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources);
			
			RenderProxy = SharedResources->CreateRealtimeMeshProxy();
			
			FRealtimeMeshUpdateContext UpdateContext(SharedResources);			

			InitializeProxy(UpdateContext);

			auto ProxyBuilder = UpdateContext.GetProxyBuilder();

			ProxyBuilder->ClearProxyRecreate();
			ProxyBuilder->Commit(this->AsShared());

			ENQUEUE_RENDER_COMMAND(RealtiemMeshRegisterMeshProxy)([ProxyWeak = RenderProxy->AsWeak()](FRHICommandListImmediate& RHICmdList)
			{
				if (auto Pinned = ProxyWeak.Pin())
				{
					FRealtimeMeshSceneViewExtension::RegisterProxy(Pinned);
				}
			});
		}
		return RenderProxy.ToSharedRef();
	}

}

#undef LOCTEXT_NAMESPACE
