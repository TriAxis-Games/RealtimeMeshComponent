// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RealtimeMeshExampleActor.h"
#include "RealtimeMeshExample_Procedural_Basic.generated.h"

/**
 * URealtimeMeshProcedural is the ProceduralMeshComponent-parity leaf: PMC users
 * migrate by find-and-replace. This example creates a box section the PMC way —
 * parallel TArrays of positions / triangles / normals / UVs / colors via
 * CreateMeshSection, with the geometry coming from the procedural-mesh helper
 * library.
 */
UCLASS()
class REALTIMEMESHEXAMPLES_API ARealtimeMeshExample_Procedural_Basic : public ARealtimeMeshExampleActor
{
	GENERATED_BODY()

public:
	ARealtimeMeshExample_Procedural_Basic();

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
};
