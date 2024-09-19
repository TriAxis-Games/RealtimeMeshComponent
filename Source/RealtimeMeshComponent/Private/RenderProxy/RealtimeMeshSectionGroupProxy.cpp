// Copyright (c) 2015-2024 TriAxis Games, L.L.C. All Rights Reserved.

#include "RenderProxy/RealtimeMeshSectionGroupProxy.h"

//#include "MaterialDomain.h"
#include "RealtimeMeshComponentModule.h"
#include "Data/RealtimeMeshShared.h"
#include "RenderProxy/RealtimeMeshLODProxy.h"
#if RMC_ENGINE_ABOVE_5_2
#include "MaterialDomain.h"
#endif
#include "RenderProxy/RealtimeMeshSectionProxy.h"
#include "RenderProxy/RealtimeMeshVertexFactory.h"
#include "Materials/Material.h"

namespace RealtimeMesh
{
	FRealtimeMeshSectionGroupProxy::FRealtimeMeshSectionGroupProxy(const FRealtimeMeshSharedResourcesRef& InSharedResources, const FRealtimeMeshSectionGroupKey& InKey)
		: SharedResources(InSharedResources)
		, Key(InKey)
		, VertexFactory(SharedResources->CreateVertexFactory())
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

	FRayTracingGeometry* FRealtimeMeshSectionGroupProxy::GetRayTracingGeometry()
	{
#if RHI_RAYTRACING
		return RayTracingGeometry.IsValid()? &RayTracingGeometry : nullptr;
#else
		return nullptr;
#endif
	}

	void FRealtimeMeshSectionGroupProxy::UpdateConfig(const FRealtimeMeshSectionGroupConfig& NewConfig)
	{
		if (Config != NewConfig)
		{
			Config = NewConfig;
		}
	}

	void FRealtimeMeshSectionGroupProxy::CreateSectionIfNotExists(const FRealtimeMeshSectionKey& SectionKey)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(FRealtimeMeshSectionGroupProxy::CreateSectionIfNotExists);

		check(SectionKey.IsPartOf(Key));

		// Does this section already exist
		if (!SectionMap.Contains(SectionKey))
		{
			const int32 SectionIndex = Sections.Add(SharedResources->CreateSectionProxy(SectionKey));
			SectionMap.Add(SectionKey, SectionIndex);
		}
		else
		{
			Sections[SectionMap[SectionKey]]->Reset();
		}
	}

	void FRealtimeMeshSectionGroupProxy::RemoveSection(const FRealtimeMeshSectionKey& SectionKey)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(FRealtimeMeshSectionGroupProxy::RemoveSection);

		check(SectionKey.IsPartOf(Key));

		if (SectionMap.Contains(SectionKey))
		{
			const int32 SectionIndex = SectionMap[SectionKey];
			Sections.RemoveAt(SectionIndex);
			RebuildSectionMap();
		}
	}

	void FRealtimeMeshSectionGroupProxy::CreateOrUpdateStream(FRHICommandListBase& RHICmdList, const FRealtimeMeshSectionGroupStreamUpdateDataRef& InStream)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(FRealtimeMeshSectionGroupProxy::CreateOrUpdateStream);

		// If we didn't create the buffers async, create them now
		InStream->FinalizeInitialization(RHICmdList);

		check(InStream->GetBuffer().IsValid() && InStream->GetBuffer()->GetSize() > 0);

		// Release any existing stream
		if (const TSharedPtr<FRealtimeMeshGPUBuffer>* FoundBuffer = Streams.Find(InStream->GetStreamKey()))
		{			
			(*FoundBuffer)->ReleaseUnderlyingResource();
			Streams.Remove(InStream->GetStreamKey());
		}

		// Create a new GPU buffer
		TSharedPtr<FRealtimeMeshGPUBuffer> GPUBuffer = InStream->GetStreamKey().GetStreamType() == ERealtimeMeshStreamType::Vertex
							? StaticCastSharedRef<FRealtimeMeshGPUBuffer>(MakeShared<FRealtimeMeshVertexBuffer>(InStream->GetBufferLayout()))
							: StaticCastSharedRef<FRealtimeMeshGPUBuffer>(MakeShared<FRealtimeMeshIndexBuffer>(InStream->GetBufferLayout()));

		GPUBuffer->InitializeResources(RHICmdList, InStream);
		Streams.Add(InStream->GetStreamKey(), GPUBuffer);

		check(GPUBuffer);
		check(GPUBuffer->IsResourceInitialized());

		/*// TODO: Allow batching across calls 
		GPUBuffer->ApplyBufferUpdate(RHICmdList, InStream);*/
	}

	void FRealtimeMeshSectionGroupProxy::RemoveStream(const FRealtimeMeshStreamKey& StreamKey)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(FRealtimeMeshSectionGroupProxy::RemoveStream);

		if (const auto* Stream = Streams.Find(StreamKey))
		{
			(*Stream)->ReleaseUnderlyingResource();
			Streams.Remove(StreamKey);
		}
	}

	bool FRealtimeMeshSectionGroupProxy::InitializeMeshBatch(FMeshBatch& MeshBatch, FRealtimeMeshResourceReferenceList& Resources, bool bIsLocalToWorldDeterminantNegative, bool bWantsDepthOnly) const
	{
		if (ensure(VertexFactory && VertexFactory->IsInitialized()) == false)
		{			
			return false;
		}
		Resources.AddResource(VertexFactory);
		VertexFactory->GatherVertexBufferResources(Resources);

		FMeshBatchElement& BatchElement = MeshBatch.Elements[0];		
		BatchElement.IndexBuffer = &VertexFactory->GetIndexBuffer(bWantsDepthOnly, bIsLocalToWorldDeterminantNegative, Resources);
		MeshBatch.VertexFactory = VertexFactory.Get();
		//MeshBatch.MaterialRenderProxy = Mat;
		
		MeshBatch.LODIndex = Key.LOD();
		MeshBatch.SegmentIndex = 0;

		MeshBatch.ReverseCulling = bIsLocalToWorldDeterminantNegative;
		MeshBatch.bDisableBackfaceCulling = false;

		MeshBatch.CastShadow = DrawMask.ShouldRenderShadow();
		MeshBatch.bUseForMaterial = true;
		MeshBatch.bUseForDepthPass = true;
		MeshBatch.bUseAsOccluder = true;
		//MeshBatch.bWireframe = false;

		MeshBatch.Type = VertexFactory->GetPrimitiveType();
		MeshBatch.DepthPriorityGroup = SDPG_World;

		// TODO: What does this really do?
		MeshBatch.bCanApplyViewModeOverrides = false;

		// TODO: Should we even set this? or let the calling code set it since it has the info needed for knowing about a lod transition
		// Maybe set it to true here and let the calling code turn if off if it's not necessary, lets this also decide it's not allowed
		MeshBatch.bDitheredLODTransition = true;

		// TODO: Support RVT rendering
		MeshBatch.bRenderToVirtualTexture = false;
		MeshBatch.RuntimeVirtualTextureMaterialType = 0;

		MeshBatch.bOverlayMaterial = false;

#if RHI_RAYTRACING
		MeshBatch.CastRayTracedShadow = MeshBatch.CastShadow;
#endif

#if RMC_ENGINE_ABOVE_5_4
		MeshBatch.bUseForWaterInfoTextureDepth = false;
		MeshBatch.bUseForLumenSurfaceCacheCapture = false;
#endif
		
#if UE_ENABLE_DEBUG_DRAWING
		MeshBatch.VisualizeLODIndex = MeshBatch.LODIndex;
#endif

		return true;
	}

	void FRealtimeMeshSectionGroupProxy::UpdateCachedState(FRHICommandListBase& RHICmdList)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(FRealtimeMeshSectionGroupProxy::UpdateCachedState);

		// Handle the vertex factory first so sections can query it

		if (!VertexFactory)
		{
			VertexFactory = SharedResources->CreateVertexFactory();
		}
		
		VertexFactory->Initialize(RHICmdList, Streams);
		
		// Handle all Section updates
		for (auto It = Sections.CreateConstIterator(); It; ++It)
		{
			const FRealtimeMeshSectionProxyRef& Section = *It;
			Section->UpdateCachedState(*this);
		}

		DrawMask = FRealtimeMeshDrawMask();
		ActiveSectionMask.SetNumUninitialized(Sections.Num());
		ActiveSectionMask.SetRange(0, Sections.Num(), false);
		
		for (auto It = Sections.CreateConstIterator(); It; ++It)
		{
			const FRealtimeMeshSectionProxyRef& Section = *It;
			auto SectionDrawMask = Section->GetDrawMask();
			DrawMask |= SectionDrawMask;

			ActiveSectionMask[It.GetIndex()] = SectionDrawMask.ShouldRender();
		}		

		if (DrawMask.HasAnyFlags())
		{
			DrawMask.SetFlag(Config.DrawType == ERealtimeMeshSectionDrawType::Static ? ERealtimeMeshDrawMask::DrawStatic : ERealtimeMeshDrawMask::DrawDynamic);
		}

		UpdateRayTracingInfo(RHICmdList);
	}

	void FRealtimeMeshSectionGroupProxy::Reset()
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(FRealtimeMeshSectionGroupProxy::Reset);

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
		SectionMap.Reset();

		DrawMask = FRealtimeMeshDrawMask();
	}

	void FRealtimeMeshSectionGroupProxy::UpdateRayTracingInfo(FRHICommandListBase& RHICmdList)
	{
#if RHI_RAYTRACING
		TRACE_CPUPROFILER_EVENT_SCOPE(FRealtimeMeshSectionGroupProxy::UpdateRayTracingInfo);
		
		RayTracingGeometry.ReleaseResource();

		if (DrawMask.HasAnyFlags() && VertexFactory.IsValid() && IsRayTracingEnabled())
		{
			check(VertexFactory->IsInitialized());

			const auto PositionStream = StaticCastSharedPtr<FRealtimeMeshVertexBuffer>(Streams.FindChecked(FRealtimeMeshStreams::Position));
			const auto IndexStream = StaticCastSharedPtr<FRealtimeMeshIndexBuffer>(Streams.FindChecked(FRealtimeMeshStreams::Triangles));

			FRayTracingGeometryInitializer Initializer;
			// TODO: Get better debug name
			Initializer.DebugName = TEXT("RealtimeMeshComponent");
			Initializer.IndexBuffer = IndexStream->IndexBufferRHI;
			Initializer.TotalPrimitiveCount = 0;
			Initializer.GeometryType = RTGT_Triangles;
			Initializer.bFastBuild = true;
			Initializer.bAllowUpdate = false;

			uint32 HighestSegmentPrimitive = 0;
			for (const auto& Section : Sections)
			{
				FRayTracingGeometrySegment Segment;
				Segment.VertexBuffer = PositionStream->VertexBufferRHI;
				Segment.VertexBufferOffset = 0; 
				Segment.MaxVertices = PositionStream->Num();
				Segment.bEnabled = Section->GetDrawMask().IsAnySet(ERealtimeMeshDrawMask::DrawDynamic | ERealtimeMeshDrawMask::DrawStatic);
				if (Segment.bEnabled)
				{
					Segment.FirstPrimitive = Section->GetStreamRange().GetMinIndex() / 3;
					Segment.NumPrimitives = Section->GetStreamRange().NumPrimitives(3);
				}
				else
				{
					Segment.FirstPrimitive = 0;
					Segment.NumPrimitives = 0;
				}
				Initializer.TotalPrimitiveCount += Segment.NumPrimitives;

				HighestSegmentPrimitive = FMath::Max<uint32>(HighestSegmentPrimitive, Segment.FirstPrimitive + Segment.NumPrimitives);
				Initializer.Segments.Add(Segment);
			}

			const bool bIsDataValid = HighestSegmentPrimitive <= Initializer.TotalPrimitiveCount;

			if (!bIsDataValid)
			{
				UE_LOG(RealtimeMeshLog, Warning, TEXT("Unable to create ray tracing accelleration structures. Some triangles in buffer are unaccounted for in sections."));
			}
			else if (Initializer.Segments.Num() > 0)
			{
				RayTracingGeometry.SetInitializer(Initializer);
#if RMC_ENGINE_ABOVE_5_3
				RayTracingGeometry.InitResource(RHICmdList);
#else
				RayTracingGeometry.InitResource();				
#endif
				check(RayTracingGeometry.RayTracingGeometryRHI.IsValid());
			}
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
