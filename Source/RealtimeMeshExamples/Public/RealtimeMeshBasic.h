// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RealtimeMeshActor.h"
#include "GameFramework/Actor.h"
#include "RealtimeMeshBasic.generated.h"

UCLASS()
class REALTIMEMESHEXAMPLES_API ARealtimeMeshBasic : public ARealtimeMeshActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ARealtimeMeshBasic();

protected:

	// Called when the mesh generation should happen. This could be called in the
	// editor for placed actors, or at runtime for spawned actors.
	virtual void OnGenerateMesh_Implementation() override;
};
