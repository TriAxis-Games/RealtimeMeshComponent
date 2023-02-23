// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "RealtimeMeshCore.h"
#include "RealtimeMeshProxyUtils.h"
#include "Data/RealtimeMeshConfig.h"

namespace RealtimeMesh
{
	struct REALTIMEMESHCOMPONENT_API FRealtimeMeshLODProxyInitializationParameters
	{
		FRealtimeMeshLODConfig Config;
		TSparseArray<FRealtimeMeshSectionGroupProxyInitializationParametersRef> SectionGroups;
	};
	
	class REALTIMEMESHCOMPONENT_API FRealtimeMeshLODProxy : public TSharedFromThis<FRealtimeMeshLODProxy>
	{
	private:
		FRealtimeMeshClassFactoryRef ClassFactory;
		FRealtimeMeshProxyWeakPtr ProxyWeak;	
		FRealtimeMeshLODKey Key;
		TSparseArray<FRealtimeMeshSectionGroupProxyRef> SectionGroups;
		FRealtimeMeshLODConfig Config;
		FRealtimeMeshDrawMask DrawMask;
		uint32 bIsStateDirty : 1;

	public:
		FRealtimeMeshLODProxy(const FRealtimeMeshClassFactoryRef& InClassFactory, const FRealtimeMeshProxyRef& InProxy,
					FRealtimeMeshLODKey InKey, const FRealtimeMeshLODProxyInitializationParametersRef& InInitParams);		

		FRealtimeMeshLODKey GetKey() const { return Key; }
		const FRealtimeMeshLODConfig& GetConfig() const { return Config; }
		FRealtimeMeshDrawMask GetDrawMask() const { return DrawMask; }
		float GetScreenSize() const { return Config.ScreenSize; }
		
		FRealtimeMeshSectionGroupProxyPtr GetSectionGroup(FRealtimeMeshSectionGroupKey SectionKey) const;	

		void UpdateConfig(const FRealtimeMeshLODConfig& NewConfig);

		void CreateSectionGroup(FRealtimeMeshSectionGroupKey SectionGroupKey, const FRealtimeMeshSectionGroupProxyInitializationParametersRef& InitParams);
		void RemoveSectionGroup(FRealtimeMeshSectionGroupKey SectionGroupKey);
		void RemoveAllSectionGroups();

		void PopulateMeshBatches(ERealtimeMeshSectionDrawType DrawType, bool bForceAllDynamic, const FLODMask& LODMask,
		                         const TRange<float>& ScreenSizeLimits, bool bIsMovable, bool bIsLocalToWorldDeterminantNegative, bool bCastRayTracedShadow,
		                         FMaterialRenderProxy* WireframeMaterial,
		                         FRHIUniformBuffer* UniformBuffer, const TMap<int32, TTuple<FMaterialRenderProxy*, bool>>& Materials,
		                         TFunctionRef<FMeshBatch&()> BatchAllocator,
		                         TFunctionRef<void(FMeshBatch&, float)> BatchSubmitter,
		                         TFunctionRef<void(const TSharedRef<FRenderResource>&)> ResourceSubmitter) const;


		void MarkStateDirty();
		virtual bool HandleUpdates(bool bShouldForceUpdate);
		virtual void Reset();
	};
}
