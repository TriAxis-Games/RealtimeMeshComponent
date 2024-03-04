// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "RealtimeMeshDataStream.h"
#include "RealtimeMeshDataTypes.h"
#include "RealtimeMeshConfig.h"
#include "Algo/StableSort.h"

struct FRealtimeMeshPolygonGroupRange;
struct FRealtimeMeshStreamKey;

namespace RealtimeMesh
{
	struct FRealtimeMeshStream;
	struct FRealtimeMeshStreamSet;
}

namespace RealtimeMeshAlgo
{
	namespace Private
	{
		struct FRealtimeMeshVertexSortElement
		{
			float Value;
			uint32 Index;

			FRealtimeMeshVertexSortElement()
				: Value(0.0f)
				  , Index(INDEX_NONE)
			{
			}

			FRealtimeMeshVertexSortElement(uint32 InIndex, const FVector3f& InVector)
			{
				Value = 0.30f * InVector.X + 0.33f * InVector.Y + 0.37f * InVector.Z;
				Index = InIndex;
			}
		};

		struct FRuntimeMeshVertexSortingFunction
		{
			FORCEINLINE bool operator()(FRealtimeMeshVertexSortElement const& Left, FRealtimeMeshVertexSortElement const& Right) const
			{
				return Left.Value < Right.Value;
			}
		};
	}


	template <typename PolygonGroupType>
	void GenerateSortedRemapTable(TConstArrayView<const PolygonGroupType> PolygonGroups, TArrayView<uint32> OutRemapTable)
	{
		check(PolygonGroups.Num() == OutRemapTable.Num());

		// Fill with starting data 0...N
		for (int32 Index = 0; Index < OutRemapTable.Num(); Index++)
		{
			OutRemapTable[Index] = Index;
		}

		// Run stable sort on the remap table, using the polygon group indices as the sorting index
		Algo::StableSortBy(OutRemapTable, [&PolygonGroups](int32 Index)
		{
			return PolygonGroups[Index];
		});
	}

	REALTIMEMESHCOMPONENT_API bool GenerateSortedRemapTable(const RealtimeMesh::FRealtimeMeshStream& PolygonGroups, TArrayView<uint32> OutRemapTable);

	REALTIMEMESHCOMPONENT_API void ApplyRemapTableToStream(TArrayView<uint32> RemapTable, RealtimeMesh::FRealtimeMeshStream& Stream);

	REALTIMEMESHCOMPONENT_API bool OrganizeTrianglesByPolygonGroup(RealtimeMesh::FRealtimeMeshStream& IndexStream, RealtimeMesh::FRealtimeMeshStream& PolygonGroupStream,
	                                                               TArrayView<uint32> OutRemapTable);

	REALTIMEMESHCOMPONENT_API bool OrganizeTrianglesByPolygonGroup(RealtimeMesh::FRealtimeMeshStreamSet& InStreamSet, const FRealtimeMeshStreamKey& IndexStreamKey,
	                                                               const FRealtimeMeshStreamKey& PolygonGroupStreamKey, TArray<uint32>* OutRemapTable = nullptr);


	template <typename PolygonGroupType>
	void PropagateTriangleSegmentsToPolygonGroups(TConstArrayView<const FRealtimeMeshPolygonGroupRange> TriangleSegments, TArrayView<PolygonGroupType>& OutPolygonGroupIndices)
	{
		for (int32 Index = 0; Index < TriangleSegments.Num(); Index++)
		{
			const FRealtimeMeshPolygonGroupRange& Segment = TriangleSegments[Index];

			for (int32 TriIdx = Segment.StartIndex; TriIdx < Segment.StartIndex + Segment.Count && TriIdx < OutPolygonGroupIndices.Num(); TriIdx++)
			{
				OutPolygonGroupIndices[TriIdx] = Segment.PolygonGroupIndex;
			}
		}
	}

	REALTIMEMESHCOMPONENT_API void PropagateTriangleSegmentsToPolygonGroups(TArrayView<FRealtimeMeshPolygonGroupRange> TriangleSegments,
	                                                                        RealtimeMesh::FRealtimeMeshStream& OutPolygonGroupIndices);

	/**
	 * @brief Checks if the polygon group indices would create disconnected segments to the same polygon group
	 * @tparam PolygonGroupType 
	 * @param PolygonGroupIndices 
	 * @return 
	 */
	template <typename PolygonGroupType>
	typename TEnableIf<!std::is_same<PolygonGroupType, FRealtimeMeshPolygonGroupRange>::value, bool>::Type ArePolygonGroupIndicesOptimal(
		TConstArrayView<const PolygonGroupType> PolygonGroupIndices)
	{
		TSet<PolygonGroupType> UniquePolygonGroupIndices;
		PolygonGroupType LastPolygonGroupIndex = INDEX_NONE;
		for (int32 Index = 0; Index < PolygonGroupIndices.Num(); Index++)
		{
			const auto& MatIdx = PolygonGroupIndices[Index];

			if (MatIdx != LastPolygonGroupIndex)
			{
				if (UniquePolygonGroupIndices.Contains(MatIdx))
				{
					return false;
				}
				UniquePolygonGroupIndices.Add(MatIdx);
				LastPolygonGroupIndex = MatIdx;
			}
		}

		return true;
	}

	REALTIMEMESHCOMPONENT_API bool ArePolygonGroupIndicesOptimal(TConstArrayView<const FRealtimeMeshPolygonGroupRange> TriangleSegments);

	REALTIMEMESHCOMPONENT_API bool ArePolygonGroupIndicesOptimal(const RealtimeMesh::FRealtimeMeshStream& PolygonGroupIndices);


	template <typename PolygonGroupType>
	void GatherSegmentsFromPolygonGroupIndices(TConstArrayView<const PolygonGroupType> PolygonGroupIndices,
	                                           const TFunctionRef<void(const FRealtimeMeshPolygonGroupRange& NewSegment)>& OnAddNewSegmentFunction)
	{
		if (PolygonGroupIndices.Num() < 1)
		{
			return;
		}

		FRealtimeMeshPolygonGroupRange NextSegment;
		NextSegment.PolygonGroupIndex = PolygonGroupIndices[0];
		NextSegment.StartIndex = 0;
		for (int32 Index = 1; Index < PolygonGroupIndices.Num(); Index++)
		{
			const int32 CurrentMatIndex = PolygonGroupIndices[Index];

			if (CurrentMatIndex != NextSegment.PolygonGroupIndex)
			{
				NextSegment.Count = Index - NextSegment.StartIndex;
				OnAddNewSegmentFunction(NextSegment);
				NextSegment.PolygonGroupIndex = CurrentMatIndex;
				NextSegment.StartIndex = Index;
			}
		}
		NextSegment.Count = PolygonGroupIndices.Num() - NextSegment.StartIndex;
		OnAddNewSegmentFunction(NextSegment);
	}

	REALTIMEMESHCOMPONENT_API void GatherSegmentsFromPolygonGroupIndices(const RealtimeMesh::FRealtimeMeshStream& PolygonGroupIndices,
	                                                                     const TFunctionRef<void(const FRealtimeMeshPolygonGroupRange& NewSegment)>& OnAddNewSegmentFunction);

	REALTIMEMESHCOMPONENT_API void GatherSegmentsFromPolygonGroupIndices(const RealtimeMesh::FRealtimeMeshStream& PolygonGroupIndices,
	                                                                     RealtimeMesh::FRealtimeMeshStream& OutSegments);


	template <typename IndexType>
	void GatherStreamRangesFromPolyGroupRanges(TConstArrayView<const FRealtimeMeshPolygonGroupRange> PolygonGroupSegments, TConstArrayView<const IndexType> Indices,
	                                           TMap<int32, FRealtimeMeshStreamRange>& OutStreamRanges)
	{
		for (const auto& PolyGroup : PolygonGroupSegments)
		{
			// Bail if this segment has no triangles
			if (PolyGroup.Count < 1)
			{
				continue;
			}

			int32 MinVertexIndex = Indices[PolyGroup.StartIndex];
			int32 MaxVertexIndex = MinVertexIndex;
			const int32 MinTriangleIndex = PolyGroup.StartIndex;
			const int32 MaxTriangleIndex = PolyGroup.StartIndex + PolyGroup.Count;

			for (int32 Index = MinTriangleIndex; Index <= MaxTriangleIndex; Index++)
			{
				MinVertexIndex = FMath::Min<IndexType>(MinVertexIndex, Indices[Index * 3 + 0]);
				MinVertexIndex = FMath::Min<IndexType>(MinVertexIndex, Indices[Index * 3 + 1]);
				MinVertexIndex = FMath::Min<IndexType>(MinVertexIndex, Indices[Index * 3 + 2]);

				MaxVertexIndex = FMath::Max<IndexType>(MaxVertexIndex, Indices[Index * 3 + 0]);
				MaxVertexIndex = FMath::Max<IndexType>(MaxVertexIndex, Indices[Index * 3 + 1]);
				MaxVertexIndex = FMath::Max<IndexType>(MaxVertexIndex, Indices[Index * 3 + 2]);
			}

			if (MaxVertexIndex != MinVertexIndex && MaxTriangleIndex != MinTriangleIndex)
			{
				const int32 VertexCount = MaxVertexIndex - MinVertexIndex;
				const int32 TriangleCount = MaxTriangleIndex - MinTriangleIndex;
				OutStreamRanges.Add(PolyGroup.PolygonGroupIndex, FRealtimeMeshStreamRange(MinVertexIndex, VertexCount, MinTriangleIndex, TriangleCount));
			}
		}
	}

	REALTIMEMESHCOMPONENT_API void GatherStreamRangesFromPolyGroupRanges(TConstArrayView<const FRealtimeMeshPolygonGroupRange> PolygonGroupSegments,
	                                                                     const RealtimeMesh::FRealtimeMeshStream& Triangles,
	                                                                     TMap<int32, FRealtimeMeshStreamRange>& OutStreamRanges);

	REALTIMEMESHCOMPONENT_API void GatherStreamRangesFromPolyGroupRanges(const RealtimeMesh::FRealtimeMeshStream& PolygonGroupSegments,
	                                                                     const RealtimeMesh::FRealtimeMeshStream& Triangles,
	                                                                     TMap<int32, FRealtimeMeshStreamRange>& OutStreamRanges);

	template <typename PolygonGroupType, typename IndexType>
	void GatherStreamRangesFromPolyGroupIndices(TConstArrayView<const PolygonGroupType> PolygonGroupIndices, TConstArrayView<const IndexType> Indices,
	                                            TMap<int32, FRealtimeMeshStreamRange>& OutStreamRanges)
	{
		ensure(Indices.Num() >= PolygonGroupIndices.Num() * 3);
		if (PolygonGroupIndices.Num() < 1 || Indices.Num() < 3)
		{
			return;
		}

		const int32 MaxTriangleCount = FMath::Min(PolygonGroupIndices.Num(), Indices.Num() / 3);

		GatherSegmentsFromPolygonGroupIndices(PolygonGroupIndices.Slice(0, MaxTriangleCount), [&](const FRealtimeMeshPolygonGroupRange& PolyGroup)
		{
			if (!OutStreamRanges.Contains(PolyGroup.PolygonGroupIndex))
			{
				int32 MinVertexIndex = Indices[PolyGroup.StartIndex * 3];
				int32 MaxVertexIndex = MinVertexIndex;
				const int32 MinTriangleIndex = PolyGroup.StartIndex;
				const int32 MaxTriangleIndex = PolyGroup.StartIndex + PolyGroup.Count;

				for (int32 Index = 0; Index < PolyGroup.Count; Index++)
				{
					const int32 FinalIndex = (PolyGroup.StartIndex + Index) * 3;
					MinVertexIndex = FMath::Min<IndexType>(MinVertexIndex, Indices[FinalIndex + 0]);
					MinVertexIndex = FMath::Min<IndexType>(MinVertexIndex, Indices[FinalIndex + 1]);
					MinVertexIndex = FMath::Min<IndexType>(MinVertexIndex, Indices[FinalIndex + 2]);

					MaxVertexIndex = FMath::Max<IndexType>(MaxVertexIndex, Indices[FinalIndex + 0]);
					MaxVertexIndex = FMath::Max<IndexType>(MaxVertexIndex, Indices[FinalIndex + 1]);
					MaxVertexIndex = FMath::Max<IndexType>(MaxVertexIndex, Indices[FinalIndex + 2]);
				}

				if (MaxVertexIndex != MinVertexIndex && MaxTriangleIndex != MinTriangleIndex)
				{
					OutStreamRanges.Add(PolyGroup.PolygonGroupIndex, FRealtimeMeshStreamRange(MinVertexIndex, MaxVertexIndex + 1, PolyGroup.StartIndex * 3, (PolyGroup.StartIndex + PolyGroup.Count) * 3));
				}
			}
		});
	}

	template <typename PolygonGroupType>
	void GatherStreamRangesFromPolyGroupIndices(TConstArrayView<const PolygonGroupType> PolygonGroupIndices, const RealtimeMesh::FRealtimeMeshStream& Indices,
	                                            TMap<int32, FRealtimeMeshStreamRange>& OutStreamRanges)
	{
		if (Indices.GetLayout().GetElementType() == RealtimeMesh::GetRealtimeMeshDataElementType<uint16>())
		{
			GatherStreamRangesFromPolyGroupIndices(PolygonGroupIndices, Indices.GetElementArrayView<uint16>(), OutStreamRanges);
		}
		else if (Indices.GetLayout().GetElementType() == RealtimeMesh::GetRealtimeMeshDataElementType<int16>())
		{
			GatherStreamRangesFromPolyGroupIndices(PolygonGroupIndices, Indices.GetElementArrayView<int16>(), OutStreamRanges);
		}
		else if (Indices.GetLayout().GetElementType() == RealtimeMesh::GetRealtimeMeshDataElementType<uint32>())
		{
			GatherStreamRangesFromPolyGroupIndices(PolygonGroupIndices, Indices.GetElementArrayView<uint32>(), OutStreamRanges);
		}
		else if (Indices.GetLayout().GetElementType() == RealtimeMesh::GetRealtimeMeshDataElementType<int32>())
		{
			GatherStreamRangesFromPolyGroupIndices(PolygonGroupIndices, Indices.GetElementArrayView<int32>(), OutStreamRanges);
		}
		else
		{
			checkf(false, TEXT("Unsupported format for Indices"));
		}
	}

	template <typename IndexType>
	void GatherStreamRangesFromPolyGroupIndices(const RealtimeMesh::FRealtimeMeshStream& PolygonGroupIndices, TConstArrayView<const IndexType> Indices,
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

	REALTIMEMESHCOMPONENT_API void GatherStreamRangesFromPolyGroupIndices(const RealtimeMesh::FRealtimeMeshStream& PolygonGroupIndices,
	                                                                      const RealtimeMesh::FRealtimeMeshStream& Indices, TMap<int32, FRealtimeMeshStreamRange>& OutStreamRanges);


	template <typename TriangleType>
	void GenerateTangents(TConstArrayView<const TriangleType> Triangles, TConstArrayView<const FVector3f> Vertices,
	                      const TFunction<FVector2f(int32)>& UVGetter, const TFunctionRef<void(int32, FVector3f, FVector3f)>& TangentsSetter, bool bComputeSmoothNormals = true)
	{
		uint32 NumIndices = Triangles.Num();
		uint32 NumVertices = Vertices.Num();

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
				new(VertexSorter)FRealtimeMeshVertexSortElement(Index, Vertices[Index]);
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
					if (Vertices[SrcVertIdx].Equals(Vertices[OtherVertIdx]))
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
				uint32 VertIndex = FMath::Min(uint32(Triangles[(TriIdx * 3) + CornerIdx]), NumVertices - 1);

				CornerIndex[CornerIdx] = VertIndex;
				P[CornerIdx] = Vertices[VertIndex];

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
			if (UVGetter)
			{
				const FVector2f T1 = UVGetter(CornerIndex[0]);
				const FVector2f T2 = UVGetter(CornerIndex[1]);
				const FVector2f T3 = UVGetter(CornerIndex[2]);

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

			TangentsSetter(VertxIdx, TangentX, TangentZ);
		}
	}
}
