// Copyright (c) 2015-2025 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RealtimeMeshActor.h"
#include "RealtimeMeshVisualTest_VertexColors.generated.h"

/**
 * Visual test actor that demonstrates vertex color rendering with various gradient patterns.
 * Creates multiple meshes showcasing different vertex color techniques including:
 * - Rainbow gradient sphere
 * - RGB corner boxes
 * - Smooth radial gradients
 * - Height-based color gradients
 */
UCLASS()
class REALTIMEMESHTESTS_API ARealtimeMeshVisualTest_VertexColors : public ARealtimeMeshActor
{
	GENERATED_BODY()

public:
	ARealtimeMeshVisualTest_VertexColors();

protected:
	virtual void OnConstruction(const FTransform& Transform) override;

private:
	/** Creates a grid mesh with rainbow gradient */
	void CreateRainbowGrid(class URealtimeMeshSimple* RealtimeMesh, const FVector3f& Offset);

	/** Creates a box with RGB corner gradients */
	void CreateRGBCornerBox(class URealtimeMeshSimple* RealtimeMesh, const FVector3f& Offset);

	/** Creates a tessellated plane with radial gradient */
	void CreateRadialGradientPlane(class URealtimeMeshSimple* RealtimeMesh, const FVector3f& Offset);

	/** Creates a height gradient mesh (vertical rainbow) */
	void CreateHeightGradientMesh(class URealtimeMeshSimple* RealtimeMesh, const FVector3f& Offset);
};
