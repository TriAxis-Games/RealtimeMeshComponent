// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RealtimeMeshExampleActor.h"
#include "RealtimeMeshExample_Simple_LODs.generated.h"

/**
 * Demonstrates multiple LODs. Each LOD holds a box at a different rotation and
 * color so the LOD transitions are obvious as the camera moves. Shows AddLOD,
 * per-LOD screen-size config, and that section groups are keyed per LOD.
 */
UCLASS()
class REALTIMEMESHEXAMPLES_API ARealtimeMeshExample_Simple_LODs : public ARealtimeMeshExampleActor
{
	GENERATED_BODY()

public:
	ARealtimeMeshExample_Simple_LODs();

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
};
