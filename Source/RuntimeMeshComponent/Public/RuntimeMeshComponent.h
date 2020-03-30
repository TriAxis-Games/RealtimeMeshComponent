// Copyright 2016-2019 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "Components/MeshComponent.h"
#include "RuntimeMeshCore.h"
#include "PhysicsEngine/ConvexElem.h"
#include "RuntimeMesh.h"
#include "Interfaces/Interface_CollisionDataProvider.h"
#include "RuntimeMeshComponent.generated.h"


/**
*	Component that allows you to specify custom triangle mesh geometry for rendering and collision.
*/
UCLASS(ClassGroup=(Rendering, Common), HideCategories=(Object, Activation, "Components|Activation"), ShowCategories=(Mobility), Meta = (BlueprintSpawnableComponent))
class RUNTIMEMESHCOMPONENT_API URuntimeMeshComponent : public UMeshComponent, public IInterface_CollisionDataProvider
{
	GENERATED_BODY()

private:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = RuntimeMesh, Meta = (AllowPrivateAccess = "true", DisplayName = "Runtime Mesh"))
	URuntimeMesh* RuntimeMeshReference;

	void EnsureHasRuntimeMesh();

public:

	URuntimeMeshComponent(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable)
	void Initialize(URuntimeMeshProvider* Provider)
	{
		GetOrCreateRuntimeMesh()->Initialize(Provider);
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void SetRuntimeMesh(URuntimeMesh* NewMesh);

	/** Clears the geometry for ALL collision only sections */
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	FORCEINLINE URuntimeMesh* GetRuntimeMesh() const
	{
		return RuntimeMeshReference;
	}

	/** Clears the geometry for ALL collision only sections */
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	FORCEINLINE URuntimeMesh* GetOrCreateRuntimeMesh()
	{
		EnsureHasRuntimeMesh();

		return RuntimeMeshReference;
	}



	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh", Meta = (AllowPrivateAccess = "true", DisplayName = "Get Mobility"))
	ERuntimeMeshMobility GetRuntimeMeshMobility()
	{
		return Mobility == EComponentMobility::Movable ? ERuntimeMeshMobility::Movable :
			Mobility == EComponentMobility::Stationary ? ERuntimeMeshMobility::Stationary : ERuntimeMeshMobility::Static;
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh", Meta = (AllowPrivateAccess = "true", DisplayName = "Set Mobility"))
	void SetRuntimeMeshMobility(ERuntimeMeshMobility NewMobility)
	{
		Super::SetMobility(
			NewMobility == ERuntimeMeshMobility::Movable ? EComponentMobility::Movable :
			NewMobility == ERuntimeMeshMobility::Stationary ? EComponentMobility::Stationary : EComponentMobility::Static);
	}


	UFUNCTION(BlueprintCallable)
	FRuntimeMeshCollisionHitInfo GetHitSource(int32 FaceIndex) const;


	static void InitializeMultiThreading(int32 NumThreads, int32 StackSize = 0, EThreadPriority ThreadPriority = TPri_BelowNormal);

	static FRuntimeMeshBackgroundWorkDelegate InitializeUserSuppliedThreading();

private:

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
	virtual int32 GetMaterialIndex(FName MaterialSlotName) const;
	virtual TArray<FName> GetMaterialSlotNames() const;
	virtual bool IsMaterialSlotNameValid(FName MaterialSlotName) const;
	//~ End UMeshComponent Interface

	//~ Being UPrimitiveComponent Interface
	virtual int32 GetNumMaterials() const override;
	virtual UMaterialInterface* GetMaterial(int32 ElementIndex) const override;
	//~ End UPrimitiveComponent Interface


public:
	//~ Begin UMeshComponent Interface.
	//~ End UMeshComponent Interface.

private:

	/* Does post load fixups */
	virtual void PostLoad() override;



	/** Called by URuntimeMesh any time it has new collision data that we should use */
	void NewCollisionMeshReceived();
	void NewBoundsReceived();
	void ForceProxyRecreate();




	friend class URuntimeMesh;
	friend class FRuntimeMeshComponentSceneProxy;
};
