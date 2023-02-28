// Fill out your copyright notice in the Description page of Project Settings.


#include "FunctionalTests/RealtimeMeshUpdateTestActor.h"

#include "RealtimeMeshLibrary.h"
#include "RealtimeMeshSimple.h"


ARealtimeMeshUpdateTestActor::ARealtimeMeshUpdateTestActor()
{
	
}


void ARealtimeMeshUpdateTestActor::OnGenerateMesh_Implementation()
{
	Super::OnGenerateMesh_Implementation();

	// Initialize the simple mesh
	RealtimeMesh = GetRealtimeMeshComponent()->InitializeRealtimeMesh<URealtimeMeshSimple>();

	// This example create 3 rectangular prisms, one on each axis, with 2 of them grouped in the same vertex buffers, but with different sections
	// This allows for setting up separate materials even if sections share a single set of buffers.
	// Here we do a latent mesh submission, so we create the mesh section group and sections first, and then apply the mesh data later
	
	FRealtimeMeshSimpleMeshData EmptyMeshData;
	
	// Create a single section, with its own dedicated section group
	StaticSectionKey = RealtimeMesh->CreateMeshSection(0, FRealtimeMeshSectionConfig(ERealtimeMeshSectionDrawType::Static, 0), EmptyMeshData, true);
	
	// Create a section group passing it our mesh data
	GroupKey = RealtimeMesh->CreateSectionGroupWithMesh(0, EmptyMeshData);

	// Create both sections on the same mesh data
	SectionInGroupA = RealtimeMesh->CreateSectionInGroup(GroupKey, FRealtimeMeshSectionConfig(ERealtimeMeshSectionDrawType::Static, 0), FRealtimeMeshStreamRange(), true);
	SectionInGroupB = RealtimeMesh->CreateSectionInGroup(GroupKey, FRealtimeMeshSectionConfig(ERealtimeMeshSectionDrawType::Static, 0), FRealtimeMeshStreamRange(), true);
	
	
	{	// Create a basic single section
		FRealtimeMeshSimpleMeshData MeshData;

		// This just adds a simple box, you can instead create your own mesh data
		URealtimeMeshBlueprintFunctionLibrary::AppendBoxMesh(FVector(100, 100, 200), FTransform::Identity, MeshData);

		RealtimeMesh->UpdateSectionMesh(StaticSectionKey, MeshData);

	}
	
	{	// Create a basic group with 2 sections
		FRealtimeMeshSimpleMeshData MeshData;

		// This just adds two simple boxes, one after the other
		URealtimeMeshBlueprintFunctionLibrary::AppendBoxMesh(FVector(200, 100, 100), FTransform::Identity, MeshData);
		URealtimeMeshBlueprintFunctionLibrary::AppendBoxMesh(FVector(100, 200, 100), FTransform::Identity, MeshData);

		RealtimeMesh->UpdateSectionGroupMesh(GroupKey, MeshData);
		RealtimeMesh->UpdateSectionSegment(SectionInGroupA, FRealtimeMeshStreamRange(0, 24, 0, 36));
		RealtimeMesh->UpdateSectionSegment(SectionInGroupB, FRealtimeMeshStreamRange(24, 48, 36, 72));

	}

}
