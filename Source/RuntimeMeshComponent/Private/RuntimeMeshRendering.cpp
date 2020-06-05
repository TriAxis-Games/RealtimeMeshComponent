// Copyright 2016-2020 Chris Conway (Koderz). All Rights Reserved.

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
	// Here we'll create a single element buffer, this is only so that we have an existing buffer
	// so that we can update the reference directly
	FRHIResourceCreateInfo CreateInfo;
	VertexBufferRHI = CreateRHIBuffer<true>(CreateInfo, VertexSize, bIsDynamicBuffer);

	if (VertexBufferRHI && RHISupportsManualVertexFetch(GMaxRHIShaderPlatform))
	{
		ShaderResourceView = RHICreateShaderResourceView(VertexBufferRHI, GetElementDatumSize(), GetElementFormat());
	}
}

void FRuntimeMeshVertexBuffer::ReleaseRHI()
{
	ShaderResourceView.SafeRelease();
	FVertexBuffer::ReleaseRHI();
}

/* Set the size of the vertex buffer */
// void FRuntimeMeshVertexBuffer::SetData(int32 NewStride, int32 NewVertexCount, const uint8* InData)
// {
// 	// Don't reallocate the buffer if it's already the right size
// 	if (NewVertexCount != NumVertices || NewStride != VertexSize)
// 	{
// 		VertexSize = NewStride;
// 		NumVertices = NewVertexCount;
// 
// 		FRHIResourceCreateInfo CreateInfo;
// 		VertexBufferRHI = CreateRHIBuffer<true>(CreateInfo, NumVertices * VertexSize, bIsDynamicBuffer);
// 
// 		if (VertexBufferRHI && RHISupportsManualVertexFetch(GMaxRHIShaderPlatform))
// 		{
// 			ShaderResourceView = RHICreateShaderResourceView(VertexBufferRHI, VertexSize, GetElementFormat());
// 		}
// 
// 		// Rebuild resource
// 		//ReleaseResource();
// 	}
// 
// 	// Now copy the new data
// 	if (VertexSize > 0 && NewVertexCount > 0)
// 	{
// 		//InitResource();
// 		check(VertexBufferRHI.IsValid());
// 
// 		// Lock the vertex buffer
// 		void* Buffer = RHILockVertexBuffer(VertexBufferRHI, 0, GetBufferSize(), RLM_WriteOnly);
// 
// 		// Write the vertices to the vertex buffer
// 		FMemory::Memcpy(Buffer, InData, GetBufferSize());
// 
// 		// Unlock the vertex buffer
// 		RHIUnlockVertexBuffer(VertexBufferRHI);
// 	}
// }




FRuntimeMeshIndexBuffer::FRuntimeMeshIndexBuffer(bool bInIsDynamicBuffer)
	: bIsDynamicBuffer(bInIsDynamicBuffer)
	, IndexSize(CalculateStride(false))
	, NumIndices(0)
{
}

void FRuntimeMeshIndexBuffer::InitRHI()
{
	FRHIResourceCreateInfo CreateInfo;
	IndexBufferRHI = CreateRHIBuffer<true>(CreateInfo, IndexSize, sizeof(uint16), bIsDynamicBuffer);


// 
// 
// 	if (IndexSize > 0 && NumIndices > 0)
// 	{
// 		// Create the index buffer
// 		FRHIResourceCreateInfo CreateInfo;
// 		IndexBufferRHI = RHICreateIndexBuffer(IndexSize, GetBufferSize(), Flags, CreateInfo);
// 	}
}

// /* Set the data for the index buffer */
// void FRuntimeMeshIndexBuffer::SetData(bool bInUse32BitIndices, int32 NewIndexCount, const uint8* InData)
// {
// 	int32 NewIndexSize = CalculateStride(bInUse32BitIndices);
// 
// 	// Make sure we're not already the right size
// 	if (NewIndexCount != NumIndices || NewIndexSize != IndexSize)
// 	{
// 		NumIndices = NewIndexCount;
// 		IndexSize = NewIndexSize;
// 
// 		FRHIResourceCreateInfo CreateInfo;
// 		IndexBufferRHI = CreateRHIBuffer<true>(CreateInfo, IndexSize, NumIndices * IndexSize, bIsDynamicBuffer);
// 
// // 		// Rebuild resource
// // 		ReleaseResource();
// 	}
// 
// 	if (NewIndexCount > 0)
// 	{
// 		//InitResource();
// 		check(IndexBufferRHI.IsValid());
// 
// 		// Lock the index buffer
// 		void* Buffer = RHILockIndexBuffer(IndexBufferRHI, 0, GetBufferSize(), RLM_WriteOnly);
// 
// 		// Write the indices to the vertex buffer	
// 		FMemory::Memcpy(Buffer, InData, GetBufferSize());
// 
// 		// Unlock the index buffer
// 		RHIUnlockIndexBuffer(IndexBufferRHI);
// 	}
// }




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
		// HORU: 4.22 rendering
		ENQUEUE_RENDER_COMMAND(InitRuntimeMeshVertexFactory)(
			[this, VertexStructure](FRHICommandListImmediate & RHICmdList)
			{
				Init(VertexStructure);
			}
		);
	}
}

///* Gets the section visibility for static sections */
//uint64 FRuntimeMeshVertexFactory::GetStaticBatchElementVisibility(const class FSceneView& View, const struct FMeshBatch* Batch, const void* InViewCustomData) const
//{
//	return SectionParent->ShouldRender();
//}