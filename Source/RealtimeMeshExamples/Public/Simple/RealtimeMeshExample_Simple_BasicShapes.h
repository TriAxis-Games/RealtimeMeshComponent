// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RealtimeMeshExampleActor.h"
#include "RealtimeMeshExample_Simple_BasicShapes.generated.h"

/**
 * Builds three boxes of different sizes/colors using the URealtimeMeshBasicShapeTools
 * helpers, each as its own section group. Shows how to generate primitive geometry
 * without hand-authoring vertices.
 */
UCLASS()
class REALTIMEMESHEXAMPLES_API ARealtimeMeshExample_Simple_BasicShapes : public ARealtimeMeshExampleActor
{
	GENERATED_BODY()

public:
	ARealtimeMeshExample_Simple_BasicShapes();

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
};
