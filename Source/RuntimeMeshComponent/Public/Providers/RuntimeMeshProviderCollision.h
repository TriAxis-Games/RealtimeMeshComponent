// Copyright 2016-2020 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RuntimeMeshProvider.h"
#include "RuntimeMeshProviderCollision.generated.h"


class RUNTIMEMESHCOMPONENT_API FRuntimeMeshProviderCollisionFromRenderableProxy : public FRuntimeMeshProviderProxyPassThrough
{
	int32 LODForMeshCollision;
	TSet<int32> SectionsForMeshCollision;

	FRuntimeMeshCollisionSettings CollisionSettings;
	FRuntimeMeshCollisionData CollisionMesh;

public:
	FRuntimeMeshProviderCollisionFromRenderableProxy(TWeakObjectPtr<URuntimeMeshProvider> InParent, const FRuntimeMeshProviderProxyPtr& InNextProvider);
	~FRuntimeMeshProviderCollisionFromRenderableProxy();

	void UpdateProxyParameters(URuntimeMeshProvider* ParentProvider, bool bIsInitialSetup) override;

protected:

	FRuntimeMeshCollisionSettings GetCollisionSettings() override;
	bool HasCollisionMesh() override;
	virtual bool GetCollisionMesh(FRuntimeMeshCollisionData& CollisionData) override;

	bool IsThreadSafe() const override;

};

UCLASS(HideCategories = Object, BlueprintType)
class RUNTIMEMESHCOMPONENT_API URuntimeMeshProviderCollisionFromRenderable : public URuntimeMeshProvider
{
	GENERATED_BODY()

private:
	int32 LODForMeshCollision;
	TSet<int32> SectionsForMeshCollision;

	FRuntimeMeshCollisionSettings CollisionSettings;
	FRuntimeMeshCollisionData CollisionMesh;

	friend class FRuntimeMeshProviderCollisionFromRenderableProxy;
public:

	UPROPERTY(EditAnywhere)
	URuntimeMeshProvider* SourceProvider;



	void SetCollisionSettings(const FRuntimeMeshCollisionSettings& NewCollisionSettings);
	void SetCollisionMesh(const FRuntimeMeshCollisionData& NewCollisionMesh);
	void SetRenderableLODForCollision(int32 LODIndex);
	void SetRenderableSectionAffectsCollision(int32 SectionId, bool bCollisionEnabled);



protected:
	virtual FRuntimeMeshProviderProxyRef GetProxy() override
	{
		FRuntimeMeshProviderProxyPtr SourceProviderProxy = SourceProvider ? SourceProvider->SetupProxy() : FRuntimeMeshProviderProxyPtr();

		return MakeShared<FRuntimeMeshProviderCollisionFromRenderableProxy, ESPMode::ThreadSafe>(TWeakObjectPtr<URuntimeMeshProvider>(this), SourceProviderProxy);
	}
};