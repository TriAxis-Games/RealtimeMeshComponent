// Copyright (c) 2015-2025 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RealtimeMeshActor.h"
#include "RealtimeMeshVisualTest_LOD.generated.h"

/**
 * Visual test actor that creates a mesh with multiple LOD levels.
 * Each LOD has visually distinct geometry (different colors) to validate LOD switching.
 */
UCLASS()
class REALTIMEMESHTESTS_API ARealtimeMeshVisualTest_LOD : public ARealtimeMeshActor
{
	GENERATED_BODY()

public:
	ARealtimeMeshVisualTest_LOD();

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
};
