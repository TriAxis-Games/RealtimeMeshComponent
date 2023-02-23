// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#include "Data/RealtimeMeshData.h"

#include "ContentBrowserPluginFilters.h"
#include "Data/RealtimeMeshLOD.h"
#include "Data/RealtimeMeshSectionGroup.h"
#include "RenderProxy/RealtimeMeshProxy.h"

#define LOCTEXT_NAMESPACE "RealtimeMesh"

namespace RealtimeMesh
{	
	FRealtimeMesh::FRealtimeMesh(const FRealtimeMeshClassFactoryRef& InClassFactory)
		: ClassFactory(InClassFactory)
		, LocalBounds(FVector3f::ZeroVector, FVector3f::OneVector, 1)
		, bIsCollisionDirty(true)
	{
		TypeName = "RealtimeMesh-Base";
	}

	int32 FRealtimeMesh::GetNumLODs() const
	{
		FReadScopeLock Lock(RenderDataLock);
		return LODs.Num();
	}

	FRealtimeMeshLODDataPtr FRealtimeMesh::GetLOD(FRealtimeMeshLODKey LODKey) const
	{
		FReadScopeLock Lock(RenderDataLock);
		return LODs.IsValidIndex(FRealtimeMeshKeyHelpers::GetLODIndex(LODKey))? LODs[FRealtimeMeshKeyHelpers::GetLODIndex(LODKey)] : FRealtimeMeshLODDataPtr();
	}

	FRealtimeMeshSectionGroupPtr FRealtimeMesh::GetSectionGroup(FRealtimeMeshSectionGroupKey SectionGroupKey) const
	{
		if (const FRealtimeMeshLODDataPtr LOD = GetLOD(SectionGroupKey.GetLODKey()))
		{
			return LOD->GetSectionGroup(SectionGroupKey);
		}
		return nullptr;
	}
	
	FRealtimeMeshSectionDataPtr FRealtimeMesh::GetSection(FRealtimeMeshSectionKey SectionKey) const
	{
		if (const FRealtimeMeshLODDataPtr LOD = GetLOD(SectionKey.GetLODKey()))
		{
			if (const FRealtimeMeshSectionGroupPtr SectionGroup = LOD->GetSectionGroup(SectionKey.GetSectionGroupKey()))
			{
				return SectionGroup->GetSection(SectionKey);
			}
		}
		return nullptr;
	}

	FBoxSphereBounds3f FRealtimeMesh::GetLocalBounds() const
	{
		FReadScopeLock ScopeLock(BoundsLock);
		return LocalBounds;
	}

	FRealtimeMeshCollisionConfiguration FRealtimeMesh::GetCollisionConfig() const
	{
		return CollisionConfig;
	}

	void FRealtimeMesh::SetCollisionConfig(const FRealtimeMeshCollisionConfiguration& InCollisionConfig)
	{
		CollisionConfig = InCollisionConfig;
		CollisionDataUpdatedEvent.Broadcast(this->AsShared());
	}

	FRealtimeMeshSimpleGeometry FRealtimeMesh::GetSimpleGeometry() const
	{
		return SimpleGeometry;
	}

	void FRealtimeMesh::SetSimpleGeometry(const FRealtimeMeshSimpleGeometry& InSimpleGeometry)
	{
		SimpleGeometry = InSimpleGeometry;
		CollisionDataUpdatedEvent.Broadcast(this->AsShared());
	}

	void FRealtimeMesh::InitializeLODs(const TFixedLODArray<FRealtimeMeshLODConfig>& InLODConfigs)
	{
		if (InLODConfigs.Num() == 0)
		{
			FMessageLog("RealtimeMesh").Error(LOCTEXT("RealtimeMeshLODCountError", "RealtimeMesh must have at least one LOD"));
			return;
		}
				
		FRWScopeLockEx Lock(RenderDataLock, SLT_Write);

		check(InLODConfigs.Num() > 0);
		LODs.Empty(InLODConfigs.Num());
		for (int32 Index = 0; Index < InLODConfigs.Num(); Index++)
		{
			LODs.Add(ClassFactory->CreateLOD(this->AsShared(), Index, InLODConfigs[Index]));
			LODs[Index]->OnBoundsUpdated().AddThreadSafeSP(this->AsShared(), &FRealtimeMesh::HandleLODBoundsChanged);
		}
		
		Lock.Release();		

		if (RenderProxy)
		{
			CreateRenderProxy(true);
		}
	}

	FRealtimeMeshLODKey FRealtimeMesh::AddLOD(const FRealtimeMeshLODConfig& LODConfig)
	{		
		FRWScopeLockEx Lock(RenderDataLock, SLT_Write);

		if (LODs.Num() >= REALTIME_MESH_MAX_LODS)
		{
			FMessageLog("RealtimeMesh").Error(FText::Format(LOCTEXT("RealtimeMeshLODCountError", "RealtimeMesh must have at most {0} LODs"),
				FText::AsNumber(REALTIME_MESH_MAX_LODS)));
			return FRealtimeMeshLODKey();
		}
		
		FRealtimeMeshLODKey NewLODKey = LODs.Add(ClassFactory->CreateLOD(this->AsShared(), LODs.Num(), LODConfig));
		FRealtimeMeshLODProxyInitializationParametersRef LODInitParams = LODs[FRealtimeMeshKeyHelpers::GetLODIndex(NewLODKey)]->GetInitializationParams();
		
		Lock.Release();
		
		DoOnRenderProxy([NewLODKey, LODInitParams](const FRealtimeMeshProxyRef& Proxy)
		{
			Proxy->AddLOD(NewLODKey, LODInitParams);
		});

		MarkRenderStateDirty(true);

		return NewLODKey;
	}

	void FRealtimeMesh::RemoveTrailingLOD()
	{
		FRWScopeLockEx Lock(RenderDataLock, SLT_Write);

		if (LODs.Num() < 2)
		{
			FMessageLog("RealtimeMesh").Error(LOCTEXT("RealtimeMeshLODCountError", "RealtimeMesh must have at least one LOD"));
			return;
		}
		
		LODs.RemoveAt(LODs.Num() - 1);
		
		Lock.Release();
		
		DoOnRenderProxy([](const FRealtimeMeshProxyRef& Proxy)
		{
			Proxy->RemoveTrailingLOD();
		});

		MarkRenderStateDirty(true);
	}

	bool FRealtimeMesh::HasRenderProxy() const
	{
		FReadScopeLock ReadLock(RenderDataLock);
		return RenderProxy.IsValid();
	}

	FRealtimeMeshProxyPtr FRealtimeMesh::GetRenderProxy(bool bCreateIfNotExists) const
	{
		FRWScopeLockEx Lock(RenderDataLock, SLT_ReadOnly);
		if (RenderProxy.IsValid() || !bCreateIfNotExists)
		{
			return RenderProxy;
		}
		Lock.Release();

		// We hold this lock the entire time we initialize proxy so any proxy calls get delayed until after we grab the starting state
		Lock.Lock(SLT_Write);
		CreateRenderProxy();
		
		return RenderProxy;
	}

	void FRealtimeMesh::Reset()
	{
		FWriteScopeLock ScopeLock(RenderDataLock);

		if (RenderProxy)
		{
			RenderProxy.Reset();
		}
		
		MarkRenderStateDirty(true);
	}

	FRealtimeMeshProxyInitializationParametersRef FRealtimeMesh::GetInitializationParams() const
	{
		auto InitParams = MakeShared<FRealtimeMeshProxyInitializationParameters>();
		InitParams->LODs.Reserve(LODs.Num());

		for (int32 Index = 0; Index < LODs.Num(); Index++)
		{
			InitParams->LODs.Add(LODs[Index]->GetInitializationParams());
		}

		return InitParams;
	}

	void FRealtimeMesh::MarkCollisionDirty()
	{
		bIsCollisionDirty = true;
		CollisionDataUpdatedEvent.Broadcast(this->AsShared());
	}

	void FRealtimeMesh::DoOnRenderProxy(TUniqueFunction<void(const FRealtimeMeshProxyRef&)>&& Function) const
	{
		if (RenderProxy)
		{
			RenderProxy->EnqueueRenderingCommand(Forward<TUniqueFunction<void(const FRealtimeMeshProxyRef&)>>(Function));
		}
	}

	bool FRealtimeMesh::Serialize(FArchive& Ar)
	{
		FWriteScopeLock ScopeLock(RenderDataLock);
		
		int32 NumLODs = LODs.Num();
		Ar << NumLODs;

		if (Ar.IsLoading())			
		{
			LODs.Empty(NumLODs);
			for (int32 Index = 0; Index < NumLODs; Index++)
			{
				LODs.Add(ClassFactory->CreateLOD(this->AsShared(), FRealtimeMeshLODKey(Index), FRealtimeMeshLODConfig()));
				LODs[Index]->Serialize(Ar);			
				LODs[Index]->OnBoundsUpdated().AddThreadSafeSP(this->AsShared(), &FRealtimeMesh::HandleLODBoundsChanged);
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
		Ar << LocalBounds;
		Ar << SimpleGeometry;
		Ar << CollisionConfig;

		if (Ar.IsLoading() && RenderProxy)
		{
			CreateRenderProxy(true);
			MarkRenderStateDirty(true);
			MarkCollisionDirty();
		}
		
		return true;
	}
	
	void FRealtimeMesh::CreateRenderProxy(bool bForceRecreate) const
	{
		if (bForceRecreate || !RenderProxy.IsValid())
		{
			RenderProxy = ClassFactory->CreateRealtimeMeshProxy(const_cast<FRealtimeMesh*>(this)->AsShared());

			auto InitParams = GetInitializationParams();
			RenderProxy->EnqueueRenderingCommand([InitParams](const FRealtimeMeshProxyRef& Proxy)
			{
				Proxy->InitializeRenderThreadResources(InitParams);
			});
		}
	}

	void FRealtimeMesh::UpdateBounds()
	{
		{
			FReadScopeLock RenderScopeLock(RenderDataLock);
			FWriteScopeLock BoundsScopeLock(BoundsLock);
				
			FBoxSphereBounds3f Bounds;
			bool bIsFirst = true;

			// Combine the bounds from all the LODs
			for (const auto& LOD : LODs)
			{
				if (LOD->HasSectionGroups())
				{
					Bounds = bIsFirst? LOD->GetLocalBounds() : LOD->GetLocalBounds() + Bounds;
				}
				bIsFirst = false;
			}

			if (bIsFirst)
			{
				// Fallback so we don't have a zero size bounds
				Bounds = FBoxSphereBounds3f(FSphere3f(FVector3f::ZeroVector, 1));
			}

			LocalBounds = Bounds;
		}

		BroadcastBoundsChangedEvent();		
	}

	void FRealtimeMesh::HandleLODBoundsChanged(const FRealtimeMeshLODDataRef& LOD)
	{
		UpdateBounds();
	}
}

#undef LOCTEXT_NAMESPACE