// Copyright (c) 2015-2025 TriAxis Games, L.L.C. All Rights Reserved.

#include "FunctionalTests/RealtimeMeshVisualTest_BasicShapes.h"
#include "RealtimeMeshSimple.h"
#include "Mesh/RealtimeMeshBasicShapeTools.h"

using namespace RealtimeMesh;

ARealtimeMeshVisualTest_BasicShapes::ARealtimeMeshVisualTest_BasicShapes()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ARealtimeMeshVisualTest_BasicShapes::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	URealtimeMeshSimple* RealtimeMesh = GetRealtimeMeshComponent()->InitializeRealtimeMesh<URealtimeMeshSimple>();

	// Setup material slot
	RealtimeMesh->SetupMaterialSlot(0, "DefaultMaterial");

	const FRealtimeMeshLODKey LODKey(0);

	// Create three boxes with different sizes and colors to demonstrate basic shapes

	// Box 1 (left) - Cube
	{
		FRealtimeMeshStreamSet BoxStreamSet;
		URealtimeMeshBasicShapeTools::AppendBoxMesh(
			BoxStreamSet,
			FVector3f(50.0f, 50.0f, 50.0f),
			FTransform3f(FVector3f(-150.0f, 0.0f, 50.0f)),
			0,
			FColor::Red
		);

		const FRealtimeMeshSectionGroupKey BoxGroupKey = FRealtimeMeshSectionGroupKey::Create(LODKey, FName("Box1"));
		const FRealtimeMeshSectionKey BoxSectionKey = FRealtimeMeshSectionKey::CreateForPolyGroup(BoxGroupKey, 0);

		RealtimeMesh->CreateSectionGroup(BoxGroupKey, BoxStreamSet);
		RealtimeMesh->UpdateSectionConfig(BoxSectionKey, FRealtimeMeshSectionConfig(0));
	}

	// Box 2 (center) - Tall box
	{
		FRealtimeMeshStreamSet BoxStreamSet;
		URealtimeMeshBasicShapeTools::AppendBoxMesh(
			BoxStreamSet,
			FVector3f(40.0f, 40.0f, 80.0f),
			FTransform3f(FVector3f(0.0f, 0.0f, 80.0f)),
			0,
			FColor::Green
		);

		const FRealtimeMeshSectionGroupKey BoxGroupKey = FRealtimeMeshSectionGroupKey::Create(LODKey, FName("Box2"));
		const FRealtimeMeshSectionKey BoxSectionKey = FRealtimeMeshSectionKey::CreateForPolyGroup(BoxGroupKey, 0);

		RealtimeMesh->CreateSectionGroup(BoxGroupKey, BoxStreamSet);
		RealtimeMesh->UpdateSectionConfig(BoxSectionKey, FRealtimeMeshSectionConfig(0));
	}

	// Box 3 (right) - Wide box
	{
		FRealtimeMeshStreamSet BoxStreamSet;
		URealtimeMeshBasicShapeTools::AppendBoxMesh(
			BoxStreamSet,
			FVector3f(70.0f, 30.0f, 40.0f),
			FTransform3f(FVector3f(150.0f, 0.0f, 40.0f)),
			0,
			FColor::Blue
		);

		const FRealtimeMeshSectionGroupKey BoxGroupKey = FRealtimeMeshSectionGroupKey::Create(LODKey, FName("Box3"));
		const FRealtimeMeshSectionKey BoxSectionKey = FRealtimeMeshSectionKey::CreateForPolyGroup(BoxGroupKey, 0);

		RealtimeMesh->CreateSectionGroup(BoxGroupKey, BoxStreamSet);
		RealtimeMesh->UpdateSectionConfig(BoxSectionKey, FRealtimeMeshSectionConfig(0));
	}
}
