// Copyright 2016 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "Components/MeshComponent.h"
#include "RuntimeMeshCore.h"
#include "RuntimeMeshSection.h"
#include "RuntimeMeshGenericVertex.h"
#include "PhysicsEngine/ConvexElem.h"
#include "RuntimeMeshComponent.generated.h"



USTRUCT()
struct RUNTIMEMESHCOMPONENT_API FRuntimeMeshComponentPrePhysicsTickFunction : public FTickFunction
{
	GENERATED_USTRUCT_BODY()

	class URuntimeMeshComponent* Target;

	virtual void ExecuteTick(float DeltaTime, ELevelTick TickType, ENamedThreads::Type CurrentThread,
		const FGraphEventRef& MyCompletionGraphEvent) override;

	virtual FString DiagnosticMessage() override;
};

/**
*	Component that allows you to specify custom triangle mesh geometry
*	Beware! This feature is experimental and may be substantially changed in future releases.
*/
UCLASS(HideCategories = (Object, LOD), Meta = (BlueprintSpawnableComponent), Experimental, ClassGroup = Experimental)
class RUNTIMEMESHCOMPONENT_API URuntimeMeshComponent : public UMeshComponent, public IInterface_CollisionDataProvider
{
	GENERATED_BODY()

private:
	
	template<typename SectionType>
	TSharedPtr<SectionType> CreateOrResetSection(int32 SectionIndex, bool bWantsSeparatePositionBuffer)
	{
		// Ensure sections array is long enough
		if (SectionIndex >= MeshSections.Num())
		{
			MeshSections.SetNum(SectionIndex + 1, false);
		}

		// Create new section
		TSharedPtr<SectionType> NewSection = MakeShareable(new SectionType(bWantsSeparatePositionBuffer));

		MeshSections[SectionIndex] = NewSection;

		return NewSection;
	}
		
	TSharedPtr<FRuntimeMeshSectionInterface> CreateOrResetSectionInternalType(int32 SectionIndex, int32 NumUVChannels, bool WantsHalfPrecsionUVs);


	UMaterialInterface* GetSectionMaterial(int32 Index)
	{
		auto Material = GetMaterial(Index);
		return Material ? Material : UMaterial::GetDefaultMaterial(MD_Surface);
	}


	void CreateSectionInternal(int32 SectionIndex, bool bNeedsBoundsUpdate);

	void UpdateSectionInternal(int32 SectionIndex, bool bHadVertexPositionsUpdate, bool bHadVertexUpdates, bool bHadIndexUpdates, bool bNeedsBoundsUpdate);

	void UpdateSectionVertexPositionsInternal(int32 SectionIndex, bool bNeedsBoundsUpdate);

	void UpdateSectionPropertiesInternal(int32 SectionIndex, bool bUpdateRequiresProxyRecreateIfStatic);






// 	void FinishCreateSectionInternal(int32 SectionIndex, RuntimeMeshSectionPtr& Section, bool bNeedsBoundsUpdate);
// 
// 	void FinishUpdateSectionInternal(int32 SectionIndex, RuntimeMeshSectionPtr& Section, bool bHadPositionUpdates, bool bHadVertexUpdates, bool bHadIndexUpdates, bool bNeedsBoundsUpdate);
// 
// 	void FinishUpdateSectionPropertiesInternal(int32 SectionIndex, RuntimeMeshSectionPtr& Section, bool bUpdateRequiresProxyRecreateIfStatic);
// 
// 	void FinishPositionOnlyUpdateSectionInternal(int32 SectionIndex, RuntimeMeshSectionPtr& Section, bool bNeedsBoundsUpdate);

public:
	URuntimeMeshComponent(const FObjectInitializer& ObjectInitializer);


	template<typename VertexType>
	void CreateMeshSection(int32 SectionIndex, TArray<VertexType>& Vertices, TArray<int32>& Triangles, bool bCreateCollision = false, 
		EUpdateFrequency UpdateFrequency = EUpdateFrequency::Average, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_CreateMeshSection_VertexType);
		TSharedPtr<FRuntimeMeshSection<VertexType>> Section = CreateOrResetSection<FRuntimeMeshSection<VertexType>>(SectionIndex, false);
		
		bool bShouldUseMove = (UpdateFlags & ESectionUpdateFlags::MoveArrays) != ESectionUpdateFlags::None;
		Section->UpdateVertexBuffer(Vertices, nullptr, bShouldUseMove);
		Section->UpdateIndexBuffer(Triangles, bShouldUseMove);

		// Track collision status and update collision information if necessary
		Section->CollisionEnabled = bCreateCollision;
		Section->UpdateFrequency = UpdateFrequency;

		CreateSectionInternal(SectionIndex, true);
	}

	template<typename VertexType>
	void CreateMeshSection(int32 SectionIndex, TArray<VertexType>& Vertices, TArray<int32>& Triangles, const FBox& BoundingBox, bool bCreateCollision = false,
		EUpdateFrequency UpdateFrequency = EUpdateFrequency::Average, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_CreateMeshSection_VertexType_WithBoundingBox);
		TSharedPtr<FRuntimeMeshSection<VertexType>> Section = CreateOrResetSection<FRuntimeMeshSection<VertexType>>(SectionIndex, false);

		bool bShouldUseMove = (UpdateFlags & ESectionUpdateFlags::MoveArrays) != ESectionUpdateFlags::None;
		Section->UpdateVertexBuffer(Vertices, &BoundingBox, bShouldUseMove);
		Section->UpdateIndexBuffer(Triangles, bShouldUseMove);

		// Track collision status and update collision information if necessary
		Section->CollisionEnabled = bCreateCollision;
		Section->UpdateFrequency = UpdateFrequency;

		CreateSectionInternal(SectionIndex, true);
	}

	template<typename VertexType>
	void UpdateMeshSection(int32 SectionIndex, TArray<VertexType>& Vertices, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_UpdateMeshSection_VertexType);

		// Validate section exists
		check(SectionIndex < MeshSections.Num() && MeshSections[SectionIndex].IsValid());

		// Validate section type
		MeshSections[SectionIndex]->GetVertexType()->EnsureEquals<VertexType>();

		// Cast section to correct type
		TSharedPtr<FRuntimeMeshSection<VertexType>> Section = StaticCastSharedPtr<FRuntimeMeshSection<VertexType>>(MeshSections[SectionIndex]);
		
		bool bShouldUseMove = (UpdateFlags & ESectionUpdateFlags::MoveArrays) != ESectionUpdateFlags::None;
		bool bNeedsBoundsUpdate = Section->UpdateVertexBuffer(Vertices, nullptr, bShouldUseMove);

		UpdateSectionInternal(SectionIndex, false, true, false, bNeedsBoundsUpdate);
	}

	template<typename VertexType>
	void UpdateMeshSection(int32 SectionIndex, TArray<VertexType>& Vertices, const FBox& BoundingBox, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_UpdateMeshSection_VertexType_WithBoundingBox);

		// Validate section exists
		check(SectionIndex < MeshSections.Num() && MeshSections[SectionIndex].IsValid());

		// Validate section type
		MeshSections[SectionIndex]->GetVertexType()->EnsureEquals<VertexType>();

		// Cast section to correct type
		TSharedPtr<FRuntimeMeshSection<VertexType>> Section = StaticCastSharedPtr<FRuntimeMeshSection<VertexType>>(MeshSections[SectionIndex]);

		bool bShouldUseMove = (UpdateFlags & ESectionUpdateFlags::MoveArrays) != ESectionUpdateFlags::None;
		bool bNeedsBoundsUpdate = Section->UpdateVertexBuffer(Vertices, &BoundingBox, bShouldUseMove);

		UpdateSectionInternal(SectionIndex, false, true, false, bNeedsBoundsUpdate);
	}

	template<typename VertexType>
	void UpdateMeshSection(int32 SectionIndex, TArray<VertexType>& Vertices, TArray<int32>& Triangles, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_UpdateMeshSection_VertexType_WithTriangles);

		// Validate section exists
		check(SectionIndex < MeshSections.Num() && MeshSections[SectionIndex].IsValid());

		// Validate section type
		MeshSections[SectionIndex]->GetVertexType()->EnsureEquals<VertexType>();

		// Cast section to correct type
		TSharedPtr<FRuntimeMeshSection<VertexType>> Section = StaticCastSharedPtr<FRuntimeMeshSection<VertexType>>(MeshSections[SectionIndex]);

		bool bShouldUseMove = (UpdateFlags & ESectionUpdateFlags::MoveArrays) != ESectionUpdateFlags::None;
		bool bNeedsBoundsUpdate = Section->UpdateVertexBuffer(Vertices, nullptr, bShouldUseMove);
		Section->UpdateIndexBuffer(Triangles, bShouldUseMove);

		UpdateSectionInternal(SectionIndex, false, true, true, true);
	}

	template<typename VertexType>
	void UpdateMeshSection(int32 SectionIndex, TArray<VertexType>& Vertices, TArray<int32>& Triangles, const FBox& BoundingBox, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_UpdateMeshSection_VertexType_WithTrianglesAndBoundinBox);

		// Validate section exists
		check(SectionIndex < MeshSections.Num() && MeshSections[SectionIndex].IsValid());

		// Validate section type
		MeshSections[SectionIndex]->GetVertexType()->EnsureEquals<VertexType>();

		// Cast section to correct type
		TSharedPtr<FRuntimeMeshSection<VertexType>> Section = StaticCastSharedPtr<FRuntimeMeshSection<VertexType>>(MeshSections[SectionIndex]);

		bool bShouldUseMove = (UpdateFlags & ESectionUpdateFlags::MoveArrays) != ESectionUpdateFlags::None;
		bool bNeedsBoundsUpdate = Section->UpdateVertexBuffer(Vertices, &BoundingBox, bShouldUseMove);
		Section->UpdateIndexBuffer(Triangles, bShouldUseMove);

		UpdateSectionInternal(SectionIndex, false, true, true, bNeedsBoundsUpdate);
	}




	template<typename VertexType>
	void CreateMeshSectionDualBuffer(int32 SectionIndex, TArray<FVector>& VertexPositions, TArray<VertexType>& VertexData, TArray<int32>& Triangles, bool bCreateCollision = false,
		EUpdateFrequency UpdateFrequency = EUpdateFrequency::Average, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_CreateMeshSectionDualBuffer);

		TSharedPtr<FRuntimeMeshSection<VertexType>> Section = CreateOrResetSection<FRuntimeMeshSection<VertexType>>(SectionIndex, true);

		bool bShouldUseMove = (UpdateFlags & ESectionUpdateFlags::MoveArrays) != ESectionUpdateFlags::None;
		Section->UpdateVertexPositionBuffer(VertexPositions, nullptr, bShouldUseMove);
		Section->UpdateVertexBuffer(VertexData, nullptr, bShouldUseMove);
		Section->UpdateIndexBuffer(Triangles, bShouldUseMove);
		
		// Track collision status and update collision information if necessary
		Section->CollisionEnabled = bCreateCollision;
		Section->UpdateFrequency = UpdateFrequency;

		CreateSectionInternal(SectionIndex, true);
	}
	
	template<typename VertexType>
	void UpdateMeshSectionPositionsImmediate(int32 SectionIndex, TArray<FVector>& VertexPositions, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_UpdateMeshSectionPositionsImmediate);

		// Validate section exists
		check(SectionIndex < MeshSections.Num() && MeshSections[SectionIndex].IsValid());

		// Validate section type
		MeshSections[SectionIndex]->GetVertexType()->EnsureEquals<VertexType>();

		// Cast section to correct type
		TSharedPtr<FRuntimeMeshSection<VertexType>> Section = StaticCastSharedPtr<FRuntimeMeshSection<VertexType>>(MeshSections[SectionIndex]);

		bool bShouldUseMove = (UpdateFlags & ESectionUpdateFlags::MoveArrays) != ESectionUpdateFlags::None;
		Section->UpdateVertexPositionBuffer(VertexPositions, nullptr, bShouldUseMove);

		UpdateSectionVertexPositionsInternal(SectionIndex, true);
	}

	template<typename VertexType>
	void UpdateMeshSectionPositionsImmediate(int32 SectionIndex, TArray<FVector>& VertexPositions, const FBox& BoundingBox, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_UpdateMeshSectionPositionsImmediate_WithBoundinBox);

		// Validate section exists
		check(SectionIndex < MeshSections.Num() && MeshSections[SectionIndex].IsValid());

		// Validate section type
		MeshSections[SectionIndex]->GetVertexType()->EnsureEquals<VertexType>();

		// Cast section to correct type
		TSharedPtr<FRuntimeMeshSection<VertexType>> Section = StaticCastSharedPtr<FRuntimeMeshSection<VertexType>>(MeshSections[SectionIndex]);


		bool bShouldUseMove = (UpdateFlags & ESectionUpdateFlags::MoveArrays) != ESectionUpdateFlags::None;
		bool bNeedsBoundsUpdate = Section->UpdateVertexPositionBuffer(VertexPositions, &BoundingBox, bShouldUseMove);

		UpdateSectionVertexPositionsInternal(SectionIndex, bNeedsBoundsUpdate);
	}






	void CreateMeshSection(int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals,
		const TArray<FVector2D>& UV0, const TArray<FColor>& Colors, const TArray<FRuntimeMeshTangent>& Tangents, bool bCreateCollision = false,
		EUpdateFrequency UpdateFrequency = EUpdateFrequency::Average);

	void CreateMeshSection(int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals,
		const TArray<FVector2D>& UV0, const TArray<FVector2D>& UV1, const TArray<FColor>& Colors, const TArray<FRuntimeMeshTangent>& Tangents,
		bool bCreateCollision = false, EUpdateFrequency UpdateFrequency = EUpdateFrequency::Average);

	
	void UpdateMeshSection(int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<FVector>& Normals, const TArray<FVector2D>& UV0, 
		const TArray<FColor>& Colors, const TArray<FRuntimeMeshTangent>& Tangents);

	void UpdateMeshSection(int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<FVector>& Normals, const TArray<FVector2D>& UV0, 
		const TArray<FVector2D>& UV1, const TArray<FColor>& Colors, const TArray<FRuntimeMeshTangent>& Tangents);

	void UpdateMeshSection(int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals, 
		const TArray<FVector2D>& UV0, const TArray<FColor>& Colors, const TArray<FRuntimeMeshTangent>& Tangents);

	void UpdateMeshSection(int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals,
		const TArray<FVector2D>& UV0, const TArray<FVector2D>& UV1, const TArray<FColor>& Colors, const TArray<FRuntimeMeshTangent>& Tangents);

	

	/**
	*	Create/replace a section for this runtime mesh component.
	*	@param	SectionIndex		Index of the section to create or replace.
	*	@param	Vertices			Vertex buffer of all vertex positions to use for this mesh section.
	*	@param	Normals				Optional array of normal vectors for each vertex. If supplied, must be same length as Vertices array.
	*	@param	Tangents			Optional array of tangent vector for each vertex. If supplied, must be same length as Vertices array.
	*	@param	UV0					Optional array of texture co-ordinates for each vertex (UV Channel 0). If supplied, must be same length as Vertices array.
	*	@param	UV1					Optional array of texture co-ordinates for each vertex (UV Channel 1). If supplied, must be same length as Vertices array.
	*	@param	VertexColors		Optional array of colors for each vertex. If supplied, must be same length as Vertices array.
	*	@param	Triangles			Index buffer indicating which vertices make up each triangle. Length must be a multiple of 3.
	*	@param	ShouldCreateCollision	Indicates whether collision should be created for this section. This adds significant cost.
	*/
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh", meta = (DisplayName = "Create Mesh Section", AutoCreateRefTerm = "Normals,Tangents,UV0,UV1,VertexColors"))
	void CreateMeshSection_Blueprint(int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals, 
		const TArray<FRuntimeMeshTangent>& Tangents, const TArray<FVector2D>& UV0, const TArray<FVector2D>& UV1, const TArray<FLinearColor>& VertexColors, 
		bool bCreateCollision, EUpdateFrequency UpdateFrequency = EUpdateFrequency::Average);

	/**
	*	Updates a section of this runtime mesh component. This is faster than CreateMeshSection. If you change the vertices count, you must update the other components.
	*	@param	SectionIndex		Index of the section to update.
	*	@param	Vertices			Vertex buffer of all vertex positions to use for this mesh section.
	*	@param	Normals				Optional array of normal vectors for each vertex. If supplied, must be same length as Vertices array.
	*	@param	Tangents			Optional array of tangent vector for each vertex. If supplied, must be same length as Vertices array.
	*	@param	UV0					Optional array of texture co-ordinates for each vertex (UV Channel 0). If supplied, must be same length as Vertices array.
	*	@param	UV1					Optional array of texture co-ordinates for each vertex (UV Channel 1). If supplied, must be same length as Vertices array.
	*	@param	VertexColors		Optional array of colors for each vertex. If supplied, must be same length as Vertices array.
	*	@param	Triangles			Index buffer indicating which vertices make up each triangle. Length must be a multiple of 3.
	*/
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh", meta = (DisplayName = "Update Mesh Section", AutoCreateRefTerm = "Normals,Tangents,UV0,UV1,VertexColors"))
	void UpdateMeshSection_Blueprint(int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals, 
		const TArray<FRuntimeMeshTangent>& Tangents, const TArray<FVector2D>& UV0, const TArray<FVector2D>& UV1, const TArray<FLinearColor>& VertexColors);
	

	/** Clear a section of the procedural mesh. Other sections do not change index. */
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void ClearMeshSection(int32 SectionIndex);

	/** Clear all mesh sections and reset to empty state */
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void ClearAllMeshSections();

	/** Gets the bounding box of a specific section */
	bool GetSectionBoundingBox(int32 SectionIndex, FBox& OutBoundingBox);


	/** Control visibility of a particular section */
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void SetMeshSectionVisible(int32 SectionIndex, bool bNewVisibility);

	/** Returns whether a particular section is currently visible */
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	bool IsMeshSectionVisible(int32 SectionIndex) const;


	/** Control whether a particular section casts a shadow */
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void SetMeshSectionCastsShadow(int32 SectionIndex, bool bNewCastsShadow);

	/** Returns whether a particular section is currently casting shadows */
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	bool IsMeshSectionCastingShadows(int32 SectionIndex) const;

	/** Control whether a particular section has collision */
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void SetMeshSectionCollisionEnabled(int32 SectionIndex, bool bNewCollisionEnabled);

	/** Returns whether a particular section has collision */
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	bool IsMeshSectionCollisionEnabled(int32 SectionIndex);


	/** Returns number of sections currently created for this component */
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	int32 GetNumSections() const;

	/** Returns whether a particular section currently exists */
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	bool DoesSectionExist(int32 SectionIndex) const;


	/** Sets the geometry for a collision only section */
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void SetMeshCollisionSection(int32 CollisionSectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles);

	/** Clears the geometry for a collision only section */
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void ClearMeshCollisionSection(int32 CollisionSectionIndex);

	/** Clears the geometry for ALL collision only sections */
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void ClearAllMeshCollisionSections();


	/** Add simple collision convex to this component */
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void AddCollisionConvexMesh(TArray<FVector> ConvexVerts);

	/** Add simple collision convex to this component */
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void ClearCollisionConvexMeshes();

	/** Function to replace _all_ simple collision in one go */
	void SetCollisionConvexMeshes(const TArray< TArray<FVector> >& ConvexMeshes);

	/** Begins a batch of updates, delays updates until you call EndBatchUpdates() */
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void BeginBatchUpdates()
	{
		BatchState.StartBatch();
	}

	/** Ends a batch of updates started with BeginBatchUpdates() */
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void EndBatchUpdates();



	/**
	*	Controls whether the complex (Per poly) geometry should be treated as 'simple' collision.
	*	Should be set to false if this component is going to be given simple collision and simulated.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components|RuntimeMesh")
	bool bUseComplexAsSimpleCollision;

	/**
	*	Controls whether the mesh data should be serialized with the component.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components|RuntimeMesh")
	bool bShouldSerializeMeshData;

	/** Collision data */
	UPROPERTY(Transient, DuplicateTransient)
	class UBodySetup* BodySetup;

private:


	//~ Begin Interface_CollisionDataProvider Interface
	virtual bool GetPhysicsTriMeshData(struct FTriMeshCollisionData* CollisionData, bool InUseAllTriData) override;
	virtual bool ContainsPhysicsTriMeshData(bool InUseAllTriData) const override;
	virtual bool WantsNegXTriMesh() override { return false; }
	//~ End Interface_CollisionDataProvider Interface


	//~ Begin USceneComponent Interface.
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
	//~ Begin USceneComponent Interface.

	//~ Begin UPrimitiveComponent Interface.
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	virtual class UBodySetup* GetBodySetup() override;
	//~ End UPrimitiveComponent Interface.

	//~ Begin UMeshComponent Interface.
	virtual int32 GetNumMaterials() const override;
	//~ End UMeshComponent Interface.



	/** Update LocalBounds member from the local box of each section */
	void UpdateLocalBounds(bool bMarkRenderTransform = true);
	/** Ensure ProcMeshBodySetup is allocated and configured */
	void EnsureBodySetupCreated();
	/** Mark collision data as dirty, and re-create on instance if necessary */
	void UpdateCollision();


	void MarkCollisionDirty();

	void BakeCollision();


	virtual void Serialize(FArchive& Ar) override;

	virtual void PostLoad() override;

	virtual void RegisterComponentTickFunctions(bool bRegister) override;


	/* Current state of a batch update. */
	FRuntimeMeshBatchUpdateState BatchState;


	bool bCollisionDirty;

	/** Array of sections of mesh */	
	TArray<RuntimeMeshSectionPtr> MeshSections;

	UPROPERTY(Transient)
	TArray<FRuntimeMeshCollisionSection> MeshCollisionSections;

	/** Convex shapes used for simple collision */
	UPROPERTY(Transient)
	TArray<FRuntimeConvexCollisionSection> ConvexCollisionSections;

	/** Local space bounds of mesh */
	UPROPERTY(Transient)
	FBoxSphereBounds LocalBounds;

	UPROPERTY(Transient)
	FRuntimeMeshComponentPrePhysicsTickFunction PrePhysicsTick;


	friend class FRuntimeMeshSceneProxy;
	friend struct FRuntimeMeshComponentPrePhysicsTickFunction;
};




