// Copyright (c) 2015-2025 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RealtimeMeshActor.h"
#include "RealtimeMeshVisualTest_MultipleSections.generated.h"

/**
 * Visual test actor that demonstrates multiple sections within section groups using polygroups.
 *
 * This test creates two distinct section groups:
 * - Group 1: Contains 3 sections using polygroups 0, 1, 2 (three boxes in one streamset)
 * - Group 2: Contains 2 sections using polygroups 0, 1 (two boxes in one streamset)
 *
 * Each section uses a different material slot to visually distinguish them.
 */
UCLASS()
class REALTIMEMESHTESTS_API ARealtimeMeshVisualTest_MultipleSections : public ARealtimeMeshActor
{
	GENERATED_BODY()

public:
	ARealtimeMeshVisualTest_MultipleSections();

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
};
