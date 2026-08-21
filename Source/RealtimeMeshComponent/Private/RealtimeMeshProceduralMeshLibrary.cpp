// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#include "RealtimeMeshProceduralMeshLibrary.h"
#include "Mesh/RealtimeMeshTangentUtils.h"

void URealtimeMeshProceduralMeshLibrary::ConvertQuadToTriangles(TArray<int32>& Triangles, int32 Vert0, int32 Vert1, int32 Vert2, int32 Vert3)
{
	Triangles.Add(Vert0);
	Triangles.Add(Vert1);
	Triangles.Add(Vert3);

	Triangles.Add(Vert1);
	Triangles.Add(Vert2);
	Triangles.Add(Vert3);
}

void URealtimeMeshProceduralMeshLibrary::CreateGridMeshTriangles(int32 NumX, int32 NumY, bool bWinding, TArray<int32>& Triangles)
{
	Triangles.Reset();

	if (NumX >= 2 && NumY >= 2)
	{
		// Build Quads
		for (int XIdx = 0; XIdx < NumX - 1; XIdx++)
		{
			for (int YIdx = 0; YIdx < NumY - 1; YIdx++)
			{
				const int32 I0 = (XIdx + 0) * NumY + (YIdx + 0);
				const int32 I1 = (XIdx + 1) * NumY + (YIdx + 0);
				const int32 I2 = (XIdx + 1) * NumY + (YIdx + 1);
				const int32 I3 = (XIdx + 0) * NumY + (YIdx + 1);

				if (bWinding)
				{
					ConvertQuadToTriangles(Triangles, I0, I1, I2, I3);
				}
				else
				{
					ConvertQuadToTriangles(Triangles, I0, I3, I2, I1);
				}
			}
		}
	}
}

void URealtimeMeshProceduralMeshLibrary::CreateGridMeshWelded(int32 NumX, int32 NumY, TArray<int32>& Triangles, TArray<FVector>& Vertices, TArray<FVector2D>& UVs, float GridSpacing)
{
	Triangles.Empty();
	Vertices.Empty();
	UVs.Empty();

	if (NumX >= 2 && NumY >= 2)
	{
		FVector2D Extent = FVector2D((NumX - 1) * GridSpacing, (NumY - 1) * GridSpacing) / 2;

		for (int i = 0; i < NumY; i++)
		{
			for (int j = 0; j < NumX; j++)
			{
				Vertices.Add(FVector((float)j * GridSpacing - Extent.X, (float)i * GridSpacing - Extent.Y, 0));
				UVs.Add(FVector2D((float)j / ((float)NumX - 1), (float)i / ((float)NumY - 1)));
			}
		}

		for (int i = 0; i < NumY - 1; i++)
		{
			for (int j = 0; j < NumX - 1; j++)
			{
				int idx = j + (i * NumX);
				Triangles.Add(idx);
				Triangles.Add(idx + NumX);
				Triangles.Add(idx + 1);

				Triangles.Add(idx + 1);
				Triangles.Add(idx + NumX);
				Triangles.Add(idx + NumX + 1);
			}
		}
	}
}

void URealtimeMeshProceduralMeshLibrary::CreateGridMeshSplit(int32 NumX, int32 NumY, TArray<int32>& Triangles, TArray<FVector>& Vertices, TArray<FVector2D>& UVs, TArray<FVector2D>& UV1s, float GridSpacing)
{
	Triangles.Empty();
	Vertices.Empty();
	UVs.Empty();
	UV1s.Empty();

	if (NumX >= 2 && NumY >= 2)
	{
		FVector2D Extent = FVector2D(NumX * GridSpacing, NumY * GridSpacing) / 2;

		for (int i = 0; i < NumY - 1; i++)
		{
			for (int j = 0; j < NumX - 1; j++)
			{
				int idx = j + (i * (NumX - 1));
				Triangles.Add(idx * 4 + 3);
				Triangles.Add(idx * 4 + 1);
				Triangles.Add(idx * 4);

				Triangles.Add(idx * 4 + 3);
				Triangles.Add(idx * 4 + 2);
				Triangles.Add(idx * 4 + 1);

				float Z = FMath::Fmod(idx, 5.f) * GridSpacing;
				FVector CornerVert = FVector((float)j * GridSpacing - Extent.X, (float)i * GridSpacing - Extent.Y, Z);
				Vertices.Add(CornerVert);
				Vertices.Add(CornerVert + FVector(GridSpacing, 0, 0));
				Vertices.Add(CornerVert + FVector(GridSpacing, GridSpacing, 0));
				Vertices.Add(CornerVert + FVector(0, GridSpacing, 0));

				UVs.Add(FVector2D(0, 0));
				UVs.Add(FVector2D(1, 0));
				UVs.Add(FVector2D(1, 1));
				UVs.Add(FVector2D(0, 1));

				FVector2D QuadCenter = FVector2D(((float)j + 0.5) / ((float)NumX), ((float)i + 0.5) / ((float)NumY));
				UV1s.Add(QuadCenter);
				UV1s.Add(QuadCenter);
				UV1s.Add(QuadCenter);
				UV1s.Add(QuadCenter);
			}
		}
	}
}

void URealtimeMeshProceduralMeshLibrary::GenerateBoxMesh(FVector BoxRadius, TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector>& Normals, TArray<FVector2D>& UVs, TArray<FRealtimeMeshProceduralTangent>& Tangents)
{
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

	// Generate triangles (from quads)
	Triangles.Reset();

	const int32 NumVerts = 24; // 6 faces x 4 verts per face

	Vertices.Reset();
	Vertices.AddUninitialized(NumVerts);

	Normals.Reset();
	Normals.AddUninitialized(NumVerts);

	Tangents.Reset();
	Tangents.AddUninitialized(NumVerts);

	Vertices[0] = BoxVerts[0];
	Vertices[1] = BoxVerts[1];
	Vertices[2] = BoxVerts[2];
	Vertices[3] = BoxVerts[3];
	ConvertQuadToTriangles(Triangles, 0, 1, 2, 3);
	Normals[0] = Normals[1] = Normals[2] = Normals[3] = FVector(0, 0, 1);
	Tangents[0] = Tangents[1] = Tangents[2] = Tangents[3] = FRealtimeMeshProceduralTangent(0.f, -1.f, 0.f);

	Vertices[4] = BoxVerts[4];
	Vertices[5] = BoxVerts[0];
	Vertices[6] = BoxVerts[3];
	Vertices[7] = BoxVerts[7];
	ConvertQuadToTriangles(Triangles, 4, 5, 6, 7);
	Normals[4] = Normals[5] = Normals[6] = Normals[7] = FVector(-1, 0, 0);
	Tangents[4] = Tangents[5] = Tangents[6] = Tangents[7] = FRealtimeMeshProceduralTangent(0.f, -1.f, 0.f);

	Vertices[8] = BoxVerts[5];
	Vertices[9] = BoxVerts[1];
	Vertices[10] = BoxVerts[0];
	Vertices[11] = BoxVerts[4];
	ConvertQuadToTriangles(Triangles, 8, 9, 10, 11);
	Normals[8] = Normals[9] = Normals[10] = Normals[11] = FVector(0, 1, 0);
	Tangents[8] = Tangents[9] = Tangents[10] = Tangents[11] = FRealtimeMeshProceduralTangent(-1.f, 0.f, 0.f);

	Vertices[12] = BoxVerts[6];
	Vertices[13] = BoxVerts[2];
	Vertices[14] = BoxVerts[1];
	Vertices[15] = BoxVerts[5];
	ConvertQuadToTriangles(Triangles, 12, 13, 14, 15);
	Normals[12] = Normals[13] = Normals[14] = Normals[15] = FVector(1, 0, 0);
	Tangents[12] = Tangents[13] = Tangents[14] = Tangents[15] = FRealtimeMeshProceduralTangent(0.f, 1.f, 0.f);

	Vertices[16] = BoxVerts[7];
	Vertices[17] = BoxVerts[3];
	Vertices[18] = BoxVerts[2];
	Vertices[19] = BoxVerts[6];
	ConvertQuadToTriangles(Triangles, 16, 17, 18, 19);
	Normals[16] = Normals[17] = Normals[18] = Normals[19] = FVector(0, -1, 0);
	Tangents[16] = Tangents[17] = Tangents[18] = Tangents[19] = FRealtimeMeshProceduralTangent(1.f, 0.f, 0.f);

	Vertices[20] = BoxVerts[7];
	Vertices[21] = BoxVerts[6];
	Vertices[22] = BoxVerts[5];
	Vertices[23] = BoxVerts[4];
	ConvertQuadToTriangles(Triangles, 20, 21, 22, 23);
	Normals[20] = Normals[21] = Normals[22] = Normals[23] = FVector(0, 0, -1);
	Tangents[20] = Tangents[21] = Tangents[22] = Tangents[23] = FRealtimeMeshProceduralTangent(0.f, 1.f, 0.f);

	// UVs
	UVs.Reset();
	UVs.AddUninitialized(NumVerts);

	UVs[0] = UVs[4] = UVs[8] = UVs[12] = UVs[16] = UVs[20] = FVector2D(0.f, 0.f);
	UVs[1] = UVs[5] = UVs[9] = UVs[13] = UVs[17] = UVs[21] = FVector2D(0.f, 1.f);
	UVs[2] = UVs[6] = UVs[10] = UVs[14] = UVs[18] = UVs[22] = FVector2D(1.f, 1.f);
	UVs[3] = UVs[7] = UVs[11] = UVs[15] = UVs[19] = UVs[23] = FVector2D(1.f, 0.f);
}

void URealtimeMeshProceduralMeshLibrary::CalculateTangentsForMesh(const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector2D>& UVs,
	TArray<FVector>& Normals, TArray<FRealtimeMeshProceduralTangent>& Tangents, bool bComputeSmoothNormals)
{
	const int32 NumVerts = Vertices.Num();

	Normals.Reset();
	Normals.SetNum(NumVerts);
	Tangents.Reset();
	Tangents.SetNum(NumVerts);

	if (NumVerts == 0 || Triangles.Num() < 3)
	{
		return;
	}

	// RealtimeMeshAlgo works in single precision; convert positions up front.
	TArray<FVector3f> Verts3f;
	Verts3f.Reserve(NumVerts);
	for (const FVector& Vert : Vertices)
	{
		Verts3f.Add(FVector3f(Vert));
	}

	// UVs are optional — only drive the tangent basis with them when they line up
	// with the vertices, otherwise fall back to the edge-based path (null getter).
	TFunction<FVector2f(int32)> UVGetter;
	if (UVs.Num() == NumVerts)
	{
		UVGetter = [&UVs](int32 Index) { return FVector2f(UVs[Index]); };
	}

	RealtimeMeshAlgo::GenerateTangents<int32>(
		TConstArrayView<const int32>(Triangles),
		TConstArrayView<const FVector3f>(Verts3f),
		UVGetter,
		[&Normals, &Tangents](int32 VertIdx, FVector3f TangentX, FVector3f TangentZ)
		{
			Normals[VertIdx] = FVector(TangentZ);
			Tangents[VertIdx] = FRealtimeMeshProceduralTangent(FVector(TangentX), false);
		},
		bComputeSmoothNormals);
}

void URealtimeMeshProceduralMeshLibrary::GetSectionFromProceduralMesh(URealtimeMeshProcedural* InProcMesh, int32 SectionIndex,
	TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector>& Normals, TArray<FVector2D>& UVs, TArray<FRealtimeMeshProceduralTangent>& Tangents)
{
	Vertices.Reset();
	Triangles.Reset();
	Normals.Reset();
	UVs.Reset();
	Tangents.Reset();

	if (!IsValid(InProcMesh))
	{
		return;
	}

	// Full read-back lives on the mesh; here we keep only UV0 and drop colors for
	// PMC signature parity.
	TArray<FVector2D> UV1, UV2, UV3;
	TArray<FColor> VertexColors;
	InProcMesh->GetMeshSection(SectionIndex, Vertices, Triangles, Normals, UVs, UV1, UV2, UV3, VertexColors, Tangents);
}
