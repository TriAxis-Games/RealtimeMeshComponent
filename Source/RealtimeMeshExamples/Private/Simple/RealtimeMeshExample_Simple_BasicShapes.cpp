// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#include "Simple/RealtimeMeshExample_Simple_BasicShapes.h"
#include "RealtimeMeshSimple.h"
#include "Mesh/RealtimeMeshBasicShapeTools.h"

using namespace RealtimeMesh;

ARealtimeMeshExample_Simple_BasicShapes::ARealtimeMeshExample_Simple_BasicShapes()
{
	Description = NSLOCTEXT("RealtimeMeshExamples", "Simple_BasicShapes",
		"URealtimeMeshSimple: generate primitive boxes with URealtimeMeshBasicShapeTools instead of hand-authoring vertices.");
}

void ARealtimeMeshExample_Simple_BasicShapes::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	URealtimeMeshSimple* RealtimeMesh = GetRealtimeMeshComponent()->InitializeRealtimeMesh<URealtimeMeshSimple>();
	RealtimeMesh->SetupMaterialSlot(0, "DefaultMaterial");

	const FRealtimeMeshLODKey LODKey(0);

	struct FBoxDef { FVector3f Radius; FVector3f Location; FColor Color; const TCHAR* Name; };
	const FBoxDef Boxes[] = {
		{ FVector3f(50.0f, 50.0f, 50.0f), FVector3f(-150.0f, 0.0f, 50.0f), FColor::Red,   TEXT("Box1") }, // cube
		{ FVector3f(40.0f, 40.0f, 80.0f), FVector3f(0.0f, 0.0f, 80.0f),    FColor::Green, TEXT("Box2") }, // tall
		{ FVector3f(70.0f, 30.0f, 40.0f), FVector3f(150.0f, 0.0f, 40.0f),  FColor::Blue,  TEXT("Box3") }, // wide
	};

	for (const FBoxDef& Box : Boxes)
	{
		FRealtimeMeshStreamSet BoxStreamSet;
		URealtimeMeshBasicShapeTools::AppendBoxMesh(BoxStreamSet, Box.Radius, FTransform3f(Box.Location), 0, Box.Color);

		const FRealtimeMeshBufferSetKey BoxGroupKey = FRealtimeMeshBufferSetKey::Create(LODKey, FName(Box.Name));
		const FRealtimeMeshSectionKey BoxSectionKey = FRealtimeMeshSectionKey::CreateForPolyGroup(BoxGroupKey, 0);

		RealtimeMesh->CreateBufferSet(BoxGroupKey, BoxStreamSet);
		RealtimeMesh->UpdateSectionConfig(BoxSectionKey, FRealtimeMeshSectionConfig(0));
	}

	VerifyMeshBuilt();
}
