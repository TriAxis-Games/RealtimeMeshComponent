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

PRAGMA_DISABLE_DEPRECATION_WARNINGS
FRealtimeMeshSimpleMeshData& URealtimeMeshBasicShapeTools::AppendBoxMesh(FVector BoxRadius, FTransform BoxTransform, FRealtimeMeshSimpleMeshData& MeshData, int32 NewMaterialGroup)
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
PRAGMA_ENABLE_DEPRECATION_WARNINGS

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


PRAGMA_DISABLE_DEPRECATION_WARNINGS
FRealtimeMeshSimpleMeshData& URealtimeMeshBasicShapeTools::AppendMesh(FRealtimeMeshSimpleMeshData& TargetMeshData, const FRealtimeMeshSimpleMeshData& MeshDataToAdd,
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
PRAGMA_ENABLE_DEPRECATION_WARNINGS

void URealtimeMeshBasicShapeTools::AppendBoxMesh(FRealtimeMeshStreamSet& StreamSet, FVector3f BoxRadius, FTransform3f BoxTransform, int32 NewMaterialGroup, FColor Color)
{
	TRealtimeMeshBuilderLocal<void, void, void, 1, void> Builder(StreamSet);
	
	FVector3f BoxVerts[8];
	BoxVerts[0] = BoxTransform.TransformPosition(FVector3f(-BoxRadius.X, BoxRadius.Y, BoxRadius.Z));
	BoxVerts[1] = BoxTransform.TransformPosition(FVector3f(BoxRadius.X, BoxRadius.Y, BoxRadius.Z));
	BoxVerts[2] = BoxTransform.TransformPosition(FVector3f(BoxRadius.X, -BoxRadius.Y, BoxRadius.Z));
	BoxVerts[3] = BoxTransform.TransformPosition(FVector3f(-BoxRadius.X, -BoxRadius.Y, BoxRadius.Z));

	BoxVerts[4] = BoxTransform.TransformPosition(FVector3f(-BoxRadius.X, BoxRadius.Y, -BoxRadius.Z));
	BoxVerts[5] = BoxTransform.TransformPosition(FVector3f(BoxRadius.X, BoxRadius.Y, -BoxRadius.Z));
	BoxVerts[6] = BoxTransform.TransformPosition(FVector3f(BoxRadius.X, -BoxRadius.Y, -BoxRadius.Z));
	BoxVerts[7] = BoxTransform.TransformPosition(FVector3f(-BoxRadius.X, -BoxRadius.Y, -BoxRadius.Z));


	FVector3f BoxNormals[6];
	BoxNormals[0] = BoxTransform.TransformVector(FVector3f(0.0f, 0.0f, 1.0f));
	BoxNormals[1] = BoxTransform.TransformVector(FVector3f(-1.0f, 0.0f, 0.0f));
	BoxNormals[2] = BoxTransform.TransformVector(FVector3f(0.0f, 1.0f, 0.0f));
	BoxNormals[3] = BoxTransform.TransformVector(FVector3f(1.0f, 0.0f, 0.0f));
	BoxNormals[4] = BoxTransform.TransformVector(FVector3f(0.0f, -1.0f, 0.0f));
	BoxNormals[5] = BoxTransform.TransformVector(FVector3f(0.0f, 0.0f, -1.0f));

	FVector3f BoxTangents[6];
	BoxTangents[0] = BoxTransform.TransformVector(FVector3f(0.0f, -1.0f, 0.0f));
	BoxTangents[1] = BoxTransform.TransformVector(FVector3f(0.0f, -1.0f, 0.0f));
	BoxTangents[2] = BoxTransform.TransformVector(FVector3f(-1.0f, 0.0f, 0.0f));
	BoxTangents[3] = BoxTransform.TransformVector(FVector3f(0.0f, 1.0f, 0.0f));
	BoxTangents[4] = BoxTransform.TransformVector(FVector3f(1.0f, 0.0f, 0.0f));
	BoxTangents[5] = BoxTransform.TransformVector(FVector3f(0.0f, 1.0f, 0.0f));
	

	// Generate triangles (from quads)
	const int32 StartVertex = Builder.NumVertices();
	constexpr int32 NumVerts = 24; // 6 faces x 4 verts per face
	constexpr int32 NumTriangles = 12;

	// Go ahead and allocate the space for the new vertices/triangles
	Builder.ReserveAdditionalVertices(NumVerts);
	Builder.ReserveAdditionalTriangles(NumTriangles);

	const auto WriteQuad = [&](int32 V0, int32 V1, int32 V2, int32 V3)
	{	
		if (Builder.HasPolyGroups())
		{
			Builder.AddTriangle(StartVertex + V0, StartVertex + V1, StartVertex + V3, NewMaterialGroup);
			Builder.AddTriangle(StartVertex + V1, StartVertex + V2, StartVertex + V3, NewMaterialGroup);
		}
		else
		{
			Builder.AddTriangle(StartVertex + V0, StartVertex + V1, StartVertex + V3);
			Builder.AddTriangle(StartVertex + V1, StartVertex + V2, StartVertex + V3);
		}
	};

	Builder.AddVertex(BoxVerts[0]).SetNormalAndTangent(BoxNormals[0], BoxTangents[0]).SetTexCoord(FVector2f(0.0f, 0.0f)).SetColor(Color);
	Builder.AddVertex(BoxVerts[1]).SetNormalAndTangent(BoxNormals[0], BoxTangents[0]).SetTexCoord(FVector2f(0.0f, 1.0f)).SetColor(Color);
	Builder.AddVertex(BoxVerts[2]).SetNormalAndTangent(BoxNormals[0], BoxTangents[0]).SetTexCoord(FVector2f(1.0f, 1.0f)).SetColor(Color);
	Builder.AddVertex(BoxVerts[3]).SetNormalAndTangent(BoxNormals[0], BoxTangents[0]).SetTexCoord(FVector2f(1.0f, 0.0f)).SetColor(Color);
	WriteQuad(0, 1, 2, 3);

	Builder.AddVertex(BoxVerts[4]).SetNormalAndTangent(BoxNormals[1], BoxTangents[1]).SetTexCoord(FVector2f(0.0f, 0.0f)).SetColor(Color);
	Builder.AddVertex(BoxVerts[0]).SetNormalAndTangent(BoxNormals[1], BoxTangents[1]).SetTexCoord(FVector2f(0.0f, 1.0f)).SetColor(Color);
	Builder.AddVertex(BoxVerts[3]).SetNormalAndTangent(BoxNormals[1], BoxTangents[1]).SetTexCoord(FVector2f(1.0f, 1.0f)).SetColor(Color);
	Builder.AddVertex(BoxVerts[7]).SetNormalAndTangent(BoxNormals[1], BoxTangents[1]).SetTexCoord(FVector2f(1.0f, 0.0f)).SetColor(Color);
	WriteQuad(4, 5, 6, 7);

	Builder.AddVertex(BoxVerts[5]).SetNormalAndTangent(BoxNormals[2], BoxTangents[2]).SetTexCoord(FVector2f(0.0f, 0.0f)).SetColor(Color);
	Builder.AddVertex(BoxVerts[1]).SetNormalAndTangent(BoxNormals[2], BoxTangents[2]).SetTexCoord(FVector2f(0.0f, 1.0f)).SetColor(Color);
	Builder.AddVertex(BoxVerts[0]).SetNormalAndTangent(BoxNormals[2], BoxTangents[2]).SetTexCoord(FVector2f(1.0f, 1.0f)).SetColor(Color);
	Builder.AddVertex(BoxVerts[4]).SetNormalAndTangent(BoxNormals[2], BoxTangents[2]).SetTexCoord(FVector2f(1.0f, 0.0f)).SetColor(Color);
	WriteQuad(8, 9, 10, 11);

	Builder.AddVertex(BoxVerts[6]).SetNormalAndTangent(BoxNormals[3], BoxTangents[3]).SetTexCoord(FVector2f(0.0f, 0.0f)).SetColor(Color);
	Builder.AddVertex(BoxVerts[2]).SetNormalAndTangent(BoxNormals[3], BoxTangents[3]).SetTexCoord(FVector2f(0.0f, 1.0f)).SetColor(Color);
	Builder.AddVertex(BoxVerts[1]).SetNormalAndTangent(BoxNormals[3], BoxTangents[3]).SetTexCoord(FVector2f(1.0f, 1.0f)).SetColor(Color);
	Builder.AddVertex(BoxVerts[5]).SetNormalAndTangent(BoxNormals[3], BoxTangents[3]).SetTexCoord(FVector2f(1.0f, 0.0f)).SetColor(Color);
	WriteQuad(12, 13, 14, 15);

	Builder.AddVertex(BoxVerts[7]).SetNormalAndTangent(BoxNormals[4], BoxTangents[4]).SetTexCoord(FVector2f(0.0f, 0.0f)).SetColor(Color);
	Builder.AddVertex(BoxVerts[3]).SetNormalAndTangent(BoxNormals[4], BoxTangents[4]).SetTexCoord(FVector2f(0.0f, 1.0f)).SetColor(Color);
	Builder.AddVertex(BoxVerts[2]).SetNormalAndTangent(BoxNormals[4], BoxTangents[4]).SetTexCoord(FVector2f(1.0f, 1.0f)).SetColor(Color);
	Builder.AddVertex(BoxVerts[6]).SetNormalAndTangent(BoxNormals[4], BoxTangents[4]).SetTexCoord(FVector2f(1.0f, 0.0f)).SetColor(Color);
	WriteQuad(16, 17, 18, 19);

	Builder.AddVertex(BoxVerts[7]).SetNormalAndTangent(BoxNormals[5], BoxTangents[5]).SetTexCoord(FVector2f(0.0f, 0.0f)).SetColor(Color);
	Builder.AddVertex(BoxVerts[6]).SetNormalAndTangent(BoxNormals[5], BoxTangents[5]).SetTexCoord(FVector2f(0.0f, 1.0f)).SetColor(Color);
	Builder.AddVertex(BoxVerts[5]).SetNormalAndTangent(BoxNormals[5], BoxTangents[5]).SetTexCoord(FVector2f(1.0f, 1.0f)).SetColor(Color);
	Builder.AddVertex(BoxVerts[4]).SetNormalAndTangent(BoxNormals[5], BoxTangents[5]).SetTexCoord(FVector2f(1.0f, 0.0f)).SetColor(Color);
	WriteQuad(20, 21, 22, 23);	
}

void URealtimeMeshBasicShapeTools::AppendMesh(FRealtimeMeshStreamSet& TargetMeshData, const FRealtimeMeshStreamSet& MeshDataToAdd, const FTransform3f& Transform)
{
	const FRealtimeMeshStream* SourcePositionStream = MeshDataToAdd.Find(FRealtimeMeshStreams::Position);
	const FRealtimeMeshStream* SourceTriangleStream = MeshDataToAdd.Find(FRealtimeMeshStreams::Triangles);
	
	if (!SourcePositionStream || !SourceTriangleStream)
	{
		UE_LOG(RealtimeMeshLog, Warning, TEXT("Unable to AppendMesh, MeshDataToAdd does not contain a position and triangle stream"));
		return;
	}

	FRealtimeMeshStream& TargetPositionStream = TargetMeshData.FindOrAdd(FRealtimeMeshStreams::Position, SourcePositionStream->GetLayout());
	
	const int32 NumVerticesToAdd = SourcePositionStream->Num();
	const int32 StartVertex = TargetPositionStream.Num();

	// Append positions.
	if (Transform.Equals(FTransform3f::Identity))
	{
		TargetPositionStream.Append(*SourcePositionStream);
	}
	else
	{
		TargetPositionStream.AppendGenerated<FVector3f>(NumVerticesToAdd, [&](int32 Index)
		{
			return Transform.TransformPosition(*SourcePositionStream->GetDataAtVertex<FVector3f>(Index));
		});
	}

	
	auto AppendStream = [&TargetMeshData, &MeshDataToAdd](int32 StartIndex, int32 NumToAdd, const FRealtimeMeshStreamKey& StreamKey, const FRealtimeMeshBufferLayout& DefaultLayout)
	{
		if (MeshDataToAdd.Contains(StreamKey) || TargetMeshData.Contains(StreamKey))
		{
			const FRealtimeMeshStream* SourceStream = MeshDataToAdd.Find(StreamKey);
			const FRealtimeMeshBufferLayout Layout = SourceStream? SourceStream->GetLayout() : DefaultLayout;
			FRealtimeMeshStream& TargetStream = TargetMeshData.FindOrAdd(StreamKey, Layout);

			// Reserve the size if it's not already big enough
			if (TargetStream.Num() < StartIndex + NumToAdd)
			{
				TargetStream.Reserve(StartIndex + NumToAdd);
			}

			// Make sure the stream is at least to this point
			if (TargetStream.Num() < StartIndex)
			{
				TargetStream.SetNumZeroed(StartIndex);
			}

			// Make sure the stream is big enough for all the n ew elements
			if (TargetStream.Num() < StartIndex + NumToAdd)
			{
				TargetStream.SetNumUninitialized(StartIndex + NumToAdd);
			}

			// Copy any existing data
			const int32 NumToCopy = FMath::Min(NumToAdd, SourceStream? SourceStream->Num() : 0);
			if (NumToCopy > 0)
			{
				TargetStream.SetRange(StartIndex, SourceStream->GetLayout(), SourceStream->GetData(), NumToCopy);
			}

			// Zero remaining elements
			if (NumToCopy < NumToAdd)
			{
				TargetStream.ZeroRange(StartIndex + NumToCopy, NumToAdd - NumToCopy);
			}

			return NumToCopy;
		}
		return 0;
	};

	// Append secondary streams
	const int32 NumCopiedTangents = AppendStream(StartVertex, NumVerticesToAdd, FRealtimeMeshStreams::Tangents, GetRealtimeMeshBufferLayout<FRealtimeMeshTangentsNormalPrecision>());

	// If we have a transform transform the new tangents
	if (NumCopiedTangents > 0 && Transform.Equals(FTransform3f::Identity))
	{
		TRealtimeMeshStreamBuilder<FRealtimeMeshTangentsHighPrecision, void> TangentBuilder(TargetMeshData.FindChecked(FRealtimeMeshStreams::Tangents));

		for (int32 Index = StartVertex; Index < StartVertex + NumVerticesToAdd; Index++)
		{
			FRealtimeMeshTangentsHighPrecision Tangent = TangentBuilder[Index];

			Tangent.SetNormal(Transform.TransformVector(Tangent.GetNormal()));
			Tangent.SetTangent(Transform.TransformVector(Tangent.GetTangent()));

			TangentBuilder[Index] = Tangent;
		}
	}
	
	AppendStream(StartVertex, NumVerticesToAdd, FRealtimeMeshStreams::TexCoords, GetRealtimeMeshBufferLayout<FRealtimeMeshTexCoordsNormal>());
	AppendStream(StartVertex, NumVerticesToAdd, FRealtimeMeshStreams::Color, GetRealtimeMeshBufferLayout<FColor>());

	
	FRealtimeMeshStream& TargetTriangleStream = TargetMeshData.FindOrAdd(FRealtimeMeshStreams::Triangles, SourceTriangleStream->GetLayout());
	
	const int32 NumTrianglesToAdd = SourceTriangleStream->Num();
	const int32 StartTriangle = TargetTriangleStream.Num();	
	const int32 NumTrianglesCopied = AppendStream(StartTriangle, NumTrianglesToAdd, FRealtimeMeshStreams::Triangles, GetRealtimeMeshBufferLayout<TIndex3<uint32>>());
	
	TRealtimeMeshStreamBuilder<TIndex3<uint32>, void> TriangleBuilder(TargetTriangleStream);
	for (int32 Index = StartTriangle; Index < StartTriangle + NumTrianglesCopied; Index++)
	{
		TIndex3<uint32> Triangle = TriangleBuilder[Index];
		Triangle.V0 += StartVertex;
		Triangle.V1 += StartVertex;
		Triangle.V2 += StartVertex;
		TriangleBuilder[Index] = Triangle;
	}


	
	AppendStream(StartTriangle, NumTrianglesToAdd, FRealtimeMeshStreams::PolyGroups, GetRealtimeMeshBufferLayout<uint16>());
}
