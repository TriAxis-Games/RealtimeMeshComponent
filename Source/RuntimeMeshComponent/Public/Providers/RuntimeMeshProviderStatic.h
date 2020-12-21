// Copyright 2016-2020 TriAxis Games L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RuntimeMeshProvider.h"
#include "RuntimeMeshProviderStatic.generated.h"

class URuntimeMeshModifier;

UCLASS(HideCategories = Object, BlueprintType)
class RUNTIMEMESHCOMPONENT_API URuntimeMeshProviderStatic : public URuntimeMeshProvider
{
	GENERATED_BODY()

protected:
	UPROPERTY(Category = "RuntimeMeshActor", EditAnywhere, BlueprintReadOnly, Meta = (AllowPrivateAccess = "true"))
	bool StoreEditorGeneratedDataForGame;

private:

	using FSectionDataMapEntry = TTuple<FRuntimeMeshSectionProperties, FRuntimeMeshRenderableMeshData, FBoxSphereBounds>;

private:
	mutable FCriticalSection MeshSyncRoot;
	mutable FCriticalSection CollisionSyncRoot;

	mutable FRWLock ModifierRWLock;

	TArray<FRuntimeMeshLODProperties> LODConfigurations;
	TMap<int32, TMap<int32, FSectionDataMapEntry>> SectionDataMap;

	int32 LODForMeshCollision;
	TSet<int32> SectionsForMeshCollision;

	FRuntimeMeshCollisionSettings CollisionSettings;
	TOptional<FRuntimeMeshCollisionData> CollisionMesh;
	FBoxSphereBounds CombinedBounds;
	
	TArray<FRuntimeMeshMaterialSlot> LoadedMaterialSlots;


	UPROPERTY(VisibleAnywhere, Category = "RuntimeMesh|Providers|Static")
	TArray<URuntimeMeshModifier*> CurrentMeshModifiers;
public:

	URuntimeMeshProviderStatic();

	UFUNCTION(BlueprintCallable, Category = "RuntimeMesh|Providers|Static")
	void RegisterModifier(URuntimeMeshModifier* Modifier);

	UFUNCTION(BlueprintCallable, Category = "RuntimeMesh|Providers|Static")
	void UnRegisterModifier(URuntimeMeshModifier* Modifier);


	
	UFUNCTION(BlueprintCallable, Category = "RuntimeMesh|Providers|Static", Meta=(DisplayName = "Create Section"))
	void CreateSection_Blueprint(int32 LODIndex, int32 SectionId, const FRuntimeMeshSectionProperties& SectionProperties, const FRuntimeMeshRenderableMeshData& SectionData)
	{
		CreateSection(LODIndex, SectionId, SectionProperties);
		UpdateSectionInternal(LODIndex, SectionId, SectionData, GetBoundsFromMeshData(SectionData));
	}

	UFUNCTION(BlueprintCallable, Category = "RuntimeMesh|Providers|Static", Meta = (DisplayName = "Update Section"))
	void UpdateSection_Blueprint(int32 LODIndex, int32 SectionId, const FRuntimeMeshRenderableMeshData& SectionData)
	{
		UpdateSectionInternal(LODIndex, SectionId, SectionData, GetBoundsFromMeshData(SectionData));
	}

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
	UFUNCTION(BlueprintCallable, Category = "RuntimeMesh|Providers|Static", Meta = (DisplayName = "Create Section From Components", AutoCreateRefTerm = "Normals,UV0,UV1,UV2,UV3,VertexColors,Tangents", AdvancedDisplay = "UV1,UV2,UV3"))
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
	UFUNCTION(BlueprintCallable, Category = "RuntimeMesh|Providers|Static", Meta = (DisplayName = "Update Section From Components", AutoCreateRefTerm = "Normals,UV0,UV1,UV2,UV3,VertexColors,Tangents", AdvancedDisplay = "UV1,UV2,UV3"))
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






	void CreateSection(int32 LODIndex, int32 SectionId, const FRuntimeMeshSectionProperties& SectionProperties, bool bCreateCollision)
	{
		CreateSection(LODIndex, SectionId, SectionProperties);
		UpdateSectionAffectsCollision(LODIndex, SectionId, bCreateCollision);
	}

	void CreateSection(int32 LODIndex, int32 SectionId, const FRuntimeMeshSectionProperties& SectionProperties, const FRuntimeMeshRenderableMeshData& SectionData, bool bCreateCollision = true)
	{
		CreateSection(LODIndex, SectionId, SectionProperties);
		UpdateSectionInternal(LODIndex, SectionId, SectionData, GetBoundsFromMeshData(SectionData));
		UpdateSectionAffectsCollision(LODIndex, SectionId, bCreateCollision);
	}
	void CreateSection(int32 LODIndex, int32 SectionId, const FRuntimeMeshSectionProperties& SectionProperties, FRuntimeMeshRenderableMeshData&& SectionData, bool bCreateCollision = true)
	{
		CreateSection(LODIndex, SectionId, SectionProperties);
		UpdateSectionInternal(LODIndex, SectionId, MoveTemp(SectionData), GetBoundsFromMeshData(SectionData));
		UpdateSectionAffectsCollision(LODIndex, SectionId, bCreateCollision);
	}
	void CreateSection(int32 LODIndex, int32 SectionId, const FRuntimeMeshSectionProperties& SectionProperties, const FRuntimeMeshRenderableMeshData& SectionData, FBoxSphereBounds KnownBounds, bool bCreateCollision = true)
	{
		CreateSection(LODIndex, SectionId, SectionProperties);
		UpdateSectionInternal(LODIndex, SectionId, SectionData, GetBoundsFromMeshData(SectionData));
		UpdateSectionAffectsCollision(LODIndex, SectionId, bCreateCollision);
	}
	void CreateSection(int32 LODIndex, int32 SectionId, const FRuntimeMeshSectionProperties& SectionProperties, FRuntimeMeshRenderableMeshData&& SectionData, FBoxSphereBounds KnownBounds, bool bCreateCollision = true)
	{
		CreateSection(LODIndex, SectionId, SectionProperties);
		UpdateSectionInternal(LODIndex, SectionId, MoveTemp(SectionData), GetBoundsFromMeshData(SectionData));
		UpdateSectionAffectsCollision(LODIndex, SectionId, bCreateCollision);
	}

	void UpdateSection(int32 LODIndex, int32 SectionId, const FRuntimeMeshRenderableMeshData& SectionData)
	{
		UpdateSectionInternal(LODIndex, SectionId, SectionData, GetBoundsFromMeshData(SectionData));
	}
	void UpdateSection(int32 LODIndex, int32 SectionId, FRuntimeMeshRenderableMeshData&& SectionData)
	{
		UpdateSectionInternal(LODIndex, SectionId, MoveTemp(SectionData), GetBoundsFromMeshData(SectionData));
	}
	void UpdateSection(int32 LODIndex, int32 SectionId, const FRuntimeMeshRenderableMeshData& SectionData, FBoxSphereBounds KnownBounds)
	{
		UpdateSectionInternal(LODIndex, SectionId, SectionData, KnownBounds);
	}
	void UpdateSection(int32 LODIndex, int32 SectionId, FRuntimeMeshRenderableMeshData&& SectionData, FBoxSphereBounds KnownBounds)
	{
		UpdateSectionInternal(LODIndex, SectionId, MoveTemp(SectionData), KnownBounds);
	}


	UFUNCTION(BlueprintCallable, Category = "RuntimeMesh|Providers|Static")
	FRuntimeMeshCollisionSettings GetCollisionSettingsStatic() const;

	UFUNCTION(BlueprintCallable, Category = "RuntimeMesh|Providers|Static")
	void SetCollisionSettings(const FRuntimeMeshCollisionSettings& NewCollisionSettings);

	UFUNCTION(BlueprintCallable, Category = "RuntimeMesh|Providers|Static")
	FRuntimeMeshCollisionData GetCollisionMeshStatic() const;

	UFUNCTION(BlueprintCallable, Category = "RuntimeMesh|Providers|Static")
	void SetCollisionMesh(const FRuntimeMeshCollisionData& NewCollisionMesh);

	UFUNCTION(BlueprintCallable, Category = "RuntimeMesh|Providers|Static")
	int32 GetLODForMeshCollision() const;

	UFUNCTION(BlueprintCallable, Category = "RuntimeMesh|Providers|Static")
	void SetRenderableLODForCollision(int32 LODIndex);

	UFUNCTION(BlueprintCallable, Category = "RuntimeMesh|Providers|Static")
	TSet<int32> GetSectionsForMeshCollision() const;

	UFUNCTION(BlueprintCallable, Category = "RuntimeMesh|Providers|Static")
	void SetRenderableSectionAffectsCollision(int32 SectionId, bool bCollisionEnabled, bool bForceUpdate = false);


	UFUNCTION(BlueprintCallable, Category="RuntimeMesh|Providers|Static")
	TArray<int32> GetSectionIds(int32 LODIndex) const;

	UFUNCTION(BlueprintCallable, Category = "RuntimeMesh|Providers|Static")
	int32 GetLastSectionId(int32 LODIndex) const;

	UFUNCTION(BlueprintCallable, Category = "RuntimeMesh|Providers|Static")
	bool DoesSectionHaveValidMeshData(int32 LODIndex, int32 SectionId) const;

	UFUNCTION(BlueprintCallable, Category = "RuntimeMesh|Providers|Static")
	void RemoveAllSectionsForLOD(int32 LODIndex);

	UFUNCTION(BlueprintCallable, Category = "RuntimeMesh|Providers|Static")
	FBoxSphereBounds GetSectionBounds(int32 LODIndex, int32 SectionId) const;

	UFUNCTION(BlueprintCallable, Category = "RuntimeMesh|Providers|Static")
	FRuntimeMeshSectionProperties GetSectionProperties(int32 LODIndex, int32 SectionId) const;

	UFUNCTION(BlueprintCallable, Category = "RuntimeMesh|Providers|Static")
	FRuntimeMeshRenderableMeshData GetSectionRenderData(int32 LODIndex, int32 SectionId) const;

	UFUNCTION(BlueprintCallable, Category = "RuntimeMesh|Providers|Static")
	FRuntimeMeshRenderableMeshData GetSectionRenderDataAndClear(int32 LODIndex, int32 SectionId);
	
	UFUNCTION(BlueprintCallable, Category = "RuntimeMesh|Providers|Static")
	void SetShouldSerializeMeshData(bool bIsSerialized);

public:

	virtual void Initialize() override;
	virtual FBoxSphereBounds GetBounds() override { return CombinedBounds; }
	virtual bool GetSectionMeshForLOD(int32 LODIndex, int32 SectionId, FRuntimeMeshRenderableMeshData& MeshData) override;
	virtual FRuntimeMeshCollisionSettings GetCollisionSettings() override;
	virtual bool HasCollisionMesh() override;
	virtual bool GetCollisionMesh(FRuntimeMeshCollisionData& CollisionData) override;


public:
	virtual void ConfigureLODs(const TArray<FRuntimeMeshLODProperties>& LODSettings) override;
	virtual void SetLODScreenSize(int32 LODIndex, float ScreenSize) override;
	virtual void CreateSection(int32 LODIndex, int32 SectionId, const FRuntimeMeshSectionProperties& SectionProperties) override;
	virtual void SetSectionVisibility(int32 LODIndex, int32 SectionId, bool bIsVisible) override;
	virtual void SetSectionCastsShadow(int32 LODIndex, int32 SectionId, bool bCastsShadow) override;
	virtual void ClearSection(int32 LODIndex, int32 SectionId) override;
	virtual void RemoveSection(int32 LODIndex, int32 SectionId) override;


	virtual void Serialize(FArchive& Ar) override;
	virtual void BeginDestroy() override;
private:
	static const TArray<FVector2D> EmptyUVs;

	template<typename TangentType, typename ColorType>
	static FRuntimeMeshRenderableMeshData FillMeshData(FRuntimeMeshSectionProperties& Properties, const TArray<FVector>& Vertices, const TArray<FVector>& Normals, const TArray<TangentType>& Tangents,
		const TArray<ColorType>& VertexColors, const TArray<FVector2D>& UV0, const TArray<FVector2D>& UV1, const TArray<FVector2D>& UV2, const TArray<FVector2D>& UV3, const TArray<int32>& Triangles)
	{
		Properties.bWants32BitIndices = Vertices.Num() > MAX_uint16;
		Properties.bUseHighPrecisionTexCoords = true;
		Properties.NumTexCoords =
			UV3.Num() > 0 ? 4 :
			UV2.Num() > 0 ? 3 :
			UV1.Num() > 0 ? 2 : 1;


		FRuntimeMeshRenderableMeshData SectionData(Properties);
		SectionData.Positions.Append(Vertices);
		SectionData.Tangents.Append(Normals, Tangents);
		if (SectionData.Tangents.Num() < SectionData.Positions.Num())
		{
			int32 Count = SectionData.Tangents.Num();
			SectionData.Tangents.SetNum(SectionData.Positions.Num());
			for (int32 Index = Count; Index < SectionData.Tangents.Num(); Index++)
			{
				SectionData.Tangents.SetTangents(Index, FVector(1, 0, 0), FVector(0, 1, 0), FVector(0, 0, 1));
			}
		}
		SectionData.Colors.Append(VertexColors);
		if (SectionData.Colors.Num() < SectionData.Positions.Num())
		{
			SectionData.Colors.SetNum(SectionData.Positions.Num());
		}

		int32 StartIndexTexCoords = SectionData.TexCoords.Num();
		SectionData.TexCoords.FillIn(StartIndexTexCoords, UV0, 0);
		SectionData.TexCoords.FillIn(StartIndexTexCoords, UV1, 1);
		SectionData.TexCoords.FillIn(StartIndexTexCoords, UV2, 2);
		SectionData.TexCoords.FillIn(StartIndexTexCoords, UV3, 3);

		if (SectionData.TexCoords.Num() < SectionData.Positions.Num())
		{
			SectionData.TexCoords.SetNum(SectionData.Positions.Num());
		}

		SectionData.Triangles.Append(Triangles);

		return SectionData;
	}

	void UpdateSectionAffectsCollision(int32 LODIndex, int32 SectionId, bool bAffectsCollision, bool bForceUpdate = false);

	void UpdateBounds();
	FBoxSphereBounds GetBoundsFromMeshData(const FRuntimeMeshRenderableMeshData& MeshData);


	void UpdateSectionInternal(int32 LODIndex, int32 SectionId, const FRuntimeMeshRenderableMeshData& SectionData, FBoxSphereBounds KnownBounds)
	{
		FRuntimeMeshRenderableMeshData TempData = SectionData;
		UpdateSectionInternal(LODIndex, SectionId, MoveTemp(TempData), KnownBounds);
	}
	void UpdateSectionInternal(int32 LODIndex, int32 SectionId, FRuntimeMeshRenderableMeshData&& SectionData, FBoxSphereBounds KnownBounds);


};