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

bool FRuntimeMeshProviderProxyPassThrough::GetSectionMeshForLOD(uint8 LODIndex, int32 SectionId, FRuntimeMeshRenderableMeshData& MeshData)
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

	virtual bool GetSectionMeshForLOD(uint8 LODIndex, int32 SectionId, FRuntimeMeshRenderableMeshData& MeshData) override
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
	virtual bool IsThreadSafe() const override { return false; }
};

FRuntimeMeshProviderProxyRef URuntimeMeshProvider::GetProxy()
{
	FRuntimeMeshProviderProxyRef NewConnector = MakeShared<FRuntimeMeshProviderProxyUObjectProviderConnector, ESPMode::ThreadSafe>(TWeakObjectPtr<URuntimeMeshProvider>(this));
	Proxy = NewConnector;
	return NewConnector;
}

