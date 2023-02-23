// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "RealtimeMeshCore.h"
#include "RealtimeMeshProxyUtils.h"
#include "Data/RealtimeMeshConfig.h"

namespace RealtimeMesh
{
	struct REALTIMEMESHCOMPONENT_API FRealtimeMeshSectionProxyInitializationParameters
	{
		FRealtimeMeshSectionConfig Config;
		FRealtimeMeshStreamRange StreamRange;
	};
	
	class REALTIMEMESHCOMPONENT_API FRealtimeMeshSectionProxy : public TSharedFromThis<FRealtimeMeshSectionProxy>
	{
	private:
		FRealtimeMeshClassFactoryRef ClassFactory;
		FRealtimeMeshProxyWeakPtr ProxyWeak;	
		FRealtimeMeshSectionKey Key;	
		FRealtimeMeshSectionConfig Config;
		FRealtimeMeshStreamRange StreamRange;
		FRealtimeMeshDrawMask DrawMask;
		uint32 bIsStateDirty : 1;

	public:
		FRealtimeMeshSectionProxy(const FRealtimeMeshClassFactoryRef& InClassFactory, const FRealtimeMeshProxyRef& InProxy, FRealtimeMeshSectionKey InKey, const FRealtimeMeshSectionProxyInitializationParametersRef& InInitParams);
		virtual ~FRealtimeMeshSectionProxy();

		FRealtimeMeshSectionKey GetKey() const { return Key; }
		const FRealtimeMeshSectionConfig& GetConfig() const { return Config; }
		int32 GetMaterialSlot() const { return Config.MaterialSlot; }
		FRealtimeMeshDrawMask GetDrawMask() const { return DrawMask; }

		// FORCE INLINE bool ShouldRenderDynamicPathRayTracing() const { return ShouldRender(); }


		void UpdateConfig(const FRealtimeMeshSectionConfig& NewConfig);

		void UpdateStreamRange(const FRealtimeMeshStreamRange& InStreamRange);


		bool CreateMeshBatch(const TSharedRef<const FRealtimeMeshSectionGroupProxy>& SectionGroup, const FLODMask& LODMask, const TRange<float>& ScreenSizeLimits,
		                     bool bIsMovable, bool bIsLocalToWorldDeterminantNegative,
		                     FRHIUniformBuffer* UniformBuffer, const FMaterialRenderProxy* Material, bool bIsWireframe, bool bSupportsDithering, bool bCastRayTracedShadow,
		                     TFunctionRef<FMeshBatch&()> BatchAllocator, TFunctionRef<void(FMeshBatch&, float)> BatchSubmitter,
		                     TFunctionRef<void(const TSharedRef<FRenderResource>&)> ResourceSubmitter) const;

		void MarkStateDirty();
		virtual bool HandleUpdates(bool bShouldForceUpdate);
		virtual void Reset();
	protected:
		void OnStreamsUpdated(const TArray<FRealtimeMeshStreamKey>& AddedOrUpdatedStreams, const TArray<FRealtimeMeshStreamKey>& RemovedStreams);
		
	};

}
