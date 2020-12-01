// Copyright 2016-2020 TriAxis Games L.L.C. All Rights Reserved.

#pragma once

#include "Engine/Engine.h"
#include "RuntimeMeshCore.h"
#include "RuntimeMeshRenderable.h"
#include "Containers/ResourceArray.h"

class FRuntimeMeshVertexBuffer;
class FRuntimeMeshIndexBuffer;


class FRuntimeMeshBufferUpdateData : public FResourceArrayInterface
{
	int32 ElementStride;
	int32 NumElements;
	TArray<uint8> Data;

public:


	FRuntimeMeshBufferUpdateData(FRuntimeMeshVertexPositionStream&& InPositions)
		: ElementStride(InPositions.GetStride())
		, NumElements(InPositions.Num())
		, Data(MoveTemp(InPositions).TakeData())
	{

	}

	FRuntimeMeshBufferUpdateData(FRuntimeMeshVertexTangentStream&& InTangents)
		: ElementStride(InTangents.GetStride())
		, NumElements(InTangents.Num())
		, Data(MoveTemp(InTangents).TakeData())
	{

	}

	FRuntimeMeshBufferUpdateData(FRuntimeMeshVertexTexCoordStream&& InTexCoords)
		: ElementStride(InTexCoords.GetStride())
		, NumElements(InTexCoords.Num())
		, Data(MoveTemp(InTexCoords).TakeData())
	{

	}

	FRuntimeMeshBufferUpdateData(FRuntimeMeshVertexColorStream&& InColors)
		: ElementStride(InColors.GetStride())
		, NumElements(InColors.Num())
		, Data(MoveTemp(InColors).TakeData())
	{

	}

	FRuntimeMeshBufferUpdateData(FRuntimeMeshTriangleStream&& InTriangles)
		: ElementStride(InTriangles.GetStride())
		, NumElements(InTriangles.Num())
		, Data(MoveTemp(InTriangles).TakeData())
	{

	}

	virtual ~FRuntimeMeshBufferUpdateData() {}


	int32 GetStride() const { return ElementStride; }
	int32 GetNumElements() const { return NumElements; }

	const void* GetResourceData() const override
	{
		return reinterpret_cast<const void*>(Data.GetData());
	}
	uint32 GetResourceDataSize() const override
	{
		return Data.Num();
	}
	void Discard() override
	{
		Data.Empty();
	}
	bool IsStatic() const override
	{
		return false;
	}
	bool GetAllowCPUAccess() const override
	{
		return true;
	}
	void SetAllowCPUAccess(bool bInNeedsCPUAccess) override
	{
	}

};

class FRuntimeMeshSectionUpdateData
{
public:

	FRuntimeMeshBufferUpdateData Positions;
	FRuntimeMeshBufferUpdateData Tangents;
	FRuntimeMeshBufferUpdateData TexCoords;
	FRuntimeMeshBufferUpdateData Colors;

	FRuntimeMeshBufferUpdateData Triangles;
	FRuntimeMeshBufferUpdateData AdjacencyTriangles;




	FVertexBufferRHIRef PositionsBuffer;
	FVertexBufferRHIRef TangentsBuffer;
	FVertexBufferRHIRef TexCoordsBuffer;
	FVertexBufferRHIRef ColorsBuffer;

	FIndexBufferRHIRef TrianglesBuffer;
	FIndexBufferRHIRef AdjacencyTrianglesBuffer;


	const bool bHighPrecisionTangents : 1;
	const bool bHighPrecisionTexCoords : 1;
	const uint8 NumTexCoordChannels;
	const bool b32BitTriangles;
	const bool b32BitAdjacencyTriangles;


	bool bBuffersCreated;


	FRuntimeMeshSectionUpdateData(FRuntimeMeshRenderableMeshData&& InMesh)
		: Positions(MoveTemp(InMesh.Positions))
		, Tangents(MoveTemp(InMesh.Tangents))
		, TexCoords(MoveTemp(InMesh.TexCoords))
		, Colors(MoveTemp(InMesh.Colors))
		, Triangles(MoveTemp(InMesh.Triangles))
		, AdjacencyTriangles(MoveTemp(InMesh.AdjacencyTriangles))
		, bHighPrecisionTangents(InMesh.Tangents.IsHighPrecision())
		, bHighPrecisionTexCoords(InMesh.TexCoords.IsHighPrecision())
		, NumTexCoordChannels(InMesh.TexCoords.NumChannels())
		, b32BitTriangles(InMesh.Triangles.IsHighPrecision())
		, b32BitAdjacencyTriangles(InMesh.AdjacencyTriangles.IsHighPrecision())
		, bBuffersCreated(false)
	{

	}

	template<bool bIsInRenderThread>
	void CreateRHIBuffers(bool bShouldUseDynamicBuffers);

};



/** Single vertex buffer to hold one vertex stream within a section */
class FRuntimeMeshVertexBuffer : public FVertexBuffer
{
protected:

	/** Should this buffer by flagged as dynamic */
	const bool bIsDynamicBuffer;

	/** Size of a single vertex */
	int32 VertexSize;

	/** The number of vertices this buffer is currently allocated to hold */
	int32 NumVertices;

	/** Shader Resource View for this buffer */
	FShaderResourceViewRHIRef ShaderResourceView;

public:

	FRuntimeMeshVertexBuffer(bool bInIsDynamicBuffer, int32 DefaultVertexSize);

	~FRuntimeMeshVertexBuffer() {}

	virtual void InitRHI() override;
	void ReleaseRHI() override;

	/** Gets the size of the vertex */
	FORCEINLINE int32 Stride() const { return VertexSize; }

	/** Get the size of the vertex buffer */
	FORCEINLINE int32 Num() const { return NumVertices; }

	/** Gets the full allocated size of the buffer (Equal to VertexSize * NumVertices) */
	FORCEINLINE int32 GetBufferSize() const { return NumVertices * VertexSize; }

	/** Gets the size of a single piece of data within an element */
	virtual int32 GetElementDatumSize() const = 0;

	/** Gets the format of the element, needed for creation of SRV's */
	virtual EPixelFormat GetElementFormat() const = 0;

	/** Binds the vertex buffer to the factory data type */
	virtual void Bind(FLocalVertexFactory::FDataType& DataType) = 0;




public:

	template<bool bIsInRenderThread>
	static FVertexBufferRHIRef CreateRHIBuffer(FRHIResourceCreateInfo& CreateInfo, uint32 SizeInBytes, bool bDynamicBuffer)
	{
		const int32 Flags = (bDynamicBuffer ? BUF_Dynamic : BUF_Static) | BUF_ShaderResource;
		CreateInfo.bWithoutNativeResource = SizeInBytes <= 0;
		if (bIsInRenderThread)
		{
			return RHICreateVertexBuffer(SizeInBytes, Flags, CreateInfo);
		}
		else
		{
			return RHIAsyncCreateVertexBuffer(SizeInBytes, Flags, CreateInfo);
		}
		return nullptr;
	}

	template<bool bIsInRenderThread>
	static FVertexBufferRHIRef CreateRHIBuffer(FRuntimeMeshBufferUpdateData& InStream, bool bDynamicBuffer)
	{
		const uint32 SizeInBytes = InStream.GetResourceDataSize();

		FRHIResourceCreateInfo CreateInfo(&InStream);

		return CreateRHIBuffer<bIsInRenderThread>(CreateInfo, SizeInBytes, bDynamicBuffer);
	}


	template <uint32 MaxNumUpdates>
	void UpdateRHIFromExisting(FRHIVertexBuffer* IntermediateBuffer, TRHIResourceUpdateBatcher<MaxNumUpdates>& Batcher)
	{
		check(VertexBufferRHI && IntermediateBuffer);

		Batcher.QueueUpdateRequest(VertexBufferRHI, IntermediateBuffer);

		if (ShaderResourceView)
		{
			Batcher.QueueUpdateRequest(ShaderResourceView, VertexBufferRHI, GetElementDatumSize(), GetElementFormat());
		}
	}

	void InitRHIFromExisting(const FVertexBufferRHIRef& InVertexBufferRHI)
	{
		InitResource();
		check(InVertexBufferRHI);

		VertexBufferRHI = InVertexBufferRHI;
		if (VertexBufferRHI.IsValid() && RHISupportsManualVertexFetch(GMaxRHIShaderPlatform))
		{
		 	ShaderResourceView = RHICreateShaderResourceView(VertexBufferRHI, GetElementDatumSize(), GetElementFormat());
		}
	}

};


class FRuntimeMeshPositionVertexBuffer : public FRuntimeMeshVertexBuffer
{
public:
	FRuntimeMeshPositionVertexBuffer(bool bInIsDynamicBuffer)
		: FRuntimeMeshVertexBuffer(bInIsDynamicBuffer, sizeof(FVector))
	{

	}

	virtual FString GetFriendlyName() const override { return TEXT("FRuntimeMeshPositionVertexBuffer"); }

	virtual int32 GetElementDatumSize() const override { return 4; }
	virtual EPixelFormat GetElementFormat() const override { return PF_R32_FLOAT; }

	virtual void Bind(FLocalVertexFactory::FDataType& DataType) override 
	{
		DataType.PositionComponent = FVertexStreamComponent(this, 0, 12, VET_Float3);
		DataType.PositionComponentSRV = ShaderResourceView;
	}

	template <uint32 MaxNumUpdates>
	void UpdateRHIFromExisting(FRHIVertexBuffer* IntermediateBuffer, int32 NumElements, TRHIResourceUpdateBatcher<MaxNumUpdates>& Batcher)
	{
		if (VertexBufferRHI && IntermediateBuffer)
		{
			VertexSize = sizeof(FVector);
			NumVertices = NumElements;

			FRuntimeMeshVertexBuffer::UpdateRHIFromExisting<MaxNumUpdates>(IntermediateBuffer, Batcher);
		}
	}

	void InitRHIFromExisting(const FVertexBufferRHIRef& InVertexBufferRHI, int32 NumElements)
	{
		VertexSize = sizeof(FVector);
		NumVertices = NumElements;

		FRuntimeMeshVertexBuffer::InitRHIFromExisting(InVertexBufferRHI);
	}
};

class FRuntimeMeshTangentsVertexBuffer : public FRuntimeMeshVertexBuffer
{
	static constexpr int32 CalculateStride(bool bShouldUseHighPrecision)
	{
		return (bShouldUseHighPrecision ? sizeof(FPackedRGBA16N) : sizeof(FPackedNormal)) * 2;
	}

private:

	/** Whether this tangent buffer is using high precision tangents */
	bool bUseHighPrecision;

public:
	FRuntimeMeshTangentsVertexBuffer(bool bInIsDynamicBuffer)
		: FRuntimeMeshVertexBuffer(bInIsDynamicBuffer, CalculateStride(false))
		, bUseHighPrecision(false)
	{

	}

	virtual FString GetFriendlyName() const override { return TEXT("FRuntimeMeshTangentsVertexBuffer"); }

	virtual int32 GetElementDatumSize() const override { return bUseHighPrecision? sizeof(FPackedRGBA16N) : sizeof(FPackedNormal); }
	virtual EPixelFormat GetElementFormat() const override { return bUseHighPrecision ? PF_R16G16B16A16_SNORM : PF_R8G8B8A8_SNORM; }

	virtual void Bind(FLocalVertexFactory::FDataType& DataType) override
	{
		uint32 TangentSizeInBytes = 0;
		uint32 TangentXOffset = 0;
		uint32 TangentZOffset = 0;
		EVertexElementType TangentElementType = VET_None;

		if (bUseHighPrecision)
		{
			TangentElementType = VET_UShort4N;
			TangentSizeInBytes = sizeof(FPackedRGBA16N) * 2;
			TangentXOffset = 0;
			TangentZOffset = sizeof(FPackedRGBA16N);
		}
		else
		{
			TangentElementType = VET_PackedNormal;
			TangentSizeInBytes = sizeof(FPackedNormal) * 2;
			TangentXOffset = 0;
			TangentZOffset = sizeof(FPackedNormal);
		}

		DataType.TangentBasisComponents[0] = FVertexStreamComponent(this, TangentXOffset, TangentSizeInBytes, TangentElementType, EVertexStreamUsage::ManualFetch);
		DataType.TangentBasisComponents[1] = FVertexStreamComponent(this, TangentZOffset, TangentSizeInBytes, TangentElementType, EVertexStreamUsage::ManualFetch);
		DataType.TangentsSRV = ShaderResourceView;
	}

	template <uint32 MaxNumUpdates>
	void UpdateRHIFromExisting(FRHIVertexBuffer* IntermediateBuffer, int32 NumElements, bool bShouldUseHighPrecision, TRHIResourceUpdateBatcher<MaxNumUpdates>& Batcher)
	{
		if (VertexBufferRHI && IntermediateBuffer)
		{
			bUseHighPrecision = bShouldUseHighPrecision;
			VertexSize = CalculateStride(bShouldUseHighPrecision);
			NumVertices = NumElements;

			FRuntimeMeshVertexBuffer::UpdateRHIFromExisting<MaxNumUpdates>(IntermediateBuffer, Batcher);
		}
	}

	void InitRHIFromExisting(const FVertexBufferRHIRef& InVertexBufferRHI, int32 NumElements, bool bShouldUseHighPrecision)
	{
		bUseHighPrecision = bShouldUseHighPrecision;
		VertexSize = CalculateStride(bShouldUseHighPrecision);
		NumVertices = NumElements;

		FRuntimeMeshVertexBuffer::InitRHIFromExisting(InVertexBufferRHI);
	}

};

class FRuntimeMeshTexCoordsVertexBuffer : public FRuntimeMeshVertexBuffer
{
	static constexpr int32 CalculateStride(bool bShouldUseHighPrecision, int32 InNumUVs)
	{
		return (bShouldUseHighPrecision ? sizeof(FVector2D) : sizeof(FVector2DHalf)) * InNumUVs;
	}

private:
	/** Whether this uv buffer is using high precision uvs */
	bool bUseHighPrecision;

	/** Num UV's in use */
	int32 NumUVs;

public:
	FRuntimeMeshTexCoordsVertexBuffer(bool bInIsDynamicBuffer)
		: FRuntimeMeshVertexBuffer(bInIsDynamicBuffer, CalculateStride(false, 1))
		, bUseHighPrecision(false)
		, NumUVs(1)
	{

	}

	virtual FString GetFriendlyName() const override { return TEXT("FRuntimeMeshUVsVertexBuffer"); }

	virtual int32 GetElementDatumSize() const override { return bUseHighPrecision ? sizeof(FVector2D) : sizeof(FVector2DHalf); }
	virtual EPixelFormat GetElementFormat() const override { return bUseHighPrecision ? PF_G32R32F : PF_G16R16F; }

	virtual void Bind(FLocalVertexFactory::FDataType& DataType) override
	{ 
		DataType.TextureCoordinates.Empty();
		DataType.NumTexCoords = NumUVs;


		EVertexElementType UVDoubleWideVertexElementType = VET_None;
		EVertexElementType UVVertexElementType = VET_None;
		uint32 UVSizeInBytes = 0;
		if (bUseHighPrecision)
		{
			UVSizeInBytes = sizeof(FVector2D);
			UVDoubleWideVertexElementType = VET_Float4;
			UVVertexElementType = VET_Float2;
		}
		else
		{
			UVSizeInBytes = sizeof(FVector2DHalf);
			UVDoubleWideVertexElementType = VET_Half4;
			UVVertexElementType = VET_Half2;
		}

		uint32 UVStride = UVSizeInBytes * NumUVs;

		int32 UVIndex;
		for (UVIndex = 0; UVIndex < NumUVs - 1; UVIndex += 2)
		{
			DataType.TextureCoordinates.Add(FVertexStreamComponent(this, UVSizeInBytes * UVIndex, UVStride, UVDoubleWideVertexElementType, EVertexStreamUsage::ManualFetch));
		}

		// possible last UV channel if we have an odd number
		if (UVIndex < NumUVs)
		{
			DataType.TextureCoordinates.Add(FVertexStreamComponent(this, UVSizeInBytes * UVIndex, UVStride, UVVertexElementType, EVertexStreamUsage::ManualFetch));
		}

		DataType.TextureCoordinatesSRV = ShaderResourceView;
	}

	template <uint32 MaxNumUpdates>
	void UpdateRHIFromExisting(FRHIVertexBuffer* IntermediateBuffer, int32 NumElements, bool bShouldUseHighPrecision, int32 NumChannels, TRHIResourceUpdateBatcher<MaxNumUpdates>& Batcher)
	{
		if (VertexBufferRHI && IntermediateBuffer)
		{
			bUseHighPrecision = bShouldUseHighPrecision;
			NumUVs = NumChannels;
			VertexSize = CalculateStride(bShouldUseHighPrecision, NumChannels);
			NumVertices = NumElements;

			FRuntimeMeshVertexBuffer::UpdateRHIFromExisting<MaxNumUpdates>(IntermediateBuffer, Batcher);
		}
	}

	void InitRHIFromExisting(const FVertexBufferRHIRef& InVertexBufferRHI, int32 NumElements, bool bShouldUseHighPrecision, int32 NumChannels)
	{
		bUseHighPrecision = bShouldUseHighPrecision;
		NumUVs = NumChannels;
		VertexSize = CalculateStride(bShouldUseHighPrecision, NumChannels);
		NumVertices = NumElements;

		FRuntimeMeshVertexBuffer::InitRHIFromExisting(InVertexBufferRHI);
	}

};

class FRuntimeMeshColorVertexBuffer : public FRuntimeMeshVertexBuffer
{
public:
	FRuntimeMeshColorVertexBuffer(bool bInIsDynamicBuffer)
		: FRuntimeMeshVertexBuffer(bInIsDynamicBuffer, sizeof(FColor))
	{

	}

	virtual FString GetFriendlyName() const override { return TEXT("FRuntimeMeshColorVertexBuffer"); }

	virtual int32 GetElementDatumSize() const override { return sizeof(FColor); }
	virtual EPixelFormat GetElementFormat() const override { return PF_R8G8B8A8; }

	virtual void Bind(FLocalVertexFactory::FDataType& DataType) override
	{
		DataType.ColorComponent = FVertexStreamComponent(this, 0, 4, EVertexElementType::VET_Color, EVertexStreamUsage::ManualFetch);
		DataType.ColorComponentsSRV = ShaderResourceView;
	}

	template <uint32 MaxNumUpdates>
	void UpdateRHIFromExisting(FRHIVertexBuffer* IntermediateBuffer, int32 NumElements, TRHIResourceUpdateBatcher<MaxNumUpdates>& Batcher)
	{
		if (VertexBufferRHI && IntermediateBuffer)
		{
			VertexSize = sizeof(FColor);
			NumVertices = NumElements;

			FRuntimeMeshVertexBuffer::UpdateRHIFromExisting<MaxNumUpdates>(IntermediateBuffer, Batcher);
		}
	}

	void InitRHIFromExisting(const FVertexBufferRHIRef& InVertexBufferRHI, int32 NumElements)
	{
		VertexSize = sizeof(FColor);
		NumVertices = NumElements;

		FRuntimeMeshVertexBuffer::InitRHIFromExisting(InVertexBufferRHI);
	}

};





/** Index Buffer */
class FRuntimeMeshIndexBuffer : public FIndexBuffer
{
	static constexpr int32 CalculateStride(bool bShouldUseHighPrecision)
	{
		return bShouldUseHighPrecision ? sizeof(int32) : sizeof(uint16);
	}

private:
	/** Should this buffer by flagged as dynamic */
	bool bIsDynamicBuffer;

	/* The size of a single index*/
	int32 IndexSize;

	/* The number of indices this buffer is currently allocated to hold */
	int32 NumIndices;

public:

	FRuntimeMeshIndexBuffer(bool bInIsDynamicBuffer);

	virtual FString GetFriendlyName() const override { return TEXT("FRuntimeMeshIndexBuffer"); }

	virtual void InitRHI() override;

	/* Get the size of the index buffer */
	int32 Num() const { return NumIndices; }

	/** Gets the full allocated size of the buffer (Equal to IndexSize * NumIndices) */
	int32 GetBufferSize() const { return NumIndices * IndexSize; }
	
	/* Set the data for the index buffer */
	//void SetData(bool bInUse32BitIndices, int32 NewIndexCount, const uint8* InData);



	template<bool bIsInRenderThread>
	static FIndexBufferRHIRef CreateRHIBuffer(FRHIResourceCreateInfo& CreateInfo, uint32 IndexSize, uint32 SizeInBytes, bool bDynamicBuffer)
	{
		const int32 Flags = (bDynamicBuffer ? BUF_Dynamic : BUF_Static) | BUF_ShaderResource;
		CreateInfo.bWithoutNativeResource = SizeInBytes <= 0;
		if (bIsInRenderThread)
		{
			return RHICreateIndexBuffer(IndexSize, SizeInBytes, Flags, CreateInfo);
		}
		else
		{
			return RHIAsyncCreateIndexBuffer(IndexSize, SizeInBytes, Flags, CreateInfo);
		}
		return nullptr;

	}

	template<bool bIsInRenderThread>
	static FIndexBufferRHIRef CreateRHIBuffer(FRuntimeMeshBufferUpdateData& InStream, bool bDynamicBuffer)
	{
		const uint32 SizeInBytes = InStream.GetResourceDataSize();

		FRHIResourceCreateInfo CreateInfo(&InStream);

		return CreateRHIBuffer<bIsInRenderThread>(CreateInfo, InStream.GetStride(), SizeInBytes, bDynamicBuffer);
	}


	template <uint32 MaxNumUpdates>
	void UpdateRHIFromExisting(FRHIIndexBuffer* IntermediateBuffer, int32 NumElements, bool bShouldUseHighPrecision, TRHIResourceUpdateBatcher<MaxNumUpdates>& Batcher)
	{
		if (IndexBufferRHI && IntermediateBuffer)
		{
			IndexSize = CalculateStride(bShouldUseHighPrecision);
			NumIndices = NumElements;

			Batcher.QueueUpdateRequest(IndexBufferRHI, IntermediateBuffer);
		}
	}

	void InitRHIFromExisting(const FIndexBufferRHIRef& InIndexBufferRHI, int32 NumElements, bool bShouldUseHighPrecision)
	{
		InitResource();

		IndexSize = CalculateStride(bShouldUseHighPrecision);
		NumIndices = NumElements;

		if (InIndexBufferRHI)
		{
			IndexBufferRHI = InIndexBufferRHI;
		}
		else
		{
			FRHIResourceCreateInfo CreateInfo;
			IndexBufferRHI = CreateRHIBuffer<true>(CreateInfo, IndexSize, 0, bIsDynamicBuffer);
		}
	}

};

/** Vertex Factory */
class FRuntimeMeshVertexFactory : public FLocalVertexFactory
{
public:

	FRuntimeMeshVertexFactory(ERHIFeatureLevel::Type InFeatureLevel);

	/** Init function that can be called on any thread, and will do the right thing (enqueue command if called on main thread) */
	void Init(FLocalVertexFactory::FDataType VertexStructure);

	/* Gets the section visibility for static sections */	
	//virtual uint64 GetStaticBatchElementVisibility(const class FSceneView& View, const struct FMeshBatch* Batch, const void* InViewCustomData = nullptr) const override;

private:
	/* Interface to the parent section for checking visibility.*/
	//FRuntimeMeshSectionProxy* SectionParent;
};



/** Deleter function for TSharedPtrs that only allows the object to be destructed on the render thread. */
template<typename Type>
struct FRuntimeMeshRenderThreadDeleter
{
	void operator()(Type* Object) const
	{
		// This is a custom deleter to make sure the runtime mesh proxy is only ever deleted on the rendering thread.
		if (IsInRenderingThread())
		{
			delete Object;
		}
		else
		{
			ENQUEUE_RENDER_COMMAND(FRuntimeMeshProxyDeleterCommand)(
				[Object](FRHICommandListImmediate& RHICmdList)
				{
					delete static_cast<Type*>(Object);
				}
			);
		}
	}
};

#define RMC_LOG_VERBOSE(Format, ...) \
	UE_LOG(RuntimeMeshLog, Verbose, TEXT("[Thread:%d]: " Format), FPlatformTLS::GetCurrentThreadId(), ##__VA_ARGS__);


template<bool bIsInRenderThread>
void FRuntimeMeshSectionUpdateData::CreateRHIBuffers(bool bShouldUseDynamicBuffers)
{
	if (!bBuffersCreated)
	{
		RMC_LOG_VERBOSE("Creating GPU buffers for section.");

		PositionsBuffer = FRuntimeMeshVertexBuffer::CreateRHIBuffer<bIsInRenderThread>(Positions, bShouldUseDynamicBuffers);
		TangentsBuffer = FRuntimeMeshVertexBuffer::CreateRHIBuffer<bIsInRenderThread>(Tangents, bShouldUseDynamicBuffers);
		TexCoordsBuffer = FRuntimeMeshVertexBuffer::CreateRHIBuffer<bIsInRenderThread>(TexCoords, bShouldUseDynamicBuffers);
		ColorsBuffer = FRuntimeMeshVertexBuffer::CreateRHIBuffer<bIsInRenderThread>(Colors, bShouldUseDynamicBuffers);

		TrianglesBuffer = FRuntimeMeshIndexBuffer::CreateRHIBuffer<bIsInRenderThread>(Triangles, bShouldUseDynamicBuffers);
		AdjacencyTrianglesBuffer = FRuntimeMeshIndexBuffer::CreateRHIBuffer<bIsInRenderThread>(AdjacencyTriangles, bShouldUseDynamicBuffers);

		bBuffersCreated = true;
	}
}


#undef RMC_LOG_VERBOSE