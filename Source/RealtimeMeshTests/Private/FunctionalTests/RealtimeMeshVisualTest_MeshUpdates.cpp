// Copyright (c) 2015-2025 TriAxis Games, L.L.C. All Rights Reserved.

#include "FunctionalTests/RealtimeMeshVisualTest_MeshUpdates.h"
#include "RealtimeMeshSimple.h"
#include "Mesh/RealtimeMeshBasicShapeTools.h"

using namespace RealtimeMesh;

ARealtimeMeshVisualTest_MeshUpdates::ARealtimeMeshVisualTest_MeshUpdates()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	
	MainGroupKey = FRealtimeMeshSectionGroupKey::Create(0, FName("MainMesh"));
	MainSectionKey = FRealtimeMeshSectionKey::CreateForPolyGroup(MainGroupKey, 0);

	SecondaryGroupKey = FRealtimeMeshSectionGroupKey::Create(0, FName("SecondaryMesh"));
	SecondarySectionKey = FRealtimeMeshSectionKey::CreateForPolyGroup(SecondaryGroupKey, 0);
}

void ARealtimeMeshVisualTest_MeshUpdates::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	URealtimeMeshSimple* RealtimeMesh = GetRealtimeMeshComponent()->InitializeRealtimeMesh<URealtimeMeshSimple>();

	// Setup material slots
	RealtimeMesh->SetupMaterialSlot(0, "Material1");
	RealtimeMesh->SetupMaterialSlot(1, "Material2");

	// Create initial mesh
	UpdateMesh(0.0f);
}

void ARealtimeMeshVisualTest_MeshUpdates::BeginPlay()
{
	Super::BeginPlay();
	AccumulatedTime = 0.0f;
}

void ARealtimeMeshVisualTest_MeshUpdates::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bEnableAnimation)
	{
		AccumulatedTime += DeltaTime * AnimationSpeed;

		// Cycle through scenarios
		int32 ScenarioIndex = static_cast<int32>(AccumulatedTime / ScenarioDuration) % 4;
		CurrentScenario = static_cast<EUpdateScenario>(ScenarioIndex);

		UpdateMesh(AccumulatedTime);
	}
}

void ARealtimeMeshVisualTest_MeshUpdates::UpdateMesh(float Time)
{
	switch (CurrentScenario)
	{
	case EUpdateScenario::FullMeshReplacement:
		CreateFullMeshReplacement(Time);
		break;
	case EUpdateScenario::PartialVertexUpdate:
		CreatePartialVertexUpdate(Time);
		break;
	case EUpdateScenario::SectionVisibility:
		CreateSectionVisibilityTest(Time);
		break;
	case EUpdateScenario::MaterialSlotSwap:
		CreateMaterialSlotSwapTest(Time);
		break;
	}
}

void ARealtimeMeshVisualTest_MeshUpdates::CreateFullMeshReplacement(float Time)
{
	// Cycle between box, sphere, and cylinder shapes
	float LocalTime = FMath::Fmod(Time, ScenarioDuration);
	float ShapeTime = LocalTime / ScenarioDuration;

	if (ShapeTime < 0.33f)
	{
		CreateBox(Time);
	}
	else if (ShapeTime < 0.66f)
	{
		CreateSphere(Time);
	}
	else
	{
		CreateCylinder(Time);
	}
}

void ARealtimeMeshVisualTest_MeshUpdates::CreatePartialVertexUpdate(float Time)
{
	URealtimeMeshSimple* RealtimeMesh = Cast<URealtimeMeshSimple>(GetRealtimeMeshComponent()->GetRealtimeMesh());
	if (!RealtimeMesh)
	{
		return;
	}

	// Create a deforming grid mesh with animated vertices
	FRealtimeMeshStreamSet StreamSet;
	TRealtimeMeshBuilderLocal<uint16, FPackedNormal, FVector2DHalf, 1> Builder(StreamSet);
	Builder.EnableTangents();
	Builder.EnableTexCoords();
	Builder.EnableColors();

	const int32 GridSize = 10;
	const float Spacing = 20.0f;
	const float WaveHeight = 30.0f;

	// Generate animated grid
	for (int32 Y = 0; Y <= GridSize; Y++)
	{
		for (int32 X = 0; X <= GridSize; X++)
		{
			float XPos = (X - GridSize * 0.5f) * Spacing;
			float YPos = (Y - GridSize * 0.5f) * Spacing;

			// Create ripple effect from center
			float Distance = FMath::Sqrt(XPos * XPos + YPos * YPos);
			float ZPos = FMath::Sin(Distance * 0.05f - Time * 3.0f) * WaveHeight;

			FVector3f Position(XPos, YPos, ZPos);
			FVector2f TexCoord(X / static_cast<float>(GridSize), Y / static_cast<float>(GridSize));

			// Color gradient based on height
			float ColorValue = (ZPos + WaveHeight) / (WaveHeight * 2.0f);
			FColor VertexColor = FLinearColor(ColorValue, 0.3f, 1.0f - ColorValue, 1.0f).ToFColor(false);

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

			Builder.AddTriangle(BottomLeft, TopLeft, TopRight);
			Builder.AddTriangle(BottomLeft, TopRight, BottomRight);
		}
	}

	// Update or create the section group
	TArray<FRealtimeMeshSectionGroupKey> ExistingGroups = RealtimeMesh->GetSectionGroups(FRealtimeMeshLODKey(0));
	if (ExistingGroups.Contains(MainGroupKey))
	{
		RealtimeMesh->UpdateSectionGroup(MainGroupKey, StreamSet);
	}
	else
	{
		RealtimeMesh->CreateSectionGroup(MainGroupKey, StreamSet);
		RealtimeMesh->UpdateSectionConfig(MainSectionKey, FRealtimeMeshSectionConfig(0));
	}
}

void ARealtimeMeshVisualTest_MeshUpdates::CreateSectionVisibilityTest(float Time)
{
	URealtimeMeshSimple* RealtimeMesh = Cast<URealtimeMeshSimple>(GetRealtimeMeshComponent()->GetRealtimeMesh());
	if (!RealtimeMesh)
	{
		return;
	}

	// Create two boxes side by side
	FRealtimeMeshStreamSet MainStreamSet;
	URealtimeMeshBasicShapeTools::AppendBoxMesh(
		MainStreamSet,
		FVector3f(50.0f, 50.0f, 50.0f),
		FTransform3f(FVector3f(-80.0f, 0.0f, 50.0f)),
		0,
		FColor::Red
	);

	FRealtimeMeshStreamSet SecondaryStreamSet;
	URealtimeMeshBasicShapeTools::AppendBoxMesh(
		SecondaryStreamSet,
		FVector3f(50.0f, 50.0f, 50.0f),
		FTransform3f(FVector3f(80.0f, 0.0f, 50.0f)),
		0,
		FColor::Blue
	);

	// Create or update section groups
	TArray<FRealtimeMeshSectionGroupKey> ExistingGroups = RealtimeMesh->GetSectionGroups(FRealtimeMeshLODKey(0));

	if (ExistingGroups.Contains(MainGroupKey))
	{
		RealtimeMesh->UpdateSectionGroup(MainGroupKey, MainStreamSet);
	}
	else
	{
		RealtimeMesh->CreateSectionGroup(MainGroupKey, MainStreamSet);
		RealtimeMesh->UpdateSectionConfig(MainSectionKey, FRealtimeMeshSectionConfig(0));
	}

	if (ExistingGroups.Contains(SecondaryGroupKey))
	{
		RealtimeMesh->UpdateSectionGroup(SecondaryGroupKey, SecondaryStreamSet);
	}
	else
	{
		RealtimeMesh->CreateSectionGroup(SecondaryGroupKey, SecondaryStreamSet);
		RealtimeMesh->UpdateSectionConfig(SecondarySectionKey, FRealtimeMeshSectionConfig(0));
	}

	// Toggle visibility based on time
	float LocalTime = FMath::Fmod(Time, ScenarioDuration);
	float VisibilityTime = LocalTime / ScenarioDuration;

	bool bMainVisible = VisibilityTime < 0.5f;
	bool bSecondaryVisible = VisibilityTime >= 0.25f && VisibilityTime < 0.75f;

	RealtimeMesh->SetSectionVisibility(MainSectionKey, bMainVisible);
	RealtimeMesh->SetSectionVisibility(SecondarySectionKey, bSecondaryVisible);
}

void ARealtimeMeshVisualTest_MeshUpdates::CreateMaterialSlotSwapTest(float Time)
{
	URealtimeMeshSimple* RealtimeMesh = Cast<URealtimeMeshSimple>(GetRealtimeMeshComponent()->GetRealtimeMesh());
	if (!RealtimeMesh)
	{
		return;
	}

	// Create two adjacent boxes
	FRealtimeMeshStreamSet MainStreamSet;
	URealtimeMeshBasicShapeTools::AppendBoxMesh(
		MainStreamSet,
		FVector3f(50.0f, 50.0f, 50.0f),
		FTransform3f(FVector3f(-80.0f, 0.0f, 50.0f)),
		0,
		FColor::Green
	);

	FRealtimeMeshStreamSet SecondaryStreamSet;
	URealtimeMeshBasicShapeTools::AppendBoxMesh(
		SecondaryStreamSet,
		FVector3f(50.0f, 50.0f, 50.0f),
		FTransform3f(FVector3f(80.0f, 0.0f, 50.0f)),
		0,
		FColor::Yellow
	);

	// Create or update section groups
	TArray<FRealtimeMeshSectionGroupKey> ExistingGroups = RealtimeMesh->GetSectionGroups(FRealtimeMeshLODKey(0));

	if (ExistingGroups.Contains(MainGroupKey))
	{
		RealtimeMesh->UpdateSectionGroup(MainGroupKey, MainStreamSet);
	}
	else
	{
		RealtimeMesh->CreateSectionGroup(MainGroupKey, MainStreamSet);
		RealtimeMesh->UpdateSectionConfig(MainSectionKey, FRealtimeMeshSectionConfig(0));
	}

	if (ExistingGroups.Contains(SecondaryGroupKey))
	{
		RealtimeMesh->UpdateSectionGroup(SecondaryGroupKey, SecondaryStreamSet);
	}
	else
	{
		RealtimeMesh->CreateSectionGroup(SecondaryGroupKey, SecondaryStreamSet);
		RealtimeMesh->UpdateSectionConfig(SecondarySectionKey, FRealtimeMeshSectionConfig(0));
	}

	// Swap material slots based on time
	float LocalTime = FMath::Fmod(Time, ScenarioDuration);
	float SwapTime = LocalTime / ScenarioDuration;

	int32 MainMaterialSlot = SwapTime < 0.5f ? 0 : 1;
	int32 SecondaryMaterialSlot = SwapTime < 0.5f ? 1 : 0;

	RealtimeMesh->UpdateSectionConfig(MainSectionKey, FRealtimeMeshSectionConfig(MainMaterialSlot));
	RealtimeMesh->UpdateSectionConfig(SecondarySectionKey, FRealtimeMeshSectionConfig(SecondaryMaterialSlot));
}

void ARealtimeMeshVisualTest_MeshUpdates::CreateBox(float Time)
{
	URealtimeMeshSimple* RealtimeMesh = Cast<URealtimeMeshSimple>(GetRealtimeMeshComponent()->GetRealtimeMesh());
	if (!RealtimeMesh)
	{
		return;
	}

	// Create an animated rotating box
	float Rotation = Time * 50.0f;
	FQuat4f RotationQuat = FQuat4f(FRotator3f(0.0f, Rotation, 0.0f));
	FTransform3f BoxTransform(RotationQuat, FVector3f(0.0f, 0.0f, 50.0f), FVector3f(1.0f));

	FRealtimeMeshStreamSet StreamSet;
	URealtimeMeshBasicShapeTools::AppendBoxMesh(
		StreamSet,
		FVector3f(50.0f, 50.0f, 50.0f),
		BoxTransform,
		0,
		FColor::Red
	);

	TArray<FRealtimeMeshSectionGroupKey> ExistingGroups = RealtimeMesh->GetSectionGroups(FRealtimeMeshLODKey(0));
	if (ExistingGroups.Contains(MainGroupKey))
	{
		RealtimeMesh->UpdateSectionGroup(MainGroupKey, StreamSet);
	}
	else
	{
		RealtimeMesh->CreateSectionGroup(MainGroupKey, StreamSet);
		RealtimeMesh->UpdateSectionConfig(MainSectionKey, FRealtimeMeshSectionConfig(0));
	}
}

void ARealtimeMeshVisualTest_MeshUpdates::CreateSphere(float Time)
{
	URealtimeMeshSimple* RealtimeMesh = Cast<URealtimeMeshSimple>(GetRealtimeMeshComponent()->GetRealtimeMesh());
	if (!RealtimeMesh)
	{
		return;
	}

	// Create an animated sphere with varying radius
	FRealtimeMeshStreamSet StreamSet;
	TRealtimeMeshBuilderLocal<uint16, FPackedNormal, FVector2DHalf, 1> Builder(StreamSet);
	Builder.EnableTangents();
	Builder.EnableTexCoords();
	Builder.EnableColors();

	const int32 Segments = 16;
	const int32 Rings = 12;
	const float BaseRadius = 50.0f;
	const float PulseAmount = 10.0f;
	float Radius = BaseRadius + FMath::Sin(Time * 2.0f) * PulseAmount;

	// Generate sphere vertices
	for (int32 Ring = 0; Ring <= Rings; Ring++)
	{
		float Phi = static_cast<float>(Ring) / Rings * PI;
		float SinPhi = FMath::Sin(Phi);
		float CosPhi = FMath::Cos(Phi);

		for (int32 Segment = 0; Segment <= Segments; Segment++)
		{
			float Theta = static_cast<float>(Segment) / Segments * 2.0f * PI;
			float SinTheta = FMath::Sin(Theta);
			float CosTheta = FMath::Cos(Theta);

			FVector3f Position(
				Radius * SinPhi * CosTheta,
				Radius * SinPhi * SinTheta,
				Radius * CosPhi + 50.0f
			);

			FVector3f Normal(SinPhi * CosTheta, SinPhi * SinTheta, CosPhi);
			FVector3f Tangent(-SinTheta, CosTheta, 0.0f);
			FVector2f TexCoord(static_cast<float>(Segment) / Segments, static_cast<float>(Ring) / Rings);

			float ColorValue = static_cast<float>(Ring) / Rings;
			FColor VertexColor = FLinearColor(0.0f, ColorValue, 1.0f - ColorValue, 1.0f).ToFColor(false);

			Builder.AddVertex(Position)
				.SetNormalAndTangent(Normal, Tangent)
				.SetTexCoord(TexCoord)
				.SetColor(VertexColor);
		}
	}

	// Generate sphere triangles
	for (int32 Ring = 0; Ring < Rings; Ring++)
	{
		for (int32 Segment = 0; Segment < Segments; Segment++)
		{
			int32 Current = Ring * (Segments + 1) + Segment;
			int32 Next = Current + (Segments + 1);

			Builder.AddTriangle(Current, Next, Current + 1);
			Builder.AddTriangle(Next, Next + 1, Current + 1);
		}
	}

	TArray<FRealtimeMeshSectionGroupKey> ExistingGroups = RealtimeMesh->GetSectionGroups(FRealtimeMeshLODKey(0));
	if (ExistingGroups.Contains(MainGroupKey))
	{
		RealtimeMesh->UpdateSectionGroup(MainGroupKey, StreamSet);
	}
	else
	{
		RealtimeMesh->CreateSectionGroup(MainGroupKey, StreamSet);
		RealtimeMesh->UpdateSectionConfig(MainSectionKey, FRealtimeMeshSectionConfig(0));
	}
}

void ARealtimeMeshVisualTest_MeshUpdates::CreateCylinder(float Time)
{
	URealtimeMeshSimple* RealtimeMesh = Cast<URealtimeMeshSimple>(GetRealtimeMeshComponent()->GetRealtimeMesh());
	if (!RealtimeMesh)
	{
		return;
	}

	// Create an animated stretching cylinder
	FRealtimeMeshStreamSet StreamSet;
	TRealtimeMeshBuilderLocal<uint16, FPackedNormal, FVector2DHalf, 1> Builder(StreamSet);
	Builder.EnableTangents();
	Builder.EnableTexCoords();
	Builder.EnableColors();

	const int32 Segments = 20;
	const float Radius = 40.0f;
	const float BaseHeight = 60.0f;
	const float StretchAmount = 30.0f;
	float Height = BaseHeight + FMath::Sin(Time * 2.5f) * StretchAmount;

	// Generate cylinder vertices
	for (int32 Z = 0; Z <= 1; Z++)
	{
		float ZPos = Z * Height;

		for (int32 Segment = 0; Segment <= Segments; Segment++)
		{
			float Theta = static_cast<float>(Segment) / Segments * 2.0f * PI;
			float SinTheta = FMath::Sin(Theta);
			float CosTheta = FMath::Cos(Theta);

			FVector3f Position(Radius * CosTheta, Radius * SinTheta, ZPos);
			FVector3f Normal(CosTheta, SinTheta, 0.0f);
			FVector3f Tangent(-SinTheta, CosTheta, 0.0f);
			FVector2f TexCoord(static_cast<float>(Segment) / Segments, static_cast<float>(Z));

			float ColorValue = static_cast<float>(Z);
			FColor VertexColor = FLinearColor(ColorValue, 1.0f - ColorValue, 0.5f, 1.0f).ToFColor(false);

			Builder.AddVertex(Position)
				.SetNormalAndTangent(Normal, Tangent)
				.SetTexCoord(TexCoord)
				.SetColor(VertexColor);
		}
	}

	// Generate cylinder triangles
	for (int32 Segment = 0; Segment < Segments; Segment++)
	{
		int32 BottomLeft = Segment;
		int32 BottomRight = Segment + 1;
		int32 TopLeft = Segment + (Segments + 1);
		int32 TopRight = TopLeft + 1;

		Builder.AddTriangle(BottomLeft, TopLeft, TopRight);
		Builder.AddTriangle(BottomLeft, TopRight, BottomRight);
	}

	// Add top and bottom caps
	int32 BottomCenterIndex = Builder.AddVertex(FVector3f(0.0f, 0.0f, 0.0f))
		.SetNormalAndTangent(FVector3f(0.0f, 0.0f, -1.0f), FVector3f(1.0f, 0.0f, 0.0f))
		.SetTexCoord(FVector2f(0.5f, 0.5f))
		.SetColor(FColor::Magenta);

	int32 TopCenterIndex = Builder.AddVertex(FVector3f(0.0f, 0.0f, Height))
		.SetNormalAndTangent(FVector3f(0.0f, 0.0f, 1.0f), FVector3f(1.0f, 0.0f, 0.0f))
		.SetTexCoord(FVector2f(0.5f, 0.5f))
		.SetColor(FColor::Cyan);

	for (int32 Segment = 0; Segment < Segments; Segment++)
	{
		// Bottom cap
		Builder.AddTriangle(BottomCenterIndex, Segment + 1, Segment);

		// Top cap
		int32 TopLeft = Segment + (Segments + 1);
		int32 TopRight = TopLeft + 1;
		Builder.AddTriangle(TopCenterIndex, TopLeft, TopRight);
	}

	TArray<FRealtimeMeshSectionGroupKey> ExistingGroups = RealtimeMesh->GetSectionGroups(FRealtimeMeshLODKey(0));
	if (ExistingGroups.Contains(MainGroupKey))
	{
		RealtimeMesh->UpdateSectionGroup(MainGroupKey, StreamSet);
	}
	else
	{
		RealtimeMesh->CreateSectionGroup(MainGroupKey, StreamSet);
		RealtimeMesh->UpdateSectionConfig(MainSectionKey, FRealtimeMeshSectionConfig(0));
	}
}
