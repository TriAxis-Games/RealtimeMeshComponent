// Fill out your copyright notice in the Description page of Project Settings.


#include "FunctionalTests/RealtimeMeshStressTestActor.h"

#include "RealtimeMeshLibrary.h"
#include "RealtimeMeshSimple.h"
#include "Mesh/RealtimeMeshBasicShapeTools.h"
#include "Mesh/RealtimeMeshBuilder.h"


ARealtimeMeshStressTestActor::ARealtimeMeshStressTestActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

static const auto SectionGroupKey = FRealtimeMeshSectionGroupKey::Create(0, FName("TestTripleBox"));

void ARealtimeMeshStressTestActor::OnGenerateMesh_Implementation()
{
	Super::OnGenerateMesh_Implementation();

	// Initialize the simple mesh
	URealtimeMeshSimple* RealtimeMesh = GetRealtimeMeshComponent()->InitializeRealtimeMesh<URealtimeMeshSimple>();
	
	// Setup the two material slots
	RealtimeMesh->SetupMaterialSlot(0, "PrimaryMaterial");
	RealtimeMesh->SetupMaterialSlot(1, "SecondaryMaterial");

	// Create a basic single section
	FRealtimeMeshSimpleMeshData MeshData;

	// This just adds a simple box, you can instead create your own mesh data
	URealtimeMeshSimpleBasicShapeTools::AppendBoxMesh(FVector(100, 100, 200), FTransform::Identity, MeshData, 2);
	URealtimeMeshSimpleBasicShapeTools::AppendBoxMesh(FVector(200, 100, 100), FTransform::Identity, MeshData, 1);
	URealtimeMeshSimpleBasicShapeTools::AppendBoxMesh(FVector(100, 200, 100), FTransform::Identity, MeshData, 3);

	RealtimeMesh->CreateSectionGroup(SectionGroupKey, MeshData);

	RealtimeMesh->UpdateSectionConfig(FRealtimeMeshSectionKey::CreateForPolyGroup(SectionGroupKey, 1), FRealtimeMeshSectionConfig(ERealtimeMeshSectionDrawType::Static, 0));
	RealtimeMesh->UpdateSectionConfig(FRealtimeMeshSectionKey::CreateForPolyGroup(SectionGroupKey, 2), FRealtimeMeshSectionConfig(ERealtimeMeshSectionDrawType::Dynamic, 0));
	RealtimeMesh->UpdateSectionConfig(FRealtimeMeshSectionKey::CreateForPolyGroup(SectionGroupKey, 3), FRealtimeMeshSectionConfig(ERealtimeMeshSectionDrawType::Static, 1));
		
}

void ARealtimeMeshStressTestActor::TickActor(float DeltaTime, ELevelTick TickType, FActorTickFunction& ThisTickFunction)
{
	if (URealtimeMeshSimple* RealtimeMesh = GetRealtimeMeshComponent()->GetRealtimeMeshAs<URealtimeMeshSimple>())
	{
		FRealtimeMeshSimpleMeshData MeshData;

		const float Scale = (FMath::Cos(FPlatformTime::Seconds() * PI) * 0.25f) + 0.5f;
		const FTransform Transform = FTransform(FQuat::Identity, FVector(0, 0, 0), FVector(Scale, Scale, Scale));

		// This just adds a simple box, you can instead create your own mesh data
		URealtimeMeshSimpleBasicShapeTools::AppendBoxMesh(FVector(100, 100, 200), Transform, MeshData, 2);
		URealtimeMeshSimpleBasicShapeTools::AppendBoxMesh(FVector(200, 100, 100), Transform, MeshData, 1);
		URealtimeMeshSimpleBasicShapeTools::AppendBoxMesh(FVector(100, 200, 100), Transform, MeshData, 3);

		RealtimeMesh->UpdateSectionGroup(SectionGroupKey, MeshData);
		
		Super::TickActor(DeltaTime, TickType, ThisTickFunction);
	}
}
