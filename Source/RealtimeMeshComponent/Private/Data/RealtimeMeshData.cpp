// Copyright (c) 2015-2024 TriAxis Games, L.L.C. All Rights Reserved.

#include "Data/RealtimeMeshData.h"

#include "RealtimeMesh.h"
#include "RealtimeMeshSceneViewExtension.h"
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
		, VersionCounter(0)
		, LastFrameProxyUpdated(INDEX_NONE)
	{
		SharedResources->OnLODBoundsChanged().AddRaw(this, &FRealtimeMesh::HandleLODBoundsChanged);
	}

	int32 FRealtimeMesh::GetProxyVersion() const
	{
		return VersionCounter.GetValue();
	}

	int32 FRealtimeMesh::IncrementProxyVersionIfNotSameFrame()
	{
		if (LastFrameProxyUpdated != GFrameCounter)
		{
			LastFrameProxyUpdated = GFrameCounter;
			return VersionCounter.Increment();
		}
		return VersionCounter.GetValue();
	}

	int32 FRealtimeMesh::GetNumLODs() const
	{
		FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources->GetGuard());
		return LODs.Num();
	}

	FRealtimeMeshLODPtr FRealtimeMesh::GetLOD(FRealtimeMeshLODKey LODKey) const
	{
		FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources->GetGuard());
		return LODs.IsValidIndex(LODKey) ? LODs[LODKey] : FRealtimeMeshLODPtr();
	}

	FRealtimeMeshSectionGroupPtr FRealtimeMesh::GetSectionGroup(FRealtimeMeshSectionGroupKey SectionGroupKey) const
	{
		FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources->GetGuard());
		if (const FRealtimeMeshLODPtr LOD = GetLOD(SectionGroupKey.LOD()))
		{
			return LOD->GetSectionGroup(SectionGroupKey);
		}
		return nullptr;
	}

	FRealtimeMeshSectionPtr FRealtimeMesh::GetSection(FRealtimeMeshSectionKey SectionKey) const
	{
		FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources->GetGuard());
		if (const FRealtimeMeshLODPtr LOD = GetLOD(SectionKey.LOD()))
		{
			if (const FRealtimeMeshSectionGroupPtr SectionGroup = LOD->GetSectionGroup(SectionKey.SectionGroup()))
			{
				return SectionGroup->GetSection(SectionKey);
			}
		}
		return nullptr;
	}

	FBoxSphereBounds3f FRealtimeMesh::GetLocalBounds() const
	{
		FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources->GetGuard());
		return Bounds.GetBounds([&]() { return CalculateBounds(); });
	}

	TFuture<ERealtimeMeshCollisionUpdateResult> FRealtimeMesh::UpdateCollision(FRealtimeMeshCollisionInfo&& InCollisionData)
	{
		const auto Promise = MakeShared<TPromise<ERealtimeMeshCollisionUpdateResult>>();
		auto CollisionData = MakeShared<FRealtimeMeshCollisionInfo>(MoveTemp(InCollisionData));

		auto Handler = [Promise, SharedResources = SharedResources, CollisionData]() mutable
		{
			FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources->GetGuard());
			if (SharedResources->GetCollisionUpdateHandler().IsBound())
			{
				SharedResources->GetCollisionUpdateHandler().Execute(Promise, CollisionData, false);
			}
			else
			{
				Promise->SetValue(ERealtimeMeshCollisionUpdateResult::Error);
			}
		};

		if (IsInGameThread())
		{
			Handler();
		}
		else
		{
			AsyncTask(ENamedThreads::GameThread, MoveTemp(Handler));
		}
		return Promise->GetFuture();
	}

	TFuture<ERealtimeMeshProxyUpdateStatus> FRealtimeMesh::InitializeLODs(const TFixedLODArray<FRealtimeMeshLODConfig>& InLODConfigs)
	{
		FRealtimeMeshUpdateContext UpdateContext(SharedResources);
		InitializeLODs(UpdateContext, InLODConfigs);
		return UpdateContext.Commit();
	}

	auto FRealtimeMesh::InitializeLODs(FRealtimeMeshProxyUpdateBuilder& ProxyBuilder, const TFixedLODArray<FRealtimeMeshLODConfig>& InLODConfigs) -> void
	{
		FRealtimeMeshScopeGuardWriteCheck LockCheck(SharedResources);
		
		if (InLODConfigs.Num() == 0)
		{
			FMessageLog("RealtimeMesh").Error(LOCTEXT("RealtimeMeshLODCountError", "RealtimeMesh must have at least one LOD"));
			return;
		}

		if (ProxyBuilder)
		{
			ProxyBuilder.AddMeshTask([](FRHICommandListBase& RHICmdList, FRealtimeMeshProxy& Proxy)
			{
				Proxy.Reset();
			}, true /* Always need to dirty render state with this */);
		}

		check(InLODConfigs.Num() > 0);
		LODs.Empty(InLODConfigs.Num());
		for (int32 Index = 0; Index < InLODConfigs.Num(); Index++)
		{
			LODs.Add(SharedResources->CreateLOD(Index));

			if (ProxyBuilder)
			{
				ProxyBuilder.AddMeshTask([LODIndex = Index](FRHICommandListBase& RHICmdList, FRealtimeMeshProxy& Proxy)
				{
					Proxy.AddLODIfNotExists(LODIndex);
				});
			}
			LODs[Index]->Initialize(ProxyBuilder, InLODConfigs[Index]);

			SharedResources->BroadcastLODChanged(FRealtimeMeshLODKey(Index), ERealtimeMeshChangeType::Added);
		}
	}

	TFuture<ERealtimeMeshProxyUpdateStatus> FRealtimeMesh::AddLOD(const FRealtimeMeshLODConfig& LODConfig, FRealtimeMeshLODKey* OutLODKey)
	{
		FRealtimeMeshUpdateContext UpdateContext(SharedResources);
		AddLOD(UpdateContext, LODConfig, OutLODKey);
		return UpdateContext.Commit();
	}

	void FRealtimeMesh::AddLOD(FRealtimeMeshProxyUpdateBuilder& ProxyBuilder, const FRealtimeMeshLODConfig& LODConfig, FRealtimeMeshLODKey* OutLODKey)
	{
		FRealtimeMeshScopeGuardWriteCheck LockCheck(SharedResources);

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

		if (ProxyBuilder)
		{
			ProxyBuilder.AddMeshTask([NewLODIndex](FRHICommandListBase& RHICmdList, FRealtimeMeshProxy& Proxy)
			{
				Proxy.AddLODIfNotExists(NewLODIndex);
			}, true /* Always need to dirty render state with this */);
		}
		NewLOD->Initialize(ProxyBuilder, LODConfig);

		if (OutLODKey)
		{
			*OutLODKey = NewLODIndex;
		}

		SharedResources->BroadcastLODChanged(FRealtimeMeshLODKey(NewLODIndex), ERealtimeMeshChangeType::Added);
	}

	TFuture<ERealtimeMeshProxyUpdateStatus> FRealtimeMesh::RemoveTrailingLOD(FRealtimeMeshLODKey* OutNewLastLODKey)
	{
		FRealtimeMeshUpdateContext UpdateContext(SharedResources);
		RemoveTrailingLOD(UpdateContext, OutNewLastLODKey);
		return UpdateContext.Commit();
	}

	void FRealtimeMesh::RemoveTrailingLOD(FRealtimeMeshProxyUpdateBuilder& ProxyBuilder, FRealtimeMeshLODKey* OutNewLastLODKey)
	{
		FRealtimeMeshScopeGuardWriteCheck LockCheck(SharedResources);

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

		if (ProxyBuilder)
		{
			ProxyBuilder.AddMeshTask([RemovedLODIndex](FRHICommandListBase& RHICmdList, FRealtimeMeshProxy& Proxy)
			{
				Proxy.RemoveLOD(RemovedLODIndex);
			}, true /* Always need to dirty render state with this */);
		}

		if (OutNewLastLODKey)
		{
			*OutNewLastLODKey = LODs.Num() - 1;
		}

		SharedResources->BroadcastLODChanged(FRealtimeMeshLODKey(RemovedLODIndex), ERealtimeMeshChangeType::Removed);
	}

	void FRealtimeMesh::SetNaniteResources(FRealtimeMeshProxyUpdateBuilder& ProxyBuilder, const TSharedRef<IRealtimeMeshNaniteResources>& InNaniteResources)
	{
		FRealtimeMeshScopeGuardWriteCheck LockCheck(SharedResources);
		
		// Create the update data for the GPU
		if (ProxyBuilder)
		{
			InNaniteResources->InitResources(SharedResources->GetOwningMesh());
			
			ProxyBuilder.AddMeshTask([InNaniteResources](FRHICommandListBase& RHICmdList, FRealtimeMeshProxy& Proxy) mutable
			{
				Proxy.SetNaniteResources(InNaniteResources);
			}, true);
		}
	}

	TFuture<ERealtimeMeshProxyUpdateStatus> FRealtimeMesh::SetNaniteResources(const TSharedRef<IRealtimeMeshNaniteResources>& InNaniteResources)
	{
		FRealtimeMeshUpdateContext UpdateContext(SharedResources);
		SetNaniteResources(UpdateContext, InNaniteResources);
		return UpdateContext.Commit();
	}

	void FRealtimeMesh::ClearNaniteResources(FRealtimeMeshProxyUpdateBuilder& ProxyBuilder)
	{
		FRealtimeMeshScopeGuardWriteCheck LockCheck(SharedResources);
		
		// Create the update data for the GPU
		if (ProxyBuilder)
		{
			ProxyBuilder.AddMeshTask([](FRHICommandListBase& RHICmdList, FRealtimeMeshProxy& Proxy) mutable
			{
				Proxy.SetNaniteResources(nullptr);
			}, true);
		}
	}

	TFuture<ERealtimeMeshProxyUpdateStatus> FRealtimeMesh::ClearNaniteResources()
	{
		FRealtimeMeshUpdateContext UpdateContext(SharedResources);
		ClearNaniteResources(UpdateContext);
		return UpdateContext.Commit();
	}

	void FRealtimeMesh::SetDistanceField(FRealtimeMeshProxyUpdateBuilder& ProxyBuilder, FRealtimeMeshDistanceField&& InDistanceField)
	{
		FRealtimeMeshScopeGuardWriteCheck LockCheck(SharedResources);
		
		// Create the update data for the GPU
		if (ProxyBuilder)
		{
			ProxyBuilder.AddMeshTask([DistanceField = MoveTemp(InDistanceField)](FRHICommandListBase& RHICmdList, FRealtimeMeshProxy& Proxy) mutable
			{
				Proxy.SetDistanceField(MoveTemp(DistanceField));
			}, true);
		}
	}

	TFuture<ERealtimeMeshProxyUpdateStatus> FRealtimeMesh::SetDistanceField(FRealtimeMeshDistanceField&& InDistanceField)
	{
		FRealtimeMeshUpdateContext UpdateContext(SharedResources);
		SetDistanceField(UpdateContext, MoveTemp(InDistanceField));
		return UpdateContext.Commit();
	}

	void FRealtimeMesh::ClearDistanceField(FRealtimeMeshProxyUpdateBuilder& ProxyBuilder)
	{
		FRealtimeMeshScopeGuardWriteCheck LockCheck(SharedResources);
		
		// Create the update data for the GPU
		if (ProxyBuilder)
		{
			ProxyBuilder.AddMeshTask([](FRHICommandListBase& RHICmdList, FRealtimeMeshProxy& Proxy) mutable
			{
				Proxy.SetDistanceField(FRealtimeMeshDistanceField());
			}, true);
		}
	}

	TFuture<ERealtimeMeshProxyUpdateStatus> FRealtimeMesh::ClearDistanceField()
	{
		FRealtimeMeshUpdateContext UpdateContext(SharedResources);
		ClearDistanceField(UpdateContext);
		return UpdateContext.Commit();
	}

	void FRealtimeMesh::SetCardRepresentation(FRealtimeMeshProxyUpdateBuilder& ProxyBuilder, FRealtimeMeshCardRepresentation&& InCardRepresentation)
	{
		FRealtimeMeshScopeGuardWriteCheck LockCheck(SharedResources);
		
		// Create the update data for the GPU
		if (ProxyBuilder)
		{
			ProxyBuilder.AddMeshTask([CardRepresentation = MoveTemp(InCardRepresentation)](FRHICommandListBase& RHICmdList, FRealtimeMeshProxy& Proxy) mutable
			{
				Proxy.SetCardRepresentation(MoveTemp(CardRepresentation));
			}, true);
		}
	}

	TFuture<ERealtimeMeshProxyUpdateStatus> FRealtimeMesh::SetCardRepresentation(FRealtimeMeshCardRepresentation&& InCardRepresentation)
	{
		FRealtimeMeshUpdateContext UpdateContext(SharedResources);
		SetCardRepresentation(UpdateContext, MoveTemp(InCardRepresentation));
		return UpdateContext.Commit();
	}

	void FRealtimeMesh::ClearCardRepresentation(FRealtimeMeshProxyUpdateBuilder& ProxyBuilder)
	{
		FRealtimeMeshScopeGuardWriteCheck LockCheck(SharedResources);
		
		// Create the update data for the GPU
		if (ProxyBuilder)
		{
			ProxyBuilder.AddMeshTask([](FRHICommandListBase& RHICmdList, FRealtimeMeshProxy& Proxy) mutable
			{
				Proxy.ClearCardRepresentation();
			}, true);
		}
	}

	TFuture<ERealtimeMeshProxyUpdateStatus> FRealtimeMesh::ClearCardRepresentation()
	{
		FRealtimeMeshUpdateContext UpdateContext(SharedResources);
		ClearCardRepresentation(UpdateContext);
		return UpdateContext.Commit();
	}

	void FRealtimeMesh::SetupMaterialSlot(int32 MaterialSlot, FName SlotName, UMaterialInterface* InMaterial)
	{
		FGCScopeGuard GCGuard;
		if (URealtimeMesh* Mesh = SharedResources->GetOwningMesh(); IsValid(Mesh))
		{
			Mesh->SetupMaterialSlot(MaterialSlot, SlotName, InMaterial);
		}
	}

	int32 FRealtimeMesh::GetMaterialIndex(FName MaterialSlotName) const
	{
		FGCScopeGuard GCGuard;
		if (URealtimeMesh* Mesh = SharedResources->GetOwningMesh(); IsValid(Mesh))
		{
			return Mesh->GetMaterialIndex(MaterialSlotName);
		}
		return INDEX_NONE;
	}

	FName FRealtimeMesh::GetMaterialSlotName(int32 Index) const
	{
		FGCScopeGuard GCGuard;
		if (URealtimeMesh* Mesh = SharedResources->GetOwningMesh(); IsValid(Mesh))
		{
			return Mesh->GetMaterialSlotName(Index);
		}
		return NAME_None;
	}

	bool FRealtimeMesh::IsMaterialSlotNameValid(FName MaterialSlotName) const
	{
		FGCScopeGuard GCGuard;
		if (URealtimeMesh* Mesh = SharedResources->GetOwningMesh(); IsValid(Mesh))
		{
			return Mesh->IsMaterialSlotNameValid(MaterialSlotName);
		}
		return false;
	}

	FRealtimeMeshMaterialSlot FRealtimeMesh::GetMaterialSlot(int32 SlotIndex) const
	{
		FGCScopeGuard GCGuard;
		if (URealtimeMesh* Mesh = SharedResources->GetOwningMesh(); IsValid(Mesh))
		{
			return Mesh->GetMaterialSlot(SlotIndex);
		}
		return FRealtimeMeshMaterialSlot();
	}

	int32 FRealtimeMesh::GetNumMaterials() const
	{
		FGCScopeGuard GCGuard;
		if (URealtimeMesh* Mesh = SharedResources->GetOwningMesh(); IsValid(Mesh))
		{
			return Mesh->GetNumMaterials();
		}
		return 0;
	}

	TArray<FName> FRealtimeMesh::GetMaterialSlotNames() const
	{
		FGCScopeGuard GCGuard;
		if (URealtimeMesh* Mesh = SharedResources->GetOwningMesh(); IsValid(Mesh))
		{
			return Mesh->GetMaterialSlotNames();
		}
		return {};
	}

	TArray<FRealtimeMeshMaterialSlot> FRealtimeMesh::GetMaterialSlots() const
	{
		FGCScopeGuard GCGuard;
		if (URealtimeMesh* Mesh = SharedResources->GetOwningMesh(); IsValid(Mesh))
		{
			return Mesh->GetMaterialSlots();
		}
		return {};
	}

	UMaterialInterface* FRealtimeMesh::GetMaterial(int32 SlotIndex) const
	{
		FGCScopeGuard GCGuard;
		if (URealtimeMesh* Mesh = SharedResources->GetOwningMesh(); IsValid(Mesh))
		{
			return Mesh->GetMaterial(SlotIndex);
		}
		return nullptr;
	}

	bool FRealtimeMesh::HasRenderProxy() const
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

	TFuture<ERealtimeMeshProxyUpdateStatus> FRealtimeMesh::Reset(bool bRemoveRenderProxy)
	{
		FRealtimeMeshUpdateContext UpdateContext(SharedResources);
		Reset(UpdateContext, bRemoveRenderProxy);
		return UpdateContext.Commit();
	}

	void FRealtimeMesh::Reset(FRealtimeMeshProxyUpdateBuilder& ProxyBuilder, bool bRemoveRenderProxy)
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
				if (ProxyBuilder)
				{
					ProxyBuilder.AddMeshTask([](FRHICommandListBase& RHICmdList, FRealtimeMeshProxy& Proxy)
					{
						Proxy.Reset();
					}, true /* Always need to dirty render state with this */);
				}
			}
		}

		Config = FRealtimeMeshConfig();
		LODs.Empty();
		Bounds.Reset();

		// TODO: Best way to handle this?
		MarkRenderStateDirty(true, INDEX_NONE);

		SharedResources->BroadcastMeshConfigChanged();
		SharedResources->BroadcastMeshBoundsChanged();
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

	void FRealtimeMesh::InitializeProxy(FRealtimeMeshProxyUpdateBuilder& ProxyBuilder) const
	{
		if (ProxyBuilder)
		{
			// Update existing LODs
			for (int32 Index = 0; Index < LODs.Num(); Index++)
			{
				ProxyBuilder.AddMeshTask([Index](FRHICommandListBase& RHICmdList, FRealtimeMeshProxy& Proxy)
				{
					Proxy.AddLODIfNotExists(Index);
				}, true /* Always need to dirty render state with this */);
				LODs[Index]->InitializeProxy(ProxyBuilder);
			}
		}
	}

	FRealtimeMeshProxyRef FRealtimeMesh::CreateRenderProxy(bool bForceRecreate) const
	{
		check(FApp::CanEverRender());
		if (bForceRecreate || !RenderProxy.IsValid())
		{
			FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources->GetGuard());
			RenderProxy = SharedResources->CreateRealtimeMeshProxy();

			FRealtimeMeshProxyUpdateBuilder ProxyBuilder;

			InitializeProxy(ProxyBuilder);

			ProxyBuilder.ClearProxyRecreate();
			ProxyBuilder.Commit(this->AsShared());

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

	FBoxSphereBounds3f FRealtimeMesh::CalculateBounds() const
	{
		FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources->GetGuard());
		TOptional<FBoxSphereBounds3f> NewBounds;
		for (const auto& LOD : LODs)
		{
			if (!NewBounds.IsSet())
			{
				NewBounds = LOD->GetLocalBounds();
				continue;
			}
			NewBounds = *NewBounds + LOD->GetLocalBounds();
		}

		return NewBounds.IsSet() ? *NewBounds : FBoxSphereBounds3f(FSphere3f(FVector3f::ZeroVector, 1.0f));
	}

	void FRealtimeMesh::HandleLODBoundsChanged(const FRealtimeMeshLODKey& LODKey)
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources->GetGuard());
		Bounds.ClearCachedValue();
		SharedResources->BroadcastMeshBoundsChanged();
	}
}

#undef LOCTEXT_NAMESPACE
