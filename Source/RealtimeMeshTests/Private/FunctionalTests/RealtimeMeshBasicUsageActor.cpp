// Fill out your copyright notice in the Description page of Project Settings.


#include "FunctionalTests/RealtimeMeshBasicUsageActor.h"

#include "RealtimeMeshLibrary.h"
#include "RealtimeMeshSimple.h"
#include "Mesh/RealtimeMeshBasicShapeTools.h"
#include "Mesh/RealtimeMeshBuilder.h"
#include "Mesh/RealtimeMeshSimpleData.h"


ARealtimeMeshBasicUsageActor::ARealtimeMeshBasicUsageActor()
{
}

void ARealtimeMeshBasicUsageActor::OnGenerateMesh_Implementation()
{
	Super::OnGenerateMesh_Implementation();

	// Initialize the simple mesh
	URealtimeMeshSimple* RealtimeMesh = GetRealtimeMeshComponent()->InitializeRealtimeMesh<URealtimeMeshSimple>();



	FRealtimeMeshStreamSet StreamSet;
	TRealtimeMeshBuilderLocal<uint16, FPackedNormal, FVector2DHalf, 1> Builder(StreamSet);

	FVector3f BoxRadius = FVector3f(100, 100, 100);

	// Generate verts
	FVector3f BoxVerts[8];
	BoxVerts[0] = FVector3f(-BoxRadius.X, BoxRadius.Y, BoxRadius.Z);
	BoxVerts[1] = FVector3f(BoxRadius.X, BoxRadius.Y, BoxRadius.Z);
	BoxVerts[2] = FVector3f(BoxRadius.X, -BoxRadius.Y, BoxRadius.Z);
	BoxVerts[3] = FVector3f(-BoxRadius.X, -BoxRadius.Y, BoxRadius.Z);

	BoxVerts[4] = FVector3f(-BoxRadius.X, BoxRadius.Y, -BoxRadius.Z);
	BoxVerts[5] = FVector3f(BoxRadius.X, BoxRadius.Y, -BoxRadius.Z);
	BoxVerts[6] = FVector3f(BoxRadius.X, -BoxRadius.Y, -BoxRadius.Z);
	BoxVerts[7] = FVector3f(-BoxRadius.X, -BoxRadius.Y, -BoxRadius.Z);

	int32 PolyGroup = 0;

	{
		int32 V0 = Builder.AddVertex(BoxVerts[0])
			.SetNormalAndTangent(FVector3f(0.0f, 0.0f, 1.0f), FVector3f(0.0f, -1.0f, 0.0f))
			.SetTexCoords(FVector2f(0.0f, 0.0f))
			.GetIndex();

		int32 V1 = Builder.AddVertex(BoxVerts[1])
			.SetNormalAndTangent(FVector3f(0.0f, 0.0f, 1.0f), FVector3f(0.0f, -1.0f, 0.0f))
			.SetTexCoords(FVector2f(0.0f, 1.0f))
			.GetIndex();

		int32 V2 = Builder.AddVertex(BoxVerts[2])
			.SetNormalAndTangent(FVector3f(0.0f, 0.0f, 1.0f), FVector3f(0.0f, -1.0f, 0.0f))
			.SetTexCoords(FVector2f(1.0f, 1.0f))
			.GetIndex();

		int32 V3 = Builder.AddVertex(BoxVerts[3])
			.SetNormalAndTangent(FVector3f(0.0f, 0.0f, 1.0f), FVector3f(0.0f, -1.0f, 0.0f))
			.SetTexCoords(FVector2f(1.0f, 0.0f))
			.GetIndex();

		Builder.AddTriangle(V0, V1, V3, PolyGroup);
		Builder.AddTriangle(V1, V2, V3, PolyGroup);		
	}

	{
		int32 V0 = Builder.AddVertex(BoxVerts[4])
			.SetNormalAndTangent(FVector3f(-1.0f, 0.0f, 0.0f), FVector3f(0.0f, -1.0f, 0.0f))
			.SetTexCoords(FVector2f(0.0f, 0.0f))
			.GetIndex();

		int32 V1 = Builder.AddVertex(BoxVerts[0])
			.SetNormalAndTangent(FVector3f(-1.0f, 0.0f, 0.0f), FVector3f(0.0f, -1.0f, 0.0f))
			.SetTexCoords(FVector2f(0.0f, 1.0f))
			.GetIndex();

		int32 V2 = Builder.AddVertex(BoxVerts[3])
			.SetNormalAndTangent(FVector3f(-1.0f, 0.0f, 0.0f), FVector3f(0.0f, -1.0f, 0.0f))
			.SetTexCoords(FVector2f(1.0f, 1.0f))
			.GetIndex();

		int32 V3 = Builder.AddVertex(BoxVerts[7])
			.SetNormalAndTangent(FVector3f(-1.0f, 0.0f, 0.0f), FVector3f(0.0f, -1.0f, 0.0f))
			.SetTexCoords(FVector2f(1.0f, 0.0f))
			.GetIndex();

		Builder.AddTriangle(V0, V1, V3, PolyGroup);
		Builder.AddTriangle(V1, V2, V3, PolyGroup);		
	}

	{
		int32 V0 = Builder.AddVertex(BoxVerts[5])
			.SetNormalAndTangent(FVector3f(0.0f, 1.0f, 0.0f), FVector3f(-1.0f, 0.0f, 0.0f))
			.SetTexCoords(FVector2f(0.0f, 0.0f))
			.GetIndex();

		int32 V1 = Builder.AddVertex(BoxVerts[1])
			.SetNormalAndTangent(FVector3f(0.0f, 1.0f, 0.0f), FVector3f(-1.0f, 0.0f, 0.0f))
			.SetTexCoords(FVector2f(0.0f, 1.0f))
			.GetIndex();

		int32 V2 = Builder.AddVertex(BoxVerts[0])
			.SetNormalAndTangent(FVector3f(0.0f, 1.0f, 0.0f), FVector3f(-1.0f, 0.0f, 0.0f))
			.SetTexCoords(FVector2f(1.0f, 1.0f))
			.GetIndex();

		int32 V3 = Builder.AddVertex(BoxVerts[4])
			.SetNormalAndTangent(FVector3f(0.0f, 1.0f, 0.0f), FVector3f(-1.0f, 0.0f, 0.0f))
			.SetTexCoords(FVector2f(1.0f, 0.0f))
			.GetIndex();

		Builder.AddTriangle(V0, V1, V3, PolyGroup);
		Builder.AddTriangle(V1, V2, V3, PolyGroup);		
	}

	{
		int32 V0 = Builder.AddVertex(BoxVerts[6])
			.SetNormalAndTangent(FVector3f(1.0f, 0.0f, 0.0f), FVector3f(0.0f, 1.0f, 0.0f))
			.SetTexCoords(FVector2f(0.0f, 0.0f))
			.GetIndex();

		int32 V1 = Builder.AddVertex(BoxVerts[2])
			.SetNormalAndTangent(FVector3f(1.0f, 0.0f, 0.0f), FVector3f(0.0f, 1.0f, 0.0f))
			.SetTexCoords(FVector2f(0.0f, 1.0f))
			.GetIndex();

		int32 V2 = Builder.AddVertex(BoxVerts[1])
			.SetNormalAndTangent(FVector3f(1.0f, 0.0f, 0.0f), FVector3f(0.0f, 1.0f, 0.0f))
			.SetTexCoords(FVector2f(1.0f, 1.0f))
			.GetIndex();

		int32 V3 = Builder.AddVertex(BoxVerts[5])
			.SetNormalAndTangent(FVector3f(1.0f, 0.0f, 0.0f), FVector3f(0.0f, 1.0f, 0.0f))
			.SetTexCoords(FVector2f(1.0f, 0.0f))
			.GetIndex();

		Builder.AddTriangle(V0, V1, V3, PolyGroup);
		Builder.AddTriangle(V1, V2, V3, PolyGroup);		
	}

	{
		int32 V0 = Builder.AddVertex(BoxVerts[7])
			.SetNormalAndTangent(FVector3f(0.0f, -1.0f, 0.0f), FVector3f(1.0f, 0.0f, 0.0f))
			.SetTexCoords(FVector2f(0.0f, 0.0f))
			.GetIndex();

		int32 V1 = Builder.AddVertex(BoxVerts[3])
			.SetNormalAndTangent(FVector3f(0.0f, -1.0f, 0.0f), FVector3f(1.0f, 0.0f, 0.0f))
			.SetTexCoords(FVector2f(0.0f, 1.0f))
			.GetIndex();

		int32 V2 = Builder.AddVertex(BoxVerts[2])
			.SetNormalAndTangent(FVector3f(0.0f, -1.0f, 0.0f), FVector3f(1.0f, 0.0f, 0.0f))
			.SetTexCoords(FVector2f(1.0f, 1.0f))
			.GetIndex();

		int32 V3 = Builder.AddVertex(BoxVerts[6])
			.SetNormalAndTangent(FVector3f(0.0f, -1.0f, 0.0f), FVector3f(1.0f, 0.0f, 0.0f))
			.SetTexCoords(FVector2f(1.0f, 0.0f))
			.GetIndex();

		Builder.AddTriangle(V0, V1, V3, PolyGroup);
		Builder.AddTriangle(V1, V2, V3, PolyGroup);		
	}

	{
		int32 V0 = Builder.AddVertex(BoxVerts[7])
			.SetNormalAndTangent(FVector3f(0.0f, 0.0f, -1.0f), FVector3f(0.0f, 1.0f, 0.0f))
			.SetTexCoords(FVector2f(0.0f, 0.0f))
			.GetIndex();

		int32 V1 = Builder.AddVertex(BoxVerts[6])
			.SetNormalAndTangent(FVector3f(0.0f, 0.0f, -1.0f), FVector3f(0.0f, 1.0f, 0.0f))
			.SetTexCoords(FVector2f(0.0f, 1.0f))
			.GetIndex();

		int32 V2 = Builder.AddVertex(BoxVerts[5])
			.SetNormalAndTangent(FVector3f(0.0f, 0.0f, -1.0f), FVector3f(0.0f, 1.0f, 0.0f))
			.SetTexCoords(FVector2f(1.0f, 1.0f))
			.GetIndex();

		int32 V3 = Builder.AddVertex(BoxVerts[4])
			.SetNormalAndTangent(FVector3f(0.0f, 0.0f, -1.0f), FVector3f(0.0f, 1.0f, 0.0f))
			.SetTexCoords(FVector2f(1.0f, 0.0f))
			.GetIndex();

		Builder.AddTriangle(V0, V1, V3, PolyGroup);
		Builder.AddTriangle(V1, V2, V3, PolyGroup);		
	}
	
	// This example create 3 rectangular prisms, one on each axis, with 2 of them grouped in the same vertex buffers, but with different sections
	// This allows for setting up separate materials even if sections share a single set of buffers

	// Setup the two material slots
	RealtimeMesh->SetupMaterialSlot(0, "PrimaryMaterial");
	RealtimeMesh->SetupMaterialSlot(1, "SecondaryMaterial");

	// This will create a new section group named "TestBox" at LOD 0, with the created stream data above. This will create the sections associated with the polygroup
	RealtimeMesh->CreateSectionGroup(FRealtimeMeshSectionGroupKey::Create(0, FName("TestBox")), StreamSet);
	
	
	
	

	/*
	{	// Create a basic single section
		FRealtimeMeshSimpleMeshData MeshData;
		FRealtimeMeshSimpleMeshData MeshData2;
		FRealtimeMeshSimpleMeshData MeshData3;

	
		// This just adds a simple box, you can instead create your own mesh data
		URealtimeMeshBasicShapeTools::AppendBoxMesh(FVector(100, 100, 200), FTransform::Identity, MeshData, 2);
		URealtimeMeshBasicShapeTools::AppendBoxMesh(FVector(200, 100, 100), FTransform::Identity, MeshData2, 1);
		URealtimeMeshBasicShapeTools::AppendBoxMesh(FVector(100, 200, 100), FTransform::Identity, MeshData3, 0);

		URealtimeMeshBasicShapeTools::AppendMesh(MeshData, MeshData2, FTransform::Identity);
		URealtimeMeshBasicShapeTools::AppendMesh(MeshData, MeshData3, FTransform::Identity);

		// Create a single section, with its own dedicated section group

		const auto SectionGroupKey = FRealtimeMeshSectionGroupKey::Create(0, FName("TestTripleBox"));
		RealtimeMesh->CreateSectionGroup(SectionGroupKey, MeshData);

		auto SectionGroup = RealtimeMesh->GetMeshData()->GetSectionGroupAs<FRealtimeMeshSectionGroupSimple>(SectionGroupKey);
		SectionGroup->SetPolyGroupSectionHandler(FRealtimeMeshPolyGroupConfigHandler::CreateUObject(this, &ARealtimeMeshBasicUsageActor::OnAddSectionToPolyGroup));

		FRealtimeMeshSectionConfig VisibleConfig;
		VisibleConfig.bIsVisible = true;

		RealtimeMesh->UpdateSectionConfig(FRealtimeMeshSectionKey::CreateForPolyGroup(SectionGroupKey, 1), VisibleConfig, false);
		RealtimeMesh->UpdateSectionConfig(FRealtimeMeshSectionKey::CreateForPolyGroup(SectionGroupKey, 2), VisibleConfig, false);
		RealtimeMesh->UpdateSectionConfig(FRealtimeMeshSectionKey::CreateForPolyGroup(SectionGroupKey, 0), VisibleConfig, false);
		
		FRealtimeMeshSimpleGeometry SimpleGeometry = RealtimeMesh->GetSimpleGeometry();
		SimpleGeometry.AddBox(FRealtimeMeshCollisionBox(FVector(200, 200, 400)));
		SimpleGeometry.AddBox(FRealtimeMeshCollisionBox(FVector(400, 200, 200)));
		SimpleGeometry.AddBox(FRealtimeMeshCollisionBox(FVector(200, 400, 200)));
		RealtimeMesh->SetSimpleGeometry(SimpleGeometry);

		FRealtimeMeshCollisionConfiguration CollisionConfig;
		CollisionConfig.bUseComplexAsSimpleCollision = false;
		RealtimeMesh->SetCollisionConfig(CollisionConfig);

		/*FRealtimeMeshSectionKey StaticSectionKey = FRealtimeMeshSectionKey::CreateUnique(FRealtimeMeshSectionGroupKey::CreateUnique(0));
		RealtimeMesh->CreateStandaloneSection(StaticSectionKey, FRealtimeMeshSectionConfig(ERealtimeMeshSectionDrawType::Static, 0), MeshData, true);

		RealtimeMesh->EditMeshInPlace(StaticSectionKey, [](FRealtimeMeshStreamSet& MeshData)
			{
				TRealtimeMeshBuilderLocal<> Builder(MeshData);

				Builder.SetColor(5, FColor::Red);

				// We only touched color so only push that
				return TSet { FRealtimeMeshStreams::Color };
			});#1#
	}*/
	
	/*{	// Create a basic group with 2 sections
		FRealtimeMeshSimpleMeshData MeshData;

		// This just adds two simple boxes, one after the other

		// Create a section group passing it our mesh data
		FRealtimeMeshSectionGroupKey GroupKey = FRealtimeMeshSectionGroupKey::CreateUnique(0);
		RealtimeMesh->CreateSectionGroup(GroupKey, MeshData);

		// Create both sections on the same mesh data
		FRealtimeMeshSectionKey SectionInGroupA = FRealtimeMeshSectionKey::CreateUnique(GroupKey);
		RealtimeMesh->CreateSection(SectionInGroupA,
			FRealtimeMeshSectionConfig(ERealtimeMeshSectionDrawType::Static, 0),
			FRealtimeMeshStreamRange(0, 24, 0, 36), true);
		
		FRealtimeMeshSectionKey SectionInGroupB = FRealtimeMeshSectionKey::CreateUnique(GroupKey);
		RealtimeMesh->CreateSection(SectionInGroupB,
			FRealtimeMeshSectionConfig(ERealtimeMeshSectionDrawType::Static, 1),
			FRealtimeMeshStreamRange(24, 48, 36, 72), true);
	}*/
}

FRealtimeMeshSectionConfig ARealtimeMeshBasicUsageActor::OnAddSectionToPolyGroup(int32 PolyGroupIndex)
{
	return FRealtimeMeshSectionConfig(ERealtimeMeshSectionDrawType::Static, PolyGroupIndex);
}
