// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#include "Simple/RealtimeMeshExample_Simple_StreamBuilders.h"
#include "RealtimeMeshSimple.h"

using namespace RealtimeMesh;

ARealtimeMeshExample_Simple_StreamBuilders::ARealtimeMeshExample_Simple_StreamBuilders()
{
	Description = NSLOCTEXT("RealtimeMeshExamples", "Simple_StreamBuilders",
		"URealtimeMeshSimple: build a mesh from individual per-stream builders instead of the all-in-one local builder.");
}

void ARealtimeMeshExample_Simple_StreamBuilders::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	URealtimeMeshSimple* RealtimeMesh = GetRealtimeMeshComponent()->InitializeRealtimeMesh<URealtimeMeshSimple>();

	FRealtimeMeshStreamSet StreamSet;

	// Position stream, wrapped in a direct builder for FVector3f.
	TRealtimeMeshStreamBuilder<FVector3f> PositionBuilder(
		StreamSet.AddStream(FRealtimeMeshStreams::Position, GetRealtimeMeshBufferLayout<FVector3f>()));

	// Tangents stored at normal precision, but accessed as high precision through the builder's interface type.
	TRealtimeMeshStreamBuilder<FRealtimeMeshTangentsHighPrecision, FRealtimeMeshTangentsNormalPrecision> TangentBuilder(
		StreamSet.AddStream(FRealtimeMeshStreams::Tangents, GetRealtimeMeshBufferLayout<FRealtimeMeshTangentsNormalPrecision>()));

	// TexCoords stored as FVector2DHalf, accessed as FVector2f.
	TRealtimeMeshStreamBuilder<FVector2f, FVector2DHalf> TexCoordsBuilder(
		StreamSet.AddStream(FRealtimeMeshStreams::TexCoords, GetRealtimeMeshBufferLayout<FVector2DHalf>()));

	// Color stream, direct FColor builder.
	TRealtimeMeshStreamBuilder<FColor> ColorBuilder(
		StreamSet.AddStream(FRealtimeMeshStreams::Color, GetRealtimeMeshBufferLayout<FColor>()));

	// Triangle index stream stored as 16-bit, accessed as 32-bit.
	TRealtimeMeshStreamBuilder<TIndex3<uint32>, TIndex3<uint16>> TrianglesBuilder(
		StreamSet.AddStream(FRealtimeMeshStreams::Triangles, GetRealtimeMeshBufferLayout<TIndex3<uint16>>()));

	// Poly group stream stored as 16-bit, accessed as 32-bit.
	TRealtimeMeshStreamBuilder<uint32, uint16> PolygroupsBuilder(
		StreamSet.AddStream(FRealtimeMeshStreams::PolyGroups, GetRealtimeMeshBufferLayout<uint16>()));

	// You can reserve up front, exactly like a TArray.
	PositionBuilder.Reserve(3);
	TangentBuilder.Reserve(3);
	ColorBuilder.Reserve(3);
	TexCoordsBuilder.Reserve(3);
	TrianglesBuilder.Reserve(2);
	PolygroupsBuilder.Reserve(2);

	const int32 V0 = PositionBuilder.Add(FVector3f(-50.0f, 0.0f, 0.0f));
	TangentBuilder.Add(FRealtimeMeshTangentsHighPrecision(FVector3f(0.0f, -1.0f, 0.0f), FVector3f(1.0f, 0.0f, 0.0f)));
	ColorBuilder.Add(FColor::Red);
	TexCoordsBuilder.Add(FVector2f(0.0f, 0.0f));

	const int32 V1 = PositionBuilder.Add(FVector3f(0.0f, 0.0f, 100.0f));
	TangentBuilder.Add(FRealtimeMeshTangentsHighPrecision(FVector3f(0.0f, -1.0f, 0.0f), FVector3f(1.0f, 0.0f, 0.0f)));
	ColorBuilder.Add(FColor::Green);
	TexCoordsBuilder.Add(FVector2f(0.5f, 1.0f));

	const int32 V2 = PositionBuilder.Add(FVector3f(50.0, 0.0, 0.0));
	TangentBuilder.Add(FRealtimeMeshTangentsHighPrecision(FVector3f(0.0f, -1.0f, 0.0f), FVector3f(1.0f, 0.0f, 0.0f)));
	ColorBuilder.Add(FColor::Blue);
	TexCoordsBuilder.Add(FVector2f(1.0f, 0.0f));

	TrianglesBuilder.Add(TIndex3<uint32>(V0, V1, V2));
	PolygroupsBuilder.Add(0);

	TrianglesBuilder.Add(TIndex3<uint32>(V2, V1, V0));
	PolygroupsBuilder.Add(1);

	RealtimeMesh->SetupMaterialSlot(0, "PrimaryMaterial");
	RealtimeMesh->SetupMaterialSlot(1, "SecondaryMaterial");

	const FRealtimeMeshBufferSetKey GroupKey = FRealtimeMeshBufferSetKey::Create(0, FName("Triangle"));
	const FRealtimeMeshSectionKey PolyGroup0SectionKey = FRealtimeMeshSectionKey::CreateForPolyGroup(GroupKey, 0);
	const FRealtimeMeshSectionKey PolyGroup1SectionKey = FRealtimeMeshSectionKey::CreateForPolyGroup(GroupKey, 1);

	RealtimeMesh->CreateBufferSet(GroupKey, StreamSet);

	RealtimeMesh->UpdateSectionConfig(PolyGroup0SectionKey, FRealtimeMeshSectionConfig(0));
	RealtimeMesh->UpdateSectionConfig(PolyGroup1SectionKey, FRealtimeMeshSectionConfig(1));

	VerifyMeshBuilt();
}
