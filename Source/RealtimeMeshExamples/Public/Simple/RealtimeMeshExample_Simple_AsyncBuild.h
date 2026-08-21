// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RealtimeMeshExampleActor.h"
#include "RealtimeMeshExample_Simple_AsyncBuild.generated.h"

/**
 * Builds the (potentially expensive) StreamSet on a worker thread, then commits
 * it back on the game thread — the recommended pattern for heavy procedural
 * geometry so you don't stall the game thread. Uses RealtimeMesh's async helpers
 * DoOnAsyncThread + ContinueOnGameThread.
 *
 * The build runs in BeginPlay (not OnConstruction) so it isn't kicked off
 * repeatedly while editing in the editor; spawn it in PIE to see it populate.
 */
UCLASS()
class REALTIMEMESHEXAMPLES_API ARealtimeMeshExample_Simple_AsyncBuild : public ARealtimeMeshExampleActor
{
	GENERATED_BODY()

public:
	ARealtimeMeshExample_Simple_AsyncBuild();

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
};
