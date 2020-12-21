// Copyright 2016-2020 TriAxis Games L.L.C. All Rights Reserved.

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
	virtual bool GetSectionMeshForLOD(int32 LODIndex, int32 SectionId, FRuntimeMeshRenderableMeshData& MeshData) override;
	virtual bool GetAllSectionsMeshForLOD(int32 LODIndex, TMap<int32, FRuntimeMeshSectionData>& MeshDatas) override;
	virtual FRuntimeMeshCollisionSettings GetCollisionSettings() override;
	virtual bool HasCollisionMesh() override;
	virtual bool GetCollisionMesh(FRuntimeMeshCollisionData& CollisionData) override;
	virtual bool IsThreadSafe() override;

	virtual void ConfigureLODs(const TArray<FRuntimeMeshLODProperties>& InLODs) override;
	virtual void CreateSection(int32 LODIndex, int32 SectionId, const FRuntimeMeshSectionProperties& SectionProperties) override;
	virtual void ClearSection(int32 LODIndex, int32 SectionId) override;
	virtual void RemoveSection(int32 LODIndex, int32 SectionId) override;

	void ClearCachedData();
	void ClearSectionData(int32 LODIndex, int32 SectionId);
};