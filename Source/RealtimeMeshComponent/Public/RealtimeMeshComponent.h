// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "RealtimeMeshCore.h"
#include "RealtimeMesh.h"
#include "Components/MeshComponent.h"
#include "RealtimeMeshComponent.generated.h"


/**
*	Component that allows you to specify custom triangle mesh geometry for rendering and collision.
*/
UCLASS(ClassGroup=(Rendering, Common), HideCategories=(Object, Activation, "Components|Activation"), ShowCategories=(Mobility), Meta = (BlueprintSpawnableComponent))
class REALTIMEMESHCOMPONENT_API URealtimeMeshComponent : public UMeshComponent
{
	GENERATED_BODY()

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = RealtimeMesh, Meta = (AllowPrivateAccess = "true", DisplayName = "RealtimeMesh", ReplicatedUsing="OnRep_RealtimeMesh"))
	TObjectPtr<URealtimeMesh> RealtimeMesh;

	/**
	 * Allow this component's managed mesh geometry to auto-replicate server->clients when an
	 * optional mesh-sync layer (RealtimeMeshNetSync + the UnrealNet plugin) is available. Inert
	 * when that layer is absent: this flag only gates whether the component is offered to it.
	 * Effective only when the owner actor replicates and the mesh is a URealtimeMeshManaged type.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RealtimeMesh|Replication", Meta = (AllowPrivateAccess = "true"))
	bool bReplicateMeshData = true;

public:
	DECLARE_MULTICAST_DELEGATE_TwoParams(FRealtimeMeshComponentSyncCandidateEvent, URealtimeMeshComponent*, bool /*bCandidate*/);

	/**
	 * Fires when a component becomes (bCandidate=true) or stops being (false) a potential
	 * mesh-sync candidate: register/unregister, mesh assignment changes, and
	 * SetReplicateMeshData. Subscribers (the optional net-sync module) evaluate their own
	 * activation gates; the core module has no subscribers of its own. Game thread only.
	 */
	static FRealtimeMeshComponentSyncCandidateEvent& OnMeshSyncCandidateChanged();

	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMeshComponent")
	bool ShouldReplicateMeshData() const { return bReplicateMeshData; }

	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMeshComponent")
	void SetReplicateMeshData(bool bNewReplicateMeshData);

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RealtimeMesh")
	bool KeepMomentumOnCollisionUpdate = false;

	/**
	 * If true (default), each collision update refreshes the navigation system for this component.
	 * Disable for frequently-updated deformable collision meshes to avoid continuous navmesh tile
	 * rebuilds. The navigation update is also skipped automatically when CanEverAffectNavigation()
	 * is false regardless of this flag.
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RealtimeMesh")
	bool bUpdateNavigationOnCollisionUpdate = true;

private:
	FDelegateHandle BoundsChangedHandle;
	FDelegateHandle RenderDataChangedHandle;
	FDelegateHandle CollisionBodyUpdatedHandle;

	// Cached mesh-space local bounds. CalcBounds is called every UpdateBounds (per movement/physics
	// tick) and otherwise takes the whole-mesh read lock via URealtimeMesh::GetLocalBounds. The cache
	// is refreshed lazily and invalidated whenever the mesh's bounds actually change (OnBoundsChanged)
	// or the mesh is swapped, so the read lock is only taken when the bounds genuinely change.
	mutable FBoxSphereBounds CachedLocalBounds = FBoxSphereBounds(FSphere(FVector::ZeroVector, 1));
	mutable bool bCachedLocalBoundsValid = false;

public:

	URealtimeMeshComponent();

	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMeshComponent")
	void SetRealtimeMesh(URealtimeMesh* NewMesh);

	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMeshComponent", meta=(DeterminesOutputType="MeshClass"))
	URealtimeMesh* InitializeRealtimeMesh(UPARAM(meta = (AllowAbstract = "false")) TSubclassOf<URealtimeMesh> MeshClass);

	template <typename MeshType>
	MeshType* InitializeRealtimeMesh(TSubclassOf<URealtimeMesh> MeshClass = MeshType::StaticClass())
	{
		return CastChecked<MeshType>(InitializeRealtimeMesh(MeshClass));
	}

	/** Returns the RealtimeMesh associated with this component, or nullptr if none is set. */
	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMeshComponent")
	URealtimeMesh* GetRealtimeMesh() const
	{
		if (IsValid(RealtimeMesh))
		{
			return RealtimeMesh;
		}
		return nullptr;
	}

	template<typename RealtimeMeshType>
	RealtimeMeshType* GetRealtimeMeshAs() const
	{
		if (IsValid(RealtimeMesh))
		{
			return CastChecked<RealtimeMeshType>(RealtimeMesh);
		}
		return nullptr;
	}
	
	UFUNCTION()
	void OnRep_RealtimeMesh(class URealtimeMesh *OldRealtimeMesh);

public:
	void GetStreamingRenderAssetInfo(FStreamingTextureLevelContext& LevelContext, TArray<FStreamingRenderAssetPrimitiveInfo>& OutStreamingRenderAssets) const override
	{
		// NOTE (IDIOM-008): RealtimeMesh does not register per-asset texture-streaming
		// info; it defers to the base PrimitiveComponent implementation. Proper texture
		// streaming would populate OutStreamingRenderAssets from the mesh's materials here.
		return Super::GetStreamingRenderAssetInfo(LevelContext, OutStreamingRenderAssets);
	}

	virtual void OnRegister() override;
	virtual void OnUnregister() override;


	//~ Begin USceneComponent Interface.
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;

	virtual bool IsSupportedForNetworking() const override
	{
		return true;
	}

	//~ Begin USceneComponent Interface.

	//~ Begin UPrimitiveComponent Interface.
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	virtual class UBodySetup* GetBodySetup() override;
	//~ End UPrimitiveComponent Interface.
public:
	//~ Begin UMeshComponent Interface
	virtual int32 GetMaterialIndex(FName MaterialSlotName) const override;
	virtual FName GetMaterialSlotName(uint32 Index) const;
	virtual TArray<FName> GetMaterialSlotNames() const override;
	virtual bool IsMaterialSlotNameValid(FName MaterialSlotName) const override;
	virtual void GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials = false) const override;
	//~ End UMeshComponent Interface

	//~ Being UPrimitiveComponent Interface
	virtual int32 GetNumMaterials() const override;
	virtual UMaterialInterface* GetMaterial(int32 ElementIndex) const override;
	virtual void CollectPSOPrecacheData(const FPSOPrecacheParams& BasePrecachePSOParams, FMaterialInterfacePSOPrecacheParamsList& OutParams) override;
	//~ End UPrimitiveComponent Interface

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	virtual void BindToEvents(URealtimeMesh* RealtimeMesh);
	virtual void UnbindFromEvents(URealtimeMesh* RealtimeMesh);

	virtual void HandleBoundsUpdated(URealtimeMesh* IncomingMesh);
	virtual void HandleMeshRenderingDataChanged(URealtimeMesh* IncomingMesh, bool bShouldProxyRecreate);
	virtual void HandleCollisionBodyUpdated(URealtimeMesh* RealtimeMesh, UBodySetup* BodySetup);

	virtual void UpdateCollision();

	friend class FRealtimeMeshDetailsCustomization;
};
