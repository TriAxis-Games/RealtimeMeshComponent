// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RealtimeMeshProcedural.h"
#include "RealtimeMeshProceduralMeshLibrary.generated.h"

class URealtimeMeshProcedural;

/**
 * Blueprint helpers mirroring UKismetProceduralMeshLibrary so that content built
 * against UProceduralMeshComponent can migrate to URealtimeMeshProcedural with a
 * find-and-replace. These produce / consume the same plain CPU arrays the PMC
 * helpers did; geometry generation matches PMC, while CalculateTangentsForMesh
 * is backed by RMC's own RealtimeMeshAlgo::GenerateTangents.
 *
 * Static-mesh conversion helpers (GetSectionFromStaticMesh /
 * CopyProceduralMeshFromStaticMeshComponent) are intentionally not mirrored here;
 * use the RealtimeMeshExt static-mesh converter for that.
 */
UCLASS(meta = (ScriptName = "RealtimeMeshProceduralMeshLibrary"))
class REALTIMEMESHCOMPONENT_API URealtimeMeshProceduralMeshLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Generate vertices/triangles/normals/UVs/tangents for a box of the given radius (half-extent). */
	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh|Procedural")
	static void GenerateBoxMesh(FVector BoxRadius,
								TArray<FVector>& Vertices,
								TArray<int32>& Triangles,
								TArray<FVector>& Normals,
								TArray<FVector2D>& UVs,
								TArray<FRealtimeMeshProceduralTangent>& Tangents);

	/**
	 * Calculate per-vertex normals and tangents for the given geometry. UVs are
	 * optional — when supplied (matching the vertex count) they drive the tangent
	 * basis; otherwise an edge-based tangent is used. Backed by RMC's
	 * RealtimeMeshAlgo::GenerateTangents.
	 */
	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh|Procedural", meta = (AutoCreateRefTerm = "UVs"))
	static void CalculateTangentsForMesh(const TArray<FVector>& Vertices,
										 const TArray<int32>& Triangles,
										 const TArray<FVector2D>& UVs,
										 TArray<FVector>& Normals,
										 TArray<FRealtimeMeshProceduralTangent>& Tangents,
										 bool bComputeSmoothNormals = true);

	/** Append two triangles forming the quad (Vert0, Vert1, Vert2, Vert3) to the index buffer. */
	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh|Procedural")
	static void ConvertQuadToTriangles(UPARAM(ref) TArray<int32>& Triangles, int32 Vert0, int32 Vert1, int32 Vert2, int32 Vert3);

	/** Generate an index buffer for a NumX by NumY grid of vertices. bWinding flips the triangle winding. */
	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh|Procedural")
	static void CreateGridMeshTriangles(int32 NumX, int32 NumY, bool bWinding, TArray<int32>& Triangles);

	/** Generate a welded NumX by NumY grid: shared vertices, triangles, and UVs. */
	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh|Procedural")
	static void CreateGridMeshWelded(int32 NumX, int32 NumY,
									 TArray<int32>& Triangles,
									 TArray<FVector>& Vertices,
									 TArray<FVector2D>& UVs,
									 float GridSpacing = 16.0f);

	/** Generate a split NumX by NumY grid: per-quad vertices with both a per-quad UV (UVs) and a quad-center UV (UV1s). */
	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh|Procedural")
	static void CreateGridMeshSplit(int32 NumX, int32 NumY,
									TArray<int32>& Triangles,
									TArray<FVector>& Vertices,
									TArray<FVector2D>& UVs,
									TArray<FVector2D>& UV1s,
									float GridSpacing = 16.0f);

	/**
	 * Grab the geometry of one section back out of a procedural mesh. PMC-parity
	 * wrapper over URealtimeMeshProcedural::GetMeshSection — returns UV channel 0
	 * only and drops vertex colors. Use GetMeshSection directly for the full set.
	 */
	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh|Procedural")
	static void GetSectionFromProceduralMesh(URealtimeMeshProcedural* InProcMesh, int32 SectionIndex,
											 TArray<FVector>& Vertices,
											 TArray<int32>& Triangles,
											 TArray<FVector>& Normals,
											 TArray<FVector2D>& UVs,
											 TArray<FRealtimeMeshProceduralTangent>& Tangents);
};
