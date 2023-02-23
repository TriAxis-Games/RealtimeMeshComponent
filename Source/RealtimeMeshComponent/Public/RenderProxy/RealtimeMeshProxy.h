// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "RealtimeMeshCore.h"
#include "RealtimeMeshProxyUtils.h"
#include "HAL/ThreadSafeBool.h"
#include "Containers/Queue.h"
#include "Data/RealtimeMeshConfig.h"

enum class ERealtimeMeshSectionDrawType : uint8;

namespace RealtimeMesh
{
	struct REALTIMEMESHCOMPONENT_API FRealtimeMeshProxyInitializationParameters
	{		
		TFixedLODArray<FRealtimeMeshLODProxyInitializationParametersRef> LODs;
	};
	
	class REALTIMEMESHCOMPONENT_API FRealtimeMeshProxy : public TSharedFromThis<FRealtimeMeshProxy>
	{
	protected:
		FRealtimeMeshClassFactoryRef ClassFactory;
		FRealtimeMeshWeakPtr MeshWeak;
		TFixedLODArray<FRealtimeMeshLODProxyRef> LODs;
		FRealtimeMeshDrawMask DrawMask;
		TRange<int8> ValidLODRange;
		TArray<TRange<float>> ScreenSizeRangeByLOD;
		mutable FThreadSafeBool IsQueuedForUpdate;
		mutable TQueue<TUniqueFunction<void(const FRealtimeMeshProxyRef&)>, EQueueMode::Mpsc> PendingUpdates;

		uint32 bIsStateDirty : 1;
	public:
		FRealtimeMeshProxy(const FRealtimeMeshClassFactoryRef& InClassFactory, const FRealtimeMeshRef& InMesh);

		virtual ~FRealtimeMeshProxy()
		{
			// The mesh proxy can only be safely destroyed from the rendering thread.
			// This is so that all the resources can be safely freed correctly.
			check(IsInRenderingThread());
		}

		virtual void InitializeRenderThreadResources(const FRealtimeMeshProxyInitializationParametersRef& InitParams);
		
		virtual ERHIFeatureLevel::Type GetRHIFeatureLevel() const;
		FRealtimeMeshDrawMask GetDrawMask() const { return DrawMask; }
		TRange<int8> GetValidLODRange() const { return ValidLODRange; }
		TRange<float> GetScreenSizeRangeForLOD(FRealtimeMeshLODKey LODKey) const { return ScreenSizeRangeByLOD[FRealtimeMeshKeyHelpers::GetLODIndex(LODKey)]; }

		int8 GetNumLODs() const { return LODs.Num(); }
		FRealtimeMeshLODProxyPtr GetLOD(FRealtimeMeshLODKey LODKey) const;
		virtual TRange<float> GetScreenSizeLimits(FRealtimeMeshLODKey LODKey) const;

		virtual void InitializeLODs(const TFixedLODArray<FRealtimeMeshLODProxyInitializationParametersRef>& LODConfigs);
		virtual void AddLOD(const FRealtimeMeshLODKey& NewLODKey, const FRealtimeMeshLODProxyInitializationParametersRef& Config);
		virtual void RemoveTrailingLOD();
		
		virtual void PopulateSectionMeshBatches(ERealtimeMeshSectionDrawType DrawType, bool bForceAllDynamic, uint8 LODIndex, 
			const FLODMask& LODMask, const TRange<float>& ScreenSizeLimits, bool bIsMovable, bool bIsLocalToWorldDeterminantNegative, bool bCastRayTracedShadow,
			FMaterialRenderProxy* WireframeMaterial, FRHIUniformBuffer* UniformBuffer, const TMap<int32, TTuple<FMaterialRenderProxy*, bool>>& Materials,
			TFunction<FMeshBatch&()> BatchAllocator, TFunction<void(FMeshBatch&, float)> BatchSubmitter, TFunction<void(const TSharedRef<FRenderResource>&)> ResourceSubmitter) const;

		void EnqueueRenderingCommand(TUniqueFunction<void(const FRealtimeMeshProxyRef&)>&& InCommand);

		void MarkStateDirty();
		virtual bool HandleUpdates(bool bShouldForceUpdate);
		virtual void Reset();

	protected:
	};
}
