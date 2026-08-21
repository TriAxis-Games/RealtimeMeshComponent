// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RealtimeMeshExampleActor.h"
#include "RealtimeMeshExample_Simple_VertexColors.generated.h"

class URealtimeMeshSimple;

/**
 * Four demonstrations of authoring per-vertex colors: a rainbow grid, an RGB
 * corner-colored box, a radial gradient disc, and a height-gradient cylinder.
 * Use a vertex-color-aware material to see the results.
 */
UCLASS()
class REALTIMEMESHEXAMPLES_API ARealtimeMeshExample_Simple_VertexColors : public ARealtimeMeshExampleActor
{
	GENERATED_BODY()

public:
	ARealtimeMeshExample_Simple_VertexColors();

protected:
	virtual void OnConstruction(const FTransform& Transform) override;

private:
	void CreateRainbowGrid(URealtimeMeshSimple* RealtimeMesh, const FVector3f& Offset);
	void CreateRGBCornerBox(URealtimeMeshSimple* RealtimeMesh, const FVector3f& Offset);
	void CreateRadialGradientPlane(URealtimeMeshSimple* RealtimeMesh, const FVector3f& Offset);
	void CreateHeightGradientMesh(URealtimeMeshSimple* RealtimeMesh, const FVector3f& Offset);
};
