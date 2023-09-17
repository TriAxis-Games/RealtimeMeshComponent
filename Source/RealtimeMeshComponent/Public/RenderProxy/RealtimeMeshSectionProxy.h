// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "RealtimeMeshCore.h"
#include "RealtimeMeshConfig.h"
#include "RealtimeMeshProxyShared.h"

namespace RealtimeMesh
{
	class REALTIMEMESHCOMPONENT_API FRealtimeMeshSectionProxy : public TSharedFromThis<FRealtimeMeshSectionProxy>
	{
	private:
		const FRealtimeMeshSharedResourcesRef SharedResources;
		const FRealtimeMeshSectionKey Key;
		FRealtimeMeshSectionConfig Config;
		FRealtimeMeshStreamRange StreamRange;
		FRealtimeMeshDrawMask DrawMask;
		uint32 bIsStateDirty : 1;

	public:
		FRealtimeMeshSectionProxy(const FRealtimeMeshSharedResourcesRef& InSharedResources, const FRealtimeMeshSectionKey InKey);
		virtual ~FRealtimeMeshSectionProxy();

		FRealtimeMeshSectionKey GetKey() const { return Key; }
		const FRealtimeMeshSectionConfig& GetConfig() const { return Config; }
		int32 GetMaterialSlot() const { return Config.MaterialSlot; }
		FRealtimeMeshDrawMask GetDrawMask() const { return DrawMask; }
		FRealtimeMeshStreamRange GetStreamRange() const { return StreamRange; }

		virtual void UpdateConfig(const FRealtimeMeshSectionConfig& NewConfig);
		virtual void UpdateStreamRange(const FRealtimeMeshStreamRange& InStreamRange);

		virtual bool CreateMeshBatch(
			const FRealtimeMeshBatchCreationParams& Params,
			const FRealtimeMeshVertexFactoryRef& VertexFactory,
			const FMaterialRenderProxy* Material,
			bool bIsWireframe,
			bool bSupportsDithering
#if RHI_RAYTRACING
			, const FRayTracingGeometry* RayTracingGeometry
#endif
		) const;

		virtual bool UpdateCachedState(bool bShouldForceUpdate, FRealtimeMeshSectionGroupProxy& ParentGroup);
		virtual void Reset();

	protected:
		void MarkStateDirty();
	};
}
