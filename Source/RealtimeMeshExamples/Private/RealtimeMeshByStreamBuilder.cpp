// Copyright TriAxis Games, L.L.C. All Rights Reserved.


#include "RealtimeMeshByStreamBuilder.h"
#include "RealtimeMeshSimple.h"


ARealtimeMeshByStreamBuilder::ARealtimeMeshByStreamBuilder()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ARealtimeMeshByStreamBuilder::OnGenerateMesh_Implementation()
{
	// Initialize to a simple mesh, this behaves the most like a ProceduralMeshComponent
	// Where you can set the mesh data and forget about it.
	URealtimeMeshSimple* RealtimeMesh = GetRealtimeMeshComponent()->InitializeRealtimeMesh<URealtimeMeshSimple>();

	// The most important part of the mesh data is the StreamSet, it contains the individual buffers,
	// like position, tangents, texcoords, triangles etc. 
	FRealtimeMeshStreamSet StreamSet;

	// Setup a stream for position of type FVector3f, and wrap it in a direct builder for FVector3f
	TRealtimeMeshStreamBuilder<FVector3f> PositionBuilder(StreamSet.AddStream(FRealtimeMeshStreams::Position, GetRealtimeMeshBufferLayout<FVector3f>()));
	
	// Setup a stream for tangents of t ype FRealtimeMeshTangentsNormalPrecision, and then wrap it in a
	// builder that lets us work with it in full precision by using the FRealtimeMeshTangentsHighPrecision as the interface type
	TRealtimeMeshStreamBuilder<FRealtimeMeshTangentsHighPrecision, FRealtimeMeshTangentsNormalPrecision> TangentBuilder(
		StreamSet.AddStream(FRealtimeMeshStreams::Tangents, GetRealtimeMeshBufferLayout<FRealtimeMeshTangentsNormalPrecision>()));

	// Setup a stream for texcoords of type FVector2DHalf, and wrap it a builder that lets us work with it as though it was FVector2f's
	TRealtimeMeshStreamBuilder<FVector2f, FVector2DHalf> TexCoordsBuilder(StreamSet.AddStream(FRealtimeMeshStreams::TexCoords, GetRealtimeMeshBufferLayout<FVector2DHalf>()));

	// Setup a stream for colors of type FColor, and wrap it in a direct builder for the FColor type
	TRealtimeMeshStreamBuilder<FColor> ColorBuilder(StreamSet.AddStream(FRealtimeMeshStreams::Color, GetRealtimeMeshBufferLayout<FColor>()));

	// Setup a stream for the triangles of type TIndex3<uint16> and wrap it in a builder that lets us work with it as though it was TIndex3<uint32>
	TRealtimeMeshStreamBuilder<TIndex3<uint32>, TIndex3<uint16>> TrianglesBuilder(StreamSet.AddStream(FRealtimeMeshStreams::Triangles, GetRealtimeMeshBufferLayout<TIndex3<uint16>>()));

	// Setup a stream for the polygroups of type uint16 and wrap it in a builder that lets us work with it as though it was uint32
	TRealtimeMeshStreamBuilder<uint32, uint16> PolygroupsBuilder(StreamSet.AddStream(FRealtimeMeshStreams::PolyGroups, GetRealtimeMeshBufferLayout<uint16>()));

	// This is kinda pointless for this example, but shows how you can reserve space in the buffers just like a TArray
	PositionBuilder.Reserve(3);
	TangentBuilder.Reserve(3);
	ColorBuilder.Reserve(3);
	TexCoordsBuilder.Reserve(3);

	TrianglesBuilder.Reserve(2);
	PolygroupsBuilder.Reserve(2);
	
	// Add first vertex
	int32 V0 = PositionBuilder.Add(FVector3f(-50.0f, 0.0f, 0.0f));
	TangentBuilder.Add(FRealtimeMeshTangentsHighPrecision(FVector3f(0.0f, -1.0f, 0.0f), FVector3f(1.0f, 0.0f, 0.0f)));
	ColorBuilder.Add(FColor::Red);
	TexCoordsBuilder.Add(FVector2f(0.0f, 0.0f));

	// Add second vertex
	int32 V1 = PositionBuilder.Add(FVector3f(0.0f, 0.0f, 100.0f));
	TangentBuilder.Add(FRealtimeMeshTangentsHighPrecision(FVector3f(0.0f, -1.0f, 0.0f), FVector3f(1.0f, 0.0f, 0.0f)));
	ColorBuilder.Add(FColor::Green);
	TexCoordsBuilder.Add(FVector2f(0.5f, 1.0f));

	// Add third vertex
	int32 V2 = PositionBuilder.Add(FVector3f(50.0, 0.0, 0.0));
	TangentBuilder.Add(FRealtimeMeshTangentsHighPrecision(FVector3f(0.0f, -1.0f, 0.0f), FVector3f(1.0f, 0.0f, 0.0f)));
	ColorBuilder.Add(FColor::Blue);
	TexCoordsBuilder.Add(FVector2f(1.0f, 0.0f));

	// Add first triangle and associated polygroup
	TrianglesBuilder.Add(TIndex3<uint32>(V0, V1, V2));
	PolygroupsBuilder.Add(0);

	// Add second triangle and associated polygroup
	TrianglesBuilder.Add(TIndex3<uint32>(V2, V1, V0));
	PolygroupsBuilder.Add(1);
	
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

