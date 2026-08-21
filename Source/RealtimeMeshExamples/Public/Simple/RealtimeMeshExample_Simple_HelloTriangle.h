// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RealtimeMeshExampleActor.h"
#include "RealtimeMeshExample_Simple_HelloTriangle.generated.h"

/**
 * The canonical first example: a single triangle on a URealtimeMeshSimple.
 *
 * Demonstrates the whole minimal pipeline — build a StreamSet with the local
 * builder, set up material slots, and create one section group. The triangle is
 * emitted twice (two poly groups / two sections) so it's visible from both sides.
 */
UCLASS()
class REALTIMEMESHEXAMPLES_API ARealtimeMeshExample_Simple_HelloTriangle : public ARealtimeMeshExampleActor
{
	GENERATED_BODY()

public:
	ARealtimeMeshExample_Simple_HelloTriangle();

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
};
