// Copyright 2016-2019 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RealtimeMeshProvider.h"
#include "RealtimeMeshProviderBox.generated.h"

class REALTIMEMESHCOMPONENT_API FRealtimeMeshProviderBoxProxy : public FRealtimeMeshProviderProxy
{
	const FVector BoxRadius;
	UMaterialInterface* Material;

public:
	FRealtimeMeshProviderBoxProxy(TWeakObjectPtr<URealtimeMeshProvider> InParent, const FVector& InBoxRadius, UMaterialInterface* InMaterial);
	~FRealtimeMeshProviderBoxProxy();

	virtual void Initialize() override;

	virtual bool GetSectionMeshForLOD(uint8 LODIndex, int32 SectionId, FRealtimeMeshRenderableMeshData& MeshData) override;

	FRealtimeMeshCollisionSettings GetCollisionSettings() override;
	bool HasCollisionMesh() override;
	virtual bool GetCollisionMesh(FRealtimeMeshCollisionData& CollisionData) override;

	virtual FBoxSphereBounds GetBounds() override { return FBoxSphereBounds(FBox(-BoxRadius, BoxRadius)); }

	bool IsThreadSafe() const override;

};

UCLASS(HideCategories = Object, BlueprintType)
class REALTIMEMESHCOMPONENT_API URealtimeMeshProviderBox : public URealtimeMeshProvider
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere)
	FVector BoxRadius;

	UPROPERTY(EditAnywhere)
	UMaterialInterface* Material;


	virtual IRealtimeMeshProviderProxyRef GetProxy() override { return MakeShared<FRealtimeMeshProviderBoxProxy, ESPMode::ThreadSafe>(TWeakObjectPtr<URealtimeMeshProvider>(this), BoxRadius, Material); }
};
