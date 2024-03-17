// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#include "Data/RealtimeMeshData.h"

#include "Data/RealtimeMeshLOD.h"
#include "Data/RealtimeMeshSectionGroup.h"
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
	{
		SharedResources->OnLODBoundsChanged().AddRaw(this, &FRealtimeMesh::HandleLODBoundsChanged);
	}

	int32 FRealtimeMesh::GetNumLODs() const
	{
		FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources->GetGuard());
		return LODs.Num();
	}

	FRealtimeMeshLODDataPtr FRealtimeMesh::GetLOD(FRealtimeMeshLODKey LODKey) const
	{
		FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources->GetGuard());
		return LODs.IsValidIndex(LODKey) ? LODs[LODKey] : FRealtimeMeshLODDataPtr();
	}

	FRealtimeMeshSectionGroupPtr FRealtimeMesh::GetSectionGroup(FRealtimeMeshSectionGroupKey SectionGroupKey) const
	{
		FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources->GetGuard());
		if (const FRealtimeMeshLODDataPtr LOD = GetLOD(SectionGroupKey.LOD()))
		{
			return LOD->GetSectionGroup(SectionGroupKey);
		}
		return nullptr;
	}

	FRealtimeMeshSectionPtr FRealtimeMesh::GetSection(FRealtimeMeshSectionKey SectionKey) const
	{
		FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources->GetGuard());
		if (const FRealtimeMeshLODDataPtr LOD = GetLOD(SectionKey.LOD()))
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

	TFuture<ERealtimeMeshCollisionUpdateResult> FRealtimeMesh::UpdateCollision(FRealtimeMeshCollisionData&& InCollisionData)
	{
		const auto Promise = MakeShared<TPromise<ERealtimeMeshCollisionUpdateResult>>();
		auto CollisionData = MakeShared<FRealtimeMeshCollisionData>(MoveTemp(InCollisionData));

		auto Handler = [Promise, SharedResources = SharedResources, CollisionData]() mutable
		{
			FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources->GetGuard());
			if (ensure(SharedResources->GetCollisionUpdateHandler().IsBound()))
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
		FRealtimeMeshProxyCommandBatch Commands(SharedResources->GetOwner());
		InitializeLODs(Commands, InLODConfigs);
		return Commands.Commit();
	}

	void FRealtimeMesh::InitializeLODs(FRealtimeMeshProxyCommandBatch& Commands, const TFixedLODArray<FRealtimeMeshLODConfig>& InLODConfigs)
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources->GetGuard());

		if (InLODConfigs.Num() == 0)
		{
			FMessageLog("RealtimeMesh").Error(LOCTEXT("RealtimeMeshLODCountError", "RealtimeMesh must have at least one LOD"));
			return;
		}

		if (Commands)
		{
			Commands.AddMeshTask([](FRealtimeMeshProxy& Proxy)
			{
				Proxy.Reset();
			}, true /* Always need to dirty render state with this */);
		}

		check(InLODConfigs.Num() > 0);
		LODs.Empty(InLODConfigs.Num());
		for (int32 Index = 0; Index < InLODConfigs.Num(); Index++)
		{
			LODs.Add(SharedResources->CreateLOD(Index));

			if (Commands)
			{
				Commands.AddMeshTask([LODIndex = Index](FRealtimeMeshProxy& Proxy)
				{
					Proxy.AddLODIfNotExists(LODIndex);
				});
			}
			LODs[Index]->Initialize(Commands, InLODConfigs[Index]);

			SharedResources->BroadcastLODChanged(FRealtimeMeshLODKey(Index), ERealtimeMeshChangeType::Added);
		}
	}

	TFuture<ERealtimeMeshProxyUpdateStatus> FRealtimeMesh::AddLOD(const FRealtimeMeshLODConfig& LODConfig, FRealtimeMeshLODKey* OutLODKey)
	{
		FRealtimeMeshProxyCommandBatch Commands(SharedResources->GetOwner());
		AddLOD(Commands, LODConfig, OutLODKey);
		return Commands.Commit();
	}

	void FRealtimeMesh::AddLOD(FRealtimeMeshProxyCommandBatch& Commands, const FRealtimeMeshLODConfig& LODConfig, FRealtimeMeshLODKey* OutLODKey)
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources->GetGuard());

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

		if (Commands)
		{
			Commands.AddMeshTask([NewLODIndex](FRealtimeMeshProxy& Proxy)
			{
				Proxy.AddLODIfNotExists(NewLODIndex);
			}, true /* Always need to dirty render state with this */);
		}
		NewLOD->Initialize(Commands, LODConfig);

		if (OutLODKey)
		{
			*OutLODKey = NewLODIndex;
		}

		SharedResources->BroadcastLODChanged(FRealtimeMeshLODKey(NewLODIndex), ERealtimeMeshChangeType::Added);
	}

	TFuture<ERealtimeMeshProxyUpdateStatus> FRealtimeMesh::RemoveTrailingLOD(FRealtimeMeshLODKey* OutNewLastLODKey)
	{
		FRealtimeMeshProxyCommandBatch Commands(SharedResources->GetOwner());
		RemoveTrailingLOD(Commands, OutNewLastLODKey);
		return Commands.Commit();
	}

	void FRealtimeMesh::RemoveTrailingLOD(FRealtimeMeshProxyCommandBatch& Commands, FRealtimeMeshLODKey* OutNewLastLODKey)
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources->GetGuard());

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

		if (Commands)
		{
			Commands.AddMeshTask([RemovedLODIndex](FRealtimeMeshProxy& Proxy)
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

	bool FRealtimeMesh::HasRenderProxy() const
	{
		FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources->GetGuard());
		return RenderProxy.IsValid();
	}

	FRealtimeMeshProxyPtr FRealtimeMesh::GetRenderProxy(bool bCreateIfNotExists) const
	{
		FRealtimeMeshScopeGuardReadWrite ScopeGuard(SharedResources->GetGuard(), ERealtimeMeshGuardLockType::Read);
		if (RenderProxy.IsValid() || !bCreateIfNotExists)
		{
			return RenderProxy;
		}
		ScopeGuard.Unlock();

		// We hold this lock the entire time we initialize proxy so any proxy calls get delayed until after we grab the starting state

		return CreateRenderProxy();
	}

	TFuture<ERealtimeMeshProxyUpdateStatus> FRealtimeMesh::Reset(bool bRemoveRenderProxy)
	{
		FRealtimeMeshProxyCommandBatch Commands(SharedResources->GetOwner());
		Reset(Commands, bRemoveRenderProxy);
		return Commands.Commit();
	}

	void FRealtimeMesh::Reset(FRealtimeMeshProxyCommandBatch& Commands, bool bRemoveRenderProxy)
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources->GetGuard());
		if (RenderProxy)
		{
			if (bRemoveRenderProxy)
			{
				RenderProxy.Reset();
			}
			else
			{
				if (Commands)
				{
					Commands.AddMeshTask([](FRealtimeMeshProxy& Proxy)
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
		MarkRenderStateDirty(true);

		SharedResources->BroadcastMeshConfigChanged();
		SharedResources->BroadcastMeshBoundsChanged();
	}

	bool FRealtimeMesh::Serialize(FArchive& Ar)
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
			for (TFixedLODArray<FRealtimeMeshLODDataRef>::TConstIterator It(LODs); It; ++It)
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
			MarkRenderStateDirty(true);
		}

		return true;
	}

	void FRealtimeMesh::InitializeProxy(FRealtimeMeshProxyCommandBatch& Commands) const
	{
		if (Commands)
		{
			Commands.AddMeshTask([Config = Config](FRealtimeMeshProxy& Proxy)
			{
				Proxy.Reset();
			}, true);

			// Update existing LODs
			for (int32 Index = 0; Index < LODs.Num(); Index++)
			{
				Commands.AddMeshTask([Index](FRealtimeMeshProxy& Proxy)
				{
					Proxy.AddLODIfNotExists(Index);
				}, true /* Always need to dirty render state with this */);
				LODs[Index]->InitializeProxy(Commands);
			}
		}
	}

	FRealtimeMeshProxyRef FRealtimeMesh::CreateRenderProxy(bool bForceRecreate) const
	{
		if (bForceRecreate || !RenderProxy.IsValid())
		{
			FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources->GetGuard());
			RenderProxy = SharedResources->CreateRealtimeMeshProxy();

			FRealtimeMeshProxyCommandBatch Commands(SharedResources->GetOwner());

			InitializeProxy(Commands);

			Commands.ClearProxyRecreate();
			Commands.Commit();
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
