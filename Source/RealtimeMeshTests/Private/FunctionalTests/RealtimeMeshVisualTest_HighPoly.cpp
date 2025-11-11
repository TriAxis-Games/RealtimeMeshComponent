// Copyright (c) 2015-2025 TriAxis Games, L.L.C. All Rights Reserved.

#include "FunctionalTests/RealtimeMeshVisualTest_HighPoly.h"
#include "RealtimeMeshSimple.h"
#include "Mesh/RealtimeMeshBasicShapeTools.h"

using namespace RealtimeMesh;

ARealtimeMeshVisualTest_HighPoly::ARealtimeMeshVisualTest_HighPoly()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ARealtimeMeshVisualTest_HighPoly::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	URealtimeMeshSimple* RealtimeMesh = GetRealtimeMeshComponent()->InitializeRealtimeMesh<URealtimeMeshSimple>();

	// Setup material slot
	RealtimeMesh->SetupMaterialSlot(0, "DefaultMaterial");

	const FRealtimeMeshLODKey LODKey(0);

	// Create a high-poly sphere using subdivision
	// Segments = 16 * 2^SubdivisionLevel
	int32 NumSegments = 16 * (1 << SubdivisionLevel);
	int32 NumRings = 8 * (1 << SubdivisionLevel);

	// Clamp to reasonable values to avoid excessive poly counts
	NumSegments = FMath::Clamp(NumSegments, 16, 512);
	NumRings = FMath::Clamp(NumRings, 8, 256);

	FRealtimeMeshStreamSet StreamSet;
	TRealtimeMeshBuilderLocal<uint32, FPackedNormal, FVector2DHalf, 1> Builder(StreamSet);
	Builder.EnableTangents();
	Builder.EnableTexCoords();
	Builder.EnableColors();

	// Generate high-poly sphere with color variation based on position
	const float Radius = 50.0f;

	// Create vertices
	for (int32 Ring = 0; Ring <= NumRings; Ring++)
	{
		float Theta = (float)Ring / (float)NumRings * PI;
		float SinTheta = FMath::Sin(Theta);
		float CosTheta = FMath::Cos(Theta);

		for (int32 Segment = 0; Segment <= NumSegments; Segment++)
		{
			float Phi = (float)Segment / (float)NumSegments * 2.0f * PI;
			float SinPhi = FMath::Sin(Phi);
			float CosPhi = FMath::Cos(Phi);

			FVector3f Position(
				Radius * SinTheta * CosPhi,
				Radius * SinTheta * SinPhi,
				Radius * CosTheta
			);

			FVector3f Normal = Position.GetSafeNormal();
			FVector3f Tangent = FVector3f(-SinPhi, CosPhi, 0.0f).GetSafeNormal();
			FVector2f TexCoord((float)Segment / (float)NumSegments, (float)Ring / (float)NumRings);

			// Color gradient based on height
			float ColorValue = (Position.Z + Radius) / (Radius * 2.0f);
			FColor VertexColor = FLinearColor(ColorValue, 0.5f, 1.0f - ColorValue).ToFColor(false);

			Builder.AddVertex(Position)
				.SetNormalAndTangent(Normal, Tangent)
				.SetTexCoord(TexCoord)
				.SetColor(VertexColor);
		}
	}

	// Create triangles
	for (int32 Ring = 0; Ring < NumRings; Ring++)
	{
		for (int32 Segment = 0; Segment < NumSegments; Segment++)
		{
			int32 Current = Ring * (NumSegments + 1) + Segment;
			int32 Next = Current + 1;
			int32 Below = (Ring + 1) * (NumSegments + 1) + Segment;
			int32 BelowNext = Below + 1;

			// Two triangles per quad
			Builder.AddTriangle(BelowNext, Below, Current);
			Builder.AddTriangle(Next, BelowNext, Current);
		}
	}

	const FRealtimeMeshSectionGroupKey GroupKey = FRealtimeMeshSectionGroupKey::Create(LODKey, FName("HighPolySphere"));

	RealtimeMesh->CreateSectionGroup(GroupKey, StreamSet);
}
