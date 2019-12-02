// Copyright 2016-2019 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RuntimeMeshProvider.h"
#include "RuntimeMeshProviderMemoryCache.generated.h"


class RUNTIMEMESHCOMPONENT_API FRuntimeMeshProviderMemoryCacheProxy : public FRuntimeMeshProviderProxyPassThrough
{
	FORCEINLINE int64 GenerateKeyFromLODAndSection(int32 LODIndex, int32 SectionId)
	{
		return (((int64)LODIndex) << 32) | ((int64)SectionId);
	}


	TMap<int64, TOptional<FRuntimeMeshRenderableMeshData>> CachedMeshData;

	TOptional<FRuntimeMeshCollisionSettings> CachedCollisionSettings;
	TOptional<bool> CachedHasCollisionMesh;
	TOptional<TPair<bool, FRuntimeMeshCollisionData>> CachedCollisionData;
public:
	FRuntimeMeshProviderMemoryCacheProxy(TWeakObjectPtr<URuntimeMeshProvider> InParent, const FRuntimeMeshProviderProxyPtr& InNextProvider);
	~FRuntimeMeshProviderMemoryCacheProxy();

	void ClearCache();

protected:
	virtual void CreateSection(int32 LODIndex, int32 SectionId, const FRuntimeMeshSectionProperties& SectionProperties);
	virtual void MarkSectionDirty(int32 LODIndex, int32 SectionId);
	virtual void RemoveSection(int32 LODIndex, int32 SectionId);
	virtual void MarkCollisionDirty();



	virtual bool GetSectionMeshForLOD(int32 LODIndex, int32 SectionId, FRuntimeMeshRenderableMeshData& MeshData) override;

	FRuntimeMeshCollisionSettings GetCollisionSettings() override;
	bool HasCollisionMesh() override;
	virtual bool GetCollisionMesh(FRuntimeMeshCollisionData& CollisionData) override;
	
	bool IsThreadSafe() const override;

};

UCLASS(HideCategories = Object, BlueprintType)
class RUNTIMEMESHCOMPONENT_API URuntimeMeshProviderMemoryCache : public URuntimeMeshProvider
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere)
	URuntimeMeshProvider* SourceProvider;

protected:
	virtual FRuntimeMeshProviderProxyRef GetProxy() override 
	{ 
		FRuntimeMeshProviderProxyPtr SourceProviderProxy = SourceProvider ? SourceProvider->SetupProxy() : FRuntimeMeshProviderProxyPtr();

		return MakeShared<FRuntimeMeshProviderMemoryCacheProxy, ESPMode::ThreadSafe>(TWeakObjectPtr<URuntimeMeshProvider>(this), SourceProviderProxy);
	}
};