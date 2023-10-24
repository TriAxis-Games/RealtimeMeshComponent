// Fill out your copyright notice in the Description page of Project Settings.


#include "FunctionalTests/RealtimeMeshUpdateTestActor.h"

#include "RealtimeMeshLibrary.h"
#include "RealtimeMeshSimple.h"
#include "Mesh/RealtimeMeshBasicShapeTools.h"
#include "Mesh/RealtimeMeshSimpleData.h"


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

	GroupKey = FRealtimeMeshSectionGroupKey::CreateUnique(0);
	RealtimeMesh->CreateSectionGroup(GroupKey);
		
	{	// Create a basic single section
		FRealtimeMeshSimpleMeshData MeshData;

		// This just adds a simple box, you can instead create your own mesh data
		URealtimeMeshSimpleBasicShapeTools::AppendBoxMesh(FVector(100, 100, 200), FTransform::Identity, MeshData, 0);
		URealtimeMeshSimpleBasicShapeTools::AppendBoxMesh(FVector(200, 100, 100), FTransform::Identity, MeshData, 2);
		URealtimeMeshSimpleBasicShapeTools::AppendBoxMesh(FVector(100, 200, 100), FTransform::Identity, MeshData, 1);

		RealtimeMesh->UpdateSectionGroup(GroupKey, MeshData);
	}
}
