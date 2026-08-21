// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#include "Simple/RealtimeMeshExample_Simple_Collision.h"
#include "RealtimeMeshSimple.h"
#include "Mesh/RealtimeMeshBasicShapeTools.h"
#include "Core/RealtimeMeshCollision.h"

using namespace RealtimeMesh;

ARealtimeMeshExample_Simple_Collision::ARealtimeMeshExample_Simple_Collision()
{
	Description = NSLOCTEXT("RealtimeMeshExamples", "Simple_Collision",
		"URealtimeMeshSimple: simple (analytic box) vs complex (per-triangle) collision. Toggle CollisionMode.");
}

void ARealtimeMeshExample_Simple_Collision::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (CollisionMode == ERealtimeMeshExampleCollisionMode::Simple)
	{
		BuildSimpleCollision();
	}
	else
	{
		BuildComplexCollision();
	}

	VerifyMeshBuilt();
}

void ARealtimeMeshExample_Simple_Collision::BuildSimpleCollision()
{
	URealtimeMeshSimple* RealtimeMesh = GetRealtimeMeshComponent()->InitializeRealtimeMesh<URealtimeMeshSimple>();
	RealtimeMesh->SetupMaterialSlot(0, "FloorMaterial");

	const FRealtimeMeshLODKey LODKey(0);

	// A 500x500x10 floor box centered at the origin.
	{
		FRealtimeMeshStreamSet FloorStreamSet;
		URealtimeMeshBasicShapeTools::AppendBoxMesh(FloorStreamSet, FVector3f(250.0f, 250.0f, 5.0f), FTransform3f(FVector3f::ZeroVector), 0, FColor::Green);
		RealtimeMesh->CreateBufferSet(FRealtimeMeshBufferSetKey::Create(LODKey, FName("Floor")), FloorStreamSet);
	}

	// Give it a matching analytic box collision shape — much cheaper than per-triangle collision.
	// NOTE: AppendBoxMesh takes half-extents (radius), but FRealtimeMeshCollisionBox::Extents are
	// FULL dimensions (they map straight to FKBoxElem.X/Y/Z), so the collision extents are 2x the radius.
	{
		FRealtimeMeshSimpleGeometry SimpleGeometry;
		FRealtimeMeshCollisionBox CollisionBox;
		CollisionBox.Extents = FVector(500.0f, 500.0f, 10.0f);
		CollisionBox.Center = FVector::ZeroVector;
		CollisionBox.Rotation = FRotator::ZeroRotator;
		CollisionBox.bContributesToMass = true;
		SimpleGeometry.Boxes.Add(CollisionBox);
		RealtimeMesh->SetSimpleGeometry(SimpleGeometry);
	}

	// Use only the simple shapes (no per-triangle collision).
	{
		FRealtimeMeshCollisionConfiguration CollisionConfig;
		CollisionConfig.bUseComplexAsSimpleCollision = false;
		CollisionConfig.bUseAsyncCook = true;
		RealtimeMesh->SetCollisionConfig(CollisionConfig);
	}
}

void ARealtimeMeshExample_Simple_Collision::BuildComplexCollision()
{
	URealtimeMeshSimple* RealtimeMesh = GetRealtimeMeshComponent()->InitializeRealtimeMesh<URealtimeMeshSimple>();
	RealtimeMesh->SetupMaterialSlot(0, "DefaultMaterial");

	// Use the rendered triangles as collision (complex-as-simple).
	{
		FRealtimeMeshCollisionConfiguration CollisionConfig;
		CollisionConfig.bUseComplexAsSimpleCollision = true;
		CollisionConfig.bUseAsyncCook = true;
		RealtimeMesh->SetCollisionConfig(CollisionConfig);
	}

	const FRealtimeMeshLODKey LODKey(0);

	FRealtimeMeshStreamSet StreamSet;
	TRealtimeMeshBuilderLocal<uint16, FPackedNormal, FVector2DHalf, 1> Builder(StreamSet);
	Builder.EnableTangents();
	Builder.EnableTexCoords();
	Builder.EnableColors();

	const int32 NumSteps = 5;
	const float StepHeight = 20.0f;
	const float BaseWidth = 200.0f;
	const float StepWidthReduction = 30.0f;

	auto AddQuad = [&Builder](const FVector3f& P0, const FVector3f& P1, const FVector3f& P2, const FVector3f& P3, const FVector3f& Normal, const FVector3f& Tangent, const FColor& Color)
	{
		const int32 V0 = Builder.AddVertex(P0).SetNormalAndTangent(Normal, Tangent).SetColor(Color).SetTexCoord(FVector2f(0.0f, 0.0f));
		const int32 V1 = Builder.AddVertex(P1).SetNormalAndTangent(Normal, Tangent).SetColor(Color).SetTexCoord(FVector2f(1.0f, 0.0f));
		const int32 V2 = Builder.AddVertex(P2).SetNormalAndTangent(Normal, Tangent).SetColor(Color).SetTexCoord(FVector2f(1.0f, 1.0f));
		const int32 V3 = Builder.AddVertex(P3).SetNormalAndTangent(Normal, Tangent).SetColor(Color).SetTexCoord(FVector2f(0.0f, 1.0f));
		Builder.AddTriangle(V0, V1, V2);
		Builder.AddTriangle(V0, V2, V3);
	};

	for (int32 StepIndex = 0; StepIndex < NumSteps; ++StepIndex)
	{
		const float CurrentWidth = BaseWidth - (StepIndex * StepWidthReduction);
		const float CurrentHeight = StepIndex * StepHeight;
		const float NextWidth = BaseWidth - ((StepIndex + 1) * StepWidthReduction);
		const float NextHeight = (StepIndex + 1) * StepHeight;

		const float ColorLerp = static_cast<float>(StepIndex) / static_cast<float>(NumSteps - 1);
		const FColor StepColor(static_cast<uint8>(255 * (1.0f - ColorLerp)), 100, static_cast<uint8>(255 * ColorLerp), 255);

		const float Top = CurrentHeight + StepHeight;
		const float HC = CurrentWidth / 2.0f;
		const float HN = NextWidth / 2.0f;

		// Top surface of the current step.
		AddQuad(FVector3f(-HC, -HC, Top), FVector3f(HC, -HC, Top), FVector3f(HC, HC, Top), FVector3f(-HC, HC, Top),
			FVector3f(0, 0, 1), FVector3f(1, 0, 0), StepColor);

		if (StepIndex < NumSteps - 1)
		{
			// Front, left, right, back risers up to the next step.
			AddQuad(FVector3f(-HN, HN, Top), FVector3f(HN, HN, Top), FVector3f(HN, HN, NextHeight), FVector3f(-HN, HN, NextHeight),
				FVector3f(0, 1, 0), FVector3f(1, 0, 0), StepColor);
			AddQuad(FVector3f(-HN, -HN, Top), FVector3f(-HN, HN, Top), FVector3f(-HN, HN, NextHeight), FVector3f(-HN, -HN, NextHeight),
				FVector3f(-1, 0, 0), FVector3f(0, 1, 0), StepColor);
			AddQuad(FVector3f(HN, HN, Top), FVector3f(HN, -HN, Top), FVector3f(HN, -HN, NextHeight), FVector3f(HN, HN, NextHeight),
				FVector3f(1, 0, 0), FVector3f(0, -1, 0), StepColor);
			AddQuad(FVector3f(HN, -HN, Top), FVector3f(-HN, -HN, Top), FVector3f(-HN, -HN, NextHeight), FVector3f(HN, -HN, NextHeight),
				FVector3f(0, -1, 0), FVector3f(-1, 0, 0), StepColor);
		}
	}

	// Bottom face (reversed winding so it faces down).
	{
		const float HB = BaseWidth / 2.0f;
		const FColor BottomColor = FColor::Red;
		const FVector3f Normal(0, 0, -1);
		const FVector3f Tangent(1, 0, 0);
		const int32 V0 = Builder.AddVertex(FVector3f(-HB, -HB, 0.0f)).SetNormalAndTangent(Normal, Tangent).SetColor(BottomColor).SetTexCoord(FVector2f(0, 0));
		const int32 V1 = Builder.AddVertex(FVector3f(HB, -HB, 0.0f)).SetNormalAndTangent(Normal, Tangent).SetColor(BottomColor).SetTexCoord(FVector2f(1, 0));
		const int32 V2 = Builder.AddVertex(FVector3f(HB, HB, 0.0f)).SetNormalAndTangent(Normal, Tangent).SetColor(BottomColor).SetTexCoord(FVector2f(1, 1));
		const int32 V3 = Builder.AddVertex(FVector3f(-HB, HB, 0.0f)).SetNormalAndTangent(Normal, Tangent).SetColor(BottomColor).SetTexCoord(FVector2f(0, 1));
		Builder.AddTriangle(V0, V2, V1);
		Builder.AddTriangle(V0, V3, V2);
	}

	const FRealtimeMeshBufferSetKey GroupKey = FRealtimeMeshBufferSetKey::Create(LODKey, FName("SteppedPlatform"));
	RealtimeMesh->CreateBufferSet(GroupKey, StreamSet);
	RealtimeMesh->UpdateSectionConfig(FRealtimeMeshSectionKey::CreateForPolyGroup(GroupKey, 0), FRealtimeMeshSectionConfig(0));
}
