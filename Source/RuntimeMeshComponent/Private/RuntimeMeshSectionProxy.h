// Copyright 2016-2020 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "Engine/Engine.h"
#include "Components/MeshComponent.h"
#include "RuntimeMeshRendering.h"
#include "RuntimeMeshRenderable.h"

struct FRuntimeMeshSectionNullBufferElement
{
	FPackedNormal Normal;
	FPackedNormal Tangent;
	FColor Color;
	FVector2DHalf UV0;

	FRuntimeMeshSectionNullBufferElement()
		: Normal(FVector(0.0f, 0.0f, 1.0f))
		, Tangent(FVector(1.0f, 0.0f, 0.0f))
		, Color(FColor::Transparent)
		, UV0(FVector2D::ZeroVector)
	{ }
};


struct FRuntimeMeshSectionProxyBuffers
{
	/** Vertex factory for this section */
	FRuntimeMeshVertexFactory VertexFactory;

	/** Vertex buffer containing the positions for this section */
	FRuntimeMeshPositionVertexBuffer PositionBuffer;

	/** Vertex buffer containing the tangents for this section */
	FRuntimeMeshTangentsVertexBuffer TangentsBuffer;

	/** Vertex buffer containing the UVs for this section */
	FRuntimeMeshTexCoordsVertexBuffer UVsBuffer;

	/** Vertex buffer containing the colors for this section */
	FRuntimeMeshColorVertexBuffer ColorBuffer;


	/** Index buffer for this section */
	FRuntimeMeshIndexBuffer IndexBuffer;

	/** Reversed index buffer, used to prevent changing culling state between drawcalls. */
	FRuntimeMeshIndexBuffer ReversedIndexBuffer;

	/** Index buffer resource for rendering in depth only passes. */
	FRuntimeMeshIndexBuffer DepthOnlyIndexBuffer;

	/** Reversed depth only index buffer, used to prevent changing culling state between drawcalls. */
	FRuntimeMeshIndexBuffer ReversedDepthOnlyIndexBuffer;

	/** Index buffer for this section */
	FRuntimeMeshIndexBuffer AdjacencyIndexBuffer;


#if RHI_RAYTRACING
	FRayTracingGeometry RayTracingGeometry;
#endif

	uint32 bIsShared : 1;


// 
// 	FRuntimeMeshSectionBuffers()
// 	{
// 
// 	}

	FRuntimeMeshSectionProxyBuffers(bool bInIsDynamicBuffer, bool bInIsShared)
		: VertexFactory(GMaxRHIFeatureLevel)
		, PositionBuffer(bInIsDynamicBuffer)
		, TangentsBuffer(bInIsDynamicBuffer)
		, UVsBuffer(bInIsDynamicBuffer)
		, ColorBuffer(bInIsDynamicBuffer)
		, IndexBuffer(bInIsDynamicBuffer)
		, ReversedIndexBuffer(bInIsDynamicBuffer)
		, DepthOnlyIndexBuffer(bInIsDynamicBuffer)
		, ReversedDepthOnlyIndexBuffer(bInIsDynamicBuffer)
		, AdjacencyIndexBuffer(bInIsDynamicBuffer)
		, bIsShared(bInIsShared)
	{

	}

	void InitResource();

	~FRuntimeMeshSectionProxyBuffers()
	{
		check(IsInRenderingThread());

		Reset();
	}

	void Reset();


	template <uint32 MaxNumUpdates>
	void ApplyRHIReferences(FRuntimeMeshSectionUpdateData& UpdateData, TRHIResourceUpdateBatcher<MaxNumUpdates>& Batcher)
	{
		PositionBuffer.InitRHIFromExisting(UpdateData.PositionsBuffer, UpdateData.Positions.GetNumElements(), Batcher);
		TangentsBuffer.InitRHIFromExisting(UpdateData.TangentsBuffer, UpdateData.Tangents.GetNumElements(), UpdateData.bHighPrecisionTangents, Batcher);
		UVsBuffer.InitRHIFromExisting(UpdateData.TexCoordsBuffer, UpdateData.TexCoords.GetNumElements(), UpdateData.bHighPrecisionTexCoords, UpdateData.NumTexCoordChannels, Batcher);
		ColorBuffer.InitRHIFromExisting(UpdateData.ColorsBuffer, UpdateData.Colors.GetNumElements(), Batcher);

		IndexBuffer.InitRHIFromExisting(UpdateData.TrianglesBuffer, UpdateData.Triangles.GetNumElements(), UpdateData.b32BitTriangles, Batcher);
		AdjacencyIndexBuffer.InitRHIFromExisting(UpdateData.AdjacencyTrianglesBuffer, UpdateData.AdjacencyTriangles.GetNumElements(), UpdateData.b32BitAdjacencyTriangles, Batcher);

	}
};



struct FRuntimeMeshSectionProxy
{
	TSharedPtr<FRuntimeMeshSectionProxyBuffers> Buffers;

	uint32 FirstIndex;
	uint32 NumTriangles;
	uint32 MinVertexIndex;
	uint32 MaxVertexIndex;

	ERuntimeMeshUpdateFrequency UpdateFrequency;
	int32 MaterialSlot;

	uint32 bIsValid : 1;
	uint32 bIsVisible : 1;
	uint32 bIsMainPassRenderable : 1;
	uint32 bCastsShadow : 1;
	// Should this section be considered opaque in ray tracing
	uint32 bForceOpaque : 1;

	uint32 bHasAdjacencyInfo : 1;
	uint32 bHasDepthOnlyIndices : 1;
	uint32 bHasReversedIndices : 1;
	uint32 bHasReversedDepthOnlyIndices : 1;
	uint32 bHasRayTracingGeometry : 1;


	FRuntimeMeshSectionProxy()
		: FirstIndex(0)
		, NumTriangles(0)
		, MinVertexIndex(0)
		, MaxVertexIndex(0)
		, UpdateFrequency(ERuntimeMeshUpdateFrequency::Infrequent)
		, MaterialSlot(0)
		, bIsValid(false)
		, bIsVisible(false)
		, bIsMainPassRenderable(false)
		, bCastsShadow(false)
		, bForceOpaque(false)
		, bHasAdjacencyInfo(false)
		, bHasDepthOnlyIndices(false)
		, bHasReversedIndices(false)
		, bHasReversedDepthOnlyIndices(false)
		, bHasRayTracingGeometry(false)
	{

	}

	FORCEINLINE bool CanRender() const 
	{
		check(!bIsValid || (Buffers.IsValid() && NumTriangles > 0 && MaxVertexIndex > 0));
		return bIsValid;
	}

	FORCEINLINE bool ShouldRenderMainPass() const { return bIsMainPassRenderable; }
	FORCEINLINE bool IsStaticSection() const { return UpdateFrequency == ERuntimeMeshUpdateFrequency::Infrequent; }

	FORCEINLINE bool ShouldRender() const { return CanRender() && bIsVisible; }
	FORCEINLINE bool ShouldRenderStaticPath() const { return ShouldRender() && ShouldRenderMainPass() && IsStaticSection(); }
	FORCEINLINE bool ShouldRenderDynamicPath() const { return ShouldRender() && ShouldRenderMainPass() && !IsStaticSection(); }
	FORCEINLINE bool ShouldRenderShadow() const { return ShouldRender() && bCastsShadow; }


	void UpdateState()
	{
		bIsValid = Buffers.IsValid();

		if (bIsValid)
		{
			bIsValid &= Buffers->PositionBuffer.Num() >= 3;
			bIsValid &= Buffers->TangentsBuffer.Num() >= Buffers->PositionBuffer.Num();
			bIsValid &= Buffers->UVsBuffer.Num() >= Buffers->PositionBuffer.Num();
			bIsValid &= Buffers->ColorBuffer.Num() >= Buffers->PositionBuffer.Num();
			bIsValid &= Buffers->IndexBuffer.Num() >= 3;


			bHasAdjacencyInfo = Buffers->AdjacencyIndexBuffer.Num() > 3;
			bHasDepthOnlyIndices = Buffers->DepthOnlyIndexBuffer.Num() == Buffers->IndexBuffer.Num();
			bHasReversedIndices = Buffers->ReversedIndexBuffer.Num() == Buffers->IndexBuffer.Num();
			bHasReversedDepthOnlyIndices = Buffers->ReversedDepthOnlyIndexBuffer.Num() == Buffers->IndexBuffer.Num();
		}
	}

	void CreateMeshBatch(FMeshBatch& MeshBatch, bool bShouldCastShadow, bool bWantsAdjacencyInfo) const
	{
		MeshBatch.VertexFactory = &Buffers->VertexFactory;
		MeshBatch.Type = bWantsAdjacencyInfo ? PT_12_ControlPointPatchList : PT_TriangleList;
		
		MeshBatch.CastShadow = bShouldCastShadow;
	
		// Make sure that if the material wants adjacency information, that you supply it
		check(!bWantsAdjacencyInfo || bHasAdjacencyInfo);
	
		const FRuntimeMeshIndexBuffer* CurrentIndexBuffer = bWantsAdjacencyInfo ? &Buffers->AdjacencyIndexBuffer : &Buffers->IndexBuffer;
	
		int32 NumIndicesPerTriangle = bWantsAdjacencyInfo ? 12 : 3;
		int32 NumPrimitives = CurrentIndexBuffer->Num() / NumIndicesPerTriangle;
	
		FMeshBatchElement& BatchElement = MeshBatch.Elements[0];
		BatchElement.IndexBuffer = CurrentIndexBuffer;
		BatchElement.FirstIndex = 0;
		BatchElement.NumPrimitives = NumPrimitives;
		BatchElement.MinVertexIndex = 0;
		BatchElement.MaxVertexIndex = Buffers->PositionBuffer.Num() - 1;
	
	
		//INC_DWORD_STAT_BY(STAT_RuntimeMeshSectionProxy_NumTriangles, NumPrimitives);
	}
};




struct FRuntimeMeshLODData
{
	/** Sections for this LOD, these do not have to be configured the same as sections in other LODs */
	TMap<int32, FRuntimeMeshSectionProxy> Sections;

	float ScreenSize;

	uint32 bShouldRender : 1;
	uint32 bShouldRenderStatic : 1;
	uint32 bShouldRenderDynamic : 1;
	uint32 bShouldRenderShadow : 1;

	uint32 bShouldMergeStaticSections : 1;


	FRuntimeMeshLODData()
		: ScreenSize(0.0f)
		, bShouldRender(false)
		, bShouldRenderStatic(false)
		, bShouldRenderDynamic(false)
		, bShouldRenderShadow(false)
		, bShouldMergeStaticSections(false)
	{

	}


	void UpdateState()
	{
		bShouldRender = false;
		bShouldRenderStatic = false;
		bShouldRenderDynamic = false;
		bShouldRenderShadow = false;
		
		for (auto& SectionEntry : Sections)
		{
			FRuntimeMeshSectionProxy& Section = SectionEntry.Value;

			Section.UpdateState();

			bShouldRender |= Section.CanRender();
			bShouldRenderStatic |= Section.ShouldRenderStaticPath();
			bShouldRenderDynamic |= Section.ShouldRenderDynamicPath();		
			bShouldRenderShadow |= Section.ShouldRenderShadow();
		}
	}


};


