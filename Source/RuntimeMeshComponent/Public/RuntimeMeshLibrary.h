// Copyright 2016 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "RuntimeMeshComponent.h"
#include "RuntimeMeshLibrary.generated.h"

UCLASS()
class RUNTIMEMESHCOMPONENT_API URuntimeMeshLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()

	/** Add a quad, specified by four indices, to a triangle index buffer as two triangles. */
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	static void ConvertQuadToTriangles(UPARAM(ref) TArray<int32>& Triangles, int32 Vert0, int32 Vert1, int32 Vert2, int32 Vert3);

	/**
	*	Generate an index buffer for a grid of quads.
	*	@param	NumX			Number of vertices in X direction (must be >= 2)
	*	@param	NumY			Number of vertices in y direction (must be >= 2)
	*	@param	bWinding		Reverses winding of indices generated for each quad
	*	@out	Triangles		Output index buffer
	*/
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	static void CreateGridMeshTriangles(int32 NumX, int32 NumY, bool bWinding, TArray<int32>& Triangles);

	/** Generate vertex and index buffer for a simple box, given the supplied dimensions. Normals, UVs and tangents are also generated for each vertex. */
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	static void CreateBoxMesh(FVector BoxRadius, TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector>& Normals, TArray<FVector2D>& UVs, TArray<FRuntimeMeshTangent>& Tangents);
	
	/**
	*	Automatically generate normals and tangent vectors for a mesh
	*	UVs are required for correct tangent generation.
	*/
// 	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh", meta = (AutoCreateRefTerm = "UVs"))
// 	static void CalculateTangentsForMesh(const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector2D>& UVs, TArray<FVector>& Normals, TArray<FRuntimeMeshTangent>& Tangents);
// 


	static void GetSectionFromStaticMesh(UStaticMesh* InMesh, int32 LODIndex, int32 SectionIndex,
		IRuntimeMeshVerticesBuilder* Vertices, FRuntimeMeshIndicesBuilder* Triangles, FRuntimeMeshIndicesBuilder* AdjacencyTriangles);

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	static void CopyRuntimeMeshFromStaticMeshComponent(UStaticMeshComponent* StaticMeshComp, int32 LODIndex, 
		URuntimeMeshComponent* RuntimeMeshComp, bool bShouldCreateCollision);

	
// 	/** Grab geometry data from a StaticMesh asset. */
// 	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
// 	static void GetSectionFromStaticMesh(UStaticMesh* InMesh, int32 LODIndex, int32 SectionIndex, TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector>& Normals, TArray<FVector2D>& UVs, TArray<FProcMeshTangent>& Tangents);
// 


	static void GenerateTessellationIndexBuffer(const IRuntimeMeshVerticesBuilder* Vertices, const FRuntimeMeshIndicesBuilder* Indices, FRuntimeMeshIndicesBuilder* OutTessellationIndices);




	
};