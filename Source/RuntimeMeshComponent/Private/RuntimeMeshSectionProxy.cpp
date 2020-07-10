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
	AdjacencyIndexBuffer.InitResource();

// #if RHI_RAYTRACING
// 	if (IsRayTracingEnabled())
// 	{
// 		RayTracingGeometry.InitResource();
// 	}
// #endif

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
	AdjacencyIndexBuffer.ReleaseResource();

#if RHI_RAYTRACING
	if (IsRayTracingEnabled())
	{
		RayTracingGeometry.ReleaseResource();
	}
#endif
}
