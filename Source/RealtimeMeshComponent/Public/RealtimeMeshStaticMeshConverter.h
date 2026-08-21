// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Core/RealtimeMeshDataStream.h"
#include "UObject/Object.h"
#include "Engine/StaticMesh.h"
#include "RealtimeMeshStaticMeshConverter.generated.h"


class URealtimeMeshSimple;
class URealtimeMeshStreamSet;


USTRUCT(BlueprintType)
struct REALTIMEMESHCOMPONENT_API FStreamSetStaticMeshConversionOptions
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RealtimeMesh|Conversion")
	int32 LODIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RealtimeMesh|Conversion")
	bool bWantTangents = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RealtimeMesh|Conversion")
	bool bWantUVs = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RealtimeMesh|Conversion")
	bool bWantVertexColors = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RealtimeMesh|Conversion")
	bool bWantPolyGroups = true;
};

USTRUCT(BlueprintType)
struct REALTIMEMESHCOMPONENT_API FRealtimeMeshStaticMeshConversionOptions
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RealtimeMesh|Conversion")
	int32 MinLODIndex = 0;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RealtimeMesh|Conversion")
	int32 MaxLODIndex = REALTIME_MESH_MAX_LOD_INDEX;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RealtimeMesh|Conversion")
	bool bWantTangents = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RealtimeMesh|Conversion")
	bool bWantUVs = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RealtimeMesh|Conversion")
	bool bWantVertexColors = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RealtimeMesh|Conversion")
	bool bWantPolyGroups = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RealtimeMesh|Conversion")
	bool bWantsMaterials = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RealtimeMesh|Conversion")
	bool bWantsDistanceField = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RealtimeMesh|Conversion")
	bool bWantsLumenCards = true;

	/*
	 * Copies the nanite representation of the mesh
	 * WARNING: Only available in RMC-Pro
	 */
	/*UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RealtimeMesh|Conversion")
	bool bWantsNanite = true;*/
};

/**
 * 
 */
UCLASS()
class REALTIMEMESHCOMPONENT_API URealtimeMeshStaticMeshConverter : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:

	static bool CopyStreamSetToStaticMesh(const RealtimeMesh::FRealtimeMeshStreamSet& InStreamSet, UStaticMesh* OutStaticMesh, const FStreamSetStaticMeshConversionOptions& Options = FStreamSetStaticMeshConversionOptions());

	static bool CopyStreamSetFromStaticMesh(const UStaticMesh* InStaticMesh, RealtimeMesh::FRealtimeMeshStreamSet& OutStreamSet, const FStreamSetStaticMeshConversionOptions& Options = FStreamSetStaticMeshConversionOptions());

private:

	UFUNCTION(BlueprintCallable, Category = "RealtimeMesh|Conversion", meta = (ExpandEnumAsExecs = "Outcome"))
	static UPARAM(DisplayName = "Stream Set") URealtimeMeshStreamSet* CopyStreamSetFromStaticMesh(
		UStaticMesh* FromStaticMeshAsset, 
		URealtimeMeshStreamSet* ToStreamSet, 
		FStreamSetStaticMeshConversionOptions Options,
		ERealtimeMeshOutcomePins& Outcome);

	UFUNCTION(BlueprintCallable, Category = "RealtimeMesh|Conversion", meta = (ExpandEnumAsExecs = "Outcome"))
	static UPARAM(DisplayName = "Static Mesh") UStaticMesh* CopyStreamSetToStaticMesh(
		URealtimeMeshStreamSet* FromStreamSet, 
		UStaticMesh* ToStaticMeshAsset,
		FStreamSetStaticMeshConversionOptions Options,
		ERealtimeMeshOutcomePins& Outcome);
	
public:
	
	UFUNCTION(BlueprintCallable, Category = "RealtimeMesh|Conversion", meta = (ExpandEnumAsExecs = "Outcome"))
	static UPARAM(DisplayName = "Realtime Mesh") URealtimeMeshSimple* CopyRealtimeMeshFromStaticMesh(
		UStaticMesh* FromStaticMeshAsset, 
		URealtimeMeshSimple* ToRealtimeMesh, 
		FRealtimeMeshStaticMeshConversionOptions Options,
		ERealtimeMeshOutcomePins& Outcome);

	UFUNCTION(BlueprintCallable, Category = "RealtimeMesh|Conversion", meta = (ExpandEnumAsExecs = "Outcome"))
	static UPARAM(DisplayName = "Static Mesh") UStaticMesh* CopyRealtimeMeshToStaticMesh(
		URealtimeMeshSimple* FromRealtimeMesh, 
		UStaticMesh* ToStaticMeshAsset,
		FRealtimeMeshStaticMeshConversionOptions Options,
		ERealtimeMeshOutcomePins& Outcome);
private:
	
	// These read FROM a static mesh INTO a stream set. _RenderData reads the cooked render data
	// (works at runtime with bAllowCPUAccess); _SourceData reads the editor source MeshDescription
	// (higher fidelity, editor-only). CopyStreamSetFromStaticMesh prefers source data in-editor.
	static bool CopyStreamSetFromStaticMesh_RenderData(const UStaticMesh* InStaticMesh, RealtimeMesh::FRealtimeMeshStreamSet& OutStreamSet, const FStreamSetStaticMeshConversionOptions& Options = FStreamSetStaticMeshConversionOptions());
	static bool CopyStreamSetFromStaticMesh_SourceData(const UStaticMesh* InStaticMesh, RealtimeMesh::FRealtimeMeshStreamSet& OutStreamSet, const FStreamSetStaticMeshConversionOptions& Options = FStreamSetStaticMeshConversionOptions());
	
};
