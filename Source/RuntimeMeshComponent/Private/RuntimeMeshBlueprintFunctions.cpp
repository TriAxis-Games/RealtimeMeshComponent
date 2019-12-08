// Copyright 2016-2019 Chris Conway (Koderz). All Rights Reserved.


#include "RuntimeMeshBlueprintFunctions.h"

// MeshData

void URuntimeMeshBlueprintFunctions::GetPositionStream(FRuntimeMeshRenderableMeshData& MeshData, FRuntimeMeshRenderableMeshData& OutMeshData, FRuntimeMeshVertexPositionStream& OutPositionStream)
{
	OutMeshData = MeshData;
	OutPositionStream = MeshData.Positions;
}

void URuntimeMeshBlueprintFunctions::GetTangentStream(FRuntimeMeshRenderableMeshData& MeshData, FRuntimeMeshRenderableMeshData& OutMeshData, FRuntimeMeshVertexTangentStream& OutTangentStream)
{
	OutMeshData = MeshData;
	OutTangentStream = MeshData.Tangents;
}

void URuntimeMeshBlueprintFunctions::GetTexCoordStream(FRuntimeMeshRenderableMeshData& MeshData, FRuntimeMeshRenderableMeshData& OutMeshData, FRuntimeMeshVertexTexCoordStream& OutTexCoordStream)
{
	OutMeshData = MeshData;
	OutTexCoordStream = MeshData.TexCoords;
}

void URuntimeMeshBlueprintFunctions::GetColorStream(FRuntimeMeshRenderableMeshData& MeshData, FRuntimeMeshRenderableMeshData& OutMeshData, FRuntimeMeshVertexColorStream& OutColorStream)
{
	OutMeshData = MeshData;
	OutColorStream = MeshData.Colors;
}

void URuntimeMeshBlueprintFunctions::GetTriangleStream(FRuntimeMeshRenderableMeshData& MeshData, FRuntimeMeshRenderableMeshData& OutMeshData, FRuntimeMeshTriangleStream& OutTriangleStream)
{
	OutMeshData = MeshData;
	OutTriangleStream = MeshData.Triangles;
}

void URuntimeMeshBlueprintFunctions::GetAdjacencyTriangleStream(FRuntimeMeshRenderableMeshData& MeshData, FRuntimeMeshRenderableMeshData& OutMeshData, FRuntimeMeshTriangleStream& OutAdjacencyTriangleStream)
{
	OutMeshData = MeshData;
	OutAdjacencyTriangleStream = MeshData.AdjacencyTriangles;
}


// Positions

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

void URuntimeMeshBlueprintFunctions::AddNormalAndTangent(FRuntimeMeshVertexTangentStream& Stream, FRuntimeMeshVertexTangentStream& OutStream, const FVector& InNormal, const FVector& InTangent, int32& OutIndex)
{
	OutStream = Stream;
	OutIndex = Stream.Add(InNormal, InTangent);
}

void URuntimeMeshBlueprintFunctions::AddTangents(FRuntimeMeshVertexTangentStream& Stream, FRuntimeMeshVertexTangentStream& OutStream, const FVector& InTangentX, const FVector& InTangentY, const FVector& InTangentZ, int32& OutIndex)
{
	OutStream = Stream;
	OutIndex = Stream.Add(InTangentX, InTangentY, InTangentZ);
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

void URuntimeMeshBlueprintFunctions::AddTexCoord(FRuntimeMeshVertexTexCoordStream& Stream, FRuntimeMeshVertexTexCoordStream& OutStream, const FVector2D& InTexCoord, int32& OutIndex)
{
	OutStream = Stream;
	OutIndex = Stream.Add(InTexCoord);
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

void URuntimeMeshBlueprintFunctions::SetTexCoord(FRuntimeMeshVertexTexCoordStream& Stream, FRuntimeMeshVertexTexCoordStream& OutStream, int32 Index, const FVector2D& NewTexCoord, int32 ChannelId /*= 0*/)
{
	OutStream = Stream;
	Stream.SetTexCoord(Index, NewTexCoord, ChannelId);
}


// Colors

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

void URuntimeMeshBlueprintFunctions::AddColor(FRuntimeMeshVertexColorStream& Stream, FRuntimeMeshVertexColorStream& OutStream, const FColor& InColor, int32& OutIndex)
{
	OutStream = Stream;
	OutIndex = Stream.Add(InColor);
}

void URuntimeMeshBlueprintFunctions::AppendColors(FRuntimeMeshVertexColorStream& Stream, FRuntimeMeshVertexColorStream& OutStream, const FRuntimeMeshVertexColorStream& InOther)
{
	OutStream = Stream;
	Stream.Append(InOther);
}

void URuntimeMeshBlueprintFunctions::GetColor(FRuntimeMeshVertexColorStream& Stream, FRuntimeMeshVertexColorStream& OutStream, int32 Index, FColor& OutColor)
{
	OutStream = Stream;
	OutColor = Stream.GetColor(Index);
}

void URuntimeMeshBlueprintFunctions::SetColor(FRuntimeMeshVertexColorStream& Stream, FRuntimeMeshVertexColorStream& OutStream, int32 Index, const FColor& NewColor)
{
	OutStream = Stream;
	Stream.SetColor(Index, NewColor);
}


// Triangles

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

void URuntimeMeshBlueprintFunctions::AddTriangle(FRuntimeMeshTriangleStream& Stream, FRuntimeMeshTriangleStream& OutStream, int32 NewIndexA, int32 NewIndexB, int32 NewIndexC)
{
	OutStream = Stream;
	Stream.AddTriangle(NewIndexA, NewIndexB, NewIndexC);
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

void URuntimeMeshBlueprintFunctions::GetCollisionVertexStream(FRuntimeMeshCollisionData& CollisionData, FRuntimeMeshCollisionData& OutCollisionData, FRuntimeMeshCollisionVertexStream& OutPositionStream)
{
	OutCollisionData = CollisionData;
	OutPositionStream = CollisionData.Vertices;
}

void URuntimeMeshBlueprintFunctions::GetCollisionTriangleStream(FRuntimeMeshCollisionData& CollisionData, FRuntimeMeshCollisionData& OutCollisionData, FRuntimeMeshCollisionTriangleStream& OutTriangleStream)
{
	OutCollisionData = CollisionData;
	OutTriangleStream = CollisionData.Triangles;
}

void URuntimeMeshBlueprintFunctions::GetCollisionTexCoordStream(FRuntimeMeshCollisionData& CollisionData, FRuntimeMeshCollisionData& OutCollisionData, FRuntimeMeshCollisionTexCoordStream& OutTexCoordStream)
{
	OutCollisionData = CollisionData;
	OutTexCoordStream = CollisionData.TexCoords;
}

void URuntimeMeshBlueprintFunctions::GetCollisionMaterialIndexStream(FRuntimeMeshCollisionData& CollisionData, FRuntimeMeshCollisionData& OutCollisionData, FRuntimeMeshCollisionMaterialIndexStream& OutMaterialIndexStream)
{
	OutCollisionData = CollisionData;
	OutMaterialIndexStream = CollisionData.MaterialIndices;
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

void URuntimeMeshBlueprintFunctions::AddCollisionTexCoord(FRuntimeMeshVertexTexCoordStream& Stream, FRuntimeMeshVertexTexCoordStream& OutStream, const FVector2D& InTexCoord, int32& OutIndex)
{
	OutStream = Stream;
	OutIndex = Stream.Add(InTexCoord);
}

void URuntimeMeshBlueprintFunctions::GetCollisionTexCoord(FRuntimeMeshVertexTexCoordStream& Stream, FRuntimeMeshVertexTexCoordStream& OutStream, int32 Index, FVector2D& OutTexCoord, int32 ChannelId)
{
	OutStream = Stream;
	OutTexCoord = Stream.GetTexCoord(Index, ChannelId);
}

void URuntimeMeshBlueprintFunctions::SetCollisionTexCoord(FRuntimeMeshVertexTexCoordStream& Stream, FRuntimeMeshVertexTexCoordStream& OutStream, int32 Index, const FVector2D& NewTexCoord, int32 ChannelId /*= 0*/)
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
