// Copyright 2016-2019 Chris Conway (Koderz). All Rights Reserved.

#include "RealtimeMeshSectionProxy.h"
#include "RealtimeMeshComponentPlugin.h"


void FRealtimeMeshSectionProxy::Reset()
{
	PositionBuffer.ReleaseResource();
	TangentsBuffer.ReleaseResource();
	UVsBuffer.ReleaseResource();
	ColorBuffer.ReleaseResource();
	IndexBuffer.ReleaseResource();
	AdjacencyIndexBuffer.ReleaseResource();
	VertexFactory.ReleaseResource();
}

bool FRealtimeMeshSectionProxy::CanRender() const
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

void FRealtimeMeshSectionProxy::BuildVertexDataType(FLocalVertexFactory::FDataType& DataType)
{
	PositionBuffer.Bind(DataType);
	TangentsBuffer.Bind(DataType);
	UVsBuffer.Bind(DataType);
	ColorBuffer.Bind(DataType);
}

void FRealtimeMeshSectionProxy::CreateMeshBatch(FMeshBatch& MeshBatch, bool bCastsShadow, bool bWantsAdjacencyInfo) const
{
	MeshBatch.VertexFactory = &VertexFactory;
	MeshBatch.Type = bWantsAdjacencyInfo ? PT_12_ControlPointPatchList : PT_TriangleList;
	
	MeshBatch.CastShadow = bCastsShadow;

	// Make sure that if the material wants adjacency information, that you supply it
	check(!bWantsAdjacencyInfo || AdjacencyIndexBuffer.Num() > 0);

	const FRealtimeMeshIndexBuffer* CurrentIndexBuffer = bWantsAdjacencyInfo ? &AdjacencyIndexBuffer : &IndexBuffer;

	int32 NumIndicesPerTriangle = bWantsAdjacencyInfo ? 12 : 3;
	int32 NumPrimitives = CurrentIndexBuffer->Num() / NumIndicesPerTriangle;

	FMeshBatchElement& BatchElement = MeshBatch.Elements[0];
	BatchElement.IndexBuffer = CurrentIndexBuffer;
	BatchElement.FirstIndex = 0;
	BatchElement.NumPrimitives = NumPrimitives;
	BatchElement.MinVertexIndex = 0;
	BatchElement.MaxVertexIndex = PositionBuffer.Num() - 1;
}

void FRealtimeMeshSectionProxy::UpdateProperties_RenderThread(const FRealtimeMeshSectionProperties& SectionProperties)
{
	Properties.MaterialSlot = SectionProperties.MaterialSlot;

	Properties.bCastsShadow = SectionProperties.bCastsShadow;
	Properties.bIsVisible = SectionProperties.bIsVisible;
}

void FRealtimeMeshSectionProxy::UpdateSection_RenderThread(const FRealtimeMeshRenderableMeshData& MeshData)
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
	}
}

void FRealtimeMeshSectionProxy::ClearSection_RenderThread()
{
	Reset();
}


FRealtimeMeshLODProxy::FRealtimeMeshLODProxy(ERHIFeatureLevel::Type InFeatureLevel)
	: FeatureLevel(InFeatureLevel)
{
}

FRealtimeMeshLODProxy::~FRealtimeMeshLODProxy()
{

}

bool FRealtimeMeshLODProxy::CanRender() const
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

bool FRealtimeMeshLODProxy::HasAnyStaticPath() const
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

bool FRealtimeMeshLODProxy::HasAnyDynamicPath() const
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


bool FRealtimeMeshLODProxy::HasAnyShadowCasters() const
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

void FRealtimeMeshLODProxy::UpdateProperties_RenderThread(const FRealtimeMeshLODProperties& InProperties)
{
	check(IsInRenderingThread());
	Properties = InProperties;
}

void FRealtimeMeshLODProxy::Clear_RenderThread()
{
	check(IsInRenderingThread());
	Sections.Empty();
}

void FRealtimeMeshLODProxy::CreateSection_RenderThread(int32 SectionId, const FRealtimeMeshSectionProperties& SectionProperties)
{
	check(IsInRenderingThread());
	Sections.Remove(SectionId);
	Sections.Add(SectionId, MakeShareable(new FRealtimeMeshSectionProxy(FeatureLevel, SectionProperties), FRealtimeMeshRenderThreadDeleter<FRealtimeMeshSectionProxy>()));
}

void FRealtimeMeshLODProxy::RemoveSection_RenderThread(int32 SectionId)
{
	check(IsInRenderingThread());
	Sections.Remove(SectionId);
}


void FRealtimeMeshLODProxy::UpdateSectionProperties_RenderThread(int32 SectionId, const FRealtimeMeshSectionProperties& SectionProperties)
{
	check(IsInRenderingThread());
	FRealtimeMeshSectionProxyPtr* Section = Sections.Find(SectionId);
	if (Section)
	{
		(*Section)->UpdateProperties_RenderThread(SectionProperties);
	}
}

void FRealtimeMeshLODProxy::UpdateSectionMesh_RenderThread(int32 SectionId, const FRealtimeMeshRenderableMeshData& MeshData)
{
	check(IsInRenderingThread());
	FRealtimeMeshSectionProxyPtr* Section = Sections.Find(SectionId);
	if (Section)
	{
		(*Section)->UpdateSection_RenderThread(MeshData);
	}
}

void FRealtimeMeshLODProxy::ClearSectionMesh_RenderThread(int32 SectionId)
{
	check(IsInRenderingThread());
	FRealtimeMeshSectionProxyPtr* Section = Sections.Find(SectionId);
	if (Section)
	{
		(*Section)->ClearSection_RenderThread();
	}
}