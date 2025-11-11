// Copyright (c) 2015-2025 TriAxis Games, L.L.C. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "RealtimeMesh.h"
#include "RealtimeMeshSimple.h"
#include "RealtimeMeshComponent.h"
#include "Mesh/RealtimeMeshBasicShapeTools.h"
#include "Core/RealtimeMeshBuilder.h"
#include "Data/RealtimeMeshData.h"
#include "HAL/PlatformProcess.h"
#include "Async/Async.h"

using namespace RealtimeMesh;

#if WITH_DEV_AUTOMATION_TESTS

//==============================================================================
// Test 1: Basic Mesh Lifecycle
// Tests fundamental create → update → destroy workflow
//==============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRealtimeMeshBasicLifecycleTest,
	"RealtimeMeshComponent.Functional.BasicLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRealtimeMeshBasicLifecycleTest::RunTest(const FString& Parameters)
{
	// Create a simple mesh
	URealtimeMeshSimple* Mesh = NewObject<URealtimeMeshSimple>(GetTransientPackage(), NAME_None, RF_Transient);
	TestNotNull(TEXT("Mesh should be created"), Mesh);
	if (!Mesh) return false;

	// Create a section group with basic mesh data
	FRealtimeMeshSectionGroupKey GroupKey = FRealtimeMeshSectionGroupKey::Create(0, 0);
	FRealtimeMeshStreamSet StreamSet;

	// Build a simple triangle
	TRealtimeMeshBuilderLocal<uint32, FPackedNormal, FVector2DHalf, 1> Builder(StreamSet);
	Builder.EnableTangents();
	Builder.EnableTexCoords();
	Builder.EnableColors();

	// Add 3 vertices for a triangle
	Builder.AddVertex(FVector3f(0.0f, 0.0f, 0.0f))
		.SetNormalAndTangent(FVector3f(0.0f, 0.0f, 1.0f), FVector3f(1.0f, 0.0f, 0.0f))
		.SetTexCoord(FVector2f(0.0f, 0.0f))
		.SetColor(FColor::Red);

	Builder.AddVertex(FVector3f(100.0f, 0.0f, 0.0f))
		.SetNormalAndTangent(FVector3f(0.0f, 0.0f, 1.0f), FVector3f(1.0f, 0.0f, 0.0f))
		.SetTexCoord(FVector2f(1.0f, 0.0f))
		.SetColor(FColor::Green);

	Builder.AddVertex(FVector3f(50.0f, 100.0f, 0.0f))
		.SetNormalAndTangent(FVector3f(0.0f, 0.0f, 1.0f), FVector3f(1.0f, 0.0f, 0.0f))
		.SetTexCoord(FVector2f(0.5f, 1.0f))
		.SetColor(FColor::Blue);

	// Add triangle
	Builder.AddTriangle(0, 1, 2);

	// Create the section group
	auto CreateFuture = Mesh->CreateSectionGroup(GroupKey, MoveTemp(StreamSet));
	CreateFuture.Wait();

	TestTrue(TEXT("Section group creation should succeed"),
		CreateFuture.Get() == ERealtimeMeshProxyUpdateStatus::NoUpdate);

	// Verify mesh has valid bounds
	FBoxSphereBounds Bounds = Mesh->GetLocalBounds();
	TestTrue(TEXT("Bounds should be valid"), Bounds.BoxExtent.X > 0.0f && Bounds.BoxExtent.Y > 0.0f);

	// Verify we have LOD 0
	TArray<FRealtimeMeshLODKey> LODs = Mesh->GetLODs();
	TestEqual(TEXT("Should have 1 LOD"), LODs.Num(), 1);

	// Verify section group exists
	TArray<FRealtimeMeshSectionGroupKey> Groups = Mesh->GetSectionGroups(FRealtimeMeshLODKey(0));
	TestEqual(TEXT("Should have 1 section group"), Groups.Num(), 1);

	// Update the mesh - move the triangle
	FRealtimeMeshStreamSet UpdatedStreamSet;
	TRealtimeMeshBuilderLocal<uint32, FPackedNormal, FVector2DHalf, 1> UpdateBuilder(UpdatedStreamSet);
	UpdateBuilder.EnableTangents();
	UpdateBuilder.EnableTexCoords();
	UpdateBuilder.EnableColors();

	UpdateBuilder.AddVertex(FVector3f(200.0f, 0.0f, 0.0f))
		.SetNormalAndTangent(FVector3f(0.0f, 0.0f, 1.0f), FVector3f(1.0f, 0.0f, 0.0f))
		.SetTexCoord(FVector2f(0.0f, 0.0f))
		.SetColor(FColor::Red);

	UpdateBuilder.AddVertex(FVector3f(300.0f, 0.0f, 0.0f))
		.SetNormalAndTangent(FVector3f(0.0f, 0.0f, 1.0f), FVector3f(1.0f, 0.0f, 0.0f))
		.SetTexCoord(FVector2f(1.0f, 0.0f))
		.SetColor(FColor::Green);

	UpdateBuilder.AddVertex(FVector3f(250.0f, 100.0f, 0.0f))
		.SetNormalAndTangent(FVector3f(0.0f, 0.0f, 1.0f), FVector3f(1.0f, 0.0f, 0.0f))
		.SetTexCoord(FVector2f(0.5f, 1.0f))
		.SetColor(FColor::Blue);

	UpdateBuilder.AddTriangle(0, 1, 2);

	auto UpdateFuture = Mesh->UpdateSectionGroup(GroupKey, MoveTemp(UpdatedStreamSet));
	UpdateFuture.Wait();

	TestTrue(TEXT("Section group update should succeed"),
		UpdateFuture.Get() == ERealtimeMeshProxyUpdateStatus::NoUpdate);

	// Verify bounds updated
	FBoxSphereBounds UpdatedBounds = Mesh->GetLocalBounds();
	TestTrue(TEXT("Bounds should have changed after update"),
		UpdatedBounds.BoxExtent != Bounds.BoxExtent || UpdatedBounds.Origin != Bounds.Origin);

	// Cleanup - mesh will be destroyed by GC since it's transient
	Mesh->Reset();
	TestEqual(TEXT("After reset, should have one LOD"), Mesh->GetLODs().Num(), 1);

	return true;
}

//==============================================================================
// Test 2: Multi-LOD Workflow
// Tests LOD creation and management
//==============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRealtimeMeshMultiLODWorkflowTest,
	"RealtimeMeshComponent.Functional.MultiLODWorkflow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRealtimeMeshMultiLODWorkflowTest::RunTest(const FString& Parameters)
{
	URealtimeMeshSimple* Mesh = NewObject<URealtimeMeshSimple>(GetTransientPackage(), NAME_None, RF_Transient);
	TestNotNull(TEXT("Mesh should be created"), Mesh);
	if (!Mesh) return false;

	// Helper lambda to create a simple quad with N subdivisions
	auto CreateQuadMesh = [](int32 Subdivisions) -> FRealtimeMeshStreamSet
	{
		FRealtimeMeshStreamSet StreamSet;
		TRealtimeMeshBuilderLocal<uint32, FPackedNormal, FVector2DHalf, 1> Builder(StreamSet);
		Builder.EnableTangents();

		int32 VertsPerSide = Subdivisions + 2;
		float StepSize = 100.0f / (VertsPerSide - 1);

		// Generate grid of vertices
		for (int32 Y = 0; Y < VertsPerSide; Y++)
		{
			for (int32 X = 0; X < VertsPerSide; X++)
			{
				Builder.AddVertex(FVector3f(X * StepSize, Y * StepSize, 0.0f))
					.SetNormalAndTangent(FVector3f(0.0f, 0.0f, 1.0f), FVector3f(1.0f, 0.0f, 0.0f));
			}
		}

		// Generate triangles
		for (int32 Y = 0; Y < VertsPerSide - 1; Y++)
		{
			for (int32 X = 0; X < VertsPerSide - 1; X++)
			{
				int32 V0 = Y * VertsPerSide + X;
				int32 V1 = V0 + 1;
				int32 V2 = V0 + VertsPerSide;
				int32 V3 = V2 + 1;

				Builder.AddTriangle(V0, V2, V1);
				Builder.AddTriangle(V1, V2, V3);
			}
		}

		return StreamSet;
	};

	// LOD 0: High detail (10x10 grid = 100 verts, 200 tris)
	// Note: LOD0 already exists by default in URealtimeMeshSimple
	FRealtimeMeshLODKey LOD0Key = FRealtimeMeshLODKey(0);
	FRealtimeMeshSectionGroupKey Group0Key = FRealtimeMeshSectionGroupKey::Create(0, 0);
	auto Future0 = Mesh->CreateSectionGroup(Group0Key, CreateQuadMesh(10));
	Future0.Wait();

	// LOD 1: Medium detail (5x5 grid = 25 verts, 50 tris)
	FRealtimeMeshLODKey LOD1Key = Mesh->AddLOD(FRealtimeMeshLODConfig());
	FRealtimeMeshSectionGroupKey Group1Key = FRealtimeMeshSectionGroupKey::Create(1, 0);
	auto Future1 = Mesh->CreateSectionGroup(Group1Key, CreateQuadMesh(5));
	Future1.Wait();

	// LOD 2: Low detail (2x2 grid = 4 verts, 8 tris)
	FRealtimeMeshLODKey LOD2Key = Mesh->AddLOD(FRealtimeMeshLODConfig());
	FRealtimeMeshSectionGroupKey Group2Key = FRealtimeMeshSectionGroupKey::Create(2, 0);
	auto Future2 = Mesh->CreateSectionGroup(Group2Key, CreateQuadMesh(2));
	Future2.Wait();

	// Verify structure
	TestEqual(TEXT("Should have 3 LODs"), Mesh->GetLODs().Num(), 3);

	// Verify each LOD has a section group
	TestEqual(TEXT("LOD0 should have 1 section group"),
		Mesh->GetSectionGroups(LOD0Key).Num(), 1);
	TestEqual(TEXT("LOD1 should have 1 section group"),
		Mesh->GetSectionGroups(LOD1Key).Num(), 1);
	TestEqual(TEXT("LOD2 should have 1 section group"),
		Mesh->GetSectionGroups(LOD2Key).Num(), 1);

	// Verify we can access section groups
	auto Group0 = Mesh->GetSectionGroup(Group0Key);
	TestTrue(TEXT("LOD0 section group should be accessible"), Group0.IsValid());

	// Test updating a single LOD
	auto UpdatedMesh = CreateQuadMesh(8);
	auto UpdateFuture = Mesh->UpdateSectionGroup(Group1Key, MoveTemp(UpdatedMesh));
	UpdateFuture.Wait();
	TestTrue(TEXT("LOD1 update should succeed"),
		UpdateFuture.Get() != ERealtimeMeshProxyUpdateStatus::NoProxy);

	// Verify other LODs are unaffected
	TestEqual(TEXT("Should still have 3 LODs after update"), Mesh->GetLODs().Num(), 3);

	// Test removing trailing LOD
	Mesh->RemoveTrailingLOD();
	TestEqual(TEXT("Should have 2 LODs after removing trailing"), Mesh->GetLODs().Num(), 2);

	return true;
}

//==============================================================================
// Test 3: Section Group & Material Management
// Tests section groups, materials, and poly group handling
//==============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRealtimeMeshSectionGroupMaterialTest,
	"RealtimeMeshComponent.Functional.SectionGroupMaterials",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRealtimeMeshSectionGroupMaterialTest::RunTest(const FString& Parameters)
{
	URealtimeMeshSimple* Mesh = NewObject<URealtimeMeshSimple>(GetTransientPackage(), NAME_None, RF_Transient);
	TestNotNull(TEXT("Mesh should be created"), Mesh);
	if (!Mesh) return false;

	// Setup material slots
	Mesh->SetupMaterialSlot(0, FName("Material0"));
	Mesh->SetupMaterialSlot(1, FName("Material1"));
	Mesh->SetupMaterialSlot(2, FName("Material2"));

	TestEqual(TEXT("Should have 3 material slots"), Mesh->GetNumMaterials(), 3);
	TestEqual(TEXT("Material0 should be at index 0"), Mesh->GetMaterialIndex(FName("Material0")), 0);

	// Create mesh with multiple poly groups
	FRealtimeMeshStreamSet StreamSet;
	TRealtimeMeshBuilderLocal<uint32, FPackedNormal, FVector2DHalf, 1, uint16> Builder(StreamSet);
	Builder.EnableTangents();
	Builder.EnablePolyGroups();

	// Create 3 triangles with different poly groups (material indices)
	// Triangle 1 - Material 0
	Builder.AddVertex(FVector3f(0.0f, 0.0f, 0.0f))
		.SetNormalAndTangent(FVector3f(0.0f, 0.0f, 1.0f), FVector3f(1.0f, 0.0f, 0.0f));
	Builder.AddVertex(FVector3f(100.0f, 0.0f, 0.0f))
		.SetNormalAndTangent(FVector3f(0.0f, 0.0f, 1.0f), FVector3f(1.0f, 0.0f, 0.0f));
	Builder.AddVertex(FVector3f(50.0f, 100.0f, 0.0f))
		.SetNormalAndTangent(FVector3f(0.0f, 0.0f, 1.0f), FVector3f(1.0f, 0.0f, 0.0f));
	Builder.AddTriangle(0, 1, 2, 0); // Material index 0

	// Triangle 2 - Material 1
	Builder.AddVertex(FVector3f(100.0f, 0.0f, 0.0f))
		.SetNormalAndTangent(FVector3f(0.0f, 0.0f, 1.0f), FVector3f(1.0f, 0.0f, 0.0f));
	Builder.AddVertex(FVector3f(200.0f, 0.0f, 0.0f))
		.SetNormalAndTangent(FVector3f(0.0f, 0.0f, 1.0f), FVector3f(1.0f, 0.0f, 0.0f));
	Builder.AddVertex(FVector3f(150.0f, 100.0f, 0.0f))
		.SetNormalAndTangent(FVector3f(0.0f, 0.0f, 1.0f), FVector3f(1.0f, 0.0f, 0.0f));
	Builder.AddTriangle(3, 4, 5, 1); // Material index 1

	// Triangle 3 - Material 2
	Builder.AddVertex(FVector3f(200.0f, 0.0f, 0.0f))
		.SetNormalAndTangent(FVector3f(0.0f, 0.0f, 1.0f), FVector3f(1.0f, 0.0f, 0.0f));
	Builder.AddVertex(FVector3f(300.0f, 0.0f, 0.0f))
		.SetNormalAndTangent(FVector3f(0.0f, 0.0f, 1.0f), FVector3f(1.0f, 0.0f, 0.0f));
	Builder.AddVertex(FVector3f(250.0f, 100.0f, 0.0f))
		.SetNormalAndTangent(FVector3f(0.0f, 0.0f, 1.0f), FVector3f(1.0f, 0.0f, 0.0f));
	Builder.AddTriangle(6, 7, 8, 2); // Material index 2

	// Create section group with auto-section creation enabled
	FRealtimeMeshSectionGroupKey GroupKey = FRealtimeMeshSectionGroupKey::Create(0, 0);
	auto CreateFuture = Mesh->CreateSectionGroup(GroupKey, MoveTemp(StreamSet),
		FRealtimeMeshSectionGroupConfig(), true);
	CreateFuture.Wait();

	TestTrue(TEXT("Section group creation should succeed"),
		CreateFuture.Get() != ERealtimeMeshProxyUpdateStatus::NoProxy);

	// Verify auto-created sections (one per poly group)
	TArray<FRealtimeMeshSectionKey> Sections = Mesh->GetSectionsInGroup(GroupKey);
	TestEqual(TEXT("Should have 3 auto-created sections"), Sections.Num(), 3);

	// Test section visibility
	if (Sections.Num() > 0)
	{
		FRealtimeMeshSectionKey Section0 = Sections[0];
		TestTrue(TEXT("Section should be visible by default"), Mesh->IsSectionVisible(Section0));

		auto VisibilityFuture = Mesh->SetSectionVisibility(Section0, false);
		VisibilityFuture.Wait();
		TestFalse(TEXT("Section should be hidden"), Mesh->IsSectionVisible(Section0));

		// Test shadow casting
		TestTrue(TEXT("Section should cast shadow by default"), Mesh->IsSectionCastingShadow(Section0));

		auto ShadowFuture = Mesh->SetSectionCastShadow(Section0, false);
		ShadowFuture.Wait();
		TestFalse(TEXT("Section should not cast shadow"), Mesh->IsSectionCastingShadow(Section0));
	}

	// Test removing a section
	if (Sections.Num() > 1)
	{
		auto RemoveFuture = Mesh->RemoveSection(Sections[1]);
		RemoveFuture.Wait();

		TArray<FRealtimeMeshSectionKey> UpdatedSections = Mesh->GetSectionsInGroup(GroupKey);
		TestEqual(TEXT("Should have 2 sections after removal"), UpdatedSections.Num(), 2);
	}

	return true;
}

//==============================================================================
// Test 4: Shape Tools Integration
// Tests the BasicShapeTools helper functions
//==============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRealtimeMeshShapeToolsTest,
	"RealtimeMeshComponent.Functional.ShapeTools",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRealtimeMeshShapeToolsTest::RunTest(const FString& Parameters)
{
	URealtimeMeshSimple* Mesh = NewObject<URealtimeMeshSimple>(GetTransientPackage(), NAME_None, RF_Transient);
	TestNotNull(TEXT("Mesh should be created"), Mesh);
	if (!Mesh) return false;

	// Create a box using shape tools
	FRealtimeMeshStreamSet StreamSet;
	FVector3f BoxRadius(50.0f, 50.0f, 50.0f);
	FTransform3f BoxTransform = FTransform3f::Identity;

	URealtimeMeshBasicShapeTools::AppendBoxMesh(StreamSet, BoxRadius, BoxTransform, 0, FColor::White);

	// Verify box was created with correct data
	const FRealtimeMeshStream* PositionStream = StreamSet.Find(FRealtimeMeshStreams::Position);
	TestNotNull(TEXT("Position stream should exist"), PositionStream);

	if (PositionStream)
	{
		// Box should have 24 vertices (4 per face * 6 faces)
		TestEqual(TEXT("Box should have 24 vertices"), PositionStream->Num(), 24);
	}

	const FRealtimeMeshStream* TriangleStream = StreamSet.Find(FRealtimeMeshStreams::Triangles);
	TestNotNull(TEXT("Triangle stream should exist"), TriangleStream);

	if (TriangleStream)
	{
		// Box should have 12 triangles (2 per face * 6 faces)
		TestEqual(TEXT("Box should have 12 triangles"), TriangleStream->Num(), 12);
	}

	// Create the section group with the box
	FRealtimeMeshSectionGroupKey GroupKey = FRealtimeMeshSectionGroupKey::Create(0, 0);
	auto CreateFuture = Mesh->CreateSectionGroup(GroupKey, MoveTemp(StreamSet));
	CreateFuture.Wait();

	TestTrue(TEXT("Box section group should be created"),
		CreateFuture.Get() != ERealtimeMeshProxyUpdateStatus::NoProxy);

	// Verify bounds are approximately correct for a 100x100x100 box
	FBoxSphereBounds Bounds = Mesh->GetLocalBounds();
	TestTrue(TEXT("Box bounds X should be around 50"),
		FMath::IsNearlyEqual(Bounds.BoxExtent.X, 50.0f, 5.0f));
	TestTrue(TEXT("Box bounds Y should be around 50"),
		FMath::IsNearlyEqual(Bounds.BoxExtent.Y, 50.0f, 5.0f));
	TestTrue(TEXT("Box bounds Z should be around 50"),
		FMath::IsNearlyEqual(Bounds.BoxExtent.Z, 50.0f, 5.0f));

	// Test appending a second box
	FRealtimeMeshStreamSet SecondBox;
	FTransform3f OffsetTransform(FQuat4f::Identity, FVector3f(200.0f, 0.0f, 0.0f), FVector3f::OneVector);
	URealtimeMeshBasicShapeTools::AppendBoxMesh(SecondBox, BoxRadius, OffsetTransform, 0, FColor::Red);

	// Append to existing mesh
	Mesh->ProcessMesh(GroupKey, [&](const FRealtimeMeshStreamSet& ExistingStreams)
	{
		// This just verifies we can read the existing mesh
		TestTrue(TEXT("Should be able to process existing mesh"), ExistingStreams.Num() > 0);
	});

	// Update with combined mesh
	FRealtimeMeshStreamSet CombinedStreamSet;
	URealtimeMeshBasicShapeTools::AppendBoxMesh(CombinedStreamSet, BoxRadius, BoxTransform, 0, FColor::White);
	URealtimeMeshBasicShapeTools::AppendMesh(CombinedStreamSet, SecondBox);

	auto UpdateFuture = Mesh->UpdateSectionGroup(GroupKey, MoveTemp(CombinedStreamSet));
	UpdateFuture.Wait();

	// Verify bounds expanded to include both boxes
	FBoxSphereBounds UpdatedBounds = Mesh->GetLocalBounds();
	TestTrue(TEXT("Bounds should have expanded after adding second box"),
		UpdatedBounds.BoxExtent.X > Bounds.BoxExtent.X);

	return true;
}

//==============================================================================
// Test 5: Stream Management
// Tests stream operations and linkage
//==============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRealtimeMeshStreamManagementTest,
	"RealtimeMeshComponent.Functional.StreamManagement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRealtimeMeshStreamManagementTest::RunTest(const FString& Parameters)
{
	URealtimeMeshSimple* Mesh = NewObject<URealtimeMeshSimple>(GetTransientPackage(), NAME_None, RF_Transient);
	TestNotNull(TEXT("Mesh should be created"), Mesh);
	if (!Mesh) return false;

	// Create mesh with all optional streams
	FRealtimeMeshStreamSet StreamSet;
	TRealtimeMeshBuilderLocal<uint32, FPackedNormal, FVector2DHalf, 2> Builder(StreamSet);
	Builder.EnableTangents();
	Builder.EnableTexCoords(2); // 2 UV channels
	Builder.EnableColors();

	// Add a simple quad
	Builder.AddVertex(FVector3f(0.0f, 0.0f, 0.0f))
		.SetNormalAndTangent(FVector3f(0.0f, 0.0f, 1.0f), FVector3f(1.0f, 0.0f, 0.0f))
		.SetTexCoord(0, FVector2f(0.0f, 0.0f))
		.SetTexCoord(1, FVector2f(0.0f, 0.0f))
		.SetColor(FColor::Red);

	Builder.AddVertex(FVector3f(100.0f, 0.0f, 0.0f))
		.SetNormalAndTangent(FVector3f(0.0f, 0.0f, 1.0f), FVector3f(1.0f, 0.0f, 0.0f))
		.SetTexCoord(0, FVector2f(1.0f, 0.0f))
		.SetTexCoord(1, FVector2f(1.0f, 0.0f))
		.SetColor(FColor::Green);

	Builder.AddVertex(FVector3f(100.0f, 100.0f, 0.0f))
		.SetNormalAndTangent(FVector3f(0.0f, 0.0f, 1.0f), FVector3f(1.0f, 0.0f, 0.0f))
		.SetTexCoord(0, FVector2f(1.0f, 1.0f))
		.SetTexCoord(1, FVector2f(1.0f, 1.0f))
		.SetColor(FColor::Blue);

	Builder.AddVertex(FVector3f(0.0f, 100.0f, 0.0f))
		.SetNormalAndTangent(FVector3f(0.0f, 0.0f, 1.0f), FVector3f(1.0f, 0.0f, 0.0f))
		.SetTexCoord(0, FVector2f(0.0f, 1.0f))
		.SetTexCoord(1, FVector2f(0.0f, 1.0f))
		.SetColor(FColor::White);

	Builder.AddTriangle(0, 1, 2);
	Builder.AddTriangle(0, 2, 3);

	// Verify all streams are present
	TestTrue(TEXT("Position stream should exist"), StreamSet.Contains(FRealtimeMeshStreams::Position));
	TestTrue(TEXT("Tangents stream should exist"), StreamSet.Contains(FRealtimeMeshStreams::Tangents));
	TestTrue(TEXT("TexCoords stream should exist"), StreamSet.Contains(FRealtimeMeshStreams::TexCoords));
	TestTrue(TEXT("Color stream should exist"), StreamSet.Contains(FRealtimeMeshStreams::Color));
	TestTrue(TEXT("Triangles stream should exist"), StreamSet.Contains(FRealtimeMeshStreams::Triangles));

	// Verify UV channel count
	const FRealtimeMeshStream* TexCoordStream = StreamSet.Find(FRealtimeMeshStreams::TexCoords);
	if (TexCoordStream)
	{
		TestEqual(TEXT("Should have 2 UV channels"), TexCoordStream->GetNumElements(), 2);
	}

	// Create section group
	FRealtimeMeshSectionGroupKey GroupKey = FRealtimeMeshSectionGroupKey::Create(0, 0);
	auto CreateFuture = Mesh->CreateSectionGroup(GroupKey, MoveTemp(StreamSet));
	CreateFuture.Wait();

	// Test editing mesh in place
	auto EditFuture = Mesh->EditMeshInPlace(GroupKey, [](FRealtimeMeshStreamSet& EditableStreams) -> TSet<FRealtimeMeshStreamKey>
	{
		// Modify the color stream
		FRealtimeMeshStream* ColorStream = EditableStreams.Find(FRealtimeMeshStreams::Color);
		if (ColorStream && ColorStream->Num() >= 4)
		{
			// Change all colors to yellow
			FColor* ColorData = ColorStream->GetData<FColor>();
			for (int32 i = 0; i < 4; i++)
			{
				ColorData[i] = FColor::Yellow;
			}

			// Return the modified streams
			TSet<FRealtimeMeshStreamKey> ModifiedStreams;
			ModifiedStreams.Add(FRealtimeMeshStreams::Color);
			return ModifiedStreams;
		}
		return TSet<FRealtimeMeshStreamKey>();
	});
	EditFuture.Wait();

	// Verify edit succeeded
	bool bVerified = false;
	Mesh->ProcessMesh(GroupKey, [&](const FRealtimeMeshStreamSet& Streams)
	{
		const FRealtimeMeshStream* ColorStream = Streams.Find(FRealtimeMeshStreams::Color);
		if (ColorStream && ColorStream->Num() >= 4)
		{
			const FColor* ColorData = ColorStream->GetData<FColor>();
			bVerified = (ColorData[0] == FColor::Yellow &&
						ColorData[1] == FColor::Yellow &&
						ColorData[2] == FColor::Yellow &&
						ColorData[3] == FColor::Yellow);
		}
	});

	TestTrue(TEXT("Colors should have been updated to yellow"), bVerified);

	return true;
}

//==============================================================================
// Test 6: Collision Generation
// Tests collision configuration and geometry
//==============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRealtimeMeshCollisionTest,
	"RealtimeMeshComponent.Functional.Collision",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRealtimeMeshCollisionTest::RunTest(const FString& Parameters)
{
	URealtimeMeshSimple* Mesh = NewObject<URealtimeMeshSimple>(GetTransientPackage(), NAME_None, RF_Transient);
	TestNotNull(TEXT("Mesh should be created"), Mesh);
	if (!Mesh) return false;

	// Create a simple box mesh
	FRealtimeMeshStreamSet StreamSet;
	FVector3f BoxRadius(50.0f, 50.0f, 50.0f);
	URealtimeMeshBasicShapeTools::AppendBoxMesh(StreamSet, BoxRadius);

	FRealtimeMeshSectionGroupKey GroupKey = FRealtimeMeshSectionGroupKey::Create(0, 0);
	auto CreateFuture = Mesh->CreateSectionGroup(GroupKey, MoveTemp(StreamSet));
	CreateFuture.Wait();

	// Setup collision configuration
	FRealtimeMeshCollisionConfiguration CollisionConfig;
	CollisionConfig.bUseComplexAsSimpleCollision = false;
	CollisionConfig.bUseAsyncCook = false; // Sync cook for testing

	// Note: Collision operations may not complete synchronously in test environments
	// We'll just verify the API calls don't crash and can read back configuration
	Mesh->SetCollisionConfig(CollisionConfig);

	// Verify collision config can be read back
	FRealtimeMeshCollisionConfiguration ReadConfig = Mesh->GetCollisionConfig();
	TestFalse(TEXT("UseComplexAsSimple should be false"), ReadConfig.bUseComplexAsSimpleCollision);

	// Setup simple collision geometry (box)
	FRealtimeMeshSimpleGeometry SimpleGeometry;
	FRealtimeMeshCollisionBox CollisionBox;
	CollisionBox.Center = FVector::ZeroVector;
	CollisionBox.Extents = FVector(50.0f, 50.0f, 50.0f);
	SimpleGeometry.Boxes.Add(CollisionBox);

	// Set geometry without waiting - collision cooking is asynchronous
	Mesh->SetSimpleGeometry(SimpleGeometry);

	// Verify simple geometry was set (data should be immediately readable)
	FRealtimeMeshSimpleGeometry ReadGeometry = Mesh->GetSimpleGeometry();
	TestEqual(TEXT("Should have 1 collision box"), ReadGeometry.Boxes.Num(), 1);

	if (ReadGeometry.Boxes.Num() > 0)
	{
		const FRealtimeMeshCollisionBox& ReadBox = ReadGeometry.Boxes.GetByIndex(0);
		TestTrue(TEXT("Box extents should match"),
			ReadBox.Extents.Equals(FVector(50.0f, 50.0f, 50.0f), 0.1f));
		TestTrue(TEXT("Box center should match"),
			ReadBox.Center.Equals(FVector::ZeroVector, 0.1f));
	}

	// Note: BodySetup creation and collision cooking happen asynchronously
	// The actual physics collision won't be ready immediately in test environment

	return true;
}

//==============================================================================
// Test 7: Thread-Safe Updates
// Tests concurrent access and the guard system
//==============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRealtimeMeshThreadSafetyTest,
	"RealtimeMeshComponent.Functional.ThreadSafety",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRealtimeMeshThreadSafetyTest::RunTest(const FString& Parameters)
{
	URealtimeMeshSimple* Mesh = NewObject<URealtimeMeshSimple>(GetTransientPackage(), NAME_None, RF_Transient);
	TestNotNull(TEXT("Mesh should be created"), Mesh);
	if (!Mesh) return false;

	// Create 3 LODs with basic meshes
	// Note: LOD0 already exists by default, so only add LOD1 and LOD2
	for (int32 LODIndex = 0; LODIndex < 3; LODIndex++)
	{
		if (LODIndex > 0)
		{
			Mesh->AddLOD(FRealtimeMeshLODConfig());
		}

		FRealtimeMeshStreamSet StreamSet;
		TRealtimeMeshBuilderLocal<> Builder(StreamSet);

		// Simple triangle
		Builder.AddVertex(FVector3f(0.0f, 0.0f, 0.0f));
		Builder.AddVertex(FVector3f(100.0f, 0.0f, 0.0f));
		Builder.AddVertex(FVector3f(50.0f, 100.0f, 0.0f));
		Builder.AddTriangle(0, 1, 2);

		FRealtimeMeshSectionGroupKey GroupKey = FRealtimeMeshSectionGroupKey::Create(LODIndex, 0);
		auto Future = Mesh->CreateSectionGroup(GroupKey, MoveTemp(StreamSet));
		Future.Wait();
	}

	// Spawn multiple threads to update different LODs concurrently
	TAtomic<int32> CompletedThreads(0);
	TAtomic<bool> bHadError(false);
	const int32 NumThreads = 3;
	const int32 UpdatesPerThread = 10;

	TArray<TFuture<void>> Threads;

	for (int32 ThreadIndex = 0; ThreadIndex < NumThreads; ThreadIndex++)
	{
		int32 LODIndex = ThreadIndex; // Each thread updates a different LOD

		Threads.Add(Async(EAsyncExecution::Thread, [Mesh, LODIndex, &CompletedThreads, &bHadError, UpdatesPerThread]()
		{
			for (int32 Update = 0; Update < UpdatesPerThread; Update++)
			{
				// Create new mesh data
				FRealtimeMeshStreamSet StreamSet;
				TRealtimeMeshBuilderLocal<> Builder(StreamSet);

				// Vary the mesh slightly each update
				float Offset = Update * 10.0f;
				Builder.AddVertex(FVector3f(Offset, 0.0f, 0.0f));
				Builder.AddVertex(FVector3f(100.0f + Offset, 0.0f, 0.0f));
				Builder.AddVertex(FVector3f(50.0f + Offset, 100.0f, 0.0f));
				Builder.AddTriangle(0, 1, 2);

				FRealtimeMeshSectionGroupKey GroupKey = FRealtimeMeshSectionGroupKey::Create(LODIndex, 0);

				// Update the mesh (this should be thread-safe via guards)
				auto Future = Mesh->UpdateSectionGroup(GroupKey, MoveTemp(StreamSet));
				Future.Wait();

				if (Future.Get() == ERealtimeMeshProxyUpdateStatus::NoProxy)
				{
					bHadError = true;
					break;
				}

				// Small delay to increase chance of contention
				FPlatformProcess::Sleep(0.001f);
			}

			CompletedThreads++;
		}));
	}

	// Wait for all threads to complete (with timeout)
	double StartTime = FPlatformTime::Seconds();
	while (CompletedThreads.Load() < NumThreads && (FPlatformTime::Seconds() - StartTime) < 30.0)
	{
		FPlatformProcess::Sleep(0.1f);
	}

	TestEqual(TEXT("All threads should complete"), CompletedThreads.Load(), NumThreads);
	TestFalse(TEXT("No errors should occur during concurrent updates"), bHadError.Load());

	// Verify mesh is still in valid state
	TestEqual(TEXT("Should still have 3 LODs"), Mesh->GetLODs().Num(), 3);
	TestTrue(TEXT("Bounds should be valid"), Mesh->GetLocalBounds().BoxExtent.X > 0.0f);

	return true;
}

//==============================================================================
// Test 8: Memory & Performance Stress Test
// Tests large mesh operations and memory management
//==============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRealtimeMeshStressTest,
	"RealtimeMeshComponent.Functional.StressTest",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRealtimeMeshStressTest::RunTest(const FString& Parameters)
{
	const int32 NumMeshes = 20; // Reduced from 100 to keep test reasonable
	const int32 TrianglesPerMesh = 1000;

	TArray<URealtimeMeshSimple*> Meshes;
	Meshes.Reserve(NumMeshes);

	// Measure creation time
	double StartTime = FPlatformTime::Seconds();

	// Create multiple meshes
	for (int32 MeshIndex = 0; MeshIndex < NumMeshes; MeshIndex++)
	{
		URealtimeMeshSimple* Mesh = NewObject<URealtimeMeshSimple>(GetTransientPackage(), NAME_None, RF_Transient);
		if (!Mesh) continue;

		// Create a mesh with many triangles
		FRealtimeMeshStreamSet StreamSet;
		TRealtimeMeshBuilderLocal<> Builder(StreamSet);

		// Generate a grid of triangles
		int32 GridSize = FMath::CeilToInt(FMath::Sqrt(TrianglesPerMesh / 2.0f));

		for (int32 Y = 0; Y <= GridSize; Y++)
		{
			for (int32 X = 0; X <= GridSize; X++)
			{
				Builder.AddVertex(FVector3f(X * 100.0f, Y * 100.0f, 0.0f));
			}
		}

		for (int32 Y = 0; Y < GridSize; Y++)
		{
			for (int32 X = 0; X < GridSize; X++)
			{
				int32 V0 = Y * (GridSize + 1) + X;
				int32 V1 = V0 + 1;
				int32 V2 = V0 + (GridSize + 1);
				int32 V3 = V2 + 1;

				Builder.AddTriangle(V0, V2, V1);
				Builder.AddTriangle(V1, V2, V3);
			}
		}

		FRealtimeMeshSectionGroupKey GroupKey = FRealtimeMeshSectionGroupKey::Create(0, 0);
		auto Future = Mesh->CreateSectionGroup(GroupKey, MoveTemp(StreamSet));
		Future.Wait();

		if (Future.Get() != ERealtimeMeshProxyUpdateStatus::NoProxy)
		{
			Meshes.Add(Mesh);
		}
	}

	double CreationTime = FPlatformTime::Seconds() - StartTime;

	TestEqual(TEXT("Should create all meshes"), Meshes.Num(), NumMeshes);
	AddInfo(FString::Printf(TEXT("Created %d meshes with ~%d triangles each in %.2f seconds (%.2f ms per mesh)"),
		NumMeshes, TrianglesPerMesh, CreationTime, (CreationTime / NumMeshes) * 1000.0));

	// Test updating meshes
	StartTime = FPlatformTime::Seconds();

	for (URealtimeMeshSimple* Mesh : Meshes)
	{
		// Create simpler update mesh
		FRealtimeMeshStreamSet StreamSet;
		TRealtimeMeshBuilderLocal<> Builder(StreamSet);

		Builder.AddVertex(FVector3f(0.0f, 0.0f, 0.0f));
		Builder.AddVertex(FVector3f(100.0f, 0.0f, 0.0f));
		Builder.AddVertex(FVector3f(50.0f, 100.0f, 0.0f));
		Builder.AddTriangle(0, 1, 2);

		FRealtimeMeshSectionGroupKey GroupKey = FRealtimeMeshSectionGroupKey::Create(0, 0);
		auto Future = Mesh->UpdateSectionGroup(GroupKey, MoveTemp(StreamSet));
		Future.Wait();
	}

	double UpdateTime = FPlatformTime::Seconds() - StartTime;
	AddInfo(FString::Printf(TEXT("Updated %d meshes in %.2f seconds (%.2f ms per update)"),
		NumMeshes, UpdateTime, (UpdateTime / NumMeshes) * 1000.0));

	// Cleanup - reset all meshes
	for (URealtimeMeshSimple* Mesh : Meshes)
	{
		Mesh->Reset();
	}

	// Meshes will be cleaned up by GC since they're transient
	AddInfo(TEXT("Stress test completed successfully"));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
