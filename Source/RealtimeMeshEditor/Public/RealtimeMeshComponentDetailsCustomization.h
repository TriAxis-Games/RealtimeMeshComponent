// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"

class URealtimeMeshComponent;

struct FRealtimeMeshStreamKey;

/**
 * Details customization for URealtimeMeshComponent that shows mesh state information
 * including section count, vertex/triangle counts, Nanite usage, etc.
 */
class FRealtimeMeshComponentDetailsCustomization : public IDetailCustomization
{
public:
	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance();

	/** IDetailCustomization interface */
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

private:
	/** True when the current component is backed by a RealtimeMeshSimple with Nanite resources. */
	bool IsNaniteActive() const;
	EVisibility GetNaniteSectionVisibility() const;
	EVisibility GetStandardSectionVisibility() const;

	/** Per-row text getters. */
	FText GetMeshTypeText() const;
	FText GetBoundsText() const;
	FText GetVisibilityText() const;
	FText GetCastShadowText() const;
	FText GetMaterialSlotsText() const;

	/** Standard-mesh section. */
	FText GetLODCountText() const;
	FText GetVertexCountText() const;
	FText GetTriangleCountText() const;

	// Shared by the two stat getters above: sums Num() (divided by Divisor) of the given
	// stream across all section groups in LOD 0.
	FText SumLOD0StreamElements(const FRealtimeMeshStreamKey& StreamKey, int32 Divisor) const;

	/** Nanite section. */
	FText GetNaniteInputTrianglesText() const;
	FText GetNaniteInputVerticesText() const;
	FText GetNaniteClustersText() const;
	FText GetNaniteHierarchyNodesText() const;
	FText GetNaniteHierarchyDepthText() const;
	FText GetNanitePagesText() const;
	FText GetNaniteRootDataText() const;
	FText GetNaniteStreamingDataText() const;

	FReply OnCreateStaticMesh();
	bool IsCreateStaticMeshEnabled() const;
	bool ShowCreateStaticMeshDialog(FString& OutPackagePath, FString& OutAssetName) const;

	TWeakObjectPtr<URealtimeMeshComponent> ComponentPtr;
};