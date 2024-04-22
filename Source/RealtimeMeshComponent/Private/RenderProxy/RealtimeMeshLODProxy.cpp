// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#include "RenderProxy/RealtimeMeshLODProxy.h"

#include "Data/RealtimeMeshShared.h"
#include "RenderProxy/RealtimeMeshSectionGroupProxy.h"
#include "RenderProxy/RealtimeMeshSectionProxy.h"

namespace RealtimeMesh
{
	FRealtimeMeshLODProxy::FRealtimeMeshLODProxy(const FRealtimeMeshSharedResourcesRef& InSharedResources, const FRealtimeMeshLODKey& InKey)
		: SharedResources(InSharedResources)
		  , Key(InKey)
		  , bIsStateDirty(true)
	{
	}

	FRealtimeMeshLODProxy::~FRealtimeMeshLODProxy()
	{
		check(IsInRenderingThread());
		Reset();
	}

	FRealtimeMeshSectionGroupProxyPtr FRealtimeMeshLODProxy::GetSectionGroup(const FRealtimeMeshSectionGroupKey& SectionGroupKey) const
	{
		check(SectionGroupKey.IsPartOf(Key));

		if (SectionGroupMap.Contains(SectionGroupKey))
		{
			return SectionGroups[SectionGroupMap[SectionGroupKey]];
		}
		return FRealtimeMeshSectionGroupProxyPtr();
	}

	void FRealtimeMeshLODProxy::UpdateConfig(const FRealtimeMeshLODConfig& NewConfig)
	{
		Config = NewConfig;
		MarkStateDirty();
	}

	void FRealtimeMeshLODProxy::CreateSectionGroupIfNotExists(const FRealtimeMeshSectionGroupKey& SectionGroupKey)
	{
		check(SectionGroupKey.IsPartOf(Key));

		// Does this section already exist
		if (!SectionGroupMap.Contains(SectionGroupKey))
		{
			const int32 SectionGroupIndex = SectionGroups.Add(SharedResources->CreateSectionGroupProxy(SectionGroupKey));
			SectionGroupMap.Add(SectionGroupKey, SectionGroupIndex);
			MarkStateDirty();
		}
	}

	void FRealtimeMeshLODProxy::RemoveSectionGroup(const FRealtimeMeshSectionGroupKey& SectionGroupKey)
	{
		check(SectionGroupKey.IsPartOf(Key));

		if (SectionGroupMap.Contains(SectionGroupKey))
		{
			const int32 SectionGroupIndex = SectionGroupMap[SectionGroupKey];
			SectionGroups.RemoveAt(SectionGroupIndex);
			RebuildSectionGroupMap();
			MarkStateDirty();
		}
	}

	void FRealtimeMeshLODProxy::CreateMeshBatches(const FRealtimeMeshBatchCreationParams& Params, const TMap<int32, TTuple<FMaterialRenderProxy*, bool>>& Materials,
	                                              const FMaterialRenderProxy* WireframeMaterial, ERealtimeMeshSectionDrawType DrawType, ERealtimeMeshBatchCreationFlags InclusionFlags) const
	{
		const ERealtimeMeshDrawMask DrawTypeMask = EnumHasAllFlags(InclusionFlags, ERealtimeMeshBatchCreationFlags::ForceAllDynamic)
			                                           ? ERealtimeMeshDrawMask::DrawPassMask
			                                           : DrawType == ERealtimeMeshSectionDrawType::Dynamic
			                                           ? ERealtimeMeshDrawMask::DrawDynamic
			                                           : ERealtimeMeshDrawMask::DrawStatic;

		if (DrawMask.IsAnySet(DrawTypeMask))
		{
			for (const auto& SectionGroup : SectionGroups)
			{
				if (!EnumHasAllFlags(InclusionFlags, ERealtimeMeshBatchCreationFlags::SkipStaticRayTracedSections) || SectionGroup != StaticRaytracingSectionGroup)
				{
					if (SectionGroup->GetDrawMask().IsAnySet(DrawTypeMask))
					{
						SectionGroup->CreateMeshBatches(Params, Materials, WireframeMaterial, DrawType, EnumHasAllFlags(InclusionFlags, ERealtimeMeshBatchCreationFlags::ForceAllDynamic));
					}
				}
			}
		}
	}

#if RHI_RAYTRACING
	FRayTracingGeometry* FRealtimeMeshLODProxy::GetStaticRayTracingGeometry() const
	{
		return StaticRaytracingSectionGroup.IsValid()? StaticRaytracingSectionGroup->GetRayTracingGeometry() : nullptr;
	}
#endif
	
	bool FRealtimeMeshLODProxy::UpdateCachedState(bool bShouldForceUpdate)
	{
		// Handle all SectionGroup updates
		for (const auto& SectionGroup : SectionGroups)
		{
			bIsStateDirty |= SectionGroup->UpdateCachedState(bIsStateDirty || bShouldForceUpdate);
		}

		if (!bIsStateDirty && !bShouldForceUpdate)
		{
			return false;
		}

		FRealtimeMeshDrawMask NewDrawMask;
		if (Config.bIsVisible && Config.ScreenSize >= 0)
		{
			for (const auto& SectionGroup : SectionGroups)
			{
				NewDrawMask |= SectionGroup->GetDrawMask();
			}
		}

		FRealtimeMeshSectionGroupProxyPtr NewStaticRaytracingGroup;

		if (NewDrawMask.ShouldRenderStaticPath())
		{
			// If the group is overriden use it.
			if (OverrideStaticRayTracingGroup.IsSet())
			{
				NewStaticRaytracingGroup = GetSectionGroup(OverrideStaticRayTracingGroup.GetValue());
			}

			if (!NewStaticRaytracingGroup.IsValid())
			{
				int32 CurrentLargestSectionGroup = 0;
				for (const auto& SectionGroup : SectionGroups)
				{
					if (SectionGroup->GetDrawMask().ShouldRenderStaticPath())
					{
						if (SectionGroup->GetVertexFactory()->GetValidRange().NumPrimitives(REALTIME_MESH_NUM_INDICES_PER_PRIMITIVE) > CurrentLargestSectionGroup)
						{
							CurrentLargestSectionGroup = SectionGroup->GetVertexFactory()->GetValidRange().NumPrimitives(REALTIME_MESH_NUM_INDICES_PER_PRIMITIVE);
							NewStaticRaytracingGroup = SectionGroup;				
						}
					}
				}				
			}			
		}

		const bool bStateChanged = DrawMask != NewDrawMask;
		DrawMask = NewDrawMask;
		bIsStateDirty = false;
		StaticRaytracingSectionGroup = NewStaticRaytracingGroup;
		return bStateChanged;
	}

	void FRealtimeMeshLODProxy::MarkStateDirty()
	{
		bIsStateDirty = true;
	}

	void FRealtimeMeshLODProxy::RebuildSectionGroupMap()
	{
		SectionGroupMap.Empty();
		for (auto It = SectionGroups.CreateIterator(); It; ++It)
		{
			SectionGroupMap.Add((*It)->GetKey(), It.GetIndex());
		}
	}

	void FRealtimeMeshLODProxy::Reset()
	{
		// Reset all the section groups
		SectionGroups.Empty();
		SectionGroupMap.Empty();

		Config = FRealtimeMeshLODConfig();
		DrawMask = FRealtimeMeshDrawMask();
		bIsStateDirty = true;
	}
}
