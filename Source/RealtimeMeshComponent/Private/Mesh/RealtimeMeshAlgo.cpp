// Copyright (c) 2015-2024 TriAxis Games, L.L.C. All Rights Reserved.


#include "Mesh/RealtimeMeshAlgo.h"

#include "Mesh/RealtimeMeshBuilder.h"
#include "Mesh/RealtimeMeshDataStream.h"
#include "Mesh/RealtimeMeshDataTypes.h"

using namespace RealtimeMesh;

bool RealtimeMeshAlgo::GenerateSortedRemapTable(const FRealtimeMeshStream& PolygonGroups, TArrayView<uint32> OutRemapTable)
{
	if (PolygonGroups.GetLayout().GetElementType() == GetRealtimeMeshDataElementType<uint16>())
	{
		GenerateSortedRemapTable(PolygonGroups.GetElementArrayView<uint16>(), OutRemapTable);
		return true;
	}
	if (PolygonGroups.GetLayout().GetElementType() == GetRealtimeMeshDataElementType<int16>())
	{
		GenerateSortedRemapTable(PolygonGroups.GetElementArrayView<int16>(), OutRemapTable);
		return true;
	}
	if (PolygonGroups.GetLayout().GetElementType() == GetRealtimeMeshDataElementType<uint32>())
	{
		GenerateSortedRemapTable(PolygonGroups.GetElementArrayView<uint32>(), OutRemapTable);
		return true;
	}
	if (PolygonGroups.GetLayout().GetElementType() == GetRealtimeMeshDataElementType<int32>())
	{
		GenerateSortedRemapTable(PolygonGroups.GetElementArrayView<int32>(), OutRemapTable);
		return true;
	}

	return false;
}

void RealtimeMeshAlgo::ApplyRemapTableToStream(TArrayView<uint32> RemapTable, FRealtimeMeshStream& Stream)
{
	check(RemapTable.Num() == Stream.Num());
	FRealtimeMeshStream NewData(Stream.GetStreamKey(), Stream.GetLayout());
	NewData.SetNumUninitialized(Stream.Num());

	for (int32 Index = 0; Index < RemapTable.Num(); Index++)
	{
		const int32 OldIndex = RemapTable[Index];
		FMemory::Memcpy(NewData.GetData() + Index * Stream.GetStride(), Stream.GetData() + OldIndex * Stream.GetStride(), Stream.GetStride());
	}

	Stream = MoveTemp(NewData);
}

bool RealtimeMeshAlgo::OrganizeTrianglesByPolygonGroup(FRealtimeMeshStream& IndexStream, FRealtimeMeshStream& PolygonGroupStream,
                                                       TArrayView<uint32> OutRemapTable)
{
	// Make sure triangle count and polygon group indices length are the same
	if ((IndexStream.Num() * IndexStream.GetNumElements() / 3) != PolygonGroupStream.Num())
	{
		return false;
	}

	// Make sure the remap table is the same length as the polygon group stream
	if (OutRemapTable.Num() != PolygonGroupStream.Num())
	{
		return false;
	}

	if (GenerateSortedRemapTable(PolygonGroupStream, OutRemapTable))
	{
		ApplyRemapTableToStream(OutRemapTable, IndexStream);
		ApplyRemapTableToStream(OutRemapTable, PolygonGroupStream);
		return true;
	}
	return false;
}

bool RealtimeMeshAlgo::OrganizeTrianglesByPolygonGroup(FRealtimeMeshStreamSet& InStreamSet, const FRealtimeMeshStreamKey& IndexStreamKey,
                                                       const FRealtimeMeshStreamKey& PolygonGroupStreamKey, TArray<uint32>* OutRemapTable)
{
	FRealtimeMeshStream* IndexStream = InStreamSet.Find(IndexStreamKey);
	FRealtimeMeshStream* PolygonGroupStream = InStreamSet.Find(PolygonGroupStreamKey);

	// Do we have both streams?
	if (!IndexStream || !PolygonGroupStream)
	{
		return false;
	}

	TArray<uint32> Temp;
	TArray<uint32>* RemapTable = OutRemapTable ? OutRemapTable : &Temp;
	RemapTable->SetNumUninitialized(PolygonGroupStream->Num());

	return OrganizeTrianglesByPolygonGroup(*IndexStream, *PolygonGroupStream, *RemapTable);
}

void RealtimeMeshAlgo::PropagateTriangleSegmentsToPolygonGroups(TArrayView<FRealtimeMeshPolygonGroupRange> TriangleSegments, FRealtimeMeshStream& OutPolygonGroupIndices)
{
	if (OutPolygonGroupIndices.GetLayout().GetElementType() == GetRealtimeMeshDataElementType<uint16>())
	{
		TArrayView<uint16> View = OutPolygonGroupIndices.GetArrayView<uint16>();
		PropagateTriangleSegmentsToPolygonGroups(TriangleSegments, View);
	}
	if (OutPolygonGroupIndices.GetLayout().GetElementType() == GetRealtimeMeshDataElementType<int16>())
	{
		TArrayView<int16> View = OutPolygonGroupIndices.GetArrayView<int16>();
		PropagateTriangleSegmentsToPolygonGroups(TriangleSegments, View);
	}
	if (OutPolygonGroupIndices.GetLayout().GetElementType() == GetRealtimeMeshDataElementType<uint32>())
	{
		TArrayView<uint32> View = OutPolygonGroupIndices.GetArrayView<uint32>();
		PropagateTriangleSegmentsToPolygonGroups(TriangleSegments, View);
	}
	if (OutPolygonGroupIndices.GetLayout().GetElementType() == GetRealtimeMeshDataElementType<int32>())
	{
		TArrayView<int32> View = OutPolygonGroupIndices.GetArrayView<int32>();
		PropagateTriangleSegmentsToPolygonGroups(TriangleSegments, View);
	}
}

bool RealtimeMeshAlgo::ArePolygonGroupIndicesOptimal(TConstArrayView<const FRealtimeMeshPolygonGroupRange> TriangleSegments)
{
	TSet<int32> UniquePolygonGroupIndices;
	for (int32 Index = 0; Index < TriangleSegments.Num(); Index++)
	{
		bool bAlreadyContains;
		UniquePolygonGroupIndices.Add(TriangleSegments[Index].PolygonGroupIndex, &bAlreadyContains);

		if (bAlreadyContains)
		{
			return false;
		}
	}

	return true;
}

bool RealtimeMeshAlgo::ArePolygonGroupIndicesOptimal(const FRealtimeMeshStream& PolygonGroupIndices)
{
	if (PolygonGroupIndices.GetLayout().GetElementType() == GetRealtimeMeshDataElementType<uint16>())
	{
		return ArePolygonGroupIndicesOptimal(PolygonGroupIndices.GetElementArrayView<uint16>());
	}
	if (PolygonGroupIndices.GetLayout().GetElementType() == GetRealtimeMeshDataElementType<int16>())
	{
		return ArePolygonGroupIndicesOptimal(PolygonGroupIndices.GetElementArrayView<int16>());
	}
	if (PolygonGroupIndices.GetLayout().GetElementType() == GetRealtimeMeshDataElementType<uint32>())
	{
		return ArePolygonGroupIndicesOptimal(PolygonGroupIndices.GetElementArrayView<uint32>());
	}
	if (PolygonGroupIndices.GetLayout().GetElementType() == GetRealtimeMeshDataElementType<int32>())
	{
		return ArePolygonGroupIndicesOptimal(PolygonGroupIndices.GetElementArrayView<int32>());
	}

	checkf(false, TEXT("Unsupported format for PolygonGroupIndices"));
	return false;
}

void RealtimeMeshAlgo::GatherSegmentsFromPolygonGroupIndices(const FRealtimeMeshStream& PolygonGroupIndices,
                                                             const TFunctionRef<void(const FRealtimeMeshPolygonGroupRange& NewSegment)>& OnAddNewSegmentFunction)
{
	if (PolygonGroupIndices.GetLayout().GetElementType() == GetRealtimeMeshDataElementType<uint16>())
	{
		GatherSegmentsFromPolygonGroupIndices(PolygonGroupIndices.GetElementArrayView<uint16>(), OnAddNewSegmentFunction);
	}
	else if (PolygonGroupIndices.GetLayout().GetElementType() == GetRealtimeMeshDataElementType<int16>())
	{
		GatherSegmentsFromPolygonGroupIndices(PolygonGroupIndices.GetElementArrayView<int16>(), OnAddNewSegmentFunction);
	}
	else if (PolygonGroupIndices.GetLayout().GetElementType() == GetRealtimeMeshDataElementType<uint32>())
	{
		GatherSegmentsFromPolygonGroupIndices(PolygonGroupIndices.GetElementArrayView<uint32>(), OnAddNewSegmentFunction);
	}
	else if (PolygonGroupIndices.GetLayout().GetElementType() == GetRealtimeMeshDataElementType<int32>())
	{
		GatherSegmentsFromPolygonGroupIndices(PolygonGroupIndices.GetElementArrayView<int32>(), OnAddNewSegmentFunction);
	}
	else
	{
		checkf(false, TEXT("Unsupported format for PolygonGroupIndices"));
	}
}

void RealtimeMeshAlgo::GatherSegmentsFromPolygonGroupIndices(const FRealtimeMeshStream& PolygonGroupIndices, FRealtimeMeshStream& OutSegments)
{
	TRealtimeMeshStreamBuilder<FRealtimeMeshPolygonGroupRange> SegmentsBuilder(OutSegments);
	SegmentsBuilder.Empty();

	GatherSegmentsFromPolygonGroupIndices(PolygonGroupIndices, [&SegmentsBuilder](const FRealtimeMeshPolygonGroupRange& NewSegment)
	{
		SegmentsBuilder.Add(NewSegment);
	});
}

void RealtimeMeshAlgo::GatherStreamRangesFromPolyGroupRanges(TConstArrayView<const FRealtimeMeshPolygonGroupRange> PolygonGroupSegments,
                                                             const FRealtimeMeshStream& Triangles, TMap<int32, FRealtimeMeshStreamRange>& OutStreamRanges)
{
	if (Triangles.GetLayout().GetElementType() == GetRealtimeMeshDataElementType<uint16>())
	{
		GatherStreamRangesFromPolyGroupRanges(PolygonGroupSegments, Triangles.GetElementArrayView<uint16>(), OutStreamRanges);
	}
	else if (Triangles.GetLayout().GetElementType() == GetRealtimeMeshDataElementType<int16>())
	{
		GatherStreamRangesFromPolyGroupRanges(PolygonGroupSegments, Triangles.GetElementArrayView<int16>(), OutStreamRanges);
	}
	else if (Triangles.GetLayout().GetElementType() == GetRealtimeMeshDataElementType<uint32>())
	{
		GatherStreamRangesFromPolyGroupRanges(PolygonGroupSegments, Triangles.GetElementArrayView<uint32>(), OutStreamRanges);
	}
	else if (Triangles.GetLayout().GetElementType() == GetRealtimeMeshDataElementType<int32>())
	{
		GatherStreamRangesFromPolyGroupRanges(PolygonGroupSegments, Triangles.GetElementArrayView<int32>(), OutStreamRanges);
	}
	else
	{
		checkf(false, TEXT("Unsupported format for PolygonGroupIndices"));
	}
}

void RealtimeMeshAlgo::GatherStreamRangesFromPolyGroupRanges(const FRealtimeMeshStream& PolygonGroupSegments,
                                                             const FRealtimeMeshStream& Triangles, TMap<int32, FRealtimeMeshStreamRange>& OutStreamRanges)
{
	const TConstArrayView<const FRealtimeMeshPolygonGroupRange> PolyGroupRanges = PolygonGroupSegments.GetArrayView<FRealtimeMeshPolygonGroupRange>();
	GatherStreamRangesFromPolyGroupRanges(PolyGroupRanges, Triangles, OutStreamRanges);
}

void RealtimeMeshAlgo::GatherStreamRangesFromPolyGroupIndices(const FRealtimeMeshStream& PolygonGroupIndices, const FRealtimeMeshStream& Indices,
                                                              TMap<int32, FRealtimeMeshStreamRange>& OutStreamRanges)
{
	if (PolygonGroupIndices.GetLayout().GetElementType() == RealtimeMesh::GetRealtimeMeshDataElementType<uint16>())
	{
		GatherStreamRangesFromPolyGroupIndices(PolygonGroupIndices.GetElementArrayView<uint16>(), Indices, OutStreamRanges);
	}
	else if (PolygonGroupIndices.GetLayout().GetElementType() == RealtimeMesh::GetRealtimeMeshDataElementType<int16>())
	{
		GatherStreamRangesFromPolyGroupIndices(PolygonGroupIndices.GetElementArrayView<int16>(), Indices, OutStreamRanges);
	}
	else if (PolygonGroupIndices.GetLayout().GetElementType() == RealtimeMesh::GetRealtimeMeshDataElementType<uint32>())
	{
		GatherStreamRangesFromPolyGroupIndices(PolygonGroupIndices.GetElementArrayView<uint32>(), Indices, OutStreamRanges);
	}
	else if (PolygonGroupIndices.GetLayout().GetElementType() == RealtimeMesh::GetRealtimeMeshDataElementType<int32>())
	{
		GatherStreamRangesFromPolyGroupIndices(PolygonGroupIndices.GetElementArrayView<int32>(), Indices, OutStreamRanges);
	}
	else
	{
		checkf(false, TEXT("Unsupported format for PolygonGroupIndices"));
	}
}

TOptional<TMap<int32, FRealtimeMeshStreamRange>> RealtimeMeshAlgo::GetStreamRangesFromPolyGroups(const FRealtimeMeshStreamSet& Streams,
                                                                                                 const FRealtimeMeshStreamKey& TrianglesKey,
                                                                                                 const FRealtimeMeshStreamKey& PolyGroupsKey,
                                                                                                 const FRealtimeMeshStreamKey& PolyGroupSegmentsKey)
{
	if (const auto Triangles = Streams.Find(TrianglesKey))
	{
		if (const auto PolyGroupSegments = Streams.Find(PolyGroupSegmentsKey))
		{
			TMap<int32, FRealtimeMeshStreamRange> Ranges;
			RealtimeMeshAlgo::GatherStreamRangesFromPolyGroupRanges(*PolyGroupSegments, *Triangles, Ranges);
			return MoveTemp(Ranges);
		}

		if (const auto PolyGroupIndices = Streams.Find(PolyGroupsKey))
		{
			TMap<int32, FRealtimeMeshStreamRange> Ranges;
			RealtimeMeshAlgo::GatherStreamRangesFromPolyGroupIndices(*PolyGroupIndices, *Triangles, Ranges);
			return MoveTemp(Ranges);
		}
	}

	return TOptional<TMap<int32, FRealtimeMeshStreamRange>>();
}

TOptional<TMap<int32, FRealtimeMeshStreamRange>> RealtimeMeshAlgo::GetStreamRangesFromPolyGroupsDepthOnly(const FRealtimeMeshStreamSet& Streams)
{
	return GetStreamRangesFromPolyGroups(Streams, FRealtimeMeshStreams::DepthOnlyTriangles, FRealtimeMeshStreams::DepthOnlyPolyGroups,
	                                     FRealtimeMeshStreams::DepthOnlyPolyGroupSegments);
}


void RealtimeMeshAlgo::GenerateTangents(RealtimeMesh::FRealtimeMeshStreamSet& StreamSet, bool bComputeSmoothNormals)
{
	if (!StreamSet.Contains(FRealtimeMeshStreams::Triangles) || !StreamSet.Contains(FRealtimeMeshStreams::Position))
	{
		return;
	}

	TRealtimeMeshStreamBuilder<FVector3f> Positions(StreamSet.FindChecked(FRealtimeMeshStreams::Position));
	TRealtimeMeshStreamBuilder<TIndex3<uint32>> Triangles(StreamSet.FindChecked(FRealtimeMeshStreams::Triangles));
	TOptional<TRealtimeMeshStridedStreamBuilder<FVector2f, void>> TexCoords;

	if (StreamSet.Contains(FRealtimeMeshStreams::TexCoords))
	{
		TexCoords = TRealtimeMeshStridedStreamBuilder<FVector2f, void>(StreamSet.FindChecked(FRealtimeMeshStreams::TexCoords));
	}

	StreamSet.Remove(FRealtimeMeshStreams::Tangents);
	TRealtimeMeshStreamBuilder<FRealtimeMeshTangentsNormalPrecision> Tangents(StreamSet.AddStream<FRealtimeMeshTangentsNormalPrecision>(FRealtimeMeshStreams::Tangents));
	Tangents.SetNumZeroed(Positions.Num());
	
	uint32 NumIndices = Triangles.Num() * 3;
	uint32 NumVertices = Positions.Num();

	// Calculate the duplicate vertices map if we're wanting smooth normals.  Don't find duplicates if we don't want smooth normals
	// that will cause it to only smooth across faces sharing a common vertex, not across faces with vertices of common position
	TMultiMap<uint32, uint32> DuplicateVertexMap;

	if (bComputeSmoothNormals)
	{
		using namespace Private;

		TArray<FRealtimeMeshVertexSortElement> VertexSorter;
		VertexSorter.Empty(NumVertices);
		for (uint32 Index = 0; Index < NumVertices; Index++)
		{
			new(VertexSorter)FRealtimeMeshVertexSortElement(Index, Positions[Index]);
		}

		// Sort the list
		VertexSorter.Sort(FRuntimeMeshVertexSortingFunction());

		// Map out the duplicates.
		for (uint32 Index = 0; Index < NumVertices; Index++)
		{
			uint32 SrcVertIdx = VertexSorter[Index].Index;
			float Value = VertexSorter[Index].Value;

			// Search forward adding pairs both ways
			for (uint32 SubIndex = Index + 1; SubIndex < NumVertices; SubIndex++)
			{
				if (FMath::Abs(VertexSorter[SubIndex].Value - Value) > THRESH_POINTS_ARE_SAME * 4.01f)
				{
					// No more possible duplicates
					break;
				}

				uint32 OtherVertIdx = VertexSorter[SubIndex].Index;
				if (Positions.GetValue(SrcVertIdx).Equals(Positions.GetValue(OtherVertIdx)))
				{
					DuplicateVertexMap.AddUnique(SrcVertIdx, OtherVertIdx);
					DuplicateVertexMap.AddUnique(OtherVertIdx, SrcVertIdx);
				}
			}
		}
	}

	// Number of triangles
	const uint32 NumTris = NumIndices / 3;

	// Map of vertex to triangles in Triangles array
	TMultiMap<uint32, uint32> VertToTriMap;
	// Map of vertex to triangles to consider for normal calculation
	TMultiMap<uint32, uint32> VertToTriSmoothMap;

	// Normal/tangents for each face
	TArray<FVector3f> FaceTangentX, FaceTangentY, FaceTangentZ;
	FaceTangentX.AddUninitialized(NumTris);
	FaceTangentY.AddUninitialized(NumTris);
	FaceTangentZ.AddUninitialized(NumTris);

	// Iterate over triangles
	for (uint32 TriIdx = 0; TriIdx < NumTris; TriIdx++)
	{
		uint32 CornerIndex[3];
		FVector3f P[3];

		for (int32 CornerIdx = 0; CornerIdx < 3; CornerIdx++)
		{
			// Find vert index (clamped within range)
			uint32 VertIndex = FMath::Min(Triangles.GetElementValue(TriIdx, CornerIdx), NumVertices - 1);

			CornerIndex[CornerIdx] = VertIndex;
			P[CornerIdx] = Positions.GetValue(VertIndex);

			// Find/add this vert to index buffer
			TArray<uint32> VertOverlaps;
			DuplicateVertexMap.MultiFind(VertIndex, VertOverlaps);

			// Remember which triangles map to this vert
			VertToTriMap.AddUnique(VertIndex, TriIdx);
			VertToTriSmoothMap.AddUnique(VertIndex, TriIdx);

			// Also update map of triangles that 'overlap' this vert (ie don't match UV, but do match smoothing) and should be considered when calculating normal
			for (int32 OverlapIdx = 0; OverlapIdx < VertOverlaps.Num(); OverlapIdx++)
			{
				// For each vert we overlap..
				int32 OverlapVertIdx = VertOverlaps[OverlapIdx];

				// Add this triangle to that vert
				VertToTriSmoothMap.AddUnique(OverlapVertIdx, TriIdx);

				// And add all of its triangles to us
				TArray<uint32> OverlapTris;
				VertToTriMap.MultiFind(OverlapVertIdx, OverlapTris);
				for (int32 OverlapTriIdx = 0; OverlapTriIdx < OverlapTris.Num(); OverlapTriIdx++)
				{
					VertToTriSmoothMap.AddUnique(VertIndex, OverlapTris[OverlapTriIdx]);
				}
			}
		}

		// Calculate triangle edge vectors and normal
		const FVector3f Edge21 = P[1] - P[2];
		const FVector3f Edge20 = P[0] - P[2];
		const FVector3f TriNormal = (Edge21 ^ Edge20).GetSafeNormal();

		// If we have UVs, use those to calculate
		if (TexCoords.IsSet())
		{
			const FVector2f T1 = TexCoords->GetValue(CornerIndex[0]);
			const FVector2f T2 = TexCoords->GetValue(CornerIndex[1]);
			const FVector2f T3 = TexCoords->GetValue(CornerIndex[2]);

			// 			float X1 = P[1].X - P[0].X;
			// 			float X2 = P[2].X - P[0].X;
			// 			float Y1 = P[1].Y - P[0].Y;
			// 			float Y2 = P[2].Y - P[0].Y;
			// 			float Z1 = P[1].Z - P[0].Z;
			// 			float Z2 = P[2].Z - P[0].Z;
			// 
			// 			float S1 = U1.X - U0.X;
			// 			float S2 = U2.X - U0.X;
			// 			float T1 = U1.Y - U0.Y;
			// 			float T2 = U2.Y - U0.Y;
			// 
			// 			float R = 1.0f / (S1 * T2 - S2 * T1);
			// 			FaceTangentX[TriIdx] = FVector((T2 * X1 - T1 * X2) * R, (T2 * Y1 - T1 * Y2) * R,
			// 				(T2 * Z1 - T1 * Z2) * R);
			// 			FaceTangentY[TriIdx] = FVector((S1 * X2 - S2 * X1) * R, (S1 * Y2 - S2 * Y1) * R,
			// 				(S1 * Z2 - S2 * Z1) * R);


			FMatrix44f ParameterToLocal(
				FPlane4f(P[1].X - P[0].X, P[1].Y - P[0].Y, P[1].Z - P[0].Z, 0),
				FPlane4f(P[2].X - P[0].X, P[2].Y - P[0].Y, P[2].Z - P[0].Z, 0),
				FPlane4f(P[0].X, P[0].Y, P[0].Z, 0),
				FPlane4f(0, 0, 0, 1)
			);

			FMatrix44f ParameterToTexture(
				FPlane4f(T2.X - T1.X, T2.Y - T1.Y, 0, 0),
				FPlane4f(T3.X - T1.X, T3.Y - T1.Y, 0, 0),
				FPlane4f(T1.X, T1.Y, 1, 0),
				FPlane4f(0, 0, 0, 1)
			);

			// Use InverseSlow to catch singular matrices.  Inverse can miss this sometimes.
			const FMatrix44f TextureToLocal = ParameterToTexture.Inverse() * ParameterToLocal;

			FaceTangentX[TriIdx] = TextureToLocal.TransformVector(FVector3f(1, 0, 0)).GetSafeNormal();
			FaceTangentY[TriIdx] = TextureToLocal.TransformVector(FVector3f(0, 1, 0)).GetSafeNormal();
		}
		else
		{
			FaceTangentX[TriIdx] = Edge20.GetSafeNormal();
			FaceTangentY[TriIdx] = (FaceTangentX[TriIdx] ^ TriNormal).GetSafeNormal();
		}

		FaceTangentZ[TriIdx] = TriNormal;
	}


	// Arrays to accumulate tangents into
	TArray<FVector3f> VertexTangentXSum, VertexTangentYSum, VertexTangentZSum;
	VertexTangentXSum.AddZeroed(NumVertices);
	VertexTangentYSum.AddZeroed(NumVertices);
	VertexTangentZSum.AddZeroed(NumVertices);

	// For each vertex..
	for (uint32 VertxIdx = 0; VertxIdx < NumVertices; VertxIdx++)
	{
		// Find relevant triangles for normal
		TArray<uint32> SmoothTris;
		VertToTriSmoothMap.MultiFind(VertxIdx, SmoothTris);

		for (int i = 0; i < SmoothTris.Num(); i++)
		{
			uint32 TriIdx = SmoothTris[i];
			VertexTangentZSum[VertxIdx] += FaceTangentZ[TriIdx];
		}

		// Find relevant triangles for tangents
		TArray<uint32> TangentTris;
		VertToTriMap.MultiFind(VertxIdx, TangentTris);

		for (int i = 0; i < TangentTris.Num(); i++)
		{
			uint32 TriIdx = TangentTris[i];
			VertexTangentXSum[VertxIdx] += FaceTangentX[TriIdx];
			VertexTangentYSum[VertxIdx] += FaceTangentY[TriIdx];
		}
	}

	// Finally, normalize tangents and build output arrays	
	for (uint32 VertxIdx = 0; VertxIdx < NumVertices; VertxIdx++)
	{
		FVector3f& TangentX = VertexTangentXSum[VertxIdx];
		FVector3f& TangentY = VertexTangentYSum[VertxIdx];
		FVector3f& TangentZ = VertexTangentZSum[VertxIdx];

		TangentX.Normalize();
		//TangentY.Normalize();
		TangentZ.Normalize();

		// Use Gram-Schmidt orthogonalization to make sure X is orthonormal with Z
		TangentX -= TangentZ * (TangentZ | TangentX);
		TangentX.Normalize();
		TangentY.Normalize();

		Tangents.Set(VertxIdx, FRealtimeMeshTangentsNormalPrecision(TangentZ, TangentY, TangentX));
	}
}
