// Copyright TriAxis Games, L.L.C. All Rights Reserved.


#include "Mesh/RealtimeMeshBlueprintMeshBuilder.h"
#include "Mesh/RealtimeMeshDataStream.h"
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

void RealtimeMesh::StreamBuilder::Private::IDataInterface::SetInt(RealtimeMesh::FRealtimeMeshStream* Stream, int32 Index, int32 ElementIdx, int32 NewValue)
{
	RMC_RATE_LIMIT_LOG({
		FMessageLog("RealtimeMesh").Error(FText::Format(LOCTEXT("StreamBuilder_SetInt", "RealtimeMeshStream: Unable to SetInt on stream of type {0}"), FText::FromString(GetTypeName())));
	});
}

void RealtimeMesh::StreamBuilder::Private::IDataInterface::SetLong(RealtimeMesh::FRealtimeMeshStream* Stream, int32 Index, int32 ElementIdx, int64 NewValue)
{
	RMC_RATE_LIMIT_LOG({
		FMessageLog("RealtimeMesh").Error(FText::Format(LOCTEXT("StreamBuilder_SetLong", "RealtimeMeshStream: Unable to SetLong on stream of type {0}"), FText::FromString(GetTypeName())));
	});
}

void RealtimeMesh::StreamBuilder::Private::IDataInterface::SetFloat(RealtimeMesh::FRealtimeMeshStream* Stream, int32 Index, int32 ElementIdx, float NewValue)
{
	RMC_RATE_LIMIT_LOG({
		FMessageLog("RealtimeMesh").Error(FText::Format(LOCTEXT("StreamBuilder_SetFloat", "RealtimeMeshStream: Unable to SetFloat on stream of type {0}"), FText::FromString(GetTypeName())));
	});
}

void RealtimeMesh::StreamBuilder::Private::IDataInterface::SetVector2(RealtimeMesh::FRealtimeMeshStream* Stream, int32 Index, int32 ElementIdx, const FVector2D& NewValue)
{
	RMC_RATE_LIMIT_LOG({
		FMessageLog("RealtimeMesh").Error(FText::Format(LOCTEXT("StreamBuilder_SetVector2", "RealtimeMeshStream: Unable to SetVector2 on stream of type {0}"), FText::FromString(GetTypeName())));
	});
}

void RealtimeMesh::StreamBuilder::Private::IDataInterface::SetVector3(RealtimeMesh::FRealtimeMeshStream* Stream, int32 Index, int32 ElementIdx, const FVector& NewValue)
{
	RMC_RATE_LIMIT_LOG({
		FMessageLog("RealtimeMesh").Error(FText::Format(LOCTEXT("StreamBuilder_SetVector3", "RealtimeMeshStream: Unable to SetVector3 on stream of type {0}"), FText::FromString(GetTypeName())));
	});
}

void RealtimeMesh::StreamBuilder::Private::IDataInterface::SetVector4(RealtimeMesh::FRealtimeMeshStream* Stream, int32 Index, int32 ElementIdx, const FVector4& NewValue)
{
	RMC_RATE_LIMIT_LOG({
		FMessageLog("RealtimeMesh").Error(FText::Format(LOCTEXT("StreamBuilder_SetVector4", "RealtimeMeshStream: Unable to SetVector4 on stream of type {0}"), FText::FromString(GetTypeName())));
	});
}

void RealtimeMesh::StreamBuilder::Private::IDataInterface::SetColor(FRealtimeMeshStream* Stream, int32 Index, int32 ElementIdx, const FLinearColor& NewValue)
{
	RMC_RATE_LIMIT_LOG({
		FMessageLog("RealtimeMesh").Error(FText::Format(LOCTEXT("StreamBuilder_SetColor", "RealtimeMeshStream: Unable to SetColor on stream of type {0}"), FText::FromString(GetTypeName())));
	});
}

int32 RealtimeMesh::StreamBuilder::Private::IDataInterface::GetInt(RealtimeMesh::FRealtimeMeshStream* Stream, int32 Index, int32 ElementIdx)
{
	RMC_RATE_LIMIT_LOG({
		FMessageLog("RealtimeMesh").Error(FText::Format(LOCTEXT("StreamBuilder_GetInt", "RealtimeMeshStream: Unable to GetInt on stream of type {0}"), FText::FromString(GetTypeName())));
	});
	return 0;
}

int64 RealtimeMesh::StreamBuilder::Private::IDataInterface::GetLong(RealtimeMesh::FRealtimeMeshStream* Stream, int32 Index, int32 ElementIdx)
{
	RMC_RATE_LIMIT_LOG({
		FMessageLog("RealtimeMesh").Error(FText::Format(LOCTEXT("StreamBuilder_GetLong", "RealtimeMeshStream: Unable to GetLong on stream of type {0}"), FText::FromString(GetTypeName())));
	});
	return 0;
}

float RealtimeMesh::StreamBuilder::Private::IDataInterface::GetFloat(RealtimeMesh::FRealtimeMeshStream* Stream, int32 Index, int32 ElementIdx)
{
	RMC_RATE_LIMIT_LOG({
		FMessageLog("RealtimeMesh").Error(FText::Format(LOCTEXT("StreamBuilder_GetFloat", "RealtimeMeshStream: Unable to GetFloat on stream of type {0}"), FText::FromString(GetTypeName())));
	});
	return 0.0f;
}

FVector2D RealtimeMesh::StreamBuilder::Private::IDataInterface::GetVector2(RealtimeMesh::FRealtimeMeshStream* Stream, int32 Index, int32 ElementIdx)
{
	RMC_RATE_LIMIT_LOG({
		FMessageLog("RealtimeMesh").Error(FText::Format(LOCTEXT("StreamBuilder_GetVector2", "RealtimeMeshStream: Unable to GetVector2 on stream of type {0}"), FText::FromString(GetTypeName())));
	});
	return FVector2d::ZeroVector;
}

FVector RealtimeMesh::StreamBuilder::Private::IDataInterface::GetVector3(RealtimeMesh::FRealtimeMeshStream* Stream, int32 Index, int32 ElementIdx)
{
	RMC_RATE_LIMIT_LOG({
		FMessageLog("RealtimeMesh").Error(FText::Format(LOCTEXT("StreamBuilder_GetVector3", "RealtimeMeshStream: Unable to GetVector3 on stream of type {0}"), FText::FromString(GetTypeName())));
	});
	return FVector3d::ZeroVector;
}

FVector4 RealtimeMesh::StreamBuilder::Private::IDataInterface::GetVector4(RealtimeMesh::FRealtimeMeshStream* Stream, int32 Index, int32 ElementIdx)
{
	RMC_RATE_LIMIT_LOG({
		FMessageLog("RealtimeMesh").Error(FText::Format(LOCTEXT("StreamBuilder_GetVector4", "RealtimeMeshStream: Unable to GetVector4 on stream of type {0}"), FText::FromString(GetTypeName())));
	});
	return FVector4d(0, 0, 0, 0);
}

FLinearColor RealtimeMesh::StreamBuilder::Private::IDataInterface::GetColor(FRealtimeMeshStream* Stream, int32 Index, int32 ElementIdx)
{
	RMC_RATE_LIMIT_LOG({
		FMessageLog("RealtimeMesh").Error(FText::Format(LOCTEXT("StreamBuilder_GetColor", "RealtimeMeshStream: Unable to GetColor on stream of type {0}"), FText::FromString(GetTypeName())));
	});
	return FLinearColor::White;
}


void FRealtimeMeshStreamPtr::UpdateIfNecessary() const
{
	if (StreamSet)
	{
		if (StreamSet->GetChangeId() != ChangeId)
		{
			StreamState = RealtimeMesh::StreamBuilder::Private::GetStream(StreamSet->GetStreamSet(), StreamKey);
			ChangeId = StreamSet->GetChangeId();
		}
	}
	else if (StreamState.IsValid() || ChangeId != INDEX_NONE)
	{
		StreamState = RealtimeMesh::StreamBuilder::Private::FStreamState();
		ChangeId = INDEX_NONE;
	}
}

void FRealtimeMeshStreamRowPtr::UpdateIfNecessary() const
{
	if (StreamSet)
	{
		if (StreamSet->GetChangeId() != ChangeId)
		{
			StreamState = RealtimeMesh::StreamBuilder::Private::GetStream(StreamSet->GetStreamSet(), StreamKey);
			ChangeId = StreamSet->GetChangeId();
		}
	}
	else if (StreamState.IsValid() || ChangeId != INDEX_NONE)
	{
		StreamState = RealtimeMesh::StreamBuilder::Private::FStreamState();
		ChangeId = INDEX_NONE;
	}
}

/*void FRealtimeMeshLocalBuilderResources::UpdateIfNecessary(URealtimeMeshStreamSet* StreamSet)
{
	using namespace RealtimeMesh::StreamBuilder::Private;
	
	if (StreamSet)
	{
		if (StreamSet->GetChangeId() != ChangeId)
		{
			if (StreamSet)
			{
				auto& StreamSetRef = StreamSet->GetStreamSet();
				
				Position = GetStream(StreamSetRef, RealtimeMesh::FRealtimeMeshStreams::Position);
				Tangents = GetStream(StreamSetRef, RealtimeMesh::FRealtimeMeshStreams::Tangents);
				Colors = GetStream(StreamSetRef, RealtimeMesh::FRealtimeMeshStreams::Color);
				TexCoords = GetStream(StreamSetRef, RealtimeMesh::FRealtimeMeshStreams::TexCoords);
	
				Triangles = GetStream(StreamSetRef, RealtimeMesh::FRealtimeMeshStreams::Triangles);
				DepthOnlyTriangles = GetStream(StreamSetRef, RealtimeMesh::FRealtimeMeshStreams::DepthOnlyTriangles);
	
				PolyGroups = GetStream(StreamSetRef, RealtimeMesh::FRealtimeMeshStreams::PolyGroups);
				DepthOnlyPolyGroups = GetStream(StreamSetRef, RealtimeMesh::FRealtimeMeshStreams::DepthOnlyPolyGroups);

				ChangeId = StreamSet->GetChangeId();
			}
			else
			{
				Position = FStreamState(nullptr, nullptr, false);
				Tangents = FStreamState(nullptr, nullptr, false);
				Colors = FStreamState(nullptr, nullptr, false);
				TexCoords = FStreamState(nullptr, nullptr, false);
				
				Triangles = FStreamState(nullptr, nullptr, false);
				DepthOnlyTriangles = FStreamState(nullptr, nullptr, false);
				
				PolyGroups = FStreamState(nullptr, nullptr, false);
				DepthOnlyPolyGroups = FStreamState(nullptr, nullptr, false);

				ChangeId = INDEX_NONE;
			}
		}
	}
}*/


URealtimeMeshStreamSet* URealtimeMeshStreamUtils::MakeRealtimeMeshStreamSet()
{
	return NewObject<URealtimeMeshStreamSet>();
}

FRealtimeMeshStreamPtr URealtimeMeshStreamUtils::GetStream(URealtimeMeshStreamSet* StreamSet, FRealtimeMeshStreamKey StreamKey)
{
	const auto StreamState = RealtimeMesh::StreamBuilder::Private::GetStream(StreamSet->GetStreamSet(), StreamKey);

	return FRealtimeMeshStreamPtr(StreamSet, StreamKey, StreamState, StreamSet->GetChangeId());
}

FRealtimeMeshStreamPtr URealtimeMeshStreamUtils::AddStream(URealtimeMeshStreamSet* StreamSet, FRealtimeMeshStreamKey StreamKey, ERealtimeMeshSimpleStreamType StreamType, int32 NumElements)
{
	using namespace RealtimeMesh::StreamBuilder::Private;

	TSharedPtr<IDataInterface> Interface = nullptr;
	RealtimeMesh::FRealtimeMeshBufferLayout BufferLayout;
	switch(StreamType)
	{
	case ERealtimeMeshSimpleStreamType::Int16:
		Interface = MakeShared<TDataInterface<int16>>();
		BufferLayout = RealtimeMesh::GetRealtimeMeshBufferLayout<int16>();
		break;
	case ERealtimeMeshSimpleStreamType::UInt16:
		Interface = MakeShared<TDataInterface<uint16>>();
		BufferLayout = RealtimeMesh::GetRealtimeMeshBufferLayout<uint16>();
		break;
	case ERealtimeMeshSimpleStreamType::Int32:
		Interface = MakeShared<TDataInterface<int32>>();
		BufferLayout = RealtimeMesh::GetRealtimeMeshBufferLayout<int32>();
		break;
	case ERealtimeMeshSimpleStreamType::UInt32:
		Interface = MakeShared<TDataInterface<uint32>>();
		BufferLayout = RealtimeMesh::GetRealtimeMeshBufferLayout<uint32>();
		break;
	case ERealtimeMeshSimpleStreamType::Float:
		Interface = MakeShared<TDataInterface<float>>();
		BufferLayout = RealtimeMesh::GetRealtimeMeshBufferLayout<float>();
		break;
	case ERealtimeMeshSimpleStreamType::Vector2:
		Interface = MakeShared<TDataInterface<FVector2f>>();
		BufferLayout = RealtimeMesh::GetRealtimeMeshBufferLayout<FVector2f>();
		break;
	case ERealtimeMeshSimpleStreamType::Vector3:
		Interface = MakeShared<TDataInterface<FVector3f>>();
		BufferLayout = RealtimeMesh::GetRealtimeMeshBufferLayout<FVector3f>();
		break;
	case ERealtimeMeshSimpleStreamType::HalfVector2:
		Interface = MakeShared<TDataInterface<FVector2DHalf>>();
		BufferLayout = RealtimeMesh::GetRealtimeMeshBufferLayout<FVector2DHalf>();
		break;
	case ERealtimeMeshSimpleStreamType::PackedNormal:
		Interface = MakeShared<TDataInterface<FPackedNormal>>();
		BufferLayout = RealtimeMesh::GetRealtimeMeshBufferLayout<FPackedNormal>();
		break;
	case ERealtimeMeshSimpleStreamType::PackedRGBA16N:
		Interface = MakeShared<TDataInterface<FPackedRGBA16N>>();
		BufferLayout = RealtimeMesh::GetRealtimeMeshBufferLayout<FPackedRGBA16N>();
		break;
	case ERealtimeMeshSimpleStreamType::Triangle16:
		Interface = MakeShared<TDataInterface<RealtimeMesh::TIndex3<uint16>>>();
		BufferLayout = RealtimeMesh::GetRealtimeMeshBufferLayout<RealtimeMesh::TIndex3<uint16>>();
		break;
	case ERealtimeMeshSimpleStreamType::Triangle32:
		Interface = MakeShared<TDataInterface<RealtimeMesh::TIndex3<uint32>>>();
		BufferLayout = RealtimeMesh::GetRealtimeMeshBufferLayout<RealtimeMesh::TIndex3<uint32>>();
		break;
	default:
		break;
	}

	const auto StreamState = GetOrCreateStream(StreamSet->GetStreamSet(), StreamKey, BufferLayout);
	
	return FRealtimeMeshStreamPtr(StreamSet, StreamKey, StreamState, StreamSet->GetChangeId());
}

const FRealtimeMeshStreamPtr& URealtimeMeshStreamUtils::GetNum(const FRealtimeMeshStreamPtr& Stream, int32& Num)
{
	Stream.UpdateIfNecessary();
	if (Stream.StreamState.IsValid())
	{
		Num = Stream.StreamState.StreamPtr->Num();
	}
	else
	{		
		Num = 0;
	}
	return Stream;
}

const FRealtimeMeshStreamPtr& URealtimeMeshStreamUtils::IsIndexValid(const FRealtimeMeshStreamPtr& Stream, int32 Index, bool& bIsValid)
{
	Stream.UpdateIfNecessary();
	if (Stream.StreamState.IsValid())
	{
		bIsValid = Stream.StreamState.StreamPtr->IsValidIndex(Index);
	}
	else
	{
		bIsValid = false;
	}
	return Stream;
}

const FRealtimeMeshStreamPtr& URealtimeMeshStreamUtils::IsEmpty(const FRealtimeMeshStreamPtr& Stream, bool& bIsEmpty)
{
	Stream.UpdateIfNecessary();
	if (Stream.StreamState.IsValid())
	{
		bIsEmpty = Stream.StreamState.StreamPtr->IsEmpty();
	}
	else
	{
		bIsEmpty = false;
	}
	return Stream;
}

const FRealtimeMeshStreamPtr& URealtimeMeshStreamUtils::Reserve(const FRealtimeMeshStreamPtr& Stream, int32 Num)
{
	Stream.UpdateIfNecessary();
	if (Stream.StreamState.IsValid())
	{
		Stream.StreamState.StreamPtr->Reserve(Num);
	}
	return Stream;
}

const FRealtimeMeshStreamPtr& URealtimeMeshStreamUtils::Shrink(const FRealtimeMeshStreamPtr& Stream)
{
	Stream.UpdateIfNecessary();
	if (Stream.StreamState.IsValid())
	{
		Stream.StreamState.StreamPtr->Shrink();
	}
	return Stream;
}

const FRealtimeMeshStreamPtr& URealtimeMeshStreamUtils::Empty(const FRealtimeMeshStreamPtr& Stream, int32 ExpectedSize)
{
	Stream.UpdateIfNecessary();
	if (Stream.StreamState.IsValid())
	{
		Stream.StreamState.StreamPtr->Empty(ExpectedSize);
	}
	return Stream;
}

const FRealtimeMeshStreamPtr& URealtimeMeshStreamUtils::SetNumUninitialized(const FRealtimeMeshStreamPtr& Stream, int32 NewNum)
{
	Stream.UpdateIfNecessary();
	if (Stream.StreamState.IsValid())
	{
		Stream.StreamState.StreamPtr->SetNumUninitialized(NewNum);
	}
	return Stream;
}

const FRealtimeMeshStreamPtr& URealtimeMeshStreamUtils::SetNumZeroed(const FRealtimeMeshStreamPtr& Stream, int32 NewNum)
{
	Stream.UpdateIfNecessary();
	if (Stream.StreamState.IsValid())
	{
		Stream.StreamState.StreamPtr->SetNumZeroed(NewNum);
	}
	return Stream;
}

FRealtimeMeshStreamRowPtr URealtimeMeshStreamUtils::AddUninitialized(const FRealtimeMeshStreamPtr& Stream, int32 NumToAdd, int32& Index)
{
	Stream.UpdateIfNecessary();
	if (Stream.StreamState.IsValid())
	{
		Index = Stream.StreamState.StreamPtr->AddUninitialized(NumToAdd);
	}
	else
	{
		Index = INDEX_NONE;
	}
	return FRealtimeMeshStreamRowPtr(Stream, Index);
}

FRealtimeMeshStreamRowPtr URealtimeMeshStreamUtils::AddZeroed(const FRealtimeMeshStreamPtr& Stream, int32 NumToAdd, int32& Index)
{
	Stream.UpdateIfNecessary();
	if (Stream.StreamState.IsValid())
	{
		Index = Stream.StreamState.StreamPtr->AddZeroed(NumToAdd);
	}
	else
	{
		Index = INDEX_NONE;
	}
	return FRealtimeMeshStreamRowPtr(Stream, Index);
}

#define RMC_IMPLEMENT_SIMPLE_ADD(Type, CodeType) \
	FRealtimeMeshStreamRowPtr URealtimeMeshStreamUtils::Add##Type(const FRealtimeMeshStreamPtr& Stream, CodeType NewValue, int32& Index) \
	{ \
		Stream.UpdateIfNecessary(); \
		if (Stream.StreamState.IsValid()) \
		{ \
			Index = Stream.StreamState.StreamPtr->AddUninitialized(); \
			Stream.StreamState.Interface->Set##Type(Stream.StreamState.StreamPtr, Index, 0, NewValue); \
		} \
		else \
		{ \
			Index = INDEX_NONE; \
		} \
		return FRealtimeMeshStreamRowPtr(Stream, Index); \
	}

RMC_IMPLEMENT_SIMPLE_ADD(Int, int32);
RMC_IMPLEMENT_SIMPLE_ADD(Long, int64);
RMC_IMPLEMENT_SIMPLE_ADD(Float, float);
RMC_IMPLEMENT_SIMPLE_ADD(Vector2, FVector2D);
RMC_IMPLEMENT_SIMPLE_ADD(Vector3, FVector);
RMC_IMPLEMENT_SIMPLE_ADD(Vector4, FVector4);



#define RMC_IMPLEMENT_SIMPLE_SET(Type, CodeType) \
FRealtimeMeshStreamRowPtr URealtimeMeshStreamUtils::Set##Type(const FRealtimeMeshStreamPtr& Stream, int32 Index, CodeType NewValue) \
	{ \
		Stream.UpdateIfNecessary(); \
		if (Stream.StreamState.IsValid()) \
		{ \
			Stream.StreamState.Interface->Set##Type(Stream.StreamState.StreamPtr, Index, 0, NewValue); \
		} \
		return FRealtimeMeshStreamRowPtr(Stream, Index); \
	}

RMC_IMPLEMENT_SIMPLE_SET(Int, int32);
RMC_IMPLEMENT_SIMPLE_SET(Long, int64);
RMC_IMPLEMENT_SIMPLE_SET(Float, float);
RMC_IMPLEMENT_SIMPLE_SET(Vector2, FVector2D);
RMC_IMPLEMENT_SIMPLE_SET(Vector3, FVector);
RMC_IMPLEMENT_SIMPLE_SET(Vector4, FVector4);



#define RMC_IMPLEMENT_SIMPLE_GET(Type, CodeType, DefaultValue) \
FRealtimeMeshStreamRowPtr URealtimeMeshStreamUtils::Get##Type(const FRealtimeMeshStreamPtr& Stream, int32 Index, int32 ElementIdx, CodeType& Value) \
	{ \
		Stream.UpdateIfNecessary(); \
		if (Stream.StreamState.IsValid()) \
		{ \
			Value = Stream.StreamState.Interface->Get##Type(Stream.StreamState.StreamPtr, Index, ElementIdx); \
		} \
		else \
		{ \
			Value = DefaultValue; \
		} \
		return FRealtimeMeshStreamRowPtr(Stream, Index); \
	}

RMC_IMPLEMENT_SIMPLE_GET(Int, int32, 0);
RMC_IMPLEMENT_SIMPLE_GET(Long, int64, 0);
RMC_IMPLEMENT_SIMPLE_GET(Float, float, 0);
RMC_IMPLEMENT_SIMPLE_GET(Vector2, FVector2D, FVector2D());
RMC_IMPLEMENT_SIMPLE_GET(Vector3, FVector, FVector());
RMC_IMPLEMENT_SIMPLE_GET(Vector4, FVector4, FVector4())


#define RMC_IMPLEMENT_SIMPLE_STREAM_SET(Type, CodeType) \
	const FRealtimeMeshStreamRowPtr& URealtimeMeshStreamUtils::Set##Type##Element(const FRealtimeMeshStreamRowPtr& Stream, int32 Index, int32 ElementIdx, CodeType NewValue) \
	{ \
		Stream.UpdateIfNecessary(); \
		if (Stream.StreamState.IsValid()) \
		{ \
			Stream.StreamState.Interface->Set##Type(Stream.StreamState.StreamPtr, Index, ElementIdx, NewValue); \
		} \
		return Stream; \
	}

RMC_IMPLEMENT_SIMPLE_STREAM_SET(Int, int32);
RMC_IMPLEMENT_SIMPLE_STREAM_SET(Long, int64);
RMC_IMPLEMENT_SIMPLE_STREAM_SET(Float, float);
RMC_IMPLEMENT_SIMPLE_STREAM_SET(Vector2, FVector2D);
RMC_IMPLEMENT_SIMPLE_STREAM_SET(Vector3, FVector);
RMC_IMPLEMENT_SIMPLE_STREAM_SET(Vector4, FVector4);


#define RMC_IMPLEMENT_SIMPLE_STREAM_GET(Type, CodeType, DefaultValue) \
const FRealtimeMeshStreamRowPtr& URealtimeMeshStreamUtils::Get##Type##Element(const FRealtimeMeshStreamRowPtr& Stream, int32 Index, int32 ElementIdx, CodeType& Value) \
	{ \
		Stream.UpdateIfNecessary(); \
		if (Stream.StreamState.IsValid()) \
		{ \
			Value = Stream.StreamState.Interface->Get##Type(Stream.StreamState.StreamPtr, Index, ElementIdx); \
		} \
		else \
		{ \
			Value = DefaultValue; \
		} \
		return Stream; \
	}

RMC_IMPLEMENT_SIMPLE_STREAM_GET(Int, int32, 0);
RMC_IMPLEMENT_SIMPLE_STREAM_GET(Long, int64, 0);
RMC_IMPLEMENT_SIMPLE_STREAM_GET(Float, float, 0);
RMC_IMPLEMENT_SIMPLE_STREAM_GET(Vector2, FVector2D, FVector2D());
RMC_IMPLEMENT_SIMPLE_STREAM_GET(Vector3, FVector, FVector());
RMC_IMPLEMENT_SIMPLE_STREAM_GET(Vector4, FVector4, FVector4())


FRealtimeMeshLocalBuilder URealtimeMeshStreamUtils::MakeLocalMeshBuilder(URealtimeMeshStreamSet* StreamSet,	ERealtimeMeshSimpleStreamConfig WantedTangents,
	ERealtimeMeshSimpleStreamConfig WantedTexCoords, bool bWants32BitIndices, ERealtimeMeshSimpleStreamConfig WantedPolyGroupType,
		bool bWantsColors, int32 WantedTexCoordChannels, bool bKeepExistingData)
{
	using namespace RealtimeMesh;
	
	if (auto* TriangleStream = StreamSet->GetStream(FRealtimeMeshStreams::Triangles))
	{
		if (!bKeepExistingData)
		{
			TriangleStream->Empty();
		}
		
		if (bWants32BitIndices && TriangleStream->GetStride() < 3 * sizeof(uint32))
		{
			TriangleStream->ConvertTo<TIndex3<uint32>>();
		}
		else if (!bWants32BitIndices && TriangleStream->GetStride() > 3 * sizeof(uint16))
		{
			TriangleStream->ConvertTo<TIndex3<uint16>>();
		}
	}

	if (auto* TriangleStream = StreamSet->GetStream(FRealtimeMeshStreams::DepthOnlyTriangles))
	{
		if (!bKeepExistingData)
		{
			TriangleStream->Empty();
		}
		
		if (bWants32BitIndices && TriangleStream->GetStride() < 3 * sizeof(uint32))
		{
			TriangleStream->ConvertTo<TIndex3<uint32>>();
		}
		else if (!bWants32BitIndices && TriangleStream->GetStride() > 3 * sizeof(uint16))
		{
			TriangleStream->ConvertTo<TIndex3<uint16>>();
		}
	}
		
	FRealtimeMeshLocalBuilder NewBuilder(StreamSet);

	if (!bKeepExistingData)
	{
		NewBuilder.Builder->EmptyVertices();
		NewBuilder.Builder->EmptyTriangles();
		if (NewBuilder.Builder->HasDepthOnlyTriangles())
		{
			NewBuilder.Builder->EmptyDepthOnlyTriangles();
		}
	}

	// Setup colors
	if (bWantsColors)
	{
		NewBuilder.Builder->EnableColors();
	}

	// Setup tangents
	if (WantedTangents != ERealtimeMeshSimpleStreamConfig::None)
	{
		NewBuilder.Builder->EnableTangents(WantedTangents == ERealtimeMeshSimpleStreamConfig::HighPrecision ?
			RealtimeMesh::GetRealtimeMeshDataElementType<FPackedRGBA16N>() : 
			RealtimeMesh::GetRealtimeMeshDataElementType<FPackedNormal>());
	}

	// Setup tex coords
	if (WantedTexCoords != ERealtimeMeshSimpleStreamConfig::None && WantedTexCoordChannels > 0)
	{
		const auto TexCoordElementType = WantedTexCoords == ERealtimeMeshSimpleStreamConfig::HighPrecision ?
			RealtimeMesh::GetRealtimeMeshDataElementType<FVector2f>() : 
			RealtimeMesh::GetRealtimeMeshDataElementType<FVector2DHalf>();
		
		NewBuilder.Builder->EnableTexCoords(TexCoordElementType, FMath::Clamp(WantedTexCoordChannels, 1, 4));

		FRealtimeMeshStream* Stream = NewBuilder.StreamSet->GetStream(RealtimeMesh::FRealtimeMeshStreams::TexCoords);
		check(Stream);

		if (WantedTexCoordChannels > 1)
		{
			NewBuilder.UV1Builder = RealtimeMesh::TRealtimeMeshStridedStreamBuilder<FVector2f, void>(*Stream, 1);
		}

		if (WantedTexCoordChannels > 2)
		{
			NewBuilder.UV2Builder = RealtimeMesh::TRealtimeMeshStridedStreamBuilder<FVector2f, void>(*Stream, 2);
		}

		if (WantedTexCoordChannels > 3)
		{
			NewBuilder.UV3Builder = RealtimeMesh::TRealtimeMeshStridedStreamBuilder<FVector2f, void>(*Stream, 3);
		}
	}

	if (WantedPolyGroupType != ERealtimeMeshSimpleStreamConfig::None)
	{
		NewBuilder.Builder->EnablePolyGroups(WantedTangents == ERealtimeMeshSimpleStreamConfig::HighPrecision ?
			RealtimeMesh::GetRealtimeMeshDataElementType<uint32>() : 
			RealtimeMesh::GetRealtimeMeshDataElementType<uint16>());
	}

	return NewBuilder;
}

const FRealtimeMeshLocalBuilder& URealtimeMeshStreamUtils::EnableTangents(const FRealtimeMeshLocalBuilder& Builder, bool bUseHighPrecision)
{
	if (Builder.Builder.IsSet())
	{
		Builder.Builder->EnableTangents(bUseHighPrecision ?
			RealtimeMesh::GetRealtimeMeshDataElementType<FPackedRGBA16N>() : 
			RealtimeMesh::GetRealtimeMeshDataElementType<FPackedNormal>());
	}
	else
	{
		RMC_RATE_LIMIT_LOG({
			FMessageLog("RealtimeMesh").Error(LOCTEXT("MeshLocalBuilder_EnableTangents_InvalidBuilder", "EnableTangents: Builder not valid"));
		});
	}
	return Builder;
}

const FRealtimeMeshLocalBuilder& URealtimeMeshStreamUtils::DisableTangents(const FRealtimeMeshLocalBuilder& Builder)
{
	if (Builder.Builder.IsSet())
	{
		Builder.Builder->DisableTangents();
	}
	else
	{
		RMC_RATE_LIMIT_LOG({
			FMessageLog("RealtimeMesh").Error(LOCTEXT("MeshLocalBuilder_DisableTangents_InvalidBuilder", "DisableTangents: Builder not valid"));
		});
	}
	return Builder;
}

const FRealtimeMeshLocalBuilder& URealtimeMeshStreamUtils::EnableColors(const FRealtimeMeshLocalBuilder& Builder)
{
	if (Builder.Builder.IsSet())
	{
		Builder.Builder->EnableColors();
	}
	else
	{
		RMC_RATE_LIMIT_LOG({
			FMessageLog("RealtimeMesh").Error(LOCTEXT("MeshLocalBuilder_EnableColors_InvalidBuilder", "EnableColors: Builder not valid"));
		});
	}
	return Builder;
}

const FRealtimeMeshLocalBuilder& URealtimeMeshStreamUtils::DisableColors(const FRealtimeMeshLocalBuilder& Builder)
{
	if (Builder.Builder.IsSet())
	{
		Builder.Builder->DisableColors();
	}
	else
	{
		RMC_RATE_LIMIT_LOG({
			FMessageLog("RealtimeMesh").Error(LOCTEXT("MeshLocalBuilder_DisableColors_InvalidBuilder", "DisableColors: Builder not valid"));
		});
	}
	return Builder;
}

const FRealtimeMeshLocalBuilder& URealtimeMeshStreamUtils::EnableTexCoords(const FRealtimeMeshLocalBuilder& Builder, int32 NumChannels, bool bUseHighPrecision)
{
	if (Builder.Builder.IsSet())
	{
		Builder.Builder->EnableTexCoords(bUseHighPrecision?
			RealtimeMesh::GetRealtimeMeshDataElementType<FVector2f>() :
			RealtimeMesh::GetRealtimeMeshDataElementType<FVector2DHalf>(), NumChannels);
	
		auto* Stream = Builder.StreamSet->GetStream(RealtimeMesh::FRealtimeMeshStreams::TexCoords);
		check(Stream);
	
		if (NumChannels > 1)
		{
			Builder.UV1Builder = RealtimeMesh::TRealtimeMeshStridedStreamBuilder<FVector2f, void>(*Stream, 1);
		}
		else
		{
			Builder.UV1Builder.Reset();
		}

		if (NumChannels > 2)
		{
			Builder.UV2Builder = RealtimeMesh::TRealtimeMeshStridedStreamBuilder<FVector2f, void>(*Stream, 2);
		}
		else
		{
			Builder.UV2Builder.Reset();
		}

		if (NumChannels > 3)
		{
			Builder.UV3Builder = RealtimeMesh::TRealtimeMeshStridedStreamBuilder<FVector2f, void>(*Stream, 3);
		}
		else
		{
			Builder.UV3Builder.Reset();
		}
	}
	else
	{
		RMC_RATE_LIMIT_LOG({
			FMessageLog("RealtimeMesh").Error(LOCTEXT("MeshLocalBuilder_EnableTexCoords_InvalidBuilder", "EnableTexCoords: Builder not valid"));
		});
	}
	return Builder;
}

const FRealtimeMeshLocalBuilder& URealtimeMeshStreamUtils::DisableTexCoords(const FRealtimeMeshLocalBuilder& Builder)
{
	if (Builder.Builder.IsSet())
	{
		Builder.Builder->DisableTexCoords();
	}
	else
	{
		RMC_RATE_LIMIT_LOG({
			FMessageLog("RealtimeMesh").Error(LOCTEXT("MeshLocalBuilder_DisableTexCoords_InvalidBuilder", "DisableTexCoords: Builder not valid"));
		});
	}	
	return Builder;
}

const FRealtimeMeshLocalBuilder& URealtimeMeshStreamUtils::EnableDepthOnlyTriangles(const FRealtimeMeshLocalBuilder& Builder, bool bUse32BitIndices)
{
	if (Builder.Builder.IsSet())
	{
		Builder.Builder->EnableDepthOnlyTriangles(bUse32BitIndices? 
			RealtimeMesh::GetRealtimeMeshDataElementType<uint32>() : 
			RealtimeMesh::GetRealtimeMeshDataElementType<uint16>());
	}
	else
	{
		RMC_RATE_LIMIT_LOG({
			FMessageLog("RealtimeMesh").Error(LOCTEXT("MeshLocalBuilder_EnableDepthOnlyTriangles_InvalidBuilder", "EnableDepthOnlyTriangles: Builder not valid"));
		});
	}
	return Builder;
}

const FRealtimeMeshLocalBuilder& URealtimeMeshStreamUtils::DisableDepthOnlyTriangles(const FRealtimeMeshLocalBuilder& Builder)
{
	if (Builder.Builder.IsSet())
	{
		Builder.Builder->DisableDepthOnlyTriangles();
	}
	else
	{
		RMC_RATE_LIMIT_LOG({
			FMessageLog("RealtimeMesh").Error(LOCTEXT("MeshLocalBuilder_DisableDepthOnlyTriangles_InvalidBuilder", "DisableDepthOnlyTriangles: Builder not valid"));
		});
	}
	return Builder;
}

const FRealtimeMeshLocalBuilder& URealtimeMeshStreamUtils::EnablePolyGroups(const FRealtimeMeshLocalBuilder& Builder, bool bUse32BitIndices)
{
	if (Builder.Builder.IsSet())
	{
		Builder.Builder->EnablePolyGroups(bUse32BitIndices? 
			RealtimeMesh::GetRealtimeMeshDataElementType<uint32>() : 
			RealtimeMesh::GetRealtimeMeshDataElementType<uint16>());
	}
	else
	{
		RMC_RATE_LIMIT_LOG({
			FMessageLog("RealtimeMesh").Error(LOCTEXT("MeshLocalBuilder_EnablePolyGroups_InvalidBuilder", "EnablePolyGroups: Builder not valid"));
		});
	}
	return Builder;
}

const FRealtimeMeshLocalBuilder& URealtimeMeshStreamUtils::DisablePolyGroups(const FRealtimeMeshLocalBuilder& Builder)
{
	if (Builder.Builder.IsSet())
	{
		Builder.Builder->DisablePolyGroups();
	}
	else
	{
		RMC_RATE_LIMIT_LOG({
			FMessageLog("RealtimeMesh").Error(LOCTEXT("MeshLocalBuilder_DisablePolyGroups_InvalidBuilder", "DisablePolyGroups: Builder not valid"));
		});
	}
	return Builder;
}

const FRealtimeMeshLocalBuilder& URealtimeMeshStreamUtils::AddTriangle(const FRealtimeMeshLocalBuilder& Builder, int32& TriangleIndex, int32 UV0, int32 UV1, int32 UV2, int32 PolyGroupIndex)
{
	if (Builder.Builder.IsSet())
	{
		TriangleIndex = Builder.Builder->AddTriangle(UV0, UV1, UV2, PolyGroupIndex);
	}
	else
	{
		RMC_RATE_LIMIT_LOG({
			FMessageLog("RealtimeMesh").Error(LOCTEXT("MeshLocalBuilder_AddTriangle_InvalidBuilder", "AddTriangle: Builder not valid"));
		});
	}
	return Builder;
}

const FRealtimeMeshLocalBuilder& URealtimeMeshStreamUtils::SetTriangle(const FRealtimeMeshLocalBuilder& Builder, int32 Index, int32 UV0, int32 UV1, int32 UV2, int32 PolyGroupIndex)
{
	if (Builder.Builder.IsSet())
	{
		Builder.Builder->SetTriangle(Index, UV0, UV1, UV2, PolyGroupIndex);
	}
	else
	{
		RMC_RATE_LIMIT_LOG({
			FMessageLog("RealtimeMesh").Error(LOCTEXT("MeshLocalBuilder_SetTriangle_InvalidBuilder", "SetTriangle: Builder not valid"));
		});
	}
	return Builder;
}

const FRealtimeMeshLocalBuilder& URealtimeMeshStreamUtils::GetTriangle(const FRealtimeMeshLocalBuilder& Builder, int32 Index, int32& UV0, int32& UV1, int32& UV2, int32& PolyGroupIndex)
{
	if (Builder.Builder.IsSet())
	{
		const auto Triangle = Builder.Builder->GetTriangle(Index);
		UV0 = Triangle.V0;
		UV1 = Triangle.V1;
		UV2 = Triangle.V2;
		PolyGroupIndex = 0;
		if (Builder.Builder->HasPolyGroups())
		{
			PolyGroupIndex = Builder.Builder->GetMaterialIndex(Index);
		}
	}
	else
	{
		UV0 = UV1 = UV2 = PolyGroupIndex = 0;
		RMC_RATE_LIMIT_LOG({
			FMessageLog("RealtimeMesh").Error(LOCTEXT("MeshLocalBuilder_GetTriangle_InvalidBuilder", "GetTriangle: Builder not valid"));
		});
	}
	return Builder;
}


const FRealtimeMeshLocalBuilder& URealtimeMeshStreamUtils::AddVertex(const FRealtimeMeshLocalBuilder& Builder, int32& Index, FVector Position, FVector Normal,
                                                                    FVector Tangent, FLinearColor Color, FVector2D UV0, FVector2D UV1, FVector2D UV2, FVector2D UV3)
{
	using namespace RealtimeMesh;
	using namespace StreamBuilder::Private;
	
	if (Builder.Builder.IsSet())
	{
		auto Vertex = Builder.Builder->AddVertex(FVector3f(Position));
	
		if (Builder.Builder->HasTangents())
		{
			Vertex.SetNormalAndTangent(FVector3f(Normal), FVector3f(Tangent));
		}
	
		if (Builder.Builder->HasVertexColors())
		{
			Vertex.SetColor(Color);
		}
	
		if (Builder.Builder->HasTexCoords())
		{
			Vertex.SetTexCoord(0, FVector2f(UV0));

			if (Builder.UV1Builder.IsSet())
			{
				Builder.UV1Builder->SetElement(Vertex.GetIndex(), 1, FVector2f(UV1));
			}
			if (Builder.UV2Builder.IsSet())
			{
				Builder.UV2Builder->SetElement(Vertex.GetIndex(), 2, FVector2f(UV2));
			}
			if (Builder.UV3Builder.IsSet())
			{
				Builder.UV3Builder->SetElement(Vertex.GetIndex(), 3, FVector2f(UV3));
			}
		}

		Index = Vertex.GetIndex();
	}
	else
	{
		RMC_RATE_LIMIT_LOG({
			FMessageLog("RealtimeMesh").Error(LOCTEXT("MeshLocalBuilder_AddVertex_InvalidBuilder", "AddVertex: Builder not valid"));
		});
	}	
	return Builder;
}

const FRealtimeMeshLocalBuilder& URealtimeMeshStreamUtils::EditVertex(const FRealtimeMeshLocalBuilder& Builder, int32 Index, FVector Position, bool bWritePosition, FVector Normal,
	bool bWriteNormal, FVector Tangent, bool bWriteTangent, FLinearColor Color, bool bWriteColor, FVector2D UV0, bool bWriteUV0, FVector2D UV1, bool bWriteUV1, FVector2D UV2,
	bool bWriteUV2, FVector2D UV3, bool bWriteUV3)
{
	using namespace RealtimeMesh;
	using namespace StreamBuilder::Private;

	if (Builder.Builder.IsSet())
	{
		auto Vertex = Builder.Builder->EditVertex(Index);

		if (bWritePosition)
		{
			Vertex.SetPosition(FVector3f(Position));
		}
	
		if (bWriteNormal && Builder.Builder->HasTangents())
		{
			Vertex.SetNormal(FVector3f(Normal));
		}
		if (bWriteTangent && Builder.Builder->HasTangents())
		{
			Vertex.SetTangent(FVector3f(Tangent));
		}
	
		if (bWriteColor && Builder.Builder->HasVertexColors())
		{
			Vertex.SetColor(Color);
		}
	
		if (bWriteUV0 && Builder.Builder->HasTexCoords())
		{
			Vertex.SetTexCoord(0, FVector2f(UV0));
		}
		if (bWriteUV1 && Builder.UV1Builder.IsSet())
		{
			Builder.UV1Builder->Set(Vertex.GetIndex(), FVector2f(UV1));
		}
		if (bWriteUV2 && Builder.UV2Builder.IsSet())
		{
			Builder.UV2Builder->Set(Vertex.GetIndex(), FVector2f(UV2));
		}
		if (bWriteUV3 && Builder.UV3Builder.IsSet())
		{
			Builder.UV3Builder->Set(Vertex.GetIndex(), FVector2f(UV3));
		}
	}
	else
	{
		RMC_RATE_LIMIT_LOG({
			FMessageLog("RealtimeMesh").Error(LOCTEXT("MeshLocalBuilder_EditVertex_InvalidBuilder", "EditVertex: Builder not valid"));
		});
	}
	return Builder;
}

const FRealtimeMeshLocalBuilder& URealtimeMeshStreamUtils::GetVertex(const FRealtimeMeshLocalBuilder& Builder, int32 Index, FVector& Position, FVector& Normal,
	FVector& Tangent, FLinearColor& Color, FVector2D& UV0, FVector2D& UV1, FVector2D& UV2, FVector2D& UV3)
{
	if (Builder.Builder.IsSet())
	{
		const auto Vertex = Builder.Builder->EditVertex(Index);

		Position = FVector(Vertex.GetPosition());

		if (Builder.Builder->HasTangents())
		{
			Normal = FVector(Vertex.GetNormal());
			Tangent = FVector(Vertex.GetTangent());		
		}
	
		if (Builder.Builder->HasVertexColors())
		{
			Color = Vertex.GetLinearColor();		
		}

		if (Builder.Builder->HasTexCoords())
		{
			UV0 = FVector2D(Vertex.GetTexCoord(0));
		}

		if (Builder.UV1Builder.IsSet())
		{
			UV1 = FVector2D(Builder.UV1Builder->Get(Index).Get());
		}
		if (Builder.UV2Builder.IsSet())
		{
			UV2 = FVector2D(Builder.UV2Builder->Get(Index).Get());
		}
		if (Builder.UV3Builder.IsSet())
		{
			UV3 = FVector2D(Builder.UV3Builder->Get(Index).Get());
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
	return Builder;
}


#undef LOCTEXT_NAMESPACE
