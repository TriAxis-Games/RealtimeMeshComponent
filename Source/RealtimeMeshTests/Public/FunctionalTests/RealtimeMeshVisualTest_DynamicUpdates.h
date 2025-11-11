// Copyright (c) 2015-2025 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RealtimeMeshActor.h"
#include "RealtimeMeshVisualTest_DynamicUpdates.generated.h"

/**
 * Visual test actor that demonstrates dynamic mesh updates.
 * Creates an animated mesh that updates its geometry every frame.
 * Useful for validating that mesh updates are correctly reflected in rendering.
 */
UCLASS()
class REALTIMEMESHTESTS_API ARealtimeMeshVisualTest_DynamicUpdates : public ARealtimeMeshActor
{
	GENERATED_BODY()

public:
	ARealtimeMeshVisualTest_DynamicUpdates();

	/** Animation speed multiplier */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	float AnimationSpeed = 1.0f;

	/** Whether animation is enabled */
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
