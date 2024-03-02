// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "RealtimeMeshCore.h"
#include "RealtimeMeshProxyShared.h"
#include "RealtimeMeshConfig.h"
#include "HAL/ThreadSafeBool.h"
#include "Containers/Queue.h"

enum class ERealtimeMeshSectionDrawType : uint8;

namespace RealtimeMesh
{
	class REALTIMEMESHCOMPONENT_API FRealtimeMeshProxy : public TSharedFromThis<FRealtimeMeshProxy>
	{
	protected:
		const FRealtimeMeshSharedResourcesRef SharedResources;
		TFixedLODArray<FRealtimeMeshLODProxyPtr> LODs;
		FRealtimeMeshDrawMask DrawMask;
		TRange<int8> ValidLODRange;
		TArray<TRange<float>> ScreenSizeRangeByLOD;
		uint32 bIsStateDirty : 1;

	public:
		FRealtimeMeshProxy(const FRealtimeMeshSharedResourcesRef& InSharedResources);
		virtual ~FRealtimeMeshProxy();

		const FRealtimeMeshSharedResourcesRef& GetSharedResources() const { return SharedResources; }

		virtual ERHIFeatureLevel::Type GetRHIFeatureLevel() const;
		FRealtimeMeshDrawMask GetDrawMask() const { return DrawMask; }
		TRange<int8> GetValidLODRange() const { return ValidLODRange; }
		TRange<float> GetScreenSizeRangeForLOD(const FRealtimeMeshLODKey& LODKey) const;

		int8 GetNumLODs() const { return LODs.Num(); }
		FRealtimeMeshLODProxyPtr GetLOD(FRealtimeMeshLODKey LODKey) const;

		virtual void AddLODIfNotExists(const FRealtimeMeshLODKey& LODKey);
		virtual void RemoveLOD(const FRealtimeMeshLODKey& LODKey);

		virtual void CreateMeshBatches(int32 LODIndex, const FRealtimeMeshBatchCreationParams& Params, const TMap<int32, TTuple<FMaterialRenderProxy*, bool>>& Materials,
		                               const FMaterialRenderProxy* WireframeMaterial, ERealtimeMeshSectionDrawType DrawType, bool bForceAllDynamic) const;

		virtual bool UpdatedCachedState(bool bShouldForceUpdate);
		virtual void Reset();

	protected:
		void MarkStateDirty();
	};
}
