// Copyright 2016-2019 Gabriel Zerbib (Moddingear). All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "RuntimeMeshManager.h"
#include "RuntimeMeshBlueprint.h"
#include "RuntimeMeshManagerBP.generated.h"

/**
 * 
 */
USTRUCT(BlueprintType)
struct /*RUNTIMEMESHCOMPONENT_API*/ FRuntimeMeshDataStructBP //Do not uncomment the API, otherwise you'll get unresolved externals
{
	GENERATED_BODY()

	TArray<FRuntimeMeshBlueprintVertexSimple> Vertices;
	TArray<int32> Triangles;

	FRuntimeMeshDataStructBP() {}

	FRuntimeMeshDataStructBP(TArray<FRuntimeMeshBlueprintVertexSimple> InVertices, TArray<int32> InTriangles) {
		Vertices = InVertices;
		Triangles = InTriangles;
	}

	FRuntimeMeshDataStructBP(FRuntimeMeshDataStruct<FRuntimeMeshBlueprintVertexSimple, int32> InMesh)
	{
		Vertices = InMesh.Vertices;
		Triangles = InMesh.Triangles;
	}

	operator FRuntimeMeshDataStruct<FRuntimeMeshBlueprintVertexSimple, int32>() {
		return FRuntimeMeshDataStruct<FRuntimeMeshBlueprintVertexSimple, int32>(Vertices, Triangles);
	}
};

UCLASS(BlueprintType)
class RUNTIMEMESHCOMPONENT_API URuntimeMeshManagerBP : public UObject
{
	GENERATED_BODY()

public:
	URuntimeMeshManagerBP();

	virtual UWorld* GetWorld() const override //Fix visibility of functions
	{
		if (HasAllFlags(RF_ClassDefaultObject))
		{
			// If we are a CDO, we must return nullptr instead of calling Outer->GetWorld() to fool UObject::ImplementsGetWorld.
			return nullptr;
		}
		return GetOuter()->GetWorld();
	}

	FRuntimeMeshManager<int32, FRuntimeMeshBlueprintVertexSimple, int32> RuntimeMeshManager;

	/* Used to know if a mesh is already present in the mesh manager */
	UFUNCTION(BlueprintPure, Category = "RuntimeMeshManager")
	bool HasSubmesh(int32 id);
	
	/* Returns the full mesh that combines all provided meshes */
	UFUNCTION(BlueprintCallable, Category = "RuntimeMeshManager")
	FRuntimeMeshDataStructBP GetMesh();

	/* Returns the internal array of the saved meshes' data, in correct order
	Only useful if you want to iterate on the meshes */
	/*UFUNCTION(BlueprintCallable, Category = "RuntimeMeshManager")
	TArray<int32> GetMeshDataArray()
	{
		return RuntimeMeshManager.GetMeshData();
	}*/

	/* Returns the mesh that was saved at the ID */
	UFUNCTION(BlueprintCallable, Category = "RuntimeMeshManager")
	FRuntimeMeshDataStructBP GetSubmesh(int32 id);

	/* Returns the mesh that was saved at the index
	Only useful when iterating */
	/*UFUNCTION(BlueprintCallable, Category = "RuntimeMeshManager")
	FRuntimeMeshDataStructBP GetSubmeshAtIndex(int32 index)
	{
		return RuntimeMeshManager.GetMeshAtIndex(index);
	}*/

	/* Adds or updates the mesh for the specified ID
	Giving a mesh with an ID that is already registered will cause it to update
	Use the same vert and tris length to avoid recreating the array */
	UFUNCTION(BlueprintCallable, Category = "RuntimeMeshManager")
	void AddSubmesh(int32 id, FRuntimeMeshDataStructBP mesh);

	/* Removes the submesh that was linked to the supplied id */
	UFUNCTION(BlueprintCallable, Category = "RuntimeMeshManager")
	void RemoveSubmesh(int32 id);

};
