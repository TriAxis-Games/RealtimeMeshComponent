// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#include "RealtimeMeshLibrary.h"
#include "RealtimeMeshSimple.h"
#include "Mesh/RealtimeMeshDataStream.h"

FRealtimeMeshLODKey URealtimeMeshBlueprintFunctionLibrary::Conv_IntToRealtimeMeshLODKey(int32 LODIndex)
{
	return FRealtimeMeshLODKey(LODIndex);
}

FRealtimeMeshLODKey URealtimeMeshBlueprintFunctionLibrary::MakeLODKey(int32 LODIndex)
{
	return FRealtimeMeshLODKey(LODIndex);
}

FRealtimeMeshSectionGroupKey URealtimeMeshBlueprintFunctionLibrary::MakeSectionGroupKeyUnique(const FRealtimeMeshLODKey& LODKey)
{
	return FRealtimeMeshSectionGroupKey::CreateUnique(LODKey);
}

FRealtimeMeshSectionGroupKey URealtimeMeshBlueprintFunctionLibrary::MakeSectionGroupKeyIndexed(const FRealtimeMeshLODKey& LODKey, int32 SectionGroupIndex)
{
	return FRealtimeMeshSectionGroupKey::Create(LODKey, SectionGroupIndex);
}

FRealtimeMeshSectionGroupKey URealtimeMeshBlueprintFunctionLibrary::MakeSectionGroupKeyNamed(const FRealtimeMeshLODKey& LODKey, FName GroupName)
{
	return FRealtimeMeshSectionGroupKey::Create(LODKey, GroupName);
}

FRealtimeMeshSectionKey URealtimeMeshBlueprintFunctionLibrary::MakeSectionKeyUnique(const FRealtimeMeshSectionGroupKey& SectionGroupKey)
{
	return FRealtimeMeshSectionKey::CreateUnique(SectionGroupKey);
}

FRealtimeMeshSectionKey URealtimeMeshBlueprintFunctionLibrary::MakeSectionKeyIndexed(const FRealtimeMeshSectionGroupKey& SectionGroupKey, int32 SectionIndex)
{
	return FRealtimeMeshSectionKey::Create(SectionGroupKey, SectionIndex);
}

FRealtimeMeshSectionKey URealtimeMeshBlueprintFunctionLibrary::MakeSectionKeyNamed(const FRealtimeMeshSectionGroupKey& SectionGroupKey, FName SectionName)
{
	return FRealtimeMeshSectionKey::Create(SectionGroupKey, SectionName);
}

FRealtimeMeshSectionKey URealtimeMeshBlueprintFunctionLibrary::MakeSectionKeyForPolygonGroup(const FRealtimeMeshSectionGroupKey& SectionGroupKey, int32 PolygonGroup)
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

