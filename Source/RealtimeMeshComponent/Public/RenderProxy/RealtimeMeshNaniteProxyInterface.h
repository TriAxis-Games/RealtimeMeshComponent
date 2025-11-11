// Copyright (c) 2015-2025 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NaniteSceneProxy.h"
#include "RealtimeMeshCore.h"
#include "Features/IModularFeatures.h"
#include "Mesh/RealtimeMeshNaniteResourcesInterface.h"

class URealtimeMeshComponent;

namespace RealtimeMesh
{
	struct IRealtimeMeshNaniteSceneProxy : public ::Nanite::FSceneProxyBase
	{
		explicit IRealtimeMeshNaniteSceneProxy(const UPrimitiveComponent* Component)
			: ::Nanite::FSceneProxyBase(Component)
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
		
		virtual bool ShouldUseNanite(URealtimeMeshComponent* RealtimeMeshComponent) = 0;
		virtual IRealtimeMeshNaniteSceneProxy* CreateNewSceneProxy(URealtimeMeshComponent* Component, const RealtimeMesh::FRealtimeMeshProxyRef& InRealtimeMeshProxy) = 0;
	};
	
}