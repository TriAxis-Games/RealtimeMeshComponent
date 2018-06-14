// Copyright 2016-2018 Chris Conway (Koderz). All Rights Reserved.

#include "RuntimeMeshComponentPlugin.h"
#include "RuntimeMeshBuilder.h"

//////////////////////////////////////////////////////////////////////////
//	FRuntimeMeshVerticesAccessor

FRuntimeMeshVerticesAccessor::FRuntimeMeshVerticesAccessor(TArray<uint8>* PositionStreamData, TArray<uint8>* TangentStreamData, TArray<uint8>* UVStreamData, TArray<uint8>* ColorStreamData)
	: bIsInitialized(false)
	, PositionStream(PositionStreamData)
	, TangentStream(TangentStreamData), bTangentHighPrecision(false), TangentSize(0), TangentStride(0)
	, UVStream(UVStreamData), bUVHighPrecision(false), UVSize(0), UVStride(0), UVChannelCount(0)
	, ColorStream(ColorStreamData)
{
}

FRuntimeMeshVerticesAccessor::FRuntimeMeshVerticesAccessor(bool bInTangentsHighPrecision, bool bInUVsHighPrecision, int32 bInUVCount,
	TArray<uint8>* PositionStreamData, TArray<uint8>* TangentStreamData, TArray<uint8>* UVStreamData, TArray<uint8>* ColorStreamData)
	: bIsInitialized(false)
	, PositionStream(PositionStreamData)
	, TangentStream(TangentStreamData), bTangentHighPrecision(false), TangentSize(0), TangentStride(0)
	, UVStream(UVStreamData), bUVHighPrecision(false), UVSize(0), UVStride(0), UVChannelCount(0)
	, ColorStream(ColorStreamData)
{
	Initialize(bInTangentsHighPrecision, bInUVsHighPrecision, bInUVCount);
}

FRuntimeMeshVerticesAccessor::~FRuntimeMeshVerticesAccessor()
{
}

void FRuntimeMeshVerticesAccessor::Initialize(bool bInTangentsHighPrecision, bool bInUVsHighPrecision, int32 bInUVCount)
{
	bIsInitialized = true;
	const_cast<bool&>(bTangentHighPrecision) = bInTangentsHighPrecision;
	const_cast<int32&>(TangentSize) = (bTangentHighPrecision ? sizeof(FPackedRGBA16N) : sizeof(FPackedNormal));
	const_cast<int32&>(TangentStride) = TangentSize * 2;

	const_cast<bool&>(bUVHighPrecision) = bInUVsHighPrecision;
	const_cast<int32&>(UVChannelCount) = bInUVCount;
	const_cast<int32&>(UVSize) = (bUVHighPrecision ? sizeof(FVector2D) : sizeof(FVector2DHalf));
	const_cast<int32&>(UVStride) = UVSize * UVChannelCount;
}

int32 FRuntimeMeshVerticesAccessor::NumVertices() const
{
	check(bIsInitialized);
	int32 Count = PositionStream->Num() / PositionStride;
	return Count;
}

int32 FRuntimeMeshVerticesAccessor::NumUVChannels() const
{
	check(bIsInitialized);
	return UVChannelCount;
}

void FRuntimeMeshVerticesAccessor::EmptyVertices(int32 Slack /*= 0*/)
{
	check(bIsInitialized);
	PositionStream->Empty(Slack * PositionStride);
	TangentStream->Empty(Slack * TangentStride);
	UVStream->Empty(Slack * UVSize);
	ColorStream->Empty(Slack * ColorStride);
}

void FRuntimeMeshVerticesAccessor::SetNumVertices(int32 NewNum)
{
	check(bIsInitialized);
	PositionStream->SetNumZeroed(NewNum * PositionStride);
	TangentStream->SetNumZeroed(NewNum * TangentStride);
	UVStream->SetNumZeroed(NewNum * UVStride);
	ColorStream->SetNumZeroed(NewNum * ColorStride);
}

int32 FRuntimeMeshVerticesAccessor::AddVertex(FVector InPosition)
{
	check(bIsInitialized);
	int32 NewIndex = AddSingleVertex();

	SetPosition(NewIndex, InPosition);

	return NewIndex;
}

FVector FRuntimeMeshVerticesAccessor::GetPosition(int32 Index) const
{
	check(bIsInitialized);
	int32 StartPosition = Index * PositionStride;
	return *((FVector*)(&(*PositionStream)[StartPosition]));
}

FVector4 FRuntimeMeshVerticesAccessor::GetNormal(int32 Index) const
{
	check(bIsInitialized);
	int32 StartPosition = Index * TangentStride;
	if (bTangentHighPrecision)
	{
		return (*((FRuntimeMeshTangentsHighPrecision*)(&(*TangentStream)[StartPosition]))).Normal;
	}
	else
	{
		return (*((FRuntimeMeshTangents*)(&(*TangentStream)[StartPosition]))).Normal;
	}
}

FVector FRuntimeMeshVerticesAccessor::GetTangent(int32 Index) const
{
	check(bIsInitialized);
	int32 StartPosition = Index * TangentStride;
	if (bTangentHighPrecision)
	{
		return (*((FRuntimeMeshTangentsHighPrecision*)(&(*TangentStream)[StartPosition]))).Tangent;
	}
	else
	{
		return (*((FRuntimeMeshTangents*)(&(*TangentStream)[StartPosition]))).Tangent;
	}
}

FColor FRuntimeMeshVerticesAccessor::GetColor(int32 Index) const
{
	check(bIsInitialized);
	int32 StartPosition = Index * ColorStride;
	return *((FColor*)(&(*ColorStream)[StartPosition]));
}

FVector2D FRuntimeMeshVerticesAccessor::GetUV(int32 Index, int32 Channel) const
{
	check(bIsInitialized);
	check(Channel >= 0 && Channel < UVChannelCount);
	return bUVHighPrecision ?
		Read<FVector2D>(Index, UVStream, UVStride, UVSize * Channel) :
		FVector2D(Read<FVector2DHalf>(Index, UVStream, UVStride, UVSize * Channel));
}

void FRuntimeMeshVerticesAccessor::SetPosition(int32 Index, FVector Value)
{
	check(bIsInitialized);
	int32 StartPosition = Index * PositionStride;
	(*((FVector*)(&(*PositionStream)[StartPosition]))) = Value;
}

void FRuntimeMeshVerticesAccessor::SetNormal(int32 Index, const FVector4& Value)
{
	check(bIsInitialized);
	int32 StartPosition = Index * TangentStride;
	if (bTangentHighPrecision)
	{
		(*((FRuntimeMeshTangentsHighPrecision*)(&(*TangentStream)[StartPosition]))).Normal = Value;
	}
	else
	{
		(*((FRuntimeMeshTangents*)(&(*TangentStream)[StartPosition]))).Normal = Value;
	}
}

void FRuntimeMeshVerticesAccessor::SetTangent(int32 Index, FVector Value)
{
	check(bIsInitialized);
	int32 StartPosition = Index * TangentStride;
	if (bTangentHighPrecision)
	{
		(*((FRuntimeMeshTangentsHighPrecision*)(&(*TangentStream)[StartPosition]))).Tangent = Value;
	}
	else
	{
		(*((FRuntimeMeshTangents*)(&(*TangentStream)[StartPosition]))).Tangent = Value;
	}
}

void FRuntimeMeshVerticesAccessor::SetTangent(int32 Index, FRuntimeMeshTangent Value)
{
	check(bIsInitialized);
	int32 StartPosition = Index * TangentStride;
	if (bTangentHighPrecision)
	{
		FRuntimeMeshTangentsHighPrecision& Tangents = (*((FRuntimeMeshTangentsHighPrecision*)(&(*TangentStream)[StartPosition])));
		FVector4 NewNormal = Tangents.Normal;
		NewNormal.W = Value.bFlipTangentY ? -1.0f : 1.0f;
		Tangents.Normal = NewNormal;
		Tangents.Tangent = Value.TangentX;
	}
	else
	{
		FRuntimeMeshTangents& Tangents = (*((FRuntimeMeshTangents*)(&(*TangentStream)[StartPosition])));
		FVector4 NewNormal = Tangents.Normal;
		NewNormal.W = Value.bFlipTangentY ? -1.0f : 1.0f;
		Tangents.Normal = NewNormal;
		Tangents.Tangent = Value.TangentX;
	}
}

void FRuntimeMeshVerticesAccessor::SetColor(int32 Index, FColor Value)
{
	check(bIsInitialized);
	int32 StartPosition = Index * ColorStride;
	*((FColor*)(&(*ColorStream)[StartPosition])) = Value;
}

void FRuntimeMeshVerticesAccessor::SetUV(int32 Index, FVector2D Value)
{
	check(bIsInitialized);
	check(UVChannelCount > 0);
	if (bUVHighPrecision)
	{
		Write<FVector2D>(Index, Value, UVStream, UVStride, 0);
	}
	else
	{
		Write<FVector2DHalf>(Index, Value, UVStream, UVStride, 0);
	}
}

void FRuntimeMeshVerticesAccessor::SetUV(int32 Index, int32 Channel, FVector2D Value)
{
	check(bIsInitialized);
	check(Channel >= 0 && Channel < UVChannelCount);
	if (bUVHighPrecision)
	{
		Write<FVector2D>(Index, Value, UVStream, UVStride, UVSize * Channel);
	}
	else
	{
		Write<FVector2DHalf>(Index, Value, UVStream, UVStride, UVSize * Channel);
	}
}

void FRuntimeMeshVerticesAccessor::SetNormalTangent(int32 Index, FVector Normal, FRuntimeMeshTangent Tangent)
{
	check(bIsInitialized);
	int32 StartPosition = Index * TangentStride;
	if (bTangentHighPrecision)
	{
		FRuntimeMeshTangentsHighPrecision& Tangents = (*((FRuntimeMeshTangentsHighPrecision*)(&(*TangentStream)[StartPosition])));
		Tangents.Normal = FVector4(Normal, Tangent.bFlipTangentY ? -1 : 1);
		Tangents.Tangent = Tangent.TangentX;
	}
	else
	{
		FRuntimeMeshTangents& Tangents = (*((FRuntimeMeshTangents*)(&(*TangentStream)[StartPosition])));
		Tangents.Normal = FVector4(Normal, Tangent.bFlipTangentY ? -1 : 1);
		Tangents.Tangent = Tangent.TangentX;
	}
}

void FRuntimeMeshVerticesAccessor::SetTangents(int32 Index, FVector TangentX, FVector TangentY, FVector TangentZ)
{
	check(bIsInitialized);
	int32 StartPosition = Index * TangentStride;
	if (bTangentHighPrecision)
	{
		FRuntimeMeshTangentsHighPrecision& Tangents = (*((FRuntimeMeshTangentsHighPrecision*)(&(*TangentStream)[StartPosition])));
		Tangents.Normal = FVector4(TangentZ, GetBasisDeterminantSign(TangentX, TangentY, TangentZ));
		Tangents.Tangent = TangentX;
	}
	else
	{
		FRuntimeMeshTangents& Tangents = (*((FRuntimeMeshTangents*)(&(*TangentStream)[StartPosition])));
		Tangents.Normal = FVector4(TangentZ, GetBasisDeterminantSign(TangentX, TangentY, TangentZ));
		Tangents.Tangent = TangentX;
	}
}





FRuntimeMeshAccessorVertex FRuntimeMeshVerticesAccessor::GetVertex(int32 Index) const
{
	check(bIsInitialized);
	FRuntimeMeshAccessorVertex Vertex;
	Vertex.Position = Read<FVector>(Index, PositionStream, PositionStride, 0);
	Vertex.Normal = bTangentHighPrecision ?
		FVector4(Read<FPackedRGBA16N>(Index, TangentStream, TangentStride, 0)) :
		FVector4(Read<FPackedNormal>(Index, TangentStream, TangentStride, 0));
	Vertex.Tangent = bTangentHighPrecision ?
		FVector(Read<FPackedRGBA16N>(Index, TangentStream, TangentStride, TangentSize)) :
		FVector(Read<FPackedNormal>(Index, TangentStream, TangentStride, TangentSize));
	Vertex.Color = Read<FColor>(Index, ColorStream, ColorStride, 0);
	Vertex.UVs.SetNum(NumUVChannels());
	for (int32 UVIndex = 0; UVIndex < Vertex.UVs.Num(); UVIndex++)
	{
		Vertex.UVs[UVIndex] = bUVHighPrecision ?
			Read<FVector2D>(Index, UVStream, UVStride, UVSize * UVIndex) :
			FVector2D(Read<FVector2DHalf>(Index, UVStream, UVStride, UVSize * UVIndex));
	}
	return Vertex;
}

void FRuntimeMeshVerticesAccessor::SetVertex(int32 Index, const FRuntimeMeshAccessorVertex& Vertex)
{
	check(bIsInitialized);
	Write<FVector>(Index, Vertex.Position, PositionStream, PositionStride, 0);
	if (bTangentHighPrecision)
	{
		Write<FPackedRGBA16N>(Index, Vertex.Normal, TangentStream, TangentStride, 0);
		Write<FPackedRGBA16N>(Index, Vertex.Tangent, TangentStream, TangentStride, TangentSize);
	}
	else
	{
		Write<FPackedNormal>(Index, Vertex.Normal, TangentStream, TangentStride, 0);
		Write<FPackedNormal>(Index, Vertex.Tangent, TangentStream, TangentStride, TangentSize);
	}
	Write<FColor>(Index, Vertex.Color, ColorStream, ColorStride, 0);
	int32 NumUVs = NumUVChannels();
	for (int32 UVIndex = 0; UVIndex < NumUVs; UVIndex++)
	{
		if (bUVHighPrecision)
		{
			Write<FVector2D>(Index, Vertex.UVs[UVIndex], UVStream, UVStride, UVSize * UVIndex);
		}
		else
		{
			Write<FVector2DHalf>(Index, Vertex.UVs[UVIndex], UVStream, UVStride, UVSize * UVIndex);
		}
	}
}

int32 FRuntimeMeshVerticesAccessor::AddVertex(const FRuntimeMeshAccessorVertex& Vertex)
{
	check(bIsInitialized);
	int32 NewIndex = AddSingleVertex();
	SetVertex(NewIndex, Vertex);
	return NewIndex;
}



int32 FRuntimeMeshVerticesAccessor::AddSingleVertex()
{
	int32 NewIndex = NumVertices();

	PositionStream->AddZeroed(PositionStride);
	TangentStream->AddZeroed(TangentStride);
	UVStream->AddZeroed(UVStride);
	ColorStream->AddZeroed(ColorStride);

	return NewIndex;
}





//////////////////////////////////////////////////////////////////////////
//	FRuntimeMeshIndicesAccessor

FRuntimeMeshIndicesAccessor::FRuntimeMeshIndicesAccessor(TArray<uint8>* IndexStreamData)
	: bIsInitialized(false)
	, IndexStream(IndexStreamData), b32BitIndices(false)
{
}

FRuntimeMeshIndicesAccessor::FRuntimeMeshIndicesAccessor(bool bIn32BitIndices, TArray<uint8>* IndexStreamData)
	: bIsInitialized(false)
	, IndexStream(IndexStreamData), b32BitIndices(false)
{
	Initialize(bIn32BitIndices);
}

FRuntimeMeshIndicesAccessor::~FRuntimeMeshIndicesAccessor()
{
}

void FRuntimeMeshIndicesAccessor::Initialize(bool bIn32BitIndices)
{
	bIsInitialized = true;
	b32BitIndices = bIn32BitIndices;

	check((IndexStream->Num() % (b32BitIndices ? 4 : 2)) == 0);

	if (b32BitIndices)
	{
		IndexReader.BindStatic(&FRuntimeMeshAccessor::ReadIndex32, IndexStream);
		IndexWriter.BindStatic(&FRuntimeMeshAccessor::WriteIndex32, IndexStream);
	}
	else
	{
		IndexReader.BindStatic(&FRuntimeMeshAccessor::ReadIndex16, IndexStream);
		IndexWriter.BindStatic(&FRuntimeMeshAccessor::WriteIndex16, IndexStream);
	}
}

int32 FRuntimeMeshIndicesAccessor::NumIndices() const
{
	check(bIsInitialized);
	return IndexStream->Num() / GetIndexStride();
}

void FRuntimeMeshIndicesAccessor::EmptyIndices(int32 Slack)
{
	check(bIsInitialized);
	IndexStream->Empty(Slack * GetIndexStride());
}

void FRuntimeMeshIndicesAccessor::SetNumIndices(int32 NewNum)
{
	check(bIsInitialized);
	IndexStream->SetNumZeroed(NewNum * GetIndexStride());
}

int32 FRuntimeMeshIndicesAccessor::AddIndex(int32 NewIndex)
{
	check(bIsInitialized);
	int32 NewPosition = NumIndices();
	IndexStream->AddZeroed(GetIndexStride());
	SetIndex(NewPosition, NewIndex);
	return NewPosition;
}

int32 FRuntimeMeshIndicesAccessor::AddTriangle(int32 Index0, int32 Index1, int32 Index2)
{
	check(bIsInitialized);
	int32 NewPosition = NumIndices();
	IndexStream->AddZeroed(GetIndexStride() * 3);
	SetIndex(NewPosition + 0, Index0);
	SetIndex(NewPosition + 1, Index1);
	SetIndex(NewPosition + 2, Index2);
	return NewPosition;
}

int32 FRuntimeMeshIndicesAccessor::GetIndex(int32 Index) const
{
	check(bIsInitialized);
	return IndexReader.Execute(Index);
}

void FRuntimeMeshIndicesAccessor::SetIndex(int32 Index, int32 Value)
{
	check(bIsInitialized);
	IndexWriter.Execute(Index, Value);
}




//////////////////////////////////////////////////////////////////////////
//	FRuntimeMeshAccessor

FRuntimeMeshAccessor::FRuntimeMeshAccessor(bool bInTangentsHighPrecision, bool bInUVsHighPrecision, int32 bInUVCount, bool bIn32BitIndices, TArray<uint8>* PositionStreamData,
	TArray<uint8>* TangentStreamData, TArray<uint8>* UVStreamData, TArray<uint8>* ColorStreamData, TArray<uint8>* IndexStreamData)
	: FRuntimeMeshVerticesAccessor(bInTangentsHighPrecision, bInUVsHighPrecision, bInUVCount, PositionStreamData, TangentStreamData, UVStreamData, ColorStreamData)
	, FRuntimeMeshIndicesAccessor(bIn32BitIndices, IndexStreamData)
{

}

FRuntimeMeshAccessor::~FRuntimeMeshAccessor()
{
}

FRuntimeMeshAccessor::FRuntimeMeshAccessor(TArray<uint8>* PositionStreamData, TArray<uint8>* TangentStreamData, TArray<uint8>* UVStreamData, TArray<uint8>* ColorStreamData, TArray<uint8>* IndexStreamData)
	: FRuntimeMeshVerticesAccessor(PositionStreamData, TangentStreamData, UVStreamData, ColorStreamData)
	, FRuntimeMeshIndicesAccessor(IndexStreamData)
{

}

void FRuntimeMeshAccessor::Initialize(bool bInTangentsHighPrecision, bool bInUVsHighPrecision, int32 bInUVCount, bool bIn32BitIndices)
{
	FRuntimeMeshVerticesAccessor::Initialize(bInTangentsHighPrecision, bInUVsHighPrecision, bInUVCount);
	FRuntimeMeshIndicesAccessor::Initialize(bIn32BitIndices);
}




void FRuntimeMeshAccessor::CopyTo(const TSharedPtr<FRuntimeMeshAccessor>& Other, bool bClearDestination) const
{
	if (bClearDestination)
	{
		Other->EmptyVertices(NumVertices());
		Other->EmptyIndices(NumIndices());
	}

	int32 StartVertex = Other->NumVertices();
	int32 NumVerts = NumVertices();
	int32 NumUVs = FMath::Min(NumUVChannels(), Other->NumUVChannels());

	for (int32 Index = 0; Index < NumVerts; Index++)
	{
		int32 NewIndex = Other->AddVertex(GetPosition(Index));
		Other->SetNormal(NewIndex, GetNormal(Index));
		Other->SetTangent(NewIndex, GetTangent(Index));
		Other->SetColor(NewIndex, GetColor(Index));
		for (int32 UVIndex = 0; UVIndex < NumUVs; UVIndex++)
		{
			Other->SetUV(NewIndex, UVIndex, GetUV(Index, UVIndex));
		}
	}

	int32 NumInds = NumIndices();
	for (int32 Index = 0; Index < NumInds; Index++)
	{
		Other->AddIndex(GetIndex(Index) + StartVertex);
	}
}

//////////////////////////////////////////////////////////////////////////
//	FRuntimeMeshBuilder

FRuntimeMeshBuilder::FRuntimeMeshBuilder(bool bInTangentsHighPrecision, bool bInUVsHighPrecision, int32 bInUVCount, bool bIn32BitIndices)
	: FRuntimeMeshAccessor(&PositionStream, &TangentStream, &UVStream, &ColorStream, &IndexStream)
{
	FRuntimeMeshAccessor::Initialize(bInTangentsHighPrecision, bInUVsHighPrecision, bInUVCount, bIn32BitIndices);
}

FRuntimeMeshBuilder::~FRuntimeMeshBuilder()
{

}



