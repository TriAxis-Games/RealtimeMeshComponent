// Copyright 2016-2020 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "RuntimeMeshComponent.h"
#include "RuntimeMeshComponentStatic.generated.h"

class URuntimeMeshProviderStatic;
class URuntimeMeshProviderNormals;
/**
*	RMC but with backed-in providers for PMC-like operation
*/
UCLASS(ClassGroup=(Rendering, Common), HideCategories=(Object, Activation, "Components|Activation"), ShowCategories=(Mobility), Meta = (BlueprintSpawnableComponent))
class RUNTIMEMESHCOMPONENT_API URuntimeMeshComponentStatic : public URuntimeMeshComponent
{
	GENERATED_BODY()

public:
	//These are only for defaults when spawning, unused at runtime (after OnRegister)

	//Sets if normals and tangents should be generated (Will add another provider)
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="Post Processing", Meta = (ExposeOnSpawn = "true"))
	bool bWantsNormals;
	//Set if normals should be generated (Will add another provider)
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Post Processing", Meta = (EditCondition = "!bWantsNormals", ExposeOnSpawn = "true"))
	bool bWantsTangents;
	//LOD to use for complex collision
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Post Processing", Meta = (ExposeOnSpawn = "true"), BlueprintSetter = SetRenderableLODForCollision)
	int32 LODForMeshCollision;
	//Sections to use to gather complex collision data
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Post Processing", Meta = (ExposeOnSpawn = "true"))
	TSet<int32> SectionsForMeshCollision;
	//Collision settings
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Post Processing", Meta = (ExposeOnSpawn = "true"), BlueprintSetter = SetCollisionSettings)
	FRuntimeMeshCollisionSettings CollisionSettings;
	//Collision mesh to add before section-gathered complex collision. Independant from bWantsCollision
	UPROPERTY(BlueprintReadWrite, Category = "Post Processing", Meta = (ExposeOnSpawn = "true"), BlueprintSetter = SetCollisionMesh)
	FRuntimeMeshCollisionData CollisionMesh;

	URuntimeMeshProviderStatic* StaticProvider;
	URuntimeMeshProviderNormals* NormalsProvider;

	URuntimeMeshComponentStatic(const FObjectInitializer& ObjectInitializer);

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

	UFUNCTION(BlueprintCallable, Category = "RuntimeMeshStatic|Collision")
	void SetCollisionSettings(const FRuntimeMeshCollisionSettings& NewCollisionSettings);

	UFUNCTION(BlueprintCallable, Category = "RuntimeMeshStatic|Collision")
	void SetCollisionMesh(const FRuntimeMeshCollisionData& NewCollisionMesh);

	UFUNCTION(BlueprintCallable, Category = "RuntimeMeshStatic|Collision")
	void SetRenderableLODForCollision(int32 LODIndex);

	UFUNCTION(BlueprintCallable, Category = "RuntimeMeshStatic|Collision")
	void SetRenderableSectionAffectsCollision(int32 SectionId, bool bCollisionEnabled);

private:
	void GetOrCreateNormalsProvider();

	//Only to be called when the reference to the Normals provider is valid, in order to remove it if it computes nothing
	void EnsureNormalsProviderIsWanted();

public:

	//Avoid calling at runtime, as this will re-run initialization. Prefer using defaults
	UFUNCTION(BlueprintCallable, Category = "RuntimeMeshStatic|Normals", Meta = (DisplayName = "Set Wants Normals"))
	void SetbWantsNormals(bool InbWantsNormals, bool bDeleteNormalsProviderIfUseless = false);

	//Avoid calling at runtime, as this will re-run initialization. Prefer using defaults
	UFUNCTION(BlueprintCallable, Category = "RuntimeMeshStatic|Normals", Meta = (DisplayName = "Set Wants Tangents"))
	void SetbWantsTangents(bool InbWantsTangents, bool bDeleteNormalsProviderIfUseless = false);
};
