// Copyright 2016-2020 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RuntimeMeshRenderable.h"
#include "RuntimeMeshCollision.h"
#include "RuntimeMeshProvider.generated.h"


class FRuntimeMeshProviderProxy;
using FRuntimeMeshProviderProxyRef = TSharedRef<FRuntimeMeshProviderProxy, ESPMode::ThreadSafe>;
using FRuntimeMeshProviderProxyPtr = TSharedPtr<FRuntimeMeshProviderProxy, ESPMode::ThreadSafe>;
using FRuntimeMeshProviderProxyWeakPtr = TWeakPtr<FRuntimeMeshProviderProxy, ESPMode::ThreadSafe>;


DECLARE_DELEGATE(FRuntimeMeshProviderThreadExclusiveFunction);

class RUNTIMEMESHCOMPONENT_API IRuntimeMeshProviderProxy
{
public:
	IRuntimeMeshProviderProxy() { }
	virtual ~IRuntimeMeshProviderProxy() { }

	virtual void Initialize() { };
	
	virtual void ConfigureLODs(TArray<FRuntimeMeshLODProperties> LODs) { }

	virtual void CreateSection(int32 LODIndex, int32 SectionId, const FRuntimeMeshSectionProperties& SectionProperties) { }
	virtual void SetupMaterialSlot(int32 MaterialSlot, FName SlotName, UMaterialInterface* InMaterial) { }
	virtual int32 GetMaterialIndex(FName MaterialSlotName) { return INDEX_NONE; }
	virtual int32 GetNumMaterials() { return 0; }
	virtual TArray<FRuntimeMeshMaterialSlot> GetMaterialSlots() const { return TArray<FRuntimeMeshMaterialSlot>(); }
	virtual void MarkSectionDirty(int32 LODIndex, int32 SectionId) { }
	virtual void MarkLODDirty(int32 LODIndex) { }
	virtual void MarkAllLODsDirty() { }
	virtual void SetSectionVisibility(int32 LODIndex, int32 SectionId, bool bIsVisible) { }
	virtual void SetSectionCastsShadow(int32 LODIndex, int32 SectionId, bool bCastsShadow) { }
	virtual void RemoveSection(int32 LODIndex, int32 SectionId) { }
	virtual void MarkCollisionDirty() { }

	virtual FBoxSphereBounds GetBounds() { return FBoxSphereBounds(); }

	virtual bool GetAllSectionsMeshForLOD(int32 LODIndex, TMap<int32, TTuple<FRuntimeMeshSectionProperties, FRuntimeMeshRenderableMeshData>>& MeshDatas) { return false; }
	virtual bool GetSectionMeshForLOD(int32 LODIndex, int32 SectionId, FRuntimeMeshRenderableMeshData& MeshData) { return false; }

	virtual FRuntimeMeshCollisionSettings GetCollisionSettings() { return FRuntimeMeshCollisionSettings(); }
	virtual bool HasCollisionMesh() { return false; }
	virtual bool GetCollisionMesh(FRuntimeMeshCollisionData& CollisionData) { return false; }

	virtual bool IsThreadSafe() const { return false; }
	virtual void DoOnGameThreadAndBlockThreads(FRuntimeMeshProviderThreadExclusiveFunction Func) { }
};

/**
 *
 */
class RUNTIMEMESHCOMPONENT_API FRuntimeMeshProviderProxy : public IRuntimeMeshProviderProxy, public TSharedFromThis<FRuntimeMeshProviderProxy, ESPMode::ThreadSafe>
{
private:
	TWeakObjectPtr<URuntimeMeshProvider> Parent;

protected:
	FRuntimeMeshProviderProxyWeakPtr PreviousProvider;

public:
	FRuntimeMeshProviderProxy(TWeakObjectPtr<URuntimeMeshProvider> InParent);

protected:
	virtual void BindPreviousProvider(const FRuntimeMeshProviderProxyPtr& InPreviousProvider);
	
public:
	virtual void UpdateProxyParameters(URuntimeMeshProvider* ParentProvider, bool bIsInitialSetup);
	
	virtual void ConfigureLODs(TArray<FRuntimeMeshLODProperties> LODs) override;

	virtual void CreateSection(int32 LODIndex, int32 SectionId, const FRuntimeMeshSectionProperties& SectionProperties) override;
	virtual void SetupMaterialSlot(int32 MaterialSlot, FName SlotName, UMaterialInterface* InMaterial) override;
	virtual int32 GetMaterialIndex(FName MaterialSlotName) override;
	virtual int32 GetNumMaterials() override;
	virtual TArray<FRuntimeMeshMaterialSlot> GetMaterialSlots() const override;
	virtual void MarkSectionDirty(int32 LODIndex, int32 SectionId) override;
	virtual void MarkLODDirty(int32 LODIndex) override;
	virtual void MarkAllLODsDirty() override;
	virtual void SetSectionVisibility(int32 LODIndex, int32 SectionId, bool bIsVisible) override;
	virtual void SetSectionCastsShadow(int32 LODIndex, int32 SectionId, bool bCastsShadow) override;
	virtual void RemoveSection(int32 LODIndex, int32 SectionId) override;
	virtual void MarkCollisionDirty() override;

	TWeakObjectPtr<URuntimeMeshProvider> GetParent() const { return Parent; }

	virtual void DoOnGameThreadAndBlockThreads(FRuntimeMeshProviderThreadExclusiveFunction Func) override;

	template<typename T>
	TSharedRef<const T, ESPMode::ThreadSafe> AsSharedType() const
	{
		return StaticCastSharedRef<T>(this->AsShared());
	}

	template<typename T>
	TSharedRef<T, ESPMode::ThreadSafe> AsSharedType()
	{
		return StaticCastSharedRef<T>(this->AsShared());
	}
private:
	friend class FRuntimeMeshData;
	friend class FRuntimeMeshProviderProxyPassThrough;
	friend class FRuntimeMeshProviderProxyUObjectProviderConnector;
};


class RUNTIMEMESHCOMPONENT_API FRuntimeMeshProviderProxyPassThrough : public FRuntimeMeshProviderProxy
{
protected:
	FRuntimeMeshProviderProxyPtr NextProvider;

public:
	FRuntimeMeshProviderProxyPassThrough(TWeakObjectPtr<URuntimeMeshProvider> InParent, const FRuntimeMeshProviderProxyPtr& InNextProvider);



protected:
	virtual void BindPreviousProvider(const FRuntimeMeshProviderProxyPtr& InPreviousProvider) override;

public:
	virtual void Initialize() override;

	virtual FBoxSphereBounds GetBounds();

	virtual bool GetAllSectionsMeshForLOD(int32 LODIndex, TMap<int32, TTuple<FRuntimeMeshSectionProperties, FRuntimeMeshRenderableMeshData>>& MeshDatas) override;
	virtual bool GetSectionMeshForLOD(int32 LODIndex, int32 SectionId, FRuntimeMeshRenderableMeshData& MeshData) override;

	virtual FRuntimeMeshCollisionSettings GetCollisionSettings();
	virtual bool HasCollisionMesh();
	virtual bool GetCollisionMesh(FRuntimeMeshCollisionData& CollisionData) override;

	virtual bool IsThreadSafe() const;
};



UCLASS(Abstract, HideCategories = Object, BlueprintType, Blueprintable, Meta = (ShortTooltip = "A RuntimeMeshProvider is a class containing the logic to create the mesh data and related information to be used by a RuntimeMeshComponent for rendering."))
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
	void ConfigureLODs(const TArray<FRuntimeMeshLODProperties>& LODs);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RuntimeMesh|Providers|Common")
	void CreateSection(int32 LODIndex, int32 SectionId, const FRuntimeMeshSectionProperties& SectionProperties);
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RuntimeMesh|Providers|Common")
	void SetupMaterialSlot(int32 MaterialSlot, FName SlotName, UMaterialInterface* InMaterial);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RuntimeMesh|Providers|Common")
	int32 GetMaterialIndex(FName MaterialSlotName);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RuntimeMesh|Providers|Common")
	int32 GetNumMaterialSlots();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RuntimeMesh|Providers|Common")
	TArray<FRuntimeMeshMaterialSlot> GetMaterialSlots();
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RuntimeMesh|Providers|Common")
	void MarkSectionDirty(int32 LODIndex, int32 SectionId);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RuntimeMesh|Providers|Common")
	void MarkLODDirty(int32 LODIndex);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RuntimeMesh|Providers|Common")
	void MarkAllLODsDirty();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RuntimeMesh|Providers|Common")
	void SetSectionVisibility(int32 LODIndex, int32 SectionId, bool bIsVisible);
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RuntimeMesh|Providers|Common")
	void SetSectionCastsShadow(int32 LODIndex, int32 SectionId, bool bCastsShadow);
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RuntimeMesh|Providers|Common")
	void RemoveSection(int32 LODIndex, int32 SectionId);
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RuntimeMesh|Providers|Common")
	void MarkCollisionDirty();


protected:

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RuntimeMesh|Providers|Common")
	FBoxSphereBounds GetBounds();

	//UFUNCTION(BlueprintNativeEvent, Category = "RuntimeMesh|Providers|Common")
	//bool GetAllSectionsMeshForLOD(int32 LODIndex, TMap<int32, TTuple<FRuntimeMeshSectionProperties, FRuntimeMeshRenderableMeshData>>& MeshDatas);

	UFUNCTION(BlueprintNativeEvent, Category = "RuntimeMesh|Providers|Common")
	bool GetSectionMeshForLOD(int32 LODIndex, int32 SectionId, UPARAM(Ref) FRuntimeMeshRenderableMeshData& MeshData);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RuntimeMesh|Providers|Common")
	FRuntimeMeshCollisionSettings GetCollisionSettings();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RuntimeMesh|Providers|Common")
	bool HasCollisionMesh();

	UFUNCTION(BlueprintNativeEvent, Category = "RuntimeMesh|Providers|Common")
	bool GetCollisionMesh(UPARAM(Ref) FRuntimeMeshCollisionData& CollisionData);

	friend class FRuntimeMeshProviderProxyUObjectProviderConnector;
}; 

