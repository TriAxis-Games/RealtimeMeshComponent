// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RealtimeMeshExampleActor.h"
#include "RealtimeMeshExample_Simple_StreamBuilders.generated.h"

/**
 * Builds the same triangle as HelloTriangle, but with the lower-level
 * per-stream TRealtimeMeshStreamBuilder instead of the all-in-one
 * TRealtimeMeshBuilderLocal. Shows that each buffer (position, tangents,
 * texcoords, color, triangles, poly groups) is an independent stream you can
 * wrap, reserve, and fill on its own — and that the "interface type" can differ
 * from the stored type (e.g. work in full-precision tangents while storing
 * packed ones).
 */
UCLASS()
class REALTIMEMESHEXAMPLES_API ARealtimeMeshExample_Simple_StreamBuilders : public ARealtimeMeshExampleActor
{
	GENERATED_BODY()

public:
	ARealtimeMeshExample_Simple_StreamBuilders();

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
};
