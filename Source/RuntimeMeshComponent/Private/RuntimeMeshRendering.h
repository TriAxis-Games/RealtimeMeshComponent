// Copyright 2016-2018 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "Engine.h"
#include "RuntimeMeshCore.h"

class FRuntimeMeshSectionProxy;
using FRuntimeMeshSectionProxyPtr = TSharedPtr<FRuntimeMeshSectionProxy, ESPMode::NotThreadSafe>;
using FRuntimeMeshSectionProxyWeakPtr = TWeakPtr<FRuntimeMeshSectionProxy, ESPMode::NotThreadSafe>;


/** Single vertex buffer to hold one vertex stream within a section */
class FRuntimeMeshVertexBuffer : public FVertexBuffer
{
private:
	/** Size of a single vertex */
	int32 VertexSize;

	/** The number of vertices this buffer is currently allocated to hold */
	int32 NumVertices;

	/** The buffer configuration to use */
	EBufferUsageFlags UsageFlags;

public:

	FRuntimeMeshVertexBuffer();

	~FRuntimeMeshVertexBuffer() {}

	void Reset(int32 InVertexSize, int32 InNumVertices, EUpdateFrequency InUpdateFrequency);

	virtual void InitRHI() override;

	/** Get the size of the vertex buffer */
	int32 Num() { return NumVertices; }

	/** Gets the full allocated size of the buffer (Equal to VertexSize * NumVertices) */
	int32 GetBufferSize() const { return NumVertices * VertexSize; }

	/* Set the size of the vertex buffer */
	void SetNum(int32 NewVertexCount);

	/* Set the data for the vertex buffer */
	void SetData(const TArray<uint8>& Data);

	bool IsEnabled() const
	{
		return VertexSize > 0;
	}
};

/** Index Buffer */
class FRuntimeMeshIndexBuffer : public FIndexBuffer
{
private:
	/* The number of indices this buffer is currently allocated to hold */
	int32 NumIndices;

	/* The size of a single index*/
	int32 IndexSize;

	/* The buffer configuration to use */
	EBufferUsageFlags UsageFlags;

public:

	FRuntimeMeshIndexBuffer();

	~FRuntimeMeshIndexBuffer() {}

	void Reset(int32 InIndexSize, int32 InNumIndices, EUpdateFrequency InUpdateFrequency);

	virtual void InitRHI() override;

	/* Get the size of the index buffer */
	int32 Num() { return NumIndices; }

	/** Gets the full allocated size of the buffer (Equal to IndexSize * NumIndices) */
	int32 GetBufferSize() const { return NumIndices * IndexSize; }

	/* Set the size of the index buffer */
	void SetNum(int32 NewIndexCount);

	/* Set the data for the index buffer */
	void SetData(const TArray<uint8>& Data);
};

/** Vertex Factory */
class FRuntimeMeshVertexFactory : public FLocalVertexFactory
{
public:

	FRuntimeMeshVertexFactory(ERHIFeatureLevel::Type InFeatureLevel, FRuntimeMeshSectionProxy* InSectionParent);

	/** Init function that can be called on any thread, and will do the right thing (enqueue command if called on main thread) */
	void Init(FLocalVertexFactory::FDataType VertexStructure);

	/* Gets the section visibility for static sections */	

#if ENGINE_MAJOR_VERSION >= 4 && ENGINE_MINOR_VERSION >= 19
	virtual uint64 GetStaticBatchElementVisibility(const class FSceneView& View, const struct FMeshBatch* Batch, const void* InViewCustomData = nullptr) const override;
#else
	virtual uint64 GetStaticBatchElementVisibility(const class FSceneView& View, const struct FMeshBatch* Batch) const override;
#endif

private:
	/* Interface to the parent section for checking visibility.*/
	FRuntimeMeshSectionProxy * SectionParent;
};
