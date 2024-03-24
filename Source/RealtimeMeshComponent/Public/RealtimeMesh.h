// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "RealtimeMeshCore.h"
#include "Data/RealtimeMeshData.h"
#include "RealtimeMeshCollision.h"
#include "Interfaces/Interface_CollisionDataProvider.h"
#if RMC_ENGINE_ABOVE_5_2
#include "Tickable.h"
#endif
#include "RealtimeMesh.generated.h"


UCLASS(Blueprintable, Abstract, ClassGroup = Rendering, HideCategories = (Object, Activation, Cooking))
class REALTIMEMESHCOMPONENT_API URealtimeMesh : public UObject, public IInterface_CollisionDataProvider
{
	GENERATED_UCLASS_BODY()
private:
	struct FRealtimeMeshCollisionUpdate
	{
		FRealtimeMeshTriMeshData TriMeshData;
		int32 UpdateKey;
		bool bFastCook;
	};

public:
	DECLARE_EVENT_OneParam(URealtimeMesh, FBoundsChangedEvent, URealtimeMesh*);

	DECLARE_EVENT_TwoParams(URealtimeMesh, FRenderDataChangedEvent, URealtimeMesh*, bool /*bShouldProxyRecreate*/);

	DECLARE_EVENT_TwoParams(URealtimeMesh, FCollisionBodyUpdated, URealtimeMesh*, UBodySetup*);

private:
	FBoundsChangedEvent BoundsChangedEvent;
	FRenderDataChangedEvent RenderDataChangedEvent;
	FCollisionBodyUpdated CollisionBodyUpdatedEvent;

public:
	FBoundsChangedEvent& OnBoundsChanged() { return BoundsChangedEvent; }
	FRenderDataChangedEvent& OnRenderDataChanged() { return RenderDataChangedEvent; }
	FCollisionBodyUpdated& OnCollisionBodyUpdated() { return CollisionBodyUpdatedEvent; }

protected:
	void BroadcastBoundsChangedEvent();
	void BroadcastRenderDataChangedEvent(bool bShouldRecreateProxies);
	void BroadcastCollisionBodyUpdatedEvent(UBodySetup* NewBodySetup);

	void Initialize(const TSharedRef<RealtimeMesh::FRealtimeMeshSharedResources>& InSharedResources);

protected:
	// Class factory to use to create all the 
	RealtimeMesh::FRealtimeMeshSharedResourcesPtr SharedResources;
	RealtimeMesh::FRealtimeMeshPtr MeshRef;

	UPROPERTY()
	TArray<FRealtimeMeshMaterialSlot> MaterialSlots;

	UPROPERTY()
	TMap<FName, int32> SlotNameLookup;

	UPROPERTY(Instanced)
	TObjectPtr<UBodySetup> BodySetup;

	/* Collision data that is pending async cook */
	UPROPERTY(Transient)
	TObjectPtr<UBodySetup> PendingBodySetup;

	/* Collision data to cook for any pending async cook */
	TOptional<FRealtimeMeshCollisionUpdate> PendingCollisionUpdate;

	/* Counter for generating version identifier for collision updates */
	int32 CollisionUpdateVersionCounter;

	/* Currently applied collision version, used for ignoring old cooks in async */
	int32 CurrentCollisionVersion;

public:
	/**
	 * @brief GetMesh returns the FRealtimeMesh data container.
	 *
	 * @details This method provides access to the RealtimeMesh object that is held by the current instance of the RealtimeMesh class.
	 *
	 * @return A shared reference to the RealtimeMesh object.
	 */
	RealtimeMesh::FRealtimeMeshRef GetMesh() const { return MeshRef.ToSharedRef(); }

	template <typename MeshType>
	/**
	 * @brief Get the mesh as a shared reference to a specific type.
	 *
	 * This method returns the mesh as a shared reference to a specific type by performing a static cast.
	 *
	 * @tparam MeshType The type of mesh to cast the mesh to.
	 * @return A shared reference to the mesh as the specified type.
	 */
	TSharedRef<MeshType> GetMeshAs() const { return StaticCastSharedRef<MeshType>(MeshRef.ToSharedRef()); }

	/**
	 * Get the body setup associated with the RealtimeMesh.
	 *
	 * @return The body setup associated with the RealtimeMesh.
	 */
	UBodySetup* GetBodySetup() const { return BodySetup; }

	/**
	 * Reset the RealtimeMesh.
	 *
	 * @param bCreateNewMeshData If true, create new mesh data. If false, reset the existing mesh data.
	 */
	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh")
	virtual void Reset(bool bCreateNewMeshData = false);

	/**
	 * Retrieves the local bounds of the RealtimeMesh.
	 *
	 * @return the local bounds as a FBoxSphereBounds object.
	 */
	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh")
	virtual FBoxSphereBounds GetLocalBounds() const;

	/**
	 * @brief Triggered when a mesh generation event occurs.
	 *
	 * This method is called when a mesh is being generated for a RealtimeMesh component.
	 * Developers can implement this method in their Blueprint or C++ code to customize the generation process.
	 *
	 * @param TargetMesh The RealtimeMesh component that is generating the mesh.
	 *
	 * @note This method is a BlueprintCallable and BlueprintImplementableEvent, meaning it can be called from Blueprint code and overridden in Blueprint subclasses.
	 *       It is also categorized under "Components|RealtimeMesh|Events" in Blueprint.
	 */
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Components|RealtimeMesh|Events")
	void OnGenerateMesh(URealtimeMesh* TargetMesh);


	/**
	 * Add a level of detail to the RealtimeMesh.
	 *
	 * @param Config The configuration for the level of detail.
	 * @return The key for the added level of detail.
	 */
	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh")
	FRealtimeMeshLODKey AddLOD(const FRealtimeMeshLODConfig& Config);

	/**
	 * Updates the configuration for a level of detail in the RealtimeMesh.
	 *
	 * @param LODKey The key of the level of detail to update.
	 * @param Config The updated configuration for the level of detail.
	 */
	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh")
	void UpdateLODConfig(FRealtimeMeshLODKey LODKey, const FRealtimeMeshLODConfig& Config);

	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh")
	void RemoveTrailingLOD();


	/**
	 * Set up a material slot for the Realtime Mesh.
	 *
	 * @param MaterialSlot The slot index for the material.
	 * @param SlotName The name of the material slot.
	 * @param InMaterial The material to be assigned to the slot.
	 */
	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh")
	void SetupMaterialSlot(int32 MaterialSlot, FName SlotName, UMaterialInterface* InMaterial = nullptr);

	/**
	 * Get the index of a material slot by its name.
	 *
	 * @param MaterialSlotName The name of the material slot.
	 * @return The index of the material slot. Returns INDEX_NONE if the material slot does not exist.
	 */
	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh")
	int32 GetMaterialIndex(FName MaterialSlotName) const;

	/**
	 * Check if the given material slot name is valid.
	 *
	 * @param MaterialSlotName The name of the material slot to check.
	 * @return true if the material slot name is valid, false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh")
	bool IsMaterialSlotNameValid(FName MaterialSlotName) const;

	/**
	 * Gets the material slot at the specified index.
	 *
	 * @param SlotIndex The index of the material slot.
	 * @return The material slot at the specified index.
	 */
	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh")
	FRealtimeMeshMaterialSlot GetMaterialSlot(int32 SlotIndex) const;

	/**
	 * Get the number of material slots in the RealtimeMesh.
	 *
	 * @return The number of material slots.
	 */
	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh")
	int32 GetNumMaterials() const;

	/**
	 * Get the names of all material slots in the Realtime Mesh.
	 *
	 * @return An array of FName representing the names of all material slots.
	 */
	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh")
	TArray<FName> GetMaterialSlotNames() const;

	/**
	 * Get the material slots of the Realtime Mesh.
	 *
	 * @return An array of FRealtimeMeshMaterialSlot representing the material slots of the Realtime Mesh.
	 */
	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh")
	TArray<FRealtimeMeshMaterialSlot> GetMaterialSlots() const;

	/**
	 * Get the material at the specified slot index.
	 *
	 * @param SlotIndex The index of the material slot.
	 * @return The material at the specified slot index. Returns nullptr if the slot index is invalid.
	 */
	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh")
	UMaterialInterface* GetMaterial(int32 SlotIndex) const;

public:
	//	Begin UObject interface
	virtual void PostInitProperties() override;
	virtual void BeginDestroy() override;
	virtual void Serialize(FArchive& Ar) override;
	virtual void PostDuplicate(bool bDuplicateForPIE) override;
	//	End UObject interface

	//	Begin IInterface_CollisionDataProvider interface
	virtual bool GetPhysicsTriMeshData(struct FTriMeshCollisionData* CollisionData, bool InUseAllTriData) override;
	virtual bool ContainsPhysicsTriMeshData(bool InUseAllTriData) const override;
	virtual bool WantsNegXTriMesh() override { return false; }
	virtual void GetMeshId(FString& OutMeshId) override { OutMeshId = GetName(); }
	//	End IInterface_CollisionDataProvider interface

protected:
	virtual void HandleBoundsUpdated();
	virtual void HandleMeshRenderingDataChanged(bool bShouldRecreateProxies);

	virtual void ProcessEndOfFrameUpdates();

	void MarkForEndOfFrameUpdate();

protected: // Collision
	void InitiateCollisionUpdate(const TSharedRef<TPromise<ERealtimeMeshCollisionUpdateResult>>& Promise, const TSharedRef<FRealtimeMeshCollisionData>& CollisionUpdate,
	                             bool bForceSyncUpdate);
	void FinishPhysicsAsyncCook(bool bSuccess, TSharedRef<struct FRealtimeMeshCookAutoPromiseOnDestruction> Promise, UBodySetup* FinishedBodySetup, int32 UpdateKey);

	
	friend struct FRealtimeMeshEndOfFrameUpdateManager;
	
};


