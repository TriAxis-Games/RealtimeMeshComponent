// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "RealtimeMeshActor.h"
#include "RealtimeMeshBasicUsageActor.generated.h"

UCLASS()
class REALTIMEMESHTESTS_API ARealtimeMeshBasicUsageActor : public ARealtimeMeshActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ARealtimeMeshBasicUsageActor();

	virtual void OnGenerateMesh_Implementation() override;
};
