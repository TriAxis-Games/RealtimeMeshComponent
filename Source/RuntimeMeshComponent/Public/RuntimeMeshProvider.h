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

	/*
	 * Called when the RMC's Initialize function is called. The provider is now connected to the component and can setup things
	 */
	UFUNCTION(BlueprintCallable, Category = "RuntimeMeshProvider|ConfigureLODs")
	virtual void Initialize() { }

	/*
	 * Called by the RMC to gather the bounds of the mesh.
	 * Bounds that are too small will cause the mesh to flicker or disappear too early
	 * Bounds that are too big will reduce the effectiveness of culling and lower performance
	 */
	UFUNCTION(BlueprintCallable, Category = "RuntimeMeshProvider|ConfigureLODs")
	virtual FBoxSphereBounds GetBounds() { return FBoxSphereBounds(FSphere(FVector::ZeroVector, 1.0f)); }

	/*
	 * Function called by the component to gather your mesh data
	 * May be called from outside of the game thread if IsThreadSafe returns true
	 * You should only return true if you believe that the mesh data you have given is valid
	 * Will be enabled if bCanGetSectionsIndependently is true in the LOD properties
	 * 
	 * @param LODIndex Index of the LOD for which your are to supply mesh data
	 * @param SectionId Index of the section for which you are to supply the mesh data
	 * @param MeshData The structure that you have to fill with mesh data
	 * @return True if you believe that you have given valid data, false if not. An empty mesh is considered invalid.
	 */
	UFUNCTION(BlueprintCallable, Category = "RuntimeMeshProvider|ConfigureLODs")
	virtual bool GetSectionMeshForLOD(int32 LODIndex, int32 SectionId, FRuntimeMeshRenderableMeshData& MeshData) { return false; }

	/*
	 * Function called by the component to gather your mesh data
	 * May be called from outside of the game thread if IsThreadSafe returns true
	 * You should only return true if you believe that the mesh data you have given is valid
	 * Will be enabled if bCanGetAllSectionsAtOnce is true in the LOD properties
	 * Will prefer calling this if possible
	 * 
	 * @param LODIndex Index of the LOD for which your are to supply mesh data
	 * @param MeshData The structures that you have to fill with mesh data. Index 0 is for LOD0 and so on
	 * @return True if you believe that you have given valid data, false if not. An empty mesh is considered invalid.
	 */
	UFUNCTION(BlueprintCallable, Category = "RuntimeMeshProvider|ConfigureLODs")
	virtual bool GetAllSectionsMeshForLOD(int32 LODIndex, TMap<int32, FRuntimeMeshSectionData>& MeshDatas) { return false; }

	/*
	 * Function called by the component to gather simple collision
	 */
	UFUNCTION(BlueprintCallable, Category = "RuntimeMeshProvider|ConfigureLODs")
	virtual FRuntimeMeshCollisionSettings GetCollisionSettings() { return FRuntimeMeshCollisionSettings(); }

	/*
	 * Function called by the component to know if a collision mesh (complex collision) is gatherable
	 */
	UFUNCTION(BlueprintCallable, Category = "RuntimeMeshProvider|ConfigureLODs")
	virtual bool HasCollisionMesh() { return false; }

	/*
	 * Function called by the component to gather a collision mesh (complex collision)
	 * @return true if the collision mesh is valid
	 */
	UFUNCTION(BlueprintCallable, Category = "RuntimeMeshProvider|ConfigureLODs")
	virtual bool GetCollisionMesh(FRuntimeMeshCollisionData& CollisionData) { return false; }

	/*
	 * Called by the component when the mesh calculation is completed.
	 * This is only a notifier
	 */
	UFUNCTION(BlueprintCallable, Category = "RuntimeMeshProvider|ConfigureLODs")
	virtual void MeshUpdateCompleted() { }

	/*
	 * Called by the component when the last given collision mesh is applied.
	 * This is only a notifier
	 */
	UFUNCTION(BlueprintCallable, Category = "RuntimeMeshProvider|ConfigureLODs")
	virtual void CollisionUpdateCompleted() { }

	/*
	 * Return true to enable out-of-gamethread renderable mesh gathering
	 */
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
	virtual void MeshUpdateCompleted() override;

	virtual bool IsThreadSafe() override;

};