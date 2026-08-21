// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#include "Simple/RealtimeMeshExample_Simple_MultipleSections.h"
#include "RealtimeMeshSimple.h"

using namespace RealtimeMesh;

ARealtimeMeshExample_Simple_MultipleSections::ARealtimeMeshExample_Simple_MultipleSections()
{
	Description = NSLOCTEXT("RealtimeMeshExamples", "Simple_MultipleSections",
		"URealtimeMeshSimple: one section group with several poly-group sections sharing the buffers, each on its own material slot.");
}

void ARealtimeMeshExample_Simple_MultipleSections::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	URealtimeMeshSimple* RealtimeMesh = GetRealtimeMeshComponent()->InitializeRealtimeMesh<URealtimeMeshSimple>();

	static constexpr int32 NumTiles = 5;

	// One material slot per tile. Assign distinct materials to these slots in the editor to see
	// each section pick up its own material.
	for (int32 Slot = 0; Slot < NumTiles; ++Slot)
	{
		RealtimeMesh->SetupMaterialSlot(Slot, FName(*FString::Printf(TEXT("Material%d"), Slot)));
	}

	const FColor Colors[NumTiles] = { FColor::Red, FColor::Green, FColor::Blue, FColor::Yellow, FColor::Cyan };

	// A single StreamSet / single builder. Poly groups let several sections share these buffers;
	// each tile's triangles are tagged with a distinct poly-group index.
	FRealtimeMeshStreamSet StreamSet;
	TRealtimeMeshBuilderLocal<uint16, FPackedNormal, FVector2DHalf, 1> Builder(StreamSet);
	Builder.EnableTangents();
	Builder.EnableTexCoords();
	Builder.EnableColors();
	Builder.EnablePolyGroups();

	for (int32 Tile = 0; Tile < NumTiles; ++Tile)
	{
		const float Cx = (Tile - (NumTiles - 1) * 0.5f) * 120.0f;
		const FColor Color = Colors[Tile];
		const FVector3f Normal(0.0f, 0.0f, 1.0f);
		const FVector3f Tangent(1.0f, 0.0f, 0.0f);

		// A flat 100x100 quad lying in XY, facing +Z. Wound (BL,TL,TR)/(BL,TR,BR) to face up.
		const int32 BL = Builder.AddVertex(FVector3f(Cx - 50.0f, -50.0f, 0.0f)).SetNormalAndTangent(Normal, Tangent).SetColor(Color).SetTexCoord(FVector2f(0.0f, 0.0f));
		const int32 BR = Builder.AddVertex(FVector3f(Cx + 50.0f, -50.0f, 0.0f)).SetNormalAndTangent(Normal, Tangent).SetColor(Color).SetTexCoord(FVector2f(1.0f, 0.0f));
		const int32 TR = Builder.AddVertex(FVector3f(Cx + 50.0f, 50.0f, 0.0f)).SetNormalAndTangent(Normal, Tangent).SetColor(Color).SetTexCoord(FVector2f(1.0f, 1.0f));
		const int32 TL = Builder.AddVertex(FVector3f(Cx - 50.0f, 50.0f, 0.0f)).SetNormalAndTangent(Normal, Tangent).SetColor(Color).SetTexCoord(FVector2f(0.0f, 1.0f));

		// Poly-group index == Tile, so each tile becomes its own auto-created section.
		Builder.AddTriangle(BL, TL, TR, Tile);
		Builder.AddTriangle(BL, TR, BR, Tile);
	}

	// Creating the group auto-creates one section per poly group (each defaulting to material slot == poly-group index).
	const FRealtimeMeshBufferSetKey GroupKey = FRealtimeMeshBufferSetKey::Create(0, FName("Tiles"));
	RealtimeMesh->CreateBufferSet(GroupKey, StreamSet);

	// Make the material-slot mapping explicit (section for poly group i -> material slot i).
	for (int32 Tile = 0; Tile < NumTiles; ++Tile)
	{
		RealtimeMesh->UpdateSectionConfig(FRealtimeMeshSectionKey::CreateForPolyGroup(GroupKey, Tile), FRealtimeMeshSectionConfig(Tile));
	}

	VerifyMeshBuilt();
}
