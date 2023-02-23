// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#include "RenderProxy/RealtimeMeshLODProxy.h"
#include "RenderProxy/RealtimeMeshSectionGroupProxy.h"
#include "RenderProxy/RealtimeMeshSectionProxy.h"

namespace RealtimeMesh
{
	FRealtimeMeshLODProxy::FRealtimeMeshLODProxy(const FRealtimeMeshClassFactoryRef& InClassFactory, const FRealtimeMeshProxyRef& InProxy,
					FRealtimeMeshLODKey InKey, const FRealtimeMeshLODProxyInitializationParametersRef& InInitParams)
		: ClassFactory(InClassFactory)
		, ProxyWeak(InProxy)
		, Key(InKey)
		, Config(InInitParams->Config)
		, bIsStateDirty(true)
	{		
		// Setup all the buffer sets
		for (TSparseArray<FRealtimeMeshSectionGroupProxyInitializationParametersRef>::TConstIterator It(InInitParams->SectionGroups); It; ++It)
		{
			SectionGroups.Insert(It.GetIndex(), ClassFactory->CreateSectionGroupProxy(InProxy, FRealtimeMeshSectionGroupKey(Key, It.GetIndex()), *It));
		}
	}

	FRealtimeMeshSectionGroupProxyPtr FRealtimeMeshLODProxy::GetSectionGroup(FRealtimeMeshSectionGroupKey SectionKey) const
	{ 
		if (SectionKey.IsPartOf(Key) && SectionGroups.IsValidIndex(FRealtimeMeshKeyHelpers::GetSectionGroupIndex(SectionKey)))
		{
			return SectionGroups[FRealtimeMeshKeyHelpers::GetSectionGroupIndex(SectionKey)];
		}
		return FRealtimeMeshSectionGroupProxyPtr();
	}

	void FRealtimeMeshLODProxy::UpdateConfig(const FRealtimeMeshLODConfig& NewConfig)
	{
		Config = NewConfig;
		MarkStateDirty();
	}

	void FRealtimeMeshLODProxy::CreateSectionGroup(FRealtimeMeshSectionGroupKey SectionGroupKey,
		const FRealtimeMeshSectionGroupProxyInitializationParametersRef& InitParams)
	{
		check(SectionGroupKey.IsPartOf(Key));
		check(!SectionGroups.IsValidIndex(FRealtimeMeshKeyHelpers::GetSectionGroupIndex(SectionGroupKey)));

		const int32 SectionGroupIndex = FRealtimeMeshKeyHelpers::GetSectionGroupIndex(SectionGroupKey);
		SectionGroups.Insert(SectionGroupIndex, ClassFactory->CreateSectionGroupProxy(ProxyWeak.Pin().ToSharedRef(),
			FRealtimeMeshSectionGroupKey(Key, SectionGroupIndex), InitParams));

		MarkStateDirty();
	}

	void FRealtimeMeshLODProxy::RemoveSectionGroup(FRealtimeMeshSectionGroupKey SectionGroupKey)
	{
		check(SectionGroupKey.IsPartOf(Key));
		check(SectionGroups.IsValidIndex(FRealtimeMeshKeyHelpers::GetSectionGroupIndex(SectionGroupKey)));

		const int32 SectionGroupIndex = FRealtimeMeshKeyHelpers::GetSectionGroupIndex(SectionGroupKey);
		SectionGroups.RemoveAt(SectionGroupIndex);
		
		MarkStateDirty();
	}

	void FRealtimeMeshLODProxy::RemoveAllSectionGroups()
	{
		SectionGroups.Empty();

		MarkStateDirty();
	}

	void FRealtimeMeshLODProxy::PopulateMeshBatches(ERealtimeMeshSectionDrawType DrawType, bool bForceAllDynamic, const FLODMask& LODMask,
        const TRange<float>& ScreenSizeLimits, bool bIsMovable, bool bIsLocalToWorldDeterminantNegative, bool bCastRayTracedShadow, FMaterialRenderProxy* WireframeMaterial,
        FRHIUniformBuffer* UniformBuffer, const TMap<int32, TTuple<FMaterialRenderProxy*, bool>>& Materials, TFunctionRef<FMeshBatch&()> BatchAllocator,
        TFunctionRef<void(FMeshBatch&, float)> BatchSubmitter, TFunctionRef<void(const TSharedRef<FRenderResource>&)> ResourceSubmitter) const
	{
		const ERealtimeMeshDrawMask DrawTypeMask = bForceAllDynamic ? ERealtimeMeshDrawMask::DrawPassMask :
			DrawType == ERealtimeMeshSectionDrawType::Dynamic ? ERealtimeMeshDrawMask::DrawDynamic :	ERealtimeMeshDrawMask::DrawStatic;

		if (DrawMask.IsAnySet(DrawTypeMask))
		{
			for (const auto& SectionGroup : SectionGroups)			
			{
				if (SectionGroup->GetDrawMask().IsAnySet(DrawTypeMask))
				{
					SectionGroup->PopulateMeshBatches(DrawType, bForceAllDynamic, LODMask, ScreenSizeLimits, bIsMovable, bIsLocalToWorldDeterminantNegative, bCastRayTracedShadow,
						WireframeMaterial, UniformBuffer, Materials, BatchAllocator, BatchSubmitter, ResourceSubmitter);
				}
			}
		}
	}
	
	void FRealtimeMeshLODProxy::MarkStateDirty()
	{
		bIsStateDirty = true;
	}

	bool FRealtimeMeshLODProxy::HandleUpdates(bool bShouldForceUpdate)
	{
		// Handle all SectionGroup updates
		for (const auto& SectionGroup : SectionGroups)
		{
			bIsStateDirty |= SectionGroup->HandleUpdates(bShouldForceUpdate);
		}
		
		if (bIsStateDirty)
		{
			bIsStateDirty = false;

			FRealtimeMeshDrawMask NewDrawMask;
			if (Config.bIsVisible && Config.ScreenSize >= 0)
			{
				for (TSparseArray<FRealtimeMeshSectionGroupProxyRef>::TIterator It(SectionGroups); It; ++It)
				{
					NewDrawMask |= (*It)->GetDrawMask();
				}
			}

			DrawMask = NewDrawMask;
			return true;			
		}
		return false;
	}
	
	void FRealtimeMeshLODProxy::Reset()
	{
		// Reset all the section groups
		for (auto& SectionGroup : SectionGroups)
		{
			SectionGroup->Reset();
		}
		SectionGroups.Empty();
		
		Config = FRealtimeMeshLODConfig();
		DrawMask = FRealtimeMeshDrawMask();
		bIsStateDirty = false;
	}
}
