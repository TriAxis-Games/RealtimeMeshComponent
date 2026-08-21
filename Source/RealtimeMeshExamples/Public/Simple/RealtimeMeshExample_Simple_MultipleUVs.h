// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RealtimeMeshExampleActor.h"
#include "RealtimeMeshExample_Simple_MultipleUVs.generated.h"

/**
 * Same triangle as HelloTriangle but with two UV channels per vertex. The builder
 * template's channel count (the trailing "2") enables the second texcoord stream;
 * SetTexCoord(channel, value) writes each one.
 */
UCLASS()
class REALTIMEMESHEXAMPLES_API ARealtimeMeshExample_Simple_MultipleUVs : public ARealtimeMeshExampleActor
{
	GENERATED_BODY()

public:
	ARealtimeMeshExample_Simple_MultipleUVs();

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
};
