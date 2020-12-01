// Copyright 2016-2020 TriAxis Games L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RuntimeMeshProvider.h"
#include "RuntimeMeshProviderSphere.generated.h"

UCLASS(HideCategories = Object, BlueprintType)
class RUNTIMEMESHCOMPONENT_API URuntimeMeshProviderSphere : public URuntimeMeshProvider
{
	GENERATED_BODY()

private:
	mutable FCriticalSection PropertySyncRoot;

	UPROPERTY()
	int32 MaxLOD;

	UPROPERTY(Category = "RuntimeMesh|Providers|Sphere", VisibleAnywhere, BlueprintGetter=GetSphereRadius, BlueprintSetter=SetSphereRadius)
	float SphereRadius;

	UPROPERTY(Category = "RuntimeMesh|Providers|Sphere", VisibleAnywhere, BlueprintGetter=GetMaxLatitudeSegments, BlueprintSetter=SetMaxLatitudeSegments)
	int32 MaxLatitudeSegments;
	
	UPROPERTY(Category = "RuntimeMesh|Providers|Sphere", VisibleAnywhere, BlueprintGetter=GetMinLatitudeSegments, BlueprintSetter=SetMinLatitudeSegments)
	int32 MinLatitudeSegments;
	
	UPROPERTY(Category = "RuntimeMesh|Providers|Sphere", VisibleAnywhere, BlueprintGetter=GetMaxLongitudeSegments, BlueprintSetter=SetMaxLongitudeSegments)
	int32 MaxLongitudeSegments;
	
	UPROPERTY(Category = "RuntimeMesh|Providers|Sphere", VisibleAnywhere, BlueprintGetter=GetMinLongitudeSegments, BlueprintSetter=SetMinLongitudeSegments)
	int32 MinLongitudeSegments;
	
	UPROPERTY(Category = "RuntimeMesh|Providers|Sphere", VisibleAnywhere, BlueprintGetter=GetLODMultiplier, BlueprintSetter=SetLODMultiplier)
	float LODMultiplier;

	UPROPERTY(Category = "RuntimeMesh|Providers|Sphere", VisibleAnywhere, BlueprintGetter=GetSphereMaterial, BlueprintSetter=SetSphereMaterial)
	UMaterialInterface* SphereMaterial;

public:

	URuntimeMeshProviderSphere();

	UFUNCTION(Category = "RuntimeMesh|Providers|Sphere", BlueprintCallable)
	float GetSphereRadius() const;
	UFUNCTION(Category = "RuntimeMesh|Providers|Sphere", BlueprintCallable)
	void SetSphereRadius(float InSphereRadius);

	UFUNCTION(Category = "RuntimeMesh|Providers|Sphere", BlueprintCallable)
	int32 GetMaxLatitudeSegments() const;
	UFUNCTION(Category = "RuntimeMesh|Providers|Sphere", BlueprintCallable)
	void SetMaxLatitudeSegments(int32 InMaxLatitudeSegments);

	UFUNCTION(Category = "RuntimeMesh|Providers|Sphere", BlueprintCallable)
	int32 GetMinLatitudeSegments() const;
	UFUNCTION(Category = "RuntimeMesh|Providers|Sphere", BlueprintCallable)
	void SetMinLatitudeSegments(int32 InMinLatitudeSegments);

	UFUNCTION(Category = "RuntimeMesh|Providers|Sphere", BlueprintCallable)
	int32 GetMaxLongitudeSegments() const;
	UFUNCTION(Category = "RuntimeMesh|Providers|Sphere", BlueprintCallable)
	void SetMaxLongitudeSegments(int32 InMaxLongitudeSegments);

	UFUNCTION(Category = "RuntimeMesh|Providers|Sphere", BlueprintCallable)
	int32 GetMinLongitudeSegments() const;
	UFUNCTION(Category = "RuntimeMesh|Providers|Sphere", BlueprintCallable)
	void SetMinLongitudeSegments(int32 InMinLongitudeSegments);

	UFUNCTION(Category = "RuntimeMesh|Providers|Sphere", BlueprintCallable)
	float GetLODMultiplier() const;
	UFUNCTION(Category = "RuntimeMesh|Providers|Sphere", BlueprintCallable)
	void SetLODMultiplier(float InLODMultiplier);

	UFUNCTION(Category = "RuntimeMesh|Providers|Sphere", BlueprintCallable)
	UMaterialInterface* GetSphereMaterial() const;
	UFUNCTION(Category = "RuntimeMesh|Providers|Sphere", BlueprintCallable)
	void SetSphereMaterial(UMaterialInterface* InSphereMaterial);



protected:
	virtual void Initialize() override;
	virtual bool GetSectionMeshForLOD(int32 LODIndex, int32 SectionId, FRuntimeMeshRenderableMeshData& MeshData) override;
	virtual FRuntimeMeshCollisionSettings GetCollisionSettings() override;
	virtual FBoxSphereBounds GetBounds() override;
	virtual bool IsThreadSafe() override;


private:
	void GetShapeParams(float& OutRadius, int32& OutMinLatitudeSegments, int32& OutMaxLatitudeSegments, int32& OutMinLongitudeSegments, int32& OutMaxLongitudeSegments, float& OutLODMultiplier);
	int32 GetMaxNumberOfLODs();
	float CalculateScreenSize(int32 LODIndex);
	void GetSegmentsForLOD(int32& LODIndex, int32& LatitudeSegments, int32& LongitudeSegments)
	{
		GetSegmentsForLOD(LODIndex, LODMultiplier, MaxLatitudeSegments, MinLatitudeSegments, MaxLongitudeSegments, MinLongitudeSegments, LatitudeSegments, LongitudeSegments);
	}
	static void GetSegmentsForLOD(int32& LODIndex, float& LODMul, int32& MaxLat, int32& MinLat, int32& MaxLon, int32& MinLon, int32& LatitudeSegments, int32& LongitudeSegments){
		LatitudeSegments = FMath::Max(FMath::RoundToInt(MaxLat * FMath::Pow(LODMul, LODIndex)), MinLat);
		LongitudeSegments = FMath::Max(FMath::RoundToInt(MaxLon * FMath::Pow(LODMul, LODIndex)), MinLon);
	}
	static bool GetSphereMesh(int32 SphereRadius, int32 LatitudeSegments, int32 LongitudeSegments, FRuntimeMeshRenderableMeshData& MeshData);
	void UpdateMeshParameters(bool bAffectsCollision);
};
