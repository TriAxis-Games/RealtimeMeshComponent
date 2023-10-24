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

void FRealtimeMeshLocalBuilderResources::UpdateIfNecessary(URealtimeMeshStreamSet* StreamSet)
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
}


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
	ERealtimeMeshSimpleStreamConfig WantedTexCoords, ERealtimeMeshSimpleStreamConfig WantedTriangleSize,	ERealtimeMeshSimpleStreamConfig WantedPolyGroupType,
		bool bWantsColors, int32 WantedTexCoordChannels, bool bKeepExistingData)
{
	if (StreamSet)
	{
		auto* PositionStream = StreamSet->GetOrAddStream(RealtimeMesh::FRealtimeMeshStreams::Position, RealtimeMesh::GetRealtimeMeshBufferLayout<FVector3f>(1), bKeepExistingData);
		const int32 NumVertices = PositionStream->Num();

		
		if (WantedTangents == ERealtimeMeshSimpleStreamConfig::HighPrecision)
		{
			auto* Stream = StreamSet->GetOrAddStream(RealtimeMesh::FRealtimeMeshStreams::Tangents, RealtimeMesh::GetRealtimeMeshBufferLayout<FPackedRGBA16N>(2), bKeepExistingData);
			Stream->SetNumZeroed(NumVertices);
		}
		else if (WantedTangents == ERealtimeMeshSimpleStreamConfig::Normal)
		{		
			auto* Stream = StreamSet->GetOrAddStream(RealtimeMesh::FRealtimeMeshStreams::Tangents, RealtimeMesh::GetRealtimeMeshBufferLayout<FPackedNormal>(2), bKeepExistingData);
			Stream->SetNumZeroed(NumVertices);
		}

		if (bWantsColors)
		{
			auto* Stream = StreamSet->GetOrAddStream(RealtimeMesh::FRealtimeMeshStreams::Color, RealtimeMesh::GetRealtimeMeshBufferLayout<FColor>(1), bKeepExistingData);
			Stream->SetNumZeroed(NumVertices);
		}
	
		if (WantedTexCoords == ERealtimeMeshSimpleStreamConfig::HighPrecision)
		{
			auto* Stream = StreamSet->GetOrAddStream(RealtimeMesh::FRealtimeMeshStreams::TexCoords, RealtimeMesh::GetRealtimeMeshBufferLayout<FVector2f>(WantedTexCoordChannels), bKeepExistingData);
			Stream->SetNumZeroed(NumVertices);
		}
		else if (WantedTexCoords == ERealtimeMeshSimpleStreamConfig::Normal)
		{		
			auto* Stream = StreamSet->GetOrAddStream(RealtimeMesh::FRealtimeMeshStreams::TexCoords, RealtimeMesh::GetRealtimeMeshBufferLayout<FVector2DHalf>(WantedTexCoordChannels), bKeepExistingData);
			Stream->SetNumZeroed(NumVertices);
		}
	
		if (WantedTriangleSize == ERealtimeMeshSimpleStreamConfig::HighPrecision)
		{
			auto* Stream = StreamSet->GetOrAddStream(RealtimeMesh::FRealtimeMeshStreams::Triangles, RealtimeMesh::GetRealtimeMeshBufferLayout<uint32>(3), bKeepExistingData);
			Stream->SetNumZeroed(NumVertices);
		}
		else if (WantedTriangleSize == ERealtimeMeshSimpleStreamConfig::Normal)
		{		
			auto* Stream = StreamSet->GetOrAddStream(RealtimeMesh::FRealtimeMeshStreams::Triangles, RealtimeMesh::GetRealtimeMeshBufferLayout<uint16>(3), bKeepExistingData);
			Stream->SetNumZeroed(NumVertices);
		}
	
		if (WantedPolyGroupType == ERealtimeMeshSimpleStreamConfig::HighPrecision)
		{
			auto* Stream = StreamSet->GetOrAddStream(RealtimeMesh::FRealtimeMeshStreams::PolyGroups, RealtimeMesh::GetRealtimeMeshBufferLayout<uint32>(), bKeepExistingData);
			Stream->SetNumZeroed(NumVertices);
		}
		else if (WantedPolyGroupType == ERealtimeMeshSimpleStreamConfig::Normal)
		{		
			auto* Stream = StreamSet->GetOrAddStream(RealtimeMesh::FRealtimeMeshStreams::PolyGroups, RealtimeMesh::GetRealtimeMeshBufferLayout<uint16>(), bKeepExistingData);
			Stream->SetNumZeroed(NumVertices);
		}
		
		auto Resources = MakeShared<FRealtimeMeshLocalBuilderResources>();
		Resources->UpdateIfNecessary(StreamSet);	
		return FRealtimeMeshLocalBuilder(StreamSet, Resources);
	}
	
	RMC_RATE_LIMIT_LOG({
		FMessageLog("RealtimeMesh").Error(FText::Format(LOCTEXT("URealtimeMeshStreamUtils_MakeLocalMeshBuilder", "MakeLocalMeshBuilder({0}), Unable to make local mesh builder, no valid StreamSet"), FText::FromString(StreamSet->GetName())));
	});
	
	return FRealtimeMeshLocalBuilder(nullptr, nullptr);
}

const FRealtimeMeshLocalBuilder& URealtimeMeshStreamUtils::EnableTangents(const FRealtimeMeshLocalBuilder& Builder, bool bUseHighPrecision)
{	
	if (Builder.StreamSet)
	{
		if (bUseHighPrecision)
		{
			Builder.StreamSet->GetOrAddStream(RealtimeMesh::FRealtimeMeshStreams::Tangents, RealtimeMesh::GetRealtimeMeshBufferLayout<FPackedRGBA16N>(2), false);
		}
		else
		{		
			Builder.StreamSet->GetOrAddStream(RealtimeMesh::FRealtimeMeshStreams::Tangents, RealtimeMesh::GetRealtimeMeshBufferLayout<FPackedNormal>(2), false);
		}
		Builder.Resources->UpdateIfNecessary(Builder.StreamSet);

		if (Builder.Resources->Position.IsValid())
		{
			Builder.Resources->Tangents.StreamPtr->SetNumZeroed(Builder.Resources->Position.StreamPtr->Num());
		}
	}
	else
	{
		RMC_RATE_LIMIT_LOG({
			FMessageLog("RealtimeMesh").Error(FText::Format(LOCTEXT("URealtimeMeshStreamUtils_EnableTangents", "EnableTangents({0}), Unable to enable stream, no valid StreamSet"), FText::FromString(Builder.StreamSet->GetName())));
		});
	}
	return Builder;
}

const FRealtimeMeshLocalBuilder& URealtimeMeshStreamUtils::DisableTangents(const FRealtimeMeshLocalBuilder& Builder)
{
	if (Builder.StreamSet)
	{
		Builder.StreamSet->RemoveStream(RealtimeMesh::FRealtimeMeshStreams::Tangents);
	}
	else
	{
		RMC_RATE_LIMIT_LOG({
			FMessageLog("RealtimeMesh").Error(FText::Format(LOCTEXT("URealtimeMeshStreamUtils_DisableTangents", "DisableTangents({0}), Unable to enable stream, no valid StreamSet"), FText::FromString(Builder.StreamSet->GetName())));
		});
	}
	return Builder;
}

const FRealtimeMeshLocalBuilder& URealtimeMeshStreamUtils::EnableColors(const FRealtimeMeshLocalBuilder& Builder)
{
	if (Builder.StreamSet)
	{		
		Builder.StreamSet->GetOrAddStream(RealtimeMesh::FRealtimeMeshStreams::Color, RealtimeMesh::GetRealtimeMeshBufferLayout<FColor>(), false);
		Builder.Resources->UpdateIfNecessary(Builder.StreamSet);

		if (Builder.Resources->Colors.IsValid())
		{
			Builder.Resources->Colors.StreamPtr->SetNumZeroed(Builder.Resources->Position.StreamPtr->Num());
		}
	}
	else
	{
		RMC_RATE_LIMIT_LOG({
			FMessageLog("RealtimeMesh").Error(FText::Format(LOCTEXT("URealtimeMeshStreamUtils_EnableColors", "EnableColors({0}), Unable to enable stream, no valid StreamSet"), FText::FromString(Builder.StreamSet->GetName())));
		});
	}
	return Builder;
}

const FRealtimeMeshLocalBuilder& URealtimeMeshStreamUtils::DisableColors(const FRealtimeMeshLocalBuilder& Builder)
{
	if (Builder.StreamSet)
	{
		Builder.StreamSet->RemoveStream(RealtimeMesh::FRealtimeMeshStreams::Color);
	}
	else
	{
		RMC_RATE_LIMIT_LOG({
			FMessageLog("RealtimeMesh").Error(FText::Format(LOCTEXT("URealtimeMeshStreamUtils_DisableColors", "DisableColors({0}), Unable to enable stream, no valid StreamSet"), FText::FromString(Builder.StreamSet->GetName())));
		});
	}
	return Builder;
}

const FRealtimeMeshLocalBuilder& URealtimeMeshStreamUtils::EnableTexCoords(const FRealtimeMeshLocalBuilder& Builder, int32 NumChannels, bool bUseHighPrecision)
{
	if (Builder.StreamSet)
	{		
		if (bUseHighPrecision)
		{
			Builder.StreamSet->GetOrAddStream(RealtimeMesh::FRealtimeMeshStreams::TexCoords, RealtimeMesh::GetRealtimeMeshBufferLayout<FVector2f>(NumChannels), false);
		}
		else
		{		
			Builder.StreamSet->GetOrAddStream(RealtimeMesh::FRealtimeMeshStreams::TexCoords, RealtimeMesh::GetRealtimeMeshBufferLayout<FVector2DHalf>(NumChannels), false);
		}
		Builder.Resources->UpdateIfNecessary(Builder.StreamSet);
		
		if (Builder.Resources->TexCoords.IsValid())
		{
			Builder.Resources->TexCoords.StreamPtr->SetNumZeroed(Builder.Resources->Position.StreamPtr->Num());
		}
	}
	else
	{
		RMC_RATE_LIMIT_LOG({
			FMessageLog("RealtimeMesh").Error(FText::Format(LOCTEXT("URealtimeMeshStreamUtils_EnableTexCoords", "EnableTexCoords({0}), Unable to enable stream, no valid StreamSet"), FText::FromString(Builder.StreamSet->GetName())));
		});
	}
	return Builder;
}

const FRealtimeMeshLocalBuilder& URealtimeMeshStreamUtils::DisableTexCoords(const FRealtimeMeshLocalBuilder& Builder)
{
	if (Builder.StreamSet)
	{
		Builder.StreamSet->RemoveStream(RealtimeMesh::FRealtimeMeshStreams::TexCoords);
	}
	else
	{
		RMC_RATE_LIMIT_LOG({
			FMessageLog("RealtimeMesh").Error(FText::Format(LOCTEXT("URealtimeMeshStreamUtils_DisableTexCoords", "DisableTexCoords({0}), Unable to enable stream, no valid StreamSet"), FText::FromString(Builder.StreamSet->GetName())));
		});
	}
	return Builder;
}

const FRealtimeMeshLocalBuilder& URealtimeMeshStreamUtils::EnableDepthOnlyTriangles(const FRealtimeMeshLocalBuilder& Builder, bool bUse32BitIndices)
{
	if (Builder.StreamSet)
	{		
		if (bUse32BitIndices)
		{
			Builder.StreamSet->GetOrAddStream(RealtimeMesh::FRealtimeMeshStreams::Triangles, RealtimeMesh::GetRealtimeMeshBufferLayout<uint32>(3), false);
		}
		else
		{		
			Builder.StreamSet->GetOrAddStream(RealtimeMesh::FRealtimeMeshStreams::Triangles, RealtimeMesh::GetRealtimeMeshBufferLayout<uint16>(3), false);
		}
		Builder.Resources->UpdateIfNecessary(Builder.StreamSet);
	}
	else
	{
		RMC_RATE_LIMIT_LOG({
			FMessageLog("RealtimeMesh").Error(FText::Format(LOCTEXT("URealtimeMeshStreamUtils_EnableDepthOnlyTriangles", "EnableDepthOnlyTriangles({0}), Unable to enable stream, no valid StreamSet"), FText::FromString(Builder.StreamSet->GetName())));
		});
	}
	return Builder;
}

const FRealtimeMeshLocalBuilder& URealtimeMeshStreamUtils::DisableDepthOnlyTriangles(const FRealtimeMeshLocalBuilder& Builder)
{
	if (Builder.StreamSet)
	{
		Builder.StreamSet->RemoveStream(RealtimeMesh::FRealtimeMeshStreams::Triangles);
	}
	else
	{
		RMC_RATE_LIMIT_LOG({
			FMessageLog("RealtimeMesh").Error(FText::Format(LOCTEXT("URealtimeMeshStreamUtils_DisableDepthOnlyTriangles", "DisableDepthOnlyTriangles({0}), Unable to enable stream, no valid StreamSet"), FText::FromString(Builder.StreamSet->GetName())));
		});
	}
	return Builder;
}

const FRealtimeMeshLocalBuilder& URealtimeMeshStreamUtils::EnablePolyGroups(const FRealtimeMeshLocalBuilder& Builder, bool bUse32BitIndices)
{
	if (Builder.StreamSet)
	{		
		if (bUse32BitIndices)
		{
			Builder.StreamSet->GetOrAddStream(RealtimeMesh::FRealtimeMeshStreams::PolyGroups, RealtimeMesh::GetRealtimeMeshBufferLayout<uint32>(1), false);
		}
		else
		{		
			Builder.StreamSet->GetOrAddStream(RealtimeMesh::FRealtimeMeshStreams::PolyGroups, RealtimeMesh::GetRealtimeMeshBufferLayout<uint16>(1), false);
		}
		Builder.Resources->UpdateIfNecessary(Builder.StreamSet);

		if (Builder.Resources->Triangles.IsValid())
		{
			Builder.Resources->PolyGroups.StreamPtr->SetNumZeroed(Builder.Resources->Triangles.StreamPtr->Num());
		}
	}
	else
	{
		RMC_RATE_LIMIT_LOG({
			FMessageLog("RealtimeMesh").Error(FText::Format(LOCTEXT("URealtimeMeshStreamUtils_EnablePolyGroups", "EnablePolyGroups({0}), Unable to enable stream, no valid StreamSet"), FText::FromString(Builder.StreamSet->GetName())));
		});
	}
	return Builder;
}

const FRealtimeMeshLocalBuilder& URealtimeMeshStreamUtils::DisablePolyGroups(const FRealtimeMeshLocalBuilder& Builder)
{
	if (Builder.StreamSet)
	{
		Builder.StreamSet->RemoveStream(RealtimeMesh::FRealtimeMeshStreams::PolyGroups);
	}
	else
	{
		RMC_RATE_LIMIT_LOG({
			FMessageLog("RealtimeMesh").Error(FText::Format(LOCTEXT("URealtimeMeshStreamUtils_DisablePolyGroups", "DisablePolyGroups({0}), Unable to enable stream, no valid StreamSet"), FText::FromString(Builder.StreamSet->GetName())));
		});
	}
	return Builder;
}

const FRealtimeMeshLocalBuilder& URealtimeMeshStreamUtils::AddTriangle(const FRealtimeMeshLocalBuilder& Builder, int32& TriangleIndex, int32 UV0, int32 UV1, int32 UV2, int32 PolyGroupIndex)
{
	Builder.Resources->UpdateIfNecessary(Builder.StreamSet);
	if (Builder.Resources->Triangles.IsValid())
	{
		const auto StreamPtr = Builder.Resources->Triangles.StreamPtr;
		const auto Interface = Builder.Resources->Triangles.Interface;
		const int32 NewIndex = StreamPtr->AddUninitialized();
		Interface->SetInt(StreamPtr, NewIndex, 0, UV0);
		Interface->SetInt(StreamPtr, NewIndex, 1, UV1);
		Interface->SetInt(StreamPtr, NewIndex, 2, UV2);

		if (Builder.Resources->PolyGroups.IsValid())
		{
			Builder.Resources->PolyGroups.StreamPtr->SetNumZeroed(StreamPtr->Num());
			Interface->SetInt(Builder.Resources->PolyGroups.StreamPtr, NewIndex, 0, PolyGroupIndex);
		}
		TriangleIndex = NewIndex;
	}
	else
	{
		TriangleIndex = INDEX_NONE;
		RMC_RATE_LIMIT_LOG({
			FMessageLog("RealtimeMesh").Error(FText::Format(LOCTEXT("URealtimeMeshStreamUtils_AddTriangle", "AddTriangle({0}), No valid triangle stream"), FText::FromString(Builder.StreamSet->GetName())));
		});
	}

	return Builder;
}

const FRealtimeMeshLocalBuilder& URealtimeMeshStreamUtils::SetTriangle(const FRealtimeMeshLocalBuilder& Builder, int32 Index, int32 UV0, int32 UV1, int32 UV2, int32 PolyGroupIndex)
{
	Builder.Resources->UpdateIfNecessary(Builder.StreamSet);
	if (Builder.Resources->Triangles.IsValid())
	{
		const auto StreamPtr = Builder.Resources->Triangles.StreamPtr;
		if (StreamPtr->IsValidIndex(Index))
		{
			const auto Interface = Builder.Resources->Triangles.Interface;
			Interface->SetInt(StreamPtr, Index, 0, UV0);
			Interface->SetInt(StreamPtr, Index, 1, UV1);
			Interface->SetInt(StreamPtr, Index, 2, UV2);

			if (Builder.Resources->PolyGroups.IsValid())
			{
				if (!Builder.Resources->PolyGroups.StreamPtr->IsValidIndex(Index))
				{
					Builder.Resources->PolyGroups.StreamPtr->SetNumZeroed(StreamPtr->Num());
				}
				Interface->SetInt(Builder.Resources->PolyGroups.StreamPtr, Index, 0, PolyGroupIndex);
			}
		}
		else
		{
			RMC_RATE_LIMIT_LOG({
				FMessageLog("RealtimeMesh").Error(FText::Format(LOCTEXT("URealtimeMeshStreamUtils_SetTriangle", "SetTriangle({0}), Invalid triangle index"), FText::FromString(Builder.StreamSet->GetName())));
			});
		}
	}
	else
	{
		RMC_RATE_LIMIT_LOG({
			FMessageLog("RealtimeMesh").Error(FText::Format(LOCTEXT("URealtimeMeshStreamUtils_SetTriangle", "SetTriangle({0}), No valid triangle stream"), FText::FromString(Builder.StreamSet->GetName())));
		});
	}

	return Builder;
}

const FRealtimeMeshLocalBuilder& URealtimeMeshStreamUtils::GetTriangle(const FRealtimeMeshLocalBuilder& Builder, int32 Index, int32& UV0, int32& UV1, int32& UV2, int32& PolyGroupIndex)
{
	Builder.Resources->UpdateIfNecessary(Builder.StreamSet);
	if (Builder.Resources->Triangles.IsValid())
	{
		const auto StreamPtr = Builder.Resources->Triangles.StreamPtr;
		if (StreamPtr->IsValidIndex(Index))
		{
			const auto Interface = Builder.Resources->Triangles.Interface;
			Interface->SetInt(StreamPtr, Index, 0, UV0);
			Interface->SetInt(StreamPtr, Index, 1, UV1);
			Interface->SetInt(StreamPtr, Index, 2, UV2);
			UV0 = Interface->GetInt(StreamPtr, Index, 0);
			UV1 = Interface->GetInt(StreamPtr, Index, 1);
			UV2 = Interface->GetInt(StreamPtr, Index, 2);

			if (Builder.Resources->PolyGroups.IsValid())
			{
				if (Builder.Resources->PolyGroups.StreamPtr->IsValidIndex(Index))
				{
					const auto PolyGroupsInterface = Builder.Resources->Triangles.Interface;
					PolyGroupIndex = PolyGroupsInterface->GetInt(Builder.Resources->PolyGroups.StreamPtr, Index, 0);
				}
				else
				{
					PolyGroupIndex = 0;
					RMC_RATE_LIMIT_LOG({
						FMessageLog("RealtimeMesh").Error(FText::Format(LOCTEXT("URealtimeMeshStreamUtils_GetTriangle", "GetTriangle({0}), Invalid poly groups stream"), FText::FromString(Builder.StreamSet->GetName())));
					});
				}
			}
			else
			{
				PolyGroupIndex = 0;
			}
			return Builder;
		}
		else
		{
			RMC_RATE_LIMIT_LOG({
				FMessageLog("RealtimeMesh").Error(FText::Format(LOCTEXT("URealtimeMeshStreamUtils_GetTriangle", "GetTriangle({0}), Invalid triangle index"), FText::FromString(Builder.StreamSet->GetName())));
			});
		}
	}
	else
	{
		RMC_RATE_LIMIT_LOG({
			FMessageLog("RealtimeMesh").Error(FText::Format(LOCTEXT("URealtimeMeshStreamUtils_GetTriangle", "GetTriangle({0}), No valid triangle stream"), FText::FromString(Builder.StreamSet->GetName())));
		});
	}

	UV0 = UV1 = UV2 = 0;
	return Builder;
}


const FRealtimeMeshLocalBuilder& URealtimeMeshStreamUtils::AddVertex(const FRealtimeMeshLocalBuilder& Builder, FVector Position, FVector Normal,
                                                                    FVector Tangent, FLinearColor Color, FVector2D UV0, FVector2D UV1, FVector2D UV2, FVector2D UV3)
{
	using namespace RealtimeMesh;
	using namespace StreamBuilder::Private;
	
	int32 NewIndex = INDEX_NONE;

	if (Builder.StreamSet)
	{
		Builder.Resources->UpdateIfNecessary(Builder.StreamSet);

		Builder.Resources->Position.DoIfValid([&](FRealtimeMeshStream* Stream, IDataInterface* Interface)
		{
			NewIndex = Stream->AddUninitialized();
			Interface->SetVector3(Stream, NewIndex, 0, Position);		
		});

		if (NewIndex != INDEX_NONE)
		{
			Builder.Resources->Tangents.DoIfValid([&](FRealtimeMeshStream* Stream, IDataInterface* Interface)
			{
				Stream->SetNumZeroed(NewIndex + 1);
				Interface->SetVector3(Stream, NewIndex, 1, Tangent);		
				Interface->SetVector3(Stream, NewIndex, 0, Normal);		
			});
			
			Builder.Resources->TexCoords.DoIfValid([&](FRealtimeMeshStream* Stream, IDataInterface* Interface)
			{
				Stream->SetNumZeroed(NewIndex + 1);
				Interface->SetVector2(Stream, NewIndex, 0, UV0);
				if (Stream->GetNumElements() > 1)
				{
					Interface->SetVector2(Stream, NewIndex, 1, UV1);
					if (Stream->GetNumElements() > 2)
					{
						Interface->SetVector2(Stream, NewIndex, 2, UV2);
						if (Stream->GetNumElements() > 3)
						{
							Interface->SetVector2(Stream, NewIndex, 3, UV3);
						}
					}					
				}
			});
			
			Builder.Resources->Colors.DoIfValid([&](FRealtimeMeshStream* Stream, IDataInterface* Interface)
			{
				Stream->SetNumZeroed(NewIndex + 1);
				Interface->SetColor(Stream, NewIndex, 0, Color.ToFColor(true));
			});
		}
		else
		{
			RMC_RATE_LIMIT_LOG({
				FMessageLog("RealtimeMesh").Error(FText::Format(LOCTEXT("RealtimeMeshLocalBuilder_AddVertex", "AddVertex({0}), Unable to write new vertex, no position stream"), FText::FromString(Builder.StreamSet->GetName())));
			});
		}
	}
	else
	{
		RMC_RATE_LIMIT_LOG({
			FMessageLog("RealtimeMesh").Error(FText::Format(LOCTEXT("RealtimeMeshLocalBuilder_AddVertex", "AddVertex({0}), Unable to write vertex, no valid stream set"), FText::FromString(Builder.StreamSet->GetName())));
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
	
	if (Builder.StreamSet)
	{
		Builder.Resources->UpdateIfNecessary(Builder.StreamSet);

		if (bWritePosition)
		{
			Builder.Resources->Position.DoIfValid([&](FRealtimeMeshStream* Stream, IDataInterface* Interface)
			{
				if (Stream->IsValidIndex(Index))
				{
					Interface->SetVector3(Stream, Index, 0, Position);							
				}
				else
				{
					RMC_RATE_LIMIT_LOG({
						FMessageLog("RealtimeMesh").Error(FText::Format(LOCTEXT("RealtimeMeshLocalBuilder_EditVertex", "EditVertex({0}), unable to set position, invalid index"), FText::FromString(Builder.StreamSet->GetName())));
					});
				}
			});
		}
		
		if (bWriteNormal || bWriteTangent)
		{
			Builder.Resources->Tangents.DoIfValid([&](FRealtimeMeshStream* Stream, IDataInterface* Interface)
			{
				if (Stream->IsValidIndex(Index))
				{
					if (bWriteNormal)
					{
						Interface->SetVector3(Stream, Index, 1, Normal);						
					}

					if (bWriteTangent)
					{
						Interface->SetVector3(Stream, Index, 0, Tangent);						
					}
				}
				else
				{
					RMC_RATE_LIMIT_LOG({
						FMessageLog("RealtimeMesh").Error(FText::Format(LOCTEXT("RealtimeMeshLocalBuilder_EditVertex", "EditVertex({0}), unable to set normal/tangent, invalid index"), FText::FromString(Builder.StreamSet->GetName())));
					});
				}
			});
		}

		if (bWriteColor)
		{
			Builder.Resources->Colors.DoIfValid([&](FRealtimeMeshStream* Stream, IDataInterface* Interface)
			{
				if (Stream->IsValidIndex(Index))
				{
					Interface->SetColor(Stream, Index, 0, Color.ToFColor(true));							
				}
				else
				{
					RMC_RATE_LIMIT_LOG({
						FMessageLog("RealtimeMesh").Error(FText::Format(LOCTEXT("RealtimeMeshLocalBuilder_EditVertex", "EditVertex({0}), unable to set color, invalid index"), FText::FromString(Builder.StreamSet->GetName())));
					});
				}
			});
		}

		if (bWriteUV0 || bWriteUV1 || bWriteUV2 || bWriteUV3)
		{
			Builder.Resources->TexCoords.DoIfValid([&](FRealtimeMeshStream* Stream, IDataInterface* Interface)
			{
				if (Stream->IsValidIndex(Index))
				{
					if (bWriteUV0)
					{
						Interface->SetVector2(Stream, Index, 0, UV0);						
					}
					if (bWriteUV1 && Stream->GetNumElements() >= 2)
					{
						Interface->SetVector2(Stream, Index, 1, UV1);						
					}
					if (bWriteUV2 && Stream->GetNumElements() >= 3)
					{
						Interface->SetVector2(Stream, Index, 2, UV2);						
					}
					if (bWriteUV3 && Stream->GetNumElements() >= 4)
					{
						Interface->SetVector2(Stream, Index, 3, UV3);						
					}					
				}
				else
				{
					RMC_RATE_LIMIT_LOG({
						FMessageLog("RealtimeMesh").Error(FText::Format(LOCTEXT("RealtimeMeshLocalBuilder_EditVertex", "EditVertex({0}), unable to set tex coords, invalid index"), FText::FromString(Builder.StreamSet->GetName())));
					});
				}
			});
		}
	}
	
	return Builder;
}

const FRealtimeMeshLocalBuilder& URealtimeMeshStreamUtils::GetVertex(const FRealtimeMeshLocalBuilder& Builder, int32 Index, FVector& Position, FVector& Normal,
	FVector& Tangent, FLinearColor& Color, FVector2D& UV0, FVector2D& UV1, FVector2D& UV2, FVector2D& UV3)
{

	
	return Builder;
}


#undef LOCTEXT_NAMESPACE
