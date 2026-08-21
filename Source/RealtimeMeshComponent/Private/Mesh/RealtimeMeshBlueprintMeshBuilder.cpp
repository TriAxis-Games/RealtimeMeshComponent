// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.


#include "Mesh/RealtimeMeshBlueprintMeshBuilder.h"
#include "Engine/Engine.h"
#include "RealtimeMeshComponentModule.h"
#include "Core/RealtimeMeshDataStream.h"
#include "Logging/MessageLog.h"

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

RealtimeMesh::FRealtimeMeshStream URealtimeMeshStream::Consume()
{
	RealtimeMesh::FRealtimeMeshStream Temp = RealtimeMesh::FRealtimeMeshStream(MoveTemp(*Stream));
	Stream.Reset();
	return RealtimeMesh::FRealtimeMeshStream(MoveTemp(Temp));
}

void URealtimeMeshStream::Initialize(const FRealtimeMeshStreamKey& StreamKey, ERealtimeMeshSimpleStreamType StreamType, int32 NumElements)
{
	ClearAccessors();

	// DUP-012: the buffer-layout half of this switch duplicated GetBufferLayout()'s
	// StreamType->layout mapping case-for-case (including the Triangle16/32 NumElements==1
	// ensure and the empty-layout default). Delegate the layout construction to it and keep
	// only the accessor-category mapping here. An unrecognized StreamType yields an invalid
	// layout, reproducing the former default branch (Stream reset, accessors cleared).
	const RealtimeMesh::FRealtimeMeshBufferLayout Layout = GetBufferLayout(StreamType, NumElements);
	if (!Layout.IsValid())
	{
		Stream.Reset();
		ClearAccessors();
		return;
	}

	Stream = MakeShared<RealtimeMesh::FRealtimeMeshStream>(StreamKey, Layout);

	switch(StreamType)
	{
	case ERealtimeMeshSimpleStreamType::Int16:
	case ERealtimeMeshSimpleStreamType::UInt16:
	case ERealtimeMeshSimpleStreamType::Int32:
	case ERealtimeMeshSimpleStreamType::UInt32:
	// Triangle streams are integer element streams (TIndex3 = 3x uint16/uint32 elements);
	// they were wrongly grouped with the packed vector types below, which asserted
	// (CanConvert(uint16 <-> FVector4d)) on any Blueprint MakeStream(Triangle16/32).
	case ERealtimeMeshSimpleStreamType::Triangle16:
	case ERealtimeMeshSimpleStreamType::Triangle32:
		SetupIntAccessors();
		break;
	case ERealtimeMeshSimpleStreamType::Float:
		SetupFloatAccessors();
		break;
	case ERealtimeMeshSimpleStreamType::Vector2:
	case ERealtimeMeshSimpleStreamType::HalfVector2:
		SetupVector2Accessors();
		break;
	case ERealtimeMeshSimpleStreamType::Vector3:
		SetupVector3Accessors();
		break;
	case ERealtimeMeshSimpleStreamType::PackedNormal:
	case ERealtimeMeshSimpleStreamType::PackedRGBA16N:
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





void URealtimeMeshStreamSet::EnsureInitialized()
{
	if (!Streams.IsValid())
	{
		Streams = MakeShared<RealtimeMesh::FRealtimeMeshStreamSet>();
	}
}

RealtimeMesh::FRealtimeMeshStreamSet URealtimeMeshStreamSet::Consume()
{
	EnsureInitialized();
	RealtimeMesh::FRealtimeMeshStreamSet Temp = RealtimeMesh::FRealtimeMeshStreamSet(MoveTemp(*Streams));
	Reset();
	return RealtimeMesh::FRealtimeMeshStreamSet(MoveTemp(Temp));
}

void URealtimeMeshStreamSet::AddStream(URealtimeMeshStream* Stream)
{
	if (ensure(IsValid(Stream)))
	{
		if (ensure(Stream->HasValidData()))
		{
			EnsureInitialized();
			Streams->AddStream(Stream->Consume());
		}
	}
}

void URealtimeMeshStreamSet::RemoveStream(const FRealtimeMeshStreamKey& StreamKey)
{
	if (Streams)
	{
		Streams->Remove(StreamKey);
	}
}

URealtimeMeshLocalBuilder* URealtimeMeshStreamSet::MakeLocalMeshBuilder(ERealtimeMeshSimpleStreamConfig WantedTangents,
	ERealtimeMeshSimpleStreamConfig WantedTexCoords, bool bWants32BitIndices, ERealtimeMeshSimpleStreamConfig WantedPolyGroupType,
	bool bWantsColors, int32 WantedTexCoordChannels, bool bKeepExistingData)
{
	URealtimeMeshLocalBuilder* Builder = NewObject<URealtimeMeshLocalBuilder>();
	Builder->Streams = MoveTemp(Streams);
	Streams.Reset();
	Builder->Initialize(WantedTangents, WantedTexCoords, bWants32BitIndices, WantedPolyGroupType, bWantsColors, WantedTexCoordChannels, bKeepExistingData);
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
		Reset();
	}
	EnsureInitialized();
	check(Streams.IsValid());
	
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
	
	Builder = MakeUnique<RealtimeMesh::FRealtimeMeshDynamicBuilder>(*Streams);

	if (!bKeepExistingData)
	{
		Builder->EmptyVertices();
		Builder->EmptyTriangles();
		if (Builder->HasDepthOnlyTriangles())
		{
			Builder->EmptyDepthOnlyTriangles();
		}
	}

	// Setup colors
	if (bWantsColors)
	{
		Builder->EnableColors();
	}

	// Setup tangents
	if (WantedTangents != ERealtimeMeshSimpleStreamConfig::None)
	{
		Builder->EnableTangents(TangentType);
	}

	// Setup tex coords
	if (WantedTexCoords != ERealtimeMeshSimpleStreamConfig::None && WantedTexCoordChannels > 0)
	{
		Builder->EnableTexCoords(TexCoordType, FMath::Clamp(WantedTexCoordChannels, 1, 4));
	}

	if (WantedPolyGroupType != ERealtimeMeshSimpleStreamConfig::None)
	{
		Builder->EnablePolyGroups(WantedTangents == ERealtimeMeshSimpleStreamConfig::HighPrecision ?
			RealtimeMesh::GetRealtimeMeshDataElementType<uint32>() :
			RealtimeMesh::GetRealtimeMeshDataElementType<uint16>());
	}

	return this;
}

void URealtimeMeshLocalBuilder::RemoveStream(const FRealtimeMeshStreamKey& StreamKey)
{
	if (StreamKey == RealtimeMesh::FRealtimeMeshStreams::Position || StreamKey == RealtimeMesh::FRealtimeMeshStreams::Triangles)
	{
		// Position / Triangles are required — drop the whole builder and let
		// the next Initialize rebuild from scratch.
		Builder.Reset();
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
	Builder.Reset();
	Super::Reset();
}

URealtimeMeshLocalBuilder* URealtimeMeshLocalBuilder::EnableTangents(bool bUseHighIsValid)
{
	if (ensure(Streams.IsValid() && Builder.IsValid()))
	{
		Builder->DisableTangents();
		Builder->EnableTangents(bUseHighIsValid ?
			RealtimeMesh::GetRealtimeMeshDataElementType<FPackedRGBA16N>() :
			RealtimeMesh::GetRealtimeMeshDataElementType<FPackedNormal>());
	}
	return this;
}

URealtimeMeshLocalBuilder* URealtimeMeshLocalBuilder::DisableTangents()
{
	if (ensure(Streams.IsValid() && Builder.IsValid()))
	{
		Builder->DisableTangents();
	}
	return this;
}

URealtimeMeshLocalBuilder* URealtimeMeshLocalBuilder::EnableColors()
{
	if (ensure(Streams.IsValid() && Builder.IsValid()))
	{
		Builder->DisableColors();
		Builder->EnableColors();
	}
	return this;
}

URealtimeMeshLocalBuilder* URealtimeMeshLocalBuilder::DisableColors()
{
	if (ensure(Streams.IsValid() && Builder.IsValid()))
	{
		Builder->DisableColors();
	}
	return this;
}

URealtimeMeshLocalBuilder* URealtimeMeshLocalBuilder::EnableTexCoords(int32 NumChannels, bool bUseHighPrecision)
{
	if (ensure(Streams.IsValid() && Builder.IsValid()))
	{
		Builder->DisableTexCoords();
		const auto TexCoordElementType = bUseHighPrecision ?
			RealtimeMesh::GetRealtimeMeshDataElementType<FVector2f>() :
			RealtimeMesh::GetRealtimeMeshDataElementType<FVector2DHalf>();

		Builder->EnableTexCoords(TexCoordElementType, FMath::Clamp(NumChannels, 1, 4));
	}
	return this;
}

URealtimeMeshLocalBuilder* URealtimeMeshLocalBuilder::DisableTexCoords()
{
	if (ensure(Streams.IsValid() && Builder.IsValid()))
	{
		Builder->DisableTexCoords();
	}
	return this;
}

URealtimeMeshLocalBuilder* URealtimeMeshLocalBuilder::EnableDepthOnlyTriangles(bool bUse32BitIndices)
{
	if (ensure(Streams.IsValid() && Builder.IsValid()))
	{
		Builder->DisableDepthOnlyTriangles();
		Builder->EnableDepthOnlyTriangles(bUse32BitIndices ?
			RealtimeMesh::GetRealtimeMeshDataElementType<uint32>() :
			RealtimeMesh::GetRealtimeMeshDataElementType<uint16>());
	}
	return this;
}

URealtimeMeshLocalBuilder* URealtimeMeshLocalBuilder::DisableDepthOnlyTriangles()
{
	if (ensure(Streams.IsValid() && Builder.IsValid()))
	{
		Builder->DisableDepthOnlyTriangles();
	}
	return this;
}

URealtimeMeshLocalBuilder* URealtimeMeshLocalBuilder::EnablePolyGroups(bool bUse32BitIndices)
{
	if (ensure(Streams.IsValid() && Builder.IsValid()))
	{
		Builder->DisablePolyGroups();
		Builder->EnablePolyGroups(bUse32BitIndices ?
			RealtimeMesh::GetRealtimeMeshDataElementType<uint32>() :
			RealtimeMesh::GetRealtimeMeshDataElementType<uint16>());
	}
	return this;
}

URealtimeMeshLocalBuilder* URealtimeMeshLocalBuilder::DisablePolyGroups()
{
	if (ensure(Streams.IsValid() && Builder.IsValid()))
	{
		Builder->DisablePolyGroups();
	}
	return this;
}

int32 URealtimeMeshLocalBuilder::AddTriangle(URealtimeMeshLocalBuilder*& OutBuilder, int32 UV0, int32 UV1, int32 UV2, int32 PolyGroupIndex)
{
	check(IsValid(this));
	OutBuilder = this;
	if (Builder.IsValid())
	{
		if (Builder->HasPolyGroups())
		{
			return Builder->AddTriangle(UV0, UV1, UV2, PolyGroupIndex);
		}
		return Builder->AddTriangle(UV0, UV1, UV2);
	}
	RMC_RATE_LIMIT_LOG({
		FMessageLog("RealtimeMesh").Error(LOCTEXT("MeshLocalBuilder_AddTriangle_InvalidBuilder", "AddTriangle: Builder not valid"));
	});
	return INDEX_NONE;
}

void URealtimeMeshLocalBuilder::SetTriangle(URealtimeMeshLocalBuilder*& OutBuilder, int32 Index, int32 UV0, int32 UV1, int32 UV2, int32 PolyGroupIndex)
{
	OutBuilder = this;
	if (Builder.IsValid())
	{
		Builder->SetTriangle(Index, UV0, UV1, UV2, PolyGroupIndex);
		return;
	}
	RMC_RATE_LIMIT_LOG({
		FMessageLog("RealtimeMesh").Error(LOCTEXT("MeshLocalBuilder_SetTriangle_InvalidBuilder", "SetTriangle: Builder not valid"));
	});
}

void URealtimeMeshLocalBuilder::GetTriangle(URealtimeMeshLocalBuilder*& OutBuilder, int32 Index, int32& UV0, int32& UV1, int32& UV2, int32& PolyGroupIndex)
{
	OutBuilder = this;
	if (Builder.IsValid())
	{
		const auto Triangle = Builder->GetTriangle(Index);
		UV0 = Triangle.V0;
		UV1 = Triangle.V1;
		UV2 = Triangle.V2;
		PolyGroupIndex = 0;
		if (Builder->HasPolyGroups())
		{
			PolyGroupIndex = Builder->GetPolyGroup(Index);
		}
		return;
	}

	UV0 = UV1 = UV2 = PolyGroupIndex = 0;
	RMC_RATE_LIMIT_LOG({
		FMessageLog("RealtimeMesh").Error(LOCTEXT("MeshLocalBuilder_GetTriangle_InvalidBuilder", "GetTriangle: Builder not valid"));
	});
}

int32 URealtimeMeshLocalBuilder::AddVertex(URealtimeMeshLocalBuilder*& OutBuilder, const FRealtimeMeshBasicVertex& InVertex)
{
	check(IsValid(this));
	OutBuilder = this;
	if (Builder.IsValid())
	{
		const int32 Row = Builder->AddVertex(FVector3f(InVertex.Position));

		if (Builder->HasTangents())
		{
			if (InVertex.Binormal.IsNearlyZero())
			{
				Builder->SetTangents(Row, FVector3f(InVertex.Normal), FVector3f(InVertex.Tangent));
			}
			else
			{
				Builder->SetTangents(Row, FVector3f(InVertex.Normal), FVector3f(InVertex.Binormal), FVector3f(InVertex.Tangent));
			}
		}

		if (Builder->HasColors())
		{
			Builder->SetColor(Row, InVertex.Color);
		}

		if (Builder->HasTexCoords())
		{
			const int32 NumChannels = Builder->NumTexCoordChannels();
			if (NumChannels > 0) Builder->SetTexCoord(Row, 0, FVector2f(InVertex.UV0));
			if (NumChannels > 1) Builder->SetTexCoord(Row, 1, FVector2f(InVertex.UV1));
			if (NumChannels > 2) Builder->SetTexCoord(Row, 2, FVector2f(InVertex.UV2));
			if (NumChannels > 3) Builder->SetTexCoord(Row, 3, FVector2f(InVertex.UV3));
		}

		return Row;
	}

	RMC_RATE_LIMIT_LOG({
		FMessageLog("RealtimeMesh").Error(LOCTEXT("MeshLocalBuilder_AddVertex_InvalidBuilder", "AddVertex: Builder not valid"));
	});
	return INDEX_NONE;
}

void URealtimeMeshLocalBuilder::EditVertex(URealtimeMeshLocalBuilder*& OutBuilder, int32 Index, FVector Position, bool bWritePosition, FVector Normal, bool bWriteNormal, FVector Tangent,
	bool bWriteTangent, FLinearColor Color, bool bWriteColor, FVector2D UV0, bool bWriteUV0, FVector2D UV1, bool bWriteUV1, FVector2D UV2, bool bWriteUV2, FVector2D UV3,
	bool bWriteUV3)
{
	OutBuilder = this;
	if (Builder.IsValid())
	{
		if (bWritePosition)
		{
			Builder->SetPosition(Index, FVector3f(Position));
		}

		// Normal & tangent share storage; if either flag is set, write both
		// (preserving the other one we're not touching).
		if ((bWriteNormal || bWriteTangent) && Builder->HasTangents())
		{
			const FVector3f N = bWriteNormal ? FVector3f(Normal) : Builder->GetNormal(Index);
			const FVector3f T = bWriteTangent ? FVector3f(Tangent) : Builder->GetTangent(Index);
			Builder->SetTangents(Index, N, T);
		}

		if (bWriteColor && Builder->HasColors())
		{
			Builder->SetColor(Index, Color);
		}

		const int32 NumChannels = Builder->NumTexCoordChannels();
		if (bWriteUV0 && NumChannels > 0) Builder->SetTexCoord(Index, 0, FVector2f(UV0));
		if (bWriteUV1 && NumChannels > 1) Builder->SetTexCoord(Index, 1, FVector2f(UV1));
		if (bWriteUV2 && NumChannels > 2) Builder->SetTexCoord(Index, 2, FVector2f(UV2));
		if (bWriteUV3 && NumChannels > 3) Builder->SetTexCoord(Index, 3, FVector2f(UV3));
		return;
	}

	RMC_RATE_LIMIT_LOG({
		FMessageLog("RealtimeMesh").Error(LOCTEXT("MeshLocalBuilder_EditVertex_InvalidBuilder", "EditVertex: Builder not valid"));
	});
}

void URealtimeMeshLocalBuilder::GetVertex(URealtimeMeshLocalBuilder*& OutBuilder, int32 Index, FVector& Position, FVector& Normal, FVector& Tangent, FLinearColor& Color, FVector2D& UV0,
	FVector2D& UV1, FVector2D& UV2, FVector2D& UV3)
{
	OutBuilder = this;
	if (Builder.IsValid())
	{
		Position = FVector(Builder->GetPosition(Index));

		if (Builder->HasTangents())
		{
			Normal = FVector(Builder->GetNormal(Index));
			Tangent = FVector(Builder->GetTangent(Index));
		}

		if (Builder->HasColors())
		{
			Color = Builder->GetColor(Index).ReinterpretAsLinear();
		}

		const int32 NumChannels = Builder->NumTexCoordChannels();
		UV0 = NumChannels > 0 ? FVector2D(Builder->GetTexCoord(Index, 0)) : FVector2D::ZeroVector;
		UV1 = NumChannels > 1 ? FVector2D(Builder->GetTexCoord(Index, 1)) : FVector2D::ZeroVector;
		UV2 = NumChannels > 2 ? FVector2D(Builder->GetTexCoord(Index, 2)) : FVector2D::ZeroVector;
		UV3 = NumChannels > 3 ? FVector2D(Builder->GetTexCoord(Index, 3)) : FVector2D::ZeroVector;
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
		auto Stream = CachedStreams.Pop(EAllowShrinking::No);
		Stream->Initialize(StreamKey, StreamType, NumElements);
		return Stream;
	}
	
	URealtimeMeshStream* NewStream = NewObject<URealtimeMeshStream>();
	NewStream->Initialize(StreamKey, StreamType, NumElements);

	// If we have allocated more streams than our safety threshold, drop our holds on the existing streams.
	// This will allow them to be garbage-collected (eventually)
	if (!ensure(AllCreatedStreams.Num() < CVarRealtimeMeshStreamPoolMaxSizeThreshold.GetValueOnGameThread()))
	{
		UE_LOG(LogRealtimeMesh, Warning, TEXT("URealtimeMeshStreamPool Threshold of %d Allocated Streams exceeded! Releasing references to all current streams and forcing a garbage collection."), CVarRealtimeMeshStreamPoolMaxSizeThreshold.GetValueOnGameThread());
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
		return CachedStreamSets.Pop(EAllowShrinking::No);
	}
	
	URealtimeMeshStreamSet* NewStreamSet = NewObject<URealtimeMeshStreamSet>();

	// If we have allocated more streams than our safety threshold, drop our holds on the existing streams.
	// This will allow them to be garbage-collected (eventually)
	if (!ensure(AllCreatedStreamSets.Num() < CVarRealtimeMeshStreamSetsPoolMaxSizeThreshold.GetValueOnGameThread()))
	{
		UE_LOG(LogRealtimeMesh, Warning, TEXT("URealtimeMeshStreamPool Threshold of %d Allocated StreamSets exceeded! Releasing references to all current streamssets and forcing a garbage collection."), CVarRealtimeMeshStreamSetsPoolMaxSizeThreshold.GetValueOnGameThread());
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
		return CachedBuilders.Pop(EAllowShrinking::No);
	}
	
	URealtimeMeshLocalBuilder* NewBuilder = NewObject<URealtimeMeshLocalBuilder>();

	// If we have allocated more streams than our safety threshold, drop our holds on the existing streams.
	// This will allow them to be garbage-collected (eventually)
	if (!ensure(AllCreatedBuilders.Num() < CVarRealtimeMeshBuilderPoolMaxSizeThreshold.GetValueOnGameThread()))
	{
		UE_LOG(LogRealtimeMesh, Warning, TEXT("URealtimeMeshStreamPool Threshold of %d Allocated Builders exceeded! Releasing references to all current builders and forcing a garbage collection."), CVarRealtimeMeshBuilderPoolMaxSizeThreshold.GetValueOnGameThread());
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

	// Components arrive with caller-controlled precision flags, so the builder
	// type is runtime-determined.
	RealtimeMesh::FRealtimeMeshDynamicBuilder Builder(Streams->GetStreamSet());

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

	Builder.ReserveAdditionalVertices(Components.Positions.Num());
	Builder.ReserveAdditionalTriangles(Components.Triangles.Num() / 3);

	for (int32 Index = 0; Index < Components.Positions.Num(); Index++)
	{
		const int32 Row = Builder.AddVertex(FVector3f(Components.Positions[Index]));

		if (Components.Normals.Num() > Row)
		{
			if (Components.Tangents.Num() > 0)
			{
				if (Components.Binormals.Num() > 0)
				{
					Builder.SetTangents(Row, FVector3f(Components.Normals[Index]), FVector3f(Components.Binormals[Index]), FVector3f(Components.Tangents[Index]));
				}
				else
				{
					Builder.SetTangents(Row, FVector3f(Components.Normals[Index]), FVector3f(Components.Tangents[Index]));
				}
			}
			else
			{
				// Only normal supplied — preserve whatever tangent is currently set.
				Builder.SetTangents(Row, FVector3f(Components.Normals[Index]), Builder.GetTangent(Row));
			}
		}

		if (Components.Colors.Num() > Row)
		{
			Builder.SetColor(Row, Components.Colors[Index]);
		}

		// Write each supplied UV channel through the dynamic builder.
		if (Builder.HasTexCoords())
		{
			const int32 NumChannels = Builder.NumTexCoordChannels();
			if (NumChannels > 0 && Components.UV0.Num() > Row) Builder.SetTexCoord(Row, 0, FVector2f(Components.UV0[Index]));
			if (NumChannels > 1 && Components.UV1.Num() > Row) Builder.SetTexCoord(Row, 1, FVector2f(Components.UV1[Index]));
			if (NumChannels > 2 && Components.UV2.Num() > Row) Builder.SetTexCoord(Row, 2, FVector2f(Components.UV2[Index]));
			if (NumChannels > 3 && Components.UV3.Num() > Row) Builder.SetTexCoord(Row, 3, FVector2f(Components.UV3[Index]));
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
