// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#include "Procedural/RealtimeMeshExample_Procedural_Update.h"
#include "RealtimeMeshProcedural.h"

ARealtimeMeshExample_Procedural_Update::ARealtimeMeshExample_Procedural_Update()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	Description = NSLOCTEXT("RealtimeMeshExamples", "Procedural_Update",
		"URealtimeMeshProcedural: create a grid once, then animate only vertex positions via UpdateMeshSection.");
}

void ARealtimeMeshExample_Procedural_Update::ComputeGridVertices(float Time, TArray<FVector>& OutVertices) const
{
	OutVertices.Reset((GridSize + 1) * (GridSize + 1));

	const float Spacing = 10.0f;
	const float WaveHeight = 20.0f;

	for (int32 Y = 0; Y <= GridSize; Y++)
	{
		for (int32 X = 0; X <= GridSize; X++)
		{
			const double XPos = (X - GridSize * 0.5) * Spacing;
			const double YPos = (Y - GridSize * 0.5) * Spacing;
			const double Distance = FMath::Sqrt(XPos * XPos + YPos * YPos);
			const double ZPos = FMath::Sin(Distance * 0.1 - Time) * WaveHeight;
			OutVertices.Add(FVector(XPos, YPos, ZPos));
		}
	}
}

void ARealtimeMeshExample_Procedural_Update::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	URealtimeMeshProcedural* RealtimeMesh = GetRealtimeMeshComponent()->InitializeRealtimeMesh<URealtimeMeshProcedural>();

	// Build the fixed topology once.
	TArray<int32> Triangles;
	for (int32 Y = 0; Y < GridSize; Y++)
	{
		for (int32 X = 0; X < GridSize; X++)
		{
			const int32 BottomLeft = Y * (GridSize + 1) + X;
			const int32 BottomRight = BottomLeft + 1;
			const int32 TopLeft = BottomLeft + (GridSize + 1);
			const int32 TopRight = TopLeft + 1;

			Triangles.Add(BottomLeft); Triangles.Add(TopLeft); Triangles.Add(TopRight);
			Triangles.Add(BottomLeft); Triangles.Add(TopRight); Triangles.Add(BottomRight);
		}
	}

	TArray<FVector> Vertices;
	ComputeGridVertices(0.0f, Vertices);

	TArray<FVector> Normals;
	Normals.Init(FVector::UpVector, Vertices.Num());

	const TArray<FVector2D> EmptyUVs;
	const TArray<FColor> EmptyColors;
	const TArray<FRealtimeMeshProceduralTangent> EmptyTangents;

	RealtimeMesh->CreateMeshSection(0, Vertices, Triangles, Normals, EmptyUVs, EmptyColors, EmptyTangents, /*bCreateCollision*/ false);

	VerifyMeshBuilt();
}

void ARealtimeMeshExample_Procedural_Update::BeginPlay()
{
	Super::BeginPlay();
	AccumulatedTime = 0.0f;
}

void ARealtimeMeshExample_Procedural_Update::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	URealtimeMeshProcedural* RealtimeMesh = Cast<URealtimeMeshProcedural>(GetRealtimeMeshComponent()->GetRealtimeMesh());
	if (!RealtimeMesh)
	{
		return;
	}

	AccumulatedTime += DeltaTime * AnimationSpeed;

	TArray<FVector> Vertices;
	ComputeGridVertices(AccumulatedTime, Vertices);

	// Positions only — leave normals/UVs/colors/tangents untouched by passing empty arrays.
	const TArray<FVector> EmptyNormals;
	const TArray<FVector2D> EmptyUVs;
	const TArray<FColor> EmptyColors;
	const TArray<FRealtimeMeshProceduralTangent> EmptyTangents;
	RealtimeMesh->UpdateMeshSection(0, Vertices, EmptyNormals, EmptyUVs, EmptyColors, EmptyTangents);
}
