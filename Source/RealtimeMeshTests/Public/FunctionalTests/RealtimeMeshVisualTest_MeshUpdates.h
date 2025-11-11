// Copyright (c) 2015-2025 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RealtimeMeshActor.h"
#include "RealtimeMeshVisualTest_MeshUpdates.generated.h"

/**
 * Visual test actor that demonstrates various mesh update operations.
 * Cycles through different update scenarios to validate mesh modification capabilities:
 * 1. Full mesh replacement - Cycles between different geometric shapes
 * 2. Partial vertex updates - Updates only vertex positions and colors
 * 3. Section visibility toggling - Shows/hides sections dynamically
 * 4. Material slot changes - Swaps material assignments on sections
 */
UCLASS()
class REALTIMEMESHTESTS_API ARealtimeMeshVisualTest_MeshUpdates : public ARealtimeMeshActor
{
	GENERATED_BODY()

public:
	ARealtimeMeshVisualTest_MeshUpdates();

	/** Time in seconds for each update scenario */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	float ScenarioDuration = 3.0f;

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
	enum class EUpdateScenario : uint8
	{
		FullMeshReplacement,    // Cycle between different shapes
		PartialVertexUpdate,    // Update only vertex data
		SectionVisibility,      // Toggle section visibility
		MaterialSlotSwap        // Swap material slots
	};

	void UpdateMesh(float Time);
	void CreateFullMeshReplacement(float Time);
	void CreatePartialVertexUpdate(float Time);
	void CreateSectionVisibilityTest(float Time);
	void CreateMaterialSlotSwapTest(float Time);

	// Helper to create different shapes
	void CreateBox(float Time);
	void CreateSphere(float Time);
	void CreateCylinder(float Time);

	float AccumulatedTime = 0.0f;
	EUpdateScenario CurrentScenario = EUpdateScenario::FullMeshReplacement;

	FRealtimeMeshSectionGroupKey MainGroupKey;
	FRealtimeMeshSectionKey MainSectionKey;

	// For multi-section tests
	FRealtimeMeshSectionGroupKey SecondaryGroupKey;
	FRealtimeMeshSectionKey SecondarySectionKey;
};
