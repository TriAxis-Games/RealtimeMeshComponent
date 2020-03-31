// Copyright 2016-2020 Chris Conway (Koderz). All Rights Reserved.

#include "RuntimeMeshRendering.h"
#include "RuntimeMeshComponentPlugin.h"
#include "RuntimeMeshSectionProxy.h"


FRuntimeMeshVertexBuffer::FRuntimeMeshVertexBuffer(ERuntimeMeshUpdateFrequency InUpdateFrequency, int32 InVertexSize)
	: UsageFlags(InUpdateFrequency == ERuntimeMeshUpdateFrequency::Frequent? BUF_Dynamic : BUF_Static)
	, VertexSize(InVertexSize)
	, NumVertices(0)
	, ShaderResourceView(nullptr)
{
}

void FRuntimeMeshVertexBuffer::Reset(int32 InNumVertices)
{
	NumVertices = InNumVertices;
	ReleaseResource();
	InitResource();
}

void FRuntimeMeshVertexBuffer::InitRHI()
{
	if (VertexSize > 0 && NumVertices > 0)
	{
		// Create the vertex buffer
		FRHIResourceCreateInfo CreateInfo;
		VertexBufferRHI = RHICreateVertexBuffer(GetBufferSize(), UsageFlags | BUF_ShaderResource, CreateInfo);
		
		if (RHISupportsManualVertexFetch(GMaxRHIShaderPlatform))
		{
			CreateSRV();
		}
	}
}

/* Set the size of the vertex buffer */
void FRuntimeMeshVertexBuffer::SetData(int32 NewVertexCount, const uint8* InData)
{
	// Don't reallocate the buffer if it's already the right size
	if (NewVertexCount != NumVertices)
	{
		NumVertices = NewVertexCount;

		// Rebuild resource
		ReleaseResource();
	}

	// Now copy the new data
	if (NewVertexCount > 0)
	{
		InitResource();
		check(VertexBufferRHI.IsValid());

		// Lock the vertex buffer
		void* Buffer = RHILockVertexBuffer(VertexBufferRHI, 0, GetBufferSize(), RLM_WriteOnly);

		// Write the vertices to the vertex buffer
		FMemory::Memcpy(Buffer, InData, GetBufferSize());

		// Unlock the vertex buffer
		RHIUnlockVertexBuffer(VertexBufferRHI);
	}
}


FRuntimeMeshIndexBuffer::FRuntimeMeshIndexBuffer(ERuntimeMeshUpdateFrequency InUpdateFrequency, bool bUse32BitIndices)
	: UsageFlags(InUpdateFrequency == ERuntimeMeshUpdateFrequency::Frequent ? BUF_Dynamic : BUF_Static), IndexSize(bUse32BitIndices? sizeof(uint32) : sizeof(uint16)), NumIndices(0)
{
	check(IndexSize != 0);
}

void FRuntimeMeshIndexBuffer::Reset(int32 InNumIndices)
{
	NumIndices = InNumIndices;
	ReleaseResource();
	InitResource();
}

void FRuntimeMeshIndexBuffer::InitRHI()
{
	if (IndexSize > 0 && NumIndices > 0)
	{
		// Create the index buffer
		FRHIResourceCreateInfo CreateInfo;
		IndexBufferRHI = RHICreateIndexBuffer(IndexSize, GetBufferSize(), UsageFlags | BUF_ShaderResource, CreateInfo);
	}
}

/* Set the data for the index buffer */
void FRuntimeMeshIndexBuffer::SetData(int32 bInUse32BitIndices, int32 NewIndexCount, const uint8* InData)
{
	int32 NewIndexSize = bInUse32BitIndices ? sizeof(uint32) : sizeof(uint16);

	// Make sure we're not already the right size
	if (NewIndexCount != NumIndices || NewIndexSize != IndexSize)
	{
		NumIndices = NewIndexCount;
		IndexSize = NewIndexSize;

		// Rebuild resource
		ReleaseResource();
	}

	if (NewIndexCount > 0)
	{
		InitResource();
		check(IndexBufferRHI.IsValid());

		// Lock the index buffer
		void* Buffer = RHILockIndexBuffer(IndexBufferRHI, 0, GetBufferSize(), RLM_WriteOnly);

		// Write the indices to the vertex buffer	
		FMemory::Memcpy(Buffer, InData, GetBufferSize());

		// Unlock the index buffer
		RHIUnlockIndexBuffer(IndexBufferRHI);
	}
}




FRuntimeMeshVertexFactory::FRuntimeMeshVertexFactory(ERHIFeatureLevel::Type InFeatureLevel, FRuntimeMeshSectionProxy* InSectionParent)
	: FLocalVertexFactory(InFeatureLevel, "FRuntimeMeshVertexFactory")
	, SectionParent(InSectionParent)
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

/* Gets the section visibility for static sections */
uint64 FRuntimeMeshVertexFactory::GetStaticBatchElementVisibility(const class FSceneView& View, const struct FMeshBatch* Batch, const void* InViewCustomData) const
{
	return SectionParent->ShouldRender();
}