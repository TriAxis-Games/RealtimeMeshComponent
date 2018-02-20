// Copyright 2016-2018 Chris Conway (Koderz). All Rights Reserved.

#include "RuntimeMeshComponentPlugin.h"
#include "RuntimeMeshBuilder.h"

//////////////////////////////////////////////////////////////////////////
//	FRuntimeMeshVerticesAccessor

FRuntimeMeshVerticesAccessor::FRuntimeMeshVerticesAccessor(TArray<uint8>* Stream0Data, TArray<uint8>* Stream1Data,
	TArray<uint8>* Stream2Data)
	: bIsInitialized(false)
	, Stream0(Stream0Data), Stream0Stride(0)
	, Stream1(Stream1Data), Stream1Stride(0)
	, Stream2(Stream2Data), Stream2Stride(0)
{
}

FRuntimeMeshVerticesAccessor::FRuntimeMeshVerticesAccessor(const FRuntimeMeshVertexStreamStructure& Stream0Structure,
	const FRuntimeMeshVertexStreamStructure& Stream1Structure,
	const FRuntimeMeshVertexStreamStructure& Stream2Structure,
	TArray<uint8>* Stream0Data, TArray<uint8>* Stream1Data, TArray<uint8>* Stream2Data)
	: bIsInitialized(false)
	, Stream0(Stream0Data), Stream0Stride(0)
	, Stream1(Stream1Data), Stream1Stride(0)
	, Stream2(Stream2Data), Stream2Stride(0)
{
	Initialize(Stream0Structure, Stream1Structure, Stream2Structure);
}

FRuntimeMeshVerticesAccessor::~FRuntimeMeshVerticesAccessor()
{
}

void FRuntimeMeshVerticesAccessor::Initialize(const FRuntimeMeshVertexStreamStructure& InStream0Structure,
	const FRuntimeMeshVertexStreamStructure& InStream1Structure,
	const FRuntimeMeshVertexStreamStructure& InStream2Structure)
{
	bIsInitialized = true;
	Stream0Structure = InStream0Structure;
	Stream0Stride = Stream0Structure.CalculateStride();
	Stream1Structure = InStream1Structure;
	Stream1Stride = Stream1Structure.CalculateStride();
	Stream2Structure = InStream2Structure;
	Stream2Stride = Stream2Structure.CalculateStride();

	// Verify all streams have the same number of elements if they're enabled
	check((Stream0Stride != 0) == Stream0Structure.HasAnyElements());
	check((Stream1Stride != 0) == Stream1Structure.HasAnyElements());
	check((Stream2Stride != 0) == Stream2Structure.HasAnyElements());

	// Verify that all streams are sized correctly for their strides
	check(Stream0Stride == 0 || ((Stream0->Num() % Stream0Stride) == 0));
	check(Stream1Stride == 0 || ((Stream1->Num() % Stream1Stride) == 0));
	check(Stream2Stride == 0 || ((Stream2->Num() % Stream2Stride) == 0));

	// The position is required to be in stream0
	check(Stream0Structure.Position.IsValid());
	PositionReader.BindStatic(&FRuntimeMeshAccessor::Read<FVector>, Stream0, (int32)Stream0Structure.Position.Stride, (int32)Stream0Structure.Position.Offset);
	PositionWriter.BindStatic(&FRuntimeMeshAccessor::Write<FVector>, Stream0, (int32)Stream0Structure.Position.Stride, (int32)Stream0Structure.Position.Offset);

	const auto BindNormalTangentReaderWriter = [](FStreamNormalTangentReader& Reader, FStreamNormalTangentWriter& Writer, const FRuntimeMeshVertexStreamStructureElement& Element, TArray<uint8>* Data)
	{
		Reader.BindStatic(Element.Type == VET_PackedNormal ?
			&FRuntimeMeshAccessor::ReadNormalTangentPackedNormal :
			&FRuntimeMeshAccessor::ReadNormalTangentPackedRGBA16N,
			Data, (int32)Element.Stride, (int32)Element.Offset);

		Writer.BindStatic(Element.Type == VET_PackedNormal ?
			&FRuntimeMeshAccessor::WriteNormalTangentPackedNormal :
			&FRuntimeMeshAccessor::WriteNormalTangentPackedRGBA16N,
			Data, (int32)Element.Stride, (int32)Element.Offset);
	};

	// Bind normal reader
	if (Stream0Structure.Normal.IsValid()) BindNormalTangentReaderWriter(NormalReader, NormalWriter, Stream0Structure.Normal, Stream0);
	else if (Stream1Structure.Normal.IsValid()) BindNormalTangentReaderWriter(NormalReader, NormalWriter, Stream1Structure.Normal, Stream1);
	else if (Stream2Structure.Normal.IsValid()) BindNormalTangentReaderWriter(NormalReader, NormalWriter, Stream2Structure.Normal, Stream2);
	else
	{
		NormalReader.BindStatic(&FRuntimeMeshAccessor::ReadNormalNull, (TArray<uint8>*)nullptr, 0, 0);
		NormalWriter.BindStatic(&FRuntimeMeshAccessor::WriteNormalNull, (TArray<uint8>*)nullptr, 0, 0);
	}

	// Bind tangent reader
	if (Stream0Structure.Tangent.IsValid()) BindNormalTangentReaderWriter(TangentReader, TangentWriter, Stream0Structure.Tangent, Stream0);
	else if (Stream1Structure.Tangent.IsValid()) BindNormalTangentReaderWriter(TangentReader, TangentWriter, Stream1Structure.Tangent, Stream1);
	else if (Stream2Structure.Tangent.IsValid()) BindNormalTangentReaderWriter(TangentReader, TangentWriter, Stream2Structure.Tangent, Stream2);
	else
	{
		TangentReader.BindStatic(&FRuntimeMeshAccessor::ReadTangentNull, (TArray<uint8>*)nullptr, 0, 0);
		TangentWriter.BindStatic(&FRuntimeMeshAccessor::WriteTangentNull, (TArray<uint8>*)nullptr, 0, 0);
	}

	// Bind color reader
	if (Stream0Structure.Color.IsValid())
	{
		ColorReader.BindStatic(&FRuntimeMeshAccessor::Read<FColor>, Stream0, (int32)Stream0Structure.Color.Stride, (int32)Stream0Structure.Color.Offset);
		ColorWriter.BindStatic(&FRuntimeMeshAccessor::Write<FColor>, Stream0, (int32)Stream0Structure.Color.Stride, (int32)Stream0Structure.Color.Offset);
	}
	else if (Stream1Structure.Color.IsValid())
	{
		ColorReader.BindStatic(&FRuntimeMeshAccessor::Read<FColor>, Stream1, (int32)Stream1Structure.Color.Stride, (int32)Stream1Structure.Color.Offset);
		ColorWriter.BindStatic(&FRuntimeMeshAccessor::Write<FColor>, Stream1, (int32)Stream1Structure.Color.Stride, (int32)Stream1Structure.Color.Offset);
	}
	else if (Stream2Structure.Color.IsValid())
	{
		ColorReader.BindStatic(&FRuntimeMeshAccessor::Read<FColor>, Stream2, (int32)Stream2Structure.Color.Stride, (int32)Stream2Structure.Color.Offset);
		ColorWriter.BindStatic(&FRuntimeMeshAccessor::Write<FColor>, Stream2, (int32)Stream2Structure.Color.Stride, (int32)Stream2Structure.Color.Offset);
	}
	else
	{
		ColorReader.BindStatic(&FRuntimeMeshAccessor::ReadColorNull, (TArray<uint8>*)nullptr, 0, 0);
	}

	const auto BindUVs = [&](const FRuntimeMeshVertexStreamStructure& Structure, TArray<uint8>* Data)
	{
		const auto IsHighPrecision = [](EVertexElementType Type) -> bool
		{
			if (Type == VET_Float2 || Type == VET_Float4)
				return true;

			check(Type == VET_Half2 || Type == VET_Half4);
			return false;
		};

		const auto IsDualUV = [](EVertexElementType Type) -> bool
		{
			if (Type == VET_Float4 || Type == VET_Half4)
				return true;

			check(Type == VET_Float2 || Type == VET_Half2);
			return false;
		};

		for (int32 Index = 0; Index < Structure.UVs.Num(); Index++)
		{
			bool bIsHighPrecision = IsHighPrecision(Structure.UVs[Index].Type);

			UVReaders.Add(FStreamUVReader::CreateStatic(
				bIsHighPrecision ? FRuntimeMeshAccessor::ReadUVVector2D : FRuntimeMeshAccessor::ReadUVVector2DHalf,
				Data, (int32)Structure.UVs[Index].Stride, (int32)Structure.UVs[Index].Offset));

			UVWriters.Add(FStreamUVWriter::CreateStatic(
				bIsHighPrecision ? FRuntimeMeshAccessor::WriteUVVector2D : FRuntimeMeshAccessor::WriteUVVector2DHalf,
				Data, (int32)Structure.UVs[Index].Stride, (int32)Structure.UVs[Index].Offset));

			if (IsDualUV(Structure.UVs[Index].Type))
			{
				const int32 SecondOffset = bIsHighPrecision ? 8 : 4;

				UVReaders.Add(FStreamUVReader::CreateStatic(
					bIsHighPrecision ? FRuntimeMeshAccessor::ReadUVVector2D : FRuntimeMeshAccessor::ReadUVVector2DHalf,
					Data, (int32)Structure.UVs[Index].Stride, (int32)Structure.UVs[Index].Offset + SecondOffset));

				UVWriters.Add(FStreamUVWriter::CreateStatic(
					bIsHighPrecision ? FRuntimeMeshAccessor::WriteUVVector2D : FRuntimeMeshAccessor::WriteUVVector2DHalf,
					Data, (int32)Structure.UVs[Index].Stride, (int32)Structure.UVs[Index].Offset + SecondOffset));
			}
		}
	};

	if (Stream0Structure.HasUVs()) BindUVs(Stream0Structure, Stream0);
	else if (Stream1Structure.HasUVs()) BindUVs(Stream1Structure, Stream1);
	else if (Stream2Structure.HasUVs()) BindUVs(Stream2Structure, Stream2);
}

int32 FRuntimeMeshVerticesAccessor::NumVertices() const
{
	check(bIsInitialized);
	int32 Count = Stream0->Num() / Stream0Stride;
	check(Stream1Stride <= 0 || (Stream1->Num() / Stream1Stride) == Count);
	check(Stream2Stride <= 0 || (Stream2->Num() / Stream2Stride) == Count);
	return Count;
}

int32 FRuntimeMeshVerticesAccessor::NumUVChannels() const
{
	check(bIsInitialized);
	check(UVWriters.Num() == UVReaders.Num());
	return UVWriters.Num();
}

void FRuntimeMeshVerticesAccessor::EmptyVertices(int32 Slack /*= 0*/)
{
	check(bIsInitialized);
	check(Stream0Stride > 0);
	Stream0->Empty(Slack * Stream0Stride);
	if (Stream1Stride > 0)
	{
		Stream1->Empty(Slack * Stream1Stride);
	}
	else
	{
		check(Stream1->Num() == 0);
	}
	if (Stream2Stride > 0)
	{
		Stream2->Empty(Slack * Stream2Stride);
	}
	else
	{
		check(Stream2->Num() == 0);
	}
}

void FRuntimeMeshVerticesAccessor::SetNumVertices(int32 NewNum)
{
	check(bIsInitialized);
	check(Stream0Stride > 0);
	Stream0->SetNumZeroed(NewNum * Stream0Stride);
	if (Stream1Stride > 0)
	{
		Stream1->SetNumZeroed(NewNum * Stream1Stride);
	}
	else
	{
		check(Stream1->Num() == 0);
	}
	if (Stream2Stride > 0)
	{
		Stream2->SetNumZeroed(NewNum * Stream2Stride);
	}
	else
	{
		check(Stream2->Num() == 0);
	}
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
	return PositionReader.Execute(Index);
}

FVector4 FRuntimeMeshVerticesAccessor::GetNormal(int32 Index) const
{
	check(bIsInitialized);
	return NormalReader.Execute(Index);
}

FVector FRuntimeMeshVerticesAccessor::GetTangent(int32 Index) const
{
	check(bIsInitialized);
	return TangentReader.Execute(Index);
}

FColor FRuntimeMeshVerticesAccessor::GetColor(int32 Index) const
{
	check(bIsInitialized);
	return ColorReader.Execute(Index);
}

FVector2D FRuntimeMeshVerticesAccessor::GetUV(int32 Index, int32 Channel) const
{
	check(bIsInitialized);
	check(Channel >= 0 && Channel < UVReaders.Num());
	return UVReaders[Channel].Execute(Index);
}

void FRuntimeMeshVerticesAccessor::SetPosition(int32 Index, FVector Value)
{
	check(bIsInitialized);
	PositionWriter.Execute(Index, Value);
}

void FRuntimeMeshVerticesAccessor::SetNormal(int32 Index, FVector4 Value)
{
	check(bIsInitialized);
	NormalWriter.Execute(Index, Value);
}

void FRuntimeMeshVerticesAccessor::SetTangent(int32 Index, FVector Value)
{
	check(bIsInitialized);
	TangentWriter.Execute(Index, Value);
}

void FRuntimeMeshVerticesAccessor::SetTangent(int32 Index, FRuntimeMeshTangent Value)
{
	FVector4 Normal = GetNormal(Index);
	Normal.W = Value.bFlipTangentY ? -1.0f : 1.0f;
	SetNormal(Index, Normal);
	SetTangent(Index, Value.TangentX);
}

void FRuntimeMeshVerticesAccessor::SetColor(int32 Index, FColor Value)
{
	check(bIsInitialized);
	ColorWriter.Execute(Index, Value);
}

void FRuntimeMeshVerticesAccessor::SetUV(int32 Index, FVector2D Value)
{
	check(bIsInitialized);
	check(UVWriters.Num());
	UVWriters[0].Execute(Index, Value);
}

void FRuntimeMeshVerticesAccessor::SetUV(int32 Index, int32 Channel, FVector2D Value)
{
	check(bIsInitialized);
	check(Channel >= 0 && Channel < UVWriters.Num());
	UVWriters[Channel].Execute(Index, Value);
}

void FRuntimeMeshVerticesAccessor::SetNormalTangent(int32 Index, FVector Normal, FRuntimeMeshTangent Tangent)
{
	check(bIsInitialized);
	NormalWriter.Execute(Index, FVector4(Normal, Tangent.bFlipTangentY? -1 : 1));
	TangentWriter.Execute(Index, Tangent.TangentX);
}

void FRuntimeMeshVerticesAccessor::SetTangents(int32 Index, FVector TangentX, FVector TangentY, FVector TangentZ)
{
	check(bIsInitialized);
	NormalWriter.Execute(Index, FVector4(TangentZ, GetBasisDeterminantSign(TangentX, TangentY, TangentZ)));
	TangentWriter.Execute(Index, TangentX);
}





FRuntimeMeshAccessorVertex FRuntimeMeshVerticesAccessor::GetVertex(int32 Index) const
{
	check(bIsInitialized);
	FRuntimeMeshAccessorVertex Vertex;
	Vertex.Position = PositionReader.Execute(Index);
	Vertex.Normal = NormalReader.Execute(Index);
	Vertex.Tangent = TangentReader.Execute(Index);
	Vertex.Color = ColorReader.Execute(Index);
	Vertex.UVs.SetNum(NumUVChannels());
	for (int32 UVIndex = 0; UVIndex < Vertex.UVs.Num(); UVIndex++)
	{
		Vertex.UVs[UVIndex] = UVReaders[UVIndex].Execute(Index);
	}
	return Vertex;
}

void FRuntimeMeshVerticesAccessor::SetVertex(int32 Index, const FRuntimeMeshAccessorVertex& Vertex)
{
	check(bIsInitialized);
	PositionWriter.Execute(Index, Vertex.Position);
	NormalWriter.Execute(Index, Vertex.Normal);
	TangentWriter.Execute(Index, Vertex.Tangent);
	ColorWriter.Execute(Index, Vertex.Color);
	int32 NumUVs = NumUVChannels();
	for (int32 UVIndex = 0; UVIndex < NumUVs; UVIndex++)
	{
		UVWriters[UVIndex].Execute(Index, Vertex.UVs[UVIndex]);
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

	check(Stream0Stride > 0);
	Stream0->AddZeroed(Stream0Stride);
	if (Stream1Stride > 0)
	{
		Stream1->AddZeroed(Stream1Stride);
	}
	else
	{
		check(Stream1->Num() == 0);
	}
	if (Stream2Stride > 0)
	{
		Stream2->AddZeroed(Stream2Stride);
	}
	else
	{
		check(Stream2->Num() == 0);
	}

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

FRuntimeMeshAccessor::FRuntimeMeshAccessor(const FRuntimeMeshVertexStreamStructure& Stream0, const FRuntimeMeshVertexStreamStructure& Stream1, const FRuntimeMeshVertexStreamStructure& Stream2,
	bool bIn32BitIndices, TArray<uint8>* Stream0Data, TArray<uint8>* Stream1Data, TArray<uint8>* Stream2Data, TArray<uint8>* IndexStreamData)
	: FRuntimeMeshVerticesAccessor(Stream0, Stream1, Stream2, Stream0Data, Stream1Data, Stream2Data)
	, FRuntimeMeshIndicesAccessor(bIn32BitIndices, IndexStreamData)
{

}

FRuntimeMeshAccessor::~FRuntimeMeshAccessor()
{
}

FRuntimeMeshAccessor::FRuntimeMeshAccessor(TArray<uint8>* Stream0Data, TArray<uint8>* Stream1Data, TArray<uint8>* Stream2Data, TArray<uint8>* IndexStreamData)
	: FRuntimeMeshVerticesAccessor(Stream0Data, Stream1Data, Stream2Data)
	, FRuntimeMeshIndicesAccessor(IndexStreamData)
{

}

void FRuntimeMeshAccessor::Initialize(const FRuntimeMeshVertexStreamStructure& InStream0Structure, const FRuntimeMeshVertexStreamStructure& InStream1Structure, const FRuntimeMeshVertexStreamStructure& InStream2Structure, bool bIn32BitIndices)
{
	FRuntimeMeshVerticesAccessor::Initialize(InStream0Structure, InStream1Structure, InStream2Structure);
	FRuntimeMeshIndicesAccessor::Initialize(bIn32BitIndices);
}




//////////////////////////////////////////////////////////////////////////
//	FRuntimeMeshBuilder

FRuntimeMeshBuilder::FRuntimeMeshBuilder(const FRuntimeMeshVertexStreamStructure& InStream0Structure,
	const FRuntimeMeshVertexStreamStructure& InStream1Structure, const FRuntimeMeshVertexStreamStructure& InStream2Structure, bool bIn32BitIndices)
	: FRuntimeMeshAccessor(&Stream0, &Stream1, &Stream2, &IndexStream)
{
	FRuntimeMeshAccessor::Initialize(InStream0Structure, InStream1Structure, InStream2Structure, bIn32BitIndices);
}

FRuntimeMeshBuilder::~FRuntimeMeshBuilder()
{

}



