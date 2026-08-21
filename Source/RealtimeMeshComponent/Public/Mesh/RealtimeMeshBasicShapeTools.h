// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RealtimeMeshBasicShapeTools.generated.h"

namespace RealtimeMesh
{
	struct FRealtimeMeshStreamSet;
}

struct FRealtimeMeshSimpleMeshData;

/**
 * 
 */
class URealtimeMeshStreamSet;

UCLASS(meta=(ScriptName="RealtimeMeshBasicShapeTools"))
class REALTIMEMESHCOMPONENT_API URealtimeMeshBasicShapeTools : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:


	static void AppendBoxMesh(RealtimeMesh::FRealtimeMeshStreamSet& StreamSet, FVector3f BoxRadius, FTransform3f BoxTransform = FTransform3f::Identity, int32 NewMaterialGroup = 0, FColor Color = FColor::White);

	/**
	 * Appends a box (given as half-extents) to a Blueprint stream set, optionally tagged
	 * into a poly group. Returns the same stream set for call chaining.
	 */
	UFUNCTION(BlueprintCallable, Category = "RealtimeMesh|MeshData", DisplayName = "Append Box Mesh")
	static URealtimeMeshStreamSet* AppendBoxMesh(URealtimeMeshStreamSet* StreamSet, FVector BoxRadius, FTransform BoxTransform, int32 NewMaterialGroup = 0, FLinearColor Color = FLinearColor::White);

	static void AppendMesh(RealtimeMesh::FRealtimeMeshStreamSet& TargetMeshData, const RealtimeMesh::FRealtimeMeshStreamSet& MeshDataToAdd, const FTransform3f& Transform = FTransform3f::Identity, bool bSkipMissingStreams = false);
	
};
