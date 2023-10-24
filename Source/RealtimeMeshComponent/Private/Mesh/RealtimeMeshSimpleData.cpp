// Copyright TriAxis Games, L.L.C. All Rights Reserved.


#include "Mesh/RealtimeMeshSimpleData.h"


FRealtimeMeshSimpleMeshData URealtimeMeshSimpleBlueprintFunctionLibrary::MakeRealtimeMeshSimpleStream(const TArray<int32>& Triangles, const TArray<FVector>& Positions,
	const TArray<FVector>& Normals, const TArray<FVector>& Tangents, const TArray<FVector>& Binormals, const TArray<FLinearColor>& LinearColors, const TArray<FVector2D>& UV0,
	const TArray<FVector2D>& UV1, const TArray<FVector2D>& UV2, const TArray<FVector2D>& UV3, const TArray<FColor>& Colors, bool bUseHighPrecisionTangents, bool bUseHighPrecisionTexCoords)
{
	FRealtimeMeshSimpleMeshData NewMeshData;
	NewMeshData.Triangles = Triangles;
	NewMeshData.Positions = Positions;
	NewMeshData.Normals = Normals;
	NewMeshData.Tangents = Tangents;
	NewMeshData.Binormals = Binormals;
	NewMeshData.LinearColors = LinearColors;
	NewMeshData.UV0 = UV0;
	NewMeshData.UV1 = UV1;
	NewMeshData.UV2 = UV2;
	NewMeshData.UV3 = UV3;
	NewMeshData.Colors = Colors;
	NewMeshData.bUseHighPrecisionTangents = bUseHighPrecisionTangents;
	NewMeshData.bUseHighPrecisionTexCoords = bUseHighPrecisionTexCoords;
	return NewMeshData;
}