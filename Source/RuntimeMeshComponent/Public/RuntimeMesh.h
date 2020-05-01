// Copyright 2016-2020 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RuntimeMeshCore.h"
#include "RuntimeMeshCollision.h"
#include "RuntimeMeshProvider.h"
#include "RuntimeMeshReference.h"
#include "RuntimeMesh.generated.h"

class URuntimeMesh;
class URuntimeMeshComponent;

class FRuntimeMeshProxy;
using FRuntimeMeshProxyPtr = TSharedPtr<FRuntimeMeshProxy, ESPMode::ThreadSafe>;

class URuntimeMeshProviderStatic;
class URuntimeMeshComponentEngineSubsystem;

enum class ESectionUpdateType : uint8
{
	None = 0x0,
	Properties = 0x1,
	Mesh = 0x2,
	Clear = 0x4,
	Remove = 0x8,


	AllData = Properties | Mesh,
	ClearOrRemove = Clear | Remove,
};

ENUM_CLASS_FLAGS(ESectionUpdateType);


/**
*	Delegate for when the collision was updated.
*/
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FRuntimeMeshCollisionUpdatedDelegate);


UCLASS(HideCategories = Object, BlueprintType)
class RUNTIMEMESHCOMPONENT_API URuntimeMesh 
	: public UObject
	, public IRuntimeMeshProviderTargetInterface
	, public IInterface_CollisionDataProvider
{
	GENERATED_UCLASS_BODY()

private:

	// State tracking for async thread synchronization
	FThreadSafeBool bQueuedForMeshUpdate;

	// Whether this mesh needs to be initialized by the tick object. 
	// This is to get away from postload so BP calls in the 
	// provider are safe 
	bool bNeedsInitialization;

	// Do we need to update our collision?
	bool bCollisionIsDirty;

	UPROPERTY()
	URuntimeMeshProvider* MeshProvider;

	UPROPERTY(Transient)
	UBodySetup* BodySetup;

	UPROPERTY()
	TArray<FRuntimeMeshCollisionSourceSectionInfo> CollisionSource;

	// Queue of pending collision cooks
	UPROPERTY(Transient)
	TArray<FRuntimeMeshAsyncBodySetupData> AsyncBodySetupQueue;

	//UPROPERTY(Transient)
	TUniquePtr<TArray<FRuntimeMeshCollisionSourceSectionInfo>> PendingSourceInfo;

	// All the LOD configuration data, within the lods is the section configuration data
	TArray<FRuntimeMeshLOD, TInlineAllocator<RUNTIMEMESH_MAXLODS>> LODs;

	// This is the proxy object on the render thread
	FRuntimeMeshProxyPtr RenderProxy;

	// All components currently linked to this
	TArray<TWeakObjectPtr<URuntimeMeshComponent>> LinkedComponents;

	// We track all registered material slots and a lookup table to quickly index them
	UPROPERTY()
	TArray<FRuntimeMeshMaterialSlot> MaterialSlots;
	UPROPERTY()
	TMap<FName, int32> SlotNameLookup;

	// Thread synchronization for the LOD/Material data
	mutable FCriticalSection SyncRoot;

	// Sections that are waiting for an update
	TMap<int32, TMap<int32, ESectionUpdateType>> SectionsToUpdate;

	// This is a GC safety construct that allows threaded referencing of this object
	// We block the GC if shared references exist
	// and stop new shared references from being created if it's been marked for collection
	FRuntimeMeshReferenceAnchor<URuntimeMesh> ReferenceAnchor;

public:

	UFUNCTION(BlueprintCallable)
	void Initialize(URuntimeMeshProvider* Provider);

	UFUNCTION(BlueprintCallable)
	URuntimeMeshProvider* GetProvider() { return MeshProvider; }

	UFUNCTION(BlueprintCallable)
	void Reset();

	UFUNCTION(BlueprintCallable)
	FRuntimeMeshCollisionHitInfo GetHitSource(int32 FaceIndex) const;
public:

	/** Event called when the collision has finished updated, this works both with standard following frame synchronous updates, as well as async updates */
	UPROPERTY(BlueprintAssignable, Category = "Components|RuntimeMesh")
	FRuntimeMeshCollisionUpdatedDelegate CollisionUpdated;

	UFUNCTION(BlueprintCallable)
	FBoxSphereBounds GetLocalBounds() const { return MeshProvider ? MeshProvider->GetBounds() : FBoxSphereBounds(FSphere(FVector::ZeroVector, 1.0f)); }

	UFUNCTION(BlueprintCallable)
	UBodySetup* GetBodySetup() { return BodySetup; }


	//	Begin IRuntimeMeshProviderTargetInterface interface
	virtual FRuntimeMeshWeakRef GetMeshReference() override { return ReferenceAnchor.GetReference(false); }
	virtual void ShutdownInternal() override;

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

	//	End IRuntimeMeshProviderTargetInterface interface

	TArray<FRuntimeMeshLOD, TInlineAllocator<RUNTIMEMESH_MAXLODS>> GetCopyOfConfiguration() const { return LODs; }

public:

	//	Begin UObject interface
	virtual void BeginDestroy() override;
	virtual bool IsReadyForFinishDestroy() override;
	virtual void PostLoad() override;
	//	End UObject interface

	//	Begin IInterface_CollisionDataProvider interface
	virtual bool GetPhysicsTriMeshData(struct FTriMeshCollisionData* CollisionData, bool InUseAllTriData) override;
	virtual bool ContainsPhysicsTriMeshData(bool InUseAllTriData) const override;
	virtual bool WantsNegXTriMesh() override;
	virtual void GetMeshId(FString& OutMeshId) override;
	//	End IInterface_CollisionDataProvider interface



private:
	void InitializeInternal();

	void QueueForDelayedInitialize();
	void QueueForUpdate();
	void QueueForMeshUpdate();
	void QueueForCollisionUpdate();

	void UpdateAllComponentBounds();
	void RecreateAllComponentSceneProxies();

	void HandleUpdate();
	void HandleFullLODUpdate(const FRuntimeMeshProxyPtr& RenderProxyRef, int32 LODId, bool& bRequiresProxyRecreate);
	void HandleSingleSectionUpdate(const FRuntimeMeshProxyPtr& RenderProxyRef, int32 LODId, int32 SectionId, bool& bRequiresProxyRecreate);

	static URuntimeMeshComponentEngineSubsystem* GetEngineSubsystem();

private:	// Collision
	// Helper to create new body setup objects
	UBodySetup* CreateNewBodySetup();
	// Mark collision data as dirty, and re-create on instance if necessary
	void UpdateCollision(bool bForceCookNow = false);
	// Once async physics cook is done, create needed state, and then call the user event
	void FinishPhysicsAsyncCook(bool bSuccess, UBodySetup* FinishedBodySetup);
	// Runs all post cook tasks like alerting the user event and alerting linked components
	void FinalizeNewCookedData();

private:	// Linked Components
	void RegisterLinkedComponent(URuntimeMeshComponent* NewComponent);
	void UnRegisterLinkedComponent(URuntimeMeshComponent* ComponentToRemove);

	template<typename Function>
	void DoForAllLinkedComponents(Function Func)
	{
		bool bShouldPurge = false;
		for (TWeakObjectPtr<URuntimeMeshComponent> MeshReference : LinkedComponents)
		{
			if (URuntimeMeshComponent* Mesh = MeshReference.Get())
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

private:	// Render Proxy
	FRuntimeMeshProxyPtr GetRenderProxy(ERHIFeatureLevel::Type InFeatureLevel);
	bool GetSceneFeatureLevel(ERHIFeatureLevel::Type& OutFeatureLevel);

	




	friend class URuntimeMeshComponent;
	friend class FRuntimeMeshComponentSceneProxy;
	friend class URuntimeMeshComponentEngineSubsystem;
	friend class FRuntimeMeshUpdateTask;
};

