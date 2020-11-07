// Copyright 2016-2020 TriAxis Games L.L.C. All Rights Reserved.

#include "Providers/RuntimeMeshProviderBox.h"



FVector URuntimeMeshProviderBox::GetBoxRadius() const
{
	FScopeLock Lock(&PropertySyncRoot);
	return BoxRadius;
}

void URuntimeMeshProviderBox::SetBoxRadius(const FVector& InRadius)
{
	FScopeLock Lock(&PropertySyncRoot);
	BoxRadius = InRadius;

	MarkAllLODsDirty();
	MarkCollisionDirty();
}

UMaterialInterface* URuntimeMeshProviderBox::GetBoxMaterial() const
{
	FScopeLock Lock(&PropertySyncRoot);
	return Material;
}

void URuntimeMeshProviderBox::SetBoxMaterial(UMaterialInterface* InMaterial)
{
	FScopeLock Lock(&PropertySyncRoot);
	Material = InMaterial;
	SetupMaterialSlot(0, FName("Cube Base"), Material);
}



void URuntimeMeshProviderBox::Initialize()
{
	FRuntimeMeshLODProperties LODProperties;
	LODProperties.ScreenSize = 0.0f;

	ConfigureLODs({ LODProperties });

	SetupMaterialSlot(0, FName("Cube Base"), GetBoxMaterial());

	FRuntimeMeshSectionProperties Properties;
	Properties.bCastsShadow = true;
	Properties.bIsVisible = true;
	Properties.MaterialSlot = 0;
	Properties.UpdateFrequency = ERuntimeMeshUpdateFrequency::Infrequent;
	CreateSection(0, 0, Properties);

	MarkCollisionDirty();
}

FBoxSphereBounds URuntimeMeshProviderBox::GetBounds()
{
	return FBoxSphereBounds(FBox(-BoxRadius, BoxRadius));
}

bool URuntimeMeshProviderBox::GetSectionMeshForLOD(int32 LODIndex, int32 SectionId, FRuntimeMeshRenderableMeshData& MeshData)
{	// We should only ever be queried for section 0 and lod 0
	check(SectionId == 0 && LODIndex == 0);

	FVector BoxRadiusTemp = BoxRadius;

	// Generate verts
	FVector BoxVerts[8];
	BoxVerts[0] = FVector(-BoxRadiusTemp.X, BoxRadiusTemp.Y, BoxRadiusTemp.Z);
	BoxVerts[1] = FVector(BoxRadiusTemp.X, BoxRadiusTemp.Y, BoxRadiusTemp.Z);
	BoxVerts[2] = FVector(BoxRadiusTemp.X, -BoxRadiusTemp.Y, BoxRadiusTemp.Z);
	BoxVerts[3] = FVector(-BoxRadiusTemp.X, -BoxRadiusTemp.Y, BoxRadiusTemp.Z);

	BoxVerts[4] = FVector(-BoxRadiusTemp.X, BoxRadiusTemp.Y, -BoxRadiusTemp.Z);
	BoxVerts[5] = FVector(BoxRadiusTemp.X, BoxRadiusTemp.Y, -BoxRadiusTemp.Z);
	BoxVerts[6] = FVector(BoxRadiusTemp.X, -BoxRadiusTemp.Y, -BoxRadiusTemp.Z);
	BoxVerts[7] = FVector(-BoxRadiusTemp.X, -BoxRadiusTemp.Y, -BoxRadiusTemp.Z);

	FVector TangentX, TangentY, TangentZ;


	auto AddVertex = [&](const FVector& InPosition, const FVector& InTangentX, const FVector& InTangentZ, const FVector2D& InTexCoord)
	{
		MeshData.Positions.Add(InPosition);
		MeshData.Tangents.Add(InTangentZ, InTangentX);
		MeshData.Colors.Add(FColor::White);
		MeshData.TexCoords.Add(InTexCoord);
	};



	// Pos Z
	TangentZ = FVector(0.0f, 0.0f, 1.0f);
	TangentX = FVector(0.0f, -1.0f, 0.0f);
	AddVertex(BoxVerts[0], TangentX, TangentZ, FVector2D(0.0f, 0.0f));
	AddVertex(BoxVerts[1], TangentX, TangentZ, FVector2D(0.0f, 1.0f));
	AddVertex(BoxVerts[2], TangentX, TangentZ, FVector2D(1.0f, 1.0f));
	AddVertex(BoxVerts[3], TangentX, TangentZ, FVector2D(1.0f, 0.0f));
	MeshData.Triangles.AddTriangle(0, 1, 3);
	MeshData.Triangles.AddTriangle(1, 2, 3);

	// Neg X
	TangentZ = FVector(-1.0f, 0.0f, 0.0f);
	TangentX = FVector(0.0f, -1.0f, 0.0f);
	AddVertex(BoxVerts[4], TangentX, TangentZ, FVector2D(0.0f, 0.0f));
	AddVertex(BoxVerts[0], TangentX, TangentZ, FVector2D(0.0f, 1.0f));
	AddVertex(BoxVerts[3], TangentX, TangentZ, FVector2D(1.0f, 1.0f));
	AddVertex(BoxVerts[7], TangentX, TangentZ, FVector2D(1.0f, 0.0f));
	MeshData.Triangles.AddTriangle(4, 5, 7);
	MeshData.Triangles.AddTriangle(5, 6, 7);

	// Pos Y
	TangentZ = FVector(0.0f, 1.0f, 0.0f);
	TangentX = FVector(-1.0f, 0.0f, 0.0f);
	AddVertex(BoxVerts[5], TangentX, TangentZ, FVector2D(0.0f, 0.0f));
	AddVertex(BoxVerts[1], TangentX, TangentZ, FVector2D(0.0f, 1.0f));
	AddVertex(BoxVerts[0], TangentX, TangentZ, FVector2D(1.0f, 1.0f));
	AddVertex(BoxVerts[4], TangentX, TangentZ, FVector2D(1.0f, 0.0f));
	MeshData.Triangles.AddTriangle(8, 9, 11);
	MeshData.Triangles.AddTriangle(9, 10, 11);

	// Pos X
	TangentZ = FVector(1.0f, 0.0f, 0.0f);
	TangentX = FVector(0.0f, 1.0f, 0.0f);
	AddVertex(BoxVerts[6], TangentX, TangentZ, FVector2D(0.0f, 0.0f));
	AddVertex(BoxVerts[2], TangentX, TangentZ, FVector2D(0.0f, 1.0f));
	AddVertex(BoxVerts[1], TangentX, TangentZ, FVector2D(1.0f, 1.0f));
	AddVertex(BoxVerts[5], TangentX, TangentZ, FVector2D(1.0f, 0.0f));
	MeshData.Triangles.AddTriangle(12, 13, 15);
	MeshData.Triangles.AddTriangle(13, 14, 15);

	// Neg Y
	TangentZ = FVector(0.0f, -1.0f, 0.0f);
	TangentX = FVector(1.0f, 0.0f, 0.0f);
	AddVertex(BoxVerts[7], TangentX, TangentZ, FVector2D(0.0f, 0.0f));
	AddVertex(BoxVerts[3], TangentX, TangentZ, FVector2D(0.0f, 1.0f));
	AddVertex(BoxVerts[2], TangentX, TangentZ, FVector2D(1.0f, 1.0f));
	AddVertex(BoxVerts[6], TangentX, TangentZ, FVector2D(1.0f, 0.0f));
	MeshData.Triangles.AddTriangle(16, 17, 19);
	MeshData.Triangles.AddTriangle(17, 18, 19);

	// Neg Z
	TangentZ = FVector(0.0f, 0.0f, -1.0f);
	TangentX = FVector(0.0f, 1.0f, 0.0f);
	AddVertex(BoxVerts[7], TangentX, TangentZ, FVector2D(0.0f, 0.0f));
	AddVertex(BoxVerts[6], TangentX, TangentZ, FVector2D(0.0f, 1.0f));
	AddVertex(BoxVerts[5], TangentX, TangentZ, FVector2D(1.0f, 1.0f));
	AddVertex(BoxVerts[4], TangentX, TangentZ, FVector2D(1.0f, 0.0f));
	MeshData.Triangles.AddTriangle(20, 21, 23);
	MeshData.Triangles.AddTriangle(21, 22, 23);

	return true;
}

FRuntimeMeshCollisionSettings URuntimeMeshProviderBox::GetCollisionSettings()
{
	FRuntimeMeshCollisionSettings Settings;
	Settings.bUseAsyncCooking = true;
	Settings.bUseComplexAsSimple = false;

	FVector BoxRadiusTemp = BoxRadius;
	Settings.Boxes.Emplace(BoxRadiusTemp.X * 2, BoxRadiusTemp.Y * 2, BoxRadiusTemp.Z * 2);

	return Settings;
}

bool URuntimeMeshProviderBox::HasCollisionMesh()
{
	return true;
}

bool URuntimeMeshProviderBox::GetCollisionMesh(FRuntimeMeshCollisionData& CollisionData)
{
	// Add the single collision section
	CollisionData.CollisionSources.Emplace(0, 5, this, 0, ERuntimeMeshCollisionFaceSourceType::Collision);

	FRuntimeMeshCollisionVertexStream& CollisionVertices = CollisionData.Vertices;
	FRuntimeMeshCollisionTriangleStream& CollisionTriangles = CollisionData.Triangles;

	FVector BoxRadiusTemp = BoxRadius;

	// Generate verts
	CollisionVertices.Add(FVector(-BoxRadiusTemp.X, BoxRadiusTemp.Y, BoxRadiusTemp.Z));
	CollisionVertices.Add(FVector(BoxRadiusTemp.X, BoxRadiusTemp.Y, BoxRadiusTemp.Z));
	CollisionVertices.Add(FVector(BoxRadiusTemp.X, -BoxRadiusTemp.Y, BoxRadiusTemp.Z));
	CollisionVertices.Add(FVector(-BoxRadiusTemp.X, -BoxRadiusTemp.Y, BoxRadiusTemp.Z));

	CollisionVertices.Add(FVector(-BoxRadiusTemp.X, BoxRadiusTemp.Y, -BoxRadiusTemp.Z));
	CollisionVertices.Add(FVector(BoxRadiusTemp.X, BoxRadiusTemp.Y, -BoxRadiusTemp.Z));
	CollisionVertices.Add(FVector(BoxRadiusTemp.X, -BoxRadiusTemp.Y, -BoxRadiusTemp.Z));
	CollisionVertices.Add(FVector(-BoxRadiusTemp.X, -BoxRadiusTemp.Y, -BoxRadiusTemp.Z));

	// Pos Z
	CollisionTriangles.Add(0, 1, 3);
	CollisionTriangles.Add(1, 2, 3);
	// Neg X
	CollisionTriangles.Add(4, 0, 7);
	CollisionTriangles.Add(0, 3, 7);
	// Pos Y
	CollisionTriangles.Add(5, 1, 4);
	CollisionTriangles.Add(1, 0, 4);
	// Pos X
	CollisionTriangles.Add(6, 2, 5);
	CollisionTriangles.Add(2, 1, 5);
	// Neg Y
	CollisionTriangles.Add(7, 3, 6);
	CollisionTriangles.Add(3, 2, 6);
	// Neg Z
	CollisionTriangles.Add(7, 6, 4);
	CollisionTriangles.Add(6, 5, 4);

	return true;
}

bool URuntimeMeshProviderBox::IsThreadSafe()
{
	return true;
}


