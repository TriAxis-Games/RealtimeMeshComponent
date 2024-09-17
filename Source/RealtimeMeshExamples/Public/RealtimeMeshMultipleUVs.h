// Copyright (c) 2015-2024 TriAxis Games, L.L.C. All Rights Reserved.

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

	virtual void OnConstruction(const FTransform& Transform) override;
};