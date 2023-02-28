// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#include "RenderProxy/RealtimeMeshSectionGroupProxy.h"
#include "RenderProxy/RealtimeMeshLODProxy.h"
#include "RenderProxy/RealtimeMeshProxy.h"
#include "RenderProxy/RealtimeMeshSectionProxy.h"
#include "RenderProxy/RealtimeMeshVertexFactory.h"
#include "Materials/Material.h"

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

		TArray<FRealtimeMeshStreamKey> UpdatedStreams;
		for (const auto& Stream : InStreams)
		{
			UpdatedStreams.Add(Stream->GetStreamKey());
			CreateOrUpdateStreamImplementation(Batcher, Stream);
		}

		AlertSectionsOfStreamUpdates(UpdatedStreams, { });
		
		MarkStateDirty();
	}

	void FRealtimeMeshSectionGroupProxy::RemoveStream(const TArray<FRealtimeMeshStreamKey>& InStreams)
	{
		for (const auto& Stream : InStreams)
		{
			Streams[Stream]->ReleaseUnderlyingResource();
			Streams.Remove(Stream);
		}
		
		AlertSectionsOfStreamUpdates({ }, InStreams);

		MarkStateDirty();
	}

	void FRealtimeMeshSectionGroupProxy::CreateMeshBatches(const FRealtimeMeshBatchCreationParams& Params, const TMap<int32, TTuple<FMaterialRenderProxy*, bool>>& Materials, const FMaterialRenderProxy* WireframeMaterial, ERealtimeMeshSectionDrawType DrawType, bool bForceAllDynamic) const
	{
		const ERealtimeMeshDrawMask DrawTypeMask = bForceAllDynamic ? ERealtimeMeshDrawMask::DrawPassMask :
													   DrawType == ERealtimeMeshSectionDrawType::Dynamic ? ERealtimeMeshDrawMask::DrawDynamic :	ERealtimeMeshDrawMask::DrawStatic;

		check(DrawMask.IsAnySet(DrawTypeMask));

		for (const auto& Section : Sections)
		{
			if (Section->GetDrawMask().IsAnySet(DrawTypeMask))
			{
				check(GetVertexFactory() && GetVertexFactory().IsValid() && GetVertexFactory()->IsInitialized());

				const bool bIsWireframe = WireframeMaterial != nullptr;
				
				FMaterialRenderProxy* SectionMaterial = nullptr;
				bool bSupportsDithering = false;

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
					ensure(SectionMaterial);
				}

#if RHI_RAYTRACING
				Section->CreateMeshBatch(Params, GetVertexFactory().ToSharedRef(), bIsWireframe? WireframeMaterial : SectionMaterial, bIsWireframe, bSupportsDithering, &RayTracingGeometry);
#else
				Section->CreateMeshBatch(Params, GetVertexFactory().ToSharedRef(), bIsWireframe? WireframeMaterial : SectionMaterial, bIsWireframe, bSupportsDithering);
#endif
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
		}
		
		// Handle all Section updates
		for (const auto& Section : Sections)
		{
			bIsStateDirty |= Section->HandleUpdates(bShouldForceUpdate);
		}

		bool bHadSectionUpdates = false;

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
			bHadSectionUpdates = true;
		}

		if (bHadSectionUpdates)
		{
			UpdateRayTracingInfo();
		}

		return bHadSectionUpdates;
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

	void FRealtimeMeshSectionGroupProxy::UpdateRayTracingInfo()
	{
#if RHI_RAYTRACING
		RayTracingGeometry.ReleaseResource();
		if (DrawMask.HasAnyFlags() && VertexFactory.IsValid() && ShouldCreateRayTracingData() && IsRayTracingEnabled())
		{
			auto PositionStream = StaticCastSharedPtr<FRealtimeMeshVertexBuffer>(Streams.FindChecked(
				FRealtimeMeshStreamKey(ERealtimeMeshStreamType::Vertex, FRealtimeMeshLocalVertexFactory::PositionStreamName)));
			auto IndexStream = StaticCastSharedPtr<FRealtimeMeshIndexBuffer>(Streams.FindChecked(
				FRealtimeMeshStreamKey(ERealtimeMeshStreamType::Index, FRealtimeMeshLocalVertexFactory::TrianglesStreamName)));
      			
			FRayTracingGeometryInitializer Initializer;
			// TODO: Get better debug name
			Initializer.DebugName = TEXT("RealtimeMeshComponent");
			Initializer.IndexBuffer = IndexStream->IndexBufferRHI;
			Initializer.TotalPrimitiveCount = 0;
			Initializer.GeometryType = RTGT_Triangles;
			Initializer.bFastBuild = true;
			Initializer.bAllowUpdate = false;

			for (const auto& Section : Sections)
			{
				if (Section->GetDrawMask().IsAnySet(ERealtimeMeshDrawMask::DrawDynamic | ERealtimeMeshDrawMask::DrawStatic))
				{
					check(GetVertexFactory() && GetVertexFactory().IsValid() && GetVertexFactory()->IsInitialized());

					FRayTracingGeometrySegment Segment;
					Segment.VertexBuffer = PositionStream->VertexBufferRHI;
					Segment.VertexBufferOffset = Section->GetStreamRange().GetMinVertex();
					Segment.MaxVertices = Section->GetStreamRange().NumVertices();
					Segment.FirstPrimitive = Section->GetStreamRange().GetMinIndex() / 3;
					Segment.NumPrimitives = Section->GetStreamRange().NumPrimitives(3);
					Segment.bEnabled = true;
					Initializer.TotalPrimitiveCount += Segment.NumPrimitives;
					Initializer.Segments.Add(Segment);
				}
			}			
			
			RayTracingGeometry.SetInitializer(Initializer);
			RayTracingGeometry.InitResource();
			check(RayTracingGeometry.RayTracingGeometryRHI.IsValid());
		}
#endif
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
		
	void FRealtimeMeshSectionGroupProxy::AlertSectionsOfStreamUpdates(const TArray<FRealtimeMeshStreamKey>& AddedOrUpdatedStreams,
		const TArray<FRealtimeMeshStreamKey>& RemovedStreams)
	{
		for (const auto& Section : Sections)
		{
			Section->OnStreamsUpdated(AddedOrUpdatedStreams, RemovedStreams);
		}
	}
}
