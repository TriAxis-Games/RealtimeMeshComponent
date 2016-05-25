// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "RuntimeMeshComponent.h"



template<typename VertexType>
class FRuntimeMeshBuilder
{
public:
	int32 AddVertex(const VertexType& InVertex);
	int32 AddVertex(const FVector& InPosition);
	int32 AddVertex(const FVector& InPosition, const FColor& InColor);
	int32 AddVertex(const FVector& InPosition, const FVector2D& InUV0);
	int32 AddVertex(const FVector& InPosition, const FVector& InNormal, const FRuntimeMeshTangent& InTangent, const FColor& InColor, const FVector2D& InUV0);
	int32 AddVertex(const FVector& InPosition, const FVector& InTangentX, const FRuntimeMeshTangent& InTangentY, const FRuntimeMeshTangent& InTangentZ, 
		const FColor& InColor, const FVector2D& InUV0);


	void SetPosition(const FVector& InPosition);

	void SetNormal(const FVector& InNormal);
	void SetTangent(const FRuntimeMeshTangent& InTangent);
	void SetTangents(const FVector& InTangentX, const FVector& InTangentY, const FVector& InTangentZ);

	void SetColor(const FColor& InColor);


	void SetUV0(const FVector2D& InUV0);
	void SetUV1(const FVector2D& InUV1);
	void SetUV2(const FVector2D& InUV2);
	void SetUV3(const FVector2D& InUV3);
	void SetUV4(const FVector2D& InUV4);
	void SetUV5(const FVector2D& InUV5);
	void SetUV6(const FVector2D& InUV6);
	void SetUV7(const FVector2D& InUV7);

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
	int32 Vertex1 = Mesh.AddVertex(FVector(0, 0, 0));
	Mesh.SetNormal(FVector(1, 0, 0));
	Mesh.SetUV0(FVector2D(0, 0));

	int32 Vertex2 = Mesh.AddVertex(FVector(1, 1, 1));
	Mesh.SetNormal(FVector(1, 0, 0));
	Mesh.SetUV0(FVector2D(1, 1));

	int32 Vertex3 = Mesh.AddVertex(FVector(2, 2, 2));
	Mesh.SetNormal(FVector(1, 0, 0));
	Mesh.SetUV0(FVector2D(1, 1));


	// Add the triangle
	Mesh.AddTriangle(Vertex1, Vertex2, Vertex3);


	// Update the 3 vertices positions, leaving everything else the same
	Mesh.SeekVertices(0);
	Mesh.AddVertex(FVector(5, 5, 5));
	Mesh.AddVertex(FVector(6, 6, 6));
	Mesh.AddVertex(FVector(7, 7, 7));



	Mesh.SeekTriangles(0);
	Mesh.AddTriangle(Vertex3, Vertex2, Vertex1);



}