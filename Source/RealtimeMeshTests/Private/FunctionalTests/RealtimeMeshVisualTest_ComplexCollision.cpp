// Copyright (c) 2015-2025 TriAxis Games, L.L.C. All Rights Reserved.

#include "FunctionalTests/RealtimeMeshVisualTest_ComplexCollision.h"
#include "RealtimeMeshSimple.h"
#include "Core/RealtimeMeshBuilder.h"

using namespace RealtimeMesh;

ARealtimeMeshVisualTest_ComplexCollision::ARealtimeMeshVisualTest_ComplexCollision()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ARealtimeMeshVisualTest_ComplexCollision::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	URealtimeMeshSimple* RealtimeMesh = GetRealtimeMeshComponent()->InitializeRealtimeMesh<URealtimeMeshSimple>();

	// Setup material slot
	RealtimeMesh->SetupMaterialSlot(0, "DefaultMaterial");

	// Configure collision to use complex (per-triangle) collision
	FRealtimeMeshCollisionConfiguration CollisionConfig;
	CollisionConfig.bUseComplexAsSimpleCollision = true;  // Enable complex collision as simple
	CollisionConfig.bUseAsyncCook = true;
	RealtimeMesh->SetCollisionConfig(CollisionConfig);

	const FRealtimeMeshLODKey LODKey(0);

	// Create a stepped platform (pyramid-like structure) using TRealtimeMeshBuilderLocal
	// This will demonstrate complex collision following the exact surface
	{
		FRealtimeMeshStreamSet StreamSet;
		TRealtimeMeshBuilderLocal<uint16, FPackedNormal, FVector2DHalf, 1> Builder(StreamSet);

		Builder.EnableTangents();
		Builder.EnableTexCoords();
		Builder.EnableColors();

		const int32 NumSteps = 5;
		const float StepHeight = 20.0f;
		const float StepDepth = 40.0f;
		const float BaseWidth = 200.0f;
		const float StepWidthReduction = 30.0f;

		// Build the stepped platform from bottom to top
		for (int32 StepIndex = 0; StepIndex < NumSteps; ++StepIndex)
		{
			const float CurrentWidth = BaseWidth - (StepIndex * StepWidthReduction);
			const float CurrentHeight = StepIndex * StepHeight;
			const float CurrentDepth = StepIndex * StepDepth;

			const float NextWidth = BaseWidth - ((StepIndex + 1) * StepWidthReduction);
			const float NextHeight = (StepIndex + 1) * StepHeight;
			const float NextDepth = (StepIndex + 1) * StepDepth;

			// Calculate color gradient from red (bottom) to blue (top)
			const float ColorLerp = static_cast<float>(StepIndex) / static_cast<float>(NumSteps - 1);
			const FColor StepColor = FColor(
				static_cast<uint8>(255 * (1.0f - ColorLerp)),
				static_cast<uint8>(100),
				static_cast<uint8>(255 * ColorLerp),
				255
			);

			// Top surface of current step
			{
				const FVector3f Normal = FVector3f(0.0f, 0.0f, 1.0f);
				const FVector3f Tangent = FVector3f(1.0f, 0.0f, 0.0f);

				// Four corners of the top surface
				int32 V0 = Builder.AddVertex(FVector3f(-CurrentWidth / 2.0f, -CurrentWidth / 2.0f, CurrentHeight + StepHeight))
					.SetNormalAndTangent(Normal, Tangent)
					.SetColor(StepColor)
					.SetTexCoord(FVector2f(0.0f, 0.0f));

				int32 V1 = Builder.AddVertex(FVector3f(CurrentWidth / 2.0f, -CurrentWidth / 2.0f, CurrentHeight + StepHeight))
					.SetNormalAndTangent(Normal, Tangent)
					.SetColor(StepColor)
					.SetTexCoord(FVector2f(1.0f, 0.0f));

				int32 V2 = Builder.AddVertex(FVector3f(CurrentWidth / 2.0f, CurrentWidth / 2.0f, CurrentHeight + StepHeight))
					.SetNormalAndTangent(Normal, Tangent)
					.SetColor(StepColor)
					.SetTexCoord(FVector2f(1.0f, 1.0f));

				int32 V3 = Builder.AddVertex(FVector3f(-CurrentWidth / 2.0f, CurrentWidth / 2.0f, CurrentHeight + StepHeight))
					.SetNormalAndTangent(Normal, Tangent)
					.SetColor(StepColor)
					.SetTexCoord(FVector2f(0.0f, 1.0f));

				// Two triangles for top surface
				Builder.AddTriangle(V0, V1, V2);
				Builder.AddTriangle(V0, V2, V3);
			}

			// Front riser (vertical face)
			if (StepIndex < NumSteps - 1)
			{
				const FVector3f Normal = FVector3f(0.0f, 1.0f, 0.0f);
				const FVector3f Tangent = FVector3f(1.0f, 0.0f, 0.0f);

				// Four corners of the front riser
				int32 V0 = Builder.AddVertex(FVector3f(-NextWidth / 2.0f, NextWidth / 2.0f, CurrentHeight + StepHeight))
					.SetNormalAndTangent(Normal, Tangent)
					.SetColor(StepColor)
					.SetTexCoord(FVector2f(0.0f, 0.0f));

				int32 V1 = Builder.AddVertex(FVector3f(NextWidth / 2.0f, NextWidth / 2.0f, CurrentHeight + StepHeight))
					.SetNormalAndTangent(Normal, Tangent)
					.SetColor(StepColor)
					.SetTexCoord(FVector2f(1.0f, 0.0f));

				int32 V2 = Builder.AddVertex(FVector3f(NextWidth / 2.0f, NextWidth / 2.0f, NextHeight))
					.SetNormalAndTangent(Normal, Tangent)
					.SetColor(StepColor)
					.SetTexCoord(FVector2f(1.0f, 1.0f));

				int32 V3 = Builder.AddVertex(FVector3f(-NextWidth / 2.0f, NextWidth / 2.0f, NextHeight))
					.SetNormalAndTangent(Normal, Tangent)
					.SetColor(StepColor)
					.SetTexCoord(FVector2f(0.0f, 1.0f));

				// Two triangles for front riser
				Builder.AddTriangle(V0, V1, V2);
				Builder.AddTriangle(V0, V2, V3);
			}

			// Left side surface
			if (StepIndex < NumSteps - 1)
			{
				const FVector3f Normal = FVector3f(-1.0f, 0.0f, 0.0f);
				const FVector3f Tangent = FVector3f(0.0f, 1.0f, 0.0f);

				int32 V0 = Builder.AddVertex(FVector3f(-NextWidth / 2.0f, -NextWidth / 2.0f, CurrentHeight + StepHeight))
					.SetNormalAndTangent(Normal, Tangent)
					.SetColor(StepColor)
					.SetTexCoord(FVector2f(0.0f, 0.0f));

				int32 V1 = Builder.AddVertex(FVector3f(-NextWidth / 2.0f, NextWidth / 2.0f, CurrentHeight + StepHeight))
					.SetNormalAndTangent(Normal, Tangent)
					.SetColor(StepColor)
					.SetTexCoord(FVector2f(1.0f, 0.0f));

				int32 V2 = Builder.AddVertex(FVector3f(-NextWidth / 2.0f, NextWidth / 2.0f, NextHeight))
					.SetNormalAndTangent(Normal, Tangent)
					.SetColor(StepColor)
					.SetTexCoord(FVector2f(1.0f, 1.0f));

				int32 V3 = Builder.AddVertex(FVector3f(-NextWidth / 2.0f, -NextWidth / 2.0f, NextHeight))
					.SetNormalAndTangent(Normal, Tangent)
					.SetColor(StepColor)
					.SetTexCoord(FVector2f(0.0f, 1.0f));

				Builder.AddTriangle(V0, V1, V2);
				Builder.AddTriangle(V0, V2, V3);
			}

			// Right side surface
			if (StepIndex < NumSteps - 1)
			{
				const FVector3f Normal = FVector3f(1.0f, 0.0f, 0.0f);
				const FVector3f Tangent = FVector3f(0.0f, -1.0f, 0.0f);

				int32 V0 = Builder.AddVertex(FVector3f(NextWidth / 2.0f, NextWidth / 2.0f, CurrentHeight + StepHeight))
					.SetNormalAndTangent(Normal, Tangent)
					.SetColor(StepColor)
					.SetTexCoord(FVector2f(0.0f, 0.0f));

				int32 V1 = Builder.AddVertex(FVector3f(NextWidth / 2.0f, -NextWidth / 2.0f, CurrentHeight + StepHeight))
					.SetNormalAndTangent(Normal, Tangent)
					.SetColor(StepColor)
					.SetTexCoord(FVector2f(1.0f, 0.0f));

				int32 V2 = Builder.AddVertex(FVector3f(NextWidth / 2.0f, -NextWidth / 2.0f, NextHeight))
					.SetNormalAndTangent(Normal, Tangent)
					.SetColor(StepColor)
					.SetTexCoord(FVector2f(1.0f, 1.0f));

				int32 V3 = Builder.AddVertex(FVector3f(NextWidth / 2.0f, NextWidth / 2.0f, NextHeight))
					.SetNormalAndTangent(Normal, Tangent)
					.SetColor(StepColor)
					.SetTexCoord(FVector2f(0.0f, 1.0f));

				Builder.AddTriangle(V0, V1, V2);
				Builder.AddTriangle(V0, V2, V3);
			}

			// Back surface
			if (StepIndex < NumSteps - 1)
			{
				const FVector3f Normal = FVector3f(0.0f, -1.0f, 0.0f);
				const FVector3f Tangent = FVector3f(-1.0f, 0.0f, 0.0f);

				int32 V0 = Builder.AddVertex(FVector3f(NextWidth / 2.0f, -NextWidth / 2.0f, CurrentHeight + StepHeight))
					.SetNormalAndTangent(Normal, Tangent)
					.SetColor(StepColor)
					.SetTexCoord(FVector2f(0.0f, 0.0f));

				int32 V1 = Builder.AddVertex(FVector3f(-NextWidth / 2.0f, -NextWidth / 2.0f, CurrentHeight + StepHeight))
					.SetNormalAndTangent(Normal, Tangent)
					.SetColor(StepColor)
					.SetTexCoord(FVector2f(1.0f, 0.0f));

				int32 V2 = Builder.AddVertex(FVector3f(-NextWidth / 2.0f, -NextWidth / 2.0f, NextHeight))
					.SetNormalAndTangent(Normal, Tangent)
					.SetColor(StepColor)
					.SetTexCoord(FVector2f(1.0f, 1.0f));

				int32 V3 = Builder.AddVertex(FVector3f(NextWidth / 2.0f, -NextWidth / 2.0f, NextHeight))
					.SetNormalAndTangent(Normal, Tangent)
					.SetColor(StepColor)
					.SetTexCoord(FVector2f(0.0f, 1.0f));

				Builder.AddTriangle(V0, V1, V2);
				Builder.AddTriangle(V0, V2, V3);
			}
		}

		// Add bottom surface
		{
			const FColor BottomColor = FColor::Red;
			const FVector3f Normal = FVector3f(0.0f, 0.0f, -1.0f);
			const FVector3f Tangent = FVector3f(1.0f, 0.0f, 0.0f);

			int32 V0 = Builder.AddVertex(FVector3f(-BaseWidth / 2.0f, -BaseWidth / 2.0f, 0.0f))
				.SetNormalAndTangent(Normal, Tangent)
				.SetColor(BottomColor)
				.SetTexCoord(FVector2f(0.0f, 0.0f));

			int32 V1 = Builder.AddVertex(FVector3f(BaseWidth / 2.0f, -BaseWidth / 2.0f, 0.0f))
				.SetNormalAndTangent(Normal, Tangent)
				.SetColor(BottomColor)
				.SetTexCoord(FVector2f(1.0f, 0.0f));

			int32 V2 = Builder.AddVertex(FVector3f(BaseWidth / 2.0f, BaseWidth / 2.0f, 0.0f))
				.SetNormalAndTangent(Normal, Tangent)
				.SetColor(BottomColor)
				.SetTexCoord(FVector2f(1.0f, 1.0f));

			int32 V3 = Builder.AddVertex(FVector3f(-BaseWidth / 2.0f, BaseWidth / 2.0f, 0.0f))
				.SetNormalAndTangent(Normal, Tangent)
				.SetColor(BottomColor)
				.SetTexCoord(FVector2f(0.0f, 1.0f));

			// Reverse winding for bottom surface (facing down)
			Builder.AddTriangle(V0, V2, V1);
			Builder.AddTriangle(V0, V3, V2);
		}

		// Create the section group
		const FRealtimeMeshSectionGroupKey GroupKey = FRealtimeMeshSectionGroupKey::Create(LODKey, FName("SteppedPlatform"));

		// CreateSectionGroup automatically creates a section, so we just need to update its config
		RealtimeMesh->CreateSectionGroup(GroupKey, StreamSet);

		// Get the auto-created section key (default is polygroup 0 when no polygroups are used)
		const FRealtimeMeshSectionKey SectionKey = FRealtimeMeshSectionKey::CreateForPolyGroup(GroupKey, 0);

		// Update the section config with material slot 0
		RealtimeMesh->UpdateSectionConfig(SectionKey, FRealtimeMeshSectionConfig(0));
	}
}
