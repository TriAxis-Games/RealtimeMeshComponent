// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#include "RealtimeMeshLibrary.h"
#include "RealtimeMeshSimple.h"
#include "Core/RealtimeMeshDataStream.h"

using namespace RealtimeMesh;

FRealtimeMeshLODKey URealtimeMeshBlueprintFunctionLibrary::Conv_IntToRealtimeMeshLODKey(int32 LODIndex)
{
	return FRealtimeMeshLODKey(LODIndex);
}

FRealtimeMeshLODKey URealtimeMeshBlueprintFunctionLibrary::MakeLODKey(int32 LODIndex)
{
	return FRealtimeMeshLODKey(LODIndex);
}

FRealtimeMeshBufferSetKey URealtimeMeshBlueprintFunctionLibrary::MakeBufferSetKeyUnique(const FRealtimeMeshLODKey& LODKey)
{
	return FRealtimeMeshBufferSetKey::CreateUnique(LODKey);
}

FRealtimeMeshBufferSetKey URealtimeMeshBlueprintFunctionLibrary::MakeBufferSetKeyIndexed(const FRealtimeMeshLODKey& LODKey, int32 BufferSetIndex)
{
	return FRealtimeMeshBufferSetKey::Create(LODKey, BufferSetIndex);
}

FRealtimeMeshBufferSetKey URealtimeMeshBlueprintFunctionLibrary::MakeBufferSetKeyNamed(const FRealtimeMeshLODKey& LODKey, FName GroupName)
{
	return FRealtimeMeshBufferSetKey::Create(LODKey, GroupName);
}

FRealtimeMeshBufferSetKey URealtimeMeshBlueprintFunctionLibrary::MakeSectionGroupKeyUnique(const FRealtimeMeshLODKey& LODKey)
{
	return MakeBufferSetKeyUnique(LODKey);
}

FRealtimeMeshBufferSetKey URealtimeMeshBlueprintFunctionLibrary::MakeSectionGroupKeyIndexed(const FRealtimeMeshLODKey& LODKey, int32 SectionGroupIndex)
{
	return MakeBufferSetKeyIndexed(LODKey, SectionGroupIndex);
}

FRealtimeMeshBufferSetKey URealtimeMeshBlueprintFunctionLibrary::MakeSectionGroupKeyNamed(const FRealtimeMeshLODKey& LODKey, FName GroupName)
{
	return MakeBufferSetKeyNamed(LODKey, GroupName);
}

FRealtimeMeshSectionKey URealtimeMeshBlueprintFunctionLibrary::MakeSectionKeyUnique(const FRealtimeMeshBufferSetKey& SectionGroupKey)
{
	return FRealtimeMeshSectionKey::CreateUnique(SectionGroupKey);
}

FRealtimeMeshSectionKey URealtimeMeshBlueprintFunctionLibrary::MakeSectionKeyIndexed(const FRealtimeMeshBufferSetKey& SectionGroupKey, int32 SectionIndex)
{
	return FRealtimeMeshSectionKey::Create(SectionGroupKey, SectionIndex);
}

FRealtimeMeshSectionKey URealtimeMeshBlueprintFunctionLibrary::MakeSectionKeyNamed(const FRealtimeMeshBufferSetKey& SectionGroupKey, FName SectionName)
{
	return FRealtimeMeshSectionKey::Create(SectionGroupKey, SectionName);
}

FRealtimeMeshSectionKey URealtimeMeshBlueprintFunctionLibrary::MakeSectionKeyForPolygonGroup(const FRealtimeMeshBufferSetKey& SectionGroupKey, int32 PolygonGroup)
{
	return FRealtimeMeshSectionKey::CreateForPolyGroup(SectionGroupKey, PolygonGroup);
}

void URealtimeMeshBlueprintFunctionLibrary::BreakLODKey(const FRealtimeMeshLODKey& LODKey, int32& LODIndex)
{
	LODIndex = LODKey.Index();
}

FRealtimeMeshStreamRange URealtimeMeshBlueprintFunctionLibrary::MakeStreamRange(int32 VerticesLowerInclusive,
                                                                                int32 VerticesUpperExclusive, int32 IndicesLowerInclusive, int32 IndicesUpperExclusive)
{
	return FRealtimeMeshStreamRange(VerticesLowerInclusive, VerticesUpperExclusive, IndicesLowerInclusive, IndicesUpperExclusive);
}

FRealtimeMeshStreamKey URealtimeMeshBlueprintFunctionLibrary::MakeStreamKey(ERealtimeMeshStreamType StreamType, FName StreamName)
{
	return FRealtimeMeshStreamKey(StreamType, StreamName);
}

FRealtimeMeshStreamKey URealtimeMeshBlueprintFunctionLibrary::GetCommonStreamKey(ERealtimeMeshCommonStream StreamType)
{
	switch(StreamType)
	{
	case ERealtimeMeshCommonStream::Position:
		return FRealtimeMeshStreams::Position;
	case ERealtimeMeshCommonStream::Tangents:
		return FRealtimeMeshStreams::Tangents;
	case ERealtimeMeshCommonStream::TexCoords:
		return FRealtimeMeshStreams::TexCoords;
	case ERealtimeMeshCommonStream::Colors:
		return FRealtimeMeshStreams::Color;
	case ERealtimeMeshCommonStream::Triangles:
		return FRealtimeMeshStreams::Triangles;
	case ERealtimeMeshCommonStream::DepthOnlyTriangles:
		return FRealtimeMeshStreams::DepthOnlyTriangles;
	case ERealtimeMeshCommonStream::PolyGroups:
		return FRealtimeMeshStreams::PolyGroups;
	case ERealtimeMeshCommonStream::DepthOnlyPolyGroups:
		return FRealtimeMeshStreams::DepthOnlyPolyGroups;
	default:
		return FRealtimeMeshStreamKey();
	}
}

