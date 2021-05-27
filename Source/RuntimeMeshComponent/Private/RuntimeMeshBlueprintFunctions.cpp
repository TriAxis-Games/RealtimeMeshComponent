// Copyright 2016-2020 TriAxis Games L.L.C. All Rights Reserved.


#include "RuntimeMeshBlueprintFunctions.h"
#include "RuntimeMesh.h"




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

void URuntimeMeshBlueprintFunctions::AddTexCoord(FRuntimeMeshVertexTexCoordStream& Stream, FRuntimeMeshVertexTexCoordStream& OutStream, int32& OutIndex, FVector2D InTexCoord, int32 ChannelId)
{
	OutStream = Stream;
	OutIndex = Stream.Add(InTexCoord, ChannelId);
}

void URuntimeMeshBlueprintFunctions::AppendTexCoords(FRuntimeMeshVertexTexCoordStream& Stream, FRuntimeMeshVertexTexCoordStream& OutStream, const FRuntimeMeshVertexTexCoordStream& InOther)
{
	OutStream = Stream;
	Stream.Append(InOther);
}

void URuntimeMeshBlueprintFunctions::GetTexCoord(FRuntimeMeshVertexTexCoordStream& Stream, FRuntimeMeshVertexTexCoordStream& OutStream, int32 Index, int32 ChannelId, FVector2D& OutTexCoord)
{
	OutStream = Stream;
	OutTexCoord = Stream.GetTexCoord(Index, ChannelId);
}

void URuntimeMeshBlueprintFunctions::SetTexCoord(FRuntimeMeshVertexTexCoordStream& Stream, FRuntimeMeshVertexTexCoordStream& OutStream, int32 Index, FVector2D NewTexCoord, int32 ChannelId /*= 0*/)
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

void URuntimeMeshBlueprintFunctions::AddColor(FRuntimeMeshVertexColorStream& Stream, FRuntimeMeshVertexColorStream& OutStream, FLinearColor InColor, int32& OutIndex)
{
	OutStream = Stream;
	OutIndex = Stream.Add(InColor.ToFColor(false));
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
