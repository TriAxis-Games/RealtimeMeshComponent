// Copyright 2016-2019 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RuntimeMeshCore.h"
#include "RuntimeMeshCollision.h"
#include "RuntimeMeshProvider.h"
#include "RuntimeMesh.generated.h"

class URuntimeMesh;
class URuntimeMeshComponent;

class FRuntimeMeshData;
using FRuntimeMeshDataPtr = TSharedPtr<FRuntimeMeshData, ESPMode::ThreadSafe>;
class FRuntimeMeshProxy;
using FRuntimeMeshProxyPtr = TSharedPtr<FRuntimeMeshProxy, ESPMode::ThreadSafe>;

/*
*	This tick function is used to drive the collision cooker.
*	It is enabled for one frame when we need to update collision.
*	This keeps from cooking on each individual create/update section as the original PMC did
*/
struct FRuntimeMeshCollisionCookTickObject : FTickableGameObject
{
	TWeakObjectPtr<URuntimeMesh> Owner;

	FRuntimeMeshCollisionCookTickObject(TWeakObjectPtr<URuntimeMesh> InOwner) : Owner(InOwner) {}
	virtual void Tick(float DeltaTime);
	virtual bool IsTickable() const;
	virtual bool IsTickableInEditor() const { return true; }
	virtual TStatId GetStatId() const;

	virtual UWorld* GetTickableGameObjectWorld() const;
};

/**
*	Delegate for when the collision was updated.
*/
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FRuntimeMeshCollisionUpdatedDelegate);


UCLASS(HideCategories = Object, BlueprintType)
class RUNTIMEMESHCOMPONENT_API URuntimeMesh : public UObject, public IInterface_CollisionDataProvider
{
	GENERATED_UCLASS_BODY()

private:
	/** Reference to the underlying data object */
	FRuntimeMeshDataPtr Data;

	/** All RuntimeMeshComponents linked to this mesh. Used to alert the components of changes */
	TArray<TWeakObjectPtr<URuntimeMeshComponent>> LinkedComponents;

	UPROPERTY()
	URuntimeMeshProvider* MeshProvider;

	/** Do we need to update our collision? */
	bool bCollisionIsDirty;

	/** Object used to tick the collision cooking at the end of the frame */
	TUniquePtr<FRuntimeMeshCollisionCookTickObject> CookTickObject;

	/** Collision data */
	UPROPERTY(Instanced)
	UBodySetup* BodySetup;

	/** Queue of pending collision cooks */
	UPROPERTY(Transient)
	TArray<UBodySetup*> AsyncBodySetupQueue;
public:

	UFUNCTION(BlueprintCallable)
	void Initialize(URuntimeMeshProvider* Provider);

	UFUNCTION(BlueprintCallable)
	URuntimeMeshProvider* GetProvider() { return MeshProvider; }

	int32 GetNumMaterials();
	void GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials);
	UMaterialInterface* GetMaterialForSlot(int32 SlotIndex);

	FBoxSphereBounds GetLocalBounds() const;
	UBodySetup* GetBodySetup() { return nullptr;  }
	
	/** Event called when the collision has finished updated, this works both with standard following frame synchronous updates, as well as async updates */
	UPROPERTY(BlueprintAssignable, Category = "Components|RuntimeMesh")
	FRuntimeMeshCollisionUpdatedDelegate CollisionUpdated;

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

	void RegisterLinkedComponent(URuntimeMeshComponent* NewComponent);
	void UnRegisterLinkedComponent(URuntimeMeshComponent* ComponentToRemove);
	bool GetSceneFeatureLevel(ERHIFeatureLevel::Type& OutFeatureLevel);


	void EnsureReadyToRender(ERHIFeatureLevel::Type InFeatureLevel);
	FRuntimeMeshProxyPtr GetRenderProxy(ERHIFeatureLevel::Type InFeatureLevel);

	template<typename Function>
	void DoForAllLinkedComponents(Function Func)
	{
		bool bShouldPurge = false;
		for (TWeakObjectPtr<URuntimeMeshComponent> MeshReference : LinkedComponents)
		{
			if (URuntimeMeshComponent * Mesh = MeshReference.Get())
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
			LinkedComponents = LinkedComponents.FilterByPredicate([](const TWeakObjectPtr<URuntimeMeshComponent>& MeshReference)
				{
					return MeshReference.IsValid();
				});
		}
	}


	void UpdateAllComponentsBounds();
	void RecreateAllComponentProxies();

	virtual void MarkChanged();

	void PostLoad();

	friend class URuntimeMeshComponent;
	friend class FRuntimeMeshComponentSceneProxy;
	friend class FRuntimeMeshData;
	friend struct FRuntimeMeshCollisionCookTickObject;
};

