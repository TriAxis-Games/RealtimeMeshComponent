// Copyright 2016-2019 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RealtimeMeshCore.h"
#include "RealtimeMeshCollision.h"
#include "RealtimeMeshProvider.h"
#include "RealtimeMesh.generated.h"

class URealtimeMesh;
class URealtimeMeshComponent;

class FRealtimeMeshData;
using FRealtimeMeshDataPtr = TSharedPtr<FRealtimeMeshData, ESPMode::ThreadSafe>;
class FRealtimeMeshProxy;
using FRealtimeMeshProxyPtr = TSharedPtr<FRealtimeMeshProxy, ESPMode::ThreadSafe>;

/*
*	This tick function is used to drive the collision cooker.
*	It is enabled for one frame when we need to update collision.
*	This keeps from cooking on each individual create/update section as the original PMC did
*/
struct FRealtimeMeshCollisionCookTickObject : FTickableGameObject
{
	TWeakObjectPtr<URealtimeMesh> Owner;

	FRealtimeMeshCollisionCookTickObject(TWeakObjectPtr<URealtimeMesh> InOwner) : Owner(InOwner) {}
	virtual void Tick(float DeltaTime);
	virtual bool IsTickable() const;
	virtual bool IsTickableInEditor() const { return false; }
	virtual TStatId GetStatId() const;

	virtual UWorld* GetTickableGameObjectWorld() const;
};

/**
*	Delegate for when the collision was updated.
*/
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FRealtimeMeshCollisionUpdatedDelegate);


UCLASS(HideCategories = Object, BlueprintType)
class REALTIMEMESHCOMPONENT_API URealtimeMesh : public UObject, public IInterface_CollisionDataProvider
{
	GENERATED_UCLASS_BODY()

private:
	/** Reference to the underlying data object */
	FRealtimeMeshDataPtr Data;

	/** All RealtimeMeshComponents linked to this mesh. Used to alert the components of changes */
	TArray<TWeakObjectPtr<URealtimeMeshComponent>> LinkedComponents;

	UPROPERTY()
	URealtimeMeshProvider* MeshProvider;

	/** Do we need to update our collision? */
	bool bCollisionIsDirty;

	/** Object used to tick the collision cooking at the end of the frame */
	TUniquePtr<FRealtimeMeshCollisionCookTickObject> CookTickObject;

	/** Collision data */
	UPROPERTY(Instanced)
	UBodySetup* BodySetup;

	/** Queue of pending collision cooks */
	UPROPERTY(Transient)
	TArray<UBodySetup*> AsyncBodySetupQueue;
public:

	UFUNCTION(BlueprintCallable)
	void Initialize(URealtimeMeshProvider* Provider);

	UFUNCTION(BlueprintCallable)
	URealtimeMeshProvider* GetProvider() { return MeshProvider; }

	int32 GetNumMaterials();
	void GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials);
	UMaterialInterface* GetMaterialForSlot(int32 SlotIndex);

	FBoxSphereBounds GetLocalBounds() const;
	UBodySetup* GetBodySetup() { return nullptr;  }
	
	/** Event called when the collision has finished updated, this works both with standard following frame synchronous updates, as well as async updates */
	UPROPERTY(BlueprintAssignable, Category = "Components|RealtimeMesh")
	FRealtimeMeshCollisionUpdatedDelegate CollisionUpdated;

private:
	/** Triggers a rebuild of the collision data on the next tick */
	void MarkCollisionDirty();

	/** Helper to create new body setup objects */
	UBodySetup* CreateNewBodySetup();
	/** Mark collision data as dirty, and re-create on instance if necessary */
	void UpdateCollision(bool bForceCookNow = false);
	/** Once async physics cook is done, create needed state, and then call the user event */
	void FinishPhysicsAsyncCook(bool bSuccess, UBodySetup* FinishedBodySetup);
	/** Runs all post cook tasks like alerting the user event and alerting linked components */
	void FinalizeNewCookedData();

protected:
	bool GetPhysicsTriMeshData(struct FTriMeshCollisionData* CollisionData, bool InUseAllTriData) override;
	bool ContainsPhysicsTriMeshData(bool InUseAllTriData) const override;
private:

	void InitializeInternal();

	void RegisterLinkedComponent(URealtimeMeshComponent* NewComponent);
	void UnRegisterLinkedComponent(URealtimeMeshComponent* ComponentToRemove);
	bool GetSceneFeatureLevel(ERHIFeatureLevel::Type& OutFeatureLevel);


	void EnsureReadyToRender(ERHIFeatureLevel::Type InFeatureLevel);
	FRealtimeMeshProxyPtr GetRenderProxy(ERHIFeatureLevel::Type InFeatureLevel);

	template<typename Function>
	void DoForAllLinkedComponents(Function Func)
	{
		bool bShouldPurge = false;
		for (TWeakObjectPtr<URealtimeMeshComponent> MeshReference : LinkedComponents)
		{
			if (URealtimeMeshComponent * Mesh = MeshReference.Get())
			{
				Func(Mesh);
			}
			else
			{
				bShouldPurge = true;
			}
		}
		if (bShouldPurge)
		{
			LinkedComponents = LinkedComponents.FilterByPredicate([](const TWeakObjectPtr<URealtimeMeshComponent>& MeshReference)
				{
					return MeshReference.IsValid();
				});
		}
	}


	void UpdateAllComponentsBounds();
	void RecreateAllComponentProxies();

	virtual void MarkChanged();

	void PostLoad();

	friend class URealtimeMeshComponent;
	friend class FRealtimeMeshComponentSceneProxy;
	friend class FRealtimeMeshData;
	friend struct FRealtimeMeshCollisionCookTickObject;
};

