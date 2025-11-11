// Copyright (c) 2015-2025 TriAxis Games, L.L.C. All Rights Reserved.

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
	/** Creates the mesh state info widget */
	TSharedRef<SWidget> CreateMeshStateWidget();
	
	/** Gets individual mesh information properties */
	FText GetBoundsText() const;
	FText GetVisibilityText() const;
	FText GetCastShadowText() const;
	FText GetNaniteText() const;
	FText GetNaniteStatsText() const;
	FText GetMaterialSlotsText() const;
	FText GetVertexCountText() const;
	FText GetTriangleCountText() const;
	FText GetMemoryUsageText() const;

	/*// Handle create static mesh button click
	FReply OnCreateStaticMesh();
	
	// Check if create static mesh button should be enabled
	bool IsCreateStaticMeshEnabled() const;

	// Show dialog to choose static mesh path and name
	bool ShowCreateStaticMeshDialog(FString& OutPackagePath, FString& OutAssetName) const;*/

	// Weak reference to the component being customized
	TWeakObjectPtr<URealtimeMeshComponent> ComponentPtr;
};