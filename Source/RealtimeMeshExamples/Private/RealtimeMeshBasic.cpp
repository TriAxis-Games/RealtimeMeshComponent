// Copyright TriAxis Games, L.L.C. All Rights Reserved.


#include "RealtimeMeshBasic.h"
#include "RealtimeMeshSimple.h"


// Sets default values
ARealtimeMeshBasic::ARealtimeMeshBasic()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ARealtimeMeshBasic::OnGenerateMesh_Implementation()
{
	// Initialize to a simple mesh, this behaves the most like a ProceduralMeshComponent
	// Where you can set the mesh data and forget about it.
	URealtimeMeshSimple* RealtimeMesh = GetRealtimeMeshComponent()->InitializeRealtimeMesh<URealtimeMeshSimple>();

	// The most important part of the mesh data is the StreamSet, it contains the individual buffers,
	// like position, tangents, texcoords, triangles etc. 
	FRealtimeMeshStreamSet StreamSet;
	
	// For this example we'll use a helper class to build the mesh data
	// You can make your own helpers or skip them and use individual TRealtimeMeshStreamBuilder,
	// or skip them entirely and copy data directly into the streams
	TRealtimeMeshBuilderLocal<uint16, FPackedNormal, FVector2DHalf, 1> Builder(StreamSet);

	// here we go ahead and enable all the basic mesh data parts
	Builder.EnableTangents();
	Builder.EnableTexCoords();
	Builder.EnableColors();

	// Poly groups allow us to easily create a single set of buffers with multiple sections by adding an index to the triangle data
	Builder.EnablePolyGroups();

	// Add our first vertex
	int32 V0 = Builder.AddVertex(FVector3f(-50.0f, 0.0f, 0.0f))
		.SetNormalAndTangent(FVector3f(0.0f, -1.0f, 0.0f), FVector3f(1.0f, 0.0f, 0.0f))
		.SetColor(FColor::Red)
		.SetTexCoord(FVector2f(0.0f, 0.0f));

	// Add our second vertex
	int32 V1 = Builder.AddVertex(FVector3f(0.0f, 0.0f, 100.0f))
		.SetNormalAndTangent(FVector3f(0.0f, -1.0f, 0.0f), FVector3f(1.0f, 0.0f, 0.0f))
		.SetColor(FColor::Green)
		.SetTexCoord(FVector2f(0.5f, 1.0f));

	// Add our third vertex
	int32 V2 = Builder.AddVertex(FVector3f(50.0, 0.0, 0.0))
		.SetNormalAndTangent(FVector3f(0.0f, -1.0f, 0.0f), FVector3f(1.0f, 0.0f, 0.0f))
		.SetColor(FColor::Blue)
		.SetTexCoord(FVector2f(1.0f, 0.0f));

	// Add our triangle, placing the vertices in counter clockwise order
	Builder.AddTriangle(V0, V1, V2, 0);

	// For this example we'll add the triangle again using reverse order so we can see the backface.
	// Usually you wouldn't want to do this, but in this case of a single triangle,
	// without it you'll only be able to see from a single side
	Builder.AddTriangle(V2, V1, V0, 1);
	
	// Setup the two material slots
	RealtimeMesh->SetupMaterialSlot(0, "PrimaryMaterial");
	RealtimeMesh->SetupMaterialSlot(1, "SecondaryMaterial");

	// Now create the group key. This is a unique identifier for the section group
	// A section group contains one or more sections that all share the underlying buffers
	// these sections can overlap the used vertex/index ranges depending on use case.
	const FRealtimeMeshSectionGroupKey GroupKey = FRealtimeMeshSectionGroupKey::Create(0, FName("TestTriangle"));

	// Now create the section key, this is a unique identifier for a section within a group
	// The section contains the configuration for the section, like the material slot,
	// and the draw type, as well as the range of the index/vertex buffers to use to render.
	// Here we're using the version to create the key based on the PolyGroup index
	const FRealtimeMeshSectionKey PolyGroup0SectionKey = FRealtimeMeshSectionKey::CreateForPolyGroup(GroupKey, 0);
	const FRealtimeMeshSectionKey PolyGroup1SectionKey = FRealtimeMeshSectionKey::CreateForPolyGroup(GroupKey, 1);
	
	// Now we create the section group, since the stream set has polygroups, this will create the sections as well
	RealtimeMesh->CreateSectionGroup(GroupKey, StreamSet);

	// Update the configuration of both the polygroup sections.
	RealtimeMesh->UpdateSectionConfig(PolyGroup0SectionKey, FRealtimeMeshSectionConfig(ERealtimeMeshSectionDrawType::Static, 0));
	RealtimeMesh->UpdateSectionConfig(PolyGroup1SectionKey, FRealtimeMeshSectionConfig(ERealtimeMeshSectionDrawType::Static, 1));
	
	Super::OnGenerateMesh_Implementation();
}


