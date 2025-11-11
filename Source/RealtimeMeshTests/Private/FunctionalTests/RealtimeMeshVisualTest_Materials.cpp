// Copyright (c) 2015-2025 TriAxis Games, L.L.C. All Rights Reserved.

#include "FunctionalTests/RealtimeMeshVisualTest_Materials.h"
#include "RealtimeMeshSimple.h"
#include "Mesh/RealtimeMeshBasicShapeTools.h"

using namespace RealtimeMesh;

ARealtimeMeshVisualTest_Materials::ARealtimeMeshVisualTest_Materials()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ARealtimeMeshVisualTest_Materials::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	URealtimeMeshSimple* RealtimeMesh = GetRealtimeMeshComponent()->InitializeRealtimeMesh<URealtimeMeshSimple>();

	// Setup three material slots
	RealtimeMesh->SetupMaterialSlot(0, "Material0");
	RealtimeMesh->SetupMaterialSlot(1, "Material1");
	RealtimeMesh->SetupMaterialSlot(2, "Material2");

	const FRealtimeMeshLODKey LODKey(0);

	// Create three boxes, each using a different material slot
	for (int32 i = 0; i < 3; i++)
	{
		FRealtimeMeshStreamSet StreamSet;
		URealtimeMeshBasicShapeTools::AppendBoxMesh(
			StreamSet,
			FVector3f(50.0f, 50.0f, 50.0f),
			FTransform3f(FVector3f((i - 1) * 150.0f, 0.0f, 50.0f))
		);

		const FRealtimeMeshSectionGroupKey GroupKey = FRealtimeMeshSectionGroupKey::Create(LODKey, FName(*FString::Printf(TEXT("Box%d"), i)));
		const FRealtimeMeshSectionKey SectionKey = FRealtimeMeshSectionKey::CreateForPolyGroup(GroupKey, 0);

		RealtimeMesh->CreateSectionGroup(GroupKey, StreamSet);
		RealtimeMesh->UpdateSectionConfig(SectionKey, FRealtimeMeshSectionConfig(i));
	}
}
