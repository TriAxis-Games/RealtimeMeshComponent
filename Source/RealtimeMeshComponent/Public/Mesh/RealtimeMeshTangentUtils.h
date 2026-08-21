// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Math/Vector.h"
#include "Math/Vector2D.h"
#include "Math/Matrix.h"
#include "Math/Plane.h"

namespace RealtimeMesh
{
	struct FRealtimeMeshStreamSet;
}

/**
 * CPU normal/tangent generation for runtime mesh data. (Split out of the former
 * RealtimeMeshAlgo.h; the namespace name is retained for source compatibility.)
 *
 * NOTE: this is an angle/area-agnostic accumulation generator, not mikktspace —
 * output does not exactly match editor-imported tangents. Fine for procedural
 * geometry; a mikktspace-quality path is a planned upgrade for import-parity
 * use cases.
 */
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

	// Shared tangent/normal generation core. This is the single surviving copy of the
	// algorithm; both public GenerateTangents entry points (the array-view/callback
	// template below and the FRealtimeMeshStreamSet overload in the .cpp) delegate
	// here. The setter receives the full basis (TangentX, TangentY = binormal,
	// TangentZ = normal); callers that don't need the binormal simply ignore it.
	template <typename TriangleType>
	void GenerateTangentsImpl(TConstArrayView<const TriangleType> Triangles, TConstArrayView<const FVector3f> Vertices,
	                          const TFunction<FVector2f(int32)>& UVGetter,
	                          const TFunctionRef<void(int32, FVector3f, FVector3f, FVector3f)>& TangentsSetter, bool bComputeSmoothNormals)
	{
		const uint32 NumIndices = Triangles.Num();
		const uint32 NumVertices = Vertices.Num();

		if (NumVertices == 0)
		{
			// Nothing to generate. Guard is also required for correctness: with
			// triangles present the `NumVertices - 1` clamp below would wrap
			// (unsigned) to 0xFFFFFFFF and index out of bounds.
			return;
		}

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
				TArray<uint32, TInlineAllocator<8>> VertOverlaps;
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
					TArray<uint32, TInlineAllocator<8>> OverlapTris;
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
			TArray<uint32, TInlineAllocator<8>> SmoothTris;
			VertToTriSmoothMap.MultiFind(VertxIdx, SmoothTris);

			for (int i = 0; i < SmoothTris.Num(); i++)
			{
				uint32 TriIdx = SmoothTris[i];
				VertexTangentZSum[VertxIdx] += FaceTangentZ[TriIdx];
			}

			// Find relevant triangles for tangents
			TArray<uint32, TInlineAllocator<8>> TangentTris;
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

			TangentsSetter(VertxIdx, TangentX, TangentY, TangentZ);
		}
	}


	// Public array-view / callback entry point. Thin adapter over GenerateTangentsImpl
	// that keeps the historical two-vector callback signature (tangent X + normal Z)
	// its callers rely on; the binormal (Y) computed by the core is dropped here.
	template <typename TriangleType>
	void GenerateTangents(TConstArrayView<const TriangleType> Triangles, TConstArrayView<const FVector3f> Vertices,
	                      const TFunction<FVector2f(int32)>& UVGetter, const TFunctionRef<void(int32, FVector3f, FVector3f)>& TangentsSetter, bool bComputeSmoothNormals = true)
	{
		GenerateTangentsImpl<TriangleType>(Triangles, Vertices, UVGetter,
			[&TangentsSetter](int32 VertIdx, FVector3f TangentX, FVector3f /*TangentY*/, FVector3f TangentZ)
			{
				TangentsSetter(VertIdx, TangentX, TangentZ);
			},
			bComputeSmoothNormals);
	}

	// StreamSet entry point: reads Position/Triangles (+ optional TexCoords), replaces
	// the Tangents stream with freshly generated normal-precision tangents.
	REALTIMEMESHCOMPONENT_API void GenerateTangents(RealtimeMesh::FRealtimeMeshStreamSet& StreamSet, bool bComputeSmoothNormals = true);
}
