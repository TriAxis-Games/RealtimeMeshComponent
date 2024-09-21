// Copyright (c) 2015-2024 TriAxis Games, L.L.C. All Rights Reserved.


#include "Mesh/RealtimeMeshBlueprintMeshBuilder.h"
#include "Engine/Engine.h"
#include "RealtimeMeshComponentModule.h"
#include "Core/RealtimeMeshDataStream.h"
#if RMC_ENGINE_ABOVE_5_2
#include "Logging/MessageLog.h"
#endif

#define LOCTEXT_NAMESPACE "RealtimeMesh"

#define RMC_RATE_LIMIT_LOG(LogLine) \
	static int32 CountTillNext = 0; \
	if (CountTillNext <= 0) \
	{ \
		LogLine \
		CountTillNext = 1000; \
	} 

static TAutoConsoleVariable<int32> CVarRealtimeMeshStreamPoolMaxSizeThreshold(
	TEXT("RealtimeMesh.StreamPool.MaxPoolSize"),
	256,
	TEXT("Maximum number of streams a URealtimeMeshStreamPool will allow to be in the pool before running garbage collection"));

static TAutoConsoleVariable<int32> CVarRealtimeMeshStreamSetsPoolMaxSizeThreshold(
	TEXT("RealtimeMesh.StreamSetPool.MaxPoolSize"),
	64,
	TEXT("Maximum number of streamsets a URealtimeMeshStreamPool will allow to be in the pool before running garbage collection"));

static TAutoConsoleVariable<int32> CVarRealtimeMeshBuilderPoolMaxSizeThreshold(
	TEXT("RealtimeMesh.Builder.MaxPoolSize"),
	16,
	TEXT("Maximum number of builders a URealtimeMeshStreamPool will allow to be in the pool before running garbage collection"));


static RealtimeMesh::FRealtimeMeshBufferLayout GetBufferLayout(ERealtimeMeshSimpleStreamType StreamType, int32 NumElements)
{
	switch(StreamType)
	{
	case ERealtimeMeshSimpleStreamType::Int16:
		return RealtimeMesh::GetRealtimeMeshBufferLayout<int16>(NumElements);
	case ERealtimeMeshSimpleStreamType::UInt16:
		return RealtimeMesh::GetRealtimeMeshBufferLayout<uint16>(NumElements);
	case ERealtimeMeshSimpleStreamType::Int32:
		return RealtimeMesh::GetRealtimeMeshBufferLayout<int32>(NumElements);
	case ERealtimeMeshSimpleStreamType::UInt32:
		return RealtimeMesh::GetRealtimeMeshBufferLayout<uint32>(NumElements);
	case ERealtimeMeshSimpleStreamType::Float:
		return RealtimeMesh::GetRealtimeMeshBufferLayout<float>(NumElements);
	case ERealtimeMeshSimpleStreamType::Vector2:
		return RealtimeMesh::GetRealtimeMeshBufferLayout<FVector2f>(NumElements);
	case ERealtimeMeshSimpleStreamType::Vector3:
		return RealtimeMesh::GetRealtimeMeshBufferLayout<FVector3f>(NumElements);
	case ERealtimeMeshSimpleStreamType::HalfVector2:
		return RealtimeMesh::GetRealtimeMeshBufferLayout<FVector2DHalf>(NumElements);
	case ERealtimeMeshSimpleStreamType::PackedNormal:
		return RealtimeMesh::GetRealtimeMeshBufferLayout<FPackedNormal>(NumElements);
	case ERealtimeMeshSimpleStreamType::PackedRGBA16N:
		return RealtimeMesh::GetRealtimeMeshBufferLayout<FPackedRGBA16N>(NumElements);
	case ERealtimeMeshSimpleStreamType::Triangle16:
		ensure(NumElements == 1);
		return RealtimeMesh::GetRealtimeMeshBufferLayout<RealtimeMesh::TIndex3<uint16>>();
	case ERealtimeMeshSimpleStreamType::Triangle32:
		ensure(NumElements == 1);
		return RealtimeMesh::GetRealtimeMeshBufferLayout<RealtimeMesh::TIndex3<uint32>>();
	default:

		return RealtimeMesh::FRealtimeMeshBufferLayout();
	}
}



bool FRealtimeMeshStreamRowPtr::IsValid() const
{
	return ::IsValid(Stream) && Stream->GetStream().IsValidIndex(RowIndex);
}

void URealtimeMeshStream::ClearAccessors()
{
	IntAccessors.Reset();
	FloatAccessors.Reset();
	Vector2Accessors.Reset();
	Vector3Accessors.Reset();
	Vector4Accessors.Reset();
}

void URealtimeMeshStream::SetupIntAccessors()
{
	for (int32 Index = 0; Index < Stream->GetNumElements(); Index++)
	{
		IntAccessors.Add(RealtimeMesh::TRealtimeMeshStridedStreamBuilder<int32, void>(*Stream, Index));
	}
}

void URealtimeMeshStream::SetupFloatAccessors()
{
	for (int32 Index = 0; Index < Stream->GetNumElements(); Index++)
	{
		FloatAccessors.Add(RealtimeMesh::TRealtimeMeshStridedStreamBuilder<float, void>(*Stream, Index));
	}
}

void URealtimeMeshStream::SetupVector2Accessors()
{
	for (int32 Index = 0; Index < Stream->GetNumElements(); Index++)
	{
		Vector2Accessors.Add(RealtimeMesh::TRealtimeMeshStridedStreamBuilder<FVector2D, void>(*Stream, Index));
	}
}

void URealtimeMeshStream::SetupVector3Accessors()
{
	for (int32 Index = 0; Index < Stream->GetNumElements(); Index++)
	{
		Vector3Accessors.Add(RealtimeMesh::TRealtimeMeshStridedStreamBuilder<FVector, void>(*Stream, Index));
	}
}

void URealtimeMeshStream::SetupVector4Accessors()
{
	for (int32 Index = 0; Index < Stream->GetNumElements(); Index++)
	{
		Vector4Accessors.Add(RealtimeMesh::TRealtimeMeshStridedStreamBuilder<FVector4, void>(*Stream, Index));
	}
}


URealtimeMeshStreamSet::URealtimeMeshStreamSet()
{
	Streams = MakeUnique<RealtimeMesh::FRealtimeMeshStreamSet>();
}

RealtimeMesh::FRealtimeMeshStream URealtimeMeshStream::Consume()
{
	RealtimeMesh::FRealtimeMeshStream Temp = RealtimeMesh::FRealtimeMeshStream(MoveTemp(*Stream));
	Stream.Reset();
	return RealtimeMesh::FRealtimeMeshStream(MoveTemp(Temp));
}

void URealtimeMeshStream::Initialize(const FRealtimeMeshStreamKey& StreamKey, ERealtimeMeshSimpleStreamType StreamType, int32 NumElements)
{
	switch(StreamType)
	{
	case ERealtimeMeshSimpleStreamType::Int16:
		Stream = MakeUnique<RealtimeMesh::FRealtimeMeshStream>(StreamKey, RealtimeMesh::GetRealtimeMeshBufferLayout<int16>(NumElements));
		SetupIntAccessors();
		break;
	case ERealtimeMeshSimpleStreamType::UInt16:
		Stream = MakeUnique<RealtimeMesh::FRealtimeMeshStream>(StreamKey, RealtimeMesh::GetRealtimeMeshBufferLayout<uint16>(NumElements));
		SetupIntAccessors();
		break;
	case ERealtimeMeshSimpleStreamType::Int32:
		Stream = MakeUnique<RealtimeMesh::FRealtimeMeshStream>(StreamKey, RealtimeMesh::GetRealtimeMeshBufferLayout<int32>(NumElements));
		SetupIntAccessors();
		break;
	case ERealtimeMeshSimpleStreamType::UInt32:
		Stream = MakeUnique<RealtimeMesh::FRealtimeMeshStream>(StreamKey, RealtimeMesh::GetRealtimeMeshBufferLayout<uint32>(NumElements));
		SetupIntAccessors();
		break;
	case ERealtimeMeshSimpleStreamType::Float:
		Stream = MakeUnique<RealtimeMesh::FRealtimeMeshStream>(StreamKey, RealtimeMesh::GetRealtimeMeshBufferLayout<float>(NumElements));
		SetupFloatAccessors();
		break;
	case ERealtimeMeshSimpleStreamType::Vector2:
		Stream = MakeUnique<RealtimeMesh::FRealtimeMeshStream>(StreamKey, RealtimeMesh::GetRealtimeMeshBufferLayout<FVector2f>(NumElements));
		SetupVector2Accessors();
		break;
	case ERealtimeMeshSimpleStreamType::Vector3:
		Stream = MakeUnique<RealtimeMesh::FRealtimeMeshStream>(StreamKey, RealtimeMesh::GetRealtimeMeshBufferLayout<FVector3f>(NumElements));
		SetupVector3Accessors();
		break;
	case ERealtimeMeshSimpleStreamType::HalfVector2:
		Stream = MakeUnique<RealtimeMesh::FRealtimeMeshStream>(StreamKey, RealtimeMesh::GetRealtimeMeshBufferLayout<FVector2DHalf>(NumElements));
		SetupVector2Accessors();
		break;
	case ERealtimeMeshSimpleStreamType::PackedNormal:
		Stream = MakeUnique<RealtimeMesh::FRealtimeMeshStream>(StreamKey, RealtimeMesh::GetRealtimeMeshBufferLayout<FPackedNormal>(NumElements));
		SetupVector4Accessors();
		break;
	case ERealtimeMeshSimpleStreamType::PackedRGBA16N:
		Stream = MakeUnique<RealtimeMesh::FRealtimeMeshStream>(StreamKey, RealtimeMesh::GetRealtimeMeshBufferLayout<FPackedRGBA16N>(NumElements));
		SetupVector4Accessors();
		break;
	case ERealtimeMeshSimpleStreamType::Triangle16:
		ensure(NumElements == 1);
		Stream = MakeUnique<RealtimeMesh::FRealtimeMeshStream>(StreamKey, RealtimeMesh::GetRealtimeMeshBufferLayout<RealtimeMesh::TIndex3<uint16>>());
		SetupVector4Accessors();
		break;
	case ERealtimeMeshSimpleStreamType::Triangle32:
		ensure(NumElements == 1);
		Stream = MakeUnique<RealtimeMesh::FRealtimeMeshStream>(StreamKey, RealtimeMesh::GetRealtimeMeshBufferLayout<RealtimeMesh::TIndex3<uint32>>());
		SetupVector4Accessors();
		break;
	default:
		Stream.Reset();
		ClearAccessors();
		break;
	}
}

int32 URealtimeMeshStream::GetNum(URealtimeMeshStream*& Builder)
{
	Builder = this;
	if (Stream.IsValid())
	{
		return Stream->Num();
	}
	return 0;
}

bool URealtimeMeshStream::IsIndexValid(URealtimeMeshStream*& Builder, int32 Index)
{
	Builder = this;
	if (Stream.IsValid())
	{
		return Stream->IsValidIndex(Index);
	}
	return false;
}

bool URealtimeMeshStream::IsEmpty(URealtimeMeshStream*& Builder)
{
	Builder = this;
	if (Stream.IsValid())
	{
		return Stream->IsEmpty();
	}
	return true;
}

void URealtimeMeshStream::Reserve(URealtimeMeshStream*& Builder, int32 ExpectedSize)
{
	Builder = this;
	if (Stream.IsValid())
	{
		Stream->Reserve(ExpectedSize);
	}
}

void URealtimeMeshStream::Shrink(URealtimeMeshStream*& Builder)
{
	Builder = this;
	if (Stream.IsValid())
	{
		Stream->Shrink();
	}
}

void URealtimeMeshStream::Empty(URealtimeMeshStream*& Builder, int32 ExpectedSize)
{
	Builder = this;
	if (Stream.IsValid())
	{
		Stream->Empty();
	}
}

void URealtimeMeshStream::SetNumUninitialized(URealtimeMeshStream*& Builder, int32 NewNum)
{
	Builder = this;
	if (Stream.IsValid())
	{
		Stream->SetNumUninitialized(NewNum);
	}
}

void URealtimeMeshStream::SetNumZeroed(URealtimeMeshStream*& Builder, int32 NewNum)
{
	Builder = this;
	if (Stream.IsValid())
	{
		Stream->SetNumZeroed(NewNum);
	}
}

int32 URealtimeMeshStream::AddUninitialized(URealtimeMeshStream*& Builder, FRealtimeMeshStreamRowPtr& Row, int32 NumToAdd)
{
	Builder = this;
	if (Stream.IsValid())
	{
		const int32 Index = Stream->AddUninitialized(NumToAdd);
		Row = FRealtimeMeshStreamRowPtr(this, Index);
		return Index;
	}
	return INDEX_NONE;
}

int32 URealtimeMeshStream::AddZeroed(URealtimeMeshStream*& Builder, FRealtimeMeshStreamRowPtr& Row, int32 NumToAdd)
{
	Builder = this;
	if (Stream.IsValid())
	{
		const int32 Index = Stream->AddZeroed(NumToAdd);
		Row = FRealtimeMeshStreamRowPtr(this, Index);
		return Index;
	}
	return INDEX_NONE;
}

FRealtimeMeshStreamRowPtr URealtimeMeshStream::EditRow(URealtimeMeshStream*& Builder, int32 Index)
{
	Builder = this;
	if (Stream.IsValid() && Stream->IsValidIndex(Index))
	{
		return FRealtimeMeshStreamRowPtr(this, Index);
	}
	return FRealtimeMeshStreamRowPtr();
}

int32 URealtimeMeshStream::AddInt(URealtimeMeshStream*& Builder, FRealtimeMeshStreamRowPtr& Row, int32 NewValue)
{
	Builder = this;
	if (Stream.IsValid() && IntAccessors.Num() >= 1)
	{
		const int32 Index = IntAccessors[0].Add(NewValue);
		Row = FRealtimeMeshStreamRowPtr(this, Index);
		return Index;
	}
	return INDEX_NONE;
}

int32 URealtimeMeshStream::AddFloat(URealtimeMeshStream*& Builder, FRealtimeMeshStreamRowPtr& Row, float NewValue)
{
	Builder = this;
	if (Stream.IsValid() && FloatAccessors.Num() >= 1)
	{
		const int32 Index = FloatAccessors[0].Add(NewValue);
		Row = FRealtimeMeshStreamRowPtr(this, Index);
		return Index;
	}
	return INDEX_NONE;
}

int32 URealtimeMeshStream::AddVector2(URealtimeMeshStream*& Builder, FRealtimeMeshStreamRowPtr& Row, FVector2D NewValue)
{
	Builder = this;
	if (Stream.IsValid() && Vector2Accessors.Num() >= 1)
	{
		const int32 Index = Vector2Accessors[0].Add(NewValue);
		Row = FRealtimeMeshStreamRowPtr(this, Index);
		return Index;
	}
	return INDEX_NONE;
}

int32 URealtimeMeshStream::AddVector3(URealtimeMeshStream*& Builder, FRealtimeMeshStreamRowPtr& Row, FVector NewValue)
{
	Builder = this;
	if (Stream.IsValid() && Vector3Accessors.Num() >= 1)
	{
		const int32 Index = Vector3Accessors[0].Add(NewValue);
		Row = FRealtimeMeshStreamRowPtr(this, Index);
		return Index;
	}
	return INDEX_NONE;
}

int32 URealtimeMeshStream::AddVector4(URealtimeMeshStream*& Builder, FRealtimeMeshStreamRowPtr& Row, FVector4 NewValue)
{
	Builder = this;
	if (Stream.IsValid() && Vector4Accessors.Num() >= 1)
	{
		const int32 Index = Vector4Accessors[0].Add(NewValue);
		Row = FRealtimeMeshStreamRowPtr(this, Index);
		return Index;
	}
	return INDEX_NONE;
}

void URealtimeMeshStream::SetInt(URealtimeMeshStream*& Builder, FRealtimeMeshStreamRowPtr& Row, int32 Index, int32 NewValue)
{
	Builder = this;
	if (Stream.IsValid() && IntAccessors.Num() >= 1 && IntAccessors[0].IsValidIndex(Index))
	{
		IntAccessors[0].Set(Index, NewValue);
		Row = FRealtimeMeshStreamRowPtr(this, Index);
	}
	else
	{
		Row = FRealtimeMeshStreamRowPtr();
	}
}

void URealtimeMeshStream::SetFloat(URealtimeMeshStream*& Builder, FRealtimeMeshStreamRowPtr& Row, int32 Index, float NewValue)
{
	Builder = this;
	if (Stream.IsValid() && FloatAccessors.Num() >= 1 && FloatAccessors[0].IsValidIndex(Index))
	{
		FloatAccessors[0].Set(Index, NewValue);
		Row = FRealtimeMeshStreamRowPtr(this, Index);
	}
	else
	{
		Row = FRealtimeMeshStreamRowPtr();
	}
}

void URealtimeMeshStream::SetVector2(URealtimeMeshStream*& Builder, FRealtimeMeshStreamRowPtr& Row, int32 Index, FVector2D NewValue)
{
	Builder = this;
	if (Stream.IsValid() && Vector2Accessors.Num() >= 1 && Vector2Accessors[0].IsValidIndex(Index))
	{
		Vector2Accessors[0].Set(Index, NewValue);
		Row = FRealtimeMeshStreamRowPtr(this, Index);
	}
	else
	{
		Row = FRealtimeMeshStreamRowPtr();
	}
}

void URealtimeMeshStream::SetVector3(URealtimeMeshStream*& Builder, FRealtimeMeshStreamRowPtr& Row, int32 Index, FVector NewValue)
{
	Builder = this;
	if (Stream.IsValid() && Vector3Accessors.Num() >= 1 && Vector3Accessors[0].IsValidIndex(Index))
	{
		Vector3Accessors[0].Set(Index, NewValue);
		Row = FRealtimeMeshStreamRowPtr(this, Index);
	}
	else
	{
		Row = FRealtimeMeshStreamRowPtr();
	}
}

void URealtimeMeshStream::SetVector4(URealtimeMeshStream*& Builder, FRealtimeMeshStreamRowPtr& Row, int32 Index, FVector4 NewValue)
{
	Builder = this;
	if (Stream.IsValid() && Vector4Accessors.Num() >= 1 && Vector4Accessors[0].IsValidIndex(Index))
	{
		Vector4Accessors[0].Set(Index, NewValue);
		Row = FRealtimeMeshStreamRowPtr(this, Index);
	}
	else
	{
		Row = FRealtimeMeshStreamRowPtr();
	}
}

int32 URealtimeMeshStream::GetInt(URealtimeMeshStream*& Builder, FRealtimeMeshStreamRowPtr& Row, int32 Index)
{
	Builder = this;
	if (Stream.IsValid() && IntAccessors.Num() >= 1 && IntAccessors[0].IsValidIndex(Index))
	{
		Row = FRealtimeMeshStreamRowPtr(this, Index);
		return IntAccessors[0].GetValue(Index);
	}
	return 0;
}

float URealtimeMeshStream::GetFloat(URealtimeMeshStream*& Builder, FRealtimeMeshStreamRowPtr& Row, int32 Index)
{
	Builder = this;
	if (Stream.IsValid() && FloatAccessors.Num() >= 1 && FloatAccessors[0].IsValidIndex(Index))
	{
		Row = FRealtimeMeshStreamRowPtr(this, Index);
		return FloatAccessors[0].GetValue(Index);
	}
	return 0;
}

FVector2D URealtimeMeshStream::GetVector2(URealtimeMeshStream*& Builder, FRealtimeMeshStreamRowPtr& Row, int32 Index)
{
	Builder = this;
	if (Stream.IsValid() && Vector2Accessors.Num() >= 1 && Vector2Accessors[0].IsValidIndex(Index))
	{
		Row = FRealtimeMeshStreamRowPtr(this, Index);
		return Vector2Accessors[0].GetValue(Index);
	}
	return FVector2D::Zero();
}

FVector URealtimeMeshStream::GetVector3(URealtimeMeshStream*& Builder, FRealtimeMeshStreamRowPtr& Row, int32 Index)
{
	Builder = this;
	if (Stream.IsValid() && Vector3Accessors.Num() >= 1 && Vector3Accessors[0].IsValidIndex(Index))
	{
		Row = FRealtimeMeshStreamRowPtr(this, Index);
		return Vector3Accessors[0].GetValue(Index);
	}
	return FVector::Zero();
}

FVector4 URealtimeMeshStream::GetVector4(URealtimeMeshStream*& Builder, FRealtimeMeshStreamRowPtr& Row, int32 Index)
{
	Builder = this;
	if (Stream.IsValid() && Vector4Accessors.Num() >= 1 && Vector4Accessors[0].IsValidIndex(Index))
	{
		Row = FRealtimeMeshStreamRowPtr(this, Index);
		return Vector4Accessors[0].GetValue(Index);
	}
	return FVector4::Zero();
}


void URealtimeMeshStreamSet::AddStream(URealtimeMeshStream* Stream)
{
	if (ensure(IsValid(Stream)))
	{
		if (ensure(Stream->HasValidData()))
		{
			Streams->AddStream(Stream->Consume());
		}
	}
}

void URealtimeMeshStreamSet::RemoveStream(const FRealtimeMeshStreamKey& StreamKey)
{
	Streams->Remove(StreamKey);
}

URealtimeMeshLocalBuilder* URealtimeMeshStreamSet::MakeLocalMeshBuilder(ERealtimeMeshSimpleStreamConfig WantedTangents,
	ERealtimeMeshSimpleStreamConfig WantedTexCoords, bool bWants32BitIndices, ERealtimeMeshSimpleStreamConfig WantedPolyGroupType,
	bool bWantsColors, int32 WantedTexCoordChannels, bool bKeepExistingData)
{
	URealtimeMeshLocalBuilder* Builder = NewObject<URealtimeMeshLocalBuilder>();
	Builder->Initialize(WantedTangents, WantedTexCoords, bWants32BitIndices, WantedPolyGroupType, bWantsColors, WantedTexCoordChannels, bKeepExistingData);

	Streams = MakeUnique<RealtimeMesh::FRealtimeMeshStreamSet>();
	return Builder;	
}


URealtimeMeshLocalBuilder* URealtimeMeshLocalBuilder::Initialize(ERealtimeMeshSimpleStreamConfig WantedTangents, ERealtimeMeshSimpleStreamConfig WantedTexCoords,
	bool bWants32BitIndices, ERealtimeMeshSimpleStreamConfig WantedPolyGroupType, bool bWantsColors, int32 WantedTexCoordChannels, bool bKeepExistingData)
{
	const auto TangentType = WantedTexCoords == ERealtimeMeshSimpleStreamConfig::HighPrecision ?
		RealtimeMesh::GetRealtimeMeshDataElementType<FPackedRGBA16N>() : 
		RealtimeMesh::GetRealtimeMeshDataElementType<FPackedNormal>();
	
	const auto TexCoordType = WantedTexCoords == ERealtimeMeshSimpleStreamConfig::HighPrecision ?
		RealtimeMesh::GetRealtimeMeshDataElementType<FVector2f>() : 
		RealtimeMesh::GetRealtimeMeshDataElementType<FVector2DHalf>();
	
	const auto TriangleType = bWants32BitIndices ?
		RealtimeMesh::GetRealtimeMeshDataElementType<uint32>() : 
		RealtimeMesh::GetRealtimeMeshDataElementType<uint16>();
	
	const auto PolyGroupType = WantedPolyGroupType == ERealtimeMeshSimpleStreamConfig::HighPrecision ?
		RealtimeMesh::GetRealtimeMeshDataElementType<uint32>() : 
		RealtimeMesh::GetRealtimeMeshDataElementType<uint16>();

	bool bIsValid = true;

	if (!bKeepExistingData)
	{
		Streams = MakeUnique<RealtimeMesh::FRealtimeMeshStreamSet>();
	}
	
	// Fixup triangles
	if (auto* Stream = Streams->Find(RealtimeMesh::FRealtimeMeshStreams::Triangles))
	{
		bIsValid &= Stream->ConvertTo(RealtimeMesh::FRealtimeMeshBufferLayout(TriangleType, 3));
	}

	// Fixup depth only triangles
	if (auto* Stream = Streams->Find(RealtimeMesh::FRealtimeMeshStreams::DepthOnlyTriangles))
	{
		bIsValid &= Stream->ConvertTo(RealtimeMesh::FRealtimeMeshBufferLayout(TriangleType, 3));
	}

	// Fixup polygroups
	if (auto* Stream = Streams->Find(RealtimeMesh::FRealtimeMeshStreams::PolyGroups))
	{
		bIsValid &= Stream->ConvertTo(RealtimeMesh::FRealtimeMeshBufferLayout(PolyGroupType, 1));
	}

	// Fixup depth only poly groups
	if (auto* Stream = Streams->Find(RealtimeMesh::FRealtimeMeshStreams::DepthOnlyPolyGroups))
	{
		bIsValid &= Stream->ConvertTo(RealtimeMesh::FRealtimeMeshBufferLayout(PolyGroupType, 1));
	}

	// Fixup tangents
	if (auto* Stream = Streams->Find(RealtimeMesh::FRealtimeMeshStreams::Tangents))
	{
		bIsValid &= Stream->ConvertTo(RealtimeMesh::FRealtimeMeshBufferLayout(TangentType, 2));
	}

	// Fixup tex coords
	if (auto* Stream = Streams->Find(RealtimeMesh::FRealtimeMeshStreams::TexCoords))
	{
		bIsValid &= Stream->ConvertTo(RealtimeMesh::FRealtimeMeshBufferLayout(TexCoordType, WantedTexCoordChannels));
	}	

	if (!ensureMsgf(bIsValid, TEXT("One of the streams is an invalid format!")))
	{
		Reset();
		return this;
	}
	
	MeshBuilder = MakeUnique<RealtimeMesh::TRealtimeMeshBuilderLocal<void, void, void, 1, void>>(*Streams);

	if (!bKeepExistingData)
	{
		MeshBuilder->EmptyVertices();
		MeshBuilder->EmptyTriangles();
		if (MeshBuilder->HasDepthOnlyTriangles())
		{
			MeshBuilder->EmptyDepthOnlyTriangles();
		}
	}

	// Setup colors
	if (bWantsColors)
	{
		MeshBuilder->EnableColors();
	}

	// Setup tangents
	if (WantedTangents != ERealtimeMeshSimpleStreamConfig::None)
	{
		MeshBuilder->EnableTangents(TangentType);
	}

	// Setup tex coords
	if (WantedTexCoords != ERealtimeMeshSimpleStreamConfig::None && WantedTexCoordChannels > 0)
	{
		MeshBuilder->EnableTexCoords(TexCoordType, FMath::Clamp(WantedTexCoordChannels, 1, 4));

		RealtimeMesh::FRealtimeMeshStream& Stream = Streams->FindChecked(RealtimeMesh::FRealtimeMeshStreams::TexCoords);

		if (WantedTexCoordChannels > 1)
		{
			UV1Builder = MakeUnique<RealtimeMesh::TRealtimeMeshStreamBuilder<FVector2d, void>>(Stream, 1);
		}

		if (WantedTexCoordChannels > 2)
		{
			UV2Builder = MakeUnique<RealtimeMesh::TRealtimeMeshStreamBuilder<FVector2d, void>>(Stream, 2);
		}

		if (WantedTexCoordChannels > 3)
		{
			UV3Builder = MakeUnique<RealtimeMesh::TRealtimeMeshStreamBuilder<FVector2d, void>>(Stream, 3);
		}
	}

	if (WantedPolyGroupType != ERealtimeMeshSimpleStreamConfig::None)
	{
		MeshBuilder->EnablePolyGroups(WantedTangents == ERealtimeMeshSimpleStreamConfig::HighPrecision ?
			RealtimeMesh::GetRealtimeMeshDataElementType<uint32>() : 
			RealtimeMesh::GetRealtimeMeshDataElementType<uint16>());
	}

	return this;
}

void URealtimeMeshLocalBuilder::AddStream(URealtimeMeshStream* Stream)
{
	Super::AddStream(Stream);
}

void URealtimeMeshLocalBuilder::RemoveStream(const FRealtimeMeshStreamKey& StreamKey)
{
	if (StreamKey == RealtimeMesh::FRealtimeMeshStreams::Position || StreamKey == RealtimeMesh::FRealtimeMeshStreams::Triangles)
	{
		MeshBuilder.Reset();
		UV1Builder.Reset();
		UV2Builder.Reset();
		UV3Builder.Reset();
	}

	if (StreamKey == RealtimeMesh::FRealtimeMeshStreams::DepthOnlyTriangles)
	{
		DisableDepthOnlyTriangles();
	}
	
	if (StreamKey == RealtimeMesh::FRealtimeMeshStreams::PolyGroups || StreamKey == RealtimeMesh::FRealtimeMeshStreams::DepthOnlyPolyGroups)
	{
		DisablePolyGroups();
	}

	if (StreamKey == RealtimeMesh::FRealtimeMeshStreams::Tangents)
	{
		DisableTangents();
	}

	if (StreamKey == RealtimeMesh::FRealtimeMeshStreams::Color)
	{
		DisableColors();
	}

	if (StreamKey == RealtimeMesh::FRealtimeMeshStreams::TexCoords)
	{
		DisableTexCoords();
	}
	
	Super::RemoveStream(StreamKey);
}

void URealtimeMeshLocalBuilder::Reset()
{
	MeshBuilder.Reset();
	UV1Builder.Reset();
	UV2Builder.Reset();
	UV3Builder.Reset();
	Super::Reset();
}

URealtimeMeshLocalBuilder* URealtimeMeshLocalBuilder::EnableTangents(bool bUseHighIsValid)
{
	if (ensure(Streams.IsValid() && MeshBuilder.IsValid()))
	{
		MeshBuilder->DisableTangents();
		MeshBuilder->EnableTangents(bUseHighIsValid ?
			RealtimeMesh::GetRealtimeMeshDataElementType<FPackedRGBA16N>() : 
			RealtimeMesh::GetRealtimeMeshDataElementType<FPackedNormal>());
	}
	return this;
}

URealtimeMeshLocalBuilder* URealtimeMeshLocalBuilder::DisableTangents()
{	
	if (ensure(Streams.IsValid() && MeshBuilder.IsValid()))
	{
		MeshBuilder->DisableTangents();
	}
	return this;
}

URealtimeMeshLocalBuilder* URealtimeMeshLocalBuilder::EnableColors()
{
	if (ensure(Streams.IsValid() && MeshBuilder.IsValid()))
	{
		MeshBuilder->DisableColors();
		MeshBuilder->EnableColors();
	}
	return this;
}

URealtimeMeshLocalBuilder* URealtimeMeshLocalBuilder::DisableColors()
{
	if (ensure(Streams.IsValid() && MeshBuilder.IsValid()))
	{
		MeshBuilder->DisableColors();
	}
	return this;
}

URealtimeMeshLocalBuilder* URealtimeMeshLocalBuilder::EnableTexCoords(int32 NumChannels, bool bUseHighPrecision)
{
	if (ensure(Streams.IsValid() && MeshBuilder.IsValid()))
	{
		MeshBuilder->DisableTexCoords();
		const auto TexCoordElementType = bUseHighPrecision ?
			RealtimeMesh::GetRealtimeMeshDataElementType<FVector2f>() : 
			RealtimeMesh::GetRealtimeMeshDataElementType<FVector2DHalf>();

		MeshBuilder->EnableTexCoords(TexCoordElementType, FMath::Clamp(NumChannels, 1, 4));

		RealtimeMesh::FRealtimeMeshStream& Stream = Streams->FindChecked(RealtimeMesh::FRealtimeMeshStreams::TexCoords);

		if (NumChannels > 1)
		{
			UV1Builder = MakeUnique<RealtimeMesh::TRealtimeMeshStreamBuilder<FVector2d, void>>(Stream, 1);
		}

		if (NumChannels > 2)
		{
			UV2Builder = MakeUnique<RealtimeMesh::TRealtimeMeshStreamBuilder<FVector2d, void>>(Stream, 2);
		}

		if (NumChannels > 3)
		{
			UV3Builder = MakeUnique<RealtimeMesh::TRealtimeMeshStreamBuilder<FVector2d, void>>(Stream, 3);
		}		
	}
	return this;
}

URealtimeMeshLocalBuilder* URealtimeMeshLocalBuilder::DisableTexCoords()
{
	if (ensure(Streams.IsValid() && MeshBuilder.IsValid()))
	{
		MeshBuilder->DisableTexCoords();
		UV1Builder.Reset();
		UV2Builder.Reset();
		UV3Builder.Reset();
	}
	return this;
}

URealtimeMeshLocalBuilder* URealtimeMeshLocalBuilder::EnableDepthOnlyTriangles(bool bUse32BitIndices)
{
	if (ensure(Streams.IsValid() && MeshBuilder.IsValid()))
	{
		MeshBuilder->DisableDepthOnlyTriangles();
		MeshBuilder->EnableDepthOnlyTriangles(bUse32BitIndices ?
			RealtimeMesh::GetRealtimeMeshDataElementType<uint32>() : 
			RealtimeMesh::GetRealtimeMeshDataElementType<uint16>());
	}
	return this;
}

URealtimeMeshLocalBuilder* URealtimeMeshLocalBuilder::DisableDepthOnlyTriangles()
{
	if (ensure(Streams.IsValid() && MeshBuilder.IsValid()))
	{
		MeshBuilder->DisableDepthOnlyTriangles();
	}
	return this;
}

URealtimeMeshLocalBuilder* URealtimeMeshLocalBuilder::EnablePolyGroups(bool bUse32BitIndices)
{
	if (ensure(Streams.IsValid() && MeshBuilder.IsValid()))
	{
		MeshBuilder->DisablePolyGroups();
		MeshBuilder->EnablePolyGroups(bUse32BitIndices ?
			RealtimeMesh::GetRealtimeMeshDataElementType<uint32>() : 
			RealtimeMesh::GetRealtimeMeshDataElementType<uint16>());
	}
	return this;
}

URealtimeMeshLocalBuilder* URealtimeMeshLocalBuilder::DisablePolyGroups()
{
	if (ensure(Streams.IsValid() && MeshBuilder.IsValid()))
	{
		MeshBuilder->DisablePolyGroups();
	}
	return this;
}

int32 URealtimeMeshLocalBuilder::AddTriangle(URealtimeMeshLocalBuilder*& Builder, int32 UV0, int32 UV1, int32 UV2, int32 PolyGroupIndex)
{
	check(IsValid(this));
	Builder = this;
	if (MeshBuilder.IsValid())
	{
		if (MeshBuilder->HasPolyGroups())
		{
			return MeshBuilder->AddTriangle(UV0, UV1, UV2, PolyGroupIndex);
		}
		else
		{
			return MeshBuilder->AddTriangle(UV0, UV1, UV2);
		}
	}
	RMC_RATE_LIMIT_LOG({
		FMessageLog("RealtimeMesh").Error(LOCTEXT("MeshLocalBuilder_AddTriangle_InvalidBuilder", "AddTriangle: Builder not valid"));
	});
	return INDEX_NONE;
}

void URealtimeMeshLocalBuilder::SetTriangle(URealtimeMeshLocalBuilder*& Builder, int32 Index, int32 UV0, int32 UV1, int32 UV2, int32 PolyGroupIndex)
{
	Builder = this;
	if (MeshBuilder.IsValid())
	{
		MeshBuilder->SetTriangle(Index, UV0, UV1, UV2, PolyGroupIndex);
		return;
	}
	RMC_RATE_LIMIT_LOG({
		FMessageLog("RealtimeMesh").Error(LOCTEXT("MeshLocalBuilder_SetTriangle_InvalidBuilder", "SetTriangle: Builder not valid"));
	});
}

void URealtimeMeshLocalBuilder::GetTriangle(URealtimeMeshLocalBuilder*& Builder, int32 Index, int32& UV0, int32& UV1, int32& UV2, int32& PolyGroupIndex)
{
	Builder = this;
	if (MeshBuilder.IsValid())
	{
		const auto Triangle = MeshBuilder->GetTriangle(Index);
		UV0 = Triangle.V0;
		UV1 = Triangle.V1;
		UV2 = Triangle.V2;
		PolyGroupIndex = 0;
		if (MeshBuilder->HasPolyGroups())
		{
			PolyGroupIndex = MeshBuilder->GetMaterialIndex(Index);
		}
		return;
	}
	
	UV0 = UV1 = UV2 = PolyGroupIndex = 0;
	RMC_RATE_LIMIT_LOG({
		FMessageLog("RealtimeMesh").Error(LOCTEXT("MeshLocalBuilder_GetTriangle_InvalidBuilder", "GetTriangle: Builder not valid"));
	});
}

int32 URealtimeMeshLocalBuilder::AddVertex(URealtimeMeshLocalBuilder*& Builder, const FRealtimeMeshBasicVertex& InVertex)
{
	using namespace RealtimeMesh;

	check(IsValid(this));
	Builder = this;
	if (MeshBuilder.IsValid())
	{
		auto Vertex = MeshBuilder->AddVertex(FVector3f(InVertex.Position));
	
		if (MeshBuilder->HasTangents())
		{
			if (InVertex.Binormal.IsNearlyZero())
			{
				Vertex.SetNormalAndTangent(FVector3f(InVertex.Normal), FVector3f(InVertex.Tangent));				
			}
			else
			{
				Vertex.SetTangents(FVector3f(InVertex.Normal), FVector3f(InVertex.Binormal), FVector3f(InVertex.Tangent));
			}
		}
	
		if (MeshBuilder->HasVertexColors())
		{
			Vertex.SetColor(InVertex.Color);
		}
	
		if (MeshBuilder->HasTexCoords())
		{
			Vertex.SetTexCoord(0, FVector2f(InVertex.UV0));

			if (UV1Builder.IsValid())
			{
				UV1Builder->SetElement(Vertex.GetIndex(), 1, InVertex.UV1);
			}
			if (UV2Builder.IsValid())
			{
				UV2Builder->SetElement(Vertex.GetIndex(), 2, InVertex.UV2);
			}
			if (UV3Builder.IsValid())
			{
				UV3Builder->SetElement(Vertex.GetIndex(), 3, InVertex.UV3);
			}
		}

		return Vertex.GetIndex();
	}
	
	RMC_RATE_LIMIT_LOG({
		FMessageLog("RealtimeMesh").Error(LOCTEXT("MeshLocalBuilder_AddVertex_InvalidBuilder", "AddVertex: Builder not valid"));
	});
	return INDEX_NONE;
}

void URealtimeMeshLocalBuilder::EditVertex(URealtimeMeshLocalBuilder*& Builder, int32 Index, FVector Position, bool bWritePosition, FVector Normal, bool bWriteNormal, FVector Tangent,
	bool bWriteTangent, FLinearColor Color, bool bWriteColor, FVector2D UV0, bool bWriteUV0, FVector2D UV1, bool bWriteUV1, FVector2D UV2, bool bWriteUV2, FVector2D UV3,
	bool bWriteUV3)
{
	using namespace RealtimeMesh;

	Builder = this;
	if (MeshBuilder.IsValid())
	{
		auto Vertex = MeshBuilder->EditVertex(Index);

		if (bWritePosition)
		{
			Vertex.SetPosition(FVector3f(Position));
		}
	
		if (bWriteNormal && MeshBuilder->HasTangents())
		{
			Vertex.SetNormal(FVector3f(Normal));
		}
		if (bWriteTangent && MeshBuilder->HasTangents())
		{
			Vertex.SetTangent(FVector3f(Tangent));
		}
	
		if (bWriteColor && MeshBuilder->HasVertexColors())
		{
			Vertex.SetColor(Color);
		}
	
		if (bWriteUV0 && MeshBuilder->HasTexCoords())
		{
			Vertex.SetTexCoord(0, FVector2f(UV0));
		}
		if (bWriteUV1 && UV1Builder.IsValid())
		{
			UV1Builder->Set(Vertex.GetIndex(), UV1);
		}
		if (bWriteUV2 && UV1Builder.IsValid())
		{
			UV2Builder->Set(Vertex.GetIndex(), UV2);
		}
		if (bWriteUV3 && UV3Builder.IsValid())
		{
			UV3Builder->Set(Vertex.GetIndex(), UV3);
		}
		return;
	}
	
	RMC_RATE_LIMIT_LOG({
		FMessageLog("RealtimeMesh").Error(LOCTEXT("MeshLocalBuilder_EditVertex_InvalidBuilder", "EditVertex: Builder not valid"));
	});
}

void URealtimeMeshLocalBuilder::GetVertex(URealtimeMeshLocalBuilder*& Builder, int32 Index, FVector& Position, FVector& Normal, FVector& Tangent, FLinearColor& Color, FVector2D& UV0,
	FVector2D& UV1, FVector2D& UV2, FVector2D& UV3)
{
	Builder = this;
	if (MeshBuilder.IsValid())
	{
		const auto Vertex = MeshBuilder->EditVertex(Index);

		Position = FVector(Vertex.GetPosition());

		if (MeshBuilder->HasTangents())
		{
			Normal = FVector(Vertex.GetNormal());
			Tangent = FVector(Vertex.GetTangent());		
		}
	
		if (MeshBuilder->HasVertexColors())
		{
			Color = Vertex.GetLinearColor();		
		}

		if (MeshBuilder->HasTexCoords())
		{
			UV0 = FVector2D(Vertex.GetTexCoord(0));
		}

		if (UV1Builder.IsValid())
		{
			UV1 = FVector2D(UV1Builder->Get(Index).Get());
		}
		if (UV2Builder.IsValid())
		{
			UV2 = FVector2D(UV2Builder->Get(Index).Get());
		}
		if (UV3Builder.IsValid())
		{
			UV3 = FVector2D(UV3Builder->Get(Index).Get());
		}
	}
	else
	{
		Position = FVector::ZeroVector;
		Normal = FVector::ZeroVector;
		Tangent = FVector::ZeroVector;
		Color = FColor::White;
		UV0 = UV1 = UV2 = UV3 = FVector2D::ZeroVector;
		
		RMC_RATE_LIMIT_LOG({
			FMessageLog("RealtimeMesh").Error(LOCTEXT("MeshLocalBuilder_GetVertex_InvalidBuilder", "GetVertex: Builder not valid"));
		});
	}
}



URealtimeMeshStream* URealtimeMeshStreamPool::RequestStream(const FRealtimeMeshStreamKey& StreamKey, ERealtimeMeshSimpleStreamType StreamType, int32 NumElements)
{	
	if (CachedStreams.Num() > 0)
	{
		auto Stream = CachedStreams.Pop(false);
		Stream->Initialize(StreamKey, StreamType, NumElements);
		return Stream;
	}
	
	URealtimeMeshStream* NewStream = NewObject<URealtimeMeshStream>();
	NewStream->Initialize(StreamKey, StreamType, NumElements);

	// If we have allocated more streams than our safety threshold, drop our holds on the existing streams.
	// This will allow them to be garbage-collected (eventually)
	if (!ensure(AllCreatedStreams.Num() < CVarRealtimeMeshStreamPoolMaxSizeThreshold.GetValueOnGameThread()))
	{
		UE_LOG(RealtimeMeshLog, Warning, TEXT("URealtimeMeshStreamPool Threshold of %d Allocated Streams exceeded! Releasing references to all current streams and forcing a garbage collection."), CVarRealtimeMeshStreamPoolMaxSizeThreshold.GetValueOnGameThread());
		AllCreatedStreams.Reset();
		GEngine->ForceGarbageCollection(true);
	}

	AllCreatedStreams.Add(NewStream);
	return NewStream;
}

void URealtimeMeshStreamPool::ReturnStream(URealtimeMeshStream* Stream)
{
	if (ensure(Stream) && ensure(AllCreatedStreams.Contains(Stream)))
	{
		Stream->Reset();
		if (ensure(CachedStreams.Contains(Stream) == false))
		{
			CachedStreams.Add(Stream);
		}
	}
}

URealtimeMeshStreamSet* URealtimeMeshStreamPool::RequestStreamSet()
{	
	if (CachedStreamSets.Num() > 0)
	{
		return CachedStreamSets.Pop(false);
	}
	
	URealtimeMeshStreamSet* NewStreamSet = NewObject<URealtimeMeshStreamSet>();

	// If we have allocated more streams than our safety threshold, drop our holds on the existing streams.
	// This will allow them to be garbage-collected (eventually)
	if (!ensure(AllCreatedStreamSets.Num() < CVarRealtimeMeshStreamSetsPoolMaxSizeThreshold.GetValueOnGameThread()))
	{
		UE_LOG(RealtimeMeshLog, Warning, TEXT("URealtimeMeshStreamPool Threshold of %d Allocated StreamSets exceeded! Releasing references to all current streamssets and forcing a garbage collection."), CVarRealtimeMeshStreamSetsPoolMaxSizeThreshold.GetValueOnGameThread());
		AllCreatedStreamSets.Reset();
		GEngine->ForceGarbageCollection(true);
	}

	AllCreatedStreamSets.Add(NewStreamSet);
	return NewStreamSet;
}

void URealtimeMeshStreamPool::ReturnStreamSet(URealtimeMeshStreamSet* StreamSet)
{
	if (ensure(StreamSet) && ensure(AllCreatedStreamSets.Contains(StreamSet)))
	{
		StreamSet->Reset();
		if (ensure(CachedStreamSets.Contains(StreamSet) == false))
		{
			CachedStreamSets.Add(StreamSet);
		}
	}
}

URealtimeMeshLocalBuilder* URealtimeMeshStreamPool::RequestMeshBuilder()
{
	if (CachedBuilders.Num() > 0)
	{
		return CachedBuilders.Pop(false);
	}
	
	URealtimeMeshLocalBuilder* NewBuilder = NewObject<URealtimeMeshLocalBuilder>();

	// If we have allocated more streams than our safety threshold, drop our holds on the existing streams.
	// This will allow them to be garbage-collected (eventually)
	if (!ensure(AllCreatedBuilders.Num() < CVarRealtimeMeshBuilderPoolMaxSizeThreshold.GetValueOnGameThread()))
	{
		UE_LOG(RealtimeMeshLog, Warning, TEXT("URealtimeMeshStreamPool Threshold of %d Allocated Builders exceeded! Releasing references to all current builders and forcing a garbage collection."), CVarRealtimeMeshBuilderPoolMaxSizeThreshold.GetValueOnGameThread());
		AllCreatedBuilders.Reset();
		GEngine->ForceGarbageCollection(true);
	}

	AllCreatedBuilders.Add(NewBuilder);
	return NewBuilder;
}

void URealtimeMeshStreamPool::ReturnMeshBuilder(URealtimeMeshLocalBuilder* Builder)
{
	if (ensure(Builder) && ensure(AllCreatedBuilders.Contains(Builder)))
	{
		Builder->Reset();
		if (ensure(CachedBuilders.Contains(Builder) == false))
		{
			CachedBuilders.Add(Builder);
		}
	}
}

void URealtimeMeshStreamPool::ReturnAllStreams()
{
	{
		CachedStreams = AllCreatedStreams;
		for (URealtimeMeshStream* Stream : CachedStreams)
		{
			if (Stream)
			{
				Stream->Reset();
			}
		}

		ensure(0 == CachedStreams.RemoveAll([](const URealtimeMeshStream* Stream) { return Stream == nullptr; }));
	}
	{
		CachedStreamSets = AllCreatedStreamSets;
		for (URealtimeMeshStreamSet* StreamSet : CachedStreamSets)
		{
			if (StreamSet)
			{
				StreamSet->Reset();
			}
		}

		ensure(0 == CachedStreamSets.RemoveAll([](const URealtimeMeshStreamSet* StreamSet) { return StreamSet == nullptr; }));
	}
	{
		CachedBuilders = AllCreatedBuilders;
		for (URealtimeMeshLocalBuilder* Builder : CachedBuilders)
		{
			if (Builder)
			{
				Builder->Reset();
			}
		}

		ensure(0 == CachedBuilders.RemoveAll([](const URealtimeMeshLocalBuilder* Builder) { return Builder == nullptr; }));
	}
}

void URealtimeMeshStreamPool::FreeAllStreams()
{
	CachedStreams.Reset();
	AllCreatedStreams.Reset();
	CachedStreamSets.Reset();
	AllCreatedStreamSets.Reset();
	CachedBuilders.Reset();
	AllCreatedBuilders.Reset();
}



URealtimeMeshStreamSet* URealtimeMeshStreamUtils::CopyStreamSetFromComponents(URealtimeMeshStreamSet* Streams, const FRealtimeMeshStreamSetFromComponents& Components)
{
	if (!IsValid(Streams))
	{
		return nullptr;
	}

	if (auto* Stream = Streams->GetStreamSet().Find(RealtimeMesh::FRealtimeMeshStreams::Triangles))
	{
		ensure(Stream->ConvertTo(RealtimeMesh::FRealtimeMeshBufferLayout(Components.bUse32BitIndices?
			RealtimeMesh::GetRealtimeMeshDataElementType<uint32>() : RealtimeMesh::GetRealtimeMeshDataElementType<uint16>() , 3)));
	}	

	RealtimeMesh::TRealtimeMeshBuilderLocal<void, void, void, 1, void> Builder(Streams->GetStreamSet());

	const bool bHasTangents = Components.Normals.Num() > 0 || Components.Tangents.Num() > 0 || Components.Binormals.Num() > 0;
	const int32 NumUVs = Components.UV3.Num() > 0 ? 4 : (Components.UV2.Num() > 0 ? 3 : (Components.UV1.Num() > 0 ? 2 : (Components.UV0.Num() > 0 ? 1 : 0)));

	if (bHasTangents)
	{
		Builder.EnableTangents(Components.bUseHighPrecisionTangents? RealtimeMesh::GetRealtimeMeshDataElementType<FPackedRGBA16N>() : RealtimeMesh::GetRealtimeMeshDataElementType<FPackedNormal>());
	}

	if (NumUVs > 0)
	{
		Builder.EnableTexCoords(Components.bUseHighPrecisionTexCoords? RealtimeMesh::GetRealtimeMeshDataElementType<FVector2f>() : RealtimeMesh::GetRealtimeMeshDataElementType<FVector2DHalf>(), NumUVs);
	}

	if (Components.Colors.Num() > 0)
	{
		Builder.EnableColors();
	}
	
	for (int32 Index = 0; Index < Components.Positions.Num(); Index++)
	{
		auto Vertex = Builder.AddVertex(FVector3f(Components.Positions[Index]));

		if (Components.Normals.Num() > Vertex)
		{
			if (Components.Tangents.Num() > 0)
			{
				if (Components.Binormals.Num() > 0)
				{
					Vertex.SetTangents(FVector3f(Components.Normals[Index]), FVector3f(Components.Binormals[Index]), FVector3f(Components.Tangents[Index]));
				}
				else
				{
					Vertex.SetNormalAndTangent(FVector3f(Components.Normals[Index]), FVector3f(Components.Tangents[Index]));
				}
			}
			else
			{
				Vertex.SetNormal(FVector3f(Components.Normals[Index]));				
			}
		}

		if (Components.Colors.Num() > Vertex)
		{
			Vertex.SetColor(Components.Colors[Index]);
		}

		if (Components.UV0.Num() > Vertex)
		{
			Vertex.SetTexCoord(FVector2f(Components.UV0[Index]));
		}	
	}

	if (NumUVs > 0)
	{
		if (Components.UV1.Num() > 0)
		{
			RealtimeMesh::TRealtimeMeshStridedStreamBuilder<FVector2f, void> UV0Builder(Streams->GetStreamSet().FindChecked(RealtimeMesh::FRealtimeMeshStreams::TexCoords), 1);

			const int32 NumToCopy = FMath::Min(Builder.NumVertices(), Components.UV1.Num());
			for (int32 Index = 0; Index < NumToCopy; Index++)
			{
				UV0Builder.Set(Index, FVector2f(Components.UV1[Index]));
			}
		}

		if (Components.UV2.Num() > 0)
		{
			RealtimeMesh::TRealtimeMeshStridedStreamBuilder<FVector2f, void> UV1Builder(Streams->GetStreamSet().FindChecked(RealtimeMesh::FRealtimeMeshStreams::TexCoords), 2);

			const int32 NumToCopy = FMath::Min(Builder.NumVertices(), Components.UV2.Num());
			for (int32 Index = 0; Index < NumToCopy; Index++)
			{
				UV1Builder.Set(Index, FVector2f(Components.UV2[Index]));
			}
		}

		if (Components.UV3.Num() > 0)
		{
			RealtimeMesh::TRealtimeMeshStridedStreamBuilder<FVector2f, void> UV2Builder(Streams->GetStreamSet().FindChecked(RealtimeMesh::FRealtimeMeshStreams::TexCoords), 3);

			const int32 NumToCopy = FMath::Min(Builder.NumVertices(), Components.UV3.Num());
			for (int32 Index = 0; Index < NumToCopy; Index++)
			{
				UV2Builder.Set(Index, FVector2f(Components.UV3[Index]));
			}
		}		
	}

	if (Components.PolyGroups.Num() > 0)
	{
		for (int32 Index = 0; Index < Components.Triangles.Num(); Index+=3)
		{
			Builder.AddTriangle(Components.Triangles[Index + 0], Components.Triangles[Index + 1], Components.Triangles[Index + 2], Components.PolyGroups.IsValidIndex(Index / 3) ? Components.PolyGroups[Index / 3] : 0);
		}	
	}
	else
	{		
		for (int32 Index = 0; Index < Components.Triangles.Num(); Index+=3)
		{
			Builder.AddTriangle(Components.Triangles[Index + 0], Components.Triangles[Index + 1], Components.Triangles[Index + 2]);
		}
	}
	return Streams;
}

const FRealtimeMeshStreamRowPtr& URealtimeMeshStreamUtils::SetIntElement(const FRealtimeMeshStreamRowPtr& Row, int32 Index, int32 ElementIdx, int32 NewValue)
{
	if (Row.IsValid() && Row.Stream->IntAccessors.Num() > ElementIdx)
	{
		Row.Stream->IntAccessors[ElementIdx].Set(Index, NewValue);
	}
	return Row;
}

const FRealtimeMeshStreamRowPtr& URealtimeMeshStreamUtils::SetFloatElement(const FRealtimeMeshStreamRowPtr& Row, int32 Index, int32 ElementIdx, float NewValue)
{
	if (Row.IsValid() && Row.Stream->FloatAccessors.Num() > ElementIdx)
	{
		Row.Stream->FloatAccessors[ElementIdx].Set(Index, NewValue);
	}
	return Row;
}

const FRealtimeMeshStreamRowPtr& URealtimeMeshStreamUtils::SetVector2Element(const FRealtimeMeshStreamRowPtr& Row, int32 Index, int32 ElementIdx, FVector2D NewValue)
{
	if (Row.IsValid() && Row.Stream->Vector2Accessors.Num() > ElementIdx)
	{
		Row.Stream->Vector2Accessors[ElementIdx].Set(Index, NewValue);
	}
	return Row;
}

const FRealtimeMeshStreamRowPtr& URealtimeMeshStreamUtils::SetVector3Element(const FRealtimeMeshStreamRowPtr& Row, int32 Index, int32 ElementIdx, FVector NewValue)
{
	if (Row.IsValid() && Row.Stream->Vector3Accessors.Num() > ElementIdx)
	{
		Row.Stream->Vector3Accessors[ElementIdx].Set(Index, NewValue);
	}
	return Row;
}

const FRealtimeMeshStreamRowPtr& URealtimeMeshStreamUtils::SetVector4Element(const FRealtimeMeshStreamRowPtr& Row, int32 Index, int32 ElementIdx, FVector4 NewValue)
{
	if (Row.IsValid() && Row.Stream->Vector4Accessors.Num() > ElementIdx)
	{
		Row.Stream->Vector4Accessors[ElementIdx].Set(Index, NewValue);
	}
	return Row;
}

int32 URealtimeMeshStreamUtils::GetIntElement(const FRealtimeMeshStreamRowPtr& Row, int32 Index, int32 ElementIdx, FRealtimeMeshStreamRowPtr& OutRow)
{
	OutRow = Row;
	if (ensure(Row.IsValid() && Row.Stream->IntAccessors.Num() > ElementIdx))
	{
		return Row.Stream->IntAccessors[ElementIdx].Get(Index);
	}
	return 0;
}

float URealtimeMeshStreamUtils::GetFloatElement(const FRealtimeMeshStreamRowPtr& Row, int32 Index, int32 ElementIdx, FRealtimeMeshStreamRowPtr& OutRow)
{
	OutRow = Row;
	if (ensure(Row.IsValid() && Row.Stream->FloatAccessors.Num() > ElementIdx))
	{
		return Row.Stream->FloatAccessors[ElementIdx].Get(Index);
	}
	return 0.0f;
}

FVector2D URealtimeMeshStreamUtils::GetVector2Element(const FRealtimeMeshStreamRowPtr& Row, int32 Index, int32 ElementIdx, FRealtimeMeshStreamRowPtr& OutRow)
{
	OutRow = Row;
	if (ensure(Row.IsValid() && Row.Stream->Vector2Accessors.Num() > ElementIdx))
	{
		return Row.Stream->Vector2Accessors[ElementIdx].Get(Index);
	}
	return FVector2D::ZeroVector;
}

FVector URealtimeMeshStreamUtils::GetVector3Element(const FRealtimeMeshStreamRowPtr& Row, int32 Index, int32 ElementIdx, FRealtimeMeshStreamRowPtr& OutRow)
{
	OutRow = Row;
	if (ensure(Row.IsValid() && Row.Stream->Vector3Accessors.Num() > ElementIdx))
	{
		return Row.Stream->Vector3Accessors[ElementIdx].Get(Index);
	}
	return FVector::ZeroVector;
}

FVector4 URealtimeMeshStreamUtils::GetVector4Element(const FRealtimeMeshStreamRowPtr& Row, int32 Index, int32 ElementIdx, FRealtimeMeshStreamRowPtr& OutRow)
{
	OutRow = Row;
	if (ensure(Row.IsValid() && Row.Stream->Vector4Accessors.Num() > ElementIdx))
	{
		return Row.Stream->Vector4Accessors[ElementIdx].Get(Index);
	}
	return FVector4::Zero();
}



#undef LOCTEXT_NAMESPACE
