// Copyright 2016-2019 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RuntimeMeshProvider.h"
#include "RuntimeMeshProviderCollision.generated.h"


class RUNTIMEMESHCOMPONENT_API FRuntimeMeshProviderCollisionFromRenderable : public FRuntimeMeshProviderProxyPassThrough
{
	int32 LODForMeshCollision;
	TSet<int32> SectionsForMeshCollision;

	FRuntimeMeshCollisionSettings CollisionSettings;
public:
	FRuntimeMeshProviderCollisionFromRenderable(TWeakObjectPtr<URuntimeMeshProvider> InParent, const FRuntimeMeshProviderProxyPtr& InNextProvider);
	~FRuntimeMeshProviderCollisionFromRenderable();

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

	friend class FRuntimeMeshProviderCollisionFromRenderable;
public:

	UPROPERTY(EditAnywhere)
	URuntimeMeshProvider* SourceProvider;



	void SetCollisionSettings(const FRuntimeMeshCollisionSettings& NewCollisionSettings);
	void SetRenderableLODForCollision(int32 LODIndex);
	void SetRenderableSectionAffectsCollision(int32 SectionId, bool bCollisionEnabled);



protected:
	virtual FRuntimeMeshProviderProxyRef GetProxy() override
	{
		FRuntimeMeshProviderProxyPtr SourceProviderProxy = SourceProvider ? SourceProvider->SetupProxy() : FRuntimeMeshProviderProxyPtr();

		return MakeShared<FRuntimeMeshProviderCollisionFromRenderable, ESPMode::ThreadSafe>(TWeakObjectPtr<URuntimeMeshProvider>(this), SourceProviderProxy);
	}
};