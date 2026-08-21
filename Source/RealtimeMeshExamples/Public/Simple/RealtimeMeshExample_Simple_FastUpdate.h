// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RealtimeMeshExampleActor.h"
#include "RealtimeMeshExample_Simple_FastUpdate.generated.h"

UENUM(BlueprintType)
enum class ERealtimeMeshFastUpdateMode : uint8
{
	/** Ripple every vertex each frame via EditMeshInPlace (whole-stream fast path). */
	FullGridFastInPlace,
	/**
	 * Ripple only the middle band of rows via EditMeshInPlaceRanged; the top and
	 * bottom of the grid stay flat. Because only the requested element range is
	 * uploaded, this is a direct visual proof that the partial update touches only
	 * those vertices.
	 */
	MiddleBandRanged,
};

/**
 * Demonstrates and lets you verify the in-place fast-update paths on
 * URealtimeMeshSimple.
 *
 * Unlike Simple_DynamicUpdate (which re-pushes a fresh StreamSet via
 * UpdateSectionGroup every frame — the full realloc + republish path), this
 * example builds a *Dynamic* draw-type section group ONCE and then animates it
 * each tick with:
 *   - EditMeshInPlace        (FullGridFastInPlace) — overwrites all vertex
 *                             positions in place, no buffer realloc / no new
 *                             proxy version.
 *   - EditMeshInPlaceRanged  (MiddleBandRanged) — uploads only the middle band's
 *                             element range; the rest of the grid never moves, so
 *                             you can see the partial update is doing exactly that.
 *
 * Dynamic draw type is what makes the buffers CPU-lockable and the fast path
 * eligible; a Static group would transparently fall back to a full update.
 */
UCLASS()
class REALTIMEMESHEXAMPLES_API ARealtimeMeshExample_Simple_FastUpdate : public ARealtimeMeshExampleActor
{
	GENERATED_BODY()

public:
	ARealtimeMeshExample_Simple_FastUpdate();

	/** Which fast-update path to demonstrate. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fast Update")
	ERealtimeMeshFastUpdateMode Mode = ERealtimeMeshFastUpdateMode::MiddleBandRanged;

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
	// Builds the flat grid once as a Dynamic-draw-type section group.
	void BuildInitialMesh();

	// Animates all vertices via EditMeshInPlace (whole-stream).
	void AnimateFullGrid(float Time);

	// Animates only the middle band of rows via EditMeshInPlaceRanged.
	void AnimateMiddleBand(float Time);

	float AccumulatedTime = 0.0f;

	// Grid is (GridSize + 1) x (GridSize + 1) vertices, laid out row-major so a row
	// span maps to a contiguous vertex (element) range — ideal for ranged updates.
	static constexpr int32 GridSize = 32;
	static constexpr float Spacing = 10.0f;
};
