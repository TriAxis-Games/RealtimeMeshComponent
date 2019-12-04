// Copyright 2016-2019 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RuntimeMeshProvider.h"
#include "RuntimeMeshProviderNormals.generated.h"


class RUNTIMEMESHCOMPONENT_API FRuntimeMeshProviderNormalsProxy : public FRuntimeMeshProviderProxyPassThrough
{
	bool bComputeNormals;
	bool bComputeTangents;

public:
	FRuntimeMeshProviderNormalsProxy(TWeakObjectPtr<URuntimeMeshProvider> InParent, const FRuntimeMeshProviderProxyPtr& InNextProvider, bool InComputeNormals, bool InComputeTangents);
	~FRuntimeMeshProviderNormalsProxy();

protected:

	virtual bool GetSectionMeshForLOD(int32 LODIndex, int32 SectionId, FRuntimeMeshRenderableMeshData& MeshData) override;

};

UCLASS(HideCategories = Object, BlueprintType)
class RUNTIMEMESHCOMPONENT_API URuntimeMeshProviderNormals : public URuntimeMeshProvider
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere)
		URuntimeMeshProvider* SourceProvider;
	UPROPERTY(EditAnywhere)
		bool ComputeNormals = true;
	UPROPERTY(EditAnywhere)
		bool ComputeTangents = true;

protected:
	virtual FRuntimeMeshProviderProxyRef GetProxy() override
	{
		FRuntimeMeshProviderProxyPtr SourceProviderProxy = SourceProvider ? SourceProvider->SetupProxy() : FRuntimeMeshProviderProxyPtr();

		return MakeShared<FRuntimeMeshProviderNormalsProxy, ESPMode::ThreadSafe>(TWeakObjectPtr<URuntimeMeshProvider>(this), SourceProviderProxy, ComputeNormals, ComputeTangents);
	}
};