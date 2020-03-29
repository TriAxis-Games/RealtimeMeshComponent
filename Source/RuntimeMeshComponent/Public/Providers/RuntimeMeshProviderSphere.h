// Copyright 2016-2019 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RuntimeMeshProvider.h"
#include "RuntimeMeshProviderSphere.generated.h"

class RUNTIMEMESHCOMPONENT_API FRuntimeMeshProviderSphereProxy : public FRuntimeMeshProviderProxy
{
	float SphereRadius;
	int32 MaxLatitudeSegments;
	int32 MinLatitudeSegments;
	int32 MaxLongitudeSegments;
	int32 MinLongitudeSegments;
	float LODMultiplier;
	TWeakObjectPtr<UMaterialInterface> Material;
private:
	int32 MaxLOD;

	int32 GetMaxNumberOfLODs();
	float CalculateScreenSize(int32 LODIndex);

	bool GetSphereMesh(int32 LatitudeSegments, int32 LongitudeSegments, FRuntimeMeshRenderableMeshData& MeshData);
public:
	FRuntimeMeshProviderSphereProxy(TWeakObjectPtr<URuntimeMeshProvider> InParent);
	~FRuntimeMeshProviderSphereProxy();

	void UpdateProxyParameters(URuntimeMeshProvider* ParentProvider, bool bIsInitialSetup) override;

	virtual void Initialize() override;

	virtual bool GetSectionMeshForLOD(int32 LODIndex, int32 SectionId, FRuntimeMeshRenderableMeshData& MeshData) override;

	FRuntimeMeshCollisionSettings GetCollisionSettings() override;
	bool HasCollisionMesh() override;
	virtual bool GetCollisionMesh(FRuntimeMeshCollisionData& CollisionData) override;

	virtual FBoxSphereBounds GetBounds() override { return FBoxSphereBounds(FSphere(FVector::ZeroVector, SphereRadius)); }

	bool IsThreadSafe() const override { return true; }
};

UCLASS(HideCategories = Object, BlueprintType)
class RUNTIMEMESHCOMPONENT_API URuntimeMeshProviderSphere : public URuntimeMeshProvider
{
	GENERATED_BODY()

public:
	UPROPERTY(Category = "RuntimeMesh|Providers|Sphere", EditAnywhere, BlueprintReadWrite)
	float SphereRadius;

	UPROPERTY(Category = "RuntimeMesh|Providers|Sphere", EditAnywhere, BlueprintReadWrite)
	int32 MaxLatitudeSegments;
	UPROPERTY(Category = "RuntimeMesh|Providers|Sphere", EditAnywhere, BlueprintReadWrite)
	int32 MinLatitudeSegments;
	UPROPERTY(Category = "RuntimeMesh|Providers|Sphere", EditAnywhere, BlueprintReadWrite)
	int32 MaxLongitudeSegments;
	UPROPERTY(Category = "RuntimeMesh|Providers|Sphere", EditAnywhere, BlueprintReadWrite)
	int32 MinLongitudeSegments;


	UPROPERTY(Category = "RuntimeMesh|Providers|Sphere", EditAnywhere, BlueprintReadWrite)
	float LODMultiplier;

	UPROPERTY(Category = "RuntimeMesh|Providers|Sphere", EditAnywhere, BlueprintReadWrite)
	UMaterialInterface* Material;

	URuntimeMeshProviderSphere();

protected:
	virtual FRuntimeMeshProviderProxyRef GetProxy() override { return MakeShared<FRuntimeMeshProviderSphereProxy, ESPMode::ThreadSafe>(TWeakObjectPtr<URuntimeMeshProvider>(this)); }
};
