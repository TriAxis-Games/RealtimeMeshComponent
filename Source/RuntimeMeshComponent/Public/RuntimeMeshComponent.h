// Copyright 2016-2018 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "Components/MeshComponent.h"
#include "RuntimeMeshCore.h"
#include "RuntimeMeshSection.h"
#include "RuntimeMesh.h"

#include "RuntimeMeshComponent.generated.h"

/**
*	Component that allows you to specify custom triangle mesh geometry for rendering and collision.
*/
UCLASS(HideCategories = (Object, LOD), Meta = (BlueprintSpawnableComponent))
class RUNTIMEMESHCOMPONENT_API URuntimeMeshComponent 
    : public UMeshComponent, 
    public IInterface_CollisionDataProvider
{
	GENERATED_BODY()

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = RuntimeMesh, Meta = (AllowPrivateAccess = "true", DisplayName = "Runtime Mesh"))
	URuntimeMesh* RuntimeMeshReference;

	void EnsureHasRuntimeMesh();

public:
	URuntimeMeshComponent(const FObjectInitializer& ObjectInitializer);

	/** Clears the geometry for ALL collision only sections */
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	FORCEINLINE URuntimeMesh* GetRuntimeMesh() const
	{
		return RuntimeMeshReference;
	}

	/** Clears the geometry for ALL collision only sections */
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	FORCEINLINE URuntimeMesh* GetOrCreateRuntimeMesh()
	{
		EnsureHasRuntimeMesh();

		return RuntimeMeshReference;
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	bool ShouldSerializeMeshData() const
    {
		return GetRuntimeMesh() ? GetRuntimeMesh()->ShouldSerializeMeshData() : false;
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void SetShouldSerializeMeshData(const bool bShouldSerialize)
	{
		GetOrCreateRuntimeMesh()->SetShouldSerializeMeshData(bShouldSerialize);
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh", Meta = (AllowPrivateAccess = "true", DisplayName = "Get Mobility"))
	ERuntimeMeshMobility GetRuntimeMeshMobility() const
    {
		return Mobility == EComponentMobility::Movable ? ERuntimeMeshMobility::Movable :
			Mobility == EComponentMobility::Stationary ? ERuntimeMeshMobility::Stationary : ERuntimeMeshMobility::Static;
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh", Meta = (AllowPrivateAccess = "true", DisplayName = "Set Mobility"))
	void SetRuntimeMeshMobility(const ERuntimeMeshMobility NewMobility)
	{
		Super::SetMobility(
			NewMobility == ERuntimeMeshMobility::Movable ? EComponentMobility::Movable :
			NewMobility == ERuntimeMeshMobility::Stationary ? EComponentMobility::Stationary : EComponentMobility::Static);
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void SetRuntimeMesh(URuntimeMesh* NewMesh);

	void CreateMeshSection(
		const int32 SectionIndex,
		const bool bWantsHighPrecisionTangents,
		const bool bWantsHighPrecisionUVs,
        const int32 NumUVs,
		const bool bWants32BitIndices,
		const bool bCreateCollision,
		const EUpdateFrequency UpdateFrequency = EUpdateFrequency::Average)
	{
		GetOrCreateRuntimeMesh()->CreateMeshSection(SectionIndex,
			                                        bWantsHighPrecisionTangents,
                                                    bWantsHighPrecisionUVs,
			                                        NumUVs,
                                                    bWants32BitIndices,
			                                        bCreateCollision,
                                                    UpdateFrequency);
	}

	template<typename TVertexType, typename TIndexType>
	FORCEINLINE void CreateMeshSection(
		int32 SectionIndex,
		TArray<TVertexType>& Vertices,
		TArray<TIndexType>& Triangles,
		bool BCreateCollision = false,
		EUpdateFrequency UpdateFrequency = EUpdateFrequency::Average,
		ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		GetOrCreateRuntimeMesh()->CreateMeshSection(SectionIndex,
			                                        Vertices,
			                                        Triangles,
                                                    BCreateCollision,
			                                        UpdateFrequency,
			                                        UpdateFlags);
	}

	template<typename TVertexType, typename TIndexType>
	FORCEINLINE void CreateMeshSection(
		int32 SectionIndex,
		TArray<TVertexType>& Vertices,
		TArray<TIndexType>& Triangles,
		const FBox& BoundingBox,
		bool bCreateCollision = false,
		EUpdateFrequency UpdateFrequency = EUpdateFrequency::Average,
		ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		GetOrCreateRuntimeMesh()->CreateMeshSection(SectionIndex,
			                                        Vertices,
			                                        Triangles,
			                                        BoundingBox,
                                                    bCreateCollision,
			                                        UpdateFrequency,
			                                        UpdateFlags);
	}

	template<typename TVertexType0, typename TVertexType1, typename TIndexType>
	FORCEINLINE void CreateMeshSectionDualBuffer(
		int32 SectionIndex,
		TArray<TVertexType0>& Vertices0,
		TArray<TVertexType1>& Vertices1,
		TArray<TIndexType>& InTriangles,
		bool bCreateCollision = false,
		EUpdateFrequency UpdateFrequency = EUpdateFrequency::Average,
		ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		GetOrCreateRuntimeMesh()->CreateMeshSectionDualBuffer(SectionIndex,
                                                                Vertices0,
                                                                Vertices1,
                                                                InTriangles,
                                                                bCreateCollision,
                                                                UpdateFrequency,
                                                                UpdateFlags);
	}

	template<typename VertexType0, typename VertexType1, typename IndexType>
	FORCEINLINE void CreateMeshSectionDualBuffer(
		int32 SectionIndex,
		TArray<VertexType0>& Vertices0,
		TArray<VertexType1>& Vertices1,
		TArray<IndexType>& Triangles,
		const FBox& BoundingBox,
		bool bCreateCollision = false,
		EUpdateFrequency UpdateFrequency = EUpdateFrequency::Average,
		ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		GetOrCreateRuntimeMesh()->CreateMeshSectionDualBuffer(SectionIndex,
                                                                Vertices0,
			                                                    Vertices1,
			                                                    Triangles,
                                                                BoundingBox,
			                                                    bCreateCollision,
                                                                UpdateFrequency,
			                                                    UpdateFlags);
	}

	template<typename TVertexType0, typename TVertexType1, typename TVertexType2, typename TIndexType>
	FORCEINLINE void CreateMeshSectionTripleBuffer(
		int32 SectionIndex,
		TArray<TVertexType0>& Vertices0,
		TArray<TVertexType1>& Vertices1,
		TArray<TVertexType2>& Vertices2,
		TArray<TIndexType>& Triangles,
		bool bCreateCollision = false,
		EUpdateFrequency UpdateFrequency = EUpdateFrequency::Average,
		ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		GetOrCreateRuntimeMesh()->CreateMeshSectionTripleBuffer(SectionIndex, Vertices0, Vertices1, Vertices2, Triangles, bCreateCollision, UpdateFrequency, UpdateFlags);
	}

	template<typename TVertexType0, typename TVertexType1, typename TVertexType2, typename TIndexType>
	FORCEINLINE void CreateMeshSectionTripleBuffer(
		int32 SectionIndex,
		TArray<TVertexType0>& Vertices0,
		TArray<TVertexType1>& Vertices1,
		TArray<TVertexType2>& Vertices2,
		TArray<TIndexType>& Triangles,
		const FBox& BoundingBox,
		bool bCreateCollision = false,
		EUpdateFrequency UpdateFrequency = EUpdateFrequency::Average,
		ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		GetOrCreateRuntimeMesh()->CreateMeshSectionTripleBuffer(SectionIndex, Vertices0, Vertices1, Vertices2, Triangles, BoundingBox, bCreateCollision, UpdateFrequency, UpdateFlags);
	}

	template<typename TVertexType>
	FORCEINLINE void UpdateMeshSection(
		int32 SectionId,
		TArray<TVertexType>& Vertices,
		ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		GetOrCreateRuntimeMesh()->UpdateMeshSection(SectionId, Vertices, UpdateFlags);
	}

	template<typename VertexType0>
	FORCEINLINE void UpdateMeshSection(
		int32 SectionId,
		TArray<VertexType0>& InVertices0,
		const FBox& BoundingBox,
		ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		GetOrCreateRuntimeMesh()->UpdateMeshSection(SectionId, InVertices0, BoundingBox, UpdateFlags);
	}

	template<typename VertexType0, typename IndexType>
	FORCEINLINE void UpdateMeshSection(
		int32 SectionId,
		TArray<VertexType0>& InVertices0,
		TArray<IndexType>& InTriangles,
		ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		GetOrCreateRuntimeMesh()->UpdateMeshSection(SectionId, InVertices0, InTriangles, UpdateFlags);
	}

	template<typename TVertexType, typename TIndexType>
	FORCEINLINE void UpdateMeshSection(
		int32 SectionId,
		TArray<TVertexType>& Vertices,
		TArray<TIndexType>& Triangles,
		const FBox& BoundingBox,
		ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		GetOrCreateRuntimeMesh()->UpdateMeshSection(SectionId, Vertices, Triangles, BoundingBox, UpdateFlags);
	}

	template<typename TVertexType0, typename TVertexType1>
	FORCEINLINE void UpdateMeshSectionDualBuffer(
		int32 SectionId,
		TArray<TVertexType0>& Vertices0,
		TArray<TVertexType1>& Vertices1,
		ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		GetOrCreateRuntimeMesh()->UpdateMeshSectionDualBuffer(SectionId, Vertices0, Vertices1, UpdateFlags);
	}

	template<typename TVertexType0, typename TVertexType1>
	FORCEINLINE void UpdateMeshSectionDualBuffer(
		int32 SectionId,
		TArray<TVertexType0>& Vertices0,
		TArray<TVertexType1>& Vertices1,
		const FBox& BoundingBox,
		ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		GetOrCreateRuntimeMesh()->UpdateMeshSectionDualBuffer(SectionId, Vertices0, Vertices1, BoundingBox, UpdateFlags);
	}

	template<typename TVertexType0, typename TVertexType1, typename TIndexType>
	FORCEINLINE void UpdateMeshSectionDualBuffer(
		int32 SectionId,
		TArray<TVertexType0>& Vertices0,
		TArray<TVertexType1>& Vertices1,
		TArray<TIndexType>& Triangles,
		ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		GetOrCreateRuntimeMesh()->UpdateMeshSectionDualBuffer(SectionId, Vertices0, Vertices1, Triangles, UpdateFlags);
	}

	template<typename TVertexType0, typename TVertexType1, typename TIndexType>
	FORCEINLINE void UpdateMeshSectionDualBuffer(
		int32 SectionId,
		TArray<TVertexType0>& Vertices0,
		TArray<TVertexType1>& Vertices1,
		TArray<TIndexType>& Triangles,
		const FBox& BoundingBox,
		ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		GetOrCreateRuntimeMesh()->UpdateMeshSectionDualBuffer(SectionId, Vertices0, Vertices1, Triangles, BoundingBox, UpdateFlags);
	}

	template<typename TVertexType0, typename TVertexType1, typename TVertexType2>
	FORCEINLINE void UpdateMeshSectionTripleBuffer(
		int32 SectionId,
		TArray<TVertexType0>& Vertices0,
		TArray<TVertexType1>& Vertices1,
		TArray<TVertexType2>& Vertices2,
		ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		GetOrCreateRuntimeMesh()->UpdateMeshSectionTripleBuffer(SectionId, Vertices0, Vertices1, Vertices2, UpdateFlags);
	}

	template<typename TVertexType0, typename TVertexType1, typename TVertexType2>
	FORCEINLINE void UpdateMeshSectionTripleBuffer(
		int32 SectionId,
		TArray<TVertexType0>& Vertices0,
		TArray<TVertexType1>& Vertices1,
		TArray<TVertexType2>& Vertices2,
		const FBox& BoundingBox,
		ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		GetOrCreateRuntimeMesh()->UpdateMeshSectionTripleBuffer(SectionId, Vertices0, Vertices1, Vertices2, BoundingBox, UpdateFlags);
	}

	template<typename TVertexType0, typename TVertexType1, typename TVertexType2, typename TIndexType>
	FORCEINLINE void UpdateMeshSectionTripleBuffer(
		int32 SectionId,
		TArray<TVertexType0>& Vertices0,
		TArray<TVertexType1>& Vertices1,
		TArray<TVertexType2>& Vertices2,
		TArray<TIndexType>& Triangles,
		ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		GetOrCreateRuntimeMesh()->UpdateMeshSectionTripleBuffer(SectionId, Vertices0, Vertices1, Vertices2, Triangles, UpdateFlags);
	}

	template<typename TVertexType0, typename TVertexType1, typename TVertexType2, typename TIndexType>
	FORCEINLINE void UpdateMeshSectionTripleBuffer(
		int32 SectionId,
		TArray<TVertexType0>& Vertices0,
		TArray<TVertexType1>& Vertices1,
		TArray<TVertexType2>& Vertices2,
		TArray<TIndexType>& Triangles,
		const FBox& BoundingBox,
		ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		GetOrCreateRuntimeMesh()->UpdateMeshSectionTripleBuffer(SectionId, Vertices0, Vertices1, Vertices2, Triangles, BoundingBox, UpdateFlags);
	}

	/** DEPRECATED! Use UpdateMeshSectionDualBuffer() instead.  Updates the dual buffer mesh section */
	template<typename TVertexType>
	DEPRECATED(3.0, "UpdateMeshSection for dual buffer sections deprecated. Please use UpdateMeshSectionDualBuffer instead.")
	void UpdateMeshSection(
		int32 SectionIndex,
		TArray<FVector>& VertexPositions,
		TArray<TVertexType>& VertexData,
		ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		UpdateMeshSectionDualBuffer(SectionIndex, VertexPositions, VertexData, UpdateFlags);
	}

	/** DEPRECATED! Use UpdateMeshSectionDualBuffer() instead.  Updates the dual buffer mesh section */
	template<typename TVertexType>
	DEPRECATED(3.0, "UpdateMeshSection for dual buffer sections deprecated. Please use UpdateMeshSectionDualBuffer instead.")
	void UpdateMeshSection(
		int32 SectionIndex,
		TArray<FVector>& VertexPositions,
		TArray<TVertexType>& VertexData,
		const FBox& BoundingBox,
		ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		UpdateMeshSectionDualBuffer(SectionIndex, VertexPositions, VertexData, BoundingBox, UpdateFlags);
	}

	/** DEPRECATED! Use UpdateMeshSectionDualBuffer() instead.  Updates the dual buffer mesh section */
	template<typename TVertexType>
	DEPRECATED(3.0, "UpdateMeshSection for dual buffer sections deprecated. Please use UpdateMeshSectionDualBuffer instead.")
	void UpdateMeshSection(
		int32 SectionIndex,
		TArray<FVector>& VertexPositions,
		TArray<TVertexType>& VertexData,
		TArray<int32>& Triangles,
		ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		UpdateMeshSectionDualBuffer(SectionIndex, VertexPositions, VertexData, Triangles, UpdateFlags);
	}

	/** DEPRECATED! Use UpdateMeshSectionDualBuffer() instead.  Updates the dual buffer mesh section */
	template<typename TVertexType>
	DEPRECATED(3.0, "UpdateMeshSection for dual buffer sections deprecated. Please use UpdateMeshSectionDualBuffer instead.")
	void UpdateMeshSection(
		int32 SectionIndex,
		TArray<FVector>& VertexPositions,
		TArray<TVertexType>& VertexData,
		TArray<int32>& Triangles,
		const FBox& BoundingBox,
		ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		UpdateMeshSectionDualBuffer(SectionIndex, VertexPositions, VertexData, Triangles, BoundingBox, UpdateFlags);
	}

	template<typename TVertexType>
	FORCEINLINE void UpdateMeshSectionPrimaryBuffer(
		int32 SectionId,
		TArray<TVertexType>& Vertices,
		ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		GetOrCreateRuntimeMesh()->UpdateMeshSectionPrimaryBuffer(SectionId, Vertices, UpdateFlags);
	}

	template<typename TVertexType>
	FORCEINLINE void UpdateMeshSectionPrimaryBuffer(
		int32 SectionId,
		TArray<TVertexType>& Vertices,
		const FBox& BoundingBox,
		ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		GetOrCreateRuntimeMesh()->UpdateMeshSectionPrimaryBuffer(SectionId, Vertices, BoundingBox, UpdateFlags);
	}

	template<typename TVertexType>
	FORCEINLINE void UpdateMeshSectionSecondaryBuffer(
		int32 SectionId,
		TArray<TVertexType>& Vertices,
		ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		GetOrCreateRuntimeMesh()->UpdateMeshSectionSecondaryBuffer(SectionId, Vertices, UpdateFlags);
	}

	template<typename TVertexType>
	FORCEINLINE void UpdateMeshSectionTertiaryBuffer(
		int32 SectionId,
		TArray<TVertexType>& Vertices,
		ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		GetOrCreateRuntimeMesh()->UpdateMeshSectionTertiaryBuffer(SectionId, Vertices, UpdateFlags);
	}

	template<typename TIndexType>
	FORCEINLINE void UpdateMeshSectionTriangles(
		int32 SectionId,
		TArray<TIndexType>& Triangles,
		ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		GetOrCreateRuntimeMesh()->UpdateMeshSectionTriangles(SectionId, Triangles, UpdateFlags);
	}

	FORCEINLINE void CreateMeshSection(
        const int32 SectionId,
		const TSharedPtr<FRuntimeMeshBuilder>& MeshData,
        const bool bCreateCollision = false,
        const EUpdateFrequency UpdateFrequency = EUpdateFrequency::Average,
        const ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		GetOrCreateRuntimeMesh()->CreateMeshSection(SectionId, MeshData, bCreateCollision, UpdateFrequency, UpdateFlags);
	}

	FORCEINLINE void CreateMeshSectionByMove(
        const int32 SectionId,
		const TSharedPtr<FRuntimeMeshBuilder>& MeshData,
        const bool bCreateCollision = false,
        const EUpdateFrequency UpdateFrequency = EUpdateFrequency::Average,
        const ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		GetOrCreateRuntimeMesh()->CreateMeshSectionByMove(SectionId, MeshData, bCreateCollision, UpdateFrequency, UpdateFlags);
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void CreateMeshSectionFromBuilder(
        const int32 SectionId,
		URuntimeBlueprintMeshBuilder* MeshData,
        const bool bCreateCollision = false,
        const EUpdateFrequency UpdateFrequency = EUpdateFrequency::Average/*, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None*/)
	{
		GetOrCreateRuntimeMesh()->CreateMeshSectionFromBuilder(SectionId, MeshData, bCreateCollision, UpdateFrequency/*, UpdateFlags*/);
	}

	FORCEINLINE void UpdateMeshSection(
        const int32 SectionId,
		const TSharedPtr<FRuntimeMeshBuilder>& MeshData,
        const ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		GetOrCreateRuntimeMesh()->UpdateMeshSection(SectionId, MeshData, UpdateFlags);
	}

	FORCEINLINE void UpdateMeshSectionByMove(
        const int32 SectionId,
		const TSharedPtr<FRuntimeMeshBuilder>& MeshData,
        const ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		GetOrCreateRuntimeMesh()->UpdateMeshSectionByMove(SectionId, MeshData, UpdateFlags);
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void UpdateMeshSectionFromBuilder(
        const int32 SectionId,
		URuntimeBlueprintMeshBuilder* MeshData
	/*, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None*/)
	{
		GetOrCreateRuntimeMesh()->UpdateMeshSectionFromBuilder(SectionId, MeshData/*, UpdateFlags*/);
	}

	TUniquePtr<FRuntimeMeshScopedUpdater> BeginSectionUpdate(
        const int32 SectionId,
        const ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		check(IsInGameThread());
		return GetOrCreateRuntimeMesh()->BeginSectionUpdate(SectionId, UpdateFlags);
	}

	TUniquePtr<FRuntimeMeshScopedUpdater> GetSectionReadonly(const int32 SectionId)
	{
		check(IsInGameThread());
		return GetOrCreateRuntimeMesh()->GetSectionReadonly(SectionId);
	}

	FORCEINLINE void CreateMeshSection(
        const int32 SectionIndex,
		const TArray<FVector>& Vertices,
		const TArray<int32>& Triangles,
		const TArray<FVector>& Normals,
		const TArray<FVector2D>& UV0,
		const TArray<FColor>& Colors,
		const TArray<FRuntimeMeshTangent>& Tangents,
        const bool bCreateCollision = false,
        const EUpdateFrequency UpdateFrequency = EUpdateFrequency::Average,
		ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None,
        const bool bUseHighPrecisionTangents = false,
		bool bUseHighPrecisionUVs = true)
	{
		GetOrCreateRuntimeMesh()->CreateMeshSection(SectionIndex, Vertices, Triangles, Normals, UV0, Colors, Tangents, bCreateCollision, UpdateFrequency, UpdateFlags, bUseHighPrecisionTangents, bUseHighPrecisionUVs);
	}

	FORCEINLINE void CreateMeshSection(
        const int32 SectionIndex,
		const TArray<FVector>& Vertices,
		const TArray<int32>& Triangles,
		const TArray<FVector>& Normals,
		const TArray<FVector2D>& UV0,
		const TArray<FVector2D>& UV1,
		const TArray<FColor>& Colors,
		const TArray<FRuntimeMeshTangent>& Tangents,
        const bool bCreateCollision = false,
        const EUpdateFrequency UpdateFrequency = EUpdateFrequency::Average,
        const ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None,
        const bool bUseHighPrecisionTangents = false,
        const bool bUseHighPrecisionUVs = true)
	{
		GetOrCreateRuntimeMesh()->CreateMeshSection(SectionIndex,
			                                        Vertices,
			                                        Triangles,
			                                        Normals,
			                                        UV0,
                                                    UV1,
			                                        Colors,
			                                        Tangents,
			                                        bCreateCollision,
                                                    UpdateFrequency,
			                                        UpdateFlags,
                                                    bUseHighPrecisionTangents,
			                                        bUseHighPrecisionUVs);
	}

	FORCEINLINE void UpdateMeshSection(
        const int32 SectionIndex,
		const TArray<FVector>& Vertices,
		const TArray<FVector>& Normals,
		const TArray<FVector2D>& UV0,
		const TArray<FColor>& Colors,
		const TArray<FRuntimeMeshTangent>& Tangents,
        const ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		GetOrCreateRuntimeMesh()->UpdateMeshSection(SectionIndex, Vertices, Normals, UV0, Colors, Tangents, UpdateFlags);
	}

	FORCEINLINE void UpdateMeshSection(
        const int32 SectionIndex,
		const TArray<FVector>& Vertices,
		const TArray<FVector>& Normals,
		const TArray<FVector2D>& UV0,
		const TArray<FVector2D>& UV1,
		const TArray<FColor>& Colors,
		const TArray<FRuntimeMeshTangent>& Tangents,
        const ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		GetOrCreateRuntimeMesh()->UpdateMeshSection(SectionIndex, Vertices, Normals, UV0, UV1, Colors, Tangents, UpdateFlags);
	}

	FORCEINLINE void UpdateMeshSection(
        const int32 SectionIndex,
		const TArray<FVector>& Vertices,
		const TArray<int32>& Triangles,
		const TArray<FVector>& Normals,
		const TArray<FVector2D>& UV0,
		const TArray<FColor>& Colors,
		const TArray<FRuntimeMeshTangent>& Tangents,
        const ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		GetOrCreateRuntimeMesh()->UpdateMeshSection(SectionIndex, Vertices, Triangles, Normals, UV0, Colors, Tangents, UpdateFlags);
	}

	FORCEINLINE void UpdateMeshSection(
        const int32 SectionIndex,
		const TArray<FVector>& Vertices,
		const TArray<int32>& Triangles,
		const TArray<FVector>& Normals,
		const TArray<FVector2D>& UV0,
		const TArray<FVector2D>& UV1,
		const TArray<FColor>& Colors,
		const TArray<FRuntimeMeshTangent>& Tangents,
        const ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		GetOrCreateRuntimeMesh()->UpdateMeshSection(SectionIndex, Vertices, Triangles, Normals, UV0, UV1, Colors, Tangents, UpdateFlags);
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh", meta = (DisplayName = "Create Mesh Section", AutoCreateRefTerm = "Normals,Tangents,UV0,UV1,Colors"))
	void CreateMeshSection_Blueprint(
        const int32 SectionIndex,
		const TArray<FVector>& Vertices,
		const TArray<int32>& Triangles,
		const TArray<FVector>& Normals,
		const TArray<FRuntimeMeshTangent>& Tangents,
		const TArray<FVector2D>& UV0,
		const TArray<FVector2D>& UV1,
		const TArray<FLinearColor>& Colors,
        const bool bCreateCollision = false,
        const bool bCalculateNormalTangent = false,
        const bool bShouldCreateHardTangents = false,
        const bool bGenerateTessellationTriangles = false,
        const EUpdateFrequency UpdateFrequency = EUpdateFrequency::Average,
        const bool bUseHighPrecisionTangents = false,
        const bool bUseHighPrecisionUVs = true)
	{
		GetOrCreateRuntimeMesh()->CreateMeshSection_Blueprint(SectionIndex,
			                                                    Vertices,
			                                                    Triangles,
                                                                Normals,
			                                                    Tangents,
			                                                    UV0,
			                                                    UV1,
			                                                    Colors,
                                                                bCreateCollision,
                                                              bCalculateNormalTangent,
                                                              bShouldCreateHardTangents,
                                                              bGenerateTessellationTriangles,
                                                              UpdateFrequency,
                                                              bUseHighPrecisionTangents,
                                                              bUseHighPrecisionUVs);
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh", meta = (DisplayName = "Update Mesh Section", AutoCreateRefTerm = "Triangles,Normals,Tangents,UV0,UV1,Colors"))
	void UpdateMeshSection_Blueprint(
        const int32 SectionIndex,
		const TArray<FVector>& Vertices,
		const TArray<int32>& Triangles,
		const TArray<FVector>& Normals,
		const TArray<FRuntimeMeshTangent>& Tangents,
		const TArray<FVector2D>& UV0,
		const TArray<FVector2D>& UV1,
		const TArray<FLinearColor>& Colors,
        const bool bCalculateNormalTangent = false,
        const bool bShouldCreateHardTangents = false,
        const bool bGenerateTessellationTriangles = false)
	{
		GetOrCreateRuntimeMesh()->UpdateMeshSection_Blueprint(SectionIndex, Vertices, Triangles, Normals, Tangents, UV0, UV1, Colors, bCalculateNormalTangent, bShouldCreateHardTangents, bGenerateTessellationTriangles);
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh", meta = (DisplayName = "Create Mesh Section Packed", AutoCreateRefTerm = "Normals,Tangents,UV0,UV1,Colors"))
	void CreateMeshSectionPacked_Blueprint(
        const int32 SectionIndex,
		const TArray<FRuntimeMeshBlueprintVertexSimple>& Vertices,
		const TArray<int32>& Triangles,
        const bool bCreateCollision = false,
        const bool bCalculateNormalTangent = false,
        const bool bShouldCreateHardTangents = false,
        const bool bGenerateTessellationTriangles = false,
        const EUpdateFrequency UpdateFrequency = EUpdateFrequency::Average,
        const bool bUseHighPrecisionTangents = false,
        const bool bUseHighPrecisionUVs = true)
	{
		GetOrCreateRuntimeMesh()->CreateMeshSectionPacked_Blueprint(
			SectionIndex,
			Vertices,
			Triangles,
			bCreateCollision,
			bCalculateNormalTangent,
			bShouldCreateHardTangents,
			bGenerateTessellationTriangles,
			UpdateFrequency,
			bUseHighPrecisionTangents,
			bUseHighPrecisionUVs);
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh", meta = (DisplayName = "Update Mesh Section Packed", AutoCreateRefTerm = "Triangles,Normals,Tangents,UV0,UV1,Colors"))
	void UpdateMeshSectionPacked_Blueprint(
        const int32 SectionIndex,
		const TArray<FRuntimeMeshBlueprintVertexSimple>& Vertices,
		const TArray<int32>& Triangles,
        const bool bCalculateNormalTangent = false,
        const bool bShouldCreateHardTangents = false,
        const bool bGenerateTessellationTriangles = false)
	{
		GetOrCreateRuntimeMesh()->UpdateMeshSectionPacked_Blueprint(SectionIndex,
			                                                        Vertices,
                                                                    Triangles,
                                                                    bCalculateNormalTangent,
                                                                    bShouldCreateHardTangents,
                                                                    bGenerateTessellationTriangles);
	}

	/** Clear a section of the procedural mesh. */
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void ClearMeshSection(const int32 SectionIndex) const
    {
		if (auto Mesh = GetRuntimeMesh())
            Mesh->ClearMeshSection(SectionIndex);
    }

	/** Clear all mesh sections and reset to empty state */
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void ClearAllMeshSections() const
    {
		if (auto Mesh = GetRuntimeMesh())
            Mesh->ClearAllMeshSections();
    }

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void SetSectionMaterial(const int32 SectionId, UMaterialInterface* Material) const
    {
		if (auto Mesh = GetRuntimeMesh())
            Mesh->SetSectionMaterial(SectionId, Material);
    }

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	UMaterialInterface* GetSectionMaterial(const int32 SectionId) const
    {
		if (auto Mesh = GetRuntimeMesh())
            return Mesh->GetSectionMaterial(SectionId);

        return nullptr;
	}

	/** Gets the bounding box of a specific section */
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	FBox GetSectionBoundingBox(const int32 SectionIndex) const
    {
		if (auto Mesh = GetRuntimeMesh())
			return Mesh->GetSectionBoundingBox(SectionIndex);

		return FBox(EForceInit::ForceInitToZero);
	}

	/** Control visibility of a particular section */
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void SetMeshSectionVisible(const int32 SectionIndex, const bool bNewVisibility) const
    {
		if (auto Mesh = GetRuntimeMesh())
            Mesh->SetMeshSectionVisible(SectionIndex, bNewVisibility);
    }

	/** Returns whether a particular section is currently visible */
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	bool IsMeshSectionVisible(const int32 SectionIndex) const
	{
		if (const auto Mesh = GetRuntimeMesh())
            return Mesh->IsMeshSectionVisible(SectionIndex);

        return false;
	}

	/** Control whether a particular section casts a shadow */
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void SetMeshSectionCastsShadow(const int32 SectionIndex, const bool bNewCastsShadow) const
    {
		if (auto Mesh = GetRuntimeMesh())
            Mesh->SetMeshSectionCastsShadow(SectionIndex, bNewCastsShadow);
    }

	/** Returns whether a particular section is currently casting shadows */
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	bool IsMeshSectionCastingShadows(const int32 SectionIndex) const
	{
		if (const auto Mesh = GetRuntimeMesh())
            return Mesh->IsMeshSectionCastingShadows(SectionIndex);

        return false;
	}

	/** Control whether a particular section has collision */
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void SetMeshSectionCollisionEnabled(const int32 SectionIndex, bool bNewCollisionEnabled) const
    {
		if (auto Mesh = GetRuntimeMesh())
            Mesh->SetMeshSectionCollisionEnabled(SectionIndex, bNewCollisionEnabled);
    }

	/** Returns whether a particular section has collision */
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	bool IsMeshSectionCollisionEnabled(const int32 SectionIndex) const
    {
		if (auto Mesh = GetRuntimeMesh())
            return Mesh->IsMeshSectionCollisionEnabled(SectionIndex);

        return false;
	}

	/** Returns number of sections currently created for this component */
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	int32 GetNumSections() const
	{
		if (const auto Mesh = GetRuntimeMesh())
            return Mesh->GetNumSections();

        return 0;
	}

	/** Returns whether a particular section currently exists */
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	bool DoesSectionExist(const int32 SectionIndex) const
	{
		if (const auto Mesh = GetRuntimeMesh())
            return Mesh->DoesSectionExist(SectionIndex);

        return false;
	}

	/** Returns first available section index */
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	int32 GetAvailableSectionIndex() const
	{
		if (const auto Mesh = GetRuntimeMesh())
            return Mesh->GetAvailableSectionIndex();

        return 0;
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void SetMeshCollisionSection(
		const int32 CollisionSectionIndex,
		const TArray<FVector>& Vertices,
		const TArray<int32>& Triangles)
	{
		GetOrCreateRuntimeMesh()->SetMeshCollisionSection(CollisionSectionIndex, Vertices, Triangles);
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void ClearMeshCollisionSection(const int32 CollisionSectionIndex)
	{
		GetOrCreateRuntimeMesh()->ClearMeshCollisionSection(CollisionSectionIndex);
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void ClearAllMeshCollisionSections()
	{
		GetOrCreateRuntimeMesh()->ClearAllMeshCollisionSections();
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	int32 AddConvexCollisionSection(const TArray<FVector> ConvexVerts)
	{
		return GetOrCreateRuntimeMesh()->AddConvexCollisionSection(ConvexVerts);
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void SetConvexCollisionSection(const int32 ConvexSectionIndex, const TArray<FVector> ConvexVerts)
	{
		GetOrCreateRuntimeMesh()->SetConvexCollisionSection(ConvexSectionIndex, ConvexVerts);
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void ClearConvexCollisionSection(const int32 ConvexSectionIndex)
	{
		GetOrCreateRuntimeMesh()->ClearConvexCollisionSection(ConvexSectionIndex);
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void ClearAllConvexCollisionSections()
	{
		GetOrCreateRuntimeMesh()->ClearAllConvexCollisionSections();
	}

	void SetCollisionConvexMeshes(const TArray<TArray<FVector>>& ConvexMeshes)
	{
		GetOrCreateRuntimeMesh()->SetCollisionConvexMeshes(ConvexMeshes);
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	int32 AddCollisionBox(const FRuntimeMeshCollisionBox& NewBox)
	{
		return GetOrCreateRuntimeMesh()->AddCollisionBox(NewBox);
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void RemoveCollisionBox(const int32 Index)
	{
		GetOrCreateRuntimeMesh()->RemoveCollisionBox(Index);
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void ClearCollisionBoxes()
	{
		GetOrCreateRuntimeMesh()->ClearCollisionBoxes();
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void SetCollisionBoxes(const TArray<FRuntimeMeshCollisionBox>& NewBoxes)
	{
		GetOrCreateRuntimeMesh()->SetCollisionBoxes(NewBoxes);
	}

	
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	int32 AddCollisionSphere(const FRuntimeMeshCollisionSphere& NewSphere)
	{
		return GetOrCreateRuntimeMesh()->AddCollisionSphere(NewSphere);
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void RemoveCollisionSphere(const int32 Index)
	{
		GetOrCreateRuntimeMesh()->RemoveCollisionSphere(Index);
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void ClearCollisionSpheres()
	{
		GetOrCreateRuntimeMesh()->ClearCollisionSpheres();
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void SetCollisionSpheres(const TArray<FRuntimeMeshCollisionSphere>& NewSpheres)
	{
		GetOrCreateRuntimeMesh()->SetCollisionSpheres(NewSpheres);
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	int32 AddCollisionCapsule(const FRuntimeMeshCollisionCapsule& NewCapsule)
	{
		return GetOrCreateRuntimeMesh()->AddCollisionCapsule(NewCapsule);
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void RemoveCollisionCapsule(const int32 Index)
	{
		GetOrCreateRuntimeMesh()->RemoveCollisionCapsule(Index);
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void ClearCollisionCapsules()
	{
		GetOrCreateRuntimeMesh()->ClearCollisionCapsules();
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void SetCollisionCapsules(const TArray<FRuntimeMeshCollisionCapsule>& NewCapsules)
	{
		GetOrCreateRuntimeMesh()->SetCollisionCapsules(NewCapsules);
	}

	/** Runs any pending collision cook (Not required to call this. This is only if you need to make sure all changes are cooked before doing something)*/
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void CookCollisionNow()
	{
		GetOrCreateRuntimeMesh()->CookCollisionNow();
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void SetCollisionUseComplexAsSimple(const bool bNewValue)
	{
		GetOrCreateRuntimeMesh()->SetCollisionUseComplexAsSimple(bNewValue);
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	bool IsCollisionUsingComplexAsSimple() const
    {
		check(IsInGameThread());
		return GetRuntimeMesh() != nullptr ? GetRuntimeMesh()->IsCollisionUsingComplexAsSimple() : true;
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void SetCollisionUseAsyncCooking(const bool bNewValue)
	{
		GetOrCreateRuntimeMesh()->SetCollisionUseAsyncCooking(bNewValue);
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	bool IsCollisionUsingAsyncCooking() const
    {
		check(IsInGameThread());
		return GetRuntimeMesh() != nullptr ? GetRuntimeMesh()->IsCollisionUsingAsyncCooking() : false;
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void SetCollisionMode(ERuntimeMeshCollisionCookingMode NewMode)
	{
		GetOrCreateRuntimeMesh()->SetCollisionMode(NewMode);
	}

private:
	//~ Begin USceneComponent Interface.
	FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
	bool IsSupportedForNetworking() const override
	{
		return true;
	}
	//~ Begin USceneComponent Interface.

	//~ Begin UPrimitiveComponent Interface.
	FPrimitiveSceneProxy* CreateSceneProxy() override;
	class UBodySetup* GetBodySetup() override;

public:
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	int32 GetSectionIdFromCollisionFaceIndex(int32 FaceIndex) const;

	UMaterialInterface* GetMaterialFromCollisionFaceIndex(int32 FaceIndex, int32& SectionIndex) const override;
	//~ End UPrimitiveComponent Interface.

	//~ Begin UMeshComponent Interface.
	int32 GetNumMaterials() const override;
	void GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials = false) const override;
	UMaterialInterface* GetMaterial(int32 ElementIndex) const override;
	virtual UMaterialInterface* GetOverrideMaterial(int32 ElementIndex) const;
	//~ End UMeshComponent Interface.

private:
	/* Serializes this component */
	void Serialize(FArchive& Ar) override;

	/* Does post load fixups */
	void PostLoad() override;

	/** Called by URuntimeMesh any time it has new collision data that we should use */
	void NewCollisionMeshReceived();
	void NewBoundsReceived();
	void ForceProxyRecreate();

	void SendSectionCreation(int32 SectionIndex);
	void SendSectionPropertiesUpdate(int32 SectionIndex);

	// This collision setup is only to support older engine versions where the BodySetup being owned by a non UActorComponent breaks runtime cooking

	/** Collision data */
	UPROPERTY(Instanced)
	UBodySetup* BodySetup;

	/** Queue of pending collision cooks */
	UPROPERTY(Transient)
	TArray<UBodySetup*> AsyncBodySetupQueue;

#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION < 21
	//~ Begin Interface_CollisionDataProvider Interface
	virtual bool GetPhysicsTriMeshData(struct FTriMeshCollisionData* CollisionData, bool InUseAllTriData) override;
	virtual bool ContainsPhysicsTriMeshData(bool InUseAllTriData) const override;
	virtual bool WantsNegXTriMesh() override { return false; }
	//~ End Interface_CollisionDataProvider Interface

	UBodySetup* CreateNewBodySetup();
	void FinishPhysicsAsyncCook(UBodySetup* FinishedBodySetup);

	void UpdateCollision(bool bForceCookNow);
#endif

	friend class URuntimeMesh;
	friend class FRuntimeMeshComponentSceneProxy;
	friend class FRuntimeMeshComponentLegacySerialization;
};
