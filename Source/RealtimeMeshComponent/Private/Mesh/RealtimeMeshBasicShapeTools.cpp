// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.


#include "Mesh/RealtimeMeshBasicShapeTools.h"
#include "Mesh/RealtimeMeshBlueprintMeshBuilder.h"
#include "Core/RealtimeMeshDynamicBuilder.h"
#include "RealtimeMeshSimple.h"
#include "RealtimeMeshComponentModule.h"

using namespace RealtimeMesh;

void URealtimeMeshBasicShapeTools::AppendBoxMesh(FRealtimeMeshStreamSet& StreamSet, FVector3f BoxRadius, FTransform3f BoxTransform, int32 NewMaterialGroup, FColor Color)
{
	// AppendBoxMesh adapts to whatever stream layouts the target StreamSet
	// already has, so it needs a runtime-typed builder.
	FRealtimeMeshDynamicBuilder Builder(StreamSet);

	if (!Builder.HasTangents())
	{
		Builder.EnableTangents();
	}

	if (!Builder.HasVertexColors())
	{
		Builder.EnableColors();
	}

	if (!Builder.HasTexCoords())
	{
		Builder.EnableTexCoords();
	}

	// A non-default material group was requested but the target set has no
	// poly-group stream; enable one so the requested group isn't silently
	// dropped by WriteQuad below. (Group 0 is the implicit default, so a
	// value of 0 needs no poly-group stream.)
	if (NewMaterialGroup != 0 && !Builder.HasPolyGroups())
	{
		Builder.EnablePolyGroups(GetRealtimeMeshDataElementType<uint16>());
	}


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

	const auto WriteVert = [&](const FVector3f& Pos, const FVector3f& N, const FVector3f& T, const FVector2f& UV)
	{
		const int32 Row = Builder.AddVertex(Pos);
		Builder.SetTangents(Row, N, T);
		Builder.SetTexCoord(Row, 0, UV);
		Builder.SetColor(Row, Color);
	};

	WriteVert(BoxVerts[0], BoxNormals[0], BoxTangents[0], FVector2f(0.0f, 0.0f));
	WriteVert(BoxVerts[1], BoxNormals[0], BoxTangents[0], FVector2f(0.0f, 1.0f));
	WriteVert(BoxVerts[2], BoxNormals[0], BoxTangents[0], FVector2f(1.0f, 1.0f));
	WriteVert(BoxVerts[3], BoxNormals[0], BoxTangents[0], FVector2f(1.0f, 0.0f));
	WriteQuad(0, 1, 2, 3);

	WriteVert(BoxVerts[4], BoxNormals[1], BoxTangents[1], FVector2f(0.0f, 0.0f));
	WriteVert(BoxVerts[0], BoxNormals[1], BoxTangents[1], FVector2f(0.0f, 1.0f));
	WriteVert(BoxVerts[3], BoxNormals[1], BoxTangents[1], FVector2f(1.0f, 1.0f));
	WriteVert(BoxVerts[7], BoxNormals[1], BoxTangents[1], FVector2f(1.0f, 0.0f));
	WriteQuad(4, 5, 6, 7);

	WriteVert(BoxVerts[5], BoxNormals[2], BoxTangents[2], FVector2f(0.0f, 0.0f));
	WriteVert(BoxVerts[1], BoxNormals[2], BoxTangents[2], FVector2f(0.0f, 1.0f));
	WriteVert(BoxVerts[0], BoxNormals[2], BoxTangents[2], FVector2f(1.0f, 1.0f));
	WriteVert(BoxVerts[4], BoxNormals[2], BoxTangents[2], FVector2f(1.0f, 0.0f));
	WriteQuad(8, 9, 10, 11);

	WriteVert(BoxVerts[6], BoxNormals[3], BoxTangents[3], FVector2f(0.0f, 0.0f));
	WriteVert(BoxVerts[2], BoxNormals[3], BoxTangents[3], FVector2f(0.0f, 1.0f));
	WriteVert(BoxVerts[1], BoxNormals[3], BoxTangents[3], FVector2f(1.0f, 1.0f));
	WriteVert(BoxVerts[5], BoxNormals[3], BoxTangents[3], FVector2f(1.0f, 0.0f));
	WriteQuad(12, 13, 14, 15);

	WriteVert(BoxVerts[7], BoxNormals[4], BoxTangents[4], FVector2f(0.0f, 0.0f));
	WriteVert(BoxVerts[3], BoxNormals[4], BoxTangents[4], FVector2f(0.0f, 1.0f));
	WriteVert(BoxVerts[2], BoxNormals[4], BoxTangents[4], FVector2f(1.0f, 1.0f));
	WriteVert(BoxVerts[6], BoxNormals[4], BoxTangents[4], FVector2f(1.0f, 0.0f));
	WriteQuad(16, 17, 18, 19);

	WriteVert(BoxVerts[7], BoxNormals[5], BoxTangents[5], FVector2f(0.0f, 0.0f));
	WriteVert(BoxVerts[6], BoxNormals[5], BoxTangents[5], FVector2f(0.0f, 1.0f));
	WriteVert(BoxVerts[5], BoxNormals[5], BoxTangents[5], FVector2f(1.0f, 1.0f));
	WriteVert(BoxVerts[4], BoxNormals[5], BoxTangents[5], FVector2f(1.0f, 0.0f));
	WriteQuad(20, 21, 22, 23);
}

void URealtimeMeshBasicShapeTools::AppendMesh(FRealtimeMeshStreamSet& TargetMeshData, const FRealtimeMeshStreamSet& MeshDataToAdd, const FTransform3f& Transform, bool bSkipMissingStreams)
{
	const FRealtimeMeshStream* SourcePositionStream = MeshDataToAdd.Find(FRealtimeMeshStreams::Position);
	const FRealtimeMeshStream* SourceTriangleStream = MeshDataToAdd.Find(FRealtimeMeshStreams::Triangles);
	
	if (!SourcePositionStream || !SourceTriangleStream)
	{
		UE_LOG(LogRealtimeMesh, Warning, TEXT("Unable to AppendMesh, MeshDataToAdd does not contain a position and triangle stream"));
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
		// The transformed path reinterprets the source positions as FVector3f; guard against
		// a double-precision (FVector3d) position stream being silently misread.
		check(SourcePositionStream->IsOfType<FVector3f>());

		TargetPositionStream.AppendGenerated<FVector3f>(NumVerticesToAdd, [&](int32 Index)
		{
			return Transform.TransformPosition(*SourcePositionStream->GetDataAtVertex<FVector3f>(Index));
		});
	}

	
	auto AppendStream = [&TargetMeshData, &MeshDataToAdd, bSkipMissingStreams](int32 StartIndex, int32 NumToAdd, const FRealtimeMeshStreamKey& StreamKey, const FRealtimeMeshBufferLayout& DefaultLayout)
	{
		if (MeshDataToAdd.Contains(StreamKey) && (!bSkipMissingStreams || TargetMeshData.Contains(StreamKey)))
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
	if (NumCopiedTangents > 0 && !Transform.Equals(FTransform3f::Identity))
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

URealtimeMeshStreamSet* URealtimeMeshBasicShapeTools::AppendBoxMesh(URealtimeMeshStreamSet* StreamSet, FVector BoxRadius, FTransform BoxTransform, int32 NewMaterialGroup, FLinearColor Color)
{
	if (IsValid(StreamSet))
	{
		AppendBoxMesh(StreamSet->GetStreamSet(), FVector3f(BoxRadius), FTransform3f(BoxTransform), NewMaterialGroup, Color.ToFColor(false));
	}
	return StreamSet;
}
