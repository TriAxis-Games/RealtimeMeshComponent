// Copyright (c) 2015-2025 TriAxis Games, L.L.C. All Rights Reserved.

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

	virtual void OnConstruction(const FTransform& Transform) override;
};
