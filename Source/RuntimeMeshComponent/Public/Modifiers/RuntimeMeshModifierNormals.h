// Copyright 2016-2020 TriAxis Games L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RuntimeMeshModifier.h"
#include "RuntimeMeshModifierNormals.generated.h"

UCLASS(HideCategories = Object, BlueprintType, Blueprintable, Meta = (ShortTooltip = "A RuntimeMeshModifierNormals is a class that implements logic to generate normals and tangents for a supplied mesh."))
class RUNTIMEMESHCOMPONENT_API URuntimeMeshModifierNormals : public URuntimeMeshModifier
{
	GENERATED_BODY()
public:

	UPROPERTY(Category = "RuntimeMesh|Modifiers|Normals", VisibleAnywhere, BlueprintReadWrite)
	bool bComputeSmoothNormals;

public:
	URuntimeMeshModifierNormals();

	virtual void ApplyToMesh_Implementation(FRuntimeMeshRenderableMeshData& MeshData) override;

	UFUNCTION(BlueprintCallable, Category = "RuntimeMesh|Modifiers|Normals")
	static void CalculateNormalsTangents(FRuntimeMeshRenderableMeshData& MeshData, bool bInComputeSmoothNormals = false);

private:

	static TMultiMap<uint32, uint32> FindDuplicateVerticesMap(const FRuntimeMeshVertexPositionStream& PositionStream, float Tollerance = 0.0);

	struct FRuntimeMeshVertexSortingElement
	{
		float Value;
		int32 Index;

		FRuntimeMeshVertexSortingElement() {}

		FRuntimeMeshVertexSortingElement(int32 InIndex, FVector Vector)
		{
			Value = 0.30f * Vector.X + 0.33f * Vector.Y + 0.37f * Vector.Z;
			Index = InIndex;
		}
	};

	struct FRuntimeMeshVertexSortingFunction
	{
		FORCEINLINE bool operator()(FRuntimeMeshVertexSortingElement const& Left, FRuntimeMeshVertexSortingElement const& Right) const
		{
			return Left.Value < Right.Value;
		}
	};

};
