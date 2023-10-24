// Fill out your copyright notice in the Description page of Project Settings.


#include "FunctionalTests/RealtimeMeshLatentUpdateTestActor.h"

#include "RealtimeMeshLibrary.h"
#include "RealtimeMeshSimple.h"
#include "Mesh/RealtimeMeshBasicShapeTools.h"


ARealtimeMeshLatentUpdateTestActor::ARealtimeMeshLatentUpdateTestActor()
{
	
}


void ARealtimeMeshLatentUpdateTestActor::OnGenerateMesh_Implementation()
{
	Super::OnGenerateMesh_Implementation();

	// Initialize the simple mesh
	RealtimeMesh = GetRealtimeMeshComponent()->InitializeRealtimeMesh<URealtimeMeshSimple>();

	// This example create 3 rectangular prisms, one on each axis, with 2 of them grouped in the same vertex buffers, but with different sections
	// This allows for setting up separate materials even if sections share a single set of buffers.
	// Here we do a latent mesh submission, so we create the mesh section group and sections first, and then apply the mesh data later

	const FRealtimeMeshSimpleMeshData EmptyMeshData;
	
	// Create a single section, with its own dedicated section group
	RealtimeMesh->CreateSectionGroup(FRealtimeMeshSectionGroupKey::Create(0, FName(TEXT("Test"))), EmptyMeshData);
}

void ARealtimeMeshLatentUpdateTestActor::BeginPlay()
{
	Super::BeginPlay();
	
	{	// Create a basic single section
		FRealtimeMeshSimpleMeshData MeshData;

		// This just adds a simple box, you can instead create your own mesh data
		URealtimeMeshSimpleBasicShapeTools::AppendBoxMesh(FVector(100, 100, 200), FTransform::Identity, MeshData);

		RealtimeMesh->UpdateSectionGroup(StaticSectionKey, MeshData);

	}
	
	{	// Create a basic group with 2 sections
		FRealtimeMeshSimpleMeshData MeshData;

		// This just adds two simple boxes, one after the other
		URealtimeMeshSimpleBasicShapeTools::AppendBoxMesh(FVector(200, 100, 100), FTransform::Identity, MeshData);
		URealtimeMeshSimpleBasicShapeTools::AppendBoxMesh(FVector(100, 200, 100), FTransform::Identity, MeshData);

		const auto SectionGroupKey = FRealtimeMeshSectionGroupKey::Create(0, FName(TEXT("Test")));
		RealtimeMesh->UpdateSectionGroup(SectionGroupKey, MeshData);
		RealtimeMesh->UpdateSectionConfig(FRealtimeMeshSectionKey::CreateForPolyGroup(SectionGroupKey, 0), FRealtimeMeshSectionConfig(ERealtimeMeshSectionDrawType::Static, 0));
		RealtimeMesh->UpdateSectionConfig(FRealtimeMeshSectionKey::CreateForPolyGroup(SectionGroupKey, 0), FRealtimeMeshSectionConfig(ERealtimeMeshSectionDrawType::Static, 1));

	}
}
