// Copyright 2016-2020 TriAxis Games L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RuntimeMeshProvider.h"
#include "RuntimeMeshProviderBox.generated.h"

UCLASS(HideCategories = Object, BlueprintType)
class RUNTIMEMESHCOMPONENT_API URuntimeMeshProviderBox : public URuntimeMeshProvider
{
	GENERATED_BODY()
private:
	mutable FCriticalSection PropertySyncRoot;

	UPROPERTY(Category = "RuntimeMesh|Providers|Box", VisibleAnywhere, BlueprintGetter=GetBoxRadius, BlueprintSetter=SetBoxRadius)
	FVector BoxRadius;

	UPROPERTY(Category = "RuntimeMesh|Providers|Box", VisibleAnywhere, BlueprintGetter=GetBoxMaterial, BlueprintSetter=SetBoxMaterial)
	UMaterialInterface* Material;
public:
	UFUNCTION(Category = "RuntimeMesh|Providers|Box", BlueprintCallable)
	FVector GetBoxRadius() const;
	UFUNCTION(Category = "RuntimeMesh|Providers|Box", BlueprintCallable)
	void SetBoxRadius(const FVector& InRadius);

	UFUNCTION(Category = "RuntimeMesh|Providers|Box", BlueprintCallable)
	UMaterialInterface* GetBoxMaterial() const;
	UFUNCTION(Category = "RuntimeMesh|Providers|Box", BlueprintCallable)
	void SetBoxMaterial(UMaterialInterface* InMaterial);


protected:
	void Initialize() override;
	FBoxSphereBounds GetBounds() override;
	bool GetSectionMeshForLOD(int32 LODIndex, int32 SectionId, FRuntimeMeshRenderableMeshData& MeshData) override;
	FRuntimeMeshCollisionSettings GetCollisionSettings() override;
	bool HasCollisionMesh() override;
	bool GetCollisionMesh(FRuntimeMeshCollisionData& CollisionData) override;
	bool IsThreadSafe() override;

};
