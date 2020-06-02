// Copyright 2016-2020 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "Engine/Engine.h"
#include "RuntimeMeshCore.h"


/** Single vertex buffer to hold one vertex stream within a section */
class FRuntimeMeshVertexBuffer : public FVertexBuffer
{
protected:
	/** The buffer configuration to use */
	const EBufferUsageFlags UsageFlags;

	/** Size of a single vertex */
	int32 VertexSize;

	/** The number of vertices this buffer is currently allocated to hold */
	int32 NumVertices;

	/** Shader Resource View for this buffer */
	FShaderResourceViewRHIRef ShaderResourceView;

public:

	FRuntimeMeshVertexBuffer(ERuntimeMeshUpdateFrequency InUpdateFrequency);

	~FRuntimeMeshVertexBuffer() {}

	virtual void InitRHI() override;

	/** Gets the size of the vertex */
	int32 Stride() const { return VertexSize; }

	/** Get the size of the vertex buffer */
	int32 Num() const { return NumVertices; }

	/** Gets the full allocated size of the buffer (Equal to VertexSize * NumVertices) */
	int32 GetBufferSize() const { return NumVertices * VertexSize; }

	/** Binds the vertex buffer to the factory data type */
	virtual void Bind(FLocalVertexFactory::FDataType& DataType) = 0;


protected:
	void SetData(int32 NewStride, int32 NewVertexCount, const uint8* InData);

	virtual void CreateSRV() = 0;
};


class FRuntimeMeshPositionVertexBuffer : public FRuntimeMeshVertexBuffer
{
public:
	FRuntimeMeshPositionVertexBuffer(ERuntimeMeshUpdateFrequency InUpdateFrequency)
		: FRuntimeMeshVertexBuffer(InUpdateFrequency)
	{

	}

	virtual FString GetFriendlyName() const override { return TEXT("FRuntimeMeshPositionVertexBuffer"); }

	void SetData(int32 NewVertexCount, const uint8* InData)
	{
		FRuntimeMeshVertexBuffer::SetData(sizeof(FVector), NewVertexCount, InData);
	}

	virtual void Bind(FLocalVertexFactory::FDataType& DataType) override 
	{
		DataType.PositionComponent = FVertexStreamComponent(this, 0, 12, VET_Float3);
		DataType.PositionComponentSRV = ShaderResourceView;
	}

protected:
	virtual void CreateSRV() override 
	{
		ShaderResourceView = RHICreateShaderResourceView(VertexBufferRHI, 4, PF_R32_FLOAT);
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
	FRuntimeMeshTangentsVertexBuffer(ERuntimeMeshUpdateFrequency InUpdateFrequency)
		: FRuntimeMeshVertexBuffer(InUpdateFrequency)
		, bUseHighPrecision(false)
	{

	}

	virtual FString GetFriendlyName() const override { return TEXT("FRuntimeMeshTangentsVertexBuffer"); }

	void SetData(bool bInUseHighPrecision, int32 NewVertexCount, const uint8* InData)
	{
		bUseHighPrecision = bInUseHighPrecision;
		FRuntimeMeshVertexBuffer::SetData(CalculateStride(bInUseHighPrecision), NewVertexCount, InData);
	}

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

protected:
	virtual void CreateSRV() override
	{
		ShaderResourceView = RHICreateShaderResourceView(VertexBufferRHI, bUseHighPrecision ? 8 : 4, bUseHighPrecision ? PF_R16G16B16A16_SNORM : PF_R8G8B8A8_SNORM);
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
	FRuntimeMeshTexCoordsVertexBuffer(ERuntimeMeshUpdateFrequency InUpdateFrequency)
		: FRuntimeMeshVertexBuffer(InUpdateFrequency)
		, bUseHighPrecision(false)
		, NumUVs(0)
	{

	}

	virtual FString GetFriendlyName() const override { return TEXT("FRuntimeMeshUVsVertexBuffer"); }

	void SetData(bool bInUseHighPrecision, int32 InNumUVs, int32 NewVertexCount, const uint8* InData)
	{
		bUseHighPrecision = bInUseHighPrecision;
		NumUVs = InNumUVs;
		FRuntimeMeshVertexBuffer::SetData(CalculateStride(bInUseHighPrecision, InNumUVs), NewVertexCount, InData);
	}

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
			DataType.TextureCoordinates.Add(FVertexStreamComponent(this, UVSizeInBytes * UVIndex, UVStride, UVDoubleWideVertexElementType, EVertexStreamUsage::ManualFetch));
		}

		DataType.TextureCoordinatesSRV = ShaderResourceView;
	}

protected:
	virtual void CreateSRV() override
	{
		ShaderResourceView = RHICreateShaderResourceView(VertexBufferRHI, bUseHighPrecision ? 8 : 4, bUseHighPrecision ? PF_G32R32F : PF_G16R16F);
	}
};

class FRuntimeMeshColorVertexBuffer : public FRuntimeMeshVertexBuffer
{
public:
	FRuntimeMeshColorVertexBuffer(ERuntimeMeshUpdateFrequency InUpdateFrequency)
		: FRuntimeMeshVertexBuffer(InUpdateFrequency)
	{

	}

	virtual FString GetFriendlyName() const override { return TEXT("FRuntimeMeshColorVertexBuffer"); }

	void SetData(int32 NewVertexCount, const uint8* InData)
	{
		FRuntimeMeshVertexBuffer::SetData(sizeof(FColor), NewVertexCount, InData);
	}

	virtual void Bind(FLocalVertexFactory::FDataType& DataType) override
	{
		DataType.ColorComponent = FVertexStreamComponent(this, 0, 4, EVertexElementType::VET_Color, EVertexStreamUsage::ManualFetch);
		DataType.ColorComponentsSRV = ShaderResourceView;
	}

protected:
	virtual void CreateSRV() override
	{
		ShaderResourceView = RHICreateShaderResourceView(VertexBufferRHI, 4, PF_R8G8B8A8);
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
	/* The buffer configuration to use */
	EBufferUsageFlags UsageFlags;

	/* The size of a single index*/
	int32 IndexSize;

	/* The number of indices this buffer is currently allocated to hold */
	int32 NumIndices;

public:

	FRuntimeMeshIndexBuffer(ERuntimeMeshUpdateFrequency InUpdateFrequency);

	virtual FString GetFriendlyName() const override { return TEXT("FRuntimeMeshIndexBuffer"); }

	virtual void InitRHI() override;

	/* Get the size of the index buffer */
	int32 Num() const { return NumIndices; }

	/** Gets the full allocated size of the buffer (Equal to IndexSize * NumIndices) */
	int32 GetBufferSize() const { return NumIndices * IndexSize; }
	
	/* Set the data for the index buffer */
	void SetData(bool bInUse32BitIndices, int32 NewIndexCount, const uint8* InData);
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