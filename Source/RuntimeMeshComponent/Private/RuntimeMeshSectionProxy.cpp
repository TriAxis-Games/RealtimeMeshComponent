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

void FRuntimeMeshSectionProxyBuffers::UpdateRayTracingGeometry()
{
#if RHI_RAYTRACING
	if (IsRayTracingEnabled())
	{
		ENQUEUE_RENDER_COMMAND(InitRuntimeMeshRayTracingGeometry)(
			[ThisShared = this->AsShared()](FRHICommandListImmediate& RHICmdList)
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

			ThisShared->RayTracingGeometry.SetInitializer(Initializer);
			ThisShared->RayTracingGeometry.InitResource();

			ThisShared->RayTracingGeometry.Initializer.PositionVertexBuffer = ThisShared->PositionBuffer.VertexBufferRHI;
			ThisShared->RayTracingGeometry.Initializer.IndexBuffer = ThisShared->IndexBuffer.IndexBufferRHI;
			ThisShared->RayTracingGeometry.Initializer.TotalPrimitiveCount = ThisShared->IndexBuffer.Num() / 3;


			ThisShared->RayTracingGeometry.UpdateRHI();
		});
	}
#endif
}
