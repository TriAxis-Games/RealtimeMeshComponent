// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "RealtimeMeshGPUBuffer.h"
#include "RealtimeMeshProxyUtils.h"
#include "RealtimeMeshVertexFactory.h"

namespace RealtimeMesh
{
	struct REALTIMEMESHCOMPONENT_API FRealtimeMeshSectionGroupProxyInitializationParameters
	{
		TArray<FRealtimeMeshSectionGroupStreamUpdateDataRef, TInlineAllocator<8>> Streams;
		TSparseArray<FRealtimeMeshSectionProxyInitializationParametersRef> Sections;
	};

	using FRealtimeMeshSectionGroupProxyStreamMap = TMap<FRealtimeMeshStreamKey, TSharedPtr<FRealtimeMeshGPUBuffer>>;

	
	class REALTIMEMESHCOMPONENT_API FRealtimeMeshSectionGroupProxy : public TSharedFromThis<FRealtimeMeshSectionGroupProxy>
	{
	private:
		FRealtimeMeshClassFactoryRef ClassFactory;
		FRealtimeMeshProxyWeakPtr ProxyWeak;	
		FRealtimeMeshSectionGroupKey Key;
		TSharedPtr<FRealtimeMeshVertexFactory> VertexFactory;
		TSparseArray<FRealtimeMeshSectionProxyRef> Sections;
		FRealtimeMeshSectionGroupProxyStreamMap Streams;
#if RHI_RAYTRACING
		FRayTracingGeometry RayTracingGeometry;
#endif

		FRealtimeMeshDrawMask DrawMask;
		uint32 bIsStateDirty : 1;
		
	public:
		FRealtimeMeshSectionGroupProxy(const FRealtimeMeshClassFactoryRef& InClassFactory, const FRealtimeMeshProxyRef& InProxy,
			FRealtimeMeshSectionGroupKey InKey, const FRealtimeMeshSectionGroupProxyInitializationParametersRef& InInitParams);
		virtual ~FRealtimeMeshSectionGroupProxy();
		
		FRealtimeMeshSectionGroupKey GetKey() const { return Key; }
		TSharedPtr<FRealtimeMeshVertexFactory> GetVertexFactory() const { return VertexFactory; }
		FRealtimeMeshDrawMask GetDrawMask() const { return DrawMask; }

		FRealtimeMeshSectionProxyPtr GetSection(FRealtimeMeshSectionKey SectionKey) const;
		TSharedPtr<FRealtimeMeshGPUBuffer> GetStream(FRealtimeMeshStreamKey StreamKey) const;

		virtual bool ShouldCreateRayTracingData() const { return Key.GetLODKey() == 0; }

		void CreateSection(FRealtimeMeshSectionKey SectionKey, const FRealtimeMeshSectionProxyInitializationParametersRef& InitParams);
		void RemoveSection(FRealtimeMeshSectionKey SectionKey);
		void RemoveAllSections();

		void CreateOrUpdateStreams(const TArray<FRealtimeMeshSectionGroupStreamUpdateDataRef>& InStreams);
		void RemoveStream(const TArray<FRealtimeMeshStreamKey>& InStreams);

		void CreateMeshBatches(const FRealtimeMeshBatchCreationParams& Params, const TMap<int32, TTuple<FMaterialRenderProxy*, bool>>& Materials, const FMaterialRenderProxy* WireframeMaterial, ERealtimeMeshSectionDrawType DrawType, bool bForceAllDynamic) const;
		

		void MarkStateDirty();
		virtual bool HandleUpdates();
		virtual void Reset();

		void UpdateRayTracingInfo();

	private:
		
		void CreateOrUpdateStreamImplementation(TRHIResourceUpdateBatcher<FRealtimeMeshGPUBuffer::RHIUpdateBatchSize>& Batcher,
			const FRealtimeMeshSectionGroupStreamUpdateDataRef& StreamData);

		void CreateSectionImplementation(FRealtimeMeshSectionKey SectionKey, const FRealtimeMeshSectionProxyInitializationParametersRef& InitParams);

		void AlertSectionsOfStreamUpdates(const TArray<FRealtimeMeshStreamKey>& AddedOrUpdatedStreams,
		const TArray<FRealtimeMeshStreamKey>& RemovedStreams);
	};

}
