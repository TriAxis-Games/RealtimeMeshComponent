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
class REALTIMEMESHCOMPONENT_API URealtimeMeshBasicShapeTools : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	UE_DEPRECATED(all, "FRealtimeMeshSimpleMeshData is deprecated, use URealtimeMeshStreamSet instead and its helper functions which are more efficient and flexible")
	/** Generate vertex and index buffer for a simple box, given the supplied dimensions. Normals, UVs and tangents are also generated for each vertex. */
	UFUNCTION(BlueprintCallable, Category = "RealtimeMesh|MeshGeneration")
	static FRealtimeMeshSimpleMeshData& AppendBoxMesh(FVector BoxRadius, FTransform BoxTransform, UPARAM(Ref) FRealtimeMeshSimpleMeshData& MeshData, int32 NewMaterialGroup = 0);

	UE_DEPRECATED(all, "FRealtimeMeshSimpleMeshData is deprecated, use URealtimeMeshStreamSet instead and its helper functions which are more efficient and flexible")
	/** Generate vertex and index buffer for a simple box, given the supplied dimensions. Normals, UVs and tangents are also generated for each vertex. */
	UFUNCTION(BlueprintCallable, Category = "RealtimeMesh|MeshGeneration", meta=(AutoCreateRefTerm="Transform"))
	static FRealtimeMeshSimpleMeshData& AppendMesh(UPARAM(Ref) FRealtimeMeshSimpleMeshData& TargetMeshData, const FRealtimeMeshSimpleMeshData& MeshDataToAdd,
												   const FTransform& Transform, int32 NewMaterialGroup = 0);
	PRAGMA_ENABLE_DEPRECATION_WARNINGS


	static void AppendBoxMesh(FRealtimeMeshStreamSet& StreamSet, FVector3f BoxRadius, FTransform3f BoxTransform = FTransform3f::Identity, int32 NewMaterialGroup = 0, FColor Color = FColor::White);

	static void AppendMesh(FRealtimeMeshStreamSet& TargetMeshData, const FRealtimeMeshStreamSet& MeshDataToAdd, const FTransform3f& Transform = FTransform3f::Identity);
	
};
