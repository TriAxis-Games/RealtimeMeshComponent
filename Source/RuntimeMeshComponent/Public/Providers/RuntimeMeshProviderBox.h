// Copyright 2016-2019 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RuntimeMeshProvider.h"
#include "RuntimeMeshProviderBox.generated.h"

class RUNTIMEMESHCOMPONENT_API FRuntimeMeshProviderBoxProxy : public FRuntimeMeshProviderProxy
{
	const FVector BoxRadius;
	UMaterialInterface* Material;

public:
	FRuntimeMeshProviderBoxProxy(TWeakObjectPtr<URuntimeMeshProvider> InParent, const FVector& InBoxRadius, UMaterialInterface* InMaterial);
	~FRuntimeMeshProviderBoxProxy();

	virtual void Initialize() override;

	virtual bool GetSectionMeshForLOD(uint8 LODIndex, int32 SectionId, FRuntimeMeshRenderableMeshData& MeshData) override;

	FRuntimeMeshCollisionSettings GetCollisionSettings() override;
	bool HasCollisionMesh() override;
	virtual bool GetCollisionMesh(FRuntimeMeshCollisionData& CollisionData) override;

	virtual FBoxSphereBounds GetBounds() override { return FBoxSphereBounds(FBox(-BoxRadius, BoxRadius)); }

	bool IsThreadSafe() const override;

};

UCLASS(HideCategories = Object, BlueprintType)
class RUNTIMEMESHCOMPONENT_API URuntimeMeshProviderBox : public URuntimeMeshProvider
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere)
	FVector BoxRadius;

	UPROPERTY(EditAnywhere)
	UMaterialInterface* Material;


	virtual IRuntimeMeshProviderProxyRef GetProxy() override { return MakeShared<FRuntimeMeshProviderBoxProxy, ESPMode::ThreadSafe>(TWeakObjectPtr<URuntimeMeshProvider>(this), BoxRadius, Material); }
};
