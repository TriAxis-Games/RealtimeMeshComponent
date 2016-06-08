// Copyright 2016 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "RuntimeMeshComponent.h"

//////////////////////////////////////////////////////////////////////////
//	This feature in development. Not usable yet.
//////////////////////////////////////////////////////////////////////////

template<typename VertexType>
class FRuntimeMeshBuilder
{
public:
	int32 SetVertex(const VertexType& InVertex);
	int32 SetVertex(const FVector& InPosition);
	int32 SetVertex(const FVector& InPosition, const FColor& InColor);
	int32 SetVertex(const FVector& InPosition, const FVector2D& InUV0);
	int32 SetVertex(const FVector& InPosition, const FVector& InNormal, const FRuntimeMeshTangent& InTangent, const FColor& InColor, const FVector2D& InUV0);
	int32 SetVertex(const FVector& InPosition, const FVector& InTangentX, const FVector& InTangentY, const FVector& InTangentZ,
		const FColor& InColor, const FVector2D& InUV0);


	void SetPosition(const FVector& InPosition);
	void SetNormal(const FVector& InNormal);
	void SetTangent(const FRuntimeMeshTangent& InTangent);
	void SetTangents(const FVector& InTangentX, const FVector& InTangentY, const FVector& InTangentZ);
	void SetColor(const FColor& InColor);
	void SetUV(int32 Channel, const FVector2D& UV);


	void MoveNext();


	int32 AddTriangle(int32 V0, int32 V1, int32 V2);

	int32 AddVertices(const TArray<VertexType>& Vertices);
	int32 AddTriangles(const TArray<int32>& Triangles);

	void SeekVertices(int32 StreamPosition);
	void SeekTriangles(int32 StreamPosition);


	








};



void Test()
{
	FRuntimeMeshBuilder<FRuntimeMeshVertexSimple> Mesh;

	// Create 3 vertices setting position, normal, and UV0
	int32 Vertex1 = Mesh.SetVertex(FVector(0, 0, 0));
	Mesh.SetNormal(FVector(1, 0, 0));
	Mesh.SetUV(0, FVector2D(0, 0));
	Mesh.MoveNext();

	int32 Vertex2 = Mesh.AddVertex(FVector(1, 1, 1));
	Mesh.SetNormal(FVector(1, 0, 0));
	Mesh.SetUV(0, FVector2D(1, 1));
	Mesh.MoveNext();

	int32 Vertex3 = Mesh.AddVertex(FVector(2, 2, 2));
	Mesh.SetNormal(FVector(1, 0, 0));
	Mesh.SetUV(0, FVector2D(1, 1));
	Mesh.MoveNext();


	// Add the triangle
	Mesh.AddTriangle(Vertex1, Vertex2, Vertex3);


	// Update the 3 vertices positions, leaving everything else the same
	Mesh.SeekVertices(0);
	Mesh.SetVertex(FVector(5, 5, 5));
	Mesh.MoveNext();
	Mesh.SetVertex(FVector(6, 6, 6));
	Mesh.MoveNext();
	Mesh.SetVertex(FVector(7, 7, 7));
	Mesh.MoveNext();



	Mesh.SeekTriangles(0);
	Mesh.AddTriangle(Vertex3, Vertex2, Vertex1);



}