// Copyright 2016-2019 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RuntimeMeshProvider.h"
#include "RuntimeMeshProviderStatic.generated.h"


class RUNTIMEMESHCOMPONENT_API FRuntimeMeshProviderStaticProxy : public FRuntimeMeshProviderProxy
{
	const FVector BoxRadius;
	UMaterialInterface* Material;

public:
	FRuntimeMeshProviderStaticProxy(TWeakObjectPtr<URuntimeMeshProvider> InParent, const FVector& InBoxRadius, UMaterialInterface* InMaterial);
	~FRuntimeMeshProviderStaticProxy();

// 	virtual void Initialize() override;
// 
// 	virtual bool GetSectionMeshForLOD(uint8 LODIndex, int32 SectionId, FRuntimeMeshRenderableMeshData& MeshData) override;
// 
// 	FRuntimeMeshCollisionSettings GetCollisionSettings() override;
// 	bool HasCollisionMesh() override;
// 	virtual bool GetCollisionMesh(FRuntimeMeshCollisionData& CollisionData) override;
// 
// 	virtual FBoxSphereBounds GetBounds() override { return FBoxSphereBounds(FBox(-BoxRadius, BoxRadius)); }
// 
// 	bool IsThreadSafe() const override;

};

UCLASS(HideCategories = Object, BlueprintType)
class RUNTIMEMESHCOMPONENT_API URuntimeMeshProviderStatic : public URuntimeMeshProvider
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere)
	FVector BoxRadius;

	UPROPERTY(EditAnywhere)
	UMaterialInterface* Material;


	virtual IRuntimeMeshProviderProxyRef GetProxy() override { return MakeShared<FRuntimeMeshProviderStaticProxy, ESPMode::ThreadSafe>(TWeakObjectPtr<URuntimeMeshProvider>(this), BoxRadius, Material); }
};