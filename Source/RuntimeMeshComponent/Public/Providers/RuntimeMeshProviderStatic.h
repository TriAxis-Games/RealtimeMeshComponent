//// Copyright 2016-2020 Chris Conway (Koderz). All Rights Reserved.
//
//#pragma once
//
//#include "CoreMinimal.h"
//#include "RuntimeMeshProvider.h"
//#include "RuntimeMeshProviderStatic.generated.h"
//
//UCLASS(HideCategories = Object, BlueprintType)
//class RUNTIMEMESHCOMPONENT_API URuntimeMeshProviderStatic : public URuntimeMeshProvider
//{
//	GENERATED_BODY()
//
//protected:
//	UPROPERTY(Category = "RuntimeMeshActor", EditAnywhere, BlueprintReadOnly, Meta = (ExposeFunctionCategories = "Mesh,Rendering,Physics,Components|RuntimeMesh", AllowPrivateAccess = "true"))
//	bool StoreEditorGeneratedDataForGame;
//private:
//
//	using FSectionDataMapEntry = TTuple<FRuntimeMeshSectionProperties, FRuntimeMeshRenderableMeshData, FBoxSphereBounds>;
//
//private:
//	TArray<FRuntimeMeshLODProperties> LODConfigurations;
//	TMap<int32, TMap<int32, FSectionDataMapEntry>> SectionDataMap;
//
//	int32 LODForMeshCollision;
//	TSet<int32> SectionsForMeshCollision;
//
//	FRuntimeMeshCollisionSettings CollisionSettings;
//	TOptional<FRuntimeMeshCollisionData> CollisionMesh;
//	FBoxSphereBounds CombinedBounds;
//
//	TArray<FRuntimeMeshMaterialSlot> LoadedMaterialSlots;
//public:
//
//	URuntimeMeshProviderStatic();
//
//	void Initialize_Implementation() override;
//	
//	UFUNCTION(BlueprintCallable, Category = "RuntimeMesh|Providers|Static", Meta=(DisplayName = "Create Section"))
//	void CreateSection_Blueprint(int32 LODIndex, int32 SectionId, const FRuntimeMeshSectionProperties& SectionProperties, const FRuntimeMeshRenderableMeshData& SectionData)
//	{
//		URuntimeMeshProvider::CreateSection(LODIndex, SectionId, SectionProperties);
//		UpdateSectionInternal(LODIndex, SectionId, SectionData, GetBoundsFromMeshData(SectionData));
//	}
//
//	UFUNCTION(BlueprintCallable, Category = "RuntimeMesh|Providers|Static", Meta = (DisplayName = "Update Section"))
//	void UpdateSection_Blueprint(int32 LODIndex, int32 SectionId, const FRuntimeMeshRenderableMeshData& SectionData)
//	{
//		UpdateSectionInternal(LODIndex, SectionId, SectionData, GetBoundsFromMeshData(SectionData));
//	}
//
//	/**
//	*	Create/replace a section for this runtime mesh.
//	*	@param	LODIndex			Index of the LOD to create the section in.
//	*	@param	SectionIndex		Index of the section to create or replace.
//	*	@param	MaterialSlot		Index of the material to use for this section
//	*	@param	Vertices			Vertex buffer of all vertex positions to use for this mesh section.
//	*	@param	Triangles			Index buffer indicating which vertices make up each triangle. Length must be a multiple of 3.
//	*	@param	Normals				Optional array of normal vectors for each vertex. If supplied, must be same length as Vertices array.
//	*	@param	UV0					Optional array of texture co-ordinates for each vertex. If supplied, must be same length as Vertices array.
//	*	@param	UV1					Optional array of texture co-ordinates for each vertex. If supplied, must be same length as Vertices array.
//	*	@param	UV2					Optional array of texture co-ordinates for each vertex. If supplied, must be same length as Vertices array.
//	*	@param	UV3					Optional array of texture co-ordinates for each vertex. If supplied, must be same length as Vertices array.
//	*	@param	VertexColors		Optional array of colors for each vertex. If supplied, must be same length as Vertices array.
//	*	@param	Tangents			Optional array of tangent vector for each vertex. If supplied, must be same length as Vertices array.
//	*	@param	UpdateFrequency		How frequently this section is expected to be updated, Infrequent draws faster than Average/Frequent but updates slower
//	*	@param	bCreateCollision	Indicates whether collision should be created for this section. This adds significant cost.
//	*/
//	UFUNCTION(BlueprintCallable, Category = "RuntimeMesh|Providers|Static", Meta = (DisplayName = "Create Section From Components", AutoCreateRefTerm = "Normals,UV0,UV1,UV2,UV3,VertexColors,Tangents", AdvancedDisplay = "UV1,UV2,UV3"))
//	void CreateSectionFromComponents(int32 LODIndex, int32 SectionIndex, int32 MaterialSlot, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals,
//		const TArray<FVector2D>& UV0, const TArray<FVector2D>& UV1, const TArray<FVector2D>& UV2, const TArray<FVector2D>& UV3, const TArray<FLinearColor>& VertexColors, 
//		const TArray<FRuntimeMeshTangent>& Tangents, ERuntimeMeshUpdateFrequency UpdateFrequency = ERuntimeMeshUpdateFrequency::Infrequent, bool bCreateCollision = true);
//	
//
//	/**
//	*	Create/replace a section for this runtime mesh.
//	*	@param	LODIndex			Index of the LOD to create the section in.
//	*	@param	SectionIndex		Index of the section to create or replace.
//	*	@param	MaterialSlot		Index of the material to use for this section
//	*	@param	Vertices			Vertex buffer of all vertex positions to use for this mesh section.
//	*	@param	Triangles			Index buffer indicating which vertices make up each triangle. Length must be a multiple of 3.
//	*	@param	Normals				Optional array of normal vectors for each vertex. If supplied, must be same length as Vertices array.
//	*	@param	UV0					Optional array of texture co-ordinates for each vertex. If supplied, must be same length as Vertices array.
//	*	@param	UV1					Optional array of texture co-ordinates for each vertex. If supplied, must be same length as Vertices array.
//	*	@param	UV2					Optional array of texture co-ordinates for each vertex. If supplied, must be same length as Vertices array.
//	*	@param	UV3					Optional array of texture co-ordinates for each vertex. If supplied, must be same length as Vertices array.
//	*	@param	VertexColors		Optional array of colors for each vertex. If supplied, must be same length as Vertices array.
//	*	@param	Tangents			Optional array of tangent vector for each vertex. If supplied, must be same length as Vertices array.
//	*	@param	UpdateFrequency		How frequently this section is expected to be updated, Infrequent draws faster than Average/Frequent but updates slower
//	*	@param	bCreateCollision	Indicates whether collision should be created for this section. This adds significant cost.
//	*/
//	void CreateSectionFromComponents(int32 LODIndex, int32 SectionIndex, int32 MaterialSlot, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals,
//		const TArray<FVector2D>& UV0, const TArray<FVector2D>& UV1, const TArray<FVector2D>& UV2, const TArray<FVector2D>& UV3, const TArray<FColor>& VertexColors, 
//		const TArray<FRuntimeMeshTangent>& Tangents, ERuntimeMeshUpdateFrequency UpdateFrequency = ERuntimeMeshUpdateFrequency::Infrequent, bool bCreateCollision = true);
//
//	/**
//	*	Create/replace a section for this runtime mesh.
//	*	@param	LODIndex			Index of the LOD to create the section in.
//	*	@param	SectionIndex		Index of the section to create or replace.
//	*	@param	MaterialSlot		Index of the material to use for this section
//	*	@param	Vertices			Vertex buffer of all vertex positions to use for this mesh section.
//	*	@param	Triangles			Index buffer indicating which vertices make up each triangle. Length must be a multiple of 3.
//	*	@param	Normals				Optional array of normal vectors for each vertex. If supplied, must be same length as Vertices array.
//	*	@param	UV0					Optional array of texture co-ordinates for each vertex. If supplied, must be same length as Vertices array.
//	*	@param	VertexColors		Optional array of colors for each vertex. If supplied, must be same length as Vertices array.
//	*	@param	Tangents			Optional array of tangent vector for each vertex. If supplied, must be same length as Vertices array.
//	*	@param	UpdateFrequency		How frequently this section is expected to be updated, Infrequent draws faster than Average/Frequent but updates slower
//	*	@param	bCreateCollision	Indicates whether collision should be created for this section. This adds significant cost.
//	*/
//	void CreateSectionFromComponents(int32 LODIndex, int32 SectionIndex, int32 MaterialSlot, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals,
//		const TArray<FVector2D>& UV0, const TArray<FLinearColor>& VertexColors, const TArray<FRuntimeMeshTangent>& Tangents,
//		ERuntimeMeshUpdateFrequency UpdateFrequency = ERuntimeMeshUpdateFrequency::Infrequent, bool bCreateCollision = true);
//
//
//	/**
//	*	Create/replace a section for this runtime mesh.
//	*	@param	LODIndex			Index of the LOD to create the section in.
//	*	@param	SectionIndex		Index of the section to create or replace.
//	*	@param	MaterialSlot		Index of the material to use for this section
//	*	@param	Vertices			Vertex buffer of all vertex positions to use for this mesh section.
//	*	@param	Triangles			Index buffer indicating which vertices make up each triangle. Length must be a multiple of 3.
//	*	@param	Normals				Optional array of normal vectors for each vertex. If supplied, must be same length as Vertices array.
//	*	@param	UV0					Optional array of texture co-ordinates for each vertex. If supplied, must be same length as Vertices array.
//	*	@param	VertexColors		Optional array of colors for each vertex. If supplied, must be same length as Vertices array.
//	*	@param	Tangents			Optional array of tangent vector for each vertex. If supplied, must be same length as Vertices array.
//	*	@param	UpdateFrequency		How frequently this section is expected to be updated, Infrequent draws faster than Average/Frequent but updates slower
//	*	@param	bCreateCollision	Indicates whether collision should be created for this section. This adds significant cost.
//	*/
//	void CreateSectionFromComponents(int32 LODIndex, int32 SectionIndex, int32 MaterialSlot, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals,
//		const TArray<FVector2D>& UV0, const TArray<FColor>& VertexColors, const TArray<FRuntimeMeshTangent>& Tangents,
//		ERuntimeMeshUpdateFrequency UpdateFrequency = ERuntimeMeshUpdateFrequency::Infrequent, bool bCreateCollision = true);
//
//
//
//
//	/**
//	*	Update the mesh data of a section.
//	*	@param	LODIndex			Index of the LOD to create the section in.
//	*	@param	SectionIndex		Index of the section to create or replace.
//	*	@param	Vertices			Vertex buffer of all vertex positions to use for this mesh section.
//	*	@param	Triangles			Index buffer indicating which vertices make up each triangle. Length must be a multiple of 3.
//	*	@param	Normals				Optional array of normal vectors for each vertex. If supplied, must be same length as Vertices array.
//	*	@param	UV0					Optional array of texture co-ordinates for each vertex. If supplied, must be same length as Vertices array.
//	*	@param	UV1					Optional array of texture co-ordinates for each vertex. If supplied, must be same length as Vertices array.
//	*	@param	UV2					Optional array of texture co-ordinates for each vertex. If supplied, must be same length as Vertices array.
//	*	@param	UV3					Optional array of texture co-ordinates for each vertex. If supplied, must be same length as Vertices array.
//	*	@param	VertexColors		Optional array of colors for each vertex. If supplied, must be same length as Vertices array.
//	*	@param	Tangents			Optional array of tangent vector for each vertex. If supplied, must be same length as Vertices array.
//	*/
//	UFUNCTION(BlueprintCallable, Category = "RuntimeMesh|Providers|Static", Meta = (DisplayName = "Update Section From Components", AutoCreateRefTerm = "Normals,UV0,UV1,UV2,UV3,VertexColors,Tangents", AdvancedDisplay = "UV1,UV2,UV3"))
//	void UpdateSectionFromComponents(int32 LODIndex, int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals, const TArray<FVector2D>& UV0, 
//		const TArray<FVector2D>& UV1, const TArray<FVector2D>& UV2, const TArray<FVector2D>& UV3, const TArray<FLinearColor>& VertexColors, const TArray<FRuntimeMeshTangent>& Tangents);
//
//
//	/**
//	*	Update the mesh data of a section.
//	*	@param	LODIndex			Index of the LOD to create the section in.
//	*	@param	SectionIndex		Index of the section to create or replace.
//	*	@param	Vertices			Vertex buffer of all vertex positions to use for this mesh section.
//	*	@param	Triangles			Index buffer indicating which vertices make up each triangle. Length must be a multiple of 3.
//	*	@param	Normals				Optional array of normal vectors for each vertex. If supplied, must be same length as Vertices array.
//	*	@param	UV0					Optional array of texture co-ordinates for each vertex. If supplied, must be same length as Vertices array.
//	*	@param	UV1					Optional array of texture co-ordinates for each vertex. If supplied, must be same length as Vertices array.
//	*	@param	UV2					Optional array of texture co-ordinates for each vertex. If supplied, must be same length as Vertices array.
//	*	@param	UV3					Optional array of texture co-ordinates for each vertex. If supplied, must be same length as Vertices array.
//	*	@param	VertexColors		Optional array of colors for each vertex. If supplied, must be same length as Vertices array.
//	*	@param	Tangents			Optional array of tangent vector for each vertex. If supplied, must be same length as Vertices array.
//	*/
//	void UpdateSectionFromComponents(int32 LODIndex, int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals, const TArray<FVector2D>& UV0, 
//		const TArray<FVector2D>& UV1, const TArray<FVector2D>& UV2, const TArray<FVector2D>& UV3, const TArray<FColor>& VertexColors, const TArray<FRuntimeMeshTangent>& Tangents);
//
//	/**
//	*	Update the mesh data of a section.
//	*	@param	LODIndex			Index of the LOD to create the section in.
//	*	@param	SectionIndex		Index of the section to create or replace.
//	*	@param	Vertices			Vertex buffer of all vertex positions to use for this mesh section.
//	*	@param	Triangles			Index buffer indicating which vertices make up each triangle. Length must be a multiple of 3.
//	*	@param	Normals				Optional array of normal vectors for each vertex. If supplied, must be same length as Vertices array.
//	*	@param	UV0					Optional array of texture co-ordinates for each vertex. If supplied, must be same length as Vertices array.
//	*	@param	VertexColors		Optional array of colors for each vertex. If supplied, must be same length as Vertices array.
//	*	@param	Tangents			Optional array of tangent vector for each vertex. If supplied, must be same length as Vertices array.
//	*/
//	void UpdateSectionFromComponents(int32 LODIndex, int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals,
//		const TArray<FVector2D>& UV0, const TArray<FLinearColor>& VertexColors, const TArray<FRuntimeMeshTangent>& Tangents);
//
//
//	/**
//	*	Update the mesh data of a section.
//	*	@param	LODIndex			Index of the LOD to create the section in.
//	*	@param	SectionIndex		Index of the section to create or replace.
//	*	@param	Vertices			Vertex buffer of all vertex positions to use for this mesh section.
//	*	@param	Triangles			Index buffer indicating which vertices make up each triangle. Length must be a multiple of 3.
//	*	@param	Normals				Optional array of normal vectors for each vertex. If supplied, must be same length as Vertices array.
//	*	@param	UV0					Optional array of texture co-ordinates for each vertex. If supplied, must be same length as Vertices array.
//	*	@param	VertexColors		Optional array of colors for each vertex. If supplied, must be same length as Vertices array.
//	*	@param	Tangents			Optional array of tangent vector for each vertex. If supplied, must be same length as Vertices array.
//	*/
//	void UpdateSectionFromComponents(int32 LODIndex, int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals,
//		const TArray<FVector2D>& UV0, const TArray<FColor>& VertexColors, const TArray<FRuntimeMeshTangent>& Tangents);
//
//
//
//
//
//
//	void CreateSection(int32 LODIndex, int32 SectionId, const FRuntimeMeshSectionProperties& SectionProperties, bool bCreateCollision = true)
//	{
//		URuntimeMeshProvider::CreateSection(LODIndex, SectionId, SectionProperties);
//		UpdateSectionAffectsCollision(LODIndex, SectionId, bCreateCollision);
//	}
//
//	void CreateSection(int32 LODIndex, int32 SectionId, const FRuntimeMeshSectionProperties& SectionProperties, const FRuntimeMeshRenderableMeshData& SectionData, bool bCreateCollision = true)
//	{
//		URuntimeMeshProvider::CreateSection(LODIndex, SectionId, SectionProperties);
//		UpdateSectionInternal(LODIndex, SectionId, SectionData, GetBoundsFromMeshData(SectionData));
//		UpdateSectionAffectsCollision(LODIndex, SectionId, bCreateCollision);
//	}
//	void CreateSection(int32 LODIndex, int32 SectionId, const FRuntimeMeshSectionProperties& SectionProperties, FRuntimeMeshRenderableMeshData&& SectionData, bool bCreateCollision = true)
//	{
//		URuntimeMeshProvider::CreateSection(LODIndex, SectionId, SectionProperties);
//		UpdateSectionInternal(LODIndex, SectionId, MoveTemp(SectionData), GetBoundsFromMeshData(SectionData));
//		UpdateSectionAffectsCollision(LODIndex, SectionId, bCreateCollision);
//	}
//	void CreateSection(int32 LODIndex, int32 SectionId, const FRuntimeMeshSectionProperties& SectionProperties, const FRuntimeMeshRenderableMeshData& SectionData, FBoxSphereBounds KnownBounds, bool bCreateCollision = true)
//	{
//		URuntimeMeshProvider::CreateSection(LODIndex, SectionId, SectionProperties);
//		UpdateSectionInternal(LODIndex, SectionId, SectionData, GetBoundsFromMeshData(SectionData));
//		UpdateSectionAffectsCollision(LODIndex, SectionId, bCreateCollision);
//	}
//	void CreateSection(int32 LODIndex, int32 SectionId, const FRuntimeMeshSectionProperties& SectionProperties, FRuntimeMeshRenderableMeshData&& SectionData, FBoxSphereBounds KnownBounds, bool bCreateCollision = true)
//	{
//		URuntimeMeshProvider::CreateSection(LODIndex, SectionId, SectionProperties);
//		UpdateSectionInternal(LODIndex, SectionId, MoveTemp(SectionData), GetBoundsFromMeshData(SectionData));
//		UpdateSectionAffectsCollision(LODIndex, SectionId, bCreateCollision);
//	}
//
//	void UpdateSection(int32 LODIndex, int32 SectionId, const FRuntimeMeshRenderableMeshData& SectionData)
//	{
//		UpdateSectionInternal(LODIndex, SectionId, SectionData, GetBoundsFromMeshData(SectionData));
//	}
//	void UpdateSection(int32 LODIndex, int32 SectionId, FRuntimeMeshRenderableMeshData&& SectionData)
//	{
//		UpdateSectionInternal(LODIndex, SectionId, MoveTemp(SectionData), GetBoundsFromMeshData(SectionData));
//	}
//	void UpdateSection(int32 LODIndex, int32 SectionId, const FRuntimeMeshRenderableMeshData& SectionData, FBoxSphereBounds KnownBounds)
//	{
//		UpdateSectionInternal(LODIndex, SectionId, SectionData, KnownBounds);
//	}
//	void UpdateSection(int32 LODIndex, int32 SectionId, FRuntimeMeshRenderableMeshData&& SectionData, FBoxSphereBounds KnownBounds)
//	{
//		UpdateSectionInternal(LODIndex, SectionId, MoveTemp(SectionData), KnownBounds);
//	}
//
//	UFUNCTION(BlueprintCallable, Category = "RuntimeMesh|Providers|Static")
//	void ClearSection(int32 LODIndex, int32 SectionId);
//
//	UFUNCTION(BlueprintCallable, Category = "RuntimeMesh|Providers|Static")
//	void SetCollisionSettings(const FRuntimeMeshCollisionSettings& NewCollisionSettings);
//
//	void SetCollisionMesh(const FRuntimeMeshCollisionData& NewCollisionMesh);
//
//	UFUNCTION(BlueprintCallable, Category = "RuntimeMesh|Providers|Static")
//	void SetRenderableLODForCollision(int32 LODIndex);
//
//	UFUNCTION(BlueprintCallable, Category = "RuntimeMesh|Providers|Static")
//	void SetRenderableSectionAffectsCollision(int32 SectionId, bool bCollisionEnabled);
//
//
//
//protected:
//
//	virtual void ConfigureLODs_Implementation(const TArray<FRuntimeMeshLODProperties>& LODSettings) override;
//	virtual void CreateSection_Implementation(int32 LODIndex, int32 SectionId, const FRuntimeMeshSectionProperties& SectionProperties) override;
//	virtual void SetSectionVisibility_Implementation(int32 LODIndex, int32 SectionId, bool bIsVisible) override;
//	virtual void SetSectionCastsShadow_Implementation(int32 LODIndex, int32 SectionId, bool bCastsShadow) override;
//	virtual void RemoveSection_Implementation(int32 LODIndex, int32 SectionId) override;
//
//protected:
//	virtual FBoxSphereBounds GetBounds_Implementation() override { return CombinedBounds; }
//
//	virtual bool GetSectionMeshForLOD_Implementation(int32 LODIndex, int32 SectionId, FRuntimeMeshRenderableMeshData& MeshData) override;
//
//	virtual FRuntimeMeshCollisionSettings GetCollisionSettings_Implementation() override;
//	virtual bool HasCollisionMesh_Implementation() override;
//	virtual bool GetCollisionMesh_Implementation(FRuntimeMeshCollisionData& CollisionData) override;
//
//private:
//	void UpdateSectionAffectsCollision(int32 LODIndex, int32 SectionId, bool bAffectsCollision);
//
//	void UpdateBounds();
//	FBoxSphereBounds GetBoundsFromMeshData(const FRuntimeMeshRenderableMeshData& MeshData);
//
//
//	void UpdateSectionInternal(int32 LODIndex, int32 SectionId, const FRuntimeMeshRenderableMeshData& SectionData, FBoxSphereBounds KnownBounds)
//	{
//		FRuntimeMeshRenderableMeshData TempData = SectionData;
//		UpdateSectionInternal(LODIndex, SectionId, MoveTemp(TempData), KnownBounds);
//	}
//	void UpdateSectionInternal(int32 LODIndex, int32 SectionId, FRuntimeMeshRenderableMeshData&& SectionData, FBoxSphereBounds KnownBounds);
//
//protected:
//	void Serialize(FArchive& Ar) override;
//};