// Copyright (c) 2015-2025 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RealtimeMeshActor.h"
#include "RealtimeMeshVisualTest_ComplexCollision.generated.h"

/**
 * Visual test actor that demonstrates complex (per-triangle) collision on a RealtimeMesh.
 * Creates a stepped platform using manual mesh building with complex collision enabled.
 * Drop a static mesh box on it to verify collision works correctly.
 */
UCLASS()
class REALTIMEMESHTESTS_API ARealtimeMeshVisualTest_ComplexCollision : public ARealtimeMeshActor
{
	GENERATED_BODY()

public:
	ARealtimeMeshVisualTest_ComplexCollision();

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
};
