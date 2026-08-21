// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RealtimeMeshActor.h"
#include "RealtimeMeshExampleActor.generated.h"

/**
 * Common base for every RealtimeMesh example actor.
 *
 * Each concrete example builds a real mesh in OnConstruction (so dropping it into
 * a level renders immediately) and demonstrates one feature or workflow. The set
 * is meant to double as a functional sweep: spawn them in a test level and every
 * supported path gets exercised. Examples are grouped in the editor under the
 * "RealtimeMesh|Examples" class group and carry a short Description so a gallery
 * level can label them.
 *
 * This base is intentionally thin — it only adds the Description and a tiny
 * self-check helper. Reuse GetRealtimeMeshComponent() from ARealtimeMeshActor.
 */
UCLASS(Abstract, ClassGroup = (RealtimeMesh), meta = (ChildCanTick))
class REALTIMEMESHEXAMPLES_API ARealtimeMeshExampleActor : public ARealtimeMeshActor
{
	GENERATED_BODY()

public:
	ARealtimeMeshExampleActor();

	/** One-line summary of what this example demonstrates (shown in a gallery / details panel). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Example")
	FText Description;

protected:
	/**
	 * Light self-check used by the examples after they build their mesh. Logs a
	 * warning (and trips an ensure in development) if the mesh is missing or has
	 * degenerate bounds, so a functional sweep surfaces a broken example without
	 * the heavier machinery of an automation test.
	 */
	void VerifyMeshBuilt() const;
};
