// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RealtimeMeshExampleActor.h"
#include "RealtimeMeshExample_Simple_Collision.generated.h"

UENUM(BlueprintType)
enum class ERealtimeMeshExampleCollisionMode : uint8
{
	/** A flat floor with an explicit simple box collision shape (cheap, analytic). */
	Simple,
	/** A stepped platform using per-triangle (complex-as-simple) collision that follows the surface. */
	Complex
};

/**
 * Demonstrates the two collision setups on URealtimeMeshSimple. Switch CollisionMode
 * in the details panel:
 *  - Simple:  build a floor and give it a FRealtimeMeshSimpleGeometry box shape.
 *  - Complex: build a stepped platform and enable complex-as-simple collision so the
 *             collision follows the rendered triangles exactly.
 */
UCLASS()
class REALTIMEMESHEXAMPLES_API ARealtimeMeshExample_Simple_Collision : public ARealtimeMeshExampleActor
{
	GENERATED_BODY()

public:
	ARealtimeMeshExample_Simple_Collision();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collision")
	ERealtimeMeshExampleCollisionMode CollisionMode = ERealtimeMeshExampleCollisionMode::Simple;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;

private:
	void BuildSimpleCollision();
	void BuildComplexCollision();
};
