// Copyright 2016-2020 TriAxis Games L.L.C. All Rights Reserved.

#include "RuntimeMeshSectionProxy.h"
#include "RuntimeMeshComponentPlugin.h"


#if RHI_RAYTRACING
#include "RayTracingInstance.h"
#endif
#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION <= 23
#define BELOW_423
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
#ifdef BELOW_423
		Initializer.PositionVertexBuffer = nullptr;
#endif
		Initializer.IndexBuffer = nullptr;
#ifdef BELOW_423
		Initializer.BaseVertexIndex = 0;
		Initializer.VertexBufferStride = 12;
		Initializer.VertexBufferByteOffset = 0;
#endif
		Initializer.TotalPrimitiveCount = 0;

#ifdef BELOW_423
		Initializer.VertexBufferElementType = VET_Float3;
#endif
		Initializer.GeometryType = RTGT_Triangles;
		Initializer.bFastBuild = true;
		Initializer.bAllowUpdate = false;

		RayTracingGeometry.SetInitializer(Initializer);
		RayTracingGeometry.InitResource();

#ifdef BELOW_423
		RayTracingGeometry.Initializer.PositionVertexBuffer = PositionBuffer.VertexBufferRHI;
#endif
		RayTracingGeometry.Initializer.IndexBuffer = IndexBuffer.IndexBufferRHI;
		RayTracingGeometry.Initializer.TotalPrimitiveCount = IndexBuffer.Num() / 3;

#ifndef BELOW_423
		FRayTracingGeometrySegment Segment;
		Segment.VertexBuffer = PositionBuffer.VertexBufferRHI;
		Segment.NumPrimitives = IndexBuffer.Num() / 3;
		RayTracingGeometry.Initializer.Segments.Add(Segment);
#endif

		RayTracingGeometry.UpdateRHI();
	}
#endif
}


#ifdef BELOW_423
#undef BELOW_423
#endif