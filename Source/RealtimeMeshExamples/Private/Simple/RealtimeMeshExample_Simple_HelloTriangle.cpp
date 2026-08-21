// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#include "Simple/RealtimeMeshExample_Simple_HelloTriangle.h"
#include "RealtimeMeshSimple.h"

using namespace RealtimeMesh;

ARealtimeMeshExample_Simple_HelloTriangle::ARealtimeMeshExample_Simple_HelloTriangle()
{
	Description = NSLOCTEXT("RealtimeMeshExamples", "Simple_HelloTriangle",
		"URealtimeMeshSimple: a single two-sided triangle. The minimal create-a-mesh path.");
}

void ARealtimeMeshExample_Simple_HelloTriangle::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Initialize to a simple mesh, this behaves the most like a ProceduralMeshComponent
	// where you can set the mesh data and forget about it.
	URealtimeMeshSimple* RealtimeMesh = GetRealtimeMeshComponent()->InitializeRealtimeMesh<URealtimeMeshSimple>();

	// The most important part of the mesh data is the StreamSet, it contains the individual buffers,
	// like position, tangents, texcoords, triangles etc.
	FRealtimeMeshStreamSet StreamSet;

	// For this example we'll use a helper class to build the mesh data. You can make your own helpers
	// or skip them and use individual TRealtimeMeshStreamBuilder (see the StreamBuilders example),
	// or skip them entirely and copy data directly into the streams.
	TRealtimeMeshBuilderLocal<uint16, FPackedNormal, FVector2DHalf, 1> Builder(StreamSet);

	// Enable the basic mesh data parts.
	Builder.EnableTangents();
	Builder.EnableTexCoords();
	Builder.EnableColors();

	// Poly groups let us pack multiple sections into one set of buffers by tagging each triangle.
	Builder.EnablePolyGroups();

	const int32 V0 = Builder.AddVertex(FVector3f(-50.0f, 0.0f, 0.0f))
		.SetNormalAndTangent(FVector3f(0.0f, -1.0f, 0.0f), FVector3f(1.0f, 0.0f, 0.0f))
		.SetColor(FColor::Red)
		.SetTexCoord(FVector2f(0.0f, 0.0f));

	const int32 V1 = Builder.AddVertex(FVector3f(0.0f, 0.0f, 100.0f))
		.SetNormalAndTangent(FVector3f(0.0f, -1.0f, 0.0f), FVector3f(1.0f, 0.0f, 0.0f))
		.SetColor(FColor::Green)
		.SetTexCoord(FVector2f(0.5f, 1.0f));

	const int32 V2 = Builder.AddVertex(FVector3f(50.0, 0.0, 0.0))
		.SetNormalAndTangent(FVector3f(0.0f, -1.0f, 0.0f), FVector3f(1.0f, 0.0f, 0.0f))
		.SetColor(FColor::Blue)
		.SetTexCoord(FVector2f(1.0f, 0.0f));

	// Add the triangle (counter-clockwise) to poly group 0.
	Builder.AddTriangle(V0, V1, V2, 0);

	// Add it again with reversed winding into poly group 1 so the back face is also visible.
	Builder.AddTriangle(V2, V1, V0, 1);

	// Two material slots, one per poly group / section.
	RealtimeMesh->SetupMaterialSlot(0, "PrimaryMaterial");
	RealtimeMesh->SetupMaterialSlot(1, "SecondaryMaterial");

	// A section group owns the shared buffers; sections inside it reference ranges of those buffers.
	const FRealtimeMeshBufferSetKey GroupKey = FRealtimeMeshBufferSetKey::Create(0, FName("HelloTriangle"));

	// Keys for the two poly-group sections that the group will auto-create.
	const FRealtimeMeshSectionKey PolyGroup0SectionKey = FRealtimeMeshSectionKey::CreateForPolyGroup(GroupKey, 0);
	const FRealtimeMeshSectionKey PolyGroup1SectionKey = FRealtimeMeshSectionKey::CreateForPolyGroup(GroupKey, 1);

	// Because the StreamSet has poly groups, creating the group auto-creates a section per group.
	RealtimeMesh->CreateBufferSet(GroupKey, StreamSet);

	// Point each section at its material slot.
	RealtimeMesh->UpdateSectionConfig(PolyGroup0SectionKey, FRealtimeMeshSectionConfig(0));
	RealtimeMesh->UpdateSectionConfig(PolyGroup1SectionKey, FRealtimeMeshSectionConfig(1));

	VerifyMeshBuilt();
}
