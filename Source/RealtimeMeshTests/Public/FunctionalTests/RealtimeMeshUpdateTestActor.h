// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "RealtimeMeshActor.h"
#include "RealtimeMeshUpdateTestActor.generated.h"

class URealtimeMeshSimple;
UCLASS()
class REALTIMEMESHTESTS_API ARealtimeMeshUpdateTestActor : public ARealtimeMeshActor
{
	GENERATED_BODY()

public:

	UPROPERTY()
	TObjectPtr<URealtimeMeshSimple> RealtimeMesh;

	UPROPERTY()
	FRealtimeMeshSectionKey StaticSectionKey;
	
	// Create a section group passing it our mesh data
	UPROPERTY()
	FRealtimeMeshSectionGroupKey GroupKey;

	// Create both sections on the same mesh data
	UPROPERTY()
	FRealtimeMeshSectionKey SectionInGroupA;
	UPROPERTY()
	FRealtimeMeshSectionKey SectionInGroupB;

	
	// Sets default values for this actor's properties
	ARealtimeMeshUpdateTestActor();

	virtual void OnGenerateMesh_Implementation() override;

};