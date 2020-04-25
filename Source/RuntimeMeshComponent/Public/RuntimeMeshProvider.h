// Copyright 2016-2020 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RuntimeMeshProviderTargetInterface.h"
#include "RuntimeMeshReference.h"
#include "RuntimeMeshProvider.generated.h"



UCLASS(HideCategories = Object, BlueprintType, Blueprintable, Meta = (ShortTooltip = "A RuntimeMeshProvider is a class containing the logic to create the mesh data and related information to be used by a RuntimeMeshComponent for rendering."))
class RUNTIMEMESHCOMPONENT_API URuntimeMeshProvider
	: public UObject
	, public IRuntimeMeshProviderTargetInterface
{
	GENERATED_BODY()
private:
    FRuntimeMeshReferenceAliasAnchor<URuntimeMeshProvider> ReferenceAnchor;

    TScriptInterface<IRuntimeMeshProviderTargetInterface> InternalTarget;
    mutable FCriticalSection TargetSyncRoot;
    FThreadSafeBool bHasBeenBound;

	TScriptInterface<IRuntimeMeshProviderTargetInterface> GetTarget() const
	{
		FScopeLock Lock(&TargetSyncRoot);
		return InternalTarget;
	}
	virtual void ShutdownInternal() override;

public:
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void BindTargetProvider(const TScriptInterface<IRuntimeMeshProviderTargetInterface>& InTarget);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void Unlink();

	UFUNCTION(BlueprintCallable)
    bool HasBeenBound() const { return bHasBeenBound; }


public:
    URuntimeMeshProvider();


    FRuntimeMeshProviderWeakRef GetReference();
	virtual FRuntimeMeshWeakRef GetMeshReference() override;


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



    UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
    void ConfigureLODs(const TArray<FRuntimeMeshLODProperties>& InLODs);

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
    void SetLODScreenSize(int32 LODIndex, float ScreenSize);

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
    void MarkLODDirty(int32 LODIndex);

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
    void MarkAllLODsDirty();


    UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
    void CreateSection(int32 LODIndex, int32 SectionId, const FRuntimeMeshSectionProperties& SectionProperties);

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
    void SetSectionVisibility(int32 LODIndex, int32 SectionId, bool bIsVisible);

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
    void SetSectionCastsShadow(int32 LODIndex, int32 SectionId, bool bCastsShadow);

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
    void MarkSectionDirty(int32 LODIndex, int32 SectionId);

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
    void ClearSection(int32 LODIndex, int32 SectionId);

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
    void RemoveSection(int32 LODIndex, int32 SectionId);

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
    void MarkCollisionDirty();


    UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
    void SetupMaterialSlot(int32 MaterialSlot, FName SlotName, UMaterialInterface* InMaterial);

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
    int32 GetMaterialIndex(FName MaterialSlotName);

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
    bool IsMaterialSlotNameValid(FName MaterialSlotName) const;

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
    FRuntimeMeshMaterialSlot GetMaterialSlot(int32 SlotIndex);

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
    int32 GetNumMaterials();

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
    TArray<FName> GetMaterialSlotNames();

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
    TArray<FRuntimeMeshMaterialSlot> GetMaterialSlots();

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
    UMaterialInterface* GetMaterial(int32 SlotIndex);

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

    void BindTargetProvider_Implementation(const TScriptInterface<IRuntimeMeshProviderTargetInterface>& InTarget);
    void Unlink_Implementation();

public:
    URuntimeMeshProviderPassthrough();

    UFUNCTION(BlueprintCallable)
	URuntimeMeshProvider* GetChildProvider() const;

	UFUNCTION(BlueprintCallable)
    void SetChildProvider(URuntimeMeshProvider* InProvider);

    void Initialize_Implementation();
    FBoxSphereBounds GetBounds_Implementation();
    bool GetSectionMeshForLOD_Implementation(int32 LODIndex, int32 SectionId, FRuntimeMeshRenderableMeshData& MeshData);
    bool GetAllSectionsMeshForLOD_Implementation(int32 LODIndex, TMap<int32, FRuntimeMeshSectionData>& MeshDatas);
    FRuntimeMeshCollisionSettings GetCollisionSettings_Implementation();
    bool HasCollisionMesh_Implementation();
    bool GetCollisionMesh_Implementation(FRuntimeMeshCollisionData& CollisionData);
    void CollisionUpdateCompleted_Implementation();
    bool IsThreadSafe_Implementation();

};