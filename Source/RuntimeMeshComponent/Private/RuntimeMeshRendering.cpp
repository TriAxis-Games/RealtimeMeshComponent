// Copyright 2016-2020 TriAxis Games L.L.C. All Rights Reserved.

#include "RuntimeMeshRendering.h"
#include "RuntimeMeshComponentPlugin.h"
#include "RuntimeMeshSectionProxy.h"


FRuntimeMeshVertexBuffer::FRuntimeMeshVertexBuffer(bool bInIsDynamicBuffer, int32 DefaultVertexSize)
	: bIsDynamicBuffer(bInIsDynamicBuffer)
	, VertexSize(DefaultVertexSize)
	, NumVertices(0)
	, ShaderResourceView(nullptr)
{
}


void FRuntimeMeshVertexBuffer::InitRHI()
{
}

void FRuntimeMeshVertexBuffer::ReleaseRHI()
{
	ShaderResourceView.SafeRelease();
	FVertexBuffer::ReleaseRHI();
}



FRuntimeMeshIndexBuffer::FRuntimeMeshIndexBuffer(bool bInIsDynamicBuffer)
	: bIsDynamicBuffer(bInIsDynamicBuffer)
	, IndexSize(CalculateStride(false))
	, NumIndices(0)
{
}

void FRuntimeMeshIndexBuffer::InitRHI()
{
}




FRuntimeMeshVertexFactory::FRuntimeMeshVertexFactory(ERHIFeatureLevel::Type InFeatureLevel)
	: FLocalVertexFactory(InFeatureLevel, "FRuntimeMeshVertexFactory")
{
}

/** Init function that can be called on any thread, and will do the right thing (enqueue command if called on main thread) */
void FRuntimeMeshVertexFactory::Init(FLocalVertexFactory::FDataType VertexStructure)
{
	if (IsInRenderingThread())
	{
		SetData(VertexStructure);
	}
	else
	{
		// Send the command to the render thread
		ENQUEUE_RENDER_COMMAND(InitRuntimeMeshVertexFactory)(
			[this, VertexStructure](FRHICommandListImmediate & RHICmdList)
			{
				Init(VertexStructure);
			}
		);
	}
}