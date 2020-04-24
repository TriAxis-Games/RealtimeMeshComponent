// Copyright 2016-2020 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RuntimeMeshProvider.h"
#include "RuntimeMeshProviderCollision.generated.h"

UCLASS(HideCategories = Object, BlueprintType)
class RUNTIMEMESHCOMPONENT_API URuntimeMeshProviderCollision : public URuntimeMeshProviderPassthrough
{
	GENERATED_BODY()

private:
	UPROPERTY()
	int32 LODForMeshCollision;

	UPROPERTY()
	TMap<int32, FRuntimeMeshRenderableCollisionData> RenderableCollisionData;

	UPROPERTY()
	TSet<int32> SectionsAffectingCollision;

	UPROPERTY()
	FRuntimeMeshCollisionSettings CollisionSettings;

	UPROPERTY()
	FRuntimeMeshCollisionData CollisionMesh;

	FCriticalSection SyncRoot;
public:

	URuntimeMeshProviderCollision();

	UFUNCTION(Category = "RuntimeMesh|Providers|Collision", BlueprintCallable)
	void SetCollisionSettings(const FRuntimeMeshCollisionSettings& NewCollisionSettings);

	UFUNCTION(Category = "RuntimeMesh|Providers|Collision", BlueprintCallable)
	void SetCollisionMesh(const FRuntimeMeshCollisionData& NewCollisionMesh);

	UFUNCTION(Category = "RuntimeMesh|Providers|Collision", BlueprintCallable)
	void SetRenderableLODForCollision(int32 LODIndex);

	UFUNCTION(Category = "RuntimeMesh|Providers|Collision", BlueprintCallable)
	void SetRenderableSectionAffectsCollision(int32 SectionId, bool bCollisionEnabled);


protected:
	virtual bool GetSectionMeshForLOD_Implementation(int32 LODIndex, int32 SectionId, FRuntimeMeshRenderableMeshData& MeshData) override;
	virtual bool GetAllSectionsMeshForLOD_Implementation(int32 LODIndex, TMap<int32, FRuntimeMeshSectionData>& MeshDatas) override;
	virtual FRuntimeMeshCollisionSettings GetCollisionSettings_Implementation() override;
	virtual bool HasCollisionMesh_Implementation() override;
	virtual bool GetCollisionMesh_Implementation(FRuntimeMeshCollisionData& CollisionData) override;
	virtual bool IsThreadSafe_Implementation() override;

	virtual void ConfigureLODs_Implementation(const TArray<FRuntimeMeshLODProperties>& InLODs) override;
	virtual void CreateSection_Implementation(int32 LODIndex, int32 SectionId, const FRuntimeMeshSectionProperties& SectionProperties) override;
	virtual void ClearSection_Implementation(int32 LODIndex, int32 SectionId) override;
	virtual void RemoveSection_Implementation(int32 LODIndex, int32 SectionId) override;

	void ClearCachedData();
	void ClearSection(int32 LODIndex, int32 SectionId);
};