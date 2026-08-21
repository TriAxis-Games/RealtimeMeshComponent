// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#include "Simple/RealtimeMeshExample_Simple_HighPoly.h"
#include "RealtimeMeshSimple.h"

using namespace RealtimeMesh;

ARealtimeMeshExample_Simple_HighPoly::ARealtimeMeshExample_Simple_HighPoly()
{
	Description = NSLOCTEXT("RealtimeMeshExamples", "Simple_HighPoly",
		"URealtimeMeshSimple: a high-poly sphere in one section (32-bit indices). Raise SubdivisionLevel to stress-test.");
}

void ARealtimeMeshExample_Simple_HighPoly::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	URealtimeMeshSimple* RealtimeMesh = GetRealtimeMeshComponent()->InitializeRealtimeMesh<URealtimeMeshSimple>();
	RealtimeMesh->SetupMaterialSlot(0, "DefaultMaterial");

	const FRealtimeMeshLODKey LODKey(0);

	int32 NumSegments = 16 * (1 << SubdivisionLevel);
	int32 NumRings = 8 * (1 << SubdivisionLevel);
	NumSegments = FMath::Clamp(NumSegments, 16, 512);
	NumRings = FMath::Clamp(NumRings, 8, 256);

	FRealtimeMeshStreamSet StreamSet;
	// 32-bit index builder: a subdivided sphere easily exceeds the 16-bit vertex limit.
	TRealtimeMeshBuilderLocal<uint32, FPackedNormal, FVector2DHalf, 1> Builder(StreamSet);
	Builder.EnableTangents();
	Builder.EnableTexCoords();
	Builder.EnableColors();

	const float Radius = 50.0f;

	for (int32 Ring = 0; Ring <= NumRings; Ring++)
	{
		const float Theta = (float)Ring / (float)NumRings * PI;
		const float SinTheta = FMath::Sin(Theta);
		const float CosTheta = FMath::Cos(Theta);

		for (int32 Segment = 0; Segment <= NumSegments; Segment++)
		{
			const float Phi = (float)Segment / (float)NumSegments * 2.0f * PI;
			const float SinPhi = FMath::Sin(Phi);
			const float CosPhi = FMath::Cos(Phi);

			const FVector3f Position(Radius * SinTheta * CosPhi, Radius * SinTheta * SinPhi, Radius * CosTheta);
			const float ColorValue = (Position.Z + Radius) / (Radius * 2.0f);

			Builder.AddVertex(Position)
				.SetNormalAndTangent(Position.GetSafeNormal(), FVector3f(-SinPhi, CosPhi, 0.0f).GetSafeNormal())
				.SetTexCoord(FVector2f((float)Segment / (float)NumSegments, (float)Ring / (float)NumRings))
				.SetColor(FLinearColor(ColorValue, 0.5f, 1.0f - ColorValue).ToFColor(false));
		}
	}

	for (int32 Ring = 0; Ring < NumRings; Ring++)
	{
		for (int32 Segment = 0; Segment < NumSegments; Segment++)
		{
			const int32 Current = Ring * (NumSegments + 1) + Segment;
			const int32 Next = Current + 1;
			const int32 Below = (Ring + 1) * (NumSegments + 1) + Segment;
			const int32 BelowNext = Below + 1;

			Builder.AddTriangle(BelowNext, Below, Current);
			Builder.AddTriangle(Next, BelowNext, Current);
		}
	}

	RealtimeMesh->CreateBufferSet(FRealtimeMeshBufferSetKey::Create(LODKey, FName("HighPolySphere")), StreamSet);

	VerifyMeshBuilt();
}
