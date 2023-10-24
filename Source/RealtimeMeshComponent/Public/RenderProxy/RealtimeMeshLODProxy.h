// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "RealtimeMeshConfig.h"
#include "RealtimeMeshCore.h"
#include "RealtimeMeshProxyShared.h"

namespace RealtimeMesh
{
	class REALTIMEMESHCOMPONENT_API FRealtimeMeshLODProxy : public TSharedFromThis<FRealtimeMeshLODProxy>
	{
	private:
		const FRealtimeMeshSharedResourcesRef SharedResources;
		const FRealtimeMeshLODKey Key;
		TArray<FRealtimeMeshSectionGroupProxyRef> SectionGroups;
		TMap<FRealtimeMeshSectionGroupKey, uint32> SectionGroupMap;

		FRealtimeMeshLODConfig Config;
		FRealtimeMeshDrawMask DrawMask;
		uint32 bIsStateDirty : 1;

	public:
		FRealtimeMeshLODProxy(const FRealtimeMeshSharedResourcesRef& InSharedResources, const FRealtimeMeshLODKey& InKey);
		virtual ~FRealtimeMeshLODProxy();

		const FRealtimeMeshLODKey& GetKey() const { return Key; }
		const FRealtimeMeshLODConfig& GetConfig() const { return Config; }
		FRealtimeMeshDrawMask GetDrawMask() const { return DrawMask; }
		float GetScreenSize() const { return Config.ScreenSize; }

		FRealtimeMeshSectionGroupProxyPtr GetSectionGroup(const FRealtimeMeshSectionGroupKey& SectionGroupKey) const;

		virtual void UpdateConfig(const FRealtimeMeshLODConfig& NewConfig);

		virtual void CreateSectionGroupIfNotExists(const FRealtimeMeshSectionGroupKey& SectionGroupKey);
		virtual void RemoveSectionGroup(const FRealtimeMeshSectionGroupKey& SectionGroupKey);

		virtual void CreateMeshBatches(const FRealtimeMeshBatchCreationParams& Params, const TMap<int32, TTuple<FMaterialRenderProxy*, bool>>& Materials,
		                               const FMaterialRenderProxy* WireframeMaterial, ERealtimeMeshSectionDrawType DrawType, bool bForceAllDynamic) const;

		virtual bool UpdateCachedState(bool bShouldForceUpdate);
		virtual void Reset();

	protected:
		void MarkStateDirty();
		void RebuildSectionGroupMap();
	};
}
