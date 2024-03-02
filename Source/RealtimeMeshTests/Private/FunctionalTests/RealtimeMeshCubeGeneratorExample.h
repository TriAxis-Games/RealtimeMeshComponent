// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Mesh/RealtimeMeshBuilder.h"



template<typename IndexType, typename TangentType, typename TexCoordType, int32 NumTexCoords>
static void AppendBox(RealtimeMesh::TRealtimeMeshBuilderLocal<IndexType, TangentType, TexCoordType, NumTexCoords>& Builder, const FVector3f& BoxRadius, uint16 PolyGroup)
{	
	// Generate verts
	FVector3f BoxVerts[8];
	BoxVerts[0] = FVector3f(-BoxRadius.X, BoxRadius.Y, BoxRadius.Z);
	BoxVerts[1] = FVector3f(BoxRadius.X, BoxRadius.Y, BoxRadius.Z);
	BoxVerts[2] = FVector3f(BoxRadius.X, -BoxRadius.Y, BoxRadius.Z);
	BoxVerts[3] = FVector3f(-BoxRadius.X, -BoxRadius.Y, BoxRadius.Z);

	BoxVerts[4] = FVector3f(-BoxRadius.X, BoxRadius.Y, -BoxRadius.Z);
	BoxVerts[5] = FVector3f(BoxRadius.X, BoxRadius.Y, -BoxRadius.Z);
	BoxVerts[6] = FVector3f(BoxRadius.X, -BoxRadius.Y, -BoxRadius.Z);
	BoxVerts[7] = FVector3f(-BoxRadius.X, -BoxRadius.Y, -BoxRadius.Z);	
	
	{ // Positive Z
		int32 V0 = Builder.AddVertex(BoxVerts[0])
			.SetNormalAndTangent(FVector3f(0.0f, 0.0f, 1.0f), FVector3f(0.0f, -1.0f, 0.0f))
			.SetTexCoord(FVector2f(0.0f, 0.0f))
			.GetIndex();

		if constexpr (NumTexCoords >= 2)
		{
			Builder.EditVertex(V0).SetTexCoord(1, FVector2f::ZeroVector);
		}

		int32 V1 = Builder.AddVertex(BoxVerts[1])
			.SetNormalAndTangent(FVector3f(0.0f, 0.0f, 1.0f), FVector3f(0.0f, -1.0f, 0.0f))
			.SetTexCoord(FVector2f(0.0f, 1.0f))
			.GetIndex();

		int32 V2 = Builder.AddVertex(BoxVerts[2])
			.SetNormalAndTangent(FVector3f(0.0f, 0.0f, 1.0f), FVector3f(0.0f, -1.0f, 0.0f))
			.SetTexCoord(FVector2f(1.0f, 1.0f))
			.GetIndex();

		int32 V3 = Builder.AddVertex(BoxVerts[3])
			.SetNormalAndTangent(FVector3f(0.0f, 0.0f, 1.0f), FVector3f(0.0f, -1.0f, 0.0f))
			.SetTexCoord(FVector2f(1.0f, 0.0f))
			.GetIndex();

		Builder.AddTriangle(V0, V1, V3, PolyGroup);
		Builder.AddTriangle(V1, V2, V3, PolyGroup);		
	}

	{ // Negative X
		int32 V0 = Builder.AddVertex(BoxVerts[4])
			.SetNormalAndTangent(FVector3f(-1.0f, 0.0f, 0.0f), FVector3f(0.0f, -1.0f, 0.0f))
			.SetTexCoord(FVector2f(0.0f, 0.0f))
			.GetIndex();

		int32 V1 = Builder.AddVertex(BoxVerts[0])
			.SetNormalAndTangent(FVector3f(-1.0f, 0.0f, 0.0f), FVector3f(0.0f, -1.0f, 0.0f))
			.SetTexCoord(FVector2f(0.0f, 1.0f))
			.GetIndex();

		int32 V2 = Builder.AddVertex(BoxVerts[3])
			.SetNormalAndTangent(FVector3f(-1.0f, 0.0f, 0.0f), FVector3f(0.0f, -1.0f, 0.0f))
			.SetTexCoord(FVector2f(1.0f, 1.0f))
			.GetIndex();

		int32 V3 = Builder.AddVertex(BoxVerts[7])
			.SetNormalAndTangent(FVector3f(-1.0f, 0.0f, 0.0f), FVector3f(0.0f, -1.0f, 0.0f))
			.SetTexCoord(FVector2f(1.0f, 0.0f))
			.GetIndex();

		Builder.AddTriangle(V0, V1, V3, PolyGroup);
		Builder.AddTriangle(V1, V2, V3, PolyGroup);		
	}

	{ // Positive Y
		int32 V0 = Builder.AddVertex(BoxVerts[5])
			.SetNormalAndTangent(FVector3f(0.0f, 1.0f, 0.0f), FVector3f(-1.0f, 0.0f, 0.0f))
			.SetTexCoord(FVector2f(0.0f, 0.0f))
			.GetIndex();

		int32 V1 = Builder.AddVertex(BoxVerts[1])
			.SetNormalAndTangent(FVector3f(0.0f, 1.0f, 0.0f), FVector3f(-1.0f, 0.0f, 0.0f))
			.SetTexCoord(FVector2f(0.0f, 1.0f))
			.GetIndex();

		int32 V2 = Builder.AddVertex(BoxVerts[0])
			.SetNormalAndTangent(FVector3f(0.0f, 1.0f, 0.0f), FVector3f(-1.0f, 0.0f, 0.0f))
			.SetTexCoord(FVector2f(1.0f, 1.0f))
			.GetIndex();

		int32 V3 = Builder.AddVertex(BoxVerts[4])
			.SetNormalAndTangent(FVector3f(0.0f, 1.0f, 0.0f), FVector3f(-1.0f, 0.0f, 0.0f))
			.SetTexCoord(FVector2f(1.0f, 0.0f))
			.GetIndex();

		Builder.AddTriangle(V0, V1, V3, PolyGroup);
		Builder.AddTriangle(V1, V2, V3, PolyGroup);		
	}

	{ // Positive X
		int32 V0 = Builder.AddVertex(BoxVerts[6])
			.SetNormalAndTangent(FVector3f(1.0f, 0.0f, 0.0f), FVector3f(0.0f, 1.0f, 0.0f))
			.SetTexCoord(FVector2f(0.0f, 0.0f))
			.GetIndex();

		int32 V1 = Builder.AddVertex(BoxVerts[2])
			.SetNormalAndTangent(FVector3f(1.0f, 0.0f, 0.0f), FVector3f(0.0f, 1.0f, 0.0f))
			.SetTexCoord(FVector2f(0.0f, 1.0f))
			.GetIndex();

		int32 V2 = Builder.AddVertex(BoxVerts[1])
			.SetNormalAndTangent(FVector3f(1.0f, 0.0f, 0.0f), FVector3f(0.0f, 1.0f, 0.0f))
			.SetTexCoord(FVector2f(1.0f, 1.0f))
			.GetIndex();

		int32 V3 = Builder.AddVertex(BoxVerts[5])
			.SetNormalAndTangent(FVector3f(1.0f, 0.0f, 0.0f), FVector3f(0.0f, 1.0f, 0.0f))
			.SetTexCoord(FVector2f(1.0f, 0.0f))
			.GetIndex();

		Builder.AddTriangle(V0, V1, V3, PolyGroup);
		Builder.AddTriangle(V1, V2, V3, PolyGroup);		
	}

	{ // Negative Y
		int32 V0 = Builder.AddVertex(BoxVerts[7])
			.SetNormalAndTangent(FVector3f(0.0f, -1.0f, 0.0f), FVector3f(1.0f, 0.0f, 0.0f))
			.SetTexCoord(FVector2f(0.0f, 0.0f))
			.GetIndex();

		int32 V1 = Builder.AddVertex(BoxVerts[3])
			.SetNormalAndTangent(FVector3f(0.0f, -1.0f, 0.0f), FVector3f(1.0f, 0.0f, 0.0f))
			.SetTexCoord(FVector2f(0.0f, 1.0f))
			.GetIndex();

		int32 V2 = Builder.AddVertex(BoxVerts[2])
			.SetNormalAndTangent(FVector3f(0.0f, -1.0f, 0.0f), FVector3f(1.0f, 0.0f, 0.0f))
			.SetTexCoord(FVector2f(1.0f, 1.0f))
			.GetIndex();

		int32 V3 = Builder.AddVertex(BoxVerts[6])
			.SetNormalAndTangent(FVector3f(0.0f, -1.0f, 0.0f), FVector3f(1.0f, 0.0f, 0.0f))
			.SetTexCoord(FVector2f(1.0f, 0.0f))
			.GetIndex();

		Builder.AddTriangle(V0, V1, V3, PolyGroup);
		Builder.AddTriangle(V1, V2, V3, PolyGroup);		
	}

	{ // Negative Z
		int32 V0 = Builder.AddVertex(BoxVerts[7])
			.SetNormalAndTangent(FVector3f(0.0f, 0.0f, -1.0f), FVector3f(0.0f, 1.0f, 0.0f))
			.SetTexCoord(FVector2f(0.0f, 0.0f))
			.GetIndex();

		int32 V1 = Builder.AddVertex(BoxVerts[6])
			.SetNormalAndTangent(FVector3f(0.0f, 0.0f, -1.0f), FVector3f(0.0f, 1.0f, 0.0f))
			.SetTexCoord(FVector2f(0.0f, 1.0f))
			.GetIndex();

		int32 V2 = Builder.AddVertex(BoxVerts[5])
			.SetNormalAndTangent(FVector3f(0.0f, 0.0f, -1.0f), FVector3f(0.0f, 1.0f, 0.0f))
			.SetTexCoord(FVector2f(1.0f, 1.0f))
			.GetIndex();

		int32 V3 = Builder.AddVertex(BoxVerts[4])
			.SetNormalAndTangent(FVector3f(0.0f, 0.0f, -1.0f), FVector3f(0.0f, 1.0f, 0.0f))
			.SetTexCoord(FVector2f(1.0f, 0.0f))
			.GetIndex();

		Builder.AddTriangle(V0, V1, V3, PolyGroup);
		Builder.AddTriangle(V1, V2, V3, PolyGroup);		
	}
}