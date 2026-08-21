// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RealtimeMeshManaged.h"
#include "Data/RealtimeMeshLOD.h"
#include "Data/RealtimeMeshSection.h"
#include "Data/RealtimeMeshBufferSet.h"
#include "Core/RealtimeMeshBuilder.h"
#include "Core/RealtimeMeshDataStream.h"
#include "RealtimeMeshProcedural.generated.h"


class URealtimeMeshProcedural;


/**
 * Mirror of UProceduralMeshComponent's FProcMeshTangent — preserves
 * the bFlipTangentY semantic. Defined here so RMC doesn't depend on
 * the ProceduralMeshComponent module.
 */
USTRUCT(BlueprintType)
struct REALTIMEMESHCOMPONENT_API FRealtimeMeshProceduralTangent
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Tangent)
	FVector TangentX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Tangent)
	bool bFlipTangentY;

	FRealtimeMeshProceduralTangent()
		: TangentX(FVector::ForwardVector)
		, bFlipTangentY(false)
	{
	}

	FRealtimeMeshProceduralTangent(float X, float Y, float Z)
		: TangentX(X, Y, Z)
		, bFlipTangentY(false)
	{
	}

	FRealtimeMeshProceduralTangent(FVector InTangentX, bool bInFlipTangentY)
		: TangentX(InTangentX)
		, bFlipTangentY(bInFlipTangentY)
	{
	}
};


namespace RealtimeMesh
{
	class REALTIMEMESHCOMPONENT_API FRealtimeMeshProcedural : public FRealtimeMeshManaged
	{
	public:
		FRealtimeMeshProcedural(const FRealtimeMeshContextRef& InContext)
			: FRealtimeMeshManaged(InContext)
		{
		}

		// No factory overrides — Procedural uses the Managed-tier types
		// (Section / BufferSet / LOD all from FRealtimeMeshManaged's factories).

		friend class ::URealtimeMeshProcedural;
	};
}


/**
 * PMC-parity UObject leaf for RMC. Mirrors UProceduralMeshComponent's
 * Blueprint surface so PMC users can migrate by find-and-replace. Backed
 * by FRealtimeMeshManaged for collision/Nanite/DistanceField/etc; uses
 * a fixed LocalVertexFactory layout internally. Each PMC int32 section
 * index maps to its own FRealtimeMeshBufferSetKey containing a single
 * section — independent vertex/index buffers per section, matching PMC's
 * "update is local" semantics.
 *
 * For users that want polymorphic stream layouts, polygroup auto-
 * sectioning, or per-frame minimum-copy updates, use URealtimeMeshSimple
 * with the builder/StreamSet API instead.
 */
UCLASS(Blueprintable)
class REALTIMEMESHCOMPONENT_API URealtimeMeshProcedural : public URealtimeMeshManaged
{
	GENERATED_UCLASS_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh|Procedural")
	static URealtimeMeshProcedural* InitializeRealtimeMeshProcedural(URealtimeMeshComponent* Owner);

	TSharedRef<RealtimeMesh::FRealtimeMeshProcedural> GetProceduralMeshData() const
	{
		return StaticCastSharedRef<RealtimeMesh::FRealtimeMeshProcedural>(GetMesh());
	}

	/**
	 * Create a mesh section. Replaces any existing section at the same index.
	 */
	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh|Procedural",
		meta = (DisplayName = "Create Mesh Section", AutoCreateRefTerm = "Normals,UV0,VertexColors,Tangents"))
	void CreateMeshSection(int32 SectionIndex,
						   const TArray<FVector>& Vertices,
						   const TArray<int32>& Triangles,
						   const TArray<FVector>& Normals,
						   const TArray<FVector2D>& UV0,
						   const TArray<FColor>& VertexColors,
						   const TArray<FRealtimeMeshProceduralTangent>& Tangents,
						   bool bCreateCollision = false);

	/**
	 * Create a mesh section with linear-color vertex colors and 4 UV channels.
	 *
	 * bSRGBConversion controls how the FLinearColor inputs are packed into the
	 * FColor vertex stream. Defaults to false (no conversion) so the values you
	 * pass round-trip faithfully to the material's Vertex Color node; pass true
	 * if you instead want the stored color to match the swatch when displayed.
	 */
	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh|Procedural",
		meta = (DisplayName = "Create Mesh Section (LinearColor)", AutoCreateRefTerm = "Normals,UV0,UV1,UV2,UV3,VertexColors,Tangents", AdvancedDisplay = "UV1,UV2,UV3,bSRGBConversion"))
	void CreateMeshSection_LinearColor(int32 SectionIndex,
									   const TArray<FVector>& Vertices,
									   const TArray<int32>& Triangles,
									   const TArray<FVector>& Normals,
									   const TArray<FVector2D>& UV0,
									   const TArray<FVector2D>& UV1,
									   const TArray<FVector2D>& UV2,
									   const TArray<FVector2D>& UV3,
									   const TArray<FLinearColor>& VertexColors,
									   const TArray<FRealtimeMeshProceduralTangent>& Tangents,
									   bool bCreateCollision = false,
									   bool bSRGBConversion = false);

	/**
	 * Update vertex attributes on an existing section. Topology (triangles)
	 * is not modified; index count must match the original CreateMeshSection.
	 * Any of the optional arrays may be empty to leave that stream as-is.
	 */
	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh|Procedural",
		meta = (DisplayName = "Update Mesh Section", AutoCreateRefTerm = "Normals,UV0,VertexColors,Tangents"))
	void UpdateMeshSection(int32 SectionIndex,
						   const TArray<FVector>& Vertices,
						   const TArray<FVector>& Normals,
						   const TArray<FVector2D>& UV0,
						   const TArray<FColor>& VertexColors,
						   const TArray<FRealtimeMeshProceduralTangent>& Tangents);

	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh|Procedural",
		meta = (DisplayName = "Update Mesh Section (LinearColor)", AutoCreateRefTerm = "Normals,UV0,UV1,UV2,UV3,VertexColors,Tangents", AdvancedDisplay = "UV1,UV2,UV3,bSRGBConversion"))
	void UpdateMeshSection_LinearColor(int32 SectionIndex,
									   const TArray<FVector>& Vertices,
									   const TArray<FVector>& Normals,
									   const TArray<FVector2D>& UV0,
									   const TArray<FVector2D>& UV1,
									   const TArray<FVector2D>& UV2,
									   const TArray<FVector2D>& UV3,
									   const TArray<FLinearColor>& VertexColors,
									   const TArray<FRealtimeMeshProceduralTangent>& Tangents,
									   bool bSRGBConversion = false);

	/**
	 * Read a section's geometry back out into CPU arrays. Mirrors the data that
	 * CreateMeshSection_LinearColor would have written: positions, the triangle
	 * index buffer, and whichever vertex attributes the section actually carries
	 * (normals/tangents, vertex colors, and up to 4 UV channels). Optional output
	 * arrays are left empty when the corresponding stream isn't present.
	 *
	 * Note: tangents come back with bFlipTangentY = false — the flip bit is baked
	 * into the stored tangent basis at write time and can't be recovered.
	 *
	 * Returns false if no section exists at this index.
	 */
	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh|Procedural",
		meta = (DisplayName = "Get Mesh Section"))
	bool GetMeshSection(int32 SectionIndex,
						TArray<FVector>& Vertices,
						TArray<int32>& Triangles,
						TArray<FVector>& Normals,
						TArray<FVector2D>& UV0,
						TArray<FVector2D>& UV1,
						TArray<FVector2D>& UV2,
						TArray<FVector2D>& UV3,
						TArray<FColor>& VertexColors,
						TArray<FRealtimeMeshProceduralTangent>& Tangents) const;

	/** Remove the section at this index, if any. Index space is not compacted. */
	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh|Procedural")
	void ClearMeshSection(int32 SectionIndex);

	/** Remove every section. */
	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh|Procedural")
	void ClearAllMeshSections();

	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh|Procedural")
	void SetMeshSectionVisible(int32 SectionIndex, bool bNewVisibility);

	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh|Procedural")
	bool IsMeshSectionVisible(int32 SectionIndex) const;

	/**
	 * Matches UProceduralMeshComponent semantics: counts the highest-ever-
	 * allocated index plus one, including holes from ClearMeshSection.
	 */
	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh|Procedural")
	int32 GetNumSections() const;

	/**
	 * Append a convex hull to this mesh's simple-collision shapes.
	 * Translates to a new convex element on the Managed-tier
	 * FRealtimeMeshSimpleGeometry.
	 */
	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh|Procedural")
	void AddCollisionConvexMesh(const TArray<FVector>& ConvexVerts);

	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh|Procedural")
	void ClearCollisionConvexMeshes();

	/**
	 * Whether the per-poly (complex) collision should also be usable as simple
	 * collision. Mirrors UProceduralMeshComponent::bUseComplexAsSimpleCollision,
	 * but routed through the Managed-tier collision config rather than exposed as
	 * a raw property. Set to false if this mesh is given simple collision shapes
	 * and is meant to be simulated.
	 */
	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh|Procedural")
	bool GetUseComplexAsSimpleCollision() const;

	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh|Procedural")
	void SetUseComplexAsSimpleCollision(bool bNewUseComplexAsSimpleCollision);

	virtual void Reset() override;
	virtual void PostLoad() override;

private:
	UPROPERTY(Transient)
	TMap<int32, FRealtimeMeshBufferSetKey> SectionGroupByIndex;

	UPROPERTY(Transient)
	int32 MaxAllocatedIndex = 0;

	FRealtimeMeshBufferSetKey AllocateOrGetGroupKey(int32 SectionIndex);
	bool TryGetGroupKey(int32 SectionIndex, FRealtimeMeshBufferSetKey& OutKey) const;
	FRealtimeMeshSectionKey MakeSectionKey(const FRealtimeMeshBufferSetKey& GroupKey) const;
};
