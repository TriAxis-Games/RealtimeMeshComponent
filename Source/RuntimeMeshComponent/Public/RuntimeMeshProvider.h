// Copyright 2016-2019 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RuntimeMeshRenderable.h"
#include "RuntimeMeshCollision.h"
#include "RuntimeMeshProvider.generated.h"


class IRuntimeMeshProviderProxy;
using IRuntimeMeshProviderProxyRef = TSharedRef<IRuntimeMeshProviderProxy, ESPMode::ThreadSafe>;
using IRuntimeMeshProviderProxyPtr = TSharedPtr<IRuntimeMeshProviderProxy, ESPMode::ThreadSafe>;
using IRuntimeMeshProviderProxyWeakPtr = TWeakPtr<IRuntimeMeshProviderProxy, ESPMode::ThreadSafe>;


class RUNTIMEMESHCOMPONENT_API IRuntimeMeshProviderProxy : public TSharedFromThis<IRuntimeMeshProviderProxy, ESPMode::ThreadSafe>
{
public:
	IRuntimeMeshProviderProxy() { }
	virtual ~IRuntimeMeshProviderProxy() { }

protected:
	virtual void BindPreviousProvider(const IRuntimeMeshProviderProxyPtr& InPreviousProvider) { }

public:
	virtual void Initialize() { };

	virtual void ConfigureLOD(uint8 LODIndex, const FRuntimeMeshLODProperties& LODProperties) { }
	virtual void CreateSection(uint8 LODIndex, int32 SectionId, const FRuntimeMeshSectionProperties& SectionProperties) { }
	virtual void SetupMaterialSlot(int32 MaterialSlot, FName SlotName, UMaterialInterface* InMaterial) { }
	virtual void MarkSectionDirty(uint8 LODIndex, int32 SectionId) { }
	virtual void SetSectionVisibility(uint8 LODIndex, int32 SectionId, bool bIsVisible) { }
	virtual void SetSectionCastsShadow(uint8 LODIndex, int32 SectionId, bool bCastsShadow) { }
	virtual void RemoveSection(uint8 LODIndex, int32 SectionId) { }
	virtual void MarkCollisionDirty() { }


	virtual FBoxSphereBounds GetBounds() { return FBoxSphereBounds(); }

	virtual bool GetSectionMeshForLOD(uint8 LODIndex, int32 SectionId, FRuntimeMeshRenderableMeshData& MeshData) { return false; }

	virtual FRuntimeMeshCollisionSettings GetCollisionSettings() { return FRuntimeMeshCollisionSettings(); }
	virtual bool HasCollisionMesh() { return false; }
	virtual bool GetCollisionMesh(FRuntimeMeshCollisionData& CollisionData) { return false; }
	   
	virtual bool IsThreadSafe() const { return false; }

private:
	friend class FRuntimeMeshData;
	friend class FRuntimeMeshProviderProxyPassThrough;
};

/**
 *
 */
class RUNTIMEMESHCOMPONENT_API FRuntimeMeshProviderProxy : public IRuntimeMeshProviderProxy
{
protected:
	TWeakObjectPtr<URuntimeMeshProvider> Parent;
	IRuntimeMeshProviderProxyPtr PreviousProvider;

public:
	FRuntimeMeshProviderProxy(TWeakObjectPtr<URuntimeMeshProvider> InParent);

protected:
	virtual void BindPreviousProvider(const IRuntimeMeshProviderProxyPtr& InPreviousProvider) override;

public:
	virtual void ConfigureLOD(uint8 LODIndex, const FRuntimeMeshLODProperties& LODProperties) override
	{
		if (PreviousProvider.IsValid())
		{
			PreviousProvider->ConfigureLOD(LODIndex, LODProperties);
		}
	}

	virtual void CreateSection(uint8 LODIndex, int32 SectionId, const FRuntimeMeshSectionProperties& SectionProperties) override
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


// class RUNTIMEMESHCOMPONENT_API FRuntimeMeshProviderProxyPassThrough : public FRuntimeMeshProviderProxy
// {
// protected:
// 	IRuntimeMeshProviderProxyWeakPtr NextProvider;
// 
// public:
// 	FRuntimeMeshProviderProxyPassThrough(const IRuntimeMeshProviderProxyRef& InNextProvider);
// 
// protected:
// 	virtual void BindPreviousProvider(const IRuntimeMeshProviderProxyPtr& InPreviousProvider) override;
// 
// public:
// 	virtual void Initialize() override;
// 
// 	virtual bool GetSectionMeshForLOD(uint8 LODIndex, int32 SectionId, FRuntimeMeshRenderableMeshData& MeshData) override
// 	{
// 		auto Temp = NextProvider.Pin();
// 		if (Temp.IsValid())
// 		{
// 			return Temp->GetSectionMeshForLOD(LODIndex, SectionId, MeshData);
// 		}
// 		return false;
// 	}
// 
// 	virtual bool GetCollisionMesh(FRuntimeMeshCollisionVertexStream& CollisionVertices, FRuntimeMeshCollisionTriangleStream& CollisionTriangles) override
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
class RUNTIMEMESHCOMPONENT_API URuntimeMeshProvider : public UObject
{
	GENERATED_BODY()

protected:
	IRuntimeMeshProviderProxyPtr Proxy;


public:
	virtual IRuntimeMeshProviderProxyRef GetProxy();

	virtual void Initialize() { };



	virtual void ConfigureLOD(uint8 LODIndex, const FRuntimeMeshLODProperties& LODProperties)
	{
		if (Proxy.IsValid())
		{
			Proxy->ConfigureLOD(LODIndex, LODProperties);
		}
	}

	virtual void CreateSection(uint8 LODIndex, int32 SectionId, const FRuntimeMeshSectionProperties& SectionProperties)
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

	virtual bool GetSectionMeshForLOD(uint8 LODIndex, int32 SectionId, FRuntimeMeshRenderableMeshData& MeshData) { return false; }
	virtual bool GetCollisionMesh(FRuntimeMeshCollisionData& CollisionData)
	{
		return false;
	}


};