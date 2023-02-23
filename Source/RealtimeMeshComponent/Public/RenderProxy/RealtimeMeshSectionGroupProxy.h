// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "RealtimeMeshGPUBuffer.h"
#include "RealtimeMeshProxyUtils.h"
#include "RealtimeMeshSectionProxy.h"
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
		TSharedPtr<FRealtimeMeshVertexFactory> GetVertexFactory() const;
		FRealtimeMeshDrawMask GetDrawMask() const { return DrawMask; }

		FRealtimeMeshSectionProxyPtr GetSection(FRealtimeMeshSectionKey SectionKey) const;
		TSharedPtr<FRealtimeMeshGPUBuffer> GetStream(FRealtimeMeshStreamKey StreamKey) const;

		void CreateSection(FRealtimeMeshSectionKey SectionKey, const FRealtimeMeshSectionProxyInitializationParametersRef& InitParams);
		void RemoveSection(FRealtimeMeshSectionKey SectionKey);
		void RemoveAllSections();

		void CreateOrUpdateStreams(const TArray<FRealtimeMeshSectionGroupStreamUpdateDataRef>& InStreams);
		void RemoveStream(const TArray<FRealtimeMeshStreamKey>& InStreams);

		void PopulateMeshBatches(ERealtimeMeshSectionDrawType DrawType, bool bForceAllDynamic, const FLODMask& LODMask,
		    const TRange<float>& ScreenSizeLimits, bool bIsMovable, bool bIsLocalToWorldDeterminantNegative, bool bCastRayTracedShadow, FMaterialRenderProxy* WireframeMaterial,
		    FRHIUniformBuffer* UniformBuffer, const TMap<int32, TTuple<FMaterialRenderProxy*, bool>>& Materials, TFunctionRef<FMeshBatch&()> BatchAllocator,
		    TFunctionRef<void(FMeshBatch&, float)> BatchSubmitter, TFunctionRef<void(const TSharedRef<FRenderResource>&)> ResourceSubmitter) const;


		void MarkStateDirty();
		virtual bool HandleUpdates(bool bShouldForceUpdate);
		virtual void Reset();

	private:
		
		void CreateOrUpdateStreamImplementation(TRHIResourceUpdateBatcher<FRealtimeMeshGPUBuffer::RHIUpdateBatchSize>& Batcher,
			const FRealtimeMeshSectionGroupStreamUpdateDataRef& StreamData);

		void CreateSectionImplementation(FRealtimeMeshSectionKey SectionKey, const FRealtimeMeshSectionProxyInitializationParametersRef& InitParams);
	};

}
