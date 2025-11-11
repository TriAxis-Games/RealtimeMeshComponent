// Copyright (c) 2015-2025 TriAxis Games, L.L.C. All Rights Reserved.

#include "FunctionalTests/RealtimeMeshVisualTest_DynamicUpdates.h"
#include "RealtimeMeshSimple.h"
#include "Mesh/RealtimeMeshBasicShapeTools.h"

using namespace RealtimeMesh;

ARealtimeMeshVisualTest_DynamicUpdates::ARealtimeMeshVisualTest_DynamicUpdates()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void ARealtimeMeshVisualTest_DynamicUpdates::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	URealtimeMeshSimple* RealtimeMesh = GetRealtimeMeshComponent()->InitializeRealtimeMesh<URealtimeMeshSimple>();

	// Setup material slot
	RealtimeMesh->SetupMaterialSlot(0, "DefaultMaterial");

	UpdateMesh(0.0f);
}

void ARealtimeMeshVisualTest_DynamicUpdates::BeginPlay()
{
	Super::BeginPlay();
	AccumulatedTime = 0.0f;
}

void ARealtimeMeshVisualTest_DynamicUpdates::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bEnableAnimation)
	{
		AccumulatedTime += DeltaTime * AnimationSpeed;
		UpdateMesh(AccumulatedTime);
	}
}

void ARealtimeMeshVisualTest_DynamicUpdates::UpdateMesh(float Time)
{
	URealtimeMeshSimple* RealtimeMesh = Cast<URealtimeMeshSimple>(GetRealtimeMeshComponent()->GetRealtimeMesh());
	if (!RealtimeMesh)
	{
		return;
	}

	// Create a plane mesh with animated vertices
	FRealtimeMeshStreamSet StreamSet;
	TRealtimeMeshBuilderLocal<uint16, FPackedNormal, FVector2DHalf, 1> Builder(StreamSet);
	Builder.EnableTangents();
	Builder.EnableTexCoords();
	Builder.EnableColors();

	const int32 GridSize = 20;
	const float Spacing = 10.0f;
	const float WaveHeight = 20.0f;
	const float Frequency = 2.0f;

	// Generate grid vertices with wave animation
	for (int32 Y = 0; Y <= GridSize; Y++)
	{
		for (int32 X = 0; X <= GridSize; X++)
		{
			float XPos = (X - GridSize * 0.5f) * Spacing;
			float YPos = (Y - GridSize * 0.5f) * Spacing;

			// Create wave effect
			float Distance = FMath::Sqrt(XPos * XPos + YPos * YPos);
			float ZPos = FMath::Sin(Distance * 0.1f * Frequency - Time) * WaveHeight;

			FVector3f Position(XPos, YPos, ZPos);
			FVector2f TexCoord(X / (float)GridSize, Y / (float)GridSize);

			// Color based on height
			float ColorValue = (ZPos + WaveHeight) / (WaveHeight * 2.0f);
			FColor VertexColor = FLinearColor(ColorValue, 0.5f, 1.0f - ColorValue).ToFColor(false);

			Builder.AddVertex(Position)
				.SetNormalAndTangent(FVector3f(0.0f, 0.0f, 1.0f), FVector3f(1.0f, 0.0f, 0.0f))
				.SetTexCoord(TexCoord)
				.SetColor(VertexColor);
		}
	}

	// Generate triangles
	for (int32 Y = 0; Y < GridSize; Y++)
	{
		for (int32 X = 0; X < GridSize; X++)
		{
			int32 BottomLeft = Y * (GridSize + 1) + X;
			int32 BottomRight = BottomLeft + 1;
			int32 TopLeft = BottomLeft + (GridSize + 1);
			int32 TopRight = TopLeft + 1;

			// Two triangles per quad
			Builder.AddTriangle(BottomLeft, TopLeft, TopRight);
			Builder.AddTriangle(BottomLeft, TopRight, BottomRight);
		}
	}

	// Update or create the section group
	FRealtimeMeshSectionGroupKey GroupKey = FRealtimeMeshSectionGroupKey::Create(0, FName("DynamicMesh"));
	TArray<FRealtimeMeshSectionGroupKey> ExistingGroups = RealtimeMesh->GetSectionGroups(FRealtimeMeshLODKey(0));
	if (ExistingGroups.Contains(GroupKey))
	{
		RealtimeMesh->UpdateSectionGroup(GroupKey, StreamSet);
	}
	else
	{
		RealtimeMesh->CreateSectionGroup(GroupKey, StreamSet);
	}
}
