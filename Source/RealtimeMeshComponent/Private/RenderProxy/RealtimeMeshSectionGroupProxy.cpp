// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#include "RenderProxy/RealtimeMeshSectionGroupProxy.h"
#include "RenderProxy/RealtimeMeshLODProxy.h"
#include "RenderProxy/RealtimeMeshProxy.h"
#include "RenderProxy/RealtimeMeshSectionProxy.h"
#include "RenderProxy/RealtimeMeshVertexFactory.h"

namespace RealtimeMesh
{
	FRealtimeMeshSectionGroupProxy::FRealtimeMeshSectionGroupProxy(const FRealtimeMeshClassFactoryRef& InClassFactory, const FRealtimeMeshProxyRef& InProxy,
		FRealtimeMeshSectionGroupKey InKey, const FRealtimeMeshSectionGroupProxyInitializationParametersRef& InInitParams)
		: ClassFactory(InClassFactory)
		, ProxyWeak(InProxy)
		, Key(InKey)
		, VertexFactory(InClassFactory->CreateVertexFactory(InProxy->GetRHIFeatureLevel()))
		, bIsStateDirty(true)
	{
		Streams.Reserve(InInitParams->Streams.Num());
		TRHIResourceUpdateBatcher<FRealtimeMeshGPUBuffer::RHIUpdateBatchSize> Batcher;
		for (const auto& Entry : InInitParams->Streams)
		{
			CreateOrUpdateStreamImplementation(Batcher, Entry);
		}
		Batcher.Flush();

		Sections.Reserve(InInitParams->Sections.Num());
		for (TSparseArray<FRealtimeMeshSectionProxyInitializationParametersRef>::TConstIterator It(InInitParams->Sections); It; ++It)
		{
			CreateSectionImplementation(FRealtimeMeshSectionKey(Key, It.GetIndex()), *It);
		}		
	}
	
	FRealtimeMeshSectionGroupProxy::~FRealtimeMeshSectionGroupProxy()
	{
		check(IsInRenderingThread());
		
		Reset();
	}

	TSharedPtr<FRealtimeMeshVertexFactory> FRealtimeMeshSectionGroupProxy::GetVertexFactory() const
	{
		return VertexFactory;
	}

	FRealtimeMeshSectionProxyPtr FRealtimeMeshSectionGroupProxy::GetSection(FRealtimeMeshSectionKey SectionKey) const
	{
		if (SectionKey.IsPartOf(Key) && Sections.IsValidIndex(FRealtimeMeshKeyHelpers::GetSectionIndex(SectionKey)))
		{
			return Sections[FRealtimeMeshKeyHelpers::GetSectionIndex(SectionKey)];
		}
		return FRealtimeMeshSectionProxyPtr();
	}

	TSharedPtr<FRealtimeMeshGPUBuffer> FRealtimeMeshSectionGroupProxy::GetStream(FRealtimeMeshStreamKey StreamKey) const
	{
		return Streams.FindRef(StreamKey);
	}

	void FRealtimeMeshSectionGroupProxy::CreateSection(FRealtimeMeshSectionKey SectionKey, const FRealtimeMeshSectionProxyInitializationParametersRef& InitParams)
	{
		check(SectionKey.IsPartOf(Key));
		check(!Sections.IsValidIndex(FRealtimeMeshKeyHelpers::GetSectionIndex(SectionKey)));

		CreateSectionImplementation(SectionKey, InitParams);
		
		MarkStateDirty();		
	}

	void FRealtimeMeshSectionGroupProxy::RemoveSection(FRealtimeMeshSectionKey SectionKey)
	{
		check(SectionKey.IsPartOf(Key));
		check(Sections.IsValidIndex(FRealtimeMeshKeyHelpers::GetSectionIndex(SectionKey)));

		const int32 SectionIndex = FRealtimeMeshKeyHelpers::GetSectionIndex(SectionKey);

		Sections[SectionIndex]->Reset();
		Sections.RemoveAt(SectionIndex);
		
		MarkStateDirty();	
	}

	void FRealtimeMeshSectionGroupProxy::RemoveAllSections()
	{
		for (const auto& Section : Sections)
		{
			Section->Reset();
		}
		Sections.Empty();

		MarkStateDirty();
	}

	void FRealtimeMeshSectionGroupProxy::CreateOrUpdateStreams(const TArray<FRealtimeMeshSectionGroupStreamUpdateDataRef>& InStreams)
	{
		TRHIResourceUpdateBatcher<FRealtimeMeshGPUBuffer::RHIUpdateBatchSize> Batcher;

		for (const auto& Stream : InStreams)
		{
			CreateOrUpdateStreamImplementation(Batcher, Stream);
		}
		
		MarkStateDirty();
	}

	void FRealtimeMeshSectionGroupProxy::RemoveStream(const TArray<FRealtimeMeshStreamKey>& InStreams)
	{
		for (const auto& Stream : InStreams)
		{
			Streams[Stream]->ReleaseUnderlyingResource();
			Streams.Remove(Stream);
		}

		MarkStateDirty();
	}

	void FRealtimeMeshSectionGroupProxy::PopulateMeshBatches(ERealtimeMeshSectionDrawType DrawType, bool bForceAllDynamic, const FLODMask& LODMask,
		const TRange<float>& ScreenSizeLimits, bool bIsMovable, bool bIsLocalToWorldDeterminantNegative, bool bCastRayTracedShadow, FMaterialRenderProxy* WireframeMaterial,
		FRHIUniformBuffer* UniformBuffer, const TMap<int32, TTuple<FMaterialRenderProxy*, bool>>& Materials, TFunctionRef<FMeshBatch&()> BatchAllocator,
		TFunctionRef<void(FMeshBatch&, float)> BatchSubmitter, TFunctionRef<void(const TSharedRef<FRenderResource>&)> ResourceSubmitter) const
	{
		const ERealtimeMeshDrawMask DrawTypeMask = bForceAllDynamic ? ERealtimeMeshDrawMask::DrawPassMask :
			                                           DrawType == ERealtimeMeshSectionDrawType::Dynamic ? ERealtimeMeshDrawMask::DrawDynamic :	ERealtimeMeshDrawMask::DrawStatic;

		check(DrawMask.IsAnySet(DrawTypeMask));

		for (const auto& Section : Sections)
		{
			if (Section->GetDrawMask().IsAnySet(DrawTypeMask))
			{
				check(GetVertexFactory() && GetVertexFactory().IsValid() && GetVertexFactory()->IsInitialized());

				FMaterialRenderProxy* SectionMaterial = nullptr;
				bool bSupportsDithering = false;
				bool bIsWireframe = WireframeMaterial != nullptr;

				if (!bIsWireframe)
				{
					const TTuple<FMaterialRenderProxy*, bool>* MatEntry = Materials.Find(Section->GetMaterialSlot());
					if (MatEntry != nullptr && MatEntry->Get<0>() != nullptr)
					{
						SectionMaterial = MatEntry->Get<0>();
						bSupportsDithering = MatEntry->Get<1>();
					}
					else
					{
						SectionMaterial = UMaterial::GetDefaultMaterial(MD_Surface)->GetRenderProxy();
					}
				}

				Section->CreateMeshBatch(this->AsShared(), LODMask, ScreenSizeLimits, bIsMovable, bIsLocalToWorldDeterminantNegative, UniformBuffer,
				    bIsWireframe ? WireframeMaterial : SectionMaterial, bIsWireframe, bSupportsDithering, bCastRayTracedShadow, BatchAllocator, BatchSubmitter, ResourceSubmitter);
			}
		}
	}

	void FRealtimeMeshSectionGroupProxy::MarkStateDirty()
	{
		bIsStateDirty = true;
	}

	bool FRealtimeMeshSectionGroupProxy::HandleUpdates(bool bShouldForceUpdate)
	{
		// Handle the vertex factory first so sections can query it
		if (bIsStateDirty || bShouldForceUpdate)
		{		
			VertexFactory->Initialize(Streams);

			// #if RHI_RAYTRACING
			// if (bShouldSupportRayTracing && IsRayTracingEnabled())
			// {
			// 	FRayTracingGeometryInitializer Initializer;
			// 	// TODO: Get better debug name
			// 	Initializer.DebugName = TEXT("RealtimeMeshComponent");;
			// 	Initializer.IndexBuffer = nullptr;
			// 	Initializer.TotalPrimitiveCount = 0;
			// 	Initializer.GeometryType = RTGT_Triangles;
			// 	Initializer.bFastBuild = true;
			// 	Initializer.bAllowUpdate = false;
			// 	
			// 	RayTracingGeometry.SetInitializer(Initializer);
			// 	RayTracingGeometry.InitResource();
			// 	
			// 	RayTracingGeometry.Initializer.IndexBuffer = IndexBuffer.IndexBufferRHI;
			// 	RayTracingGeometry.Initializer.TotalPrimitiveCount = IndexBuffer.Indices.Num() / 3;
			// 	
			// 	FRayTracingGeometrySegment Segment;
			// 	Segment.VertexBuffer = VertexBuffers.PositionVertexBuffer.VertexBufferRHI;
			// 	Segment.NumPrimitives = RayTracingGeometry.Initializer.TotalPrimitiveCount;
			// 	Segment.MaxVertices = VertexBuffers.PositionVertexBuffer.GetNumVertices();
			// 	RayTracingGeometry.Initializer.Segments.Add(Segment);
			// 	
			// 	RayTracingGeometry.UpdateRHI();
			// }
			// #endif
			
		}
		
		// Handle all Section updates
		for (const auto& Section : Sections)
		{
			bIsStateDirty |= Section->HandleUpdates(bShouldForceUpdate);
		}

		// Handle remaining state updates
		if (bIsStateDirty || bShouldForceUpdate)
		{
			bIsStateDirty = false;

			FRealtimeMeshDrawMask NewDrawMask;
			for (const auto& Section : Sections)
			{
				NewDrawMask |= Section->GetDrawMask();
			}

			DrawMask = NewDrawMask;
			return true;
		}
		return false;
	}

	void FRealtimeMeshSectionGroupProxy::Reset()
	{		
		VertexFactory->ReleaseResource();
#if RHI_RAYTRACING
		RayTracingGeometry.ReleaseResource();
#endif

		// Reset the streams and release all resources.
		for (const auto& Stream : Streams)
		{
			Stream.Value->ReleaseUnderlyingResource();
		}
		Streams.Empty();

		// Reset the sections and clear them
		for (const auto& Section : Sections)
		{
			Section->Reset();
		}
		Sections.Empty();
		
		DrawMask = FRealtimeMeshDrawMask();
		bIsStateDirty = false;
	}
	
	void FRealtimeMeshSectionGroupProxy::CreateOrUpdateStreamImplementation(TRHIResourceUpdateBatcher<FRealtimeMeshGPUBuffer::RHIUpdateBatchSize>& Batcher,
		const FRealtimeMeshSectionGroupStreamUpdateDataRef& StreamData)
	{
		// If we didn't create the buffers async, create them now
		StreamData->InitializeIfRequired();

		TSharedPtr<FRealtimeMeshGPUBuffer, ESPMode::ThreadSafe> GPUBuffer;
		
		// If we have the stream already, just update it
		if (const TSharedPtr<FRealtimeMeshGPUBuffer, ESPMode::ThreadSafe>* FoundBuffer = Streams.Find(StreamData->GetStreamKey()))
		{
			GPUBuffer = *FoundBuffer;
		}
		else
		{
			GPUBuffer = StreamData->GetStreamKey().GetStreamType() == ERealtimeMeshStreamType::Vertex
				 ? StaticCastSharedRef<FRealtimeMeshGPUBuffer>(MakeShared<FRealtimeMeshVertexBuffer>())
				 : StaticCastSharedRef<FRealtimeMeshGPUBuffer>(MakeShared<FRealtimeMeshIndexBuffer>());
			
			// We must initialize the resources first before we then apply a buffer to it.
			GPUBuffer->InitializeResources();
			
			// Add it to the buffer set		
			Streams.Add(StreamData->GetStreamKey(), GPUBuffer);
		}

		GPUBuffer->ApplyBufferUpdate(Batcher, StreamData);		

		MarkStateDirty();
	}

	void FRealtimeMeshSectionGroupProxy::CreateSectionImplementation(FRealtimeMeshSectionKey SectionKey,
		const FRealtimeMeshSectionProxyInitializationParametersRef& InitParams)
	{
		const int32 SectionIndex = FRealtimeMeshKeyHelpers::GetSectionIndex(SectionKey);
		Sections.Insert(SectionIndex, ClassFactory->CreateSectionProxy(ProxyWeak.Pin().ToSharedRef(), SectionKey, InitParams));
	}
}
