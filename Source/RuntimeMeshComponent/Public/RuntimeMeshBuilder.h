// Copyright 2016-2018 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RuntimeMeshCore.h"

class FRuntimeMeshData;
using FRuntimeMeshDataPtr = TSharedRef<FRuntimeMeshData, ESPMode::ThreadSafe>;
class FRuntimeMeshSection;
using FRuntimeMeshSectionPtr = TSharedPtr<FRuntimeMeshSection, ESPMode::ThreadSafe>;


struct RUNTIMEMESHCOMPONENT_API FRuntimeMeshAccessorVertex
{
	FVector Position;
	FVector4 Normal;
	FVector Tangent;
	FColor Color;

	TArray<FVector2D, TInlineAllocator<RUNTIMEMESH_MAXTEXCOORDS>> UVs;
};

class RUNTIMEMESHCOMPONENT_API FRuntimeMeshVerticesAccessor
{
	bool bIsInitialized;
	TArray<uint8>* Stream0;
	int32 Stream0Stride;
	TArray<uint8>* Stream1;
	int32 Stream1Stride;
	TArray<uint8>* Stream2;
	int32 Stream2Stride;

	DECLARE_DELEGATE_RetVal_OneParam(FVector, FStreamPositionReader, int32);
	DECLARE_DELEGATE_RetVal_OneParam(FVector4, FStreamNormalTangentReader, int32);
	DECLARE_DELEGATE_RetVal_OneParam(FColor, FStreamColorReader, int32);
	DECLARE_DELEGATE_RetVal_OneParam(FVector2D, FStreamUVReader, int32);

	DECLARE_DELEGATE_TwoParams(FStreamPositionWriter, int32, FVector);
	DECLARE_DELEGATE_TwoParams(FStreamNormalTangentWriter, int32, FVector4);
	DECLARE_DELEGATE_TwoParams(FStreamColorWriter, int32, FColor);
	DECLARE_DELEGATE_TwoParams(FStreamUVWriter, int32, FVector2D);

	FStreamPositionReader PositionReader;
	FStreamNormalTangentReader NormalReader;
	FStreamNormalTangentReader TangentReader;
	FStreamColorReader ColorReader;
	TArray<FStreamUVReader, TInlineAllocator<RUNTIMEMESH_MAXTEXCOORDS>> UVReaders;

	FStreamPositionWriter PositionWriter;
	FStreamNormalTangentWriter NormalWriter;
	FStreamNormalTangentWriter TangentWriter;
	FStreamColorWriter ColorWriter;
	TArray<FStreamUVWriter, TInlineAllocator<RUNTIMEMESH_MAXTEXCOORDS>> UVWriters;

public:

	FRuntimeMeshVerticesAccessor(const FRuntimeMeshVertexStreamStructure& Stream0,
		const FRuntimeMeshVertexStreamStructure& Stream1,
		const FRuntimeMeshVertexStreamStructure& Stream2,
		TArray<uint8>* Stream0Data, TArray<uint8>* Stream1Data, TArray<uint8>* Stream2Data);
	virtual ~FRuntimeMeshVerticesAccessor();

protected:
	FRuntimeMeshVerticesAccessor(TArray<uint8>* Stream0Data, TArray<uint8>* Stream1Data,
		TArray<uint8>* Stream2Data);

	void Initialize(const FRuntimeMeshVertexStreamStructure& Stream0Structure,
		const FRuntimeMeshVertexStreamStructure& Stream1Structure,
		const FRuntimeMeshVertexStreamStructure& Stream2Structure);

public:

	int32 NumVertices() const;
	int32 NumUVChannels() const;

	void EmptyVertices(int32 Slack = 0);
	void SetNumVertices(int32 NewNum);

	int32 AddVertex(FVector InPosition);

	FVector GetPosition(int32 Index) const;
	FVector4 GetNormal(int32 Index) const;
	FVector GetTangent(int32 Index) const;
	FColor GetColor(int32 Index) const;
	FVector2D GetUV(int32 Index, int32 Channel = 0) const;


	void SetPosition(int32 Index, FVector Value);
	void SetNormal(int32 Index, FVector4 Value);
	void SetTangent(int32 Index, FVector Value);
	void SetTangent(int32 Index, FRuntimeMeshTangent Value);
	void SetColor(int32 Index, FColor Value);
	void SetUV(int32 Index, FVector2D Value);
	void SetUV(int32 Index, int32 Channel, FVector2D Value);

	void SetNormalTangent(int32 Index, FVector Normal, FRuntimeMeshTangent Tangent);
	void SetTangents(int32 Index, FVector TangentX, FVector TangentY, FVector TangentZ);

	FRuntimeMeshAccessorVertex GetVertex(int32 Index) const;
	void SetVertex(int32 Index, const FRuntimeMeshAccessorVertex& Vertex);
	int32 AddVertex(const FRuntimeMeshAccessorVertex& Vertex);

protected:

	int32 AddSingleVertex();

	template<typename TYPE>
	FORCEINLINE static TYPE Read(int32 Index, TArray<uint8>* Data, int32 Stride, int32 Offset)
	{
		int32 StartPosition = (Index * Stride + Offset);
		return *((TYPE*)(&(*Data)[StartPosition]));
	}

	template<typename TYPE>
	FORCEINLINE static void Write(int32 Index, TYPE Value, TArray<uint8>* Data, int32 Stride, int32 Offset)
	{
		int32 StartPosition = (Index * Stride + Offset);
		*((TYPE*)(&(*Data)[StartPosition])) = Value;
	}

	FORCEINLINE static FVector4 ReadNormalTangentPackedNormal(int32 Index, TArray<uint8>* Data, int32 Stride, int32 Offset)
	{
		return Read<FPackedNormal>(Index, Data, Stride, Offset);
	}
	FORCEINLINE static FVector4 ReadNormalTangentPackedRGBA16N(int32 Index, TArray<uint8>* Data, int32 Stride, int32 Offset)
	{
		return Read<FPackedRGBA16N>(Index, Data, Stride, Offset);
	}
	FORCEINLINE static FVector4 ReadNormalNull(int32 Index, TArray<uint8>* Data, int32 Stride, int32 Offset)
	{
		check(false && "This stream has no normal channel.");
		return FVector4(EForceInit::ForceInitToZero);
	}
	FORCEINLINE static FVector4 ReadTangentNull(int32 Index, TArray<uint8>* Data, int32 Stride, int32 Offset)
	{
		check(false && "This stream has no tangent channel.");
		return FVector4(EForceInit::ForceInitToZero);
	}

	FORCEINLINE static void WriteNormalTangentPackedNormal(int32 Index, FVector4 Value, TArray<uint8>* Data, int32 Stride, int32 Offset)
	{
		return Write<FPackedNormal>(Index, FPackedNormal(Value), Data, Stride, Offset);
	}
	FORCEINLINE static void WriteNormalTangentPackedRGBA16N(int32 Index, FVector4 Value, TArray<uint8>* Data, int32 Stride, int32 Offset)
	{
		return Write<FPackedRGBA16N>(Index, Value, Data, Stride, Offset);
	}
	FORCEINLINE static void WriteNormalNull(int32 Index, FVector4 Value, TArray<uint8>* Data, int32 Stride, int32 Offset)
	{
		check(false && "This stream has no normal channel.");
	}
	FORCEINLINE static void WriteTangentNull(int32 Index, FVector4 Value, TArray<uint8>* Data, int32 Stride, int32 Offset)
	{
		check(false && "This stream has no tangent channel.");
	}

	FORCEINLINE static FVector2D ReadUVVector2D(int32 Index, TArray<uint8>* Data, int32 Stride, int32 Offset)
	{
		return Read<FVector2D>(Index, Data, Stride, Offset);
	}
	FORCEINLINE static FVector2D ReadUVVector2DHalf(int32 Index, TArray<uint8>* Data, int32 Stride, int32 Offset)
	{
		return Read<FVector2DHalf>(Index, Data, Stride, Offset);
	}
	FORCEINLINE static FVector2D ReadUVVector2DNull(int32 Index, TArray<uint8>* Data, int32 Stride, int32 Offset)
	{
		check(false && "This stream has no UV channel.");
		return FVector2D::ZeroVector;
	}

	FORCEINLINE static void WriteUVVector2D(int32 Index, FVector2D Value, TArray<uint8>* Data, int32 Stride, int32 Offset)
	{
		return Write<FVector2D>(Index, Value, Data, Stride, Offset);
	}
	FORCEINLINE static void WriteUVVector2DHalf(int32 Index, FVector2D Value, TArray<uint8>* Data, int32 Stride, int32 Offset)
	{
		return Write<FVector2DHalf>(Index, Value, Data, Stride, Offset);
	}
	FORCEINLINE static void WriteUVVector2DNull(int32 Index, FVector2D Value, TArray<uint8>* Data, int32 Stride, int32 Offset)
	{
		check(false && "This stream has no UV channel.");
	}

	FORCEINLINE static FColor ReadColorNull(int32 Index, TArray<uint8>* Data, int32 Stride, int32 Offset)
	{
		check(false && "This stream has no color channel.");
		return FColor::Transparent;
	}

	FORCEINLINE static void WriteColorNull(int32 Index, FColor Value, TArray<uint8>* Data, int32 Stride, int32 Offset)
	{
		check(false && "This stream has no color channel.");
	}
};




class RUNTIMEMESHCOMPONENT_API FRuntimeMeshIndicesAccessor
{
	bool bIsInitialized;
	TArray<uint8>* IndexStream;
	bool b32BitIndices;

	DECLARE_DELEGATE_RetVal_OneParam(int32, FIndexStreamReader, int32);
	DECLARE_DELEGATE_TwoParams(FIndexStreamWriter, int32, int32);

	FIndexStreamReader IndexReader;
	FIndexStreamWriter IndexWriter;

public:

	FRuntimeMeshIndicesAccessor(bool bIn32BitIndices, TArray<uint8>* IndexStreamData);
	virtual ~FRuntimeMeshIndicesAccessor();

protected:
	FRuntimeMeshIndicesAccessor(TArray<uint8>* IndexStreamData);

	void Initialize(bool bIn32BitIndices);

public:

	int32 NumIndices() const;
	void EmptyIndices(int32 Slack = 0);
	void SetNumIndices(int32 NewNum);
	int32 AddIndex(int32 NewIndex);
	int32 AddTriangle(int32 Index0, int32 Index1, int32 Index2);

	int32 GetIndex(int32 Index) const;
	void SetIndex(int32 Index, int32 Value);

protected:

	FORCEINLINE int32 GetIndexStride() const { return b32BitIndices ? 4 : 2; }

	FORCEINLINE static int32 ReadIndex16(int32 Index, TArray<uint8>* Data)
	{
		int32 StartPosition = (Index * 2);
		return *((uint16*)(&(*Data)[StartPosition]));
	}
	FORCEINLINE static int32 ReadIndex32(int32 Index, TArray<uint8>* Data)
	{
		int32 StartPosition = (Index * 4);
		return *((int32*)(&(*Data)[StartPosition]));
	}

	FORCEINLINE static void WriteIndex16(int32 Index, int32 Value, TArray<uint8>* Data)
	{
		int32 StartPosition = (Index * 2);
		*((uint16*)(&(*Data)[StartPosition])) = Value;
	}
	FORCEINLINE static void WriteIndex32(int32 Index, int32 Value, TArray<uint8>* Data)
	{
		int32 StartPosition = (Index * 4);
		*((int32*)(&(*Data)[StartPosition])) = Value;
	}

};




/**
*
*/
class RUNTIMEMESHCOMPONENT_API FRuntimeMeshAccessor : public FRuntimeMeshVerticesAccessor, public FRuntimeMeshIndicesAccessor
{


public:

	FRuntimeMeshAccessor(const FRuntimeMeshVertexStreamStructure& Stream0,
		const FRuntimeMeshVertexStreamStructure& Stream1,
		const FRuntimeMeshVertexStreamStructure& Stream2, bool bIn32BitIndices,
		TArray<uint8>* Stream0Data, TArray<uint8>* Stream1Data, TArray<uint8>* Stream2Data, TArray<uint8>* IndexStreamData);
	virtual ~FRuntimeMeshAccessor() override;

protected:
	FRuntimeMeshAccessor(TArray<uint8>* Stream0Data, TArray<uint8>* Stream1Data,
		TArray<uint8>* Stream2Data, TArray<uint8>* IndexStreamData);

	void Initialize(const FRuntimeMeshVertexStreamStructure& Stream0Structure,
		const FRuntimeMeshVertexStreamStructure& Stream1Structure,
		const FRuntimeMeshVertexStreamStructure& Stream2Structure, bool bIn32BitIndices);

public:


};


/**
 * Generic mesh builder. Can work on any valid stream configuration.
 * Wraps FRuntimeMeshAccessor to provide standalone operation for creating new mesh data.
 */
class RUNTIMEMESHCOMPONENT_API FRuntimeMeshBuilder : public FRuntimeMeshAccessor
{
	TArray<uint8> Stream0;
	FRuntimeMeshVertexStreamStructure Stream0Structure;
	TArray<uint8> Stream1;
	FRuntimeMeshVertexStreamStructure Stream1Structure;
	TArray<uint8> Stream2;
	FRuntimeMeshVertexStreamStructure Stream2Structure;

	TArray<uint8> IndexStream;
	bool b32BitIndices;

public:
	FRuntimeMeshBuilder(const FRuntimeMeshVertexStreamStructure& InStream0Structure, const FRuntimeMeshVertexStreamStructure& InStream1Structure,
		const FRuntimeMeshVertexStreamStructure& InStream2Structure, bool bIn32BitIndices);

	virtual ~FRuntimeMeshBuilder() override;


	const FRuntimeMeshVertexStreamStructure& GetStream0Structure() const { return Stream0Structure; }
	const FRuntimeMeshVertexStreamStructure& GetStream1Structure() const { return Stream1Structure; }
	const FRuntimeMeshVertexStreamStructure& GetStream2Structure() const { return Stream2Structure; }
	bool IsUsing32BitIndices() const { return b32BitIndices; }

	TArray<uint8>& GetStream0() { return Stream0; }
	TArray<uint8>& GetStream1() { return Stream1; }
	TArray<uint8>& GetStream2() { return Stream2; }
	TArray<uint8>& GetIndexStream() { return IndexStream; }
};


template<typename VertexType0, typename IndexType>
FORCEINLINE TSharedPtr<FRuntimeMeshBuilder> MakeRuntimeMeshBuilder()
{
	return MakeShared<FRuntimeMeshBuilder>(GetStreamStructure<VertexType0>(), GetStreamStructure<FRuntimeMeshNullVertex>(), GetStreamStructure<FRuntimeMeshNullVertex>(), FRuntimeMeshIndexTraits<IndexType>::Is32Bit);
}

template<typename VertexType0, typename VertexType1, typename IndexType>
FORCEINLINE TSharedPtr<FRuntimeMeshBuilder> MakeRuntimeMeshBuilder()
{
	return MakeShared<FRuntimeMeshBuilder>(GetStreamStructure<VertexType0>(), GetStreamStructure<VertexType1>(), GetStreamStructure<FRuntimeMeshNullVertex>(), FRuntimeMeshIndexTraits<IndexType>::Is32Bit);
}

template<typename VertexType0, typename VertexType1, typename VertexType2, typename IndexType>
FORCEINLINE TSharedPtr<FRuntimeMeshBuilder> MakeRuntimeMeshBuilder()
{
	return MakeShared<FRuntimeMeshBuilder>(GetStreamStructure<VertexType0>(), GetStreamStructure<VertexType1>(), GetStreamStructure<VertexType2>(), FRuntimeMeshIndexTraits<IndexType>::Is32Bit);
}
