// Copyright 2016-2018 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RuntimeMeshCore.h"
#include "RuntimeMeshGenericVertex.h"

class FRuntimeMeshData;
using FRuntimeMeshDataPtr = TSharedPtr<FRuntimeMeshData, ESPMode::ThreadSafe>;
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
	TArray<uint8>* PositionStream;
	static const int32 PositionStride = 12;
	TArray<uint8>* TangentStream;
	const bool bTangentHighPrecision;
	const int32 TangentSize;
	const int32 TangentStride;
	TArray<uint8>* UVStream;
	const bool bUVHighPrecision;
	const int32 UVChannelCount;
	const int32 UVSize;
	const int32 UVStride;
	TArray<uint8>* ColorStream;
	static const int32 ColorStride = 4;

public:

	FRuntimeMeshVerticesAccessor(bool bInTangentsHighPrecision, bool bInUVsHighPrecision, int32 bInUVCount,
		TArray<uint8>* PositionStreamData, TArray<uint8>* TangentStreamData, TArray<uint8>* UVStreamData, TArray<uint8>* ColorStreamData);
	virtual ~FRuntimeMeshVerticesAccessor();

protected:
	FRuntimeMeshVerticesAccessor(TArray<uint8>* PositionStreamData, TArray<uint8>* TangentStreamData, TArray<uint8>* UVStreamData, TArray<uint8>* ColorStreamData);

	void Initialize(bool bInTangentsHighPrecision, bool bInUVsHighPrecision, int32 bInUVCount);

public:
	const bool IsUsingHighPrecisionTangents() const { return bTangentHighPrecision; }
	const bool IsUsingHighPrecisionUVs() const { return bUVHighPrecision; }

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
	void SetNormal(int32 Index, const FVector4& Value);
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
	FORCEINLINE_DEBUGGABLE static TYPE Read(int32 Index, TArray<uint8>* Data, int32 Stride, int32 Offset)
	{
		int32 StartPosition = (Index * Stride + Offset);
		return *((TYPE*)(&(*Data)[StartPosition]));
	}

	template<typename TYPE>
	FORCENOINLINE static void Write(int32 Index, TYPE Value, TArray<uint8>* Data, int32 Stride, int32 Offset)
	{
		int32 StartPosition = (Index * Stride + Offset);
		*((TYPE*)(&(*Data)[StartPosition])) = Value;
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
	bool IsUsing32BitIndices() const { return b32BitIndices; }

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

	FRuntimeMeshAccessor(bool bInTangentsHighPrecision, bool bInUVsHighPrecision, int32 bInUVCount, bool bIn32BitIndices, TArray<uint8>* PositionStreamData,
		TArray<uint8>* TangentStreamData, TArray<uint8>* UVStreamData, TArray<uint8>* ColorStreamData, TArray<uint8>* IndexStreamData);
	virtual ~FRuntimeMeshAccessor() override;

protected:
	FRuntimeMeshAccessor(TArray<uint8>* PositionStreamData, TArray<uint8>* TangentStreamData, TArray<uint8>* UVStreamData, TArray<uint8>* ColorStreamData, TArray<uint8>* IndexStreamData);

	void Initialize(bool bInTangentsHighPrecision, bool bInUVsHighPrecision, int32 bInUVCount, bool bIn32BitIndices);

public:
	void CopyTo(const TSharedPtr<FRuntimeMeshAccessor>& Other, bool bClearDestination = false) const;

};


/**
 * Generic mesh builder. Can work on any valid stream configuration.
 * Wraps FRuntimeMeshAccessor to provide standalone operation for creating new mesh data.
 */
class RUNTIMEMESHCOMPONENT_API FRuntimeMeshBuilder : public FRuntimeMeshAccessor
{
	TArray<uint8> PositionStream;
	TArray<uint8> TangentStream;
	TArray<uint8> UVStream;
	TArray<uint8> ColorStream;

	TArray<uint8> IndexStream;

public:
	FRuntimeMeshBuilder(bool bInTangentsHighPrecision, bool bInUVsHighPrecision, int32 bInUVCount, bool bIn32BitIndices);

	virtual ~FRuntimeMeshBuilder() override;
	
	TArray<uint8>& GetPositionStream() { return PositionStream; }
	TArray<uint8>& GetTangentStream() { return TangentStream; }
	TArray<uint8>& GetUVStream() { return UVStream; }
	TArray<uint8>& GetColorStream() { return ColorStream; }
	TArray<uint8>& GetIndexStream() { return IndexStream; }
};



template<typename TangentType, typename UVType, typename IndexType>
FORCEINLINE TSharedRef<FRuntimeMeshBuilder> MakeRuntimeMeshBuilder()
{
	bool bIsUsingHighPrecisionUVs;
	int32 NumUVChannels;
	GetUVVertexProperties<UVType>(bIsUsingHighPrecisionUVs, NumUVChannels);

	return MakeShared<FRuntimeMeshBuilder>(GetTangentIsHighPrecision<TangentType>(), bIsUsingHighPrecisionUVs, NumUVChannels, (bool)FRuntimeMeshIndexTraits<IndexType>::Is32Bit);
}


FORCEINLINE TSharedRef<FRuntimeMeshBuilder> MakeRuntimeMeshBuilder(const TSharedRef<const FRuntimeMeshAccessor>& StructureToCopy)
{
	return MakeShared<FRuntimeMeshBuilder>(StructureToCopy->IsUsingHighPrecisionTangents(), StructureToCopy->IsUsingHighPrecisionUVs(), StructureToCopy->NumUVChannels(), StructureToCopy->IsUsing32BitIndices());
}
