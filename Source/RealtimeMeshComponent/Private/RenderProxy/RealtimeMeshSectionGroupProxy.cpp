// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#include "RenderProxy/RealtimeMeshSectionGroupProxy.h"

#include "MaterialDomain.h"
#include "RealtimeMeshShared.h"
#include "RenderProxy/RealtimeMeshLODProxy.h"
#include "RenderProxy/RealtimeMeshProxy.h"
#include "RenderProxy/RealtimeMeshSectionProxy.h"
#include "RenderProxy/RealtimeMeshVertexFactory.h"
#include "Materials/Material.h"

namespace RealtimeMesh
{
	FRealtimeMeshSectionGroupProxy::FRealtimeMeshSectionGroupProxy(const FRealtimeMeshSharedResourcesRef& InSharedResources, const FRealtimeMeshSectionGroupKey& InKey)
		: SharedResources(InSharedResources)
		  , Key(InKey)
		  , VertexFactory(SharedResources->CreateVertexFactory())
		  , bIsStateDirty(true)
	{
	}

	FRealtimeMeshSectionGroupProxy::~FRealtimeMeshSectionGroupProxy()
	{
		check(IsInRenderingThread());
		Reset();
	}

	FRealtimeMeshSectionProxyPtr FRealtimeMeshSectionGroupProxy::GetSection(const FRealtimeMeshSectionKey& SectionKey) const
	{
		check(SectionKey.IsPartOf(Key));

		if (SectionMap.Contains(SectionKey))
		{
			return Sections[SectionMap[SectionKey]];
		}
		return FRealtimeMeshSectionProxyPtr();
	}

	TSharedPtr<FRealtimeMeshGPUBuffer> FRealtimeMeshSectionGroupProxy::GetStream(const FRealtimeMeshStreamKey& StreamKey) const
	{
		return Streams.FindRef(StreamKey);
	}


	void FRealtimeMeshSectionGroupProxy::CreateSectionIfNotExists(const FRealtimeMeshSectionKey& SectionKey)
	{
		check(SectionKey.IsPartOf(Key));

		// Does this section already exist
		if (!SectionMap.Contains(SectionKey))
		{
			const int32 SectionIndex = Sections.Add(SharedResources->CreateSectionProxy(SectionKey));
			SectionMap.Add(SectionKey, SectionIndex);
			MarkStateDirty();
		}
		else
		{
			Sections[SectionMap[SectionKey]]->Reset();
		}
	}

	void FRealtimeMeshSectionGroupProxy::RemoveSection(const FRealtimeMeshSectionKey& SectionKey)
	{
		check(SectionKey.IsPartOf(Key));

		if (SectionMap.Contains(SectionKey))
		{
			const int32 SectionIndex = SectionMap[SectionKey];
			Sections.RemoveAt(SectionIndex);
			RebuildSectionMap();
			MarkStateDirty();
		}
	}

	void FRealtimeMeshSectionGroupProxy::CreateOrUpdateStream(const FRealtimeMeshSectionGroupStreamUpdateDataRef& InStream)
	{
		// If we didn't create the buffers async, create them now
		InStream->InitializeIfRequired();

		TSharedPtr<FRealtimeMeshGPUBuffer, ESPMode::ThreadSafe> GPUBuffer;


		// If we have the stream already, just update it
		if (const TSharedPtr<FRealtimeMeshGPUBuffer, ESPMode::ThreadSafe>* FoundBuffer = Streams.Find(InStream->GetStreamKey()))
		{
			// We only stream in place if the existing buffer is zero or the same size as the new one
			if (FoundBuffer->Get()->Num() == 0 || FoundBuffer->Get()->Num() == InStream->GetNumElements())
			{
				GPUBuffer = *FoundBuffer;
			}
			else
			{
				(*FoundBuffer)->ReleaseUnderlyingResource();
			}
		}

		if (!GPUBuffer)
		{
			GPUBuffer = InStream->GetStreamKey().GetStreamType() == ERealtimeMeshStreamType::Vertex
				            ? StaticCastSharedRef<FRealtimeMeshGPUBuffer>(MakeShared<FRealtimeMeshVertexBuffer>())
				            : StaticCastSharedRef<FRealtimeMeshGPUBuffer>(MakeShared<FRealtimeMeshIndexBuffer>());

			// We must initialize the resources first before we then apply a buffer to it.
			GPUBuffer->InitializeResources();

			// Add it to the buffer set		
			Streams.Add(InStream->GetStreamKey(), GPUBuffer);
		}

		check(GPUBuffer);
		check(GPUBuffer->IsResourceInitialized());

		// TODO: Allow batching across calls 
		TRHIResourceUpdateBatcher<FRealtimeMeshGPUBuffer::RHIUpdateBatchSize> Batcher;
		GPUBuffer->ApplyBufferUpdate(Batcher, InStream);

		MarkStateDirty();
	}

	void FRealtimeMeshSectionGroupProxy::RemoveStream(const FRealtimeMeshStreamKey& StreamKey)
	{
		if (const auto* Stream = Streams.Find(StreamKey))
		{
			(*Stream)->ReleaseUnderlyingResource();
			Streams.Remove(StreamKey);
			MarkStateDirty();
		}
	}

	void FRealtimeMeshSectionGroupProxy::CreateMeshBatches(const FRealtimeMeshBatchCreationParams& Params, const TMap<int32, TTuple<FMaterialRenderProxy*, bool>>& Materials,
	                                                       const FMaterialRenderProxy* WireframeMaterial, ERealtimeMeshSectionDrawType DrawType, bool bForceAllDynamic) const
	{
		const ERealtimeMeshDrawMask DrawTypeMask = bForceAllDynamic
			                                           ? ERealtimeMeshDrawMask::DrawPassMask
			                                           : DrawType == ERealtimeMeshSectionDrawType::Dynamic
			                                           ? ERealtimeMeshDrawMask::DrawDynamic
			                                           : ERealtimeMeshDrawMask::DrawStatic;

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
				Section->CreateMeshBatch(Params, GetVertexFactory().ToSharedRef(), bIsWireframe ? WireframeMaterial : SectionMaterial, bIsWireframe, bSupportsDithering,
				                         &RayTracingGeometry);
#else
				Section->CreateMeshBatch(Params, GetVertexFactory().ToSharedRef(), bIsWireframe? WireframeMaterial : SectionMaterial, bIsWireframe, bSupportsDithering);
#endif
			}
		}
	}

	bool FRealtimeMeshSectionGroupProxy::UpdateCachedState(bool bShouldForceUpdate)
	{
		// Handle the vertex factory first so sections can query it
		if (bIsStateDirty || bShouldForceUpdate)
		{
			VertexFactory->Initialize(Streams);
		}

		// Handle all Section updates
		for (const auto& Section : Sections)
		{
			bIsStateDirty |= Section->UpdateCachedState(bIsStateDirty || bShouldForceUpdate, *this);
		}

		if (!bIsStateDirty && !bShouldForceUpdate)
		{
			return false;
		}

		UpdateRayTracingInfo();

		FRealtimeMeshDrawMask NewDrawMask;
		for (const auto& Section : Sections)
		{
			NewDrawMask |= Section->GetDrawMask();
		}

		const bool bStateChanged = DrawMask != NewDrawMask;
		DrawMask = NewDrawMask;
		bIsStateDirty = false;
		return bStateChanged;
	}

	void FRealtimeMeshSectionGroupProxy::MarkStateDirty()
	{
		bIsStateDirty = true;
	}

	void FRealtimeMeshSectionGroupProxy::Reset()
	{
#if RHI_RAYTRACING
		RayTracingGeometry.ReleaseResource();
#endif
		if (VertexFactory)
		{
			VertexFactory->ReleaseResource();
			VertexFactory.Reset();
		}

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
		bIsStateDirty = true;
	}

	void FRealtimeMeshSectionGroupProxy::UpdateRayTracingInfo()
	{
#if RHI_RAYTRACING
		RayTracingGeometry.ReleaseResource();
		if (DrawMask.HasAnyFlags() && VertexFactory.IsValid() && ShouldCreateRayTracingData() && IsRayTracingEnabled())
		{
			const auto PositionStream = StaticCastSharedPtr<FRealtimeMeshVertexBuffer>(Streams.FindChecked(
				FRealtimeMeshStreamKey(ERealtimeMeshStreamType::Vertex, FRealtimeMeshStreams::PositionStreamName)));
			const auto IndexStream = StaticCastSharedPtr<FRealtimeMeshIndexBuffer>(Streams.FindChecked(
				FRealtimeMeshStreamKey(ERealtimeMeshStreamType::Index, FRealtimeMeshStreams::TrianglesStreamName)));

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

	void FRealtimeMeshSectionGroupProxy::RebuildSectionMap()
	{
		SectionMap.Empty();
		for (auto It = Sections.CreateIterator(); It; ++It)
		{
			SectionMap.Add((*It)->GetKey(), It.GetIndex());
		}
	}
}
