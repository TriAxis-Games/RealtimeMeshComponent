// Copyright (c) 2015-2025 TriAxis Games, L.L.C. All Rights Reserved.

#include "RenderProxy/RealtimeMeshLODProxy.h"

#include "Algo/IndexOf.h"
#include "Data/RealtimeMeshShared.h"
#include "Core/RealtimeMeshLODConfig.h"
#include "RenderProxy/RealtimeMeshSectionGroupProxy.h"
#include "RenderProxy/RealtimeMeshSectionProxy.h"

namespace RealtimeMesh
{
	FRealtimeMeshLODProxy::FRealtimeMeshLODProxy(const FRealtimeMeshSharedResourcesRef& InSharedResources, const FRealtimeMeshLODKey& InKey)
		: SharedResources(InSharedResources)
		  , Key(InKey)
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
	}

	void FRealtimeMeshLODProxy::CreateSectionGroupIfNotExists(const FRealtimeMeshSectionGroupKey& SectionGroupKey)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(FRealtimeMeshLODProxy::CreateSectionGroupIfNotExists);
		
		check(SectionGroupKey.IsPartOf(Key));

		// Does this section already exist
		if (!SectionGroupMap.Contains(SectionGroupKey))
		{
			const int32 SectionGroupIndex = SectionGroups.Add(SharedResources->CreateSectionGroupProxy(SectionGroupKey));
			SectionGroupMap.Add(SectionGroupKey, SectionGroupIndex);
		}
	}

	void FRealtimeMeshLODProxy::RemoveSectionGroup(const FRealtimeMeshSectionGroupKey& SectionGroupKey)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(FRealtimeMeshLODProxy::RemoveSectionGroup);
		
		check(SectionGroupKey.IsPartOf(Key));

		if (SectionGroupMap.Contains(SectionGroupKey))
		{
			const int32 SectionGroupIndex = SectionGroupMap[SectionGroupKey];
			SectionGroups.RemoveAt(SectionGroupIndex);
			RebuildSectionGroupMap();
		}
	}

#if RHI_RAYTRACING
	FRayTracingGeometry* FRealtimeMeshLODProxy::GetStaticRayTracingGeometry() const
	{
		return SectionGroups.IsValidIndex(StaticRayTraceSectionGroup)? SectionGroups[StaticRayTraceSectionGroup]->GetRayTracingGeometry() : nullptr;
	}
#endif
	
	void FRealtimeMeshLODProxy::UpdateCachedState(FRHICommandListBase& RHICmdList)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(FRealtimeMeshLODProxy::UpdateCachedState);
		
		// Handle all SectionGroup updates
		for (const auto& SectionGroup : SectionGroups)
		{
			SectionGroup->UpdateCachedState(RHICmdList);
		}

		DrawMask = FRealtimeMeshDrawMask();
		ActiveSectionGroupMask.SetNumUninitialized(SectionGroups.Num());
		ActiveSectionGroupMask.SetRange(0, SectionGroups.Num(), false);
		if (Config.bIsVisible && Config.ScreenSize >= 0)
		{
			uint32 RayTracingRelevantSectionGroupCount = 0;
			
			for (auto It = SectionGroups.CreateConstIterator(); It; ++It)
			{
				const FRealtimeMeshSectionGroupProxyRef& SectionGroup = *It;
				auto SectionGroupDrawMask = SectionGroup->GetDrawMask();
				DrawMask |= SectionGroupDrawMask;

				if (SectionGroupDrawMask.ShouldRenderInRayTracing())
				{
					RayTracingRelevantSectionGroupCount++;
				}
				
				ActiveSectionGroupMask[It.GetIndex()] = SectionGroupDrawMask.ShouldRender();
			}

			if (RayTracingRelevantSectionGroupCount > 1 || DrawMask.IsSet(ERealtimeMeshDrawMask::DrawDynamic))
			{
				DrawMask.SetFlag(ERealtimeMeshDrawMask::DynamicRayTracing);
			}
		}

		if (DrawMask.CanRenderInStaticRayTracing())
		{
			StaticRayTraceSectionGroup = Algo::IndexOfByPredicate(SectionGroups, [](const FRealtimeMeshSectionGroupProxyRef& SectionGroup)
			{
				return SectionGroup->GetDrawMask().CanRenderInStaticRayTracing();
			});		
		}
		else
		{
			StaticRayTraceSectionGroup = INDEX_NONE;
		}
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
		TRACE_CPUPROFILER_EVENT_SCOPE(FRealtimeMeshLODProxy::Reset);
		
		// Reset all the section groups
		SectionGroups.Empty();
		SectionGroupMap.Empty();

		Config = FRealtimeMeshLODConfig();
		DrawMask = FRealtimeMeshDrawMask();
	}
}
