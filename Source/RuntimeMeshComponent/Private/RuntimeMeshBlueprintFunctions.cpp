// Copyright 2016-2019 Chris Conway (Koderz). All Rights Reserved.


#include "RuntimeMeshBlueprintFunctions.h"

// MeshData


FRuntimeMeshVertexPositionStream& URuntimeMeshBlueprintFunctions::GetPositionStream(FRuntimeMeshRenderableMeshData& MeshData, FRuntimeMeshRenderableMeshData& OutMeshData)
{
	OutMeshData = MeshData;
	return MeshData.Positions;
}

FRuntimeMeshVertexTangentStream& URuntimeMeshBlueprintFunctions::GetTangentStream(FRuntimeMeshRenderableMeshData& MeshData, FRuntimeMeshRenderableMeshData& OutMeshData)
{
	OutMeshData = MeshData;
	return MeshData.Tangents;
}

FRuntimeMeshVertexTexCoordStream& URuntimeMeshBlueprintFunctions::GetTexCoordStream(FRuntimeMeshRenderableMeshData& MeshData, FRuntimeMeshRenderableMeshData& OutMeshData)
{
	OutMeshData = MeshData;
	return MeshData.TexCoords;
}

FRuntimeMeshVertexColorStream& URuntimeMeshBlueprintFunctions::GetColorStream(FRuntimeMeshRenderableMeshData& MeshData, FRuntimeMeshRenderableMeshData& OutMeshData)
{
	OutMeshData = MeshData;
	return MeshData.Colors;
}

FRuntimeMeshTriangleStream& URuntimeMeshBlueprintFunctions::GetTriangleStream(FRuntimeMeshRenderableMeshData& MeshData, FRuntimeMeshRenderableMeshData& OutMeshData)
{
	OutMeshData = MeshData;
	return MeshData.Triangles;
}

FRuntimeMeshTriangleStream& URuntimeMeshBlueprintFunctions::GetAdjacencyTriangleStream(FRuntimeMeshRenderableMeshData& MeshData, FRuntimeMeshRenderableMeshData& OutMeshData)
{
	OutMeshData = MeshData;
	return MeshData.AdjacencyTriangles;
}


// Positions

FRuntimeMeshVertexPositionStream URuntimeMeshBlueprintFunctions::MakeRuntimeMeshVertexPositionStream_Fvectors(const TArray<FVector>& InPositions)
{
	FRuntimeMeshVertexPositionStream out;
	out.AddRange(InPositions);
	return out;
}

FRuntimeMeshVertexPositionStream URuntimeMeshBlueprintFunctions::MakeRuntimeMeshVertexPositionStream_Triangles(const TArray<FRuntimeMeshRenerableTriangleVertices>& InTriangles)
{
	FRuntimeMeshVertexPositionStream out;
	out.AddTriangles(InTriangles);
	return out;
}

void URuntimeMeshBlueprintFunctions::SetNumPositions(FRuntimeMeshVertexPositionStream& Stream, FRuntimeMeshVertexPositionStream& OutStream, int32 NewNum, bool bAllowShrinking /*= true*/)
{
	OutStream = Stream;
	Stream.SetNum(NewNum, bAllowShrinking);
}

void URuntimeMeshBlueprintFunctions::NumPositions(FRuntimeMeshVertexPositionStream& Stream, FRuntimeMeshVertexPositionStream& OutStream, int32& OutNumPositions)
{
	OutStream = Stream;
	OutNumPositions = Stream.Num();
}

void URuntimeMeshBlueprintFunctions::EmptyPositions(FRuntimeMeshVertexPositionStream& Stream, FRuntimeMeshVertexPositionStream& OutStream, int32 Slack /*= 0*/)
{
	OutStream = Stream;
	Stream.Empty(Slack);
}

void URuntimeMeshBlueprintFunctions::AddPosition(FRuntimeMeshVertexPositionStream& Stream, FRuntimeMeshVertexPositionStream& OutStream, FVector InPosition, int32& OutIndex)
{
	OutStream = Stream;
	OutIndex = Stream.Add(InPosition);
}

void URuntimeMeshBlueprintFunctions::AddPositions(FRuntimeMeshVertexPositionStream& Stream,	FRuntimeMeshVertexPositionStream& OutStream, const TArray<FVector>& InPositions)
{
	OutStream = Stream;
	Stream.AddRange(InPositions);
}

void URuntimeMeshBlueprintFunctions::AddPositionTriangle(FRuntimeMeshVertexPositionStream& Stream, FRuntimeMeshVertexPositionStream& OutStream, const FRuntimeMeshRenerableTriangleVertices& InTriangle)
{
	OutStream = Stream;
	Stream.AddTriangle(InTriangle);
}

void URuntimeMeshBlueprintFunctions::AddPositionTriangles(FRuntimeMeshVertexPositionStream& Stream, FRuntimeMeshVertexPositionStream& OutStream, const TArray<FRuntimeMeshRenerableTriangleVertices>& InTriangles)
{
	OutStream = Stream;
	Stream.AddTriangles(InTriangles);
}

void URuntimeMeshBlueprintFunctions::AppendPositions(FRuntimeMeshVertexPositionStream& Stream, FRuntimeMeshVertexPositionStream& OutStream, const FRuntimeMeshVertexPositionStream& InOther)
{
	OutStream = Stream;
	Stream.Append(InOther);
}

void URuntimeMeshBlueprintFunctions::GetPosition(FRuntimeMeshVertexPositionStream& Stream, FRuntimeMeshVertexPositionStream& OutStream, int32 Index, FVector& OutPosition)
{
	OutStream = Stream;
	OutPosition = Stream.GetPosition(Index);
}

void URuntimeMeshBlueprintFunctions::SetPosition(FRuntimeMeshVertexPositionStream& Stream, FRuntimeMeshVertexPositionStream& OutStream, int32 Index, FVector NewPosition)
{
	OutStream = Stream;
	Stream.SetPosition(Index, NewPosition);
}

void URuntimeMeshBlueprintFunctions::GetBounds(FRuntimeMeshVertexPositionStream& Stream, FRuntimeMeshVertexPositionStream& OutStream, FBox& OutBounds)
{
	OutStream = Stream;
	OutBounds = Stream.GetBounds();
}


// Tangents

FRuntimeMeshVertexTangentStream URuntimeMeshBlueprintFunctions::MakeRuntimeMeshVertexTangentStream_NormalsTangets(const TArray<FVector>& InNormals, const TArray<FVector>& InTangents, bool bUseHighPrecision)
{
	FRuntimeMeshVertexTangentStream out(bUseHighPrecision);
	out.AddRange(InNormals, InTangents);
	return out;
}

FRuntimeMeshVertexTangentStream URuntimeMeshBlueprintFunctions::MakeRuntimeMeshVertexTangentStream_Tangents(const TArray<FVector>& InTangentsX, const TArray<FVector>& InTangentsY, const TArray<FVector>& InTangentsZ, bool bUseHighPrecision)
{
	FRuntimeMeshVertexTangentStream out(bUseHighPrecision);
	out.AddRange(InTangentsX, InTangentsY, InTangentsZ);
	return out;
}

void URuntimeMeshBlueprintFunctions::SetNumTangents(FRuntimeMeshVertexTangentStream& Stream, FRuntimeMeshVertexTangentStream& OutStream, int32 NewNum, bool bAllowShrinking /*= true*/)
{
	OutStream = Stream;
	Stream.SetNum(NewNum, bAllowShrinking);
}

void URuntimeMeshBlueprintFunctions::NumTangents(FRuntimeMeshVertexTangentStream& Stream, FRuntimeMeshVertexTangentStream& OutStream, int32& OutNumTangents)
{
	OutStream = Stream;
	OutNumTangents = Stream.Num();
}

void URuntimeMeshBlueprintFunctions::EmptyTangents(FRuntimeMeshVertexTangentStream& Stream, FRuntimeMeshVertexTangentStream& OutStream, int32 Slack /*= 0*/)
{
	OutStream = Stream;
	Stream.Empty(Slack);
}

void URuntimeMeshBlueprintFunctions::AddNormalAndTangent(FRuntimeMeshVertexTangentStream& Stream, FRuntimeMeshVertexTangentStream& OutStream, FVector InNormal, FVector InTangent, int32& OutIndex)
{
	OutStream = Stream;
	OutIndex = Stream.Add(InNormal, InTangent);
}

void URuntimeMeshBlueprintFunctions::AddTangents(FRuntimeMeshVertexTangentStream& Stream, FRuntimeMeshVertexTangentStream& OutStream, FVector InTangentX, FVector InTangentY, FVector InTangentZ, int32& OutIndex)
{
	OutStream = Stream;
	OutIndex = Stream.Add(InTangentX, InTangentY, InTangentZ);
}

void URuntimeMeshBlueprintFunctions::AddNormalsAndTangents(FRuntimeMeshVertexTangentStream& Stream,	FRuntimeMeshVertexTangentStream& OutStream, const TArray<FVector>& InNormals, const TArray<FVector>& InTangents)
{
	OutStream = Stream;
	Stream.AddRange(InNormals, InTangents);
}

void URuntimeMeshBlueprintFunctions::AddTangentRange(FRuntimeMeshVertexTangentStream& Stream, FRuntimeMeshVertexTangentStream& OutStream, const TArray<FVector>& InTangentsX, const TArray<FVector>& InTangentsY,
	const TArray<FVector>& InTangentsZ)
{
	OutStream = Stream;
	Stream.AddRange(InTangentsX, InTangentsY, InTangentsZ);
}

void URuntimeMeshBlueprintFunctions::AppendTangents(FRuntimeMeshVertexTangentStream& Stream, FRuntimeMeshVertexTangentStream& OutStream, const FRuntimeMeshVertexTangentStream& InOther)
{
	OutStream = Stream;
	Stream.Append(InOther);
}

void URuntimeMeshBlueprintFunctions::GetNormal(FRuntimeMeshVertexTangentStream& Stream, FRuntimeMeshVertexTangentStream& OutStream, int32 Index, FVector& OutNormal)
{
	OutStream = Stream;
	OutNormal = Stream.GetNormal(Index);
}

void URuntimeMeshBlueprintFunctions::SetNormal(FRuntimeMeshVertexTangentStream& Stream, FRuntimeMeshVertexTangentStream& OutStream, int32 Index, FVector NewNormal)
{
	OutStream = Stream;
	Stream.SetNormal(Index, NewNormal);
}

void URuntimeMeshBlueprintFunctions::GetTangent(FRuntimeMeshVertexTangentStream& Stream, FRuntimeMeshVertexTangentStream& OutStream, int32 Index, FVector& OutTangent)
{
	OutStream = Stream;
	OutTangent = Stream.GetTangent(Index);
}

void URuntimeMeshBlueprintFunctions::SetTangent(FRuntimeMeshVertexTangentStream& Stream, FRuntimeMeshVertexTangentStream& OutStream, int32 Index, FVector NewTangent)
{
	OutStream = Stream;
	Stream.SetTangent(Index, NewTangent);
}

void URuntimeMeshBlueprintFunctions::GetTangents(FRuntimeMeshVertexTangentStream& Stream, FRuntimeMeshVertexTangentStream& OutStream, int32 Index, FVector& OutTangentX, FVector& OutTangentY, FVector& OutTangentZ)
{
	OutStream = Stream;
	Stream.GetTangents(Index, OutTangentX, OutTangentY, OutTangentZ);
}

void URuntimeMeshBlueprintFunctions::SetTangents(FRuntimeMeshVertexTangentStream& Stream, FRuntimeMeshVertexTangentStream& OutStream, int32 Index, FVector InTangentX, FVector InTangentY, FVector InTangentZ)
{
	OutStream = Stream;
	Stream.SetTangents(Index, InTangentX, InTangentY, InTangentZ);
}


// Texture Coordinates

FRuntimeMeshVertexTexCoordStream URuntimeMeshBlueprintFunctions::MakeRuntimeMeshVertexTexCoordStream_SingleChannel(const TArray<FVector2D>& InTexCoords)
{
	FRuntimeMeshVertexTexCoordStream out;
	out.AddRange(InTexCoords);
	return out;
}

FRuntimeMeshVertexTexCoordStream URuntimeMeshBlueprintFunctions::MakeRuntimeMeshVertexTexCoordStream_MultiChannel(int32 ChannelCount, const TArray<FVector2D>& InTexCoords, const TArray<int32> InChannelIds)
{
	FRuntimeMeshVertexTexCoordStream out(ChannelCount);
	out.AddRange(InTexCoords, InChannelIds);
	return out;
}

void URuntimeMeshBlueprintFunctions::SetNumTexCoords(FRuntimeMeshVertexTexCoordStream& Stream, FRuntimeMeshVertexTexCoordStream& OutStream, int32 NewNum, bool bAllowShrinking /*= true*/)
{
	OutStream = Stream;
	Stream.SetNum(NewNum, bAllowShrinking);
}

void URuntimeMeshBlueprintFunctions::NumTexCoords(FRuntimeMeshVertexTexCoordStream& Stream, FRuntimeMeshVertexTexCoordStream& OutStream, int32& OutNumTexCoords)
{
	OutStream = Stream;
	OutNumTexCoords = Stream.Num();
}

void URuntimeMeshBlueprintFunctions::NumTexCoordChannels(FRuntimeMeshVertexTexCoordStream& Stream, FRuntimeMeshVertexTexCoordStream& OutStream, int32& OutNumTexCoordChannels)
{
	OutStream = Stream;
	OutNumTexCoordChannels = Stream.NumChannels();
}

void URuntimeMeshBlueprintFunctions::EmptyTexCoords(FRuntimeMeshVertexTexCoordStream& Stream, FRuntimeMeshVertexTexCoordStream& OutStream, int32 Slack /*= 0*/)
{
	OutStream = Stream;
	Stream.Empty(Slack);
}

void URuntimeMeshBlueprintFunctions::AddTexCoord(FRuntimeMeshVertexTexCoordStream& Stream, FRuntimeMeshVertexTexCoordStream& OutStream, int32& OutIndex, FVector2D InTexCoord, int32 ChannelId)
{
	OutStream = Stream;
	OutIndex = Stream.Add(InTexCoord, ChannelId);
}

void URuntimeMeshBlueprintFunctions::AddTexCoords(FRuntimeMeshVertexTexCoordStream& Stream,	FRuntimeMeshVertexTexCoordStream& OutStream, const TArray<FVector2D>& InTexCoords, int32 ChannelId)
{
	OutStream = Stream;
	Stream.AddRange(InTexCoords, ChannelId);
}

void URuntimeMeshBlueprintFunctions::AddTexCoordsForChannels(FRuntimeMeshVertexTexCoordStream& Stream, FRuntimeMeshVertexTexCoordStream& OutStream, const TArray<FVector2D>& InTexCoords, const TArray<int32> InChannelIds)
{
	OutStream = Stream;
	Stream.AddRange(InTexCoords, InChannelIds);
}

void URuntimeMeshBlueprintFunctions::AppendTexCoords(FRuntimeMeshVertexTexCoordStream& Stream, FRuntimeMeshVertexTexCoordStream& OutStream, const FRuntimeMeshVertexTexCoordStream& InOther)
{
	OutStream = Stream;
	Stream.Append(InOther);
}

void URuntimeMeshBlueprintFunctions::GetTexCoord(FRuntimeMeshVertexTexCoordStream& Stream, FRuntimeMeshVertexTexCoordStream& OutStream, int32 Index, FVector2D& OutTexCoord)
{
	OutStream = Stream;
	OutTexCoord = Stream.GetTexCoord(Index);
}

void URuntimeMeshBlueprintFunctions::SetTexCoord(FRuntimeMeshVertexTexCoordStream& Stream, FRuntimeMeshVertexTexCoordStream& OutStream, int32 Index, FVector2D NewTexCoord, int32 ChannelId /*= 0*/)
{
	OutStream = Stream;
	Stream.SetTexCoord(Index, NewTexCoord, ChannelId);
}


// Colors

FRuntimeMeshVertexColorStream URuntimeMeshBlueprintFunctions::MakeRuntimeMeshVertexColorStream(const TArray<FLinearColor> InColors)
{
	FRuntimeMeshVertexColorStream out;
	// We call this to avoid reimplementing the color conversion.
	URuntimeMeshBlueprintFunctions::AddColors(out, out, InColors);
	return out;
}

void URuntimeMeshBlueprintFunctions::SetNumColors(FRuntimeMeshVertexColorStream& Stream, FRuntimeMeshVertexColorStream& OutStream, int32 NewNum, bool bAllowShrinking /*= true*/)
{
	OutStream = Stream;
	Stream.SetNum(NewNum, bAllowShrinking);
}

void URuntimeMeshBlueprintFunctions::NumColors(FRuntimeMeshVertexColorStream& Stream, FRuntimeMeshVertexColorStream& OutStream, int32& OutNumColors)
{
	OutStream = Stream;
	OutNumColors = Stream.Num();
}

void URuntimeMeshBlueprintFunctions::EmptyColors(FRuntimeMeshVertexColorStream& Stream, FRuntimeMeshVertexColorStream& OutStream, int32 Slack /*= 0*/)
{
	OutStream = Stream;
	Stream.Empty(Slack);
}

void URuntimeMeshBlueprintFunctions::AddColor(FRuntimeMeshVertexColorStream& Stream, FRuntimeMeshVertexColorStream& OutStream, FLinearColor InColor, int32& OutIndex)
{
	OutStream = Stream;
	OutIndex = Stream.Add(InColor.ToFColor(false));
}

void URuntimeMeshBlueprintFunctions::AddColors(FRuntimeMeshVertexColorStream& Stream, FRuntimeMeshVertexColorStream& OutStream, const TArray<FLinearColor>& InColors)
{
	OutStream = Stream;
	TArray<FColor> colors;
	colors.SetNumUninitialized(InColors.Num());
	int idx = 0;
	for (auto& InColor : InColors)
	{
		colors[idx++] = InColor.ToFColor(false);
	}
	
	Stream.AddRange(colors);
}

void URuntimeMeshBlueprintFunctions::AppendColors(FRuntimeMeshVertexColorStream& Stream, FRuntimeMeshVertexColorStream& OutStream, const FRuntimeMeshVertexColorStream& InOther)
{
	OutStream = Stream;
	Stream.Append(InOther);
}

void URuntimeMeshBlueprintFunctions::GetColor(FRuntimeMeshVertexColorStream& Stream, FRuntimeMeshVertexColorStream& OutStream, int32 Index, FLinearColor& OutColor)
{
	OutStream = Stream;
	OutColor = FLinearColor(Stream.GetColor(Index));
}

void URuntimeMeshBlueprintFunctions::SetColor(FRuntimeMeshVertexColorStream& Stream, FRuntimeMeshVertexColorStream& OutStream, int32 Index, FLinearColor NewColor)
{
	OutStream = Stream;
	Stream.SetColor(Index, NewColor.ToFColor(false));
}


// Triangles
FRuntimeMeshTriangleStream URuntimeMeshBlueprintFunctions::MakeRuntimeMeshTriangleStream_Int32(const TArray<int32>& InIndices, bool bUse32BitIndices)
{
	FRuntimeMeshTriangleStream out(bUse32BitIndices);
	// We call this to avoid reimplementing the color conversion.
	URuntimeMeshBlueprintFunctions::AddIndices(out, out, InIndices);
	return out;
}

FRuntimeMeshTriangleStream URuntimeMeshBlueprintFunctions::MakeRuntimeMeshTriangleStream_Triangle(const TArray<FRuntimeMeshRenerableTriangleIndices>& InTriangles, bool bUse32BitIndices)
{
	FRuntimeMeshTriangleStream out(bUse32BitIndices);
	out.AddTriangles(InTriangles);
	return out;
}

void URuntimeMeshBlueprintFunctions::SetNumTriangles(FRuntimeMeshTriangleStream& Stream, FRuntimeMeshTriangleStream& OutStream, int32 NewNum, bool bAllowShrinking /*= true*/)
{
	OutStream = Stream;
	Stream.SetNum(NewNum, bAllowShrinking);
}

void URuntimeMeshBlueprintFunctions::NumIndices(FRuntimeMeshTriangleStream& Stream, FRuntimeMeshTriangleStream& OutStream, int32& OutNumIndices)
{
	OutStream = Stream;
	OutNumIndices = Stream.Num();
}

void URuntimeMeshBlueprintFunctions::NumTriangles(FRuntimeMeshTriangleStream& Stream, FRuntimeMeshTriangleStream& OutStream, int32& OutNumTriangles)
{
	OutStream = Stream;
	OutNumTriangles = Stream.NumTriangles();
}

void URuntimeMeshBlueprintFunctions::EmptyTriangles(FRuntimeMeshTriangleStream& Stream, FRuntimeMeshTriangleStream& OutStream, int32 Slack /*= 0*/)
{
	OutStream = Stream;
	Stream.Empty(Slack);
}

void URuntimeMeshBlueprintFunctions::AddIndex(FRuntimeMeshTriangleStream& Stream, FRuntimeMeshTriangleStream& OutStream, int32 NewIndex, int32& OutIndex)
{
	OutStream = Stream;
	OutIndex = Stream.Add(NewIndex);
}

void URuntimeMeshBlueprintFunctions::AddIndices(FRuntimeMeshTriangleStream& Stream,	FRuntimeMeshTriangleStream& OutStream, const TArray<int32>& InIndices)
{
	OutStream = Stream;
	TArray<uint32> indices;
	indices.SetNumUninitialized(InIndices.Num());
	int32 idx = 0;
	for (auto InIndex : InIndices)
	{
		indices[idx++] = InIndex;
	}
	Stream.AddRange(indices);
}

void URuntimeMeshBlueprintFunctions::AddTriangle(FRuntimeMeshTriangleStream& Stream, FRuntimeMeshTriangleStream& OutStream, int32 NewIndexA, int32 NewIndexB, int32 NewIndexC)
{
	OutStream = Stream;
	Stream.AddTriangle(NewIndexA, NewIndexB, NewIndexC);
}

void URuntimeMeshBlueprintFunctions::AddTriangleFromStructure(FRuntimeMeshTriangleStream& Stream, FRuntimeMeshTriangleStream& OutStream, const FRuntimeMeshRenerableTriangleIndices& InTriangle)
{
	OutStream = Stream;
	Stream.AddTriangle(InTriangle);
}

void URuntimeMeshBlueprintFunctions::AddTriangles(FRuntimeMeshTriangleStream& Stream, FRuntimeMeshTriangleStream& OutStream, const TArray<FRuntimeMeshRenerableTriangleIndices>& InTriangles)
{
	OutStream = Stream;
	Stream.AddTriangles(InTriangles);
}

void URuntimeMeshBlueprintFunctions::AppendTriangles(FRuntimeMeshTriangleStream& Stream, FRuntimeMeshTriangleStream& OutStream, const FRuntimeMeshTriangleStream& InOther)
{
	OutStream = Stream;
	Stream.Append(InOther);
}

void URuntimeMeshBlueprintFunctions::GetVertexIndex(FRuntimeMeshTriangleStream& Stream, FRuntimeMeshTriangleStream& OutStream, int32 Index, int32& OutIndex)
{
	OutStream = Stream;
	OutIndex = Stream.GetVertexIndex(Index);
}

void URuntimeMeshBlueprintFunctions::SetVertexIndex(FRuntimeMeshTriangleStream& Stream, FRuntimeMeshTriangleStream& OutStream, int32 Index, int32 NewIndex)
{
	OutStream = Stream;
	Stream.SetVertexIndex(Index, NewIndex);
}


// CollisionData

FRuntimeMeshCollisionVertexStream& URuntimeMeshBlueprintFunctions::GetCollisionVertexStream(FRuntimeMeshCollisionData& CollisionData, FRuntimeMeshCollisionData& OutCollisionData)
{
	OutCollisionData = CollisionData;
	return CollisionData.Vertices;
}

FRuntimeMeshCollisionTriangleStream& URuntimeMeshBlueprintFunctions::GetCollisionTriangleStream(FRuntimeMeshCollisionData& CollisionData, FRuntimeMeshCollisionData& OutCollisionData)
{
	OutCollisionData = CollisionData;
	return CollisionData.Triangles;
}

FRuntimeMeshCollisionTexCoordStream& URuntimeMeshBlueprintFunctions::GetCollisionTexCoordStream(FRuntimeMeshCollisionData& CollisionData, FRuntimeMeshCollisionData& OutCollisionData)
{
	OutCollisionData = CollisionData;
	return CollisionData.TexCoords;
}

FRuntimeMeshCollisionMaterialIndexStream& URuntimeMeshBlueprintFunctions::GetCollisionMaterialIndexStream(FRuntimeMeshCollisionData& CollisionData, FRuntimeMeshCollisionData& OutCollisionData)
{
	OutCollisionData = CollisionData;
	return CollisionData.MaterialIndices;
}


// Vertices

void URuntimeMeshBlueprintFunctions::SetNumCollisionVertices(FRuntimeMeshCollisionVertexStream& Stream, FRuntimeMeshCollisionVertexStream& OutStream, int32 NewNum, bool bAllowShrinking /*= true*/)
{
	OutStream = Stream;
	Stream.SetNum(NewNum, bAllowShrinking);
}

void URuntimeMeshBlueprintFunctions::NumCollisionVertices(FRuntimeMeshCollisionVertexStream& Stream, FRuntimeMeshCollisionVertexStream& OutStream, int32& OutNumVertices)
{
	OutStream = Stream;
	OutNumVertices = Stream.Num();
}

void URuntimeMeshBlueprintFunctions::EmptyCollisionVertices(FRuntimeMeshCollisionVertexStream& Stream, FRuntimeMeshCollisionVertexStream& OutStream, int32 Slack /*= 0*/)
{
	OutStream = Stream;
	Stream.Empty(Slack);
}

void URuntimeMeshBlueprintFunctions::AddCollisionVertex(FRuntimeMeshCollisionVertexStream& Stream, FRuntimeMeshCollisionVertexStream& OutStream, FVector InVertex, int32& OutIndex)
{
	OutStream = Stream;
	OutIndex = Stream.Add(InVertex);
}

void URuntimeMeshBlueprintFunctions::GetCollisionVertex(FRuntimeMeshCollisionVertexStream& Stream, FRuntimeMeshCollisionVertexStream& OutStream, int32 Index, FVector& OutVertex)
{
	OutStream = Stream;
	OutVertex = Stream.GetPosition(Index);
}

void URuntimeMeshBlueprintFunctions::SetCollisionVertex(FRuntimeMeshCollisionVertexStream& Stream, FRuntimeMeshCollisionVertexStream& OutStream, int32 Index, FVector NewVertex)
{
	OutStream = Stream;
	Stream.SetPosition(Index, NewVertex);
}


// Triangles

void URuntimeMeshBlueprintFunctions::SetNumCollisionTriangles(FRuntimeMeshCollisionTriangleStream& Stream, FRuntimeMeshCollisionTriangleStream& OutStream, int32 NewNum, bool bAllowShrinking /*= true*/)
{
	OutStream = Stream;
	Stream.SetNum(NewNum, bAllowShrinking);
}

void URuntimeMeshBlueprintFunctions::NumCollisionTriangles(FRuntimeMeshCollisionTriangleStream& Stream, FRuntimeMeshCollisionTriangleStream& OutStream, int32& OutNumTriangles)
{
	OutStream = Stream;
	OutNumTriangles = Stream.Num();
}

void URuntimeMeshBlueprintFunctions::EmptyCollisionTriangles(FRuntimeMeshCollisionTriangleStream& Stream, FRuntimeMeshCollisionTriangleStream& OutStream, int32 Slack /*= 0*/)
{
	OutStream = Stream;
	Stream.Empty(Slack);
}

void URuntimeMeshBlueprintFunctions::AddCollisionTriangle(FRuntimeMeshCollisionTriangleStream& Stream, FRuntimeMeshCollisionTriangleStream& OutStream, int32 NewIndexA, int32 NewIndexB, int32 NewIndexC, int32& OutTriangleIndex)
{
	OutStream = Stream;
	OutTriangleIndex = Stream.Add(NewIndexA, NewIndexB, NewIndexC);
}

void URuntimeMeshBlueprintFunctions::GetCollisionTriangle(FRuntimeMeshCollisionTriangleStream& Stream, FRuntimeMeshCollisionTriangleStream& OutStream, int32 TriangleIndex, int32& OutIndexA, int32& OutIndexB, int32& OutIndexC)
{
	OutStream = Stream;
	Stream.GetTriangleIndices(TriangleIndex, OutIndexA, OutIndexB, OutIndexC);
}


// Texture Coordinates

void URuntimeMeshBlueprintFunctions::SetNumCollisionTexCoords(FRuntimeMeshCollisionTexCoordStream& Stream, FRuntimeMeshCollisionTexCoordStream& OutStream, int32 NewNumChannels, int32 NewNumTexCoords, bool bAllowShrinking /*= true*/)
{
	OutStream = Stream;
	Stream.SetNum(NewNumChannels, NewNumTexCoords, bAllowShrinking);
}

void URuntimeMeshBlueprintFunctions::NumCollisionTexCoords(FRuntimeMeshCollisionTexCoordStream& Stream, FRuntimeMeshCollisionTexCoordStream& OutStream, int32 ChannelId, int32& OutNumTexCoords)
{
	OutStream = Stream;
	OutNumTexCoords = Stream.NumTexCoords(ChannelId);
}

void URuntimeMeshBlueprintFunctions::NumCollisionTexCoordChannels(FRuntimeMeshVertexTexCoordStream& Stream, FRuntimeMeshVertexTexCoordStream& OutStream, int32& OutNumTexCoordChannels)
{
	OutStream = Stream;
	OutNumTexCoordChannels = Stream.NumChannels();
}

void URuntimeMeshBlueprintFunctions::EmptyCollisionTexCoords(FRuntimeMeshVertexTexCoordStream& Stream, FRuntimeMeshVertexTexCoordStream& OutStream, int32 Slack /*= 0*/)
{
	OutStream = Stream;
	Stream.Empty(Slack);
}

void URuntimeMeshBlueprintFunctions::AddCollisionTexCoord(FRuntimeMeshVertexTexCoordStream& Stream, FRuntimeMeshVertexTexCoordStream& OutStream, FVector2D InTexCoord, int32& OutIndex)
{
	OutStream = Stream;
	OutIndex = Stream.Add(InTexCoord);
}

void URuntimeMeshBlueprintFunctions::GetCollisionTexCoord(FRuntimeMeshVertexTexCoordStream& Stream, FRuntimeMeshVertexTexCoordStream& OutStream, int32 Index, FVector2D& OutTexCoord, int32 ChannelId)
{
	OutStream = Stream;
	OutTexCoord = Stream.GetTexCoord(Index, ChannelId);
}

void URuntimeMeshBlueprintFunctions::SetCollisionTexCoord(FRuntimeMeshVertexTexCoordStream& Stream, FRuntimeMeshVertexTexCoordStream& OutStream, int32 Index, FVector2D NewTexCoord, int32 ChannelId /*= 0*/)
{
	OutStream = Stream;
	Stream.SetTexCoord(Index, NewTexCoord, ChannelId);
}


// Material Indices

void URuntimeMeshBlueprintFunctions::SetNumCollisionMaterialIndices(FRuntimeMeshCollisionMaterialIndexStream& Stream, FRuntimeMeshCollisionMaterialIndexStream& OutStream, int32 NewNum, bool bAllowShrinking /*= true*/)
{
	OutStream = Stream;
	Stream.SetNum(NewNum, bAllowShrinking);
}

void URuntimeMeshBlueprintFunctions::NumCollisionMaterialIndices(FRuntimeMeshCollisionMaterialIndexStream& Stream, FRuntimeMeshCollisionMaterialIndexStream& OutStream, int32& OutNumIndices)
{
	OutStream = Stream;
	OutNumIndices = Stream.Num();
}

void URuntimeMeshBlueprintFunctions::EmptyCollisionMaterialIndices(FRuntimeMeshCollisionMaterialIndexStream& Stream, FRuntimeMeshCollisionMaterialIndexStream& OutStream, int32 Slack /*= 0*/)
{
	OutStream = Stream;
	Stream.Empty(Slack);
}

void URuntimeMeshBlueprintFunctions::AddCollisionMaterialIndex(FRuntimeMeshCollisionMaterialIndexStream& Stream, FRuntimeMeshCollisionMaterialIndexStream& OutStream, int32 NewIndex, int32& OutIndex)
{
	OutStream = Stream;
	OutIndex = Stream.Add(NewIndex);
}

void URuntimeMeshBlueprintFunctions::GetCollisionMaterialIndex(FRuntimeMeshCollisionMaterialIndexStream& Stream, FRuntimeMeshCollisionMaterialIndexStream& OutStream, int32 Index, int32& OutIndex)
{
	OutStream = Stream;
	OutIndex = Stream.GetMaterialIndex(Index);
}

void URuntimeMeshBlueprintFunctions::SeCollisionMaterialIndex(FRuntimeMeshCollisionMaterialIndexStream& Stream, FRuntimeMeshCollisionMaterialIndexStream& OutStream, int32 Index, int32 NewIndex)
{
	OutStream = Stream;
	Stream.SetMaterialIndex(Index, NewIndex);
}


// Collision Settings

void URuntimeMeshBlueprintFunctions::AddCollisionBox(FRuntimeMeshCollisionSettings& Settings, FRuntimeMeshCollisionSettings& OutSettings, FRuntimeMeshCollisionBox NewBox)
{
	Settings.Boxes.Add(NewBox);
	OutSettings = Settings;
}

void URuntimeMeshBlueprintFunctions::AddCollisionSphere(FRuntimeMeshCollisionSettings& Settings, FRuntimeMeshCollisionSettings& OutSettings, FRuntimeMeshCollisionSphere NewSphere)
{
	Settings.Spheres.Add(NewSphere);
	OutSettings = Settings;
}

void URuntimeMeshBlueprintFunctions::AddCollisionCapsule(FRuntimeMeshCollisionSettings& Settings, FRuntimeMeshCollisionSettings& OutSettings, FRuntimeMeshCollisionCapsule NewCapsule)
{
	Settings.Capsules.Add(NewCapsule);
	OutSettings = Settings;
}

void URuntimeMeshBlueprintFunctions::AddCollisionConvex(FRuntimeMeshCollisionSettings& Settings, FRuntimeMeshCollisionSettings& OutSettings, FRuntimeMeshCollisionConvexMesh NewConvex)
{
	Settings.ConvexElements.Add(NewConvex);
	OutSettings = Settings;
}
