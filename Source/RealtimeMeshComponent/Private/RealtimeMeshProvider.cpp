// Copyright 2016-2019 Chris Conway (Koderz). All Rights Reserved.


#include "RealtimeMeshProvider.h"

FRealtimeMeshProviderProxy::FRealtimeMeshProviderProxy(TWeakObjectPtr<URealtimeMeshProvider> InParent)
	: Parent(InParent)
{
}

void FRealtimeMeshProviderProxy::BindPreviousProvider(const IRealtimeMeshProviderProxyPtr& InPreviousProvider)
{
	PreviousProvider = InPreviousProvider;
}


// FRealtimeMeshProviderProxyPassThrough::FRealtimeMeshProviderProxyPassThrough(const IRealtimeMeshProviderProxyRef& InNextProvider)
// 	: NextProvider(InNextProvider)
// {
// }
// 
// void FRealtimeMeshProviderProxyPassThrough::BindPreviousProvider(const IRealtimeMeshProviderProxyPtr& InPreviousProvider)
// {
// 	auto Next = NextProvider.Pin();
// 	if (Next.IsValid())
// 	{
// 		Next->BindPreviousProvider(this->AsShared());
// 	}
// 
// 	FRealtimeMeshProviderProxy::BindPreviousProvider(InPreviousProvider);
// }
// 
// void FRealtimeMeshProviderProxyPassThrough::Initialize()
// {
// 	auto Next = NextProvider.Pin();
// 	if (Next.IsValid())
// 	{
// 		Next->Initialize();
// 	}
// }




class REALTIMEMESHCOMPONENT_API FRealtimeMeshProviderProxyUObjectProviderConnector : public FRealtimeMeshProviderProxy
{

public:
	FRealtimeMeshProviderProxyUObjectProviderConnector(TWeakObjectPtr<URealtimeMeshProvider> InParent)
		: FRealtimeMeshProviderProxy(InParent)
	{

	}

public:
	virtual void Initialize() override
	{
		check(IsInGameThread());
		URealtimeMeshProvider* Temp = Parent.Get();
		if (Temp)
		{
			Temp->Initialize();
		}
	}

	virtual FBoxSphereBounds GetBounds() override
	{
		check(IsInGameThread());
		URealtimeMeshProvider* Temp = Parent.Get();
		if (Temp)
		{
			return Temp->GetBounds();
		}
		return FRealtimeMeshProviderProxy::GetBounds();
	}

	virtual bool GetSectionMeshForLOD(uint8 LODIndex, int32 SectionId, FRealtimeMeshRenderableMeshData& MeshData) override
	{
		check(IsInGameThread());
		URealtimeMeshProvider* Temp = Parent.Get();
		if (Temp)
		{
			return Temp->GetSectionMeshForLOD(LODIndex, SectionId, MeshData);
		}
		return false;
	}

	virtual bool GetCollisionMesh(FRealtimeMeshCollisionData& CollisionDatas) override
	{
		check(IsInGameThread());
		URealtimeMeshProvider* Temp = Parent.Get();
		if (Temp)
		{
			return Temp->GetCollisionMesh(CollisionDatas);
		}
		return false;
	}

	virtual bool IsThreadSafe() const override { return false; }
};

IRealtimeMeshProviderProxyRef URealtimeMeshProvider::GetProxy()
{
	IRealtimeMeshProviderProxyRef NewConnector = MakeShared<FRealtimeMeshProviderProxyUObjectProviderConnector, ESPMode::ThreadSafe>(TWeakObjectPtr<URealtimeMeshProvider>(this));
	Proxy = NewConnector;
	return NewConnector;
}
