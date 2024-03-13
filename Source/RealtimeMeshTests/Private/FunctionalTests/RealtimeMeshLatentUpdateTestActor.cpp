// Copyright TriAxis Games, L.L.C. All Rights Reserved.


#include "FunctionalTests/RealtimeMeshLatentUpdateTestActor.h"

#include "RealtimeMeshLibrary.h"
#include "RealtimeMeshSimple.h"
#include "RealtimeMeshCubeGeneratorExample.h"


ARealtimeMeshLatentUpdateTestActor::ARealtimeMeshLatentUpdateTestActor()
	: RealtimeMesh(nullptr)
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

	FRealtimeMeshStreamSet StreamSet;
	TRealtimeMeshBuilderLocal<uint16, FPackedNormal, FVector2DHalf, 1> Builder(StreamSet);
	Builder.EnableTangents();
	Builder.EnableTexCoords();
	Builder.EnablePolyGroups();
	Builder.EnableColors();
	
	// Create a single section, with its own dedicated section group
	RealtimeMesh->CreateSectionGroup(FRealtimeMeshSectionGroupKey::Create(0, FName(TEXT("Test"))), StreamSet);
}

void ARealtimeMeshLatentUpdateTestActor::BeginPlay()
{
	Super::BeginPlay();
	
	{	// Create a basic single section
		
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

		RealtimeMesh->UpdateSectionGroup(StaticSectionKey, MoveTemp(StreamSet));
	}
	
	{	// Create a basic group with 2 sections
		// Create the new stream set and builder
		FRealtimeMeshStreamSet StreamSet;
		TRealtimeMeshBuilderLocal<uint16, FPackedNormal, FVector2DHalf, 1> Builder(StreamSet);
		Builder.EnableTangents();
		Builder.EnableTexCoords();
		Builder.EnablePolyGroups();
		Builder.EnableColors();
	
		// This example create 3 rectangular prisms, one on each axis, with all
		// of them sharing a single set of buffers, but using separate sections for separate materials
		AppendBox(Builder, FVector3f(200, 100, 100), 1);
		AppendBox(Builder, FVector3f(100, 200, 100), 2);
		
		const auto SectionGroupKey = FRealtimeMeshSectionGroupKey::Create(0, FName(TEXT("Test")));
		RealtimeMesh->UpdateSectionGroup(StaticSectionKey, MoveTemp(StreamSet));

		RealtimeMesh->UpdateSectionConfig(FRealtimeMeshSectionKey::CreateForPolyGroup(SectionGroupKey, 1), FRealtimeMeshSectionConfig(ERealtimeMeshSectionDrawType::Static, 0));
		RealtimeMesh->UpdateSectionConfig(FRealtimeMeshSectionKey::CreateForPolyGroup(SectionGroupKey, 2), FRealtimeMeshSectionConfig(ERealtimeMeshSectionDrawType::Static, 1));
	}
}
