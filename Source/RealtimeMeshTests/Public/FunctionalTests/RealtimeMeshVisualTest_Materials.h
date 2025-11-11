// Copyright (c) 2015-2025 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RealtimeMeshActor.h"
#include "RealtimeMeshVisualTest_Materials.generated.h"

/**
 * Visual test actor that creates multiple boxes with different material slots
 * to validate material assignment and rendering.
 */
UCLASS()
class REALTIMEMESHTESTS_API ARealtimeMeshVisualTest_Materials : public ARealtimeMeshActor
{
	GENERATED_BODY()

public:
	ARealtimeMeshVisualTest_Materials();

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
};
