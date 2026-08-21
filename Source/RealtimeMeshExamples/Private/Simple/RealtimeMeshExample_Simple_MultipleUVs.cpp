// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#include "Simple/RealtimeMeshExample_Simple_MultipleUVs.h"
#include "RealtimeMeshSimple.h"

using namespace RealtimeMesh;

ARealtimeMeshExample_Simple_MultipleUVs::ARealtimeMeshExample_Simple_MultipleUVs()
{
	Description = NSLOCTEXT("RealtimeMeshExamples", "Simple_MultipleUVs",
		"URealtimeMeshSimple: a triangle with two UV channels per vertex.");
}

void ARealtimeMeshExample_Simple_MultipleUVs::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	URealtimeMeshSimple* RealtimeMesh = GetRealtimeMeshComponent()->InitializeRealtimeMesh<URealtimeMeshSimple>();

	FRealtimeMeshStreamSet StreamSet;

	// The trailing "2" is the number of texcoord channels this layout carries.
	TRealtimeMeshBuilderLocal<uint16, FPackedNormal, FVector2DHalf, 2> Builder(StreamSet);
	Builder.EnableTangents();
	Builder.EnableTexCoords();
	Builder.EnableColors();
	Builder.EnablePolyGroups();

	const int32 V0 = Builder.AddVertex(FVector3f(-50.0f, 0.0f, 0.0f))
		.SetNormalAndTangent(FVector3f(0.0f, -1.0f, 1.0f), FVector3f(1.0f, 0.0f, 0.0f))
		.SetColor(FColor::Red)
		.SetTexCoord(0, FVector2f(0.0f, 0.0f))
		.SetTexCoord(1, FVector2f(0.25f, 0.25f));

	const int32 V1 = Builder.AddVertex(FVector3f(0.0f, 0.0f, 100.0f))
		.SetNormalAndTangent(FVector3f(0.0f, -1.0f, 1.0f), FVector3f(1.0f, 0.0f, 0.0f))
		.SetColor(FColor::Green)
		.SetTexCoord(0, FVector2f(0.5f, 1.0f))
		.SetTexCoord(1, FVector2f(0.5f, 0.75f));

	const int32 V2 = Builder.AddVertex(FVector3f(50.0, 0.0, 0.0))
		.SetNormalAndTangent(FVector3f(0.0f, -1.0f, 1.0f), FVector3f(1.0f, 0.0f, 0.0f))
		.SetColor(FColor::Blue)
		.SetTexCoord(0, FVector2f(1.0f, 0.0f))
		.SetTexCoord(1, FVector2f(0.75f, 0.25f));

	Builder.AddTriangle(V0, V1, V2, 0);
	Builder.AddTriangle(V2, V1, V0, 1);

	RealtimeMesh->SetupMaterialSlot(0, "PrimaryMaterial");
	RealtimeMesh->SetupMaterialSlot(1, "SecondaryMaterial");

	const FRealtimeMeshBufferSetKey GroupKey = FRealtimeMeshBufferSetKey::Create(0, FName("Triangle"));
	const FRealtimeMeshSectionKey PolyGroup0SectionKey = FRealtimeMeshSectionKey::CreateForPolyGroup(GroupKey, 0);
	const FRealtimeMeshSectionKey PolyGroup1SectionKey = FRealtimeMeshSectionKey::CreateForPolyGroup(GroupKey, 1);

	RealtimeMesh->CreateBufferSet(GroupKey, StreamSet, ERealtimeMeshSectionDrawType::Static);

	RealtimeMesh->UpdateSectionConfig(PolyGroup0SectionKey, FRealtimeMeshSectionConfig(0));
	RealtimeMesh->UpdateSectionConfig(PolyGroup1SectionKey, FRealtimeMeshSectionConfig(1));

	VerifyMeshBuilt();
}
