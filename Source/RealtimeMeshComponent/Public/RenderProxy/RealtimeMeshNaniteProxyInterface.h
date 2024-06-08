// Copyright (c) 2015-2024 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NaniteSceneProxy.h"
#include "RealtimeMeshCore.h"
#include "Features/IModularFeatures.h"
#include "Mesh/RealtimeMeshNaniteResourcesInterface.h"

class URealtimeMeshComponent;

namespace RealtimeMesh
{
	struct FRealtimeMeshStreamSet;


	struct IRealtimeMeshNaniteSceneProxy : public Nanite::FSceneProxyBase
	{
		explicit IRealtimeMeshNaniteSceneProxy(const UPrimitiveComponent* Component)
			: Nanite::FSceneProxyBase((UPrimitiveComponent*)Component)
		{
		}
	};

	class IRealtimeMeshNaniteSceneProxyManager : public IModuleInterface, public IModularFeature
	{
	public:
		static FName GetModularFeatureName()
		{
			return TEXT("RealtimeMeshNanite");
		}

		static bool IsNaniteSupportAvailable() { return IModularFeatures::Get().IsModularFeatureAvailable(GetModularFeatureName()); }
		static IRealtimeMeshNaniteSceneProxyManager& GetNaniteModule()
		{
			return IModularFeatures::Get().GetModularFeature<IRealtimeMeshNaniteSceneProxyManager>(GetModularFeatureName());
		}
		
		virtual TSharedRef<IRealtimeMeshNaniteResources> CreateNewResources(const Nanite::FResources& InResources) = 0;
		virtual bool ShouldUseNanite(URealtimeMeshComponent* RealtimeMeshComponent) = 0;
		virtual IRealtimeMeshNaniteSceneProxy* CreateNewSceneProxy(URealtimeMeshComponent* Component, const RealtimeMesh::FRealtimeMeshProxyRef& InRealtimeMeshProxy) = 0;

		virtual bool BuildRealtimeMeshNaniteData(Nanite::FResources& Resources, const FMeshNaniteSettings& Settings, const RealtimeMesh::FRealtimeMeshStreamSet& Streams) = 0;
	};
	
}