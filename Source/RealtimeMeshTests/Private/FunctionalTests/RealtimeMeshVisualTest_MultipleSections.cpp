// Copyright (c) 2015-2025 TriAxis Games, L.L.C. All Rights Reserved.

#include "FunctionalTests/RealtimeMeshVisualTest_MultipleSections.h"
#include "RealtimeMeshSimple.h"
#include "Mesh/RealtimeMeshBasicShapeTools.h"

using namespace RealtimeMesh;

ARealtimeMeshVisualTest_MultipleSections::ARealtimeMeshVisualTest_MultipleSections()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ARealtimeMeshVisualTest_MultipleSections::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	URealtimeMeshSimple* RealtimeMesh = GetRealtimeMeshComponent()->InitializeRealtimeMesh<URealtimeMeshSimple>();

	// Setup material slots for all sections
	RealtimeMesh->SetupMaterialSlot(0, "Material0");
	RealtimeMesh->SetupMaterialSlot(1, "Material1");
	RealtimeMesh->SetupMaterialSlot(2, "Material2");
	RealtimeMesh->SetupMaterialSlot(3, "Material3");
	RealtimeMesh->SetupMaterialSlot(4, "Material4");

	const FRealtimeMeshLODKey LODKey(0);

	// ========================================
	// GROUP 1: Three sections using polygroups 0, 1, 2
	// ========================================
	{
		FRealtimeMeshStreamSet StreamSet;
		TRealtimeMeshBuilderLocal<uint16, FPackedNormal, FVector2DHalf, 1> Builder(StreamSet);

		// Enable polygroups to allow multiple sections in one streamset
		Builder.EnablePolyGroups();

		// Box 1 (left) - Polygroup 0, Material slot 0
		URealtimeMeshBasicShapeTools::AppendBoxMesh(
			StreamSet,
			FVector3f(40.0f, 40.0f, 40.0f),
			FTransform3f(FVector3f(-250.0f, -100.0f, 40.0f)),
			0, // Polygroup index 0
			FColor::Red
		);

		// Box 2 (center) - Polygroup 1, Material slot 1
		URealtimeMeshBasicShapeTools::AppendBoxMesh(
			StreamSet,
			FVector3f(40.0f, 40.0f, 60.0f),
			FTransform3f(FVector3f(-250.0f, 0.0f, 60.0f)),
			1, // Polygroup index 1
			FColor::Green
		);

		// Box 3 (right) - Polygroup 2, Material slot 2
		URealtimeMeshBasicShapeTools::AppendBoxMesh(
			StreamSet,
			FVector3f(40.0f, 40.0f, 80.0f),
			FTransform3f(FVector3f(-250.0f, 100.0f, 80.0f)),
			2, // Polygroup index 2
			FColor::Blue
		);

		// Create the section group
		const FRealtimeMeshSectionGroupKey GroupKey = FRealtimeMeshSectionGroupKey::Create(LODKey, FName("Group1"));

		// Create section keys for each polygroup
		const FRealtimeMeshSectionKey Section0Key = FRealtimeMeshSectionKey::CreateForPolyGroup(GroupKey, 0);
		const FRealtimeMeshSectionKey Section1Key = FRealtimeMeshSectionKey::CreateForPolyGroup(GroupKey, 1);
		const FRealtimeMeshSectionKey Section2Key = FRealtimeMeshSectionKey::CreateForPolyGroup(GroupKey, 2);

		// Create the section group - this will automatically create sections for each polygroup
		RealtimeMesh->CreateSectionGroup(GroupKey, StreamSet);

		// Update section configurations to assign different material slots
		RealtimeMesh->UpdateSectionConfig(Section0Key, FRealtimeMeshSectionConfig(0));
		RealtimeMesh->UpdateSectionConfig(Section1Key, FRealtimeMeshSectionConfig(1));
		RealtimeMesh->UpdateSectionConfig(Section2Key, FRealtimeMeshSectionConfig(2));
	}

	// ========================================
	// GROUP 2: Two sections using polygroups 0, 1
	// ========================================
	{
		FRealtimeMeshStreamSet StreamSet;
		TRealtimeMeshBuilderLocal<uint16, FPackedNormal, FVector2DHalf, 1> Builder(StreamSet);

		// Enable polygroups to allow multiple sections in one streamset
		Builder.EnablePolyGroups();

		// Box 1 (left) - Polygroup 0, Material slot 3
		URealtimeMeshBasicShapeTools::AppendBoxMesh(
			StreamSet,
			FVector3f(50.0f, 50.0f, 50.0f),
			FTransform3f(FVector3f(100.0f, -75.0f, 50.0f)),
			0, // Polygroup index 0
			FColor::Yellow
		);

		// Box 2 (right) - Polygroup 1, Material slot 4
		URealtimeMeshBasicShapeTools::AppendBoxMesh(
			StreamSet,
			FVector3f(50.0f, 50.0f, 70.0f),
			FTransform3f(FVector3f(100.0f, 75.0f, 70.0f)),
			1, // Polygroup index 1
			FColor::Cyan
		);

		// Create the section group
		const FRealtimeMeshSectionGroupKey GroupKey = FRealtimeMeshSectionGroupKey::Create(LODKey, FName("Group2"));

		// Create section keys for each polygroup
		const FRealtimeMeshSectionKey Section0Key = FRealtimeMeshSectionKey::CreateForPolyGroup(GroupKey, 0);
		const FRealtimeMeshSectionKey Section1Key = FRealtimeMeshSectionKey::CreateForPolyGroup(GroupKey, 1);

		// Create the section group - this will automatically create sections for each polygroup
		RealtimeMesh->CreateSectionGroup(GroupKey, StreamSet);

		// Update section configurations to assign different material slots
		RealtimeMesh->UpdateSectionConfig(Section0Key, FRealtimeMeshSectionConfig(3));
		RealtimeMesh->UpdateSectionConfig(Section1Key, FRealtimeMeshSectionConfig(4));
	}
}
