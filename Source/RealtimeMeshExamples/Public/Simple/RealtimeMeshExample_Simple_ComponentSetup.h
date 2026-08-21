// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RealtimeMeshComponent.h"
#include "RealtimeMeshExample_Simple_ComponentSetup.generated.h"

/**
 * Shows how to wire up a URealtimeMeshComponent by hand on a plain AActor — the
 * setup that ARealtimeMeshActor normally does for you (mobility, collision
 * profile, root component). Use this when you're adding a RealtimeMesh to your
 * own actor rather than subclassing ARealtimeMeshActor.
 *
 * This one deliberately does NOT derive from ARealtimeMeshExampleActor, since the
 * whole point is to create the component manually.
 */
UCLASS(ClassGroup = (RealtimeMesh))
class REALTIMEMESHEXAMPLES_API ARealtimeMeshExample_Simple_ComponentSetup : public AActor
{
	GENERATED_BODY()

public:
	ARealtimeMeshExample_Simple_ComponentSetup();

protected:
	virtual void OnConstruction(const FTransform& Transform) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RealtimeMesh")
	TObjectPtr<URealtimeMeshComponent> RealtimeMeshComponent;
};
