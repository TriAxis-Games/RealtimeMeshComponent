// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "RealtimeMeshLibrary.generated.h"


UENUM()
enum class ERealtimeMeshCommonStream : uint8
{
	Unknown,
	Position,
	Tangents,
	TexCoords,
	Colors,
	Triangles,
	DepthOnlyTriangles,
	PolyGroups,
	DepthOnlyPolyGroups
};

UCLASS(meta=(ScriptName="RealtimeMeshLibrary"))
class REALTIMEMESHCOMPONENT_API URealtimeMeshBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "RealtimeMesh|Key", meta = (DisplayName = "LODIndex to LODKey", CompactNodeTitle = "->", BlueprintAutocast))
	static FRealtimeMeshLODKey Conv_IntToRealtimeMeshLODKey(int32 LODIndex);

	UFUNCTION(BlueprintPure, Category = "RealtimeMesh|Key")
	static FRealtimeMeshLODKey MakeLODKey(int32 LODIndex);


	UFUNCTION(BlueprintPure, Category = "RealtimeMesh|Key")
	static FRealtimeMeshBufferSetKey MakeBufferSetKeyUnique(const FRealtimeMeshLODKey& LODKey);

	UFUNCTION(BlueprintPure, Category = "RealtimeMesh|Key")
	static FRealtimeMeshBufferSetKey MakeBufferSetKeyIndexed(const FRealtimeMeshLODKey& LODKey, int32 BufferSetIndex);

	UFUNCTION(BlueprintPure, Category = "RealtimeMesh|Key")
	static FRealtimeMeshBufferSetKey MakeBufferSetKeyNamed(const FRealtimeMeshLODKey& LODKey, FName GroupName);

	// --- Deprecated SectionGroup-terminology aliases (use the MakeBufferSetKey* forms) ---

	UFUNCTION(BlueprintPure, Category = "RealtimeMesh|Key", meta = (DeprecatedFunction, DeprecationMessage = "Use MakeBufferSetKeyUnique"))
	static FRealtimeMeshBufferSetKey MakeSectionGroupKeyUnique(const FRealtimeMeshLODKey& LODKey);

	UFUNCTION(BlueprintPure, Category = "RealtimeMesh|Key", meta = (DeprecatedFunction, DeprecationMessage = "Use MakeBufferSetKeyIndexed"))
	static FRealtimeMeshBufferSetKey MakeSectionGroupKeyIndexed(const FRealtimeMeshLODKey& LODKey, int32 SectionGroupIndex);

	UFUNCTION(BlueprintPure, Category = "RealtimeMesh|Key", meta = (DeprecatedFunction, DeprecationMessage = "Use MakeBufferSetKeyNamed"))
	static FRealtimeMeshBufferSetKey MakeSectionGroupKeyNamed(const FRealtimeMeshLODKey& LODKey, FName GroupName);


	UFUNCTION(BlueprintPure, Category = "RealtimeMesh|Key")
	static FRealtimeMeshSectionKey MakeSectionKeyUnique(const FRealtimeMeshBufferSetKey& SectionGroupKey);

	UFUNCTION(BlueprintPure, Category = "RealtimeMesh|Key")
	static FRealtimeMeshSectionKey MakeSectionKeyIndexed(const FRealtimeMeshBufferSetKey& SectionGroupKey, int32 SectionIndex);

	UFUNCTION(BlueprintPure, Category = "RealtimeMesh|Key")
	static FRealtimeMeshSectionKey MakeSectionKeyNamed(const FRealtimeMeshBufferSetKey& SectionGroupKey, FName SectionName);

	UFUNCTION(BlueprintPure, Category = "RealtimeMesh|Stream")
	static FRealtimeMeshSectionKey MakeSectionKeyForPolygonGroup(const FRealtimeMeshBufferSetKey& SectionGroupKey, int32 PolygonGroup);

	
	UFUNCTION(BlueprintPure, Category = "RealtimeMesh|Key")
	static void BreakLODKey(const FRealtimeMeshLODKey& LODKey, int32& LODIndex);

	UFUNCTION(BlueprintPure, Category = "RealtimeMesh|Stream")
	static FRealtimeMeshStreamRange MakeStreamRange(int32 VerticesLowerInclusive = 0, int32 VerticesUpperExclusive = 0, int32 IndicesLowerInclusive = 0, int32 IndicesUpperExclusive = 0);

	UFUNCTION(BlueprintPure, Category = "RealtimeMesh|Key")
	static FRealtimeMeshStreamKey MakeStreamKey(ERealtimeMeshStreamType StreamType, FName StreamName);

	UFUNCTION(BlueprintPure, Category = "RealtimeMesh|Key")
	static FRealtimeMeshStreamKey GetCommonStreamKey(ERealtimeMeshCommonStream StreamType);
};
