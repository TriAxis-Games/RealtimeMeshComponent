// Copyright (c) 2015-2024 TriAxis Games, L.L.C. All Rights Reserved.


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

}

void ARealtimeMeshLatentUpdateTestActor::BeginPlay()
{
	Super::BeginPlay();
	
	// Initialize the simple mesh
	RealtimeMesh = GetRealtimeMeshComponent()->InitializeRealtimeMesh<URealtimeMeshSimple>();
	
	GroupA = FRealtimeMeshSectionGroupKey::Create(0, "MainGroup");
	GroupB = FRealtimeMeshSectionGroupKey::Create(0, "SecondaryGroup");
	
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

		RealtimeMesh->CreateSectionGroup(GroupA, MoveTemp(StreamSet));
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
		RealtimeMesh->CreateSectionGroup(GroupB, MoveTemp(StreamSet));

		RealtimeMesh->UpdateSectionConfig(FRealtimeMeshSectionKey::CreateForPolyGroup(SectionGroupKey, 1), FRealtimeMeshSectionConfig(ERealtimeMeshSectionDrawType::Static, 0));
		RealtimeMesh->UpdateSectionConfig(FRealtimeMeshSectionKey::CreateForPolyGroup(SectionGroupKey, 2), FRealtimeMeshSectionConfig(ERealtimeMeshSectionDrawType::Static, 1));
	}
}
