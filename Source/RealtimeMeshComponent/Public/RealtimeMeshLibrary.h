// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RealtimeMeshSimple.h"
#include "Data/RealtimeMeshConfig.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RealtimeMeshLibrary.generated.h"


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
	static FRealtimeMeshSectionGroupKey MakeSectionGroupKey(FRealtimeMeshLODKey LODKey, int32 SectionGroupIndex);
	
	UFUNCTION(BlueprintPure, Category = "RealtimeMesh|Key")
	static FRealtimeMeshSectionKey MakeSectionKey(FRealtimeMeshSectionGroupKey SectionGroupKey, int32 SectionIndex);

	UFUNCTION(BlueprintPure, Category = "RealtimeMesh|Key")
	static void BreakLODKey(const FRealtimeMeshLODKey& LODKey, int32& LODIndex);

	UFUNCTION(BlueprintPure, Category = "RealtimeMesh|Stream")
	static FRealtimeMeshStreamRange MakeStreamRange(int32 VerticesLowerInclusive = 0, int32 VerticesUpperExclusive = 0, int32 IndicesLowerInclusive = 0, int32 IndicesUpperExclusive = 0);

	/** Generate vertex and index buffer for a simple box, given the supplied dimensions. Normals, UVs and tangents are also generated for each vertex. */
	UFUNCTION(BlueprintCallable, Category = "RealtimeMesh|MeshGeneration")
	static FRealtimeMeshSimpleMeshData& AppendBoxMesh(FVector BoxRadius, FTransform BoxTransform, UPARAM(Ref) FRealtimeMeshSimpleMeshData& MeshData);

	/** Generate vertex and index buffer for a simple box, given the supplied dimensions. Normals, UVs and tangents are also generated for each vertex. */
	UFUNCTION(BlueprintCallable, Category = "RealtimeMesh|MeshGeneration", meta=(AutoCreateRefTerm="Transform"))
	static FRealtimeMeshSimpleMeshData& AppendMesh(UPARAM(Ref) FRealtimeMeshSimpleMeshData& TargetMeshData, const FRealtimeMeshSimpleMeshData& MeshDataToAdd, const FTransform& Transform);
	
};