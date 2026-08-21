// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RealtimeMeshExampleActor.h"
#include "RealtimeMeshExample_Simple_DynamicUpdate.generated.h"

/**
 * An animated mesh: a grid whose vertices ripple with a sine wave each frame.
 * Builds the section group on first update and re-pushes a fresh StreamSet via
 * UpdateSectionGroup every tick. The simplest "mesh that changes over time"
 * pattern; see the Procedural/Dynamic examples for cheaper partial-update paths.
 */
UCLASS()
class REALTIMEMESHEXAMPLES_API ARealtimeMeshExample_Simple_DynamicUpdate : public ARealtimeMeshExampleActor
{
	GENERATED_BODY()

public:
	ARealtimeMeshExample_Simple_DynamicUpdate();

	/** Animation speed multiplier. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	float AnimationSpeed = 1.0f;

	/** Whether the wave animates. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	bool bEnableAnimation = true;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

private:
	void UpdateMesh(float Time);

	float AccumulatedTime = 0.0f;
};
