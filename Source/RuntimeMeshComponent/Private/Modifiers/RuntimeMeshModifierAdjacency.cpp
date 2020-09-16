// Copyright 2016-2020 TriAxis Games L.L.C. All Rights Reserved.


#include "Modifiers/RuntimeMeshModifierAdjacency.h"
#include "Logging/MessageLog.h"

const uint32 EdgesPerTriangle = 3;
const uint32 IndicesPerTriangle = 3;
const uint32 VerticesPerTriangle = 3;
const uint32 DuplicateIndexCount = 3;

const uint32 PnAenDomCorner_IndicesPerPatch = 12;

DECLARE_CYCLE_STAT(TEXT("RML - Calculate Tessellation Indices"), STAT_RuntimeMeshLibrary_CalculateTessellationIndices, STATGROUP_RuntimeMesh);

URuntimeMeshModifierAdjacency::URuntimeMeshModifierAdjacency()
{

}

void URuntimeMeshModifierAdjacency::ApplyToMesh_Implementation(FRuntimeMeshRenderableMeshData& MeshData)
{
	if (MeshData.TexCoords.Num() == MeshData.Positions.Num() && MeshData.Tangents.Num() == MeshData.Positions.Num())
	{
		CalculateTessellationIndices(MeshData);
	}
	else
	{
		FFormatNamedArguments Arguments;
		Arguments.Add(TEXT("NumPositions"), MeshData.Positions.Num());
		Arguments.Add(TEXT("NumNormals"), MeshData.Tangents.Num());
		Arguments.Add(TEXT("NumTexCoords"), MeshData.TexCoords.Num());
		FText Message = FText::Format(NSLOCTEXT("RuntimeMeshModifierAdjacency", "InvalidMeshDataForTessellation", "Supplied mesh doesn't contain enough data to calculate adjacency triangles for tessellation. All must be same length: Positions:{NumPositions} Normals:{NumNormals} TexCoords:{NumTexCoords}"), Arguments);
		FMessageLog("RuntimeMesh").Error(Message);
	}
}

void URuntimeMeshModifierAdjacency::CalculateTessellationIndices(FRuntimeMeshRenderableMeshData& MeshData)
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMeshLibrary_CalculateTessellationIndices);

	int32 NumIndices = MeshData.Triangles.Num();

	EdgeDictionary EdgeDict;
	EdgeDict.Reserve(NumIndices);
	PositionDictionary PosDict;
	PosDict.Reserve(NumIndices);

	MeshData.AdjacencyTriangles.SetNum(PnAenDomCorner_IndicesPerPatch * (NumIndices / IndicesPerTriangle));

	ExpandIB(MeshData, EdgeDict, PosDict);

	ReplacePlaceholderIndices(MeshData, EdgeDict, PosDict);
}

void URuntimeMeshModifierAdjacency::AddIfLeastUV(PositionDictionary& PosDict, const Vertex& Vert, uint32 Index)
{
	auto* Pos = PosDict.Find(Vert.Position);
	if (Pos == nullptr)
	{
		PosDict.Add(Vert.Position, Corner(Index, Vert.TexCoord));
	}
	else if (Vert.TexCoord < Pos->TexCoord)
	{
		PosDict[Vert.Position] = Corner(Index, Vert.TexCoord);
	}
}


void URuntimeMeshModifierAdjacency::ReplacePlaceholderIndices(FRuntimeMeshRenderableMeshData& MeshData, EdgeDictionary& EdgeDict, PositionDictionary& PosDict)
{
	int32 NumIndices = MeshData.Triangles.Num();
	const uint32 TriangleCount = NumIndices / PnAenDomCorner_IndicesPerPatch;

	for (uint32 U = 0; U < TriangleCount; U++)
	{
		const uint32 StartOutIndex = U * PnAenDomCorner_IndicesPerPatch;

		const uint32 Index0 = MeshData.AdjacencyTriangles.GetVertexIndex(StartOutIndex + 0);
		const uint32 Index1 = MeshData.AdjacencyTriangles.GetVertexIndex(StartOutIndex + 1);
		const uint32 Index2 = MeshData.AdjacencyTriangles.GetVertexIndex(StartOutIndex + 2);

		const Vertex Vertex0(MeshData.Positions.GetPosition(Index0), MeshData.TexCoords.GetTexCoord(Index0));
		const Vertex Vertex1(MeshData.Positions.GetPosition(Index1), MeshData.TexCoords.GetTexCoord(Index1));
		const Vertex Vertex2(MeshData.Positions.GetPosition(Index2), MeshData.TexCoords.GetTexCoord(Index2));

		Triangle Tri(Index0, Index1, Index2, Vertex0, Vertex1, Vertex2);

		Edge* Ed = EdgeDict.Find(Tri.GetEdge(0));
		if (Ed != nullptr)
		{
			MeshData.AdjacencyTriangles.SetVertexIndex(StartOutIndex + 3, Ed->GetIndex(0));
			MeshData.AdjacencyTriangles.SetVertexIndex(StartOutIndex + 4, Ed->GetIndex(1));
		}

		Ed = EdgeDict.Find(Tri.GetEdge(1));
		if (Ed != nullptr)
		{
			MeshData.AdjacencyTriangles.SetVertexIndex(StartOutIndex + 5, Ed->GetIndex(0));
			MeshData.AdjacencyTriangles.SetVertexIndex(StartOutIndex + 6, Ed->GetIndex(1));
		}

		Ed = EdgeDict.Find(Tri.GetEdge(2));
		if (Ed != nullptr)
		{
			MeshData.AdjacencyTriangles.SetVertexIndex(StartOutIndex + 7, Ed->GetIndex(0));
			MeshData.AdjacencyTriangles.SetVertexIndex(StartOutIndex + 8, Ed->GetIndex(1));
		}

		// Deal with dominant positions.
		for (uint32 V = 0; V < VerticesPerTriangle; V++)
		{
			Corner* Corn = PosDict.Find(Tri.GetEdge(V).GetVertex(0).Position);
			if (Corn != nullptr)
			{
				MeshData.AdjacencyTriangles.SetVertexIndex(StartOutIndex + 9 + V, Corn->Index);
			}
		}
	}
}


void URuntimeMeshModifierAdjacency::ExpandIB(FRuntimeMeshRenderableMeshData& MeshData, EdgeDictionary& OutEdgeDict, PositionDictionary& OutPosDict)
{
	int32 NumIndices = MeshData.Triangles.Num();
	const uint32 TriangleCount = NumIndices / IndicesPerTriangle;

	for (uint32 U = 0; U < TriangleCount; U++)
	{
		const uint32 StartInIndex = U * IndicesPerTriangle;
		const uint32 StartOutIndex = U * PnAenDomCorner_IndicesPerPatch;

		const uint32 Index0 = MeshData.Triangles.GetVertexIndex(U * 3 + 0);
		const uint32 Index1 = MeshData.Triangles.GetVertexIndex(U * 3 + 1);
		const uint32 Index2 = MeshData.Triangles.GetVertexIndex(U * 3 + 2);

		const Vertex Vertex0(MeshData.Positions.GetPosition(Index0), MeshData.TexCoords.GetTexCoord(Index0));
		const Vertex Vertex1(MeshData.Positions.GetPosition(Index1), MeshData.TexCoords.GetTexCoord(Index1));
		const Vertex Vertex2(MeshData.Positions.GetPosition(Index2), MeshData.TexCoords.GetTexCoord(Index2));

		Triangle Tri(Index0, Index1, Index2, Vertex0, Vertex1, Vertex2);

		if ((uint32)MeshData.AdjacencyTriangles.Num() <= (StartOutIndex + PnAenDomCorner_IndicesPerPatch))
		{
			MeshData.AdjacencyTriangles.SetNum((StartOutIndex + PnAenDomCorner_IndicesPerPatch) + 1);
		}

		MeshData.AdjacencyTriangles.SetVertexIndex(StartOutIndex + 0, Tri.GetIndex(0));
		MeshData.AdjacencyTriangles.SetVertexIndex(StartOutIndex + 1, Tri.GetIndex(1));
		MeshData.AdjacencyTriangles.SetVertexIndex(StartOutIndex + 2, Tri.GetIndex(2));

		MeshData.AdjacencyTriangles.SetVertexIndex(StartOutIndex + 3, Tri.GetIndex(0));
		MeshData.AdjacencyTriangles.SetVertexIndex(StartOutIndex + 4, Tri.GetIndex(1));

		MeshData.AdjacencyTriangles.SetVertexIndex(StartOutIndex + 5, Tri.GetIndex(1));
		MeshData.AdjacencyTriangles.SetVertexIndex(StartOutIndex + 6, Tri.GetIndex(2));

		MeshData.AdjacencyTriangles.SetVertexIndex(StartOutIndex + 7, Tri.GetIndex(2));
		MeshData.AdjacencyTriangles.SetVertexIndex(StartOutIndex + 8, Tri.GetIndex(0));

		MeshData.AdjacencyTriangles.SetVertexIndex(StartOutIndex + 9, Tri.GetIndex(0));
		MeshData.AdjacencyTriangles.SetVertexIndex(StartOutIndex + 10, Tri.GetIndex(1));
		MeshData.AdjacencyTriangles.SetVertexIndex(StartOutIndex + 11, Tri.GetIndex(2));


		Edge Rev0 = Tri.GetEdge(0).GetReverse();
		Edge Rev1 = Tri.GetEdge(1).GetReverse();
		Edge Rev2 = Tri.GetEdge(2).GetReverse();

		OutEdgeDict.Add(Rev0, Rev0);
		OutEdgeDict.Add(Rev1, Rev1);
		OutEdgeDict.Add(Rev2, Rev2);

		AddIfLeastUV(OutPosDict, Vertex0, Index0);
		AddIfLeastUV(OutPosDict, Vertex1, Index1);
		AddIfLeastUV(OutPosDict, Vertex2, Index2);
	}
}


