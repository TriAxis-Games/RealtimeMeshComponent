// Copyright (c) 2015-2024 TriAxis Games, L.L.C. All Rights Reserved.


#include "FunctionalTests/RealtimeMeshStressTestActor.h"

#include "MaterialDomain.h"
#include "RealtimeMeshLibrary.h"
#include "RealtimeMeshSimple.h"
#include "Mesh/RealtimeMeshBasicShapeTools.h"
#include "Core/RealtimeMeshBuilder.h"


ARealtimeMeshStressTestActor2::ARealtimeMeshStressTestActor2()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

static const auto SectionGroupKey = FRealtimeMeshSectionGroupKey::Create(0, FName("TestTripleBox"));

static const auto Section0Key = FRealtimeMeshSectionKey::Create(SectionGroupKey, FName("Section0"));
static const auto Section1Key = FRealtimeMeshSectionKey::Create(SectionGroupKey, FName("Section1"));
static const auto Section2Key = FRealtimeMeshSectionKey::Create(SectionGroupKey, FName("Section2"));

void ARealtimeMeshStressTestActor2::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Initialize the simple mesh
	URealtimeMeshSimple* RealtimeMesh = GetRealtimeMeshComponent()->InitializeRealtimeMesh<URealtimeMeshSimple>();

	// Setup the two material slots
	RealtimeMesh->SetupMaterialSlot(0, "PrimaryMaterial", UMaterial::GetDefaultMaterial(EMaterialDomain::MD_Surface));
	RealtimeMesh->SetupMaterialSlot(1, "SecondaryMaterial", UMaterial::GetDefaultMaterial(EMaterialDomain::MD_Surface));

	// Setup empty structure
	RealtimeMesh->CreateSectionGroup(SectionGroupKey, FRealtimeMeshSectionGroupConfig(ERealtimeMeshSectionDrawType::Static), false);
	RealtimeMesh->CreateSection(Section0Key, FRealtimeMeshSectionConfig(0), FRealtimeMeshStreamRange(), false);
	RealtimeMesh->CreateSection(Section1Key, FRealtimeMeshSectionConfig(1), FRealtimeMeshStreamRange(), false);
	RealtimeMesh->CreateSection(Section2Key, FRealtimeMeshSectionConfig(2), FRealtimeMeshStreamRange(), false);

}

void ARealtimeMeshStressTestActor2::TickActor(float DeltaTime, ELevelTick TickType, FActorTickFunction& ThisTickFunction)
{
	if (PendingGeneration.IsValid())
	{
		PendingGeneration.Wait();
		PendingGeneration.Reset();
	}
	
	if (URealtimeMeshSimple* RealtimeMesh = GetRealtimeMeshComponent()->GetRealtimeMeshAs<URealtimeMeshSimple>())
	{
		TSharedRef<TStrongObjectPtr<URealtimeMeshSimple>> RealtimeMeshRef = MakeShared<TStrongObjectPtr<URealtimeMeshSimple>>(RealtimeMesh);

		TPromise<TSharedPtr<TStrongObjectPtr<URealtimeMeshSimple>>> Promise;
		PendingGeneration = Promise.GetFuture();

		AsyncTask(ENamedThreads::AnyThread, [RealtimeMeshRef, Promise = MoveTemp(Promise)]() mutable
		{
			FRealtimeMeshStreamSet StreamSet;
			// Here we're using the builder just to initialize the stream set
			TRealtimeMeshBuilderLocal<uint16, FPackedNormal, FVector2DHalf, 2> Builder(StreamSet);
			Builder.EnableTangents();
			Builder.EnableTexCoords();
			Builder.EnableColors();

			const float Scale = (FMath::Cos(FPlatformTime::Seconds() * PI) * 0.25f) + 0.5f;
			const FTransform3f Transform = FTransform3f(FQuat4f::Identity, FVector3f(0, 0, 0), FVector3f(Scale, Scale, Scale));


			int32 Section0StartVertex = Builder.NumVertices();
			int32 Section0StartTriangle = Builder.NumTriangles();			
			URealtimeMeshBasicShapeTools::AppendBoxMesh(StreamSet, FVector3f(100.0f, 100.0f, 200.0f), Transform, 2, FColor::White);
			
			int32 Section1StartVertex = Builder.NumVertices();
			int32 Section1StartTriangle = Builder.NumTriangles();
			URealtimeMeshBasicShapeTools::AppendBoxMesh(StreamSet, FVector3f(200.0f, 100.0f, 100.0f), Transform, 1, FColor::White);
			
			int32 Section2StartVertex = Builder.NumVertices();
			int32 Section2StartTriangle = Builder.NumTriangles();
			URealtimeMeshBasicShapeTools::AppendBoxMesh(StreamSet, FVector3f(100.0f, 200.0f, 100.0f), Transform, 3, FColor::White);
			
			int32 FinalNumVertices = Builder.NumVertices();
			int32 FinalNumTriangles = Builder.NumTriangles();

			FRealtimeMeshStreamRange Section0Range(Section0StartVertex, Section1StartVertex, Section0StartTriangle * 3, Section1StartTriangle * 3);
			FRealtimeMeshStreamRange Section1Range(Section1StartVertex, Section2StartVertex, Section1StartTriangle * 3, Section2StartTriangle * 3);
			FRealtimeMeshStreamRange Section2Range(Section2StartVertex, FinalNumVertices, Section2StartTriangle * 3, FinalNumTriangles * 3);
			

			RealtimeMeshRef.Get()->UpdateSectionGroup(SectionGroupKey, MoveTemp(StreamSet));

			// Update the ranges for all the sections.
			RealtimeMeshRef.Get()->UpdateSectionRange(Section0Key, Section0Range);
			RealtimeMeshRef.Get()->UpdateSectionRange(Section1Key, Section1Range);
			RealtimeMeshRef.Get()->UpdateSectionRange(Section2Key, Section2Range);
			
			Promise.EmplaceValue(RealtimeMeshRef);
		});
	}
		
	Super::TickActor(DeltaTime, TickType, ThisTickFunction);
}
