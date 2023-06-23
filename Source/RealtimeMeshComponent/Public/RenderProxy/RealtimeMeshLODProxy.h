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

		virtual ~FRealtimeMeshLODProxy();
		
		FRealtimeMeshLODKey GetKey() const { return Key; }
		const FRealtimeMeshLODConfig& GetConfig() const { return Config; }
		FRealtimeMeshDrawMask GetDrawMask() const { return DrawMask; }
		float GetScreenSize() const { return Config.ScreenSize; }
		
		FRealtimeMeshSectionGroupProxyPtr GetSectionGroup(FRealtimeMeshSectionGroupKey SectionKey) const;	

		void UpdateConfig(const FRealtimeMeshLODConfig& NewConfig);

		void CreateSectionGroup(FRealtimeMeshSectionGroupKey SectionGroupKey, const FRealtimeMeshSectionGroupProxyInitializationParametersRef& InitParams);
		void RemoveSectionGroup(FRealtimeMeshSectionGroupKey SectionGroupKey);
		void RemoveAllSectionGroups();

		void CreateMeshBatches(const FRealtimeMeshBatchCreationParams& Params, const TMap<int32, TTuple<FMaterialRenderProxy*, bool>>& Materials,
			const FMaterialRenderProxy* WireframeMaterial, ERealtimeMeshSectionDrawType DrawType, bool bForceAllDynamic) const;


		void MarkStateDirty();
		virtual bool HandleUpdates();
		virtual void Reset();
	};
}
