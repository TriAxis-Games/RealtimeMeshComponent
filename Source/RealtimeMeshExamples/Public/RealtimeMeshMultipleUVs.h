// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RealtimeMeshActor.h"
#include "GameFramework/Actor.h"
#include "RealtimeMeshMultipleUVs.generated.h"

UCLASS()
class REALTIMEMESHEXAMPLES_API ARealtimeMeshMultipleUVs : public ARealtimeMeshActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ARealtimeMeshMultipleUVs();

protected:

	// Called when the mesh generation should happen. This could be called in the
	// editor for placed actors, or at runtime for spawned actors.
	virtual void OnGenerateMesh_Implementation() override;
};