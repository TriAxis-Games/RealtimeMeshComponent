// Copyright 2016-2020 Chris Conway (Koderz). All Rights Reserved.


#include "RuntimeMeshProvider.h"

FRuntimeMeshProviderProxy::FRuntimeMeshProviderProxy(TWeakObjectPtr<URuntimeMeshProvider> InParent)
	: Parent(InParent)
{
}

void FRuntimeMeshProviderProxy::BindPreviousProvider(const FRuntimeMeshProviderProxyPtr& InPreviousProvider)
{
	PreviousProvider = InPreviousProvider;
}

void FRuntimeMeshProviderProxy::UpdateProxyParameters(URuntimeMeshProvider* ParentProvider, bool bIsInitialSetup)
{
}

void FRuntimeMeshProviderProxy::ConfigureLODs(TArray<FRuntimeMeshLODProperties> LODs)
{
	FRuntimeMeshProviderProxyPtr Pinned = PreviousProvider.Pin();
	if (Pinned.IsValid())
	{
		Pinned->ConfigureLODs(LODs);
	}
}

void FRuntimeMeshProviderProxy::CreateSection(int32 LODIndex, int32 SectionId, const FRuntimeMeshSectionProperties& SectionProperties)
{
	FRuntimeMeshProviderProxyPtr Pinned = PreviousProvider.Pin();
	if (Pinned.IsValid())
	{
		Pinned->CreateSection(LODIndex, SectionId, SectionProperties);
	}
}

void FRuntimeMeshProviderProxy::SetupMaterialSlot(int32 MaterialSlot, FName SlotName, UMaterialInterface* InMaterial)
{
	FRuntimeMeshProviderProxyPtr Pinned = PreviousProvider.Pin();
	if (Pinned.IsValid())
	{
		Pinned->SetupMaterialSlot(MaterialSlot, SlotName, InMaterial);
	}
}

int32 FRuntimeMeshProviderProxy::GetMaterialIndex(FName MaterialSlotName)
{
	FRuntimeMeshProviderProxyPtr Pinned = PreviousProvider.Pin();
	if (Pinned.IsValid())
	{
		return Pinned->GetMaterialIndex(MaterialSlotName);
	}
	return INDEX_NONE;
}

int32 FRuntimeMeshProviderProxy::GetNumMaterials()
{
	FRuntimeMeshProviderProxyPtr Pinned = PreviousProvider.Pin();
	if (Pinned.IsValid())
	{
		return Pinned->GetNumMaterials();
	}
	return 0;
}

TArray<FRuntimeMeshMaterialSlot> FRuntimeMeshProviderProxy::GetMaterialSlots() const
{
	FRuntimeMeshProviderProxyPtr Pinned = PreviousProvider.Pin();
	if (Pinned.IsValid())
	{
		return Pinned->GetMaterialSlots();
	}
	return TArray<FRuntimeMeshMaterialSlot>();
}

void FRuntimeMeshProviderProxy::MarkSectionDirty(int32 LODIndex, int32 SectionId)
{
	FRuntimeMeshProviderProxyPtr Pinned = PreviousProvider.Pin();
	if (Pinned.IsValid())
	{
		Pinned->MarkSectionDirty(LODIndex, SectionId);
	}
}

void FRuntimeMeshProviderProxy::MarkLODDirty(int32 LODIndex)
{
	FRuntimeMeshProviderProxyPtr Pinned = PreviousProvider.Pin();
	if (Pinned.IsValid())
	{
		Pinned->MarkLODDirty(LODIndex);
	}
}

void FRuntimeMeshProviderProxy::MarkAllLODsDirty()
{
	FRuntimeMeshProviderProxyPtr Pinned = PreviousProvider.Pin();
	if (Pinned.IsValid())
	{
		Pinned->MarkAllLODsDirty();
	}
}

void FRuntimeMeshProviderProxy::SetSectionVisibility(int32 LODIndex, int32 SectionId, bool bIsVisible)
{
	FRuntimeMeshProviderProxyPtr Pinned = PreviousProvider.Pin();
	if (Pinned.IsValid())
	{
		Pinned->SetSectionVisibility(LODIndex, SectionId, bIsVisible);
	}
}

void FRuntimeMeshProviderProxy::SetSectionCastsShadow(int32 LODIndex, int32 SectionId, bool bCastsShadow)
{
	FRuntimeMeshProviderProxyPtr Pinned = PreviousProvider.Pin();
	if (Pinned.IsValid())
	{
		Pinned->SetSectionCastsShadow(LODIndex, SectionId, bCastsShadow);
	}
}

void FRuntimeMeshProviderProxy::RemoveSection(int32 LODIndex, int32 SectionId)
{
	FRuntimeMeshProviderProxyPtr Pinned = PreviousProvider.Pin();
	if (Pinned.IsValid())
	{
		Pinned->RemoveSection(LODIndex, SectionId);
	}
}

void FRuntimeMeshProviderProxy::MarkCollisionDirty()
{
	FRuntimeMeshProviderProxyPtr Pinned = PreviousProvider.Pin();
	if (Pinned.IsValid())
	{
		Pinned->MarkCollisionDirty();
	}
}

void FRuntimeMeshProviderProxy::DoOnGameThreadAndBlockThreads(FRuntimeMeshProviderThreadExclusiveFunction Func)
{
	FRuntimeMeshProviderProxyPtr Pinned = PreviousProvider.Pin();
	if (Pinned.IsValid())
	{
		Pinned->DoOnGameThreadAndBlockThreads(Func);
	}
}



FRuntimeMeshProviderProxyPassThrough::FRuntimeMeshProviderProxyPassThrough(TWeakObjectPtr<URuntimeMeshProvider> InParent, const FRuntimeMeshProviderProxyPtr& InNextProvider)
	: FRuntimeMeshProviderProxy(InParent), NextProvider(InNextProvider)
{
}

void FRuntimeMeshProviderProxyPassThrough::BindPreviousProvider(const FRuntimeMeshProviderProxyPtr& InPreviousProvider)
{
	if (NextProvider.IsValid())
	{
		NextProvider->BindPreviousProvider(this->AsShared());
	}

	FRuntimeMeshProviderProxy::BindPreviousProvider(InPreviousProvider);
}

void FRuntimeMeshProviderProxyPassThrough::Initialize()
{
	if (NextProvider.IsValid())
	{
		NextProvider->Initialize();
	}
}

FBoxSphereBounds FRuntimeMeshProviderProxyPassThrough::GetBounds()
{
	if (NextProvider.IsValid())
	{
		return NextProvider->GetBounds();
	}
	return FBoxSphereBounds();
}

bool FRuntimeMeshProviderProxyPassThrough::GetAllSectionsMeshForLOD(int32 LODIndex, TMap<int32, TTuple<FRuntimeMeshSectionProperties, FRuntimeMeshRenderableMeshData>>& MeshDatas)
{
	if (NextProvider.IsValid())
	{
		return NextProvider->GetAllSectionsMeshForLOD(LODIndex, MeshDatas);
	}
	return false;
}

bool FRuntimeMeshProviderProxyPassThrough::GetSectionMeshForLOD(int32 LODIndex, int32 SectionId, FRuntimeMeshRenderableMeshData& MeshData)
{
	if (NextProvider.IsValid())
	{
		return NextProvider->GetSectionMeshForLOD(LODIndex, SectionId, MeshData);
	}
	return false;
}

FRuntimeMeshCollisionSettings FRuntimeMeshProviderProxyPassThrough::GetCollisionSettings()
{
	if (NextProvider.IsValid())
	{
		return NextProvider->GetCollisionSettings();
	}
	return FRuntimeMeshCollisionSettings();
}

bool FRuntimeMeshProviderProxyPassThrough::HasCollisionMesh()
{
	if (NextProvider.IsValid())
	{
		return NextProvider->HasCollisionMesh();
	}
	return false;
}

bool FRuntimeMeshProviderProxyPassThrough::GetCollisionMesh(FRuntimeMeshCollisionData& CollisionData)
{
	if (NextProvider.IsValid())
	{
		return NextProvider->GetCollisionMesh(CollisionData);
	}
	return false;
}

bool FRuntimeMeshProviderProxyPassThrough::IsThreadSafe() const
{
	if (NextProvider.IsValid())
	{
		return NextProvider->IsThreadSafe();

	}
	return false;
}





class RUNTIMEMESHCOMPONENT_API FRuntimeMeshProviderProxyUObjectProviderConnector : public FRuntimeMeshProviderProxy
{

public:
	FRuntimeMeshProviderProxyUObjectProviderConnector(TWeakObjectPtr<URuntimeMeshProvider> InParent)
		: FRuntimeMeshProviderProxy(InParent)
	{

	}

public:
	virtual void Initialize() override
	{
		check(IsInGameThread());
		URuntimeMeshProvider* Temp = Parent.Get();
		if (Temp)
		{
			Temp->Initialize();
		}
	}

	virtual FBoxSphereBounds GetBounds() override
	{
		check(IsInGameThread());
		URuntimeMeshProvider* Temp = Parent.Get();
		if (Temp)
		{
			return Temp->GetBounds();
		}
		return FRuntimeMeshProviderProxy::GetBounds();
	}

	virtual bool GetAllSectionsMeshForLOD(int32 LODIndex, TMap<int32, TTuple<FRuntimeMeshSectionProperties, FRuntimeMeshRenderableMeshData>>& MeshDatas) override
	{
// 		check(IsInGameThread());
// 		URuntimeMeshProvider* Temp = Parent.Get();
// 		if (Temp)
// 		{
// 			return Temp->GetAllSectionsMeshForLOD(LODIndex, MeshDatas);
// 		}
		return false;
	}
	virtual bool GetSectionMeshForLOD(int32 LODIndex, int32 SectionId, FRuntimeMeshRenderableMeshData& MeshData) override
	{
		check(IsInGameThread());
		URuntimeMeshProvider* Temp = Parent.Get();
		if (Temp)
		{
			return Temp->GetSectionMeshForLOD(LODIndex, SectionId, MeshData);
		}
		return false;
	}

	virtual FRuntimeMeshCollisionSettings GetCollisionSettings()
	{
		check(IsInGameThread());
		URuntimeMeshProvider* Temp = Parent.Get();
		if (Temp)
		{
			return Temp->GetCollisionSettings();
		}
		return FRuntimeMeshCollisionSettings();
	}
	virtual bool HasCollisionMesh()
	{
		check(IsInGameThread());
		URuntimeMeshProvider* Temp = Parent.Get();
		if (Temp)
		{
			return Temp->HasCollisionMesh();
		}
		return false;
	}

	virtual bool GetCollisionMesh(FRuntimeMeshCollisionData& CollisionDatas) override
	{
		check(IsInGameThread());
		URuntimeMeshProvider* Temp = Parent.Get();
		if (Temp)
		{
			return Temp->GetCollisionMesh(CollisionDatas);
		}
		return false;
	}


	// UObject based providers are never thread safe
	// So here we'll ignore any result that the actual provider 
	// would have otherwise said and just say no, so that nobody has the brainy 
	// idea of saying yes in the UObject provider and causing issues. :)
	virtual bool IsThreadSafe() const override { return false; }
};



FRuntimeMeshProviderProxyRef URuntimeMeshProvider::GetProxy()
{
	FRuntimeMeshProviderProxyRef NewConnector = MakeShared<FRuntimeMeshProviderProxyUObjectProviderConnector, ESPMode::ThreadSafe>(TWeakObjectPtr<URuntimeMeshProvider>(this));
	Proxy = NewConnector;
	return NewConnector;
}

FRuntimeMeshProviderProxyRef URuntimeMeshProvider::SetupProxy()
{
	FRuntimeMeshProviderProxyRef NewProxy = GetProxy();
	Proxy = NewProxy;

	// We can go ahead and update the parameters once to kick it off.
	NewProxy->UpdateProxyParameters(this, true);
	return NewProxy;
}

void URuntimeMeshProvider::MarkProxyParametersDirty()
{
	// Right now this just blindly updates the parameters
	// This gives us the ability to add the threading support later hopefully without
	// affecting the public API

	if (Proxy.IsValid())
	{
		TWeakObjectPtr<URuntimeMeshProvider> ProviderRef(this);
		FRuntimeMeshProviderProxyPtr ProxyPtr = Proxy;
		Proxy->DoOnGameThreadAndBlockThreads(FRuntimeMeshProviderThreadExclusiveFunction::CreateLambda([ProviderRef, ProxyPtr]()
			{
				if (ProviderRef.IsValid())
				{
					ProxyPtr->UpdateProxyParameters(ProviderRef.Get(), false);
				}
			}));
	}
}

void URuntimeMeshProvider::Initialize_Implementation()
{

}

void URuntimeMeshProvider::ConfigureLODs_Implementation(const TArray<FRuntimeMeshLODProperties>& LODs)
{
	if (Proxy.IsValid())
	{
		Proxy->ConfigureLODs(LODs);
	}
}

void URuntimeMeshProvider::CreateSection_Implementation(int32 LODIndex, int32 SectionId, const FRuntimeMeshSectionProperties& SectionProperties)
{
	if (Proxy.IsValid())
	{
		Proxy->CreateSection(LODIndex, SectionId, SectionProperties);
	}
}

void URuntimeMeshProvider::SetupMaterialSlot_Implementation(int32 MaterialSlot, FName SlotName, UMaterialInterface* InMaterial)
{
	if (Proxy.IsValid())
	{
		Proxy->SetupMaterialSlot(MaterialSlot, SlotName, InMaterial);
	}
}

int32 URuntimeMeshProvider::GetMaterialIndex_Implementation(FName MaterialSlotName)
{
	if (Proxy.IsValid())
	{
		return Proxy->GetMaterialIndex(MaterialSlotName);
	}
	return INDEX_NONE;
}

int32 URuntimeMeshProvider::GetNumMaterialSlots_Implementation()
{
	if (Proxy.IsValid())
	{
		return Proxy->GetNumMaterials();
	}
	return 0;
}

TArray<FRuntimeMeshMaterialSlot> URuntimeMeshProvider::GetMaterialSlots_Implementation()
{
	if (Proxy.IsValid())
	{
		return Proxy->GetMaterialSlots();
	}

	return TArray<FRuntimeMeshMaterialSlot>();
}

void URuntimeMeshProvider::MarkSectionDirty_Implementation(int32 LODIndex, int32 SectionId)
{
	if (Proxy.IsValid())
	{
		Proxy->MarkSectionDirty(LODIndex, SectionId);
	}
}

void URuntimeMeshProvider::MarkLODDirty_Implementation(int32 LODIndex)
{
	if (Proxy.IsValid())
	{
		Proxy->MarkSectionDirty(LODIndex, INDEX_NONE);
	}
}

void URuntimeMeshProvider::MarkAllLODsDirty_Implementation()
{
	if (Proxy.IsValid())
	{
		Proxy->MarkAllLODsDirty();
	}
}

void URuntimeMeshProvider::SetSectionVisibility_Implementation(int32 LODIndex, int32 SectionId, bool bIsVisible)
{
	if (Proxy.IsValid())
	{
		Proxy->SetSectionVisibility(LODIndex, SectionId, bIsVisible);
	}
}

void URuntimeMeshProvider::SetSectionCastsShadow_Implementation(int32 LODIndex, int32 SectionId, bool bCastsShadow)
{
	if (Proxy.IsValid())
	{
		Proxy->SetSectionCastsShadow(LODIndex, SectionId, bCastsShadow);
	}
}

void URuntimeMeshProvider::RemoveSection_Implementation(int32 LODIndex, int32 SectionId)
{
	if (Proxy.IsValid())
	{
		Proxy->RemoveSection(LODIndex, SectionId);
	}
}

void URuntimeMeshProvider::MarkCollisionDirty_Implementation()
{
	if (Proxy.IsValid())
	{
		Proxy->MarkCollisionDirty();
	}
}

FBoxSphereBounds URuntimeMeshProvider::GetBounds_Implementation()
{
	return FBoxSphereBounds();
}

// bool URuntimeMeshProvider::GetAllSectionsMeshForLOD_Implementation(int32 LODIndex, TMap<int32, FRuntimeMeshRenderableMeshData>& MeshDatas)
// {
// 	return false;
// }

bool URuntimeMeshProvider::GetSectionMeshForLOD_Implementation(int32 LODIndex, int32 SectionId, FRuntimeMeshRenderableMeshData& MeshData)
{
	return false;
}

FRuntimeMeshCollisionSettings URuntimeMeshProvider::GetCollisionSettings_Implementation()
{
	return FRuntimeMeshCollisionSettings();
}

bool URuntimeMeshProvider::HasCollisionMesh_Implementation()
{
	return false;
}

bool URuntimeMeshProvider::GetCollisionMesh_Implementation(FRuntimeMeshCollisionData& CollisionData)
{
	return false;
}

