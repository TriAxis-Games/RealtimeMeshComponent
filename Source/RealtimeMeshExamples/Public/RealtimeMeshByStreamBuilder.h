// Copyright (c) 2015-2025 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RealtimeMeshActor.h"
#include "GameFramework/Actor.h"
#include "RealtimeMeshByStreamBuilder.generated.h"

UCLASS()
class REALTIMEMESHEXAMPLES_API ARealtimeMeshByStreamBuilder : public ARealtimeMeshActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ARealtimeMeshByStreamBuilder();

protected:

	virtual void OnConstruction(const FTransform& Transform) override;
};