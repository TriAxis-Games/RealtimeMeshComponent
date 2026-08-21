// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RealtimeMeshExampleActor.h"
#include "RealtimeMeshExample_Simple_HighPoly.generated.h"

/**
 * Builds a high-poly sphere to exercise a large vertex/triangle buffer in a
 * single section. Uses a 32-bit index builder since the count can exceed 65k.
 * Crank SubdivisionLevel to stress-test.
 */
UCLASS()
class REALTIMEMESHEXAMPLES_API ARealtimeMeshExample_Simple_HighPoly : public ARealtimeMeshExampleActor
{
	GENERATED_BODY()

public:
	ARealtimeMeshExample_Simple_HighPoly();

	/** Higher = more polygons (segments/rings scale by 2^level). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh", meta = (ClampMin = "0", ClampMax = "6"))
	int32 SubdivisionLevel = 3;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
};
