// Copyright 2016-2019 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RealtimeMeshRenderable.h"
#include "RealtimeMeshCollision.h"
#include "RealtimeMeshProvider.generated.h"


class IRealtimeMeshProviderProxy;
using IRealtimeMeshProviderProxyRef = TSharedRef<IRealtimeMeshProviderProxy, ESPMode::ThreadSafe>;
using IRealtimeMeshProviderProxyPtr = TSharedPtr<IRealtimeMeshProviderProxy, ESPMode::ThreadSafe>;
using IRealtimeMeshProviderProxyWeakPtr = TWeakPtr<IRealtimeMeshProviderProxy, ESPMode::ThreadSafe>;


class REALTIMEMESHCOMPONENT_API IRealtimeMeshProviderProxy : public TSharedFromThis<IRealtimeMeshProviderProxy, ESPMode::ThreadSafe>
{
public:
	IRealtimeMeshProviderProxy() { }
	virtual ~IRealtimeMeshProviderProxy() { }

protected:
	virtual void BindPreviousProvider(const IRealtimeMeshProviderProxyPtr& InPreviousProvider) { }

public:
	virtual void Initialize() { };

	virtual void ConfigureLOD(uint8 LODIndex, const FRealtimeMeshLODProperties& LODProperties) { }
	virtual void CreateSection(uint8 LODIndex, int32 SectionId, const FRealtimeMeshSectionProperties& SectionProperties) { }
	virtual void SetupMaterialSlot(int32 MaterialSlot, FName SlotName, UMaterialInterface* InMaterial) { }
	virtual void MarkSectionDirty(uint8 LODIndex, int32 SectionId) { }
	virtual void SetSectionVisibility(uint8 LODIndex, int32 SectionId, bool bIsVisible) { }
	virtual void SetSectionCastsShadow(uint8 LODIndex, int32 SectionId, bool bCastsShadow) { }
	virtual void RemoveSection(uint8 LODIndex, int32 SectionId) { }
	virtual void MarkCollisionDirty() { }


	virtual FBoxSphereBounds GetBounds() { return FBoxSphereBounds(); }

	virtual bool GetSectionMeshForLOD(uint8 LODIndex, int32 SectionId, FRealtimeMeshRenderableMeshData& MeshData) { return false; }

	virtual FRealtimeMeshCollisionSettings GetCollisionSettings() { return FRealtimeMeshCollisionSettings(); }
	virtual bool HasCollisionMesh() { return false; }
	virtual bool GetCollisionMesh(FRealtimeMeshCollisionData& CollisionData) { return false; }
	   
	virtual bool IsThreadSafe() const { return false; }

private:
	friend class FRealtimeMeshData;
	friend class FRealtimeMeshProviderProxyPassThrough;
};

/**
 *
 */
class REALTIMEMESHCOMPONENT_API FRealtimeMeshProviderProxy : public IRealtimeMeshProviderProxy
{
protected:
	TWeakObjectPtr<URealtimeMeshProvider> Parent;
	IRealtimeMeshProviderProxyPtr PreviousProvider;

public:
	FRealtimeMeshProviderProxy(TWeakObjectPtr<URealtimeMeshProvider> InParent);

protected:
	virtual void BindPreviousProvider(const IRealtimeMeshProviderProxyPtr& InPreviousProvider) override;

public:
	virtual void ConfigureLOD(uint8 LODIndex, const FRealtimeMeshLODProperties& LODProperties) override
	{
		if (PreviousProvider.IsValid())
		{
			PreviousProvider->ConfigureLOD(LODIndex, LODProperties);
		}
	}

	virtual void CreateSection(uint8 LODIndex, int32 SectionId, const FRealtimeMeshSectionProperties& SectionProperties) override
	{
		if (PreviousProvider.IsValid())
		{
			PreviousProvider->CreateSection(LODIndex, SectionId, SectionProperties);
		}
	}

	virtual void SetupMaterialSlot(int32 MaterialSlot, FName SlotName, UMaterialInterface* InMaterial) override
	{
		if (PreviousProvider.IsValid())
		{
			PreviousProvider->SetupMaterialSlot(MaterialSlot, SlotName, InMaterial);
		}
	}

	virtual void MarkSectionDirty(uint8 LODIndex, int32 SectionId) override
	{
		if (PreviousProvider.IsValid())
		{
			PreviousProvider->MarkSectionDirty(LODIndex, SectionId);
		}
	}

	virtual void SetSectionVisibility(uint8 LODIndex, int32 SectionId, bool bIsVisible) override
	{
		if (PreviousProvider.IsValid())
		{
			PreviousProvider->SetSectionVisibility(LODIndex, SectionId, bIsVisible);
		}
	}

	virtual void SetSectionCastsShadow(uint8 LODIndex, int32 SectionId, bool bCastsShadow) override
	{
		if (PreviousProvider.IsValid())
		{
			PreviousProvider->SetSectionCastsShadow(LODIndex, SectionId, bCastsShadow);
		}
	}

	virtual void RemoveSection(uint8 LODIndex, int32 SectionId) override
	{
		if (PreviousProvider.IsValid())
		{
			PreviousProvider->RemoveSection(LODIndex, SectionId);
		}
	}

	virtual void MarkCollisionDirty() override
	{
		if (PreviousProvider.IsValid())
		{
			PreviousProvider->MarkCollisionDirty();
		}
	}
};


// class REALTIMEMESHCOMPONENT_API FRealtimeMeshProviderProxyPassThrough : public FRealtimeMeshProviderProxy
// {
// protected:
// 	IRealtimeMeshProviderProxyWeakPtr NextProvider;
// 
// public:
// 	FRealtimeMeshProviderProxyPassThrough(const IRealtimeMeshProviderProxyRef& InNextProvider);
// 
// protected:
// 	virtual void BindPreviousProvider(const IRealtimeMeshProviderProxyPtr& InPreviousProvider) override;
// 
// public:
// 	virtual void Initialize() override;
// 
// 	virtual bool GetSectionMeshForLOD(uint8 LODIndex, int32 SectionId, FRealtimeMeshRenderableMeshData& MeshData) override
// 	{
// 		auto Temp = NextProvider.Pin();
// 		if (Temp.IsValid())
// 		{
// 			return Temp->GetSectionMeshForLOD(LODIndex, SectionId, MeshData);
// 		}
// 		return false;
// 	}
// 
// 	virtual bool GetCollisionMesh(FRealtimeMeshCollisionVertexStream& CollisionVertices, FRealtimeMeshCollisionTriangleStream& CollisionTriangles) override
// 	{
// 		auto Temp = NextProvider.Pin();
// 		if (Temp.IsValid())
// 		{
// 			return Temp->GetCollisionMesh(CollisionVertices, CollisionTriangles);
// 
// 		}
// 		return false;
// 	}
// 
// }; 



UCLASS(Abstract, HideCategories = Object, BlueprintType)
class REALTIMEMESHCOMPONENT_API URealtimeMeshProvider : public UObject
{
	GENERATED_BODY()

protected:
	IRealtimeMeshProviderProxyPtr Proxy;


public:
	virtual IRealtimeMeshProviderProxyRef GetProxy();

	virtual void Initialize() { };



	virtual void ConfigureLOD(uint8 LODIndex, const FRealtimeMeshLODProperties& LODProperties)
	{
		if (Proxy.IsValid())
		{
			Proxy->ConfigureLOD(LODIndex, LODProperties);
		}
	}

	virtual void CreateSection(uint8 LODIndex, int32 SectionId, const FRealtimeMeshSectionProperties& SectionProperties)
	{
		if (Proxy.IsValid())
		{
			Proxy->CreateSection(LODIndex, SectionId, SectionProperties);
		}
	}

	virtual void SetupMaterialSlot(int32 MaterialSlot, FName SlotName, UMaterialInterface* InMaterial)
	{
		if (Proxy.IsValid())
		{
			Proxy->SetupMaterialSlot(MaterialSlot, SlotName, InMaterial);
		}		
	}

	virtual void MarkSectionDirty(uint8 LODIndex, int32 SectionId)
	{
		if (Proxy.IsValid())
		{
			Proxy->MarkSectionDirty(LODIndex, SectionId);
		}
	}

	virtual void SetSectionVisibility(uint8 LODIndex, int32 SectionId, bool bIsVisible)
	{
		if (Proxy.IsValid())
		{
			Proxy->SetSectionVisibility(LODIndex, SectionId, bIsVisible);
		}
	}

	virtual void SetSectionCastsShadow(uint8 LODIndex, int32 SectionId, bool bCastsShadow)
	{
		if (Proxy.IsValid())
		{
			Proxy->SetSectionCastsShadow(LODIndex, SectionId, bCastsShadow);
		}
	}

	virtual void RemoveSection(uint8 LODIndex, int32 SectionId)
	{
		if (Proxy.IsValid())
		{
			Proxy->RemoveSection(LODIndex, SectionId);
		}
	}

	virtual void MarkCollisionDirty()
	{
		if (Proxy.IsValid())
		{
			Proxy->MarkCollisionDirty();
		}
	}

	   	  
	virtual FBoxSphereBounds GetBounds() { return FBoxSphereBounds(); }

	virtual bool GetSectionMeshForLOD(uint8 LODIndex, int32 SectionId, FRealtimeMeshRenderableMeshData& MeshData) { return false; }
	virtual bool GetCollisionMesh(FRealtimeMeshCollisionData& CollisionData)
	{
		return false;
	}


};