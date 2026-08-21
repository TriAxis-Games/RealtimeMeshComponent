// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RealtimeMeshExampleActor.h"
#include "RealtimeMeshExample_Procedural_Update.generated.h"

/**
 * Creates a grid section once, then animates only its vertex positions every
 * frame with UpdateMeshSection — the PMC-parity partial-update path. Topology
 * (the triangle list) is set up once at create time and never re-sent.
 */
UCLASS()
class REALTIMEMESHEXAMPLES_API ARealtimeMeshExample_Procedural_Update : public ARealtimeMeshExampleActor
{
	GENERATED_BODY()

public:
	ARealtimeMeshExample_Procedural_Update();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	float AnimationSpeed = 1.0f;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

private:
	/** Fill Vertices with the grid positions for the given time (topology is fixed). */
	void ComputeGridVertices(float Time, TArray<FVector>& OutVertices) const;

	static constexpr int32 GridSize = 20;

	float AccumulatedTime = 0.0f;
};
