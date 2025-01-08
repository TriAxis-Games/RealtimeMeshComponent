// Copyright (c) 2015-2025 TriAxis Games, L.L.C. All Rights Reserved.

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

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RealtimeMesh")
	bool KeepMomentumOnCollisionUpdate = false;

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

	/** Clears the geometry for ALL collision only sections */
	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMeshComponent")
	FORCEINLINE URealtimeMesh* GetRealtimeMesh() const
	{
		if (RealtimeMesh && RealtimeMesh->IsValidLowLevel())
		{
			return RealtimeMesh;
		}
		return nullptr;
	}

	template<typename RealtimeMeshType>
	FORCEINLINE RealtimeMeshType* GetRealtimeMeshAs() const
	{
		if (RealtimeMesh && RealtimeMesh->IsValidLowLevel())
		{
			return CastChecked<RealtimeMeshType>(RealtimeMesh);
		}
		return nullptr;
	}
	
	UFUNCTION()
	void OnRep_RealtimeMesh(class URealtimeMesh *OldRealtimeMesh);

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
	
	virtual bool UseNaniteOverrideMaterials() const override;
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
#if RMC_ENGINE_ABOVE_5_4
	virtual void CollectPSOPrecacheData(const FPSOPrecacheParams& BasePrecachePSOParams, FMaterialInterfacePSOPrecacheParamsList& OutParams) override;
#endif
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
