// Copyright TriAxis Games, L.L.C. All Rights Reserved.

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
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = RealtimeMesh, Meta = (AllowPrivateAccess = "true", DisplayName = "Runtime Mesh"))
	TObjectPtr<URealtimeMesh> RealtimeMeshReference;

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RealtimeMesh")
	bool KeepMomentumOnCollisionUpdate = false;

	URealtimeMeshComponent();

	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMeshComponent")
	void SetRealtimeMesh(URealtimeMesh* NewMesh);

	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMeshComponent", meta=(DeterminesOutputType="MeshClass"))
	URealtimeMesh* InitializeRealtimeMesh(TSubclassOf<URealtimeMesh> MeshClass);

	template <typename MeshType>
	MeshType* InitializeRealtimeMesh(TSubclassOf<URealtimeMesh> MeshClass = MeshType::StaticClass())
	{
		return CastChecked<MeshType>(InitializeRealtimeMesh(MeshClass));
	}

	/** Clears the geometry for ALL collision only sections */
	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMeshComponent")
	FORCEINLINE URealtimeMesh* GetRealtimeMesh() const
	{
		if (RealtimeMeshReference && RealtimeMeshReference->IsValidLowLevel())
		{
			return RealtimeMeshReference;
		}
		return nullptr;
	}

	template<typename RealtimeMeshType>
	FORCEINLINE RealtimeMeshType* GetRealtimeMeshAs() const
	{
		if (RealtimeMeshReference && RealtimeMeshReference->IsValidLowLevel())
		{
			return CastChecked<RealtimeMeshType>(RealtimeMeshReference);
		}
		return nullptr;
	}

public:
	void GetStreamingRenderAssetInfo(FStreamingTextureLevelContext& LevelContext, TArray<FStreamingRenderAssetPrimitiveInfo>& OutStreamingRenderAssets) const
	{
		//@TODO: Need to support this for proper texture streaming
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
	virtual TArray<FName> GetMaterialSlotNames() const override;
	virtual bool IsMaterialSlotNameValid(FName MaterialSlotName) const override;
	virtual void GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials = false) const override;
	//~ End UMeshComponent Interface

	//~ Being UPrimitiveComponent Interface
	virtual int32 GetNumMaterials() const override;
	virtual UMaterialInterface* GetMaterial(int32 ElementIndex) const override;
	//~ End UPrimitiveComponent Interface


private:
	virtual void BindToEvents(URealtimeMesh* RealtimeMesh);
	virtual void UnbindFromEvents(URealtimeMesh* RealtimeMesh);

	virtual void HandleBoundsUpdated(URealtimeMesh* IncomingMesh);
	virtual void HandleMeshRenderingDataChanged(URealtimeMesh* IncomingMesh, bool bShouldProxyRecreate);
	virtual void HandleCollisionBodyUpdated(URealtimeMesh* RealtimeMesh, UBodySetup* BodySetup);

	virtual void UpdateCollision();
};
