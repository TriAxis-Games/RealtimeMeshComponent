// Copyright (c) 2015-2025 TriAxis Games, L.L.C. All Rights Reserved.

#include "FunctionalTests/RealtimeMeshVisualTest_SimpleCollision.h"
#include "RealtimeMeshSimple.h"
#include "Mesh/RealtimeMeshBasicShapeTools.h"
#include "Core/RealtimeMeshCollision.h"

using namespace RealtimeMesh;

ARealtimeMeshVisualTest_SimpleCollision::ARealtimeMeshVisualTest_SimpleCollision()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ARealtimeMeshVisualTest_SimpleCollision::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	URealtimeMeshSimple* RealtimeMesh = GetRealtimeMeshComponent()->InitializeRealtimeMesh<URealtimeMeshSimple>();

	// Setup material slot
	RealtimeMesh->SetupMaterialSlot(0, "FloorMaterial");

	const FRealtimeMeshLODKey LODKey(0);

	// Create a large flat box mesh for the floor (500x500x10 units)
	// Positioned at Z=0 so the top surface is at ground level (box extends from -5 to +5 in Z)
	{
		FRealtimeMeshStreamSet FloorStreamSet;

		// Box dimensions: 500 wide (X), 500 deep (Y), 10 tall (Z)
		const FVector3f BoxRadius(250.0f, 250.0f, 5.0f); // Half-extents

		// Position at ground level (Z=0 means center of box, so top at +5, bottom at -5)
		const FTransform3f FloorTransform(FVector3f(0.0f, 0.0f, 0.0f));

		URealtimeMeshBasicShapeTools::AppendBoxMesh(
			FloorStreamSet,
			BoxRadius,
			FloorTransform,
			0, // Material index
			FColor::Green // Visual color
		);

		const FRealtimeMeshSectionGroupKey FloorGroupKey = FRealtimeMeshSectionGroupKey::Create(LODKey, FName("Floor"));

		RealtimeMesh->CreateSectionGroup(FloorGroupKey, FloorStreamSet);
	}

	// Setup simple collision geometry
	// Create a box collision shape matching the mesh dimensions
	{
		FRealtimeMeshSimpleGeometry SimpleGeometry;

		// Create a box collision with the same dimensions as the visual mesh
		FRealtimeMeshCollisionBox CollisionBox;
		CollisionBox.Extents = FVector(250.0f, 250.0f, 5.0f); // Match the box radius from above
		CollisionBox.Center = FVector(0.0f, 0.0f, 0.0f); // Same center position
		CollisionBox.Rotation = FRotator::ZeroRotator;
		CollisionBox.bContributesToMass = true;

		// Add the box to the simple geometry
		SimpleGeometry.Boxes.Add(CollisionBox);

		// Apply the simple geometry to the mesh
		RealtimeMesh->SetSimpleGeometry(SimpleGeometry);
	}

	// Configure collision settings
	// Disable complex collision and use only simple collision
	{
		FRealtimeMeshCollisionConfiguration CollisionConfig;
		CollisionConfig.bUseComplexAsSimpleCollision = false; // Use simple collision shapes
		CollisionConfig.bUseAsyncCook = true;
		CollisionConfig.bShouldFastCookMeshes = false;
		CollisionConfig.bFlipNormals = false;
		CollisionConfig.bDeformableMesh = false;
		CollisionConfig.bMergeAllMeshes = false;

		RealtimeMesh->SetCollisionConfig(CollisionConfig);
	}
}
