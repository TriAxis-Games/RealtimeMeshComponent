// Copyright (c) 2015-2025 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RealtimeMeshActor.h"
#include "UObject/StrongObjectPtr.h"
#include "RealtimeMeshSimple.h"
#include "RealtimeMeshStressTestActor.generated.h"



UCLASS()
class REALTIMEMESHTESTS_API ARealtimeMeshStressTestActor2 : public ARealtimeMeshActor
{
	GENERATED_BODY()
private:
	TFuture<void> PendingGeneration;

public:
	// Sets default values for this actor's properties
	ARealtimeMeshStressTestActor2();

	virtual void OnConstruction(const FTransform& Transform) override;

	virtual void TickActor(float DeltaTime, ELevelTick TickType, FActorTickFunction& ThisTickFunction) override;
};

