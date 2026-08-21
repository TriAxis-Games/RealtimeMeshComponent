// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#include "Procedural/RealtimeMeshExample_Procedural_Basic.h"
#include "RealtimeMeshProcedural.h"
#include "RealtimeMeshProceduralMeshLibrary.h"

ARealtimeMeshExample_Procedural_Basic::ARealtimeMeshExample_Procedural_Basic()
{
	Description = NSLOCTEXT("RealtimeMeshExamples", "Procedural_Basic",
		"URealtimeMeshProcedural: create a box section the ProceduralMeshComponent way (parallel TArrays via CreateMeshSection).");
}

void ARealtimeMeshExample_Procedural_Basic::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	URealtimeMeshProcedural* RealtimeMesh = GetRealtimeMeshComponent()->InitializeRealtimeMesh<URealtimeMeshProcedural>();

	// Generate box geometry into PMC-style parallel arrays.
	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FRealtimeMeshProceduralTangent> Tangents;
	URealtimeMeshProceduralMeshLibrary::GenerateBoxMesh(FVector(50.0f, 50.0f, 50.0f), Vertices, Triangles, Normals, UVs, Tangents);

	TArray<FColor> VertexColors;
	VertexColors.Init(FColor::White, Vertices.Num());

	// Create section 0. Each PMC section index maps to its own section group internally.
	RealtimeMesh->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, VertexColors, Tangents, /*bCreateCollision*/ true);

	VerifyMeshBuilt();
}
