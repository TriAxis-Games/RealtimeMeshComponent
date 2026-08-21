// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RealtimeMeshExampleActor.h"
#include "RealtimeMeshExample_Simple_ComputeIndirect.generated.h"

/**
 * Phase 1 verification for the compute-mesh hooks: GPU-driven indirect draw.
 *
 * Builds a box (12 triangles) in a compute-writable section group, then draws only the first
 * VisibleTriangles of it. Two mechanisms produce the same picture, which is the A/B check:
 *   - bUseIndirect = true : the full geometry stays bound, but an FRHIDrawIndexedIndirectParameters
 *     buffer (here CPU-initialized for the test; compute-written in later phases) supplies the index
 *     count, so the engine renders VisibleTriangles via NumPrimitives==0 / IndirectArgsBuffer.
 *   - bUseIndirect = false: the section's CPU stream range is shrunk to VisibleTriangles (normal draw).
 *
 * Flipping bUseIndirect with the same VisibleTriangles must render an identical partial box. None of
 * this is testable under NullRHI (no real RHI for the indirect draw), so it is verified in PIE.
 */
UCLASS()
class REALTIMEMESHEXAMPLES_API ARealtimeMeshExample_Simple_ComputeIndirect : public ARealtimeMeshExampleActor
{
	GENERATED_BODY()

public:
	ARealtimeMeshExample_Simple_ComputeIndirect();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	/** Half-extent of the box. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Example")
	FVector3f BoxRadius = FVector3f(100.0f);

	/** How many of the box's 12 triangles to draw. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Example", meta = (ClampMin = "0", ClampMax = "12"))
	int32 VisibleTriangles = 6;

	/** true: limit the draw with a GPU indirect-args buffer. false: limit it with the CPU stream range. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Example")
	bool bUseIndirect = true;

private:
	void ApplyDrawLimit();
};
