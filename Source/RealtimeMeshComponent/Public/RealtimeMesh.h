// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "RealtimeMeshCore.h"
#include "Data/RealtimeMeshData.h"
#include "RealtimeMeshCollisionLibrary.h"
#include "Interfaces/Interface_CollisionDataProvider.h"
#include "Templates/Function.h"
#include "Tickable.h"
#include "RealtimeMesh.generated.h"


UCLASS(BlueprintType, Blueprintable, ConversionRoot, Abstract, ClassGroup = Rendering, HideCategories = (Object, Activation, Cooking))
class REALTIMEMESHCOMPONENT_API URealtimeMesh : public UObject
{
	GENERATED_UCLASS_BODY()
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

	void DispatchToGameThread(TUniqueFunction<void(URealtimeMesh*)>&& Func);

	void Initialize(const TSharedRef<RealtimeMesh::FRealtimeMeshContext>& InContext,
	                const RealtimeMesh::FRealtimeMeshRef& InMesh);

protected:
	RealtimeMesh::FRealtimeMeshContextPtr Context;
	RealtimeMesh::FRealtimeMeshPtr MeshRef;

	UPROPERTY()
	TArray<FRealtimeMeshMaterialSlot> MaterialSlots;

	UPROPERTY(Transient)
	TMap<FName, int32> SlotNameLookup;

	UPROPERTY(Transient)
	TObjectPtr<UBodySetup> BodySetup;
	TArray<FRealtimeMeshCollisionMeshCookedUVData> UVData;

	/* Currently applied collision version, used for ignoring old cooks in async */
	int32 CurrentCollisionVersion;

public:
	/** The FRealtimeMesh data container backing this object. */
	RealtimeMesh::FRealtimeMeshRef GetMesh() const { return MeshRef.ToSharedRef(); }

	/** GetMesh() static-cast to a concrete FRealtimeMesh subclass. */
	template <typename MeshType>
	TSharedRef<MeshType> GetMeshAs() const { return StaticCastSharedRef<MeshType>(MeshRef.ToSharedRef()); }

	/**
	 * Get the body setup associated with the RealtimeMesh.
	 *
	 * @return The body setup associated with the RealtimeMesh.
	 */
	UBodySetup* GetBodySetup() const { return BodySetup; }

	/**
	 * Get the UV position for the supplied hit location.
	 * 
	 * @return The UV coordinate for the hit.
	 */
	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh")
	bool CalcTexCoordAtLocation(const FVector& BodySpaceLocation, int32 ElementIndex, int32 FaceIndex, int32 UVChannel, FVector2D& UV) const;


	/**
	 * Reset the RealtimeMesh, clearing its geometry, material slots, collision and UV data.
	 */
	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh")
	virtual void Reset();

	/**
	 * Retrieves the local bounds of the RealtimeMesh.
	 *
	 * @return the local bounds as a FBoxSphereBounds object.
	 */
	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh")
	virtual FBoxSphereBounds GetLocalBounds() const;

	/**
	 * Implement in Blueprint or a subclass to populate TargetMesh's geometry. Called when the
	 * mesh is (re)generated.
	 *
	 * @param TargetMesh The RealtimeMesh being generated.
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
	 * Set the number of material slots for the Realtime Mesh.
	 *
	 * When NewNumSlots is less than the current count, the trailing slots are trimmed and any of
	 * their names are removed from the slot-name lookup. When greater, the list is grown with empty
	 * (unnamed/None) slots for symmetry.
	 *
	 * Note: any sections that still reference a material slot trimmed away by a shrink will render
	 * with the engine default material (UMaterial::GetDefaultMaterial) rather than error out. This
	 * graceful fallback is intentional, so shrinking the slot count below a live section's slot
	 * index degrades that section's appearance instead of breaking rendering.
	 *
	 * @param NewNumSlots The desired number of material slots.
	 */
	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh")
	void SetNumMaterialSlots(int32 NewNumSlots);

	/**
	 * Get the index of a material slot by its name.
	 *
	 * @param MaterialSlotName The name of the material slot.
	 * @return The index of the material slot. Returns INDEX_NONE if the material slot does not exist.
	 */
	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh")
	int32 GetMaterialIndex(FName MaterialSlotName) const;

	/**
	 * Get the name of the material slot at the specified index
	 * @param Index Index of the material to get the name for
	 * @return 
	 */
	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh")
	FName GetMaterialSlotName(int32 Index) const;
	
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
	virtual bool IsSupportedForNetworking() const override { return true; }
	virtual void PostInitProperties() override;
	virtual void BeginDestroy() override;
	virtual void Serialize(FArchive& Ar) override;
	//	End UObject interface

protected:
	virtual void HandleBoundsUpdated();
	virtual void HandleRenderProxyRequiresUpdate();

protected: // Collision

	/*void InitiateCollisionUpdate(const TSharedRef<TPromise<ERealtimeMeshCollisionUpdateResult>>& Promise,
		const TSharedRef<FRealtimeMeshCollisionInfo>& NewCollisionInfo, bool bForceSyncUpdate);*/

	ERealtimeMeshCollisionUpdateResult ApplyCollisionUpdate(FRealtimeMeshCollisionInfo&& InCollisionData, int32 NewCollisionKey);
	
	friend class FRealtimeMeshDetailsCustomization;
	friend class RealtimeMesh::FRealtimeMesh;
};


