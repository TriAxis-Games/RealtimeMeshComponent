// Copyright (c) 2015-2025 TriAxis Games, L.L.C. All Rights Reserved.

#include "FunctionalTests/RealtimeMeshVisualTest_LOD.h"
#include "RealtimeMeshSimple.h"
#include "Mesh/RealtimeMeshBasicShapeTools.h"

using namespace RealtimeMesh;

ARealtimeMeshVisualTest_LOD::ARealtimeMeshVisualTest_LOD()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ARealtimeMeshVisualTest_LOD::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	URealtimeMeshSimple* RealtimeMesh = GetRealtimeMeshComponent()->InitializeRealtimeMesh<URealtimeMeshSimple>();

	// Setup material slot
	RealtimeMesh->SetupMaterialSlot(0, "DefaultMaterial");

	// Configure LOD screen sizes
	RealtimeMesh->UpdateLODConfig(0, FRealtimeMeshLODConfig(0.75f));  // LOD0 at 75% screen size

	// Create 3 LODs with different vertex colors
	FColor LODColors[3] = { FColor::White, FColor::Blue, FColor::Red };

	for (int32 LODIndex = 0; LODIndex < 3; LODIndex++)
	{
		// LOD0 already exists by default, only add LOD1 and LOD2
		if (LODIndex > 0)
		{
			RealtimeMesh->AddLOD(FRealtimeMeshLODConfig(FMath::Pow(0.5f, LODIndex)));
		}

		// Create a sphere for this LOD with progressively fewer segments
		int32 NumSegments = 32 >> LODIndex;  // 32, 16, 8
		int32 NumRings = 16 >> LODIndex;     // 16, 8, 4

		FRealtimeMeshStreamSet StreamSet;
		TRealtimeMeshBuilderLocal<uint16, FPackedNormal, FVector2DHalf, 1> Builder(StreamSet);
		Builder.EnableTangents();
		Builder.EnableTexCoords();
		Builder.EnableColors();

		// Generate sphere vertices
		const float Radius = 50.0f;
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

				FVector3f Position(Radius * SinTheta * CosPhi, Radius * SinTheta * SinPhi, Radius * CosTheta);
				FVector3f Normal = Position.GetSafeNormal();
				FVector3f Tangent = FVector3f(-SinPhi, CosPhi, 0.0f).GetSafeNormal();
				FVector2f TexCoord((float)Segment / (float)NumSegments, (float)Ring / (float)NumRings);

				Builder.AddVertex(Position)
					.SetNormalAndTangent(Normal, Tangent)
					.SetTexCoord(TexCoord)
					.SetColor(LODColors[LODIndex]);
			}
		}

		// Generate triangles
		for (int32 Ring = 0; Ring < NumRings; Ring++)
		{
			for (int32 Segment = 0; Segment < NumSegments; Segment++)
			{
				int32 Current = Ring * (NumSegments + 1) + Segment;
				int32 Next = Current + 1;
				int32 Below = (Ring + 1) * (NumSegments + 1) + Segment;
				int32 BelowNext = Below + 1;

				Builder.AddTriangle(BelowNext, Below, Current);
				Builder.AddTriangle(Next, BelowNext, Current);
			}
		}

		const FRealtimeMeshSectionGroupKey GroupKey = FRealtimeMeshSectionGroupKey::Create(LODIndex, FName("Sphere"));

		RealtimeMesh->CreateSectionGroup(GroupKey, StreamSet);
	}
}
