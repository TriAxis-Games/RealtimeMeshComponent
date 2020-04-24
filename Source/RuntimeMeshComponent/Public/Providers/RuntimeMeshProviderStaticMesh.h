// Copyright 2016-2020 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RuntimeMeshProvider.h"
#include "RuntimeMeshStaticMeshConverter.h"
#include "RuntimeMeshProviderStaticMesh.generated.h"

/**
 * 
 */
UCLASS()
class RUNTIMEMESHCOMPONENT_API URuntimeMeshProviderStaticMesh : public URuntimeMeshProvider
{
	GENERATED_BODY()

private:
	UPROPERTY(Category = "RuntimeMesh|Providers|StaticMesh", VisibleAnywhere, BlueprintGetter=GetStaticMesh, BlueprintSetter=SetStaticMesh)
	UStaticMesh* StaticMesh;

	UPROPERTY(Category = "RuntimeMesh|Providers|StaticMesh", VisibleAnywhere, BlueprintGetter=GetMaxLOD, BlueprintSetter=SetMaxLOD)
	int32 MaxLOD;

	UPROPERTY(Category = "RuntimeMesh|Providers|StaticMesh", VisibleAnywhere, BlueprintGetter=GetComplexCollisionLOD, BlueprintSetter=SetComplexCollisionLOD)
	int32 ComplexCollisionLOD;
	
public:
	URuntimeMeshProviderStaticMesh();

	UFUNCTION(Category = "RuntimeMesh|Providers|StaticMesh", BlueprintCallable)
	UStaticMesh* GetStaticMesh() const;

	UFUNCTION(Category = "RuntimeMesh|Providers|StaticMesh", BlueprintCallable)
	void SetStaticMesh(UStaticMesh* InStaticMesh);

	UFUNCTION(Category = "RuntimeMesh|Providers|StaticMesh", BlueprintCallable)
	int32 GetMaxLOD() const;

	UFUNCTION(Category = "RuntimeMesh|Providers|StaticMesh", BlueprintCallable)
	void SetMaxLOD(int32 InMaxLOD);

	UFUNCTION(Category = "RuntimeMesh|Providers|StaticMesh", BlueprintCallable)
	int32 GetComplexCollisionLOD() const;

	UFUNCTION(Category = "RuntimeMesh|Providers|StaticMesh", BlueprintCallable)
	void SetComplexCollisionLOD(int32 InLOD);


protected:
	virtual void Initialize_Implementation() override;
	virtual FBoxSphereBounds GetBounds_Implementation() override;
	virtual bool GetSectionMeshForLOD_Implementation(int32 LODIndex, int32 SectionId, FRuntimeMeshRenderableMeshData& MeshData) override;
	virtual FRuntimeMeshCollisionSettings GetCollisionSettings_Implementation() override;
	virtual bool HasCollisionMesh_Implementation() override;
	virtual bool GetCollisionMesh_Implementation(FRuntimeMeshCollisionData& CollisionData) override;

	void UpdateCollisionFromStaticMesh();
	void UpdateRenderingFromStaticMesh();
};
