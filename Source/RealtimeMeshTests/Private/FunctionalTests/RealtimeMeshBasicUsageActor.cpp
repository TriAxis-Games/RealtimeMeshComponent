// Fill out your copyright notice in the Description page of Project Settings.


#include "FunctionalTests/RealtimeMeshBasicUsageActor.h"

#include "RealtimeMeshLibrary.h"
#include "RealtimeMeshSimple.h"
#include "Data/RealtimeMeshBuilder.h"
#include "DynamicMesh/DynamicMesh3.h"


ARealtimeMeshBasicUsageActor::ARealtimeMeshBasicUsageActor()
{
}

void ARealtimeMeshBasicUsageActor::OnGenerateMesh_Implementation()
{
	Super::OnGenerateMesh_Implementation();

	// Initialize the simple mesh
	URealtimeMeshSimple* RealtimeMesh = GetRealtimeMeshComponent()->InitializeRealtimeMesh<URealtimeMeshSimple>();
	
	// This example create 3 rectangular prisms, one on each axis, with 2 of them grouped in the same vertex buffers, but with different sections
	// This allows for setting up separate materials even if sections share a single set of buffers

	// Setup the two material slots
	RealtimeMesh->SetupMaterialSlot(0, "PrimaryMaterial");
	RealtimeMesh->SetupMaterialSlot(1, "SecondaryMaterial");

	{	// Create a basic single section
		FRealtimeMeshSimpleMeshData MeshData;

		// This just adds a simple box, you can instead create your own mesh data
		URealtimeMeshBlueprintFunctionLibrary::AppendBoxMesh(FVector(100, 100, 200), FTransform::Identity, MeshData);

		// Create a single section, with its own dedicated section group

		FRealtimeMeshSectionKey StaticSectionKey = FRealtimeMeshSectionKey::CreateUnique(FRealtimeMeshSectionGroupKey::CreateUnique(0));
		RealtimeMesh->CreateStandaloneSection(StaticSectionKey, FRealtimeMeshSectionConfig(ERealtimeMeshSectionDrawType::Static, 0), MeshData, true);

		RealtimeMesh->EditMeshInPlace(StaticSectionKey, [](FRealtimeMeshDataStreamRefSet& MeshData)
			{
				TRealtimeMeshBuilderLocal<> Builder(MeshData);

				Builder.SetColor(5, FColor::Red);

				// We only touched color so only push that
				return TSet { FRealtimeMeshStreams::Color };
			});
	}
	
	{	// Create a basic group with 2 sections
		FRealtimeMeshSimpleMeshData MeshData;

		// This just adds two simple boxes, one after the other
		URealtimeMeshBlueprintFunctionLibrary::AppendBoxMesh(FVector(200, 100, 100), FTransform::Identity, MeshData);
		URealtimeMeshBlueprintFunctionLibrary::AppendBoxMesh(FVector(100, 200, 100), FTransform::Identity, MeshData);

		// Create a section group passing it our mesh data
		FRealtimeMeshSectionGroupKey GroupKey = FRealtimeMeshSectionGroupKey::CreateUnique(0);
		RealtimeMesh->CreateSectionGroup(GroupKey, MeshData);

		// Create both sections on the same mesh data
		FRealtimeMeshSectionKey SectionInGroupA = FRealtimeMeshSectionKey::CreateUnique(GroupKey);
		RealtimeMesh->CreateSection(SectionInGroupA,
			FRealtimeMeshSectionConfig(ERealtimeMeshSectionDrawType::Static, 0),
			FRealtimeMeshStreamRange(0, 24, 0, 36), true);
		
		FRealtimeMeshSectionKey SectionInGroupB = FRealtimeMeshSectionKey::CreateUnique(GroupKey);
		RealtimeMesh->CreateSection(SectionInGroupB,
			FRealtimeMeshSectionConfig(ERealtimeMeshSectionDrawType::Static, 1),
			FRealtimeMeshStreamRange(24, 48, 36, 72), true);
	}
}
