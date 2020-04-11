// Copyright 2016-2019 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RuntimeMeshProvider.h"
#include "RuntimeMeshProviderPlane.generated.h"

/*
A - - - - B		Supply with A, B and C, D will be computed
|        /|		
       /  |
|    /    |
   /      |
|/        |
C - - - - D
*/
class RUNTIMEMESHCOMPONENT_API FRuntimeMeshProviderPlaneProxy : public FRuntimeMeshProviderProxy
{
	FVector LocationA, LocationB, LocationC;
	TArray<int32> VertsAB;
	TArray<int32> VertsAC;
	TArray<float> ScreenSize;
	TWeakObjectPtr<UMaterialInterface> Material;
private:
	int32 MaxLOD;

	int32 GetMaximumPossibleLOD() 
	{ 
		return FMath::Min3(VertsAB.Num()-1, VertsAC.Num()-1, ScreenSize.Num());
	}
public:
	FRuntimeMeshProviderPlaneProxy(TWeakObjectPtr<URuntimeMeshProvider> InParent);
	~FRuntimeMeshProviderPlaneProxy();

	void UpdateProxyParameters(URuntimeMeshProvider* ParentProvider, bool bIsInitialSetup) override;

	virtual void Initialize() override;

	virtual bool GetSectionMeshForLOD(int32 LODIndex, int32 SectionId, FRuntimeMeshRenderableMeshData& MeshData) override;

	FRuntimeMeshCollisionSettings GetCollisionSettings() override;
	bool HasCollisionMesh() override;
	virtual bool GetCollisionMesh(FRuntimeMeshCollisionData& CollisionData) override;

	virtual FBoxSphereBounds GetBounds() override
	{
		FVector LocationD = LocationB - LocationA + LocationC; // C + BA
		FVector points[4] = { LocationA, LocationB, LocationC, LocationD };
		FBox BoundingBox = FBox(points, 4);
		return FBoxSphereBounds(BoundingBox); 
	}

	bool IsThreadSafe() const override { return true; }
};

UCLASS(HideCategories = Object, BlueprintType)
class RUNTIMEMESHCOMPONENT_API URuntimeMeshProviderPlane : public URuntimeMeshProvider
{
	GENERATED_BODY()

public:
	UPROPERTY(Category = "RuntimeMesh|Providers|Sphere", EditAnywhere, BlueprintReadWrite)
	FVector LocationA;
	UPROPERTY(Category = "RuntimeMesh|Providers|Sphere", EditAnywhere, BlueprintReadWrite)
	FVector LocationB;
	UPROPERTY(Category = "RuntimeMesh|Providers|Sphere", EditAnywhere, BlueprintReadWrite)
	FVector LocationC;
	UPROPERTY(Category = "RuntimeMesh|Providers|Sphere", EditAnywhere, BlueprintReadWrite)
	TArray<int32> VertsAB;
	UPROPERTY(Category = "RuntimeMesh|Providers|Sphere", EditAnywhere, BlueprintReadWrite)
	TArray<int32> VertsAC;
	UPROPERTY(Category = "RuntimeMesh|Providers|Sphere", EditAnywhere, BlueprintReadWrite)
	TArray<float> ScreenSize;

	UPROPERTY(Category = "RuntimeMesh|Providers|Sphere", EditAnywhere, BlueprintReadWrite)
	UMaterialInterface* Material;

	URuntimeMeshProviderPlane();

protected:
	virtual FRuntimeMeshProviderProxyRef GetProxy() override { return MakeShared<FRuntimeMeshProviderPlaneProxy, ESPMode::ThreadSafe>(TWeakObjectPtr<URuntimeMeshProvider>(this)); }
};
