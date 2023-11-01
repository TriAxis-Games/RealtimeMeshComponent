// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RealtimeMeshSimple.h"
#include "RealtimeMeshBasicShapeTools.generated.h"

struct FRealtimeMeshSimpleMeshData;

/**
 * 
 */
UCLASS(meta=(ScriptName="RealtimeMeshBasicShapeTools"))
class REALTIMEMESHCOMPONENT_API URealtimeMeshSimpleBasicShapeTools : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	
	/** Generate vertex and index buffer for a simple box, given the supplied dimensions. Normals, UVs and tangents are also generated for each vertex. */
	UFUNCTION(BlueprintCallable, Category = "RealtimeMesh|MeshGeneration")
	static FRealtimeMeshSimpleMeshData& AppendBoxMesh(FVector BoxRadius, FTransform BoxTransform, UPARAM(Ref) FRealtimeMeshSimpleMeshData& MeshData, int32 NewMaterialGroup = 0);

	/** Generate vertex and index buffer for a simple box, given the supplied dimensions. Normals, UVs and tangents are also generated for each vertex. */
	UFUNCTION(BlueprintCallable, Category = "RealtimeMesh|MeshGeneration", meta=(AutoCreateRefTerm="Transform"))
	static FRealtimeMeshSimpleMeshData& AppendMesh(UPARAM(Ref) FRealtimeMeshSimpleMeshData& TargetMeshData, const FRealtimeMeshSimpleMeshData& MeshDataToAdd,
												   const FTransform& Transform, int32 NewMaterialGroup = 0);
};
