// Copyright 2016-2020 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RuntimeMeshProvider.h"
#include "RuntimeMeshProviderBox.generated.h"

class RUNTIMEMESHCOMPONENT_API FRuntimeMeshProviderBoxProxy : public FRuntimeMeshProviderProxy
{
	FVector BoxRadius;
	TWeakObjectPtr<UMaterialInterface> Material;

public:
	FRuntimeMeshProviderBoxProxy(TWeakObjectPtr<URuntimeMeshProvider> InParent);
	~FRuntimeMeshProviderBoxProxy();

	void UpdateProxyParameters(URuntimeMeshProvider* ParentProvider, bool bIsInitialSetup) override;

	virtual void Initialize() override;

	virtual bool GetSectionMeshForLOD(int32 LODIndex, int32 SectionId, FRuntimeMeshRenderableMeshData& MeshData) override;

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
	UPROPERTY(Category = "RuntimeMesh|Providers|Box", EditAnywhere, BlueprintReadWrite)
	FVector BoxRadius;

	UPROPERTY(Category = "RuntimeMesh|Providers|Box", EditAnywhere, BlueprintReadWrite)
	UMaterialInterface* Material;

protected:
	virtual FRuntimeMeshProviderProxyRef GetProxy() override { return MakeShared<FRuntimeMeshProviderBoxProxy, ESPMode::ThreadSafe>(TWeakObjectPtr<URuntimeMeshProvider>(this)); }
};
