// Copyright 2016-2020 Chris Conway (Koderz). All Rights Reserved.

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
	virtual bool GetSectionMeshForLOD_Implementation(int32 LODIndex, int32 SectionId, FRuntimeMeshRenderableMeshData& MeshData) override;
	virtual bool GetAllSectionsMeshForLOD_Implementation(int32 LODIndex, TMap<int32, FRuntimeMeshSectionData>& MeshDatas) override;
	virtual FRuntimeMeshCollisionSettings GetCollisionSettings_Implementation() override;
	virtual bool HasCollisionMesh_Implementation() override;
	virtual bool GetCollisionMesh_Implementation(FRuntimeMeshCollisionData& CollisionData) override;
	virtual bool IsThreadSafe_Implementation() override;

	virtual void ConfigureLODs_Implementation(const TArray<FRuntimeMeshLODProperties>& InLODs) override;
	virtual void MarkLODDirty_Implementation(int32 LODIndex) override;
	virtual void MarkAllLODsDirty_Implementation() override;
	virtual void CreateSection_Implementation(int32 LODIndex, int32 SectionId, const FRuntimeMeshSectionProperties& SectionProperties) override;
	virtual void MarkSectionDirty_Implementation(int32 LODIndex, int32 SectionId) override;
	virtual void ClearSection_Implementation(int32 LODIndex, int32 SectionId) override;
	virtual void RemoveSection_Implementation(int32 LODIndex, int32 SectionId) override;
	virtual void MarkCollisionDirty_Implementation() override;

	virtual void BeginDestroy() override;

private:
	void ClearCacheLOD(int32 LODIndex);
	void ClearCacheSection(int32 LODIndex, int32 SectionId);

};