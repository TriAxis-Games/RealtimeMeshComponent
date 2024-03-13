// Copyright TriAxis Games, L.L.C. All Rights Reserved.


#include "FunctionalTests/RealtimeMeshUpdateTestActor.h"

#include "RealtimeMeshLibrary.h"
#include "RealtimeMeshSimple.h"
#include "RealtimeMeshCubeGeneratorExample.h"


ARealtimeMeshUpdateTestActor::ARealtimeMeshUpdateTestActor()
	: RealtimeMesh(nullptr)
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
	
	GroupKey = FRealtimeMeshSectionGroupKey::CreateUnique(0);
	RealtimeMesh->CreateSectionGroup(GroupKey);
		
	{	// Create a basic single sectiong
		// Create the new stream set and builder
		FRealtimeMeshStreamSet StreamSet;
		TRealtimeMeshBuilderLocal<uint16, FPackedNormal, FVector2DHalf, 1> Builder(StreamSet);
		Builder.EnableTangents();
		Builder.EnableTexCoords();
		Builder.EnablePolyGroups();
		Builder.EnableColors();
	
		// This example create 3 rectangular prisms, one on each axis, with all
		// of them sharing a single set of buffers, but using separate sections for separate materials
		AppendBox(Builder, FVector3f(100, 100, 200), 0);
		AppendBox(Builder, FVector3f(200, 100, 100), 1);
		AppendBox(Builder, FVector3f(100, 200, 100), 2);
		
		RealtimeMesh->UpdateSectionGroup(GroupKey, MoveTemp(StreamSet));
	}
}
