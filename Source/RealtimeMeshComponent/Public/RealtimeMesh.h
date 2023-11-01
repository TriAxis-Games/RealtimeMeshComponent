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
	void BroadcastBoundsChangedEvent() { BoundsChangedEvent.Broadcast(this); }
	void BroadcastRenderDataChangedEvent(bool bShouldRecreateProxies) { RenderDataChangedEvent.Broadcast(this, bShouldRecreateProxies); }
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
	RealtimeMesh::FRealtimeMeshRef GetMesh() const { return MeshRef.ToSharedRef(); }

	template <typename MeshType>
	TSharedRef<MeshType> GetMeshAs() const { return StaticCastSharedRef<MeshType>(MeshRef.ToSharedRef()); }

	UBodySetup* GetBodySetup() const { return BodySetup; }

	/**
	 * Reset the mesh to its initial state
	 */
	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh")
	virtual void Reset(bool bCreateNewMeshData = false);

	/**
	 * Get the local-space bounds of the mesh
	 */
	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh")
	virtual FBoxSphereBounds GetLocalBounds() const;


	/**
	 * This event will be fired to notify the BP that the generated Mesh should
	 * be rebuilt. GeneratedDynamicMeshActor BP subclasses should rebuild their 
	 * meshes on this event, instead of doing so directly from the Construction Script.
	 */
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Components|RealtimeMesh|Events")
	void OnGenerateMesh(URealtimeMesh* TargetMesh);


	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh")
	FRealtimeMeshLODKey AddLOD(const FRealtimeMeshLODConfig& Config);

	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh")
	void UpdateLODConfig(FRealtimeMeshLODKey LODKey, const FRealtimeMeshLODConfig& Config);

	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh")
	void RemoveTrailingLOD();


	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh")
	void SetupMaterialSlot(int32 MaterialSlot, FName SlotName, UMaterialInterface* InMaterial = nullptr);

	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh")
	int32 GetMaterialIndex(FName MaterialSlotName) const;

	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh")
	bool IsMaterialSlotNameValid(FName MaterialSlotName) const;

	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh")
	FRealtimeMeshMaterialSlot GetMaterialSlot(int32 SlotIndex) const;

	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh")
	int32 GetNumMaterials() const;

	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh")
	TArray<FName> GetMaterialSlotNames() const;

	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh")
	TArray<FRealtimeMeshMaterialSlot> GetMaterialSlots() const;

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
	virtual void HandleMeshRenderingDataChanged();

	virtual void ProcessEndOfFrameUpdates();

	void MarkForEndOfFrameUpdate();

protected: // Collision
	void InitiateCollisionUpdate(const TSharedRef<TPromise<ERealtimeMeshCollisionUpdateResult>>& Promise, const TSharedRef<FRealtimeMeshCollisionData>& CollisionUpdate,
	                             bool bForceSyncUpdate);
	void FinishPhysicsAsyncCook(bool bSuccess, TSharedRef<TPromise<ERealtimeMeshCollisionUpdateResult>> Promise, UBodySetup* FinishedBodySetup, int32 UpdateKey);

	
	friend struct FRealtimeMeshEndOfFrameUpdateManager;
	
};
