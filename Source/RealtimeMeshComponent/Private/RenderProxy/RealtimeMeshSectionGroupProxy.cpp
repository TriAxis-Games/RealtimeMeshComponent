// Copyright (c) 2015-2025 TriAxis Games, L.L.C. All Rights Reserved.

#include "RenderProxy/RealtimeMeshSectionGroupProxy.h"

//#include "MaterialDomain.h"
#include "RealtimeMeshComponentModule.h"
#include "Data/RealtimeMeshShared.h"
#include "RenderProxy/RealtimeMeshLODProxy.h"
#if RMC_ENGINE_ABOVE_5_2
#include "MaterialDomain.h"
#endif
#include "Algo/AnyOf.h"
#include "RenderProxy/RealtimeMeshSectionProxy.h"
#include "RenderProxy/RealtimeMeshVertexFactory.h"
#include "Materials/Material.h"

namespace RealtimeMesh
{
	FRealtimeMeshSectionGroupProxy::FRealtimeMeshSectionGroupProxy(const FRealtimeMeshSharedResourcesRef& InSharedResources, const FRealtimeMeshSectionGroupKey& InKey)
		: SharedResources(InSharedResources)
		, Key(InKey)
		, VertexFactory(SharedResources->CreateVertexFactory())
		, bVertexFactoryDirty(false)
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
			bVertexFactoryDirty = true;
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
			bVertexFactoryDirty = true;
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

		bVertexFactoryDirty = true;
	}

	void FRealtimeMeshSectionGroupProxy::RemoveStream(const FRealtimeMeshStreamKey& StreamKey)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(FRealtimeMeshSectionGroupProxy::RemoveStream);

		if (const auto* Stream = Streams.Find(StreamKey))
		{
			(*Stream)->ReleaseUnderlyingResource();
			Streams.Remove(StreamKey);
			bVertexFactoryDirty = true;
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

		bool bNeedsFactoryInitialization = bVertexFactoryDirty || !VertexFactory->IsInitialized() ||
			Algo::AnyOf(Sections, [](const FRealtimeMeshSectionProxyRef& Section) { return Section->IsRangeDirty(); });
		
		if (!VertexFactory)
		{
			VertexFactory = SharedResources->CreateVertexFactory();
		}

		if (bNeedsFactoryInitialization)
		{
			VertexFactory->Initialize(RHICmdList, Streams);
		}
		
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

		if (bNeedsFactoryInitialization)
		{
			DrawMask.SetFlag(UpdateRayTracingInfo(RHICmdList)? ERealtimeMeshDrawMask::RayTracing : ERealtimeMeshDrawMask::None);
		}
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

	bool FRealtimeMeshSectionGroupProxy::UpdateRayTracingInfo(FRHICommandListBase& RHICmdList)
	{
#if RHI_RAYTRACING
		TRACE_CPUPROFILER_EVENT_SCOPE(FRealtimeMeshSectionGroupProxy::UpdateRayTracingInfo);
		
		RayTracingGeometry.ReleaseResource();

		bool bShouldGenerateRayTracingGeometry = DrawMask.HasAnyFlags() && VertexFactory.IsValid() && IsRayTracingEnabled();

		// We need to check if the sections are contiguous with no gaps and using the entire index buffer...
		// If it is not then we weed to allocate a ray tracing index buffer and pack the active sections down into it.
		// This is because the ray tracing geometry can't have gaps in the index buffer.

		TRangeSet<int32> SectionIndexRanges;
		for (const auto& Section : Sections)
		{
			if (Section->GetDrawMask().ShouldRender() && Section->GetDrawMask().ShouldRenderMainPass())
			{
				SectionIndexRanges.Add(TRange<int32>(Section->GetStreamRange().GetMinIndex(), Section->GetStreamRange().GetMaxIndex() + 1));
			}
		}

		const int32 MinIndex = SectionIndexRanges.HasMinBound() ? (SectionIndexRanges.GetMinBoundValue() + (SectionIndexRanges.GetMinBound().IsInclusive()? 0 : 1)) : 0;
		const int32 MaxIndex = SectionIndexRanges.HasMaxBound() ? (SectionIndexRanges.GetMaxBoundValue() - (SectionIndexRanges.GetMaxBound().IsInclusive()? 0 : 1)) : 0;

		const bool bAreSectionsContiguous = !SectionIndexRanges.IsEmpty() && SectionIndexRanges.Contains(TRange<int32>(MinIndex, MaxIndex + 1));

		if (!bAreSectionsContiguous)
		{
			// TODO: Implement ray tracing index buffer creation
			UE_LOG(LogRealtimeMesh, Warning, TEXT("Unable to create ray tracing accelleration structures. Some triangles in buffer are unaccounted for in sections."));
			bShouldGenerateRayTracingGeometry = false;
		}		

		if (bShouldGenerateRayTracingGeometry)
		{
			check(VertexFactory->IsInitialized());

			const auto PositionStream = StaticCastSharedPtr<FRealtimeMeshVertexBuffer>(Streams.FindChecked(FRealtimeMeshStreams::Position));
			const auto IndexStream = StaticCastSharedPtr<FRealtimeMeshIndexBuffer>(Streams.FindChecked(FRealtimeMeshStreams::Triangles));

			FRayTracingGeometryInitializer Initializer;
			Initializer.DebugName = *(SharedResources->GetMeshName().ToString() + TEXT("_") + Key.ToString() + " RTGeometry");
			//Initializer.OwnerName = SharedResources->GetMeshName();
			
			Initializer.IndexBuffer = IndexStream->IndexBufferRHI;
			Initializer.IndexBufferOffset = IndexStream->IndexBufferRHI->GetStride() * MinIndex;
			Initializer.TotalPrimitiveCount = ((MaxIndex - MinIndex) + 1) / 3;
			Initializer.GeometryType = RTGT_Triangles;
			Initializer.bFastBuild = true;
			Initializer.bAllowUpdate = false;

			for (const auto& Section : Sections)
			{
				if (Section->GetDrawMask().ShouldRender() && Section->GetDrawMask().ShouldRenderMainPass())
				{
					FRayTracingGeometrySegment Segment;
					Segment.VertexBuffer = PositionStream->VertexBufferRHI;
					Segment.VertexBufferOffset = 0; 
					Segment.MaxVertices = PositionStream->Num();
					Segment.FirstPrimitive = Section->GetStreamRange().GetMinIndex() / 3;
					Segment.NumPrimitives = Section->GetStreamRange().NumPrimitives(3);
					check(Segment.NumPrimitives > 0);
					Initializer.Segments.Add(Segment);
				}
			}

			RayTracingGeometry.SetInitializer(Initializer);
#if RMC_ENGINE_ABOVE_5_3
			RayTracingGeometry.InitResource(RHICmdList);
#else
			RayTracingGeometry.InitResource();				
#endif
				
#if RMC_ENGINE_ABOVE_5_5				
			check(RayTracingGeometry.GetRHI()->IsValid());
#else
			check(RayTracingGeometry.RayTracingGeometryRHI.IsValid());
#endif

			return true;
		}
		
		return false;
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
