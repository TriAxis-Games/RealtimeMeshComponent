// Copyright 2016-2019 Chris Conway (Koderz). All Rights Reserved.


#include "RuntimeMeshProvider.h"

FRuntimeMeshProviderProxy::FRuntimeMeshProviderProxy(TWeakObjectPtr<URuntimeMeshProvider> InParent)
	: Parent(InParent)
{
}

void FRuntimeMeshProviderProxy::BindPreviousProvider(const IRuntimeMeshProviderProxyPtr& InPreviousProvider)
{
	PreviousProvider = InPreviousProvider;
}


// FRuntimeMeshProviderProxyPassThrough::FRuntimeMeshProviderProxyPassThrough(const IRuntimeMeshProviderProxyRef& InNextProvider)
// 	: NextProvider(InNextProvider)
// {
// }
// 
// void FRuntimeMeshProviderProxyPassThrough::BindPreviousProvider(const IRuntimeMeshProviderProxyPtr& InPreviousProvider)
// {
// 	auto Next = NextProvider.Pin();
// 	if (Next.IsValid())
// 	{
// 		Next->BindPreviousProvider(this->AsShared());
// 	}
// 
// 	FRuntimeMeshProviderProxy::BindPreviousProvider(InPreviousProvider);
// }
// 
// void FRuntimeMeshProviderProxyPassThrough::Initialize()
// {
// 	auto Next = NextProvider.Pin();
// 	if (Next.IsValid())
// 	{
// 		Next->Initialize();
// 	}
// }




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

	virtual bool IsThreadSafe() const override { return false; }
};

IRuntimeMeshProviderProxyRef URuntimeMeshProvider::GetProxy()
{
	IRuntimeMeshProviderProxyRef NewConnector = MakeShared<FRuntimeMeshProviderProxyUObjectProviderConnector, ESPMode::ThreadSafe>(TWeakObjectPtr<URuntimeMeshProvider>(this));
	Proxy = NewConnector;
	return NewConnector;
}
