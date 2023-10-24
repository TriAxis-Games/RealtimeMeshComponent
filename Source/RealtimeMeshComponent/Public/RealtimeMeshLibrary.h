// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "RealtimeMeshConfig.h"
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
	static FRealtimeMeshSectionGroupKey MakeSectionGroupKeyUnique(const FRealtimeMeshLODKey& LODKey);

	UFUNCTION(BlueprintPure, Category = "RealtimeMesh|Key")
	static FRealtimeMeshSectionGroupKey MakeSectionGroupKeyIndexed(const FRealtimeMeshLODKey& LODKey, int32 SectionGroupIndex);

	UFUNCTION(BlueprintPure, Category = "RealtimeMesh|Key")
	static FRealtimeMeshSectionGroupKey MakeSectionGroupKeyNamed(const FRealtimeMeshLODKey& LODKey, FName GroupName);


	UFUNCTION(BlueprintPure, Category = "RealtimeMesh|Key")
	static FRealtimeMeshSectionKey MakeSectionKeyUnique(const FRealtimeMeshSectionGroupKey& SectionGroupKey);

	UFUNCTION(BlueprintPure, Category = "RealtimeMesh|Key")
	static FRealtimeMeshSectionKey MakeSectionKeyIndexed(const FRealtimeMeshSectionGroupKey& SectionGroupKey, int32 SectionIndex);

	UFUNCTION(BlueprintPure, Category = "RealtimeMesh|Key")
	static FRealtimeMeshSectionKey MakeSectionKeyNamed(const FRealtimeMeshSectionGroupKey& SectionGroupKey, FName SectionName);

	UFUNCTION(BlueprintPure, Category = "RealtimeMesh|Stream")
	static FRealtimeMeshSectionKey MakeSectionKeyForPolygonGroup(const FRealtimeMeshSectionGroupKey& SectionGroupKey, int32 PolygonGroup);

	
	UFUNCTION(BlueprintPure, Category = "RealtimeMesh|Key")
	static void BreakLODKey(const FRealtimeMeshLODKey& LODKey, int32& LODIndex);

	UFUNCTION(BlueprintPure, Category = "RealtimeMesh|Stream")
	static FRealtimeMeshStreamRange MakeStreamRange(int32 VerticesLowerInclusive = 0, int32 VerticesUpperExclusive = 0, int32 IndicesLowerInclusive = 0, int32 IndicesUpperExclusive = 0);

	UFUNCTION(BlueprintPure, Category = "RealtimeMesh|Key")
	static FRealtimeMeshStreamKey MakeStreamKey(ERealtimeMeshStreamType StreamType, FName StreamName);

	UFUNCTION(BlueprintPure, Category = "RealtimeMesh|Key")
	static FRealtimeMeshStreamKey GetCommonStreamKey(ERealtimeMeshCommonStream StreamType);
};
