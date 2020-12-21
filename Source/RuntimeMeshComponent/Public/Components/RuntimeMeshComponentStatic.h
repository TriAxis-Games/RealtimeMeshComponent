// Copyright 2016-2020 TriAxis Games L.L.C. All Rights Reserved.

#pragma once

#include "RuntimeMeshComponent.h"
#include "RuntimeMeshComponentStatic.generated.h"

class URuntimeMeshProviderStatic;
class URuntimeMeshModifierNormals;
class URuntimeMeshModifierAdjacency;
/**
*	RMC but with automatic static provider, to work like PMC or RMC3 and down
*/
UCLASS(ClassGroup=(Rendering, Common), HideCategories=(Object, Activation, "Components|Activation"), ShowCategories=(Mobility), Meta = (BlueprintSpawnableComponent))
class RUNTIMEMESHCOMPONENT_API URuntimeMeshComponentStatic : public URuntimeMeshComponent
{
	GENERATED_BODY()

private:
	UPROPERTY( EditAnywhere, Category = "Mesh Sharing", Meta = (ExposeOnSpawn = "true", AllowPrivateAccess = "true"), BlueprintGetter = GetRuntimeMesh)
	URuntimeMesh* RuntimeMesh;

	UPROPERTY()
	URuntimeMeshProviderStatic* StaticProvider;

	UPROPERTY()
	URuntimeMeshModifierNormals* NormalsModifier;

	UPROPERTY()
	URuntimeMeshModifierAdjacency* AdjacencyModifier;


public:

	URuntimeMeshComponentStatic();

	void OnRegister() override;

	UFUNCTION(BlueprintCallable, Category = "RuntimeMeshStatic|Mesh", Meta = (DisplayName = "Create Section"))
	void CreateSection_Blueprint(int32 LODIndex, int32 SectionId, const FRuntimeMeshSectionProperties& SectionProperties, const FRuntimeMeshRenderableMeshData& SectionData);

	/**
	*	Create/replace a section for this runtime mesh.
	*	@param	LODIndex			Index of the LOD to create the section in.
	*	@param	SectionIndex		Index of the section to create or replace.
	*	@param	MaterialSlot		Index of the material to use for this section
	*	@param	Vertices			Vertex buffer of all vertex positions to use for this mesh section.
	*	@param	Triangles			Index buffer indicating which vertices make up each triangle. Length must be a multiple of 3.
	*	@param	Normals				Optional array of normal vectors for each vertex. If supplied, must be same length as Vertices array.
	*	@param	UV0					Optional array of texture co-ordinates for each vertex. If supplied, must be same length as Vertices array.
	*	@param	UV1					Optional array of texture co-ordinates for each vertex. If supplied, must be same length as Vertices array.
	*	@param	UV2					Optional array of texture co-ordinates for each vertex. If supplied, must be same length as Vertices array.
	*	@param	UV3					Optional array of texture co-ordinates for each vertex. If supplied, must be same length as Vertices array.
	*	@param	VertexColors		Optional array of colors for each vertex. If supplied, must be same length as Vertices array.
	*	@param	Tangents			Optional array of tangent vector for each vertex. If supplied, must be same length as Vertices array.
	*	@param	UpdateFrequency		How frequently this section is expected to be updated, Infrequent draws faster than Average/Frequent but updates slower
	*	@param	bCreateCollision	Indicates whether collision should be created for this section. This adds significant cost.
	*/
	UFUNCTION(BlueprintCallable, Category = "RuntimeMeshStatic|Mesh", Meta = (DisplayName = "Create Section From Components", AutoCreateRefTerm = "Normals,UV0,UV1,UV2,UV3,VertexColors,Tangents", AdvancedDisplay = "UV1,UV2,UV3"))
	void CreateSectionFromComponents(int32 LODIndex, int32 SectionIndex, int32 MaterialSlot, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals,
		const TArray<FVector2D>& UV0, const TArray<FVector2D>& UV1, const TArray<FVector2D>& UV2, const TArray<FVector2D>& UV3, const TArray<FLinearColor>& VertexColors,
		const TArray<FRuntimeMeshTangent>& Tangents, ERuntimeMeshUpdateFrequency UpdateFrequency = ERuntimeMeshUpdateFrequency::Infrequent, bool bCreateCollision = true);

	/**
	*	Create/replace a section for this runtime mesh.
	*	@param	LODIndex			Index of the LOD to create the section in.
	*	@param	SectionIndex		Index of the section to create or replace.
	*	@param	MaterialSlot		Index of the material to use for this section
	*	@param	Vertices			Vertex buffer of all vertex positions to use for this mesh section.
	*	@param	Triangles			Index buffer indicating which vertices make up each triangle. Length must be a multiple of 3.
	*	@param	Normals				Optional array of normal vectors for each vertex. If supplied, must be same length as Vertices array.
	*	@param	UV0					Optional array of texture co-ordinates for each vertex. If supplied, must be same length as Vertices array.
	*	@param	UV1					Optional array of texture co-ordinates for each vertex. If supplied, must be same length as Vertices array.
	*	@param	UV2					Optional array of texture co-ordinates for each vertex. If supplied, must be same length as Vertices array.
	*	@param	UV3					Optional array of texture co-ordinates for each vertex. If supplied, must be same length as Vertices array.
	*	@param	VertexColors		Optional array of colors for each vertex. If supplied, must be same length as Vertices array.
	*	@param	Tangents			Optional array of tangent vector for each vertex. If supplied, must be same length as Vertices array.
	*	@param	UpdateFrequency		How frequently this section is expected to be updated, Infrequent draws faster than Average/Frequent but updates slower
	*	@param	bCreateCollision	Indicates whether collision should be created for this section. This adds significant cost.
	*/
	void CreateSectionFromComponents(int32 LODIndex, int32 SectionIndex, int32 MaterialSlot, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals,
		const TArray<FVector2D>& UV0, const TArray<FVector2D>& UV1, const TArray<FVector2D>& UV2, const TArray<FVector2D>& UV3, const TArray<FColor>& VertexColors,
		const TArray<FRuntimeMeshTangent>& Tangents, ERuntimeMeshUpdateFrequency UpdateFrequency = ERuntimeMeshUpdateFrequency::Infrequent, bool bCreateCollision = true);

	/**
	*	Create/replace a section for this runtime mesh.
	*	@param	LODIndex			Index of the LOD to create the section in.
	*	@param	SectionIndex		Index of the section to create or replace.
	*	@param	MaterialSlot		Index of the material to use for this section
	*	@param	Vertices			Vertex buffer of all vertex positions to use for this mesh section.
	*	@param	Triangles			Index buffer indicating which vertices make up each triangle. Length must be a multiple of 3.
	*	@param	Normals				Optional array of normal vectors for each vertex. If supplied, must be same length as Vertices array.
	*	@param	UV0					Optional array of texture co-ordinates for each vertex. If supplied, must be same length as Vertices array.
	*	@param	VertexColors		Optional array of colors for each vertex. If supplied, must be same length as Vertices array.
	*	@param	Tangents			Optional array of tangent vector for each vertex. If supplied, must be same length as Vertices array.
	*	@param	UpdateFrequency		How frequently this section is expected to be updated, Infrequent draws faster than Average/Frequent but updates slower
	*	@param	bCreateCollision	Indicates whether collision should be created for this section. This adds significant cost.
	*/
	void CreateSectionFromComponents(int32 LODIndex, int32 SectionIndex, int32 MaterialSlot, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals,
		const TArray<FVector2D>& UV0, const TArray<FLinearColor>& VertexColors, const TArray<FRuntimeMeshTangent>& Tangents,
		ERuntimeMeshUpdateFrequency UpdateFrequency = ERuntimeMeshUpdateFrequency::Infrequent, bool bCreateCollision = true);


	/**
	*	Create/replace a section for this runtime mesh.
	*	@param	LODIndex			Index of the LOD to create the section in.
	*	@param	SectionIndex		Index of the section to create or replace.
	*	@param	MaterialSlot		Index of the material to use for this section
	*	@param	Vertices			Vertex buffer of all vertex positions to use for this mesh section.
	*	@param	Triangles			Index buffer indicating which vertices make up each triangle. Length must be a multiple of 3.
	*	@param	Normals				Optional array of normal vectors for each vertex. If supplied, must be same length as Vertices array.
	*	@param	UV0					Optional array of texture co-ordinates for each vertex. If supplied, must be same length as Vertices array.
	*	@param	VertexColors		Optional array of colors for each vertex. If supplied, must be same length as Vertices array.
	*	@param	Tangents			Optional array of tangent vector for each vertex. If supplied, must be same length as Vertices array.
	*	@param	UpdateFrequency		How frequently this section is expected to be updated, Infrequent draws faster than Average/Frequent but updates slower
	*	@param	bCreateCollision	Indicates whether collision should be created for this section. This adds significant cost.
	*/
	void CreateSectionFromComponents(int32 LODIndex, int32 SectionIndex, int32 MaterialSlot, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals,
		const TArray<FVector2D>& UV0, const TArray<FColor>& VertexColors, const TArray<FRuntimeMeshTangent>& Tangents,
		ERuntimeMeshUpdateFrequency UpdateFrequency = ERuntimeMeshUpdateFrequency::Infrequent, bool bCreateCollision = true);

	UFUNCTION(BlueprintCallable, Category = "RuntimeMeshStatic|Mesh", Meta = (DisplayName = "Update Section"))
	void UpdateSection_Blueprint(int32 LODIndex, int32 SectionId, const FRuntimeMeshRenderableMeshData& SectionData);

	/**
	*	Update the mesh data of a section.
	*	@param	LODIndex			Index of the LOD to create the section in.
	*	@param	SectionIndex		Index of the section to create or replace.
	*	@param	Vertices			Vertex buffer of all vertex positions to use for this mesh section.
	*	@param	Triangles			Index buffer indicating which vertices make up each triangle. Length must be a multiple of 3.
	*	@param	Normals				Optional array of normal vectors for each vertex. If supplied, must be same length as Vertices array.
	*	@param	UV0					Optional array of texture co-ordinates for each vertex. If supplied, must be same length as Vertices array.
	*	@param	UV1					Optional array of texture co-ordinates for each vertex. If supplied, must be same length as Vertices array.
	*	@param	UV2					Optional array of texture co-ordinates for each vertex. If supplied, must be same length as Vertices array.
	*	@param	UV3					Optional array of texture co-ordinates for each vertex. If supplied, must be same length as Vertices array.
	*	@param	VertexColors		Optional array of colors for each vertex. If supplied, must be same length as Vertices array.
	*	@param	Tangents			Optional array of tangent vector for each vertex. If supplied, must be same length as Vertices array.
	*/
	UFUNCTION(BlueprintCallable, Category = "RuntimeMeshStatic|Mesh", Meta = (DisplayName = "Update Section From Components", AutoCreateRefTerm = "Normals,UV0,UV1,UV2,UV3,VertexColors,Tangents", AdvancedDisplay = "UV1,UV2,UV3"))
	void UpdateSectionFromComponents(int32 LODIndex, int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals, const TArray<FVector2D>& UV0,
		const TArray<FVector2D>& UV1, const TArray<FVector2D>& UV2, const TArray<FVector2D>& UV3, const TArray<FLinearColor>& VertexColors, const TArray<FRuntimeMeshTangent>& Tangents);

	/**
	*	Update the mesh data of a section.
	*	@param	LODIndex			Index of the LOD to create the section in.
	*	@param	SectionIndex		Index of the section to create or replace.
	*	@param	Vertices			Vertex buffer of all vertex positions to use for this mesh section.
	*	@param	Triangles			Index buffer indicating which vertices make up each triangle. Length must be a multiple of 3.
	*	@param	Normals				Optional array of normal vectors for each vertex. If supplied, must be same length as Vertices array.
	*	@param	UV0					Optional array of texture co-ordinates for each vertex. If supplied, must be same length as Vertices array.
	*	@param	UV1					Optional array of texture co-ordinates for each vertex. If supplied, must be same length as Vertices array.
	*	@param	UV2					Optional array of texture co-ordinates for each vertex. If supplied, must be same length as Vertices array.
	*	@param	UV3					Optional array of texture co-ordinates for each vertex. If supplied, must be same length as Vertices array.
	*	@param	VertexColors		Optional array of colors for each vertex. If supplied, must be same length as Vertices array.
	*	@param	Tangents			Optional array of tangent vector for each vertex. If supplied, must be same length as Vertices array.
	*/
	void UpdateSectionFromComponents(int32 LODIndex, int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals, const TArray<FVector2D>& UV0,
		const TArray<FVector2D>& UV1, const TArray<FVector2D>& UV2, const TArray<FVector2D>& UV3, const TArray<FColor>& VertexColors, const TArray<FRuntimeMeshTangent>& Tangents);

	/**
	*	Update the mesh data of a section.
	*	@param	LODIndex			Index of the LOD to create the section in.
	*	@param	SectionIndex		Index of the section to create or replace.
	*	@param	Vertices			Vertex buffer of all vertex positions to use for this mesh section.
	*	@param	Triangles			Index buffer indicating which vertices make up each triangle. Length must be a multiple of 3.
	*	@param	Normals				Optional array of normal vectors for each vertex. If supplied, must be same length as Vertices array.
	*	@param	UV0					Optional array of texture co-ordinates for each vertex. If supplied, must be same length as Vertices array.
	*	@param	VertexColors		Optional array of colors for each vertex. If supplied, must be same length as Vertices array.
	*	@param	Tangents			Optional array of tangent vector for each vertex. If supplied, must be same length as Vertices array.
	*/
	void UpdateSectionFromComponents(int32 LODIndex, int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals,
		const TArray<FVector2D>& UV0, const TArray<FLinearColor>& VertexColors, const TArray<FRuntimeMeshTangent>& Tangents);


	/**
	*	Update the mesh data of a section.
	*	@param	LODIndex			Index of the LOD to create the section in.
	*	@param	SectionIndex		Index of the section to create or replace.
	*	@param	Vertices			Vertex buffer of all vertex positions to use for this mesh section.
	*	@param	Triangles			Index buffer indicating which vertices make up each triangle. Length must be a multiple of 3.
	*	@param	Normals				Optional array of normal vectors for each vertex. If supplied, must be same length as Vertices array.
	*	@param	UV0					Optional array of texture co-ordinates for each vertex. If supplied, must be same length as Vertices array.
	*	@param	VertexColors		Optional array of colors for each vertex. If supplied, must be same length as Vertices array.
	*	@param	Tangents			Optional array of tangent vector for each vertex. If supplied, must be same length as Vertices array.
	*/
	void UpdateSectionFromComponents(int32 LODIndex, int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals,
		const TArray<FVector2D>& UV0, const TArray<FColor>& VertexColors, const TArray<FRuntimeMeshTangent>& Tangents);

	UFUNCTION(BlueprintCallable, Category = "RuntimeMeshStatic|Mesh", Meta = (DisplayName = "Clear Section"))
	void ClearSection(int32 LODIndex, int32 SectionId);

	UFUNCTION(BlueprintCallable, Category = "RuntimeMeshStatic|Mesh", Meta = (DisplayName = "Clear Section"))
	void RemoveSection(int32 LODIndex, int32 SectionId);



	UFUNCTION(BlueprintPure, Category = "RuntimeMeshStatic|Collision")
	FRuntimeMeshCollisionSettings GetCollisionSettings() const;

	UFUNCTION(BlueprintCallable, Category = "RuntimeMeshStatic|Collision")
	void SetCollisionSettings(const FRuntimeMeshCollisionSettings& NewCollisionSettings);

	UFUNCTION(BlueprintPure, Category = "RuntimeMeshStatic|Collision")
	FRuntimeMeshCollisionData GetCollisionMesh() const;

	UFUNCTION(BlueprintCallable, Category = "RuntimeMeshStatic|Collision")
	void SetCollisionMesh(const FRuntimeMeshCollisionData& NewCollisionMesh);

	UFUNCTION(BlueprintPure, Category = "RuntimeMeshStatic|Collision")
	int32 GetLODForMeshCollision() const;

	UFUNCTION(BlueprintCallable, Category = "RuntimeMeshStatic|Collision")
	void SetRenderableLODForCollision(int32 LODIndex);

	UFUNCTION(BlueprintPure, Category = "RuntimeMeshStatic|Collision")
	TSet<int32> GetSectionsForMeshCollision() const;

	UFUNCTION(BlueprintCallable, Category = "RuntimeMeshStatic|Collision")
	void SetRenderableSectionAffectsCollision(int32 SectionId, bool bCollisionEnabled);



	UFUNCTION(BlueprintCallable, Category = "RuntimeMeshStatic|Mesh")
	TArray<int32> GetSectionIds(int32 LODIndex) const;

	UFUNCTION(BlueprintCallable, Category = "RuntimeMeshStatic|Mesh")
	int32 GetLastSectionId(int32 LODIndex) const;

	UFUNCTION(BlueprintCallable, Category = "RuntimeMeshStatic|Mesh")
	bool DoesSectionHaveValidMeshData(int32 LODIndex, int32 SectionId) const;

	UFUNCTION(BlueprintCallable, Category = "RuntimeMeshStatic|Mesh")
	void RemoveAllSectionsForLOD(int32 LODIndex);

	UFUNCTION(BlueprintCallable, Category = "RuntimeMeshStatic|Mesh")
	FBoxSphereBounds GetSectionBounds(int32 LODIndex, int32 SectionId) const;

	UFUNCTION(BlueprintCallable, Category = "RuntimeMeshStatic|Mesh")
	FRuntimeMeshSectionProperties GetSectionProperties(int32 LODIndex, int32 SectionId) const;

	UFUNCTION(BlueprintCallable, Category = "RuntimeMeshStatic|Mesh")
	FRuntimeMeshRenderableMeshData GetSectionRenderData(int32 LODIndex, int32 SectionId) const;

	UFUNCTION(BlueprintCallable, Category = "RuntimeMeshStatic|Mesh")
	FRuntimeMeshRenderableMeshData GetSectionRenderDataAndClear(int32 LODIndex, int32 SectionId);




	UFUNCTION(BlueprintCallable, Category = "RuntimeMeshStatic")
	URuntimeMeshProviderStatic* GetStaticProvider();


	UFUNCTION(BlueprintCallable, Category = "RuntimeMeshStatic|Normals")
	void EnableNormalTangentGeneration();

	UFUNCTION(BlueprintCallable, Category = "RuntimeMeshStatic|Normals")
	void DisableNormalTangentGeneration();

	UFUNCTION(BlueprintCallable, Category = "RuntimeMeshStatic|Normals")
	bool HasNormalTangentGenerationEnabled() const;


	UFUNCTION(BlueprintCallable, Category = "RuntimeMeshStatic|Tessellation")
	void EnabledTessellationTrianglesGeneration();

	UFUNCTION(BlueprintCallable, Category = "RuntimeMeshStatic|Tessellation")
	void DisableTessellationTrianglesGeneration();

	UFUNCTION(BlueprintCallable, Category = "RuntimeMeshStatic|Tessellation")
	bool HasTessellationTriangleGenerationEnabled() const;


};
