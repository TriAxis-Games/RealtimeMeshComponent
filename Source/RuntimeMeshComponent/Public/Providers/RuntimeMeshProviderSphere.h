// Copyright 2016-2020 Chris Conway (Koderz). All Rights Reserved.

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
	FBoxSphereBounds LocalBounds;
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

	UFUNCTION(BlueprintCallable)
	float GetSphereRadius() const;
	UFUNCTION(BlueprintCallable)
	void SetSphereRadius(float InSphereRadius);

	UFUNCTION(BlueprintCallable)
	int32 GetMaxLatitudeSegments() const;
	UFUNCTION(BlueprintCallable)
	void SetMaxLatitudeSegments(int32 InMaxLatitudeSegments);

	UFUNCTION(BlueprintCallable)
	int32 GetMinLatitudeSegments() const;
	UFUNCTION(BlueprintCallable)
	void SetMinLatitudeSegments(int32 InMinLatitudeSegments);

	UFUNCTION(BlueprintCallable)
	int32 GetMaxLongitudeSegments() const;
	UFUNCTION(BlueprintCallable)
	void SetMaxLongitudeSegments(int32 InMaxLongitudeSegments);

	UFUNCTION(BlueprintCallable)
	int32 GetMinLongitudeSegments() const;
	UFUNCTION(BlueprintCallable)
	void SetMinLongitudeSegments(int32 InMinLongitudeSegments);

	UFUNCTION(BlueprintCallable)
	float GetLODMultiplier() const;
	UFUNCTION(BlueprintCallable)
	void SetLODMultiplier(float InLODMultiplier);

	UFUNCTION(BlueprintCallable)
	UMaterialInterface* GetSphereMaterial() const;
	UFUNCTION(BlueprintCallable)
	void SetSphereMaterial(UMaterialInterface* InSphereMaterial);



protected:
	virtual void Initialize_Implementation() override;

	virtual bool GetSectionMeshForLOD_Implementation(int32 LODIndex, int32 SectionId, FRuntimeMeshRenderableMeshData& MeshData) override;

	virtual FRuntimeMeshCollisionSettings GetCollisionSettings_Implementation() override;

	virtual FBoxSphereBounds GetBounds_Implementation() override;
	virtual bool IsThreadSafe_Implementation() override;


private:
	void GetShapeParams(float& OutRadius, int32& OutMinLatitudeSegments, int32& OutMaxLatitudeSegments, int32& OutMinLongitudeSegments, int32& OutMaxLongitudeSegments, float& OutLODMultiplier);
	int32 GetMaxNumberOfLODs();
	float CalculateScreenSize(int32 LODIndex);
	static bool GetSphereMesh(int32 SphereRadius, int32 LatitudeSegments, int32 LongitudeSegments, FRuntimeMeshRenderableMeshData& MeshData);
	void UpdateMeshParameters(bool bAffectsCollision);
};
