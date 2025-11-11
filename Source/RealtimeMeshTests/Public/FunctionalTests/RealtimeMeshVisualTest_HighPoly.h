// Copyright (c) 2015-2025 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RealtimeMeshActor.h"
#include "RealtimeMeshVisualTest_HighPoly.generated.h"

/**
 * Visual test actor that creates a high-poly mesh to validate rendering
 * of complex geometry with many vertices and triangles.
 */
UCLASS()
class REALTIMEMESHTESTS_API ARealtimeMeshVisualTest_HighPoly : public ARealtimeMeshActor
{
	GENERATED_BODY()

public:
	ARealtimeMeshVisualTest_HighPoly();

	/** Subdivision level (higher = more polygons) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh", meta = (ClampMin = "0", ClampMax = "6"))
	int32 SubdivisionLevel = 3;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
};
