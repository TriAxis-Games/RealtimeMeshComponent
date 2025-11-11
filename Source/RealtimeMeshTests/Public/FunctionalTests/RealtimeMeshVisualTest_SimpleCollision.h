// Copyright (c) 2015-2025 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RealtimeMeshActor.h"
#include "RealtimeMeshVisualTest_SimpleCollision.generated.h"

/**
 * Visual test actor that demonstrates simple collision on a RealtimeMesh floor.
 * Creates a large flat box mesh positioned at ground level with simple box collision.
 * When placed in a level, objects should collide with it and land on top.
 * If collision fails, objects will fall through.
 */
UCLASS()
class REALTIMEMESHTESTS_API ARealtimeMeshVisualTest_SimpleCollision : public ARealtimeMeshActor
{
	GENERATED_BODY()

public:
	ARealtimeMeshVisualTest_SimpleCollision();

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
};
