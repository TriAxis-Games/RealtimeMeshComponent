// Copyright 2016-2020 TriAxis Games L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RuntimeMeshRenderable.h"
#include "RuntimeMeshSlicer.generated.h"

/** Options for creating cap geometry when slicing */
UENUM()
enum class ERuntimeMeshSliceCapOption : uint8
{
	/** Do not create cap geometry */
	NoCap,
	/** Add a new section to RuntimeMesh for cap */
	CreateNewSectionForCap,
	/** Add cap geometry to existing last section */
	UseLastSectionForCap
};


UCLASS()
class RUNTIMEMESHCOMPONENT_API URuntimeMeshSlicer : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/**
	*	Slice the RuntimeMeshComponent (including simple convex collision) using a plane. Optionally create 'cap' geometry.
	*	@param	InRuntimeMesh			RuntimeMeshComponent to slice
	*	@param	PlanePosition			Point on the plane to use for slicing, in world space
	*	@param	PlaneNormal				Normal of plane used for slicing. Geometry on the positive side of the plane will be kept.
	*	@param	bCreateOtherHalf		If true, an additional RuntimeMeshComponent (OutOtherHalfRuntimeMesh) will be created using the other half of the sliced geometry
	*	@param	OutOtherHalfRuntimeMesh	If bCreateOtherHalf is set, this is the new component created. Its owner will be the same as the supplied InRuntimeMesh.
	*	@param	CapOption				If and how to create 'cap' geometry on the slicing plane
	*	@param	CapMaterial				If creating a new section for the cap, assign this material to that section
	*/
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	static void SliceRuntimeMesh(URuntimeMeshComponent* InRuntimeMesh, FVector PlanePosition, FVector PlaneNormal, bool bCreateOtherHalf,
		URuntimeMeshComponent*& OutOtherHalfRuntimeMesh, ERuntimeMeshSliceCapOption CapOption, UMaterialInterface* CapMaterial);


	/**
	*	Slice the RuntimeMeshData using a plane. Optionally create 'cap' geometry.
	*	@param	SourceSection			RuntimeMeshData to slice
	*	@param	PlanePosition			Point on the plane to use for slicing, in world space
	*	@param	PlaneNormal				Normal of plane used for slicing. Geometry on the positive side of the plane will be kept.
	*	@param	CapOption				If and how to create 'cap' geometry on the slicing plane
	*	@param	NewSourceSection		Resulting mesh data for origin section
	*	@param	DestinationSection		Mesh data sliced from source
	*/
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	static bool SliceRuntimeMeshData(FRuntimeMeshRenderableMeshData& SourceSection, const FPlane& SlicePlane, ERuntimeMeshSliceCapOption CapOption,	FRuntimeMeshRenderableMeshData& NewSourceSection, 
		FRuntimeMeshRenderableMeshData& NewSourceSectionCap, bool bCreateDestination, FRuntimeMeshRenderableMeshData& DestinationSection, FRuntimeMeshRenderableMeshData& NewDestinationSectionCap);
};
