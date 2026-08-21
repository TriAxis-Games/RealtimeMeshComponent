// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#include "Simple/RealtimeMeshExample_Simple_LODs.h"
#include "RealtimeMeshSimple.h"
#include "Mesh/RealtimeMeshBasicShapeTools.h"

using namespace RealtimeMesh;

ARealtimeMeshExample_Simple_LODs::ARealtimeMeshExample_Simple_LODs()
{
	Description = NSLOCTEXT("RealtimeMeshExamples", "Simple_LODs",
		"URealtimeMeshSimple: four LODs, each a differently rotated/colored box, so LOD switches are easy to see.");
}

void ARealtimeMeshExample_Simple_LODs::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	URealtimeMeshSimple* RealtimeMesh = GetRealtimeMeshComponent()->InitializeRealtimeMesh<URealtimeMeshSimple>();

	RealtimeMesh->SetupMaterialSlot(0, "PrimaryMaterial");

	// Tighten LOD0's screen size so the lower LODs actually get a chance to show.
	RealtimeMesh->UpdateLODConfig(0, FRealtimeMeshLODConfig(0.75));

	const FColor Colors[4] = { FColor::White, FColor::Blue, FColor::Red, FColor::Green };
	const FTransform3f Transforms[4] = {
		FTransform3f::Identity,
		FTransform3f(FRotator3f(0.0f, 45.0f, 0.0f)),
		FTransform3f(FRotator3f(0.0f, 90.0f, 45.0f)),
		FTransform3f(FRotator3f(45.0f, 135.0f, 0.0f)) };

	for (int32 LODIndex = 0; LODIndex < 4; LODIndex++)
	{
		// LOD0 already exists; add the rest with progressively smaller screen-size thresholds.
		if (LODIndex > 0)
		{
			RealtimeMesh->AddLOD(FRealtimeMeshLODConfig(FMath::Pow(0.5f, LODIndex)));
		}

		// AppendBoxMesh builds the whole StreamSet through its own internal builder, so we must
		// NOT bind a second builder (e.g. TRealtimeMeshBuilderLocal) to the same set — a stream
		// can only belong to one linkage at a time or BindStream will assert.
		FRealtimeMeshStreamSet StreamSet;
		URealtimeMeshBasicShapeTools::AppendBoxMesh(StreamSet, FVector3f(100.0f, 100.0f, 100.0f), Transforms[LODIndex], 0, Colors[LODIndex]);

		// Section groups are keyed per LOD.
		const FRealtimeMeshBufferSetKey GroupKey = FRealtimeMeshBufferSetKey::Create(LODIndex, FName("Box"));
		const FRealtimeMeshSectionKey PolyGroup0SectionKey = FRealtimeMeshSectionKey::CreateForPolyGroup(GroupKey, 0);

		RealtimeMesh->CreateBufferSet(GroupKey, StreamSet);
		RealtimeMesh->UpdateSectionConfig(PolyGroup0SectionKey, FRealtimeMeshSectionConfig(0));
	}

	VerifyMeshBuilt();
}
