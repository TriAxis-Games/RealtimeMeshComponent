// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RealtimeMeshExampleActor.h"
#include "RealtimeMeshExample_Simple_MultipleSections.generated.h"

/**
 * Demonstrates multiple sections and material slots. One section group packs several
 * flat tiles into a single StreamSet via poly groups, with every section pointed at
 * a different material slot. This is the idiomatic way to render one mesh with many
 * materials.
 */
UCLASS()
class REALTIMEMESHEXAMPLES_API ARealtimeMeshExample_Simple_MultipleSections : public ARealtimeMeshExampleActor
{
	GENERATED_BODY()

public:
	ARealtimeMeshExample_Simple_MultipleSections();

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
};
