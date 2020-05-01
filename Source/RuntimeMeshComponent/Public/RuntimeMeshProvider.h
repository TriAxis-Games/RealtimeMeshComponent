// Copyright 2016-2020 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RuntimeMeshProviderTargetInterface.h"
#include "RuntimeMeshReference.h"
#include "RuntimeMeshProvider.generated.h"



UCLASS(HideCategories = Object, BlueprintType, Blueprintable, Meta = (ShortTooltip = "A RuntimeMeshProvider is a class containing the logic to create the mesh data and related information to be used by a RuntimeMeshComponent for rendering."))
class RUNTIMEMESHCOMPONENT_API URuntimeMeshProvider : public URuntimeMeshProviderTargetInterface
{
	GENERATED_BODY()
private:
    FRuntimeMeshReferenceAliasAnchor<URuntimeMeshProvider> ReferenceAnchor;

    URuntimeMeshProviderTargetInterface* InternalTarget;
    mutable FCriticalSection TargetSyncRoot;

    URuntimeMeshProviderTargetInterface* GetTarget() const
	{
		FScopeLock Lock(&TargetSyncRoot);
		return InternalTarget;
	}
	virtual void ShutdownInternal() override;

public:
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void BindTargetProvider(URuntimeMeshProviderTargetInterface* InTarget);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void Unlink();

	UFUNCTION(BlueprintCallable)
	bool HasBeenBound() const { return InternalTarget != nullptr; }

public:
    URuntimeMeshProvider();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void Initialize();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	FBoxSphereBounds GetBounds();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	bool GetSectionMeshForLOD(int32 LODIndex, int32 SectionId, FRuntimeMeshRenderableMeshData& MeshData);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	bool GetAllSectionsMeshForLOD(int32 LODIndex, TMap<int32, FRuntimeMeshSectionData>& MeshDatas);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	FRuntimeMeshCollisionSettings GetCollisionSettings();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	bool HasCollisionMesh();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	bool GetCollisionMesh(FRuntimeMeshCollisionData& CollisionData);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void CollisionUpdateCompleted();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	bool IsThreadSafe();

	FRuntimeMeshProviderWeakRef GetReference();

	//	Begin IRuntimeMeshProviderTargetInterface interface
	virtual FRuntimeMeshWeakRef GetMeshReference() override;
	virtual void ConfigureLODs_Implementation(const TArray<FRuntimeMeshLODProperties>& InLODs) override;
	virtual void SetLODScreenSize_Implementation(int32 LODIndex, float ScreenSize) override;
	virtual void MarkLODDirty_Implementation(int32 LODIndex) override;
	virtual void MarkAllLODsDirty_Implementation() override;

	virtual void CreateSection_Implementation(int32 LODIndex, int32 SectionId, const FRuntimeMeshSectionProperties& SectionProperties) override;
	virtual void SetSectionVisibility_Implementation(int32 LODIndex, int32 SectionId, bool bIsVisible) override;
	virtual void SetSectionCastsShadow_Implementation(int32 LODIndex, int32 SectionId, bool bCastsShadow) override;
	virtual void MarkSectionDirty_Implementation(int32 LODIndex, int32 SectionId) override;
	virtual void ClearSection_Implementation(int32 LODIndex, int32 SectionId) override;
	virtual void RemoveSection_Implementation(int32 LODIndex, int32 SectionId) override;
	virtual void MarkCollisionDirty_Implementation() override;

	virtual void SetupMaterialSlot_Implementation(int32 MaterialSlot, FName SlotName, UMaterialInterface* InMaterial) override;
	virtual int32 GetMaterialIndex_Implementation(FName MaterialSlotName) override;
	virtual bool IsMaterialSlotNameValid_Implementation(FName MaterialSlotName) const override;
	virtual FRuntimeMeshMaterialSlot GetMaterialSlot_Implementation(int32 SlotIndex) override;
	virtual int32 GetNumMaterials_Implementation() override;
	virtual TArray<FName> GetMaterialSlotNames_Implementation() override;
	virtual TArray<FRuntimeMeshMaterialSlot> GetMaterialSlots_Implementation() override;
	virtual UMaterialInterface* GetMaterial_Implementation(int32 SlotIndex) override;
	//	End IRuntimeMeshProviderTargetInterface interface

    // Begin UObject interface
	void BeginDestroy() override;
    bool IsReadyForFinishDestroy() override;
    // End UObject interface
protected:

    friend class URuntimeMesh;
};

UCLASS(HideCategories = Object, BlueprintType, Blueprintable, Meta = (ShortTooltip = "A RuntimeMeshProviderPassthrough is a class containing logic to modify the mesh data from a linked provider before passing it onto the RuntimeMeshComponent."))
class RUNTIMEMESHCOMPONENT_API URuntimeMeshProviderPassthrough
    : public URuntimeMeshProvider
{
    GENERATED_BODY()
private:
    UPROPERTY(VisibleAnywhere, BlueprintGetter = GetChildProvider, BlueprintSetter = SetChildProvider)
    URuntimeMeshProvider* ChildProvider;
public:

	virtual void BindTargetProvider_Implementation(URuntimeMeshProviderTargetInterface* InTarget) override;
	virtual void Unlink_Implementation() override;

public:
    URuntimeMeshProviderPassthrough();

    UFUNCTION(BlueprintCallable)
	URuntimeMeshProvider* GetChildProvider() const;

	UFUNCTION(BlueprintCallable)
    void SetChildProvider(URuntimeMeshProvider* InProvider);

	virtual void Initialize_Implementation() override;
	virtual FBoxSphereBounds GetBounds_Implementation() override;
	virtual bool GetSectionMeshForLOD_Implementation(int32 LODIndex, int32 SectionId, FRuntimeMeshRenderableMeshData& MeshData) override;
	virtual bool GetAllSectionsMeshForLOD_Implementation(int32 LODIndex, TMap<int32, FRuntimeMeshSectionData>& MeshDatas) override;
	virtual FRuntimeMeshCollisionSettings GetCollisionSettings_Implementation() override;
	virtual bool HasCollisionMesh_Implementation() override;
	virtual bool GetCollisionMesh_Implementation(FRuntimeMeshCollisionData& CollisionData) override;
	virtual void CollisionUpdateCompleted_Implementation() override;
	virtual bool IsThreadSafe_Implementation() override;

};