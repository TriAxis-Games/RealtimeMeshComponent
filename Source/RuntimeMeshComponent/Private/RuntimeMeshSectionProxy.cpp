// Copyright 2016-2020 TriAxis Games L.L.C. All Rights Reserved.

#include "RuntimeMeshSectionProxy.h"
#include "RuntimeMeshComponentPlugin.h"


#if RHI_RAYTRACING
#include "RayTracingInstance.h"
#endif

DECLARE_DWORD_COUNTER_STAT(TEXT("RuntimeMeshSectionProxy - Num Triangles"), STAT_RuntimeMeshSectionProxy_NumTriangles, STATGROUP_RuntimeMesh);


void FRuntimeMeshSectionProxyBuffers::Reset()
{
	VertexFactory.ReleaseResource();

	PositionBuffer.ReleaseResource();
	TangentsBuffer.ReleaseResource();
	UVsBuffer.ReleaseResource();
	ColorBuffer.ReleaseResource();

	IndexBuffer.ReleaseResource();
	AdjacencyIndexBuffer.ReleaseResource();

#if RHI_RAYTRACING
	if (IsRayTracingEnabled())
	{
		RayTracingGeometry.ReleaseResource();
	}
#endif
}

void FRuntimeMeshSectionProxyBuffers::UpdateRayTracingGeometry()
{
#if RHI_RAYTRACING
	if (IsRayTracingEnabled())
	{
		FRayTracingGeometryInitializer Initializer;
		Initializer.DebugName = FName(TEXT("RMC RT Initializer"));
		
		Initializer.IndexBuffer = nullptr;
		Initializer.TotalPrimitiveCount = 0; 
		
		Initializer.GeometryType = RTGT_Triangles;
		Initializer.bFastBuild = true;
		Initializer.bAllowUpdate = false;

		RayTracingGeometry.SetInitializer(Initializer);
		RayTracingGeometry.InitResource();

		RayTracingGeometry.Initializer.IndexBuffer = IndexBuffer.IndexBufferRHI;
		RayTracingGeometry.Initializer.TotalPrimitiveCount = IndexBuffer.Num() / 3; // should be the total number of triangles

		FRayTracingGeometrySegment Segment;
		Segment.VertexBufferStride = sizeof(FVector3f); //could also be PositionBuffer.Stride()
		Segment.VertexBufferElementType = VET_Float3;
		Segment.VertexBuffer = PositionBuffer.VertexBufferRHI;
		Segment.NumPrimitives = IndexBuffer.Num() / 3; //number of triangles
		Segment.MaxVertices = PositionBuffer.Num();
		RayTracingGeometry.Initializer.Segments.Add(Segment);

		RayTracingGeometry.UpdateRHI();
	}
#endif
}