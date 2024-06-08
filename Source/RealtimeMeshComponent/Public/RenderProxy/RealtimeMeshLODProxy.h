// Copyright (c) 2015-2024 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "RealtimeMeshConfig.h"
#include "RealtimeMeshCore.h"
#include "RealtimeMeshProxyShared.h"
#include "RealtimeMeshSectionGroupProxy.h"

namespace RealtimeMesh
{	
	class REALTIMEMESHCOMPONENT_API FRealtimeMeshLODProxy : public TSharedFromThis<FRealtimeMeshLODProxy>
	{
	private:
		const FRealtimeMeshSharedResourcesRef SharedResources;
		const FRealtimeMeshLODKey Key;
		TArray<FRealtimeMeshSectionGroupProxyRef> SectionGroups;
		TMap<FRealtimeMeshSectionGroupKey, uint32> SectionGroupMap;
		TOptional<FRealtimeMeshSectionGroupKey> OverrideStaticRayTracingGroup;

		FRealtimeMeshLODConfig Config;
		FRealtimeMeshDrawMask DrawMask;
#if RHI_RAYTRACING
		FRealtimeMeshSectionGroupProxyPtr StaticRaytracingSectionGroup;
#endif
		uint32 bIsStateDirty : 1;
		

	public:
		FRealtimeMeshLODProxy(const FRealtimeMeshSharedResourcesRef& InSharedResources, const FRealtimeMeshLODKey& InKey);
		virtual ~FRealtimeMeshLODProxy();

		const FRealtimeMeshLODKey& GetKey() const { return Key; }
		const FRealtimeMeshLODConfig& GetConfig() const { return Config; }
		FRealtimeMeshDrawMask GetDrawMask() const { return DrawMask; }
		float GetScreenSize() const { return Config.ScreenSize; }

		FRealtimeMeshSectionGroupProxyPtr GetSectionGroup(const FRealtimeMeshSectionGroupKey& SectionGroupKey) const;

		template<typename ProcessFunc>
		void ProcessSections(ERealtimeMeshDrawMask InDrawMask, ProcessFunc ProcessFunction) const
		{
			if (DrawMask.IsSet(InDrawMask))
			{
				for (const FRealtimeMeshSectionGroupProxyRef& SectionGroup : SectionGroups)
				{
					if (SectionGroup->GetDrawMask().IsSet(InDrawMask))
					{
						SectionGroup->ProcessSections(InDrawMask, [&](const FRealtimeMeshSectionProxyRef& Section)
						{
							ProcessFunction(SectionGroup, Section);
						});
					}
				}
			}
		}

		virtual void UpdateConfig(const FRealtimeMeshLODConfig& NewConfig);

		virtual void CreateSectionGroupIfNotExists(const FRealtimeMeshSectionGroupKey& SectionGroupKey);
		virtual void RemoveSectionGroup(const FRealtimeMeshSectionGroupKey& SectionGroupKey);

		virtual void CreateMeshBatches(const FRealtimeMeshBatchCreationParams& Params, const TMap<int32, TTuple<FMaterialRenderProxy*, bool>>& Materials,
		                               const FMaterialRenderProxy* WireframeMaterial, ERealtimeMeshSectionDrawType DrawType, ERealtimeMeshBatchCreationFlags InclusionFlags) const;

#if RHI_RAYTRACING
		virtual FRayTracingGeometry* GetStaticRayTracingGeometry() const;
#endif
		
		virtual bool UpdateCachedState(bool bShouldForceUpdate);
		virtual void Reset();

	protected:
		void MarkStateDirty();
		void RebuildSectionGroupMap();
	};
}
