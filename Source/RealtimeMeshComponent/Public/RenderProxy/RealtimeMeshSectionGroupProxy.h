// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "RealtimeMeshConfig.h"
#include "RealtimeMeshGPUBuffer.h"
#include "RealtimeMeshProxyShared.h"
#include "RealtimeMeshVertexFactory.h"

namespace RealtimeMesh
{
	class REALTIMEMESHCOMPONENT_API FRealtimeMeshSectionGroupProxy : public TSharedFromThis<FRealtimeMeshSectionGroupProxy>
	{
	private:
		const FRealtimeMeshSharedResourcesRef SharedResources;
		const FRealtimeMeshSectionGroupKey Key;
		TSharedPtr<FRealtimeMeshVertexFactory> VertexFactory;
		TArray<FRealtimeMeshSectionProxyRef> Sections;
		TMap<FRealtimeMeshSectionKey, uint32> SectionMap;
		FRealtimeMeshStreamProxyMap Streams;
#if RHI_RAYTRACING
		FRayTracingGeometry RayTracingGeometry;
#endif

		FRealtimeMeshDrawMask DrawMask;
		uint32 bIsStateDirty : 1;

	public:
		FRealtimeMeshSectionGroupProxy(const FRealtimeMeshSharedResourcesRef& InSharedResources, const FRealtimeMeshSectionGroupKey& InKey);
		virtual ~FRealtimeMeshSectionGroupProxy();

		const FRealtimeMeshSectionGroupKey& GetKey() const { return Key; }
		TSharedPtr<FRealtimeMeshVertexFactory> GetVertexFactory() const { return VertexFactory; }
		FRealtimeMeshDrawMask GetDrawMask() const { return DrawMask; }

		FRealtimeMeshSectionProxyPtr GetSection(const FRealtimeMeshSectionKey& SectionKey) const;
		TSharedPtr<FRealtimeMeshGPUBuffer> GetStream(const FRealtimeMeshStreamKey& StreamKey) const;

		virtual bool ShouldCreateRayTracingData() const { return Key.LOD().Index() == 0; }

		virtual void CreateSectionIfNotExists(const FRealtimeMeshSectionKey& SectionKey);
		virtual void RemoveSection(const FRealtimeMeshSectionKey& SectionKey);

		virtual void CreateOrUpdateStream(const FRealtimeMeshSectionGroupStreamUpdateDataRef& InStream);
		virtual void RemoveStream(const FRealtimeMeshStreamKey& StreamKey);

		virtual void CreateMeshBatches(const FRealtimeMeshBatchCreationParams& Params, const TMap<int32, TTuple<FMaterialRenderProxy*, bool>>& Materials,
		                               const FMaterialRenderProxy* WireframeMaterial, ERealtimeMeshSectionDrawType DrawType, bool bForceAllDynamic) const;

		virtual bool UpdateCachedState(bool bShouldForceUpdate);
		virtual void Reset();

	protected:
		virtual void UpdateRayTracingInfo();

		void MarkStateDirty();
		void RebuildSectionMap();
	};
}
