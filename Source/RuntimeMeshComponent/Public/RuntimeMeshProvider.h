// Copyright 2016-2019 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RuntimeMeshRenderable.h"
#include "RuntimeMeshCollision.h"
#include "RuntimeMeshProvider.generated.h"


class FRuntimeMeshProviderProxy;
using FRuntimeMeshProviderProxyRef = TSharedRef<FRuntimeMeshProviderProxy, ESPMode::ThreadSafe>;
using FRuntimeMeshProviderProxyPtr = TSharedPtr<FRuntimeMeshProviderProxy, ESPMode::ThreadSafe>;
using FRuntimeMeshProviderProxyWeakPtr = TWeakPtr<FRuntimeMeshProviderProxy, ESPMode::ThreadSafe>;


class RUNTIMEMESHCOMPONENT_API IRuntimeMeshProviderProxy
{
public:
	IRuntimeMeshProviderProxy() { }
	virtual ~IRuntimeMeshProviderProxy() { }

	virtual void Initialize() { };

	virtual void ConfigureLOD(int32 LODIndex, const FRuntimeMeshLODProperties& LODProperties) { }
	virtual void CreateSection(int32 LODIndex, int32 SectionId, const FRuntimeMeshSectionProperties& SectionProperties) { }
	virtual void SetupMaterialSlot(int32 MaterialSlot, FName SlotName, UMaterialInterface* InMaterial) { }
	virtual void MarkSectionDirty(int32 LODIndex, int32 SectionId) { }
	virtual void SetSectionVisibility(int32 LODIndex, int32 SectionId, bool bIsVisible) { }
	virtual void SetSectionCastsShadow(int32 LODIndex, int32 SectionId, bool bCastsShadow) { }
	virtual void RemoveSection(int32 LODIndex, int32 SectionId) { }
	virtual void MarkCollisionDirty() { }

	virtual FBoxSphereBounds GetBounds() { return FBoxSphereBounds(); }

	virtual bool GetSectionMeshForLOD(int32 LODIndex, int32 SectionId, FRuntimeMeshRenderableMeshData& MeshData) { return false; }

	virtual FRuntimeMeshCollisionSettings GetCollisionSettings() { return FRuntimeMeshCollisionSettings(); }
	virtual bool HasCollisionMesh() { return false; }
	virtual bool GetCollisionMesh(FRuntimeMeshCollisionData& CollisionData) { return false; }

	virtual bool IsThreadSafe() const { return false; }
};

/**
 *
 */
class RUNTIMEMESHCOMPONENT_API FRuntimeMeshProviderProxy : public IRuntimeMeshProviderProxy, public TSharedFromThis<FRuntimeMeshProviderProxy, ESPMode::ThreadSafe>
{
private:
	TWeakObjectPtr<URuntimeMeshProvider> Parent;

protected:
	FRuntimeMeshProviderProxyPtr PreviousProvider;

public:
	FRuntimeMeshProviderProxy(TWeakObjectPtr<URuntimeMeshProvider> InParent);

protected:
	virtual void BindPreviousProvider(const FRuntimeMeshProviderProxyPtr& InPreviousProvider);
	
public:
	virtual void UpdateProxyParameters(URuntimeMeshProvider* ParentProvider, bool bIsInitialSetup);

	virtual void ConfigureLOD(int32 LODIndex, const FRuntimeMeshLODProperties& LODProperties) override;
	virtual void CreateSection(int32 LODIndex, int32 SectionId, const FRuntimeMeshSectionProperties& SectionProperties) override;
	virtual void SetupMaterialSlot(int32 MaterialSlot, FName SlotName, UMaterialInterface* InMaterial) override;
	virtual void MarkSectionDirty(int32 LODIndex, int32 SectionId) override;
	virtual void SetSectionVisibility(int32 LODIndex, int32 SectionId, bool bIsVisible) override;
	virtual void SetSectionCastsShadow(int32 LODIndex, int32 SectionId, bool bCastsShadow) override;
	virtual void RemoveSection(int32 LODIndex, int32 SectionId) override;
	virtual void MarkCollisionDirty() override;

private:
	friend class FRuntimeMeshData;
	friend class FRuntimeMeshProviderProxyPassThrough;
	friend class FRuntimeMeshProviderProxyUObjectProviderConnector;
};


class RUNTIMEMESHCOMPONENT_API FRuntimeMeshProviderProxyPassThrough : public FRuntimeMeshProviderProxy
{
protected:
	FRuntimeMeshProviderProxyWeakPtr NextProvider;

public:
	FRuntimeMeshProviderProxyPassThrough(TWeakObjectPtr<URuntimeMeshProvider> InParent, const FRuntimeMeshProviderProxyPtr& InNextProvider);

protected:
	virtual void BindPreviousProvider(const FRuntimeMeshProviderProxyPtr& InPreviousProvider) override;

public:
	virtual void Initialize() override;

	virtual FBoxSphereBounds GetBounds();

	virtual bool GetSectionMeshForLOD(int32 LODIndex, int32 SectionId, FRuntimeMeshRenderableMeshData& MeshData) override;

	virtual FRuntimeMeshCollisionSettings GetCollisionSettings();
	virtual bool HasCollisionMesh();
	virtual bool GetCollisionMesh(FRuntimeMeshCollisionData& CollisionData) override;

	virtual bool IsThreadSafe() const;
};



UCLASS(Abstract, HideCategories = Object, BlueprintType)
class RUNTIMEMESHCOMPONENT_API URuntimeMeshProvider : public UObject
{
	GENERATED_BODY()

private:
	FRuntimeMeshProviderProxyPtr Proxy;

protected:
	virtual FRuntimeMeshProviderProxyRef GetProxy();

public:
	FRuntimeMeshProviderProxyRef SetupProxy();

	UFUNCTION(BlueprintCallable, Category = "RuntimeMesh|Providers|Common")
	void MarkProxyParametersDirty();

	UFUNCTION(BlueprintNativeEvent, Category = "RuntimeMesh|Providers|Common")
	void Initialize();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RuntimeMesh|Providers|Common")
	void ConfigureLOD(int32 LODIndex, const FRuntimeMeshLODProperties& LODProperties);
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RuntimeMesh|Providers|Common")
	void CreateSection(int32 LODIndex, int32 SectionId, const FRuntimeMeshSectionProperties& SectionProperties);
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RuntimeMesh|Providers|Common")
	void SetupMaterialSlot(int32 MaterialSlot, FName SlotName, UMaterialInterface* InMaterial);
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RuntimeMesh|Providers|Common")
	void MarkSectionDirty(int32 LODIndex, int32 SectionId);
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RuntimeMesh|Providers|Common")
	void SetSectionVisibility(int32 LODIndex, int32 SectionId, bool bIsVisible);
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RuntimeMesh|Providers|Common")
	void SetSectionCastsShadow(int32 LODIndex, int32 SectionId, bool bCastsShadow);
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RuntimeMesh|Providers|Common")
	void RemoveSection(int32 LODIndex, int32 SectionId);
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RuntimeMesh|Providers|Common")
	void MarkCollisionDirty();



	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RuntimeMesh|Providers|Common")
	FBoxSphereBounds GetBounds();

	UFUNCTION(BlueprintNativeEvent, Category = "RuntimeMesh|Providers|Common")
	bool GetSectionMeshForLOD(int32 LODIndex, int32 SectionId, FRuntimeMeshRenderableMeshData& MeshData);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RuntimeMesh|Providers|Common")
	FRuntimeMeshCollisionSettings GetCollisionSettings();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RuntimeMesh|Providers|Common")
	bool HasCollisionMesh();

	UFUNCTION(BlueprintNativeEvent, Category = "RuntimeMesh|Providers|Common")
	bool GetCollisionMesh(FRuntimeMeshCollisionData& CollisionData);

}; 

