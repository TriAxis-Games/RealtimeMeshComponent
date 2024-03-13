// Copyright TriAxis Games, L.L.C. All Rights Reserved.

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
	
	// Create a section group passing it our mesh data
	UPROPERTY()
	FRealtimeMeshSectionGroupKey GroupKey;
	
	// Sets default values for this actor's properties
	ARealtimeMeshUpdateTestActor();

	virtual void OnGenerateMesh_Implementation() override;

};