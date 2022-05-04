// Copyright 2016-2020 TriAxis Games L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RuntimeMeshRenderable.h"
#include "RuntimeMeshCollision.h"
#include "StaticMeshResources.h"
#include "RuntimeMeshStaticMeshConverter.generated.h"

class URuntimeMeshComponent;
/**
 * 
 */
UCLASS()
class RUNTIMEMESHCOMPONENT_API URuntimeMeshStaticMeshConverter : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

private:
	static bool CheckStaticMeshAccessible(UStaticMesh* StaticMesh);
	
	static int32 CopyVertexOrGetIndex(const FStaticMeshLODResources& LOD, const FStaticMeshSection& Section, TMap<int32, int32>& MeshToSectionVertexMap, int32 VertexIndex, FRuntimeMeshRenderableMeshData& NewMeshData);
	
	static int32 CopyVertexOrGetIndex(const FStaticMeshLODResources& LOD, const FStaticMeshSection& Section, TMap<int32, int32>& MeshToSectionVertexMap, int32 VertexIndex, int32 NumUVChannels, FRuntimeMeshCollisionData& NewMeshData);

public:
	static bool CopyStaticMeshSectionToRenderableMeshData(UStaticMesh* StaticMesh, int32 LODIndex, int32 SectionId,
	FRuntimeMeshRenderableMeshData& OutMeshData);
	
	UFUNCTION(BlueprintCallable, Category = "RuntimeMesh|StaticMeshConversion")
	static bool CopyStaticMeshSectionToRenderableMeshData(UStaticMesh* StaticMesh, int32 LODIndex, int32 SectionId,
	FRuntimeMeshRenderableMeshData& OutMeshData, FRuntimeMeshSectionProperties& OutProperties);
	
	UFUNCTION(BlueprintCallable, Category = "RuntimeMesh|StaticMeshConversion")
	static bool CopyStaticMeshCollisionToCollisionSettings(UStaticMesh* StaticMesh,
		FRuntimeMeshCollisionSettings& OutCollisionSettings);
	
	UFUNCTION(BlueprintCallable, Category = "RuntimeMesh|StaticMeshConversion")
	static bool CopyStaticMeshLODToCollisionData(UStaticMesh* StaticMesh, int32 LODIndex,
		FRuntimeMeshCollisionData& OutCollisionData);

	UFUNCTION(BlueprintCallable, Category = "RuntimeMesh|StaticMeshConversion")
	static bool CopyStaticMeshToRuntimeMesh(UStaticMesh* StaticMesh, URuntimeMeshComponent* RuntimeMeshComponent,
		int32 CollisionLODIndex = -1, int32 MaxLODToCopy = 8);

	static bool CopyStaticMeshData(UStaticMesh* StaticMesh,
	                               TArray<TArray<FRuntimeMeshSectionData>>& OutSectionData, TArray<FRuntimeMeshLODProperties>& OutLODProperties,
	                               TArray<FStaticMaterial>& OutMaterials, FRuntimeMeshCollisionData& OutCollisionData,
	                               FRuntimeMeshCollisionSettings& OutCollisionSettings,
	                               int32 MaxLODToCopy = 8);

	UFUNCTION(BlueprintCallable, Category = "RuntimeMesh|StaticMeshConversion")
	static bool CopyStaticMeshComponentToRuntimeMesh(UStaticMeshComponent* StaticMeshComponent,
		URuntimeMeshComponent* RuntimeMeshComponent, int32 CollisionLODIndex = -1, int32 MaxLODToCopy = 8);
};
