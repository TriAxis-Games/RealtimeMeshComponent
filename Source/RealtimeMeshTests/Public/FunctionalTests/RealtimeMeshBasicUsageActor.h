// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "RealtimeMeshActor.h"
#include "RealtimeMeshBasicUsageActor.generated.h"

UCLASS()
class REALTIMEMESHTESTS_API ARealtimeMeshBasicUsageActor : public ARealtimeMeshActor
{
	GENERATED_BODY()
private:
	FLinearColor LastColor;
	FLinearColor CurrentColor;
	float TimeRemaining;

public:
	// Sets default values for this actor's properties
	ARealtimeMeshBasicUsageActor();

	virtual void OnGenerateMesh_Implementation() override;
	
	virtual void TickActor(float DeltaTime, ELevelTick TickType, FActorTickFunction& ThisTickFunction) override;
};
