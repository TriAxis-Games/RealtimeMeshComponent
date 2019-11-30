// Copyright 2016-2019 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "Components/MeshComponent.h"
#include "RealtimeMeshCore.h"
#include "PhysicsEngine/ConvexElem.h"
#include "RealtimeMesh.h"
#include "Interfaces/Interface_CollisionDataProvider.h"
#include "RealtimeMeshComponent.generated.h"


/**
*	Component that allows you to specify custom triangle mesh geometry for rendering and collision.
*/
UCLASS(HideCategories = (Object, LOD), Meta = (BlueprintSpawnableComponent))
class REALTIMEMESHCOMPONENT_API URealtimeMeshComponent : public UMeshComponent, public IInterface_CollisionDataProvider
{
	GENERATED_BODY()

private:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = RealtimeMesh, Meta = (AllowPrivateAccess = "true", DisplayName = "Runtime Mesh"))
	URealtimeMesh* RealtimeMeshReference;

	void EnsureHasRealtimeMesh();

public:

	URealtimeMeshComponent(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh")
	void SetRealtimeMesh(URealtimeMesh* NewMesh);

	/** Clears the geometry for ALL collision only sections */
	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh")
	FORCEINLINE URealtimeMesh* GetRealtimeMesh() const
	{
		return RealtimeMeshReference;
	}

	/** Clears the geometry for ALL collision only sections */
	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh")
	FORCEINLINE URealtimeMesh* GetOrCreateRealtimeMesh()
	{
		EnsureHasRealtimeMesh();

		return RealtimeMeshReference;
	}



	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh", Meta = (AllowPrivateAccess = "true", DisplayName = "Get Mobility"))
	ERealtimeMeshMobility GetRealtimeMeshMobility()
	{
		return Mobility == EComponentMobility::Movable ? ERealtimeMeshMobility::Movable :
			Mobility == EComponentMobility::Stationary ? ERealtimeMeshMobility::Stationary : ERealtimeMeshMobility::Static;
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh", Meta = (AllowPrivateAccess = "true", DisplayName = "Set Mobility"))
	void SetRealtimeMeshMobility(ERealtimeMeshMobility NewMobility)
	{
		Super::SetMobility(
			NewMobility == ERealtimeMeshMobility::Movable ? EComponentMobility::Movable :
			NewMobility == ERealtimeMeshMobility::Stationary ? EComponentMobility::Stationary : EComponentMobility::Static);
	}




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

	//~ Begin UMeshComponent Interface.
	virtual int32 GetNumMaterials() const override;
	virtual void GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials = false) const override;
	virtual UMaterialInterface* GetMaterial(int32 ElementIndex) const override;
	virtual UMaterialInterface* GetOverrideMaterial(int32 ElementIndex) const;
	//~ End UMeshComponent Interface.


public:
	//~ Begin UMeshComponent Interface.
	//~ End UMeshComponent Interface.

private:

	/* Does post load fixups */
	virtual void PostLoad() override;



	/** Called by URealtimeMesh any time it has new collision data that we should use */
	void NewCollisionMeshReceived();
	void NewBoundsReceived();
	void ForceProxyRecreate();




	friend class URealtimeMesh;
	friend class FRealtimeMeshComponentSceneProxy;
};
