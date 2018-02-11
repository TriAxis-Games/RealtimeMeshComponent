// Copyright 2016-2018 Chris Conway (Koderz). All Rights Reserved.

#include "RuntimeMeshComponentPlugin.h"
#include "RuntimeMeshRendering.h"
#include "RuntimeMeshSectionProxy.h"


FRuntimeMeshVertexBuffer::FRuntimeMeshVertexBuffer()
	: VertexSize(-1), NumVertices(0), UsageFlags(EBufferUsageFlags::BUF_None)
{
}

void FRuntimeMeshVertexBuffer::Reset(int32 InVertexSize, int32 InNumVertices, EUpdateFrequency InUpdateFrequency)
{
	VertexSize = InVertexSize;
	NumVertices = InNumVertices;
	UsageFlags = InUpdateFrequency == EUpdateFrequency::Frequent ? BUF_Dynamic : BUF_Static;
	ReleaseResource();
	InitResource();
}

void FRuntimeMeshVertexBuffer::InitRHI()
{
	if (VertexSize > 0 && NumVertices > 0)
	{
		// Create the vertex buffer
		FRHIResourceCreateInfo CreateInfo;
		VertexBufferRHI = RHICreateVertexBuffer(GetBufferSize(), UsageFlags, CreateInfo);
	}
}

/* Set the size of the vertex buffer */
void FRuntimeMeshVertexBuffer::SetNum(int32 NewVertexCount)
{
	// Make sure we're not already the right size
	if (NewVertexCount != NumVertices)
	{
		NumVertices = NewVertexCount;

		// Rebuild resource
		ReleaseResource();
		InitResource();
	}
}

/* Set the data for the vertex buffer */
void FRuntimeMeshVertexBuffer::SetData(const TArray<uint8>& Data)
{
	check(Data.Num() == GetBufferSize());

	if (GetBufferSize() > 0)
	{
		check(VertexBufferRHI.IsValid());

		// Lock the vertex buffer
		void* Buffer = RHILockVertexBuffer(VertexBufferRHI, 0, Data.Num(), RLM_WriteOnly);

		// Write the vertices to the vertex buffer
		FMemory::Memcpy(Buffer, Data.GetData(), Data.Num());

		// Unlock the vertex buffer
		RHIUnlockVertexBuffer(VertexBufferRHI);
	}
}


FRuntimeMeshIndexBuffer::FRuntimeMeshIndexBuffer()
	: IndexSize(-1), NumIndices(0), UsageFlags(EBufferUsageFlags::BUF_None)
{
}

void FRuntimeMeshIndexBuffer::Reset(int32 InIndexSize, int32 InNumIndices, EUpdateFrequency InUpdateFrequency)
{
	IndexSize = InIndexSize;
	NumIndices = InNumIndices;
	UsageFlags = InUpdateFrequency == EUpdateFrequency::Frequent ? BUF_Dynamic : BUF_Static;
	ReleaseResource();
	InitResource();
}

void FRuntimeMeshIndexBuffer::InitRHI()
{
	if (IndexSize > 0 && NumIndices > 0)
	{
		// Create the index buffer
		FRHIResourceCreateInfo CreateInfo;
		IndexBufferRHI = RHICreateIndexBuffer(IndexSize, GetBufferSize(), BUF_Dynamic, CreateInfo);
	}
}

/* Set the size of the index buffer */
void FRuntimeMeshIndexBuffer::SetNum(int32 NewIndexCount)
{
	// Make sure we're not already the right size
	if (NewIndexCount != NumIndices)
	{
		NumIndices = NewIndexCount;

		// Rebuild resource
		ReleaseResource();
		InitResource();
	}
}

/* Set the data for the index buffer */
void FRuntimeMeshIndexBuffer::SetData(const TArray<uint8>& Data)
{
	check(Data.Num() == GetBufferSize());

	if (GetBufferSize() > 0)
	{
		check(IndexBufferRHI.IsValid());

		// Lock the index buffer
		void* Buffer = RHILockIndexBuffer(IndexBufferRHI, 0, Data.Num(), RLM_WriteOnly);

		// Write the indices to the vertex buffer	
		FMemory::Memcpy(Buffer, Data.GetData(), Data.Num());

		// Unlock the index buffer
		RHIUnlockIndexBuffer(IndexBufferRHI);
	}
}




FRuntimeMeshVertexFactory::FRuntimeMeshVertexFactory(FRuntimeMeshSectionProxy* InSectionParent)
	: SectionParent(InSectionParent)
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
		ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
			InitRuntimeMeshVertexFactory,
			FRuntimeMeshVertexFactory*, VertexFactory, this,
			FLocalVertexFactory::FDataType, VertexStructure, VertexStructure,
			{
				VertexFactory->Init(VertexStructure);
			});
	}
}

/* Gets the section visibility for static sections */
uint64 FRuntimeMeshVertexFactory::GetStaticBatchElementVisibility(const class FSceneView& View, const struct FMeshBatch* Batch) const
{
	return SectionParent->ShouldRender();
}