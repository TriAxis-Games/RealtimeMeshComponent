// Copyright 2016-2019 Chris Conway (Koderz). All Rights Reserved.

#include "RuntimeMeshSectionProxy.h"
#include "RuntimeMeshComponentPlugin.h"



DECLARE_DWORD_COUNTER_STAT(TEXT("RuntimeMeshSectionProxy - Num Triangles"), STAT_RuntimeMeshSectionProxy_NumTriangles, STATGROUP_RuntimeMesh);


void FRuntimeMeshSectionProxy::Reset()
{
	PositionBuffer.ReleaseResource();
	TangentsBuffer.ReleaseResource();
	UVsBuffer.ReleaseResource();
	ColorBuffer.ReleaseResource();
	IndexBuffer.ReleaseResource();
	AdjacencyIndexBuffer.ReleaseResource();
	VertexFactory.ReleaseResource();

#if RHI_RAYTRACING
	if (IsRayTracingEnabled())
	{
		RayTracingGeometry.ReleaseResource();
	}
#endif
}

bool FRuntimeMeshSectionProxy::CanRender() const
{
	if (PositionBuffer.Num() <= 0)
	{
		return false;
	}

	if (TangentsBuffer.Num() < PositionBuffer.Num())
	{
		return false;
	}

	if (UVsBuffer.Num() < PositionBuffer.Num())
	{
		return false;
	}

	if (IndexBuffer.Num() <= 0)
	{
		return false;
	}

	return true;
}

void FRuntimeMeshSectionProxy::BuildVertexDataType(FLocalVertexFactory::FDataType& DataType)
{
	PositionBuffer.Bind(DataType);
	TangentsBuffer.Bind(DataType);
	UVsBuffer.Bind(DataType);
	ColorBuffer.Bind(DataType);
}

void FRuntimeMeshSectionProxy::CreateMeshBatch(FMeshBatch& MeshBatch, bool bCastsShadow, bool bWantsAdjacencyInfo) const
{
	MeshBatch.VertexFactory = &VertexFactory;
	MeshBatch.Type = bWantsAdjacencyInfo ? PT_12_ControlPointPatchList : PT_TriangleList;
	
	MeshBatch.CastShadow = bCastsShadow;

	// Make sure that if the material wants adjacency information, that you supply it
	check(!bWantsAdjacencyInfo || AdjacencyIndexBuffer.Num() > 0);

	const FRuntimeMeshIndexBuffer* CurrentIndexBuffer = bWantsAdjacencyInfo ? &AdjacencyIndexBuffer : &IndexBuffer;

	int32 NumIndicesPerTriangle = bWantsAdjacencyInfo ? 12 : 3;
	int32 NumPrimitives = CurrentIndexBuffer->Num() / NumIndicesPerTriangle;

	FMeshBatchElement& BatchElement = MeshBatch.Elements[0];
	BatchElement.IndexBuffer = CurrentIndexBuffer;
	BatchElement.FirstIndex = 0;
	BatchElement.NumPrimitives = NumPrimitives;
	BatchElement.MinVertexIndex = 0;
	BatchElement.MaxVertexIndex = PositionBuffer.Num() - 1;


	INC_DWORD_STAT_BY(STAT_RuntimeMeshSectionProxy_NumTriangles, NumPrimitives);
}

void FRuntimeMeshSectionProxy::UpdateProperties_RenderThread(const FRuntimeMeshSectionProperties& SectionProperties)
{
	Properties.MaterialSlot = SectionProperties.MaterialSlot;

	Properties.bCastsShadow = SectionProperties.bCastsShadow;
	Properties.bIsVisible = SectionProperties.bIsVisible;
}

void FRuntimeMeshSectionProxy::UpdateSection_RenderThread(const FRuntimeMeshRenderableMeshData& MeshData)
{
	check(IsInRenderingThread());

	// Drop the factory before changing buffers so not to cause issues
	VertexFactory.ReleaseResource();

	// Update all the buffers and recreate the factory if we have valid mesh data to render
	PositionBuffer.SetData(MeshData.Positions.Num(), MeshData.Positions.GetData());
	TangentsBuffer.SetData(MeshData.Tangents.Num(), MeshData.Tangents.GetData());
	UVsBuffer.SetData(MeshData.TexCoords.Num(), MeshData.TexCoords.GetData());
	ColorBuffer.SetData(MeshData.Colors.Num(), MeshData.Colors.GetData());
	IndexBuffer.SetData(MeshData.Triangles.Num(), MeshData.Triangles.GetData());
	AdjacencyIndexBuffer.SetData(MeshData.AdjacencyTriangles.Num(), MeshData.AdjacencyTriangles.GetData());
	   
	if (CanRender())
	{
		FLocalVertexFactory::FDataType DataType;
		BuildVertexDataType(DataType);

		VertexFactory.Init(DataType);
		VertexFactory.InitResource();


#if RHI_RAYTRACING
		if (IsRayTracingEnabled())
		{
			ENQUEUE_RENDER_COMMAND(InitRuntimeMeshSectionRayTracingGeometry)(
				[this](FRHICommandListImmediate& RHICmdList)
				{
					FRayTracingGeometryInitializer Initializer;
					Initializer.PositionVertexBuffer = nullptr;
					Initializer.IndexBuffer = nullptr;
					Initializer.BaseVertexIndex = 0;
					Initializer.VertexBufferStride = 12;
					Initializer.VertexBufferByteOffset = 0;
					Initializer.TotalPrimitiveCount = 0;
					Initializer.VertexBufferElementType = VET_Float3;
					Initializer.GeometryType = RTGT_Triangles;
					Initializer.bFastBuild = true;
					Initializer.bAllowUpdate = false;

					RayTracingGeometry.SetInitializer(Initializer);
					RayTracingGeometry.InitResource();

					RayTracingGeometry.Initializer.PositionVertexBuffer = PositionBuffer.VertexBufferRHI;
					RayTracingGeometry.Initializer.IndexBuffer = IndexBuffer.IndexBufferRHI;
					RayTracingGeometry.Initializer.TotalPrimitiveCount = IndexBuffer.Num() / 3;

					//#dxr_todo: add support for segments?

					RayTracingGeometry.UpdateRHI();
				});
		}
#endif


	}
}

void FRuntimeMeshSectionProxy::ClearSection_RenderThread()
{
	Reset();
}


FRuntimeMeshLODProxy::FRuntimeMeshLODProxy(ERHIFeatureLevel::Type InFeatureLevel)
	: FeatureLevel(InFeatureLevel)
{
}

FRuntimeMeshLODProxy::~FRuntimeMeshLODProxy()
{

}

bool FRuntimeMeshLODProxy::CanRender() const
{
	for (const auto& Section : Sections)
	{
		if (Section.Value->CanRender())
		{
			return true;
		}
	}
	return false;
}

bool FRuntimeMeshLODProxy::HasAnyStaticPath() const
{
	for (const auto& Section : Sections)
	{
		if (Section.Value->CanRender() && Section.Value->WantsToRenderInStaticPath())
		{
			return true;
		}
	}
	return false;
}

bool FRuntimeMeshLODProxy::HasAnyDynamicPath() const
{
	for (const auto& Section : Sections)
	{
		if (Section.Value->CanRender() && !Section.Value->WantsToRenderInStaticPath())
		{
			return true;
		}
	}
	return false;
}


bool FRuntimeMeshLODProxy::HasAnyShadowCasters() const
{
	for (const auto& Section : Sections)
	{
		if (Section.Value->CanRender() && !Section.Value->CastsShadow())
		{
			return true;
		}
	}
	return false;
}

void FRuntimeMeshLODProxy::UpdateProperties_RenderThread(const FRuntimeMeshLODProperties& InProperties)
{
	check(IsInRenderingThread());
	Properties = InProperties;
}

void FRuntimeMeshLODProxy::Clear_RenderThread()
{
	check(IsInRenderingThread());
	Sections.Empty();
}

void FRuntimeMeshLODProxy::CreateSection_RenderThread(int32 SectionId, const FRuntimeMeshSectionProperties& SectionProperties)
{
	check(IsInRenderingThread());
	Sections.Remove(SectionId);
	Sections.Add(SectionId, MakeShareable(new FRuntimeMeshSectionProxy(FeatureLevel, SectionProperties), FRuntimeMeshRenderThreadDeleter<FRuntimeMeshSectionProxy>()));
}

void FRuntimeMeshLODProxy::RemoveSection_RenderThread(int32 SectionId)
{
	check(IsInRenderingThread());
	Sections.Remove(SectionId);
}


void FRuntimeMeshLODProxy::UpdateSectionProperties_RenderThread(int32 SectionId, const FRuntimeMeshSectionProperties& SectionProperties)
{
	check(IsInRenderingThread());
	FRuntimeMeshSectionProxyPtr* Section = Sections.Find(SectionId);
	if (Section)
	{
		(*Section)->UpdateProperties_RenderThread(SectionProperties);
	}
}

void FRuntimeMeshLODProxy::UpdateSectionMesh_RenderThread(int32 SectionId, const FRuntimeMeshRenderableMeshData& MeshData)
{
	check(IsInRenderingThread());
	FRuntimeMeshSectionProxyPtr* Section = Sections.Find(SectionId);
	if (Section)
	{
		(*Section)->UpdateSection_RenderThread(MeshData);
	}
}

void FRuntimeMeshLODProxy::ClearSectionMesh_RenderThread(int32 SectionId)
{
	check(IsInRenderingThread());

	if (SectionId == INDEX_NONE)
	{
		for (const auto& Section : Sections)
		{
			Section.Value->ClearSection_RenderThread();
		}
	}
	else
	{
		FRuntimeMeshSectionProxyPtr* Section = Sections.Find(SectionId);
		if (Section)
		{
			(*Section)->ClearSection_RenderThread();
		}
	}	
}