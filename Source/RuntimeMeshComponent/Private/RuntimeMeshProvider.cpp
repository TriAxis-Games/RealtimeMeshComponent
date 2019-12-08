// Copyright 2016-2019 Chris Conway (Koderz). All Rights Reserved.


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

void FRuntimeMeshProviderProxy::ConfigureLOD(int32 LODIndex, const FRuntimeMeshLODProperties& LODProperties)
{
	if (PreviousProvider.IsValid())
	{
		PreviousProvider->ConfigureLOD(LODIndex, LODProperties);
	}
}

void FRuntimeMeshProviderProxy::CreateSection(int32 LODIndex, int32 SectionId, const FRuntimeMeshSectionProperties& SectionProperties)
{
	if (PreviousProvider.IsValid())
	{
		PreviousProvider->CreateSection(LODIndex, SectionId, SectionProperties);
	}
}

void FRuntimeMeshProviderProxy::SetupMaterialSlot(int32 MaterialSlot, FName SlotName, UMaterialInterface* InMaterial)
{
	if (PreviousProvider.IsValid())
	{
		PreviousProvider->SetupMaterialSlot(MaterialSlot, SlotName, InMaterial);
	}
}

void FRuntimeMeshProviderProxy::MarkSectionDirty(int32 LODIndex, int32 SectionId)
{
	if (PreviousProvider.IsValid())
	{
		PreviousProvider->MarkSectionDirty(LODIndex, SectionId);
	}
}

void FRuntimeMeshProviderProxy::SetSectionVisibility(int32 LODIndex, int32 SectionId, bool bIsVisible)
{
	if (PreviousProvider.IsValid())
	{
		PreviousProvider->SetSectionVisibility(LODIndex, SectionId, bIsVisible);
	}
}

void FRuntimeMeshProviderProxy::SetSectionCastsShadow(int32 LODIndex, int32 SectionId, bool bCastsShadow)
{
	if (PreviousProvider.IsValid())
	{
		PreviousProvider->SetSectionCastsShadow(LODIndex, SectionId, bCastsShadow);
	}
}

void FRuntimeMeshProviderProxy::RemoveSection(int32 LODIndex, int32 SectionId)
{
	if (PreviousProvider.IsValid())
	{
		PreviousProvider->RemoveSection(LODIndex, SectionId);
	}
}

void FRuntimeMeshProviderProxy::MarkCollisionDirty()
{
	if (PreviousProvider.IsValid())
	{
		PreviousProvider->MarkCollisionDirty();
	}
}




FRuntimeMeshProviderProxyPassThrough::FRuntimeMeshProviderProxyPassThrough(TWeakObjectPtr<URuntimeMeshProvider> InParent, const FRuntimeMeshProviderProxyPtr& InNextProvider)
	: FRuntimeMeshProviderProxy(InParent), NextProvider(InNextProvider)
{
}

void FRuntimeMeshProviderProxyPassThrough::BindPreviousProvider(const FRuntimeMeshProviderProxyPtr& InPreviousProvider)
{
	auto Next = NextProvider.Pin();
	if (Next.IsValid())
	{
		Next->BindPreviousProvider(this->AsShared());
	}

	FRuntimeMeshProviderProxy::BindPreviousProvider(InPreviousProvider);
}

void FRuntimeMeshProviderProxyPassThrough::Initialize()
{
	auto Next = NextProvider.Pin();
	if (Next.IsValid())
	{
		Next->Initialize();
	}
}

FBoxSphereBounds FRuntimeMeshProviderProxyPassThrough::GetBounds()
{
	auto Temp = NextProvider.Pin();
	if (Temp.IsValid())
	{
		return Temp->GetBounds();
	}
	return FBoxSphereBounds();
}

bool FRuntimeMeshProviderProxyPassThrough::GetSectionMeshForLOD(int32 LODIndex, int32 SectionId, FRuntimeMeshRenderableMeshData& MeshData)
{
	auto Temp = NextProvider.Pin();
	if (Temp.IsValid())
	{
		return Temp->GetSectionMeshForLOD(LODIndex, SectionId, MeshData);
	}
	return false;
}

FRuntimeMeshCollisionSettings FRuntimeMeshProviderProxyPassThrough::GetCollisionSettings()
{
	auto Temp = NextProvider.Pin();
	if (Temp.IsValid())
	{
		return Temp->GetCollisionSettings();
	}
	return FRuntimeMeshCollisionSettings();
}

bool FRuntimeMeshProviderProxyPassThrough::HasCollisionMesh()
{
	auto Temp = NextProvider.Pin();
	if (Temp.IsValid())
	{
		return Temp->HasCollisionMesh();
	}
	return false;
}

bool FRuntimeMeshProviderProxyPassThrough::GetCollisionMesh(FRuntimeMeshCollisionData& CollisionData)
{
	auto Temp = NextProvider.Pin();
	if (Temp.IsValid())
	{
		return Temp->GetCollisionMesh(CollisionData);
	}
	return false;
}

bool FRuntimeMeshProviderProxyPassThrough::IsThreadSafe() const
{
	auto Temp = NextProvider.Pin();
	if (Temp.IsValid())
	{
		return Temp->IsThreadSafe();

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
		Proxy->UpdateProxyParameters(this, false);
	}
}

void URuntimeMeshProvider::Initialize_Implementation()
{

}

void URuntimeMeshProvider::ConfigureLOD_Implementation(int32 LODIndex, const FRuntimeMeshLODProperties& LODProperties)
{
	if (Proxy.IsValid())
	{
		Proxy->ConfigureLOD(LODIndex, LODProperties);
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

void URuntimeMeshProvider::MarkSectionDirty_Implementation(int32 LODIndex, int32 SectionId)
{
	if (Proxy.IsValid())
	{
		Proxy->MarkSectionDirty(LODIndex, SectionId);
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

