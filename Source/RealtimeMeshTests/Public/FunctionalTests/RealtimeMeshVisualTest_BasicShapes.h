// Copyright (c) 2015-2025 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RealtimeMeshActor.h"
#include "RealtimeMeshVisualTest_BasicShapes.generated.h"

/**
 * Visual test actor that creates basic geometric shapes (box, sphere, cylinder)
 * arranged in a row for visual validation.
 */
UCLASS()
class REALTIMEMESHTESTS_API ARealtimeMeshVisualTest_BasicShapes : public ARealtimeMeshActor
{
	GENERATED_BODY()

public:
	ARealtimeMeshVisualTest_BasicShapes();

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
};
