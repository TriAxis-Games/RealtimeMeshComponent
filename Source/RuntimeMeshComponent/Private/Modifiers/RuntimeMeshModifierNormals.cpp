// Copyright 2016-2020 TriAxis Games L.L.C. All Rights Reserved.


#include "Modifiers/RuntimeMeshModifierNormals.h"


URuntimeMeshModifierNormals::URuntimeMeshModifierNormals()
	: bComputeSmoothNormals(false)
{

}

void URuntimeMeshModifierNormals::ApplyToMesh_Implementation(FRuntimeMeshRenderableMeshData& MeshData)
{
	CalculateNormalsTangents(MeshData, bComputeSmoothNormals);	
}

void URuntimeMeshModifierNormals::CalculateNormalsTangents(FRuntimeMeshRenderableMeshData& MeshData, bool bInComputeSmoothNormals)
{
	int32 NumVertices = MeshData.Positions.Num();
	int32 NumIndices = MeshData.Triangles.Num();
	int32 NumUVs = MeshData.TexCoords.Num();

	// Resize the tangents array to fit the new normals/tangents
	MeshData.Tangents.SetNum(NumVertices, true);

	// Calculate the duplicate vertices map if we're wanting smooth normals.  Don't find duplicates if we don't want smooth normals
	// that will cause it to only smooth across faces sharing a common vertex, not across faces with vertices of common position
	const TMultiMap<uint32, uint32> DuplicateVertexMap = bInComputeSmoothNormals ? FindDuplicateVerticesMap(MeshData.Positions, NumVertices) : TMultiMap<uint32, uint32>();


	// Number of triangles
	const int32 NumTris = NumIndices / 3;

	// Map of vertex to triangles in Triangles array
	TMultiMap<uint32, uint32> VertToTriMap;
	// Map of vertex to triangles to consider for normal calculation
	TMultiMap<uint32, uint32> VertToTriSmoothMap;

	// Normal/tangents for each face
	TArray<FVector> FaceTangentX, FaceTangentY, FaceTangentZ;
	FaceTangentX.AddUninitialized(NumTris);
	FaceTangentY.AddUninitialized(NumTris);
	FaceTangentZ.AddUninitialized(NumTris);

	// Iterate over triangles
	for (int32 TriIdx = 0; TriIdx < NumTris; TriIdx++)
	{
		uint32 CornerIndex[3];
		FVector P[3];

		for (int32 CornerIdx = 0; CornerIdx < 3; CornerIdx++)
		{
			// Find vert index (clamped within range)
			uint32 VertIndex = FMath::Min(MeshData.Triangles.GetVertexIndex((TriIdx * 3) + CornerIdx), (uint32)NumVertices - 1);

			CornerIndex[CornerIdx] = VertIndex;
			P[CornerIdx] = MeshData.Positions.GetPosition(VertIndex);

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
		const FVector Edge21 = P[1] - P[2];
		const FVector Edge20 = P[0] - P[2];
		const FVector TriNormal = (Edge21 ^ Edge20).GetSafeNormal();

		// If we have UVs, use those to calculate
		if (NumUVs == NumVertices)
		{
			const FVector2D T1 = MeshData.TexCoords.GetTexCoord(CornerIndex[0]);
			const FVector2D T2 = MeshData.TexCoords.GetTexCoord(CornerIndex[1]);
			const FVector2D T3 = MeshData.TexCoords.GetTexCoord(CornerIndex[2]);

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




			FMatrix	ParameterToLocal(
				FPlane(P[1].X - P[0].X, P[1].Y - P[0].Y, P[1].Z - P[0].Z, 0),
				FPlane(P[2].X - P[0].X, P[2].Y - P[0].Y, P[2].Z - P[0].Z, 0),
				FPlane(P[0].X, P[0].Y, P[0].Z, 0),
				FPlane(0, 0, 0, 1)
				);

			FMatrix ParameterToTexture(
				FPlane(T2.X - T1.X, T2.Y - T1.Y, 0, 0),
				FPlane(T3.X - T1.X, T3.Y - T1.Y, 0, 0),
				FPlane(T1.X, T1.Y, 1, 0),
				FPlane(0, 0, 0, 1)
				);

			// Use InverseSlow to catch singular matrices.  Inverse can miss this sometimes.
			const FMatrix TextureToLocal = ParameterToTexture.Inverse() * ParameterToLocal;

			FaceTangentX[TriIdx] = TextureToLocal.TransformVector(FVector(1, 0, 0)).GetSafeNormal();
			FaceTangentY[TriIdx] = TextureToLocal.TransformVector(FVector(0, 1, 0)).GetSafeNormal();
		}
		else
		{
			FaceTangentX[TriIdx] = Edge20.GetSafeNormal();
			FaceTangentY[TriIdx] = (FaceTangentX[TriIdx] ^ TriNormal).GetSafeNormal();
		}

		FaceTangentZ[TriIdx] = TriNormal;
	}


	// Arrays to accumulate tangents into
	TArray<FVector> VertexTangentXSum, VertexTangentYSum, VertexTangentZSum;
	VertexTangentXSum.AddZeroed(NumVertices);
	VertexTangentYSum.AddZeroed(NumVertices);
	VertexTangentZSum.AddZeroed(NumVertices);

	// For each vertex..
	for (int VertxIdx = 0; VertxIdx < NumVertices; VertxIdx++)
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
	for (int VertxIdx = 0; VertxIdx < NumVertices; VertxIdx++)
	{
		FVector& TangentX = VertexTangentXSum[VertxIdx];
		FVector& TangentY = VertexTangentYSum[VertxIdx];
		FVector& TangentZ = VertexTangentZSum[VertxIdx];

		TangentX.Normalize();
		//TangentY.Normalize();
		TangentZ.Normalize();

		// Use Gram-Schmidt orthogonalization to make sure X is orthonormal with Z
		TangentX -= TangentZ * (TangentZ | TangentX);
		TangentX.Normalize();
		TangentY.Normalize();

		MeshData.Tangents.SetNormal(VertxIdx, TangentZ);
		MeshData.Tangents.SetTangent(VertxIdx, TangentX);
	}
}


TMultiMap<uint32, uint32> URuntimeMeshModifierNormals::FindDuplicateVerticesMap(const FRuntimeMeshVertexPositionStream& PositionStream, float Tollerance)
{
	int32 NumVertices = PositionStream.Num();


	TArray<FRuntimeMeshVertexSortingElement> VertexSorter;
	VertexSorter.Empty(NumVertices);
	for (int32 Index = 0; Index < NumVertices; Index++)
	{
		new (VertexSorter)FRuntimeMeshVertexSortingElement(Index, PositionStream.GetPosition(Index));
	}

	// Sort the list
	VertexSorter.Sort(FRuntimeMeshVertexSortingFunction());

	// Clear the index map.
	TMultiMap<uint32, uint32> IndexMap;

	// Map out the duplicates.
	for (int32 Index = 0; Index < NumVertices; Index++)
	{
		uint32 SrcVertIdx = VertexSorter[Index].Index;
		float Value = VertexSorter[Index].Value;

		// Search forward adding pairs both ways
		for (int32 SubIndex = Index + 1; SubIndex < NumVertices; SubIndex++)
		{
			if (FMath::Abs(VertexSorter[SubIndex].Value - Value) > THRESH_POINTS_ARE_SAME * 4.01f)
			{
				// No more possible duplicates
				break;
			}

			uint32 OtherVertIdx = VertexSorter[SubIndex].Index;
			if (PositionStream.GetPosition(SrcVertIdx).Equals(PositionStream.GetPosition(OtherVertIdx), Tollerance))
			{
				IndexMap.AddUnique(SrcVertIdx, OtherVertIdx);
				IndexMap.AddUnique(OtherVertIdx, SrcVertIdx);
			}
		}
	}

	return IndexMap;
}