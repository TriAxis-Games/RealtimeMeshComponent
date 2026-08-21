// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#include "Simple/RealtimeMeshExample_Simple_FastUpdate.h"
#include "RealtimeMeshSimple.h"
#include "Core/RealtimeMeshBuilder.h"
#include "Core/RealtimeMeshBufferSetConfig.h"

using namespace RealtimeMesh;

namespace
{
	// Single section group, reused across the build + every animated update.
	FRealtimeMeshBufferSetKey GetGroupKey()
	{
		return FRealtimeMeshBufferSetKey::Create(0, FName("FastUpdate"));
	}

	// A radial sine wave; same function used to build and to animate so the surface
	// is continuous. Time = 0 gives the flat-ish initial state.
	float WaveZ(float XPos, float YPos, float Time)
	{
		const float Distance = FMath::Sqrt(XPos * XPos + YPos * YPos);
		return FMath::Sin(Distance * 0.06f - Time) * 25.0f;
	}
}

ARealtimeMeshExample_Simple_FastUpdate::ARealtimeMeshExample_Simple_FastUpdate()
{
	// This example animates, so opt back into ticking (the base class disables it by default).
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	Description = NSLOCTEXT("RealtimeMeshExamples", "Simple_FastUpdate",
		"URealtimeMeshSimple: a Dynamic grid animated in place via EditMeshInPlace / "
		"EditMeshInPlaceRanged (fast path — no realloc, no republish).");
}

void ARealtimeMeshExample_Simple_FastUpdate::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	URealtimeMeshSimple* RealtimeMesh = GetRealtimeMeshComponent()->InitializeRealtimeMesh<URealtimeMeshSimple>();
	RealtimeMesh->SetupMaterialSlot(0, "DefaultMaterial");

	BuildInitialMesh();

	// Apply one frame so the mesh looks alive even before play (and, in MiddleBandRanged
	// mode, so the static-vs-animated regions are already distinguishable in the editor).
	if (Mode == ERealtimeMeshFastUpdateMode::MiddleBandRanged)
	{
		AnimateMiddleBand(0.0f);
	}
	else
	{
		AnimateFullGrid(0.0f);
	}

	VerifyMeshBuilt();
}

void ARealtimeMeshExample_Simple_FastUpdate::BeginPlay()
{
	Super::BeginPlay();
	AccumulatedTime = 0.0f;
}

void ARealtimeMeshExample_Simple_FastUpdate::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bEnableAnimation)
	{
		return;
	}

	AccumulatedTime += DeltaTime * AnimationSpeed;

	if (Mode == ERealtimeMeshFastUpdateMode::MiddleBandRanged)
	{
		AnimateMiddleBand(AccumulatedTime);
	}
	else
	{
		AnimateFullGrid(AccumulatedTime);
	}
}

void ARealtimeMeshExample_Simple_FastUpdate::BuildInitialMesh()
{
	URealtimeMeshSimple* RealtimeMesh = Cast<URealtimeMeshSimple>(GetRealtimeMeshComponent()->GetRealtimeMesh());
	if (!RealtimeMesh)
	{
		return;
	}

	const FRealtimeMeshBufferSetKey GroupKey = GetGroupKey();
	if (RealtimeMesh->GetSectionGroups(FRealtimeMeshLODKey(0)).Contains(GroupKey))
	{
		// Already built (e.g. a redundant OnConstruction); leave it for the in-place path.
		return;
	}

	FRealtimeMeshStreamSet StreamSet;
	TRealtimeMeshBuilderLocal<uint32, FPackedNormal, FVector2DHalf, 1> Builder(StreamSet);
	Builder.EnableTangents();
	Builder.EnableTexCoords();
	Builder.EnableColors();

	const int32 Width = GridSize + 1;
	for (int32 Y = 0; Y < Width; ++Y)
	{
		for (int32 X = 0; X < Width; ++X)
		{
			const float XPos = (X - GridSize * 0.5f) * Spacing;
			const float YPos = (Y - GridSize * 0.5f) * Spacing;

			// Flat to start; the in-place updates drive Z from here on.
			Builder.AddVertex(FVector3f(XPos, YPos, 0.0f))
				.SetNormalAndTangent(FVector3f(0.0f, 0.0f, 1.0f), FVector3f(1.0f, 0.0f, 0.0f))
				.SetTexCoord(FVector2f(X / static_cast<float>(GridSize), Y / static_cast<float>(GridSize)))
				.SetColor(FColor::White);
		}
	}

	for (int32 Y = 0; Y < GridSize; ++Y)
	{
		for (int32 X = 0; X < GridSize; ++X)
		{
			const int32 BottomLeft = Y * Width + X;
			const int32 BottomRight = BottomLeft + 1;
			const int32 TopLeft = BottomLeft + Width;
			const int32 TopRight = TopLeft + 1;

			Builder.AddTriangle(BottomLeft, TopLeft, TopRight);
			Builder.AddTriangle(BottomLeft, TopRight, BottomRight);
		}
	}

	// Dynamic draw type → BUF_Dynamic buffers → the in-place fast path is eligible.
	RealtimeMesh->CreateBufferSet(GroupKey, StreamSet, FRealtimeMeshBufferSetConfig(ERealtimeMeshSectionDrawType::Dynamic));
}

void ARealtimeMeshExample_Simple_FastUpdate::AnimateFullGrid(float Time)
{
	URealtimeMeshSimple* RealtimeMesh = Cast<URealtimeMeshSimple>(GetRealtimeMeshComponent()->GetRealtimeMesh());
	if (!RealtimeMesh)
	{
		return;
	}

	const int32 Width = GridSize + 1;

	// EditMeshInPlace edits the section group's CPU streams in place; eligible vertex
	// streams on a Dynamic group are then uploaded via RHILockBuffer with no realloc
	// and no new proxy version.
	RealtimeMesh->EditMeshInPlace(GetGroupKey(), [Time, Width](FRealtimeMeshStreamSet& Streams) -> TSet<FRealtimeMeshStreamKey>
	{
		FRealtimeMeshStream* Positions = Streams.Find(FRealtimeMeshStreams::Position);
		if (!Positions)
		{
			return {};
		}

		TRealtimeMeshStreamBuilder<FVector3f, void> Position(*Positions);
		for (int32 Index = 0; Index < Position.Num(); ++Index)
		{
			const int32 X = Index % Width;
			const int32 Y = Index / Width;
			FVector3f Vertex = Position.GetValue(Index);
			Vertex.Z = WaveZ((X - GridSize * 0.5f) * Spacing, (Y - GridSize * 0.5f) * Spacing, Time);
			Position.Set(Index, Vertex);
		}

		return { FRealtimeMeshStreams::Position };
	});
}

void ARealtimeMeshExample_Simple_FastUpdate::AnimateMiddleBand(float Time)
{
	URealtimeMeshSimple* RealtimeMesh = Cast<URealtimeMeshSimple>(GetRealtimeMeshComponent()->GetRealtimeMesh());
	if (!RealtimeMesh)
	{
		return;
	}

	const int32 Width = GridSize + 1;

	// Middle third of the rows. Row-major layout → a contiguous vertex element range.
	const int32 FirstRow = GridSize / 3;
	const int32 LastRow = GridSize - GridSize / 3;          // exclusive
	const int32 VertexStart = FirstRow * Width;
	const int32 VertexEnd = LastRow * Width;                 // half-open [start, end)

	// EditMeshInPlaceRanged uploads ONLY the returned element range. The top and bottom
	// bands are never written, so they stay flat — making the partial update visible.
	RealtimeMesh->EditMeshInPlaceRanged(GetGroupKey(),
		[Time, Width, VertexStart, VertexEnd](FRealtimeMeshStreamSet& Streams) -> TMap<FRealtimeMeshStreamKey, FInt32Range>
	{
		TMap<FRealtimeMeshStreamKey, FInt32Range> Updated;

		FRealtimeMeshStream* Positions = Streams.Find(FRealtimeMeshStreams::Position);
		if (!Positions)
		{
			return Updated;
		}

		TRealtimeMeshStreamBuilder<FVector3f, void> Position(*Positions);
		for (int32 Index = VertexStart; Index < VertexEnd; ++Index)
		{
			const int32 X = Index % Width;
			const int32 Y = Index / Width;
			FVector3f Vertex = Position.GetValue(Index);
			Vertex.Z = WaveZ((X - GridSize * 0.5f) * Spacing, (Y - GridSize * 0.5f) * Spacing, Time);
			Position.Set(Index, Vertex);
		}

		Updated.Add(FRealtimeMeshStreams::Position, FInt32Range(VertexStart, VertexEnd));
		return Updated;
	});
}
