// Copyright 2016-2020 TriAxis Games L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RuntimeMeshProvider.h"
#include "RuntimeMeshProviderMemoryCache.generated.h"


UCLASS(HideCategories = Object, BlueprintType)
class RUNTIMEMESHCOMPONENT_API URuntimeMeshProviderMemoryCache : public URuntimeMeshProviderPassthrough
{
	GENERATED_BODY()
private:

	TMap<int32, TMap<int32, FRuntimeMeshSectionProperties>> CacheSectionConfig;
	TMap<int32, TMap<int32, TOptional<FRuntimeMeshRenderableMeshData>>> CachedMeshData;

	TOptional<FRuntimeMeshCollisionSettings> CachedCollisionSettings;
	TOptional<bool> CachedHasCollisionMesh;
	TOptional<TPair<bool, FRuntimeMeshCollisionData>> CachedCollisionData;

	FCriticalSection MeshSyncRoot;
	FCriticalSection CollisionSyncRoot;
public:
	UFUNCTION(Category = "RuntimeMesh|Providers|Cache", BlueprintCallable)
	void ClearCache();

protected:
	virtual bool GetSectionMeshForLOD(int32 LODIndex, int32 SectionId, FRuntimeMeshRenderableMeshData& MeshData) override;
	virtual bool GetAllSectionsMeshForLOD(int32 LODIndex, TMap<int32, FRuntimeMeshSectionData>& MeshDatas) override;
	virtual FRuntimeMeshCollisionSettings GetCollisionSettings() override;
	virtual bool HasCollisionMesh() override;
	virtual bool GetCollisionMesh(FRuntimeMeshCollisionData& CollisionData) override;
	virtual bool IsThreadSafe() override;

	virtual void ConfigureLODs(const TArray<FRuntimeMeshLODProperties>& InLODs) override;
	virtual void MarkLODDirty(int32 LODIndex) override;
	virtual void MarkAllLODsDirty() override;
	virtual void CreateSection(int32 LODIndex, int32 SectionId, const FRuntimeMeshSectionProperties& SectionProperties) override;
	virtual void MarkSectionDirty(int32 LODIndex, int32 SectionId) override;
	virtual void ClearSection(int32 LODIndex, int32 SectionId) override;
	virtual void RemoveSection(int32 LODIndex, int32 SectionId) override;
	virtual void MarkCollisionDirty() override;

	virtual void BeginDestroy() override;

private:
	void ClearCacheLOD(int32 LODIndex);
	void ClearCacheSection(int32 LODIndex, int32 SectionId);

};