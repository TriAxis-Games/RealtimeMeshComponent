// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RealtimeMeshActor.h"
#include "RealtimeMeshLatentUpdateTestActor.generated.h"

class URealtimeMeshSimple;


/*
 * This is a test where the mesh structure is created on mesh generation, and the actual data is generated and applied later in BeginPlay
 */
UCLASS()
class REALTIMEMESHTESTS_API ARealtimeMeshLatentUpdateTestActor : public ARealtimeMeshActor
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
	ARealtimeMeshLatentUpdateTestActor();

	virtual void OnGenerateMesh_Implementation() override;

	virtual void BeginPlay() override;
};