// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "RealtimeMeshCollision.h"
#include "Data/RealtimeMeshData.h"
#include "Interfaces/Interface_CollisionDataProvider.h"
#include "RealtimeMesh.generated.h"


UCLASS(Blueprintable, Abstract, ClassGroup = Rendering, HideCategories = (Object, Activation, Cooking))
class REALTIMEMESHCOMPONENT_API URealtimeMesh : public UObject, public FTickableGameObject, public IInterface_CollisionDataProvider
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
	void BroadcastBoundsChangedEvent() { BoundsChangedEvent.Broadcast(this); }
	void BroadcastRenderDataChangedEvent(bool bShouldRecreateProxies) { RenderDataChangedEvent.Broadcast(this, bShouldRecreateProxies); }
	void BroadcastCollisionBodyUpdatedEvent(UBodySetup* NewBodySetup);

	virtual void UnbindEvents();

	//virtual RealtimeMesh::FRealtimeMeshClassFactoryRef GetClassFactory() const PURE_VIRTUAL(URealtimeMesh::GetClassFactory, return MakeShared<RealtimeMesh::FRealtimeMeshClassFactory>(););

private:
	UPROPERTY()
	TArray<FRealtimeMeshMaterialSlot> MaterialSlots;
	
	UPROPERTY()
	TMap<FName, int32> SlotNameLookup;

	UPROPERTY(Transient)
	UBodySetup* BodySetup;
	
	UPROPERTY(Transient)
	TArray<UBodySetup*> AsyncBodySetupQueue;
	
	// UPROPERTY()
	// TArray<FRealtimeMeshCollisionSourceSectionInfo> CollisionSource;
	//
	// // // Do we need to update our collision?
	// // bool bCollisionIsDirty;
	// //
	//
	// //
	//
	// //
	// // Queue of pending collision cooks
	//
	// UPROPERTY(Transient)
	// TUniquePtr<TArray<FRealtimeMeshCollisionSourceSectionInfo>> PendingSourceInfo;

	
public:
	virtual RealtimeMesh::FRealtimeMeshRef GetMesh() const
	{
		// We should not ever bee here
		check(false);
		return RealtimeMesh::FRealtimeMeshRef(static_cast<RealtimeMesh::FRealtimeMesh*>(nullptr));
	}

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
	FRealtimeMeshCollisionConfiguration GetCollisionConfig() const;
	
	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh")
	void SetCollisionConfig(const FRealtimeMeshCollisionConfiguration& InCollisionConfig);
	
	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh")
	FRealtimeMeshSimpleGeometry GetSimpleGeometry() const;
	
	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh")
	void SetSimpleGeometry(const FRealtimeMeshSimpleGeometry& InSimpleGeometry);
	
	
	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh")
	void SetupMaterialSlot(int32 MaterialSlot, FName SlotName, UMaterialInterface* InMaterial);
	
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

	// Cook collision, either asynchronously or synchronously depending on config
	void UpdateCollision(bool bForceCookNow = false);
	
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


	// Begin FTickableGameObject interface
	virtual void Tick( float DeltaTime ) override;
	virtual ETickableTickType GetTickableTickType() const override;
	virtual bool IsTickable() const override;
	virtual bool IsAllowedToTick() const override;
	virtual bool IsTickableWhenPaused() const { return true; }
	virtual bool IsTickableInEditor() const { return true; }
	virtual TStatId GetStatId() const override;
	// End FTickableObjectBase interface

private: // Collision
	// Helper to create new body setup objects
	UBodySetup* CreateNewBodySetup();
	// Once async physics cook is done, create needed state, and then call the user event
	void FinishPhysicsAsyncCook(bool bSuccess, UBodySetup* FinishedBodySetup);


private: // Render Proxy

public:
	virtual void HandleBoundsUpdated(const RealtimeMesh::FRealtimeMeshRef& IncomingMesh);
	virtual void HandleMeshRenderingDataChanged(const RealtimeMesh::FRealtimeMeshRef& IncomingMesh, bool bShouldProxyRecreate);

};

