// Copyright 2016-2020 Chris Conway (Koderz). All Rights Reserved.

#include "RuntimeMeshSectionProxy.h"
#include "RuntimeMeshComponentPlugin.h"


#if RHI_RAYTRACING
#include "RayTracingInstance.h"
#endif


DECLARE_DWORD_COUNTER_STAT(TEXT("RuntimeMeshSectionProxy - Num Triangles"), STAT_RuntimeMeshSectionProxy_NumTriangles, STATGROUP_RuntimeMesh);



void FRuntimeMeshSectionProxyBuffers::InitResource()
{
	PositionBuffer.InitResource();
	TangentsBuffer.InitResource();
	UVsBuffer.InitResource();
	ColorBuffer.InitResource();

	IndexBuffer.InitResource();
	ReversedIndexBuffer.InitResource();
	DepthOnlyIndexBuffer.InitResource();
	ReversedDepthOnlyIndexBuffer.InitResource();
	AdjacencyIndexBuffer.InitResource();

#if RHI_RAYTRACING
	if (IsRayTracingEnabled())
	{
		RayTracingGeometry.InitResource();
	}
#endif

	//VertexFactory.InitResource();
}

void FRuntimeMeshSectionProxyBuffers::Reset()
{
	VertexFactory.ReleaseResource();

	PositionBuffer.ReleaseResource();
	TangentsBuffer.ReleaseResource();
	UVsBuffer.ReleaseResource();
	ColorBuffer.ReleaseResource();

	IndexBuffer.ReleaseResource();
	ReversedIndexBuffer.ReleaseResource();
	DepthOnlyIndexBuffer.ReleaseResource();
	ReversedDepthOnlyIndexBuffer.ReleaseResource();
	AdjacencyIndexBuffer.ReleaseResource();

#if RHI_RAYTRACING
	if (IsRayTracingEnabled())
	{
		RayTracingGeometry.ReleaseResource();
	}
#endif
}



//void FRuntimeMeshSectionProxy::Reset()
//{
//	if (Buffers.IsValid())
//	{
//		Buffers->Reset();
//	}
//}
//
//bool FRuntimeMeshSectionProxy::CanRender() const
//{
//	if (PositionBuffer.Num() <= 0)
//	{
//		return false;
//	}
//
//	if (TangentsBuffer.Num() < PositionBuffer.Num())
//	{
//		return false;
//	}
//
//	if (UVsBuffer.Num() < PositionBuffer.Num())
//	{
//		return false;
//	}
//
//	if (IndexBuffer.Num() <= 0)
//	{
//		return false;
//	}
//
//	return true;
//}
//
//void FRuntimeMeshSectionProxy::BuildVertexDataType(FLocalVertexFactory::FDataType& DataType)
//{
//	PositionBuffer.Bind(DataType);
//	TangentsBuffer.Bind(DataType);
//	UVsBuffer.Bind(DataType);
//	ColorBuffer.Bind(DataType);
//}
//
//void FRuntimeMeshSectionProxy::CreateMeshBatch(FMeshBatch& MeshBatch, bool bCastsShadow, bool bWantsAdjacencyInfo) const
//{
//	MeshBatch.VertexFactory = &VertexFactory;
//	MeshBatch.Type = bWantsAdjacencyInfo ? PT_12_ControlPointPatchList : PT_TriangleList;
//	
//	MeshBatch.CastShadow = bCastsShadow;
//
//	// Make sure that if the material wants adjacency information, that you supply it
//	check(!bWantsAdjacencyInfo || AdjacencyIndexBuffer.Num() > 0);
//
//	const FRuntimeMeshIndexBuffer* CurrentIndexBuffer = bWantsAdjacencyInfo ? &AdjacencyIndexBuffer : &IndexBuffer;
//
//	int32 NumIndicesPerTriangle = bWantsAdjacencyInfo ? 12 : 3;
//	int32 NumPrimitives = CurrentIndexBuffer->Num() / NumIndicesPerTriangle;
//
//	FMeshBatchElement& BatchElement = MeshBatch.Elements[0];
//	BatchElement.IndexBuffer = CurrentIndexBuffer;
//	BatchElement.FirstIndex = 0;
//	BatchElement.NumPrimitives = NumPrimitives;
//	BatchElement.MinVertexIndex = 0;
//	BatchElement.MaxVertexIndex = PositionBuffer.Num() - 1;
//
//
//	INC_DWORD_STAT_BY(STAT_RuntimeMeshSectionProxy_NumTriangles, NumPrimitives);
//}
//
//void FRuntimeMeshSectionProxy::UpdateProperties_RenderThread(const FRuntimeMeshSectionProperties& SectionProperties)
//{
//	Properties.MaterialSlot = SectionProperties.MaterialSlot;
//
//	Properties.bCastsShadow = SectionProperties.bCastsShadow;
//	Properties.bIsVisible = SectionProperties.bIsVisible;
//}
//
//void FRuntimeMeshSectionProxy::UpdateSection_RenderThread(const FRuntimeMeshRenderableMeshData& MeshData)
//{
//	check(IsInRenderingThread());
//
//	// Drop the factory before changing buffers so not to cause issues
//	VertexFactory.ReleaseResource();
//
//	// Update all the buffers and recreate the factory if we have valid mesh data to render
//	PositionBuffer.SetData(MeshData.Positions.Num(), MeshData.Positions.GetData());
//	TangentsBuffer.SetData(MeshData.Tangents.IsHighPrecision(), MeshData.Tangents.Num(), MeshData.Tangents.GetData());
//	UVsBuffer.SetData(MeshData.TexCoords.IsHighPrecision(), MeshData.TexCoords.NumChannels(), MeshData.TexCoords.Num(), MeshData.TexCoords.GetData());
//	ColorBuffer.SetData(MeshData.Colors.Num(), MeshData.Colors.GetData());
//	IndexBuffer.SetData(MeshData.Triangles.IsHighPrecision(), MeshData.Triangles.Num(), MeshData.Triangles.GetData());
//	AdjacencyIndexBuffer.SetData(MeshData.AdjacencyTriangles.IsHighPrecision(), MeshData.AdjacencyTriangles.Num(), MeshData.AdjacencyTriangles.GetData());
//	   
//	if (CanRender())
//	{
//		FLocalVertexFactory::FDataType DataType;
//		BuildVertexDataType(DataType);
//
//		VertexFactory.Init(DataType);
//		VertexFactory.InitResource();
//
//
//#if RHI_RAYTRACING
//		if (IsRayTracingEnabled())
//		{
//			ENQUEUE_RENDER_COMMAND(InitRuntimeMeshSectionRayTracingGeometry)(
//				[this](FRHICommandListImmediate& RHICmdList)
//				{
//					FRayTracingGeometryInitializer Initializer;
//#if ENGINE_MAJOR_VERSION <= 4 && ENGINE_MINOR_VERSION <= 23
//					Initializer.VertexBufferStride = 12;
//					Initializer.VertexBufferByteOffset = 0;
//					Initializer.VertexBufferElementType = VET_Float3;
//#endif
//
//#if ENGINE_MAJOR_VERSION <= 4 && ENGINE_MINOR_VERSION <= 22
//					Initializer.PrimitiveType = PT_TriangleList;
//#else
//					Initializer.GeometryType = RTGT_Triangles;
//#endif
//
//					Initializer.bFastBuild = true;
//					Initializer.bAllowUpdate = false;
//
//					Initializer.IndexBuffer = IndexBuffer.IndexBufferRHI;
//#if ENGINE_MAJOR_VERSION <= 4 && ENGINE_MINOR_VERSION <= 23
//					Initializer.PositionVertexBuffer = PositionBuffer.VertexBufferRHI;
//					Initializer.BaseVertexIndex = 0;
//#endif
//
//					TArray<FRayTracingGeometrySegment> GeometrySections;
//					FRayTracingGeometrySegment Segment;
//					Segment.FirstPrimitive = 0;
//					Segment.NumPrimitives = IndexBuffer.Num() / 3;
//#if ENGINE_MAJOR_VERSION >= 4 && ENGINE_MINOR_VERSION >= 24
//					Segment.VertexBuffer = PositionBuffer.VertexBufferRHI;
//#endif
//					GeometrySections.Add(Segment);
//					Initializer.TotalPrimitiveCount = Segment.NumPrimitives;
//					Initializer.Segments = GeometrySections;
//
//
//					RayTracingGeometry.SetInitializer(Initializer);
//					RayTracingGeometry.InitResource();
//
//
//					//#dxr_todo: add support for segments?
//
//					RayTracingGeometry.UpdateRHI();
//				});
//		}
//#endif
//
//
//	}
//}
//
//void FRuntimeMeshSectionProxy::ClearSection_RenderThread()
//{
//	Reset();
//}
//
//
//FRuntimeMeshLODProxy::FRuntimeMeshLODProxy(ERHIFeatureLevel::Type InFeatureLevel, const FRuntimeMeshLODProperties& InProperties)
//	: Properties(InProperties)
//	, FeatureLevel(InFeatureLevel)
//{
//}
//
//FRuntimeMeshLODProxy::~FRuntimeMeshLODProxy()
//{
//
//}
//
//bool FRuntimeMeshLODProxy::CanRender() const
//{
//	for (const auto& Section : Sections)
//	{
//		if (Section.Value->CanRender())
//		{
//			return true;
//		}
//	}
//	return false;
//}
//
//bool FRuntimeMeshLODProxy::HasAnyStaticPath() const
//{
//	for (const auto& Section : Sections)
//	{
//		if (Section.Value->CanRender() && Section.Value->WantsToRenderInStaticPath())
//		{
//			return true;
//		}
//	}
//	return false;
//}
//
//bool FRuntimeMeshLODProxy::HasAnyDynamicPath() const
//{
//	for (const auto& Section : Sections)
//	{
//		if (Section.Value->CanRender() && !Section.Value->WantsToRenderInStaticPath())
//		{
//			return true;
//		}
//	}
//	return false;
//}
//
//
//bool FRuntimeMeshLODProxy::HasAnyShadowCasters() const
//{
//	for (const auto& Section : Sections)
//	{
//		if (Section.Value->CanRender() && !Section.Value->CastsShadow())
//		{
//			return true;
//		}
//	}
//	return false;
//}
//
//void FRuntimeMeshLODProxy::Configure_RenderThread(const FRuntimeMeshLODProperties& InProperties)
//{
//	check(IsInRenderingThread());
//	Properties = InProperties;
//}
//
//void FRuntimeMeshLODProxy::Reset_RenderThread()
//{
//	Sections.Empty();
//}
//
//
//
//void FRuntimeMeshLODProxy::CreateOrUpdateSection_RenderThread(int32 SectionId, const FRuntimeMeshSectionProperties& SectionProperties, bool bShouldReset)
//{
//	check(IsInRenderingThread());
//
//	FRuntimeMeshSectionProxyPtr* Section = nullptr;
//	if (bShouldReset || (Section = Sections.Find(SectionId)) == nullptr)
//	{
//		Sections.Add(SectionId, MakeShareable(new FRuntimeMeshSectionProxy(FeatureLevel, SectionProperties), FRuntimeMeshRenderThreadDeleter<FRuntimeMeshSectionProxy>()));
//	}
//	else
//	{
//		(*Section)->UpdateProperties_RenderThread(SectionProperties);
//	}
//}
//
//void FRuntimeMeshLODProxy::UpdateSectionMesh_RenderThread(int32 SectionId, const FRuntimeMeshRenderableMeshData& MeshData)
//{
//	check(IsInRenderingThread());
//	FRuntimeMeshSectionProxyPtr* Section = Sections.Find(SectionId);
//	if (Section)
//	{
//		(*Section)->UpdateSection_RenderThread(MeshData);
//	}
//}
//
//void FRuntimeMeshLODProxy::ClearSection_RenderThread(int32 SectionId)
//{
//	check(IsInRenderingThread());
//	FRuntimeMeshSectionProxyPtr* Section = Sections.Find(SectionId);
//	if (Section)
//	{
//		(*Section)->ClearSection_RenderThread();
//	}
//}
//
//void FRuntimeMeshLODProxy::ClearAllSections_RenderThread()
//{
//	check(IsInRenderingThread());
//
//	for (const auto& Section : Sections)
//	{
//		Section.Value->ClearSection_RenderThread();
//	}
//}
//
//void FRuntimeMeshLODProxy::RemoveAllSections_RenderThread()
//{
//	check(IsInRenderingThread());
//	Sections.Empty();
//}
//
//void FRuntimeMeshLODProxy::RemoveSection_RenderThread(int32 SectionId)
//{
//	check(IsInRenderingThread());
//	Sections.Remove(SectionId);
//}
//
