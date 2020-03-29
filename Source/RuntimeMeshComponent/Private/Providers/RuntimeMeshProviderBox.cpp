// Copyright 2016-2019 Chris Conway (Koderz). All Rights Reserved.


#include "Providers/RuntimeMeshProviderBox.h"

FRuntimeMeshProviderBoxProxy::FRuntimeMeshProviderBoxProxy(TWeakObjectPtr<URuntimeMeshProvider> InParent)
	: FRuntimeMeshProviderProxy(InParent)
{

}

FRuntimeMeshProviderBoxProxy::~FRuntimeMeshProviderBoxProxy()
{

}

void FRuntimeMeshProviderBoxProxy::UpdateProxyParameters(URuntimeMeshProvider* ParentProvider, bool bIsInitialSetup)
{
	URuntimeMeshProviderBox* BoxProvider = Cast<URuntimeMeshProviderBox>(ParentProvider);
	if (BoxRadius != BoxProvider->BoxRadius)
	{
		MarkCollisionDirty();
	}
	BoxRadius = BoxProvider->BoxRadius;
	Material = BoxProvider->Material;
	MarkSectionDirty(0, 0);
}

void FRuntimeMeshProviderBoxProxy::Initialize()
{
	FRuntimeMeshLODProperties LODProperties;
	LODProperties.ScreenSize = 0.0f;

	ConfigureLODs({ LODProperties });

	SetupMaterialSlot(0, FName("Cube Base"), Material.Get());

	FRuntimeMeshSectionProperties Properties;
	Properties.bCastsShadow = true;
	Properties.bIsVisible = true;
	Properties.MaterialSlot = 0;
	Properties.UpdateFrequency = ERuntimeMeshUpdateFrequency::Infrequent;
	CreateSection(0, 0, Properties);
}

bool FRuntimeMeshProviderBoxProxy::GetSectionMeshForLOD(int32 LODIndex, int32 SectionId, FRuntimeMeshRenderableMeshData& MeshData)
{
	// We should only ever be queried for section 0 and lod 0
	check(SectionId == 0 && LODIndex == 0);

	// Generate verts
	FVector BoxVerts[8];
	BoxVerts[0] = FVector(-BoxRadius.X, BoxRadius.Y, BoxRadius.Z);
	BoxVerts[1] = FVector(BoxRadius.X, BoxRadius.Y, BoxRadius.Z);
	BoxVerts[2] = FVector(BoxRadius.X, -BoxRadius.Y, BoxRadius.Z);
	BoxVerts[3] = FVector(-BoxRadius.X, -BoxRadius.Y, BoxRadius.Z);

	BoxVerts[4] = FVector(-BoxRadius.X, BoxRadius.Y, -BoxRadius.Z);
	BoxVerts[5] = FVector(BoxRadius.X, BoxRadius.Y, -BoxRadius.Z);
	BoxVerts[6] = FVector(BoxRadius.X, -BoxRadius.Y, -BoxRadius.Z);
	BoxVerts[7] = FVector(-BoxRadius.X, -BoxRadius.Y, -BoxRadius.Z);

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

FRuntimeMeshCollisionSettings FRuntimeMeshProviderBoxProxy::GetCollisionSettings()
{
	FRuntimeMeshCollisionSettings Settings;
	Settings.bUseAsyncCooking = true;
	Settings.bUseComplexAsSimple = false;

	Settings.Boxes.Emplace(BoxRadius.X * 2, BoxRadius.Y * 2, BoxRadius.Z * 2);
	   	 
	return Settings;
}

bool FRuntimeMeshProviderBoxProxy::HasCollisionMesh()
{
	return true;
}

bool FRuntimeMeshProviderBoxProxy::GetCollisionMesh(FRuntimeMeshCollisionData& CollisionData)
{
	// Add the single collision section
	CollisionData.CollisionSources.Emplace(0, 5, GetParent(), 0, ERuntimeMeshCollisionFaceSourceType::Collision);
	
	FRuntimeMeshCollisionVertexStream& CollisionVertices = CollisionData.Vertices;
	FRuntimeMeshCollisionTriangleStream& CollisionTriangles = CollisionData.Triangles;

	// Generate verts
	CollisionVertices.Add(FVector(-BoxRadius.X, BoxRadius.Y, BoxRadius.Z));
	CollisionVertices.Add(FVector(BoxRadius.X, BoxRadius.Y, BoxRadius.Z));
	CollisionVertices.Add(FVector(BoxRadius.X, -BoxRadius.Y, BoxRadius.Z));
	CollisionVertices.Add(FVector(-BoxRadius.X, -BoxRadius.Y, BoxRadius.Z));

	CollisionVertices.Add(FVector(-BoxRadius.X, BoxRadius.Y, -BoxRadius.Z));
	CollisionVertices.Add(FVector(BoxRadius.X, BoxRadius.Y, -BoxRadius.Z));
	CollisionVertices.Add(FVector(BoxRadius.X, -BoxRadius.Y, -BoxRadius.Z));
	CollisionVertices.Add(FVector(-BoxRadius.X, -BoxRadius.Y, -BoxRadius.Z));

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

bool FRuntimeMeshProviderBoxProxy::IsThreadSafe() const
{
	return true;
}
