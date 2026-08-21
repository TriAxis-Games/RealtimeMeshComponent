// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#include "Simple/RealtimeMeshExample_Simple_AsyncBuild.h"
#include "RealtimeMeshSimple.h"
#include "Core/RealtimeMeshFuture.h"

using namespace RealtimeMesh;

namespace
{
	// Pure-CPU mesh build — safe to run on a worker thread (no UObject access).
	FRealtimeMeshStreamSet BuildGridStreamSet()
	{
		FRealtimeMeshStreamSet StreamSet;
		TRealtimeMeshBuilderLocal<uint32, FPackedNormal, FVector2DHalf, 1> Builder(StreamSet);
		Builder.EnableTangents();
		Builder.EnableTexCoords();
		Builder.EnableColors();

		const int32 GridSize = 64;
		const float Spacing = 10.0f;

		for (int32 Y = 0; Y <= GridSize; Y++)
		{
			for (int32 X = 0; X <= GridSize; X++)
			{
				const float XPos = (X - GridSize * 0.5f) * Spacing;
				const float YPos = (Y - GridSize * 0.5f) * Spacing;
				const float ZPos = FMath::Sin(X * 0.3f) * FMath::Cos(Y * 0.3f) * 30.0f;

				Builder.AddVertex(FVector3f(XPos, YPos, ZPos))
					.SetNormalAndTangent(FVector3f(0.0f, 0.0f, 1.0f), FVector3f(1.0f, 0.0f, 0.0f))
					.SetTexCoord(FVector2f(X / (float)GridSize, Y / (float)GridSize))
					.SetColor(FColor::White);
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

		return StreamSet;
	}
}

ARealtimeMeshExample_Simple_AsyncBuild::ARealtimeMeshExample_Simple_AsyncBuild()
{
	Description = NSLOCTEXT("RealtimeMeshExamples", "Simple_AsyncBuild",
		"URealtimeMeshSimple: build the StreamSet on a worker thread, then commit on the game thread (DoOnAsyncThread + ContinueOnGameThread).");
}

void ARealtimeMeshExample_Simple_AsyncBuild::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Initialize the mesh and material here; the geometry itself is built asynchronously in BeginPlay.
	URealtimeMeshSimple* RealtimeMesh = GetRealtimeMeshComponent()->InitializeRealtimeMesh<URealtimeMeshSimple>();
	RealtimeMesh->SetupMaterialSlot(0, "DefaultMaterial");
}

void ARealtimeMeshExample_Simple_AsyncBuild::BeginPlay()
{
	Super::BeginPlay();

	URealtimeMeshSimple* RealtimeMesh = Cast<URealtimeMeshSimple>(GetRealtimeMeshComponent()->GetRealtimeMesh());
	if (!RealtimeMesh)
	{
		return;
	}

	TWeakObjectPtr<URealtimeMeshSimple> WeakMesh(RealtimeMesh);
	TWeakObjectPtr<ARealtimeMeshExample_Simple_AsyncBuild> WeakThis(this);

	// 1) Build the heavy StreamSet off the game thread.
	TFuture<FRealtimeMeshStreamSet> BuildFuture = DoOnAsyncThread([]()
	{
		return BuildGridStreamSet();
	});

	// 2) Hop back to the game thread to create the section group with the finished data.
	ContinueOnGameThread(MoveTemp(BuildFuture), [WeakMesh, WeakThis](TFuture<FRealtimeMeshStreamSet>&& Result)
	{
		const FRealtimeMeshStreamSet& StreamSet = Result.Get();
		if (URealtimeMeshSimple* Mesh = WeakMesh.Get())
		{
			Mesh->CreateBufferSet(FRealtimeMeshBufferSetKey::Create(0, FName("AsyncMesh")), StreamSet);
			if (ARealtimeMeshExample_Simple_AsyncBuild* Self = WeakThis.Get())
			{
				Self->VerifyMeshBuilt();
			}
		}
	});
}
