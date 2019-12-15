// Copyright 2016-2019 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RuntimeMeshProvider.h"
#include "RuntimeMeshProviderSphere.generated.h"

class RUNTIMEMESHCOMPONENT_API FRuntimeMeshProviderSphereProxy : public FRuntimeMeshProviderProxy
{
	float SphereRadius;
	int32 LatitudeSegmentsLOD0;
	int32 LongitudeSegmentsLOD0;
	float LODMultiplier;
	TWeakObjectPtr<UMaterialInterface> Material;
private:
	int32 MaxLOD;

	int32 GetMaximumPossibleLOD() 
	{ 
		int32 MaxLODs = FMath::Min(
			FMath::LogX(LODMultiplier, LatitudeSegmentsLOD0),
			FMath::LogX(LODMultiplier, LongitudeSegmentsLOD0));

		return FMath::Max(1, FMath::Min<int32>(MaxLODs - 1, RUNTIMEMESH_MAXLODS));
	}
	float CalculateScreenSize(int32 LODIndex)
	{
		float ScreenSize = FMath::Pow(LODMultiplier, LODIndex);

		return ScreenSize;
	}

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
	int32 LatitudeSegments;
	UPROPERTY(Category = "RuntimeMesh|Providers|Sphere", EditAnywhere, BlueprintReadWrite)
	int32 LongitudeSegments;
	UPROPERTY(Category = "RuntimeMesh|Providers|Sphere", EditAnywhere, BlueprintReadWrite)
	float LODMultiplier;

	UPROPERTY(Category = "RuntimeMesh|Providers|Sphere", EditAnywhere, BlueprintReadWrite)
	UMaterialInterface* Material;

	URuntimeMeshProviderSphere();

protected:
	virtual FRuntimeMeshProviderProxyRef GetProxy() override { return MakeShared<FRuntimeMeshProviderSphereProxy, ESPMode::ThreadSafe>(TWeakObjectPtr<URuntimeMeshProvider>(this)); }
};
