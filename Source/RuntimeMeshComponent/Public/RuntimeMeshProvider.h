// Copyright 2016-2020 TriAxis Games L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RuntimeMeshProviderTargetInterface.h"
#include "RuntimeMeshReference.h"
#include "RuntimeMeshProvider.generated.h"



UCLASS(HideCategories = Object, BlueprintType, Meta = (ShortTooltip = "A RuntimeMeshProvider is a class containing the logic to create the mesh data and related information to be used by a RuntimeMeshComponent for rendering."))
class RUNTIMEMESHCOMPONENT_API URuntimeMeshProvider : public URuntimeMeshProviderTargetInterface
{
	GENERATED_BODY()
protected:

	FRuntimeMeshReferenceAnchor<URuntimeMeshProvider> GCAnchor;

	URuntimeMeshProviderTargetInterface* Target;
	mutable FRWLock TargetLock;
	FThreadSafeBool bIsBound;

public:
	UFUNCTION(BlueprintCallable, Category = "RuntimeMeshProvider|ConfigureLODs")
	bool IsBound() const { check(bIsBound == (Target != nullptr)); return bIsBound; }

	UFUNCTION(BlueprintCallable, Category = "RuntimeMeshProvider|ConfigureLODs")
	void Shutdown();

	virtual void BindTargetProvider(URuntimeMeshProviderTargetInterface* InTarget);
	virtual void UnlinkProviders() override;


public:
    URuntimeMeshProvider();

	UFUNCTION(BlueprintCallable, Category = "RuntimeMeshProvider|ConfigureLODs")
	virtual void Initialize() { }

	UFUNCTION(BlueprintCallable, Category = "RuntimeMeshProvider|ConfigureLODs")
	virtual FBoxSphereBounds GetBounds() { return FBoxSphereBounds(FSphere(FVector::ZeroVector, 1.0f)); }

	UFUNCTION(BlueprintCallable, Category = "RuntimeMeshProvider|ConfigureLODs")
	virtual bool GetSectionMeshForLOD(int32 LODIndex, int32 SectionId, FRuntimeMeshRenderableMeshData& MeshData) { return false; }

	UFUNCTION(BlueprintCallable, Category = "RuntimeMeshProvider|ConfigureLODs")
	virtual bool GetAllSectionsMeshForLOD(int32 LODIndex, TMap<int32, FRuntimeMeshSectionData>& MeshDatas) { return false; }

	UFUNCTION(BlueprintCallable, Category = "RuntimeMeshProvider|ConfigureLODs")
	virtual FRuntimeMeshCollisionSettings GetCollisionSettings() { return FRuntimeMeshCollisionSettings(); }

	UFUNCTION(BlueprintCallable, Category = "RuntimeMeshProvider|ConfigureLODs")
	virtual bool HasCollisionMesh() { return false; }

	UFUNCTION(BlueprintCallable, Category = "RuntimeMeshProvider|ConfigureLODs")
	virtual bool GetCollisionMesh(FRuntimeMeshCollisionData& CollisionData) { return false; }

	UFUNCTION(BlueprintCallable, Category = "RuntimeMeshProvider|ConfigureLODs")
	virtual void CollisionUpdateCompleted() { }


	UFUNCTION(BlueprintCallable, Category = "RuntimeMeshProvider|ConfigureLODs")
	virtual bool IsThreadSafe() { return false; }

	FRuntimeMeshProviderWeakRef GetReference();

	//	Begin IRuntimeMeshProviderTargetInterface interface
	virtual FRuntimeMeshWeakRef GetMeshReference() override;
	virtual void ConfigureLODs(const TArray<FRuntimeMeshLODProperties>& InLODs) override;
	virtual void SetLODScreenSize(int32 LODIndex, float ScreenSize) override;
	virtual void MarkLODDirty(int32 LODIndex) override;
	virtual void MarkAllLODsDirty() override;

	virtual void CreateSection(int32 LODIndex, int32 SectionId, const FRuntimeMeshSectionProperties& SectionProperties) override;
	virtual void SetSectionVisibility(int32 LODIndex, int32 SectionId, bool bIsVisible) override;
	virtual void SetSectionCastsShadow(int32 LODIndex, int32 SectionId, bool bCastsShadow) override;
	virtual void MarkSectionDirty(int32 LODIndex, int32 SectionId) override;
	virtual void ClearSection(int32 LODIndex, int32 SectionId) override;
	virtual void RemoveSection(int32 LODIndex, int32 SectionId) override;
	virtual void MarkCollisionDirty() override;


	virtual void SetupMaterialSlot(int32 MaterialSlot, FName SlotName, UMaterialInterface* InMaterial) override;
	virtual int32 GetMaterialIndex(FName MaterialSlotName) override;
	virtual bool IsMaterialSlotNameValid(FName MaterialSlotName) const override;
	virtual FRuntimeMeshMaterialSlot GetMaterialSlot(int32 SlotIndex) override;
	virtual int32 GetNumMaterials() override;
	virtual TArray<FName> GetMaterialSlotNames() override;
	virtual TArray<FRuntimeMeshMaterialSlot> GetMaterialSlots() override;
	virtual UMaterialInterface* GetMaterial(int32 SlotIndex) override;
	//	End IRuntimeMeshProviderTargetInterface interface

    // Begin UObject interface
	void BeginDestroy() override;
    bool IsReadyForFinishDestroy() override;
    // End UObject interface
protected:

    friend class URuntimeMesh;
};




UCLASS(HideCategories = Object, BlueprintType, Meta = (ShortTooltip = "A RuntimeMeshProviderPassthrough is a class containing logic to modify the mesh data from a linked provider before passing it onto the RuntimeMeshComponent."))
class RUNTIMEMESHCOMPONENT_API URuntimeMeshProviderPassthrough
    : public URuntimeMeshProvider
{
    GENERATED_BODY()
private:
    UPROPERTY(VisibleAnywhere, BlueprintGetter = GetChildProvider, BlueprintSetter = SetChildProvider, Category = "RuntimeMesh|Providers")
    URuntimeMeshProvider* ChildProvider;

	mutable FRWLock ChildLock;
public:

	virtual void BindTargetProvider(URuntimeMeshProviderTargetInterface* InTarget) override;
	virtual void UnlinkProviders() override;

public:
    URuntimeMeshProviderPassthrough();

    UFUNCTION(BlueprintCallable, Category = "RuntimeMesh|Providers")
	URuntimeMeshProvider* GetChildProvider() const
	{
		FReadScopeLock Lock(ChildLock);
		return ChildProvider;
	}

	UFUNCTION(BlueprintCallable, Category = "RuntimeMesh|Providers")
    void SetChildProvider(URuntimeMeshProvider* InProvider);

	virtual void Initialize() override;
	virtual FBoxSphereBounds GetBounds() override;
	virtual bool GetSectionMeshForLOD(int32 LODIndex, int32 SectionId, FRuntimeMeshRenderableMeshData& MeshData) override;
	virtual bool GetAllSectionsMeshForLOD(int32 LODIndex, TMap<int32, FRuntimeMeshSectionData>& MeshDatas) override;
	virtual FRuntimeMeshCollisionSettings GetCollisionSettings() override;
	virtual bool HasCollisionMesh() override;
	virtual bool GetCollisionMesh(FRuntimeMeshCollisionData& CollisionData) override;
	virtual void CollisionUpdateCompleted() override;


	virtual bool IsThreadSafe() override;

};