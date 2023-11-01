// Copyright TriAxis Games, L.L.C. All Rights Reserved.


#include "Mesh/RealtimeMeshBasicShapeTools.h"
#include "RealtimeMeshSimple.h"
#include "Mesh/RealtimeMeshSimpleData.h"

static void ConvertQuadToTriangles(TArray<int32>& Triangles, TArray<int32>& MaterialIndices, int32 Vert0, int32 Vert1, int32 Vert2, int32 Vert3, int32 NewMaterialGroup)
{
	Triangles.Add(Vert0);
	Triangles.Add(Vert1);
	Triangles.Add(Vert3);

	Triangles.Add(Vert1);
	Triangles.Add(Vert2);
	Triangles.Add(Vert3);

	if (NewMaterialGroup != INDEX_NONE)
	{
		MaterialIndices.Add(NewMaterialGroup);
		
		MaterialIndices.Add(NewMaterialGroup);
	}
}

FRealtimeMeshSimpleMeshData& URealtimeMeshSimpleBasicShapeTools::AppendBoxMesh(FVector BoxRadius, FTransform BoxTransform, FRealtimeMeshSimpleMeshData& MeshData, int32 NewMaterialGroup)
{
	// Generate verts
	FVector BoxVerts[8];
	BoxVerts[0] = BoxTransform.TransformPosition(FVector(-BoxRadius.X, BoxRadius.Y, BoxRadius.Z));
	BoxVerts[1] = BoxTransform.TransformPosition(FVector(BoxRadius.X, BoxRadius.Y, BoxRadius.Z));
	BoxVerts[2] = BoxTransform.TransformPosition(FVector(BoxRadius.X, -BoxRadius.Y, BoxRadius.Z));
	BoxVerts[3] = BoxTransform.TransformPosition(FVector(-BoxRadius.X, -BoxRadius.Y, BoxRadius.Z));

	BoxVerts[4] = BoxTransform.TransformPosition(FVector(-BoxRadius.X, BoxRadius.Y, -BoxRadius.Z));
	BoxVerts[5] = BoxTransform.TransformPosition(FVector(BoxRadius.X, BoxRadius.Y, -BoxRadius.Z));
	BoxVerts[6] = BoxTransform.TransformPosition(FVector(BoxRadius.X, -BoxRadius.Y, -BoxRadius.Z));
	BoxVerts[7] = BoxTransform.TransformPosition(FVector(-BoxRadius.X, -BoxRadius.Y, -BoxRadius.Z));

	// Generate triangles (from quads)
	const int32 StartVertex = MeshData.Positions.Num();
	constexpr int32 NumVerts = 24; // 6 faces x 4 verts per face
	constexpr int32 NumIndices = 36;

	// Make sure the secondary arrays are the same length, zeroing them if necessary
	MeshData.Normals.SetNumZeroed(StartVertex);
	MeshData.Tangents.SetNumZeroed(StartVertex);
	MeshData.UV0.SetNumZeroed(StartVertex);

	MeshData.Positions.Reserve(StartVertex + NumVerts);
	MeshData.Normals.Reserve(StartVertex + NumVerts);
	MeshData.Tangents.Reserve(StartVertex + NumVerts);
	MeshData.UV0.Reserve(StartVertex + NumVerts);
	MeshData.Triangles.Reserve(MeshData.Triangles.Num() + NumIndices);

	if (NewMaterialGroup != INDEX_NONE)
	{
		const int32 NumExistingTriangles = (MeshData.Triangles.Num()) / 3;
		constexpr int32 NumTrianglesToAdd = NumIndices / 3;
		MeshData.MaterialIndex.Reserve(NumExistingTriangles + NumTrianglesToAdd);
		MeshData.MaterialIndex.SetNumZeroed(NumExistingTriangles);
	}
	else if (MeshData.MaterialIndex.Num() > 0)
	{
		// If we're not setting a new material group, but there's already material indices, we need to zero the new ones.
		const int32 NumExistingTriangles = (MeshData.Triangles.Num()) / 3;
		constexpr int32 NumTrianglesToAdd = NumIndices / 3;
		MeshData.MaterialIndex.SetNumZeroed(NumExistingTriangles + NumTrianglesToAdd);
	}

	const auto WriteToNextFour = [](TArray<FVector>& Array, const FVector& Value)
	{
		Array.Add(Value);
		Array.Add(Value);
		Array.Add(Value);
		Array.Add(Value);
	};

	const auto WriteQuadPositions = [&MeshData](const FVector& VertA, const FVector& VertB, const FVector& VertC, const FVector& VertD)
	{
		MeshData.Positions.Add(VertA);
		MeshData.Positions.Add(VertB);
		MeshData.Positions.Add(VertC);
		MeshData.Positions.Add(VertD);
	};

	WriteQuadPositions(BoxVerts[0], BoxVerts[1], BoxVerts[2], BoxVerts[3]);
	WriteToNextFour(MeshData.Normals, BoxTransform.TransformVectorNoScale(FVector(0.0f, 0.0f, 1.0f)));
	WriteToNextFour(MeshData.Tangents, BoxTransform.TransformVectorNoScale(FVector(0.0f, -1.0f, 0.0f)));
	ConvertQuadToTriangles(MeshData.Triangles, MeshData.MaterialIndex, StartVertex + 0, StartVertex + 1, StartVertex + 2, StartVertex + 3, NewMaterialGroup);

	WriteQuadPositions(BoxVerts[4], BoxVerts[0], BoxVerts[3], BoxVerts[7]);
	WriteToNextFour(MeshData.Normals, BoxTransform.TransformVectorNoScale(FVector(-1.0, 0.0, 0.0)));
	WriteToNextFour(MeshData.Tangents, BoxTransform.TransformVectorNoScale(FVector(0.0f, -1.0f, 0.0f)));
	ConvertQuadToTriangles(MeshData.Triangles, MeshData.MaterialIndex, StartVertex + 4, StartVertex + 5, StartVertex + 6, StartVertex + 7, NewMaterialGroup);

	WriteQuadPositions(BoxVerts[5], BoxVerts[1], BoxVerts[0], BoxVerts[4]);
	WriteToNextFour(MeshData.Normals, BoxTransform.TransformVectorNoScale(FVector(0.0, 1.0, 0.0)));
	WriteToNextFour(MeshData.Tangents, BoxTransform.TransformVectorNoScale(FVector(-1.0f, 0.0f, 0.0f)));
	ConvertQuadToTriangles(MeshData.Triangles, MeshData.MaterialIndex, StartVertex + 8, StartVertex + 9, StartVertex + 10, StartVertex + 11, NewMaterialGroup);

	WriteQuadPositions(BoxVerts[6], BoxVerts[2], BoxVerts[1], BoxVerts[5]);
	WriteToNextFour(MeshData.Normals, BoxTransform.TransformVectorNoScale(FVector(1.0, 0.0, 0.0)));
	WriteToNextFour(MeshData.Tangents, BoxTransform.TransformVectorNoScale(FVector(0.0f, 1.0f, 0.0f)));
	ConvertQuadToTriangles(MeshData.Triangles, MeshData.MaterialIndex, StartVertex + 12, StartVertex + 13, StartVertex + 14, StartVertex + 15, NewMaterialGroup);

	WriteQuadPositions(BoxVerts[7], BoxVerts[3], BoxVerts[2], BoxVerts[6]);
	WriteToNextFour(MeshData.Normals, BoxTransform.TransformVectorNoScale(FVector(0.0, -1.0, 0.0)));
	WriteToNextFour(MeshData.Tangents, BoxTransform.TransformVectorNoScale(FVector(1.0f, 0.0f, 0.0f)));
	ConvertQuadToTriangles(MeshData.Triangles, MeshData.MaterialIndex, StartVertex + 16, StartVertex + 17, StartVertex + 18, StartVertex + 19, NewMaterialGroup);

	WriteQuadPositions(BoxVerts[7], BoxVerts[6], BoxVerts[5], BoxVerts[4]);
	WriteToNextFour(MeshData.Normals, BoxTransform.TransformVectorNoScale(FVector(0.0, 0.0, -1.0)));
	WriteToNextFour(MeshData.Tangents, BoxTransform.TransformVectorNoScale(FVector(0.0f, 1.0f, 0.0f)));
	ConvertQuadToTriangles(MeshData.Triangles, MeshData.MaterialIndex, StartVertex + 20, StartVertex + 21, StartVertex + 22, StartVertex + 23, NewMaterialGroup);

	// UVs
	for (int32 Index = 0; Index < 6; Index++)
	{
		MeshData.UV0.Add(FVector2D(0.0f, 0.0f));
		MeshData.UV0.Add(FVector2D(0.0f, 1.0f));
		MeshData.UV0.Add(FVector2D(1.0f, 1.0f));
		MeshData.UV0.Add(FVector2D(1.0f, 0.0f));
	}

	return MeshData;
}

template <typename Type>
static void AppendVertexArrayIfContains(TArray<Type>& Destination, const TArray<Type>& Source, int32 VertexOffset, int32 FinalLength)
{
	if (Source.Num() > 0)
	{
		Destination.SetNumZeroed(VertexOffset);
		Destination.Append(Source.GetData(), FMath::Min(FinalLength - VertexOffset, Source.Num()));
	}
}

static void AppendTransformedTangentArray(TArray<FVector>& Destination, const TArray<FVector>& Source, int32 VertexOffset, int32 FinalLength, const FTransform& Transform)
{
	if (Source.Num() > 0)
	{
		const int32 NumToCopy = FMath::Min(FinalLength - VertexOffset, Source.Num());
		Destination.SetNumZeroed(FinalLength);
		for (int32 Index = 0; Index < NumToCopy; Index++)
		{
			Destination[VertexOffset + Index] = Transform.TransformVector(Source[Index]);
		}
	}
}


FRealtimeMeshSimpleMeshData& URealtimeMeshSimpleBasicShapeTools::AppendMesh(FRealtimeMeshSimpleMeshData& TargetMeshData, const FRealtimeMeshSimpleMeshData& MeshDataToAdd,
                                                                               const FTransform& Transform, int32 NewMaterialGroup)
{
	const int32 StartVertex = TargetMeshData.Positions.Num();

	// Skip slower transform logic if transform == identity
	if (Transform.Equals(FTransform::Identity))
	{
		TargetMeshData.Positions.Append(MeshDataToAdd.Positions);
		AppendVertexArrayIfContains(TargetMeshData.Normals, MeshDataToAdd.Normals, StartVertex, TargetMeshData.Positions.Num());
		AppendVertexArrayIfContains(TargetMeshData.Binormals, MeshDataToAdd.Binormals, StartVertex, TargetMeshData.Positions.Num());
		AppendVertexArrayIfContains(TargetMeshData.Tangents, MeshDataToAdd.Tangents, StartVertex, TargetMeshData.Positions.Num());
	}
	else
	{
		TargetMeshData.Positions.Reserve(TargetMeshData.Positions.Num() + MeshDataToAdd.Positions.Num());
		for (int32 Index = 0; Index < MeshDataToAdd.Positions.Num(); Index++)
		{
			TargetMeshData.Positions.Add(Transform.TransformPosition(MeshDataToAdd.Positions[Index]));
		}
		AppendTransformedTangentArray(TargetMeshData.Normals, MeshDataToAdd.Normals, StartVertex, TargetMeshData.Positions.Num(), Transform);
		AppendTransformedTangentArray(TargetMeshData.Binormals, MeshDataToAdd.Binormals, StartVertex, TargetMeshData.Positions.Num(), Transform);
		AppendTransformedTangentArray(TargetMeshData.Tangents, MeshDataToAdd.Tangents, StartVertex, TargetMeshData.Positions.Num(), Transform);
	}

	AppendVertexArrayIfContains(TargetMeshData.Colors, MeshDataToAdd.Colors, StartVertex, TargetMeshData.Positions.Num());
	AppendVertexArrayIfContains(TargetMeshData.LinearColors, MeshDataToAdd.LinearColors, StartVertex, TargetMeshData.Positions.Num());

	AppendVertexArrayIfContains(TargetMeshData.UV0, MeshDataToAdd.UV0, StartVertex, TargetMeshData.Positions.Num());
	AppendVertexArrayIfContains(TargetMeshData.UV1, MeshDataToAdd.UV1, StartVertex, TargetMeshData.Positions.Num());
	AppendVertexArrayIfContains(TargetMeshData.UV2, MeshDataToAdd.UV2, StartVertex, TargetMeshData.Positions.Num());
	AppendVertexArrayIfContains(TargetMeshData.UV3, MeshDataToAdd.UV3, StartVertex, TargetMeshData.Positions.Num());

	// Copy Triangles
	
	const int32 NumExistingTriangles = (TargetMeshData.Triangles.Num()) / 3;
	const int32 NumTrianglesToAdd = MeshDataToAdd.Triangles.Num() / 3;

	TargetMeshData.Triangles.Reserve(TargetMeshData.Triangles.Num() + MeshDataToAdd.Triangles.Num());
	for (int32 Index = 0; Index < MeshDataToAdd.Triangles.Num(); Index++)
	{
		TargetMeshData.Triangles.Add(MeshDataToAdd.Triangles[Index] + StartVertex);		
	}

	if (NewMaterialGroup != 0)
	{
		TargetMeshData.MaterialIndex.Reserve(NumExistingTriangles + NumTrianglesToAdd);
		TargetMeshData.MaterialIndex.SetNumZeroed(NumExistingTriangles);

		for (int32 Index = 0; Index < NumTrianglesToAdd; Index++)
		{
			TargetMeshData.MaterialIndex.Add(NewMaterialGroup);
		}
	}
	else if (MeshDataToAdd.MaterialIndex.Num())
	{
		TargetMeshData.MaterialIndex.Reserve(NumExistingTriangles + NumTrianglesToAdd);
		TargetMeshData.MaterialIndex.SetNumZeroed(NumExistingTriangles);

		for (int32 Index = 0; Index < MeshDataToAdd.MaterialIndex.Num() && Index < NumTrianglesToAdd; Index++)
		{
			TargetMeshData.MaterialIndex.Add(MeshDataToAdd.MaterialIndex[Index]);
		}

		TargetMeshData.MaterialIndex.SetNumZeroed(NumExistingTriangles + NumTrianglesToAdd);
	}

	return TargetMeshData;
}