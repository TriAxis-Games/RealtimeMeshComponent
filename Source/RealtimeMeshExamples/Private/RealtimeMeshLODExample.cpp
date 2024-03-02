// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#include "RealtimeMeshLODExample.h"
#include "Mesh/RealtimeMeshBasicShapeTools.h"

/*
 *	This example demonstrates how to create a mesh with multiple LODs.
 *	Each LOD has a simple box at a different rotation so it's very easy to see the LODs change.
 *
 */


// Sets default values
ARealtimeMeshLODExample::ARealtimeMeshLODExample()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ARealtimeMeshLODExample::OnGenerateMesh_Implementation()
{
	// Initialize to a simple mesh, this behaves the most like a ProceduralMeshComponent
	// Where you can set the mesh data and forget about it.
	URealtimeMeshSimple* RealtimeMesh = GetRealtimeMeshComponent()->InitializeRealtimeMesh<URealtimeMeshSimple>();

	// Setup the material slot
	RealtimeMesh->SetupMaterialSlot(0, "PrimaryMaterial");
	RealtimeMesh->UpdateLODConfig(0, FRealtimeMeshLODConfig(0.75));
	
	FColor Colors[4] = { FColor::White, FColor::Blue, FColor::Red, FColor::Green };
	FTransform3f Transforms[4] = {
		FTransform3f::Identity,
		FTransform3f(FRotator3f(0.0f, 45.0f, 0.0f)),
		FTransform3f(FRotator3f(0.0f, 90.0f, 45.0f)),
		FTransform3f(FRotator3f(45.0f, 135.0f, 0.0f)) };
	
	for (int32 LODIndex = 0; LODIndex < 4; LODIndex++)
	{
		// The first LOD is already created, so we only need to add the other LODs
		if (LODIndex > 0)
		{
			
			RealtimeMesh->AddLOD(FRealtimeMeshLODConfig(FMath::Pow(0.5f, LODIndex)));
		}
		
		// The most important part of the mesh data is the StreamSet, it contains the individual buffers,
		// like position, tangents, texcoords, triangles etc. 
		FRealtimeMeshStreamSet StreamSet;

		// For this example we'll use a helper class to build the mesh data
		// You can make your own helpers or skip them and use individual TRealtimeMeshStreamBuilder,
		// or skip them entirely and copy data directly into the streams
		TRealtimeMeshBuilderLocal<uint16, FPackedNormal, FVector2DHalf, 2> Builder(StreamSet);

		// here we go ahead and enable all the basic mesh data parts
		Builder.EnableTangents();
		Builder.EnableTexCoords();
		Builder.EnableColors();

		// Poly groups allow us to easily create a single set of buffers with multiple sections by adding an index to the triangle data
		Builder.EnablePolyGroups();

		URealtimeMeshBasicShapeTools::AppendBoxMesh(StreamSet, FVector3f(100.0f, 100.0f, 100.0f), Transforms[LODIndex], 0, Colors[LODIndex]);
	

		// Now create the group key. This is a unique identifier for the section group
		// A section group contains one or more sections that all share the underlying buffers
		// these sections can overlap the used vertex/index ranges depending on use case.
		const FRealtimeMeshSectionGroupKey GroupKey = FRealtimeMeshSectionGroupKey::Create(LODIndex, FName("TestBox"));

		// Now create the section key, this is a unique identifier for a section within a group
		// The section contains the configuration for the section, like the material slot,
		// and the draw type, as well as the range of the index/vertex buffers to use to render.
		// Here we're using the version to create the key based on the PolyGroup index
		const FRealtimeMeshSectionKey PolyGroup0SectionKey = FRealtimeMeshSectionKey::CreateForPolyGroup(GroupKey, 0);
	
		// Now we create the section group, since the stream set has polygroups, this will create the sections as well
		RealtimeMesh->CreateSectionGroup(GroupKey, StreamSet);

		// Update the configuration of both the polygroup sections.
		RealtimeMesh->UpdateSectionConfig(PolyGroup0SectionKey, FRealtimeMeshSectionConfig(ERealtimeMeshSectionDrawType::Static, 0));
	}
	
	Super::OnGenerateMesh_Implementation();
}

