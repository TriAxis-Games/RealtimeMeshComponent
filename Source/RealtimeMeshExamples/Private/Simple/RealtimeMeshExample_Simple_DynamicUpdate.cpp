// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#include "Simple/RealtimeMeshExample_Simple_DynamicUpdate.h"
#include "RealtimeMeshSimple.h"

using namespace RealtimeMesh;

ARealtimeMeshExample_Simple_DynamicUpdate::ARealtimeMeshExample_Simple_DynamicUpdate()
{
	// This example animates, so opt back into ticking (the base class disables it by default).
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	Description = NSLOCTEXT("RealtimeMeshExamples", "Simple_DynamicUpdate",
		"URealtimeMeshSimple: a grid that ripples every frame via UpdateSectionGroup.");
}

void ARealtimeMeshExample_Simple_DynamicUpdate::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	URealtimeMeshSimple* RealtimeMesh = GetRealtimeMeshComponent()->InitializeRealtimeMesh<URealtimeMeshSimple>();
	RealtimeMesh->SetupMaterialSlot(0, "DefaultMaterial");

	UpdateMesh(0.0f);

	VerifyMeshBuilt();
}

void ARealtimeMeshExample_Simple_DynamicUpdate::BeginPlay()
{
	Super::BeginPlay();
	AccumulatedTime = 0.0f;
}

void ARealtimeMeshExample_Simple_DynamicUpdate::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bEnableAnimation)
	{
		AccumulatedTime += DeltaTime * AnimationSpeed;
		UpdateMesh(AccumulatedTime);
	}
}

void ARealtimeMeshExample_Simple_DynamicUpdate::UpdateMesh(float Time)
{
	URealtimeMeshSimple* RealtimeMesh = Cast<URealtimeMeshSimple>(GetRealtimeMeshComponent()->GetRealtimeMesh());
	if (!RealtimeMesh)
	{
		return;
	}

	FRealtimeMeshStreamSet StreamSet;
	TRealtimeMeshBuilderLocal<uint16, FPackedNormal, FVector2DHalf, 1> Builder(StreamSet);
	Builder.EnableTangents();
	Builder.EnableTexCoords();
	Builder.EnableColors();

	const int32 GridSize = 20;
	const float Spacing = 10.0f;
	const float WaveHeight = 20.0f;
	const float Frequency = 2.0f;

	for (int32 Y = 0; Y <= GridSize; Y++)
	{
		for (int32 X = 0; X <= GridSize; X++)
		{
			const float XPos = (X - GridSize * 0.5f) * Spacing;
			const float YPos = (Y - GridSize * 0.5f) * Spacing;

			const float Distance = FMath::Sqrt(XPos * XPos + YPos * YPos);
			const float ZPos = FMath::Sin(Distance * 0.1f * Frequency - Time) * WaveHeight;

			const float ColorValue = (ZPos + WaveHeight) / (WaveHeight * 2.0f);
			const FColor VertexColor = FLinearColor(ColorValue, 0.5f, 1.0f - ColorValue).ToFColor(false);

			Builder.AddVertex(FVector3f(XPos, YPos, ZPos))
				.SetNormalAndTangent(FVector3f(0.0f, 0.0f, 1.0f), FVector3f(1.0f, 0.0f, 0.0f))
				.SetTexCoord(FVector2f(X / (float)GridSize, Y / (float)GridSize))
				.SetColor(VertexColor);
		}
	}

	for (int32 Y = 0; Y < GridSize; Y++)
	{
		for (int32 X = 0; X < GridSize; X++)
		{
			const int32 BottomLeft = Y * (GridSize + 1) + X;
			const int32 BottomRight = BottomLeft + 1;
			const int32 TopLeft = BottomLeft + (GridSize + 1);
			const int32 TopRight = TopLeft + 1;

			Builder.AddTriangle(BottomLeft, TopLeft, TopRight);
			Builder.AddTriangle(BottomLeft, TopRight, BottomRight);
		}
	}

	// Create the group on the first call, then just update it on subsequent frames.
	const FRealtimeMeshBufferSetKey GroupKey = FRealtimeMeshBufferSetKey::Create(0, FName("DynamicMesh"));
	const TArray<FRealtimeMeshBufferSetKey> ExistingGroups = RealtimeMesh->GetBufferSets(FRealtimeMeshLODKey(0));
	if (ExistingGroups.Contains(GroupKey))
	{
		RealtimeMesh->UpdateBufferSet(GroupKey, StreamSet);
	}
	else
	{
		RealtimeMesh->CreateBufferSet(GroupKey, StreamSet);
	}
}
