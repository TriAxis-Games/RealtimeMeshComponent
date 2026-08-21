// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#include "Simple/RealtimeMeshExample_Simple_ComponentSetup.h"
#include "RealtimeMeshSimple.h"
#include "Engine/CollisionProfile.h"

using namespace RealtimeMesh;

ARealtimeMeshExample_Simple_ComponentSetup::ARealtimeMeshExample_Simple_ComponentSetup()
{
	PrimaryActorTick.bCanEverTick = false;

	// Create and configure the component ourselves, then make it the root. This is exactly what
	// ARealtimeMeshActor does internally — replicate it when adding a mesh to your own actor type.
	RealtimeMeshComponent = CreateDefaultSubobject<URealtimeMeshComponent>(TEXT("RealtimeMeshComponent"));
	RealtimeMeshComponent->SetMobility(EComponentMobility::Movable);
	RealtimeMeshComponent->SetGenerateOverlapEvents(false);
	RealtimeMeshComponent->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	SetRootComponent(RealtimeMeshComponent);
}

void ARealtimeMeshExample_Simple_ComponentSetup::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	URealtimeMeshSimple* RealtimeMesh = RealtimeMeshComponent->InitializeRealtimeMesh<URealtimeMeshSimple>();

	FRealtimeMeshStreamSet StreamSet;
	TRealtimeMeshBuilderLocal<uint16, FPackedNormal, FVector2DHalf, 1> Builder(StreamSet);
	Builder.EnableTangents();
	Builder.EnableTexCoords();
	Builder.EnableColors();
	Builder.EnablePolyGroups();

	const int32 V0 = Builder.AddVertex(FVector3f(-50.0f, 0.0f, 0.0f))
		.SetNormalAndTangent(FVector3f(0.0f, -1.0f, 1.0f), FVector3f(1.0f, 0.0f, 0.0f))
		.SetColor(FColor::Red)
		.SetTexCoord(FVector2f(0.0f, 0.0f));

	const int32 V1 = Builder.AddVertex(FVector3f(0.0f, 0.0f, 100.0f))
		.SetNormalAndTangent(FVector3f(0.0f, -1.0f, 1.0f), FVector3f(1.0f, 0.0f, 0.0f))
		.SetColor(FColor::Green)
		.SetTexCoord(FVector2f(0.5f, 1.0f));

	const int32 V2 = Builder.AddVertex(FVector3f(50.0, 0.0, 0.0))
		.SetNormalAndTangent(FVector3f(0.0f, -1.0f, 1.0f), FVector3f(1.0f, 0.0f, 0.0f))
		.SetColor(FColor::Blue)
		.SetTexCoord(FVector2f(1.0f, 0.0f));

	Builder.AddTriangle(V0, V1, V2, 0);
	Builder.AddTriangle(V2, V1, V0, 1);

	RealtimeMesh->SetupMaterialSlot(0, "PrimaryMaterial");
	RealtimeMesh->SetupMaterialSlot(1, "SecondaryMaterial");

	const FRealtimeMeshBufferSetKey GroupKey = FRealtimeMeshBufferSetKey::Create(0, FName("Triangle"));
	const FRealtimeMeshSectionKey PolyGroup0SectionKey = FRealtimeMeshSectionKey::CreateForPolyGroup(GroupKey, 0);
	const FRealtimeMeshSectionKey PolyGroup1SectionKey = FRealtimeMeshSectionKey::CreateForPolyGroup(GroupKey, 1);

	// This variant shows passing an explicit section-group config (Static draw type).
	RealtimeMesh->CreateBufferSet(GroupKey, StreamSet, FRealtimeMeshBufferSetConfig(ERealtimeMeshSectionDrawType::Static));

	RealtimeMesh->UpdateSectionConfig(PolyGroup0SectionKey, FRealtimeMeshSectionConfig(0));
	RealtimeMesh->UpdateSectionConfig(PolyGroup1SectionKey, FRealtimeMeshSectionConfig(1));
}
