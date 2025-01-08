// Copyright (c) 2015-2025 TriAxis Games, L.L.C. All Rights Reserved.

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
UCLASS(meta=(ScriptName="RealtimeMeshBasicShapeTools"))
class REALTIMEMESHCOMPONENT_API URealtimeMeshBasicShapeTools : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:


	static void AppendBoxMesh(RealtimeMesh::FRealtimeMeshStreamSet& StreamSet, FVector3f BoxRadius, FTransform3f BoxTransform = FTransform3f::Identity, int32 NewMaterialGroup = 0, FColor Color = FColor::White);

	static void AppendMesh(RealtimeMesh::FRealtimeMeshStreamSet& TargetMeshData, const RealtimeMesh::FRealtimeMeshStreamSet& MeshDataToAdd, const FTransform3f& Transform = FTransform3f::Identity, bool bSkipMissingStreams = false);
	
};
