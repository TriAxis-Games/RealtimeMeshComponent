// Copyright (c) 2015-2025 TriAxis Games, L.L.C. All Rights Reserved.

#include "RealtimeMeshComponentDetailsCustomization.h"
#include "RealtimeMeshComponent.h"
#include "RealtimeMesh.h"
#include "RealtimeMeshSimple.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Framework/Text/SlateTextLayout.h"
#include "Styling/SlateColor.h"
#include "Styling/AppStyle.h"
#include "Core/RealtimeMeshDataTypes.h"
#include "Interface/Core/RealtimeMeshDataStream.h"
#include "PhysicsEngine/BodySetup.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/StaticMesh.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Dialogs/Dialogs.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "EditorDirectories.h"

#define LOCTEXT_NAMESPACE "RealtimeMeshComponentDetailsCustomization"

TSharedRef<IDetailCustomization> FRealtimeMeshComponentDetailsCustomization::MakeInstance()
{
	return MakeShareable(new FRealtimeMeshComponentDetailsCustomization);
}

void FRealtimeMeshComponentDetailsCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	// Get the object being customized
	TArray<TWeakObjectPtr<UObject>> ObjectsBeingCustomized;
	DetailBuilder.GetObjectsBeingCustomized(ObjectsBeingCustomized);
	
	if (ObjectsBeingCustomized.Num() > 0)
	{
		ComponentPtr = Cast<URealtimeMeshComponent>(ObjectsBeingCustomized[0].Get());
	}

	// Add a new category for mesh state information
	IDetailCategoryBuilder& MeshStateCategory = DetailBuilder.EditCategory("Mesh State", LOCTEXT("MeshStateCategory", "Mesh State"));
	
	// Add mesh state widget as full width
	MeshStateCategory.AddCustomRow(LOCTEXT("MeshStateRow", "Mesh State"))
		.WholeRowContent()
		[
			CreateMeshStateWidget()
		];

	// Add Create Static Mesh button to RealtimeMesh category
	IDetailCategoryBuilder& RealtimeMeshCategory = DetailBuilder.EditCategory("RealtimeMesh", LOCTEXT("RealtimeMeshCategory", "Realtime Mesh"));
	
	/*RealtimeMeshCategory.AddCustomRow(LOCTEXT("CreateStaticMeshRow", "Create Static Mesh"))
		.WholeRowContent()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0, 4, 8, 4)
			[
				SNew(SButton)
				.Text(LOCTEXT("CreateStaticMesh", "Create Static Mesh"))
				.ToolTipText(LOCTEXT("CreateStaticMeshTooltip", "Create a new Static Mesh asset from this Realtime Mesh Component"))
				.OnClicked(this, &FRealtimeMeshComponentDetailsCustomization::OnCreateStaticMesh)
				.IsEnabled(this, &FRealtimeMeshComponentDetailsCustomization::IsCreateStaticMeshEnabled)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.ButtonStyle(FAppStyle::Get(), "FlatButton.Success")
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("CreateStaticMeshDescription", "Export mesh geometry to a Static Mesh asset"))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				.Justification(ETextJustify::Left)
			]
		];*/
}

TSharedRef<SWidget> FRealtimeMeshComponentDetailsCustomization::CreateMeshStateWidget()
{
	return SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("ToolPanel.GroupBorder"))
		.Padding(8.0f)
		[
			SNew(SHorizontalBox)
			
			// Left Column
			+ SHorizontalBox::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Top)
			.FillWidth(0.5f)
			[
				SNew(SVerticalBox)
				
				// Bounds
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 2)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("BoundsLabel", "Bounds:"))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
						.ColorAndOpacity(FSlateColor::UseForeground())
					]
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.Padding(8, 0, 0, 0)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(this, &FRealtimeMeshComponentDetailsCustomization::GetBoundsText)
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
						.AutoWrapText(true)
					]
				]
				
				// Visibility
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 2)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("VisibleLabel", "Visible:"))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
						.ColorAndOpacity(FSlateColor::UseForeground())
					]
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.Padding(8, 0, 0, 0)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(this, &FRealtimeMeshComponentDetailsCustomization::GetVisibilityText)
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
					]
				]
				
				// Cast Shadow
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 2)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("CastShadowLabel", "Cast Shadow:"))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
						.ColorAndOpacity(FSlateColor::UseForeground())
					]
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.Padding(8, 0, 0, 0)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(this, &FRealtimeMeshComponentDetailsCustomization::GetCastShadowText)
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
					]
				]
			]
			
			// Right Column
			+ SHorizontalBox::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Top)
			.FillWidth(0.5f)
			.Padding(16, 0, 0, 0)
			[
				SNew(SVerticalBox)
				
				// Nanite
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 2)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("NaniteLabel", "Nanite:"))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
						.ColorAndOpacity(FSlateColor::UseForeground())
					]
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.Padding(8, 0, 0, 0)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(this, &FRealtimeMeshComponentDetailsCustomization::GetNaniteText)
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
					]
				]

				// Nanite Stats
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 2)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("NaniteStatsLabel", "Nanite Stats:"))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
						.ColorAndOpacity(FSlateColor::UseForeground())
					]
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.Padding(8, 0, 0, 0)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(this, &FRealtimeMeshComponentDetailsCustomization::GetNaniteStatsText)
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
						.AutoWrapText(true)
					]
				]
				
				// Material Slots
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 2)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("MaterialSlotsLabel", "Material Slots:"))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
						.ColorAndOpacity(FSlateColor::UseForeground())
					]
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.Padding(8, 0, 0, 0)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(this, &FRealtimeMeshComponentDetailsCustomization::GetMaterialSlotsText)
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
					]
				]
				
				// Vertex Count
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 2)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("VertexCountLabel", "Vertices:"))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
						.ColorAndOpacity(FSlateColor::UseForeground())
					]
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.Padding(8, 0, 0, 0)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(this, &FRealtimeMeshComponentDetailsCustomization::GetVertexCountText)
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
					]
				]
				
				// Triangle Count
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 2)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("TriangleCountLabel", "Triangles:"))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
						.ColorAndOpacity(FSlateColor::UseForeground())
					]
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.Padding(8, 0, 0, 0)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(this, &FRealtimeMeshComponentDetailsCustomization::GetTriangleCountText)
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
					]
				]
				
				// Memory Usage
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 2)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("MemoryUsageLabel", "Memory Usage:"))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
						.ColorAndOpacity(FSlateColor::UseForeground())
					]
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.Padding(8, 0, 0, 0)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(this, &FRealtimeMeshComponentDetailsCustomization::GetMemoryUsageText)
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
					]
				]
			]
		];
}

FText FRealtimeMeshComponentDetailsCustomization::GetBoundsText() const
{
	if (!ComponentPtr.IsValid())
	{
		return LOCTEXT("NoComponent", "N/A");
	}

	URealtimeMeshComponent* Component = ComponentPtr.Get();
	FBoxSphereBounds LocalBounds = Component->GetLocalBounds();
	
	FString BoundsText = FString::Printf(TEXT("%.1f x %.1f x %.1f"), 
		LocalBounds.BoxExtent.X * 2.0f, LocalBounds.BoxExtent.Y * 2.0f, LocalBounds.BoxExtent.Z * 2.0f);
	
	return FText::FromString(BoundsText);
}

FText FRealtimeMeshComponentDetailsCustomization::GetVisibilityText() const
{
	if (!ComponentPtr.IsValid())
	{
		return LOCTEXT("NoComponent", "N/A");
	}

	URealtimeMeshComponent* Component = ComponentPtr.Get();
	return Component->IsVisible() ? LOCTEXT("Yes", "Yes") : LOCTEXT("No", "No");
}

FText FRealtimeMeshComponentDetailsCustomization::GetCastShadowText() const
{
	if (!ComponentPtr.IsValid())
	{
		return LOCTEXT("NoComponent", "N/A");
	}

	URealtimeMeshComponent* Component = ComponentPtr.Get();
	return Component->CastShadow ? LOCTEXT("Yes", "Yes") : LOCTEXT("No", "No");
}

FText FRealtimeMeshComponentDetailsCustomization::GetNaniteText() const
{
	if (!ComponentPtr.IsValid())
	{
		return LOCTEXT("NoComponent", "N/A");
	}

	URealtimeMeshComponent* Component = ComponentPtr.Get();
	URealtimeMesh* Mesh = Component->GetRealtimeMesh();

	if (!IsValid(Mesh))
	{
		return LOCTEXT("NoMesh", "N/A");
	}

	// Check if this is a RealtimeMeshSimple with Nanite resources
	if (URealtimeMeshSimple* RealtimeMeshSimple = Cast<URealtimeMeshSimple>(Mesh))
	{
		RealtimeMesh::FRealtimeMeshAccessContext LockContext(RealtimeMeshSimple->GetMesh()->GetSharedResources());
		bool bHasNanite = RealtimeMeshSimple->GetMeshData()->HasNaniteResources(LockContext);
		return bHasNanite ? LOCTEXT("Active", "Active") : LOCTEXT("Inactive", "Inactive");
	}

	return LOCTEXT("NotSupported", "Not Supported");
}

FText FRealtimeMeshComponentDetailsCustomization::GetNaniteStatsText() const
{
	if (!ComponentPtr.IsValid())
	{
		return LOCTEXT("NoComponent", "N/A");
	}

	URealtimeMeshComponent* Component = ComponentPtr.Get();
	URealtimeMesh* Mesh = Component->GetRealtimeMesh();

	if (!IsValid(Mesh))
	{
		return LOCTEXT("NoMesh", "N/A");
	}

	// Check if this is a RealtimeMeshSimple with Nanite resources
	if (URealtimeMeshSimple* RealtimeMeshSimple = Cast<URealtimeMeshSimple>(Mesh))
	{
		RealtimeMesh::FRealtimeMeshAccessContext LockContext(RealtimeMeshSimple->GetMesh()->GetSharedResources());

		if (!RealtimeMeshSimple->GetMeshData()->HasNaniteResources(LockContext))
		{
			return LOCTEXT("NoNaniteData", "N/A");
		}

		const RealtimeMesh::FRealtimeMeshNaniteResources& NaniteResources = RealtimeMeshSimple->GetMeshData()->GetNaniteResources();

		// Access Nanite stats from the resources (inherited from ::Nanite::FResources)
		const ::Nanite::FResources* NanitePtr = const_cast<RealtimeMesh::FRealtimeMeshNaniteResources&>(NaniteResources).GetNanitePtr();

		if (NanitePtr)
		{
			FString StatsText = FString::Printf(TEXT("Clusters: %d, Nodes: %d"),
				NanitePtr->NumClusters,
				NanitePtr->HierarchyNodes.Num());
			return FText::FromString(StatsText);
		}
	}

	return LOCTEXT("NoNaniteStats", "N/A");
}

FText FRealtimeMeshComponentDetailsCustomization::GetMaterialSlotsText() const
{
	if (!ComponentPtr.IsValid())
	{
		return LOCTEXT("NoComponent", "N/A");
	}

	URealtimeMeshComponent* Component = ComponentPtr.Get();
	int32 MaterialCount = Component->GetNumMaterials();
	return FText::AsNumber(MaterialCount);
}

FText FRealtimeMeshComponentDetailsCustomization::GetVertexCountText() const
{
	if (!ComponentPtr.IsValid())
	{
		return LOCTEXT("NoComponent", "N/A");
	}

	URealtimeMeshComponent* Component = ComponentPtr.Get();
	URealtimeMesh* Mesh = Component->GetRealtimeMesh();
	
	if (!IsValid(Mesh))
	{
		return LOCTEXT("NoMesh", "N/A");
	}

	// Try to cast to URealtimeMeshSimple to get mesh data
	if (URealtimeMeshSimple* RealtimeMeshSimple = Cast<URealtimeMeshSimple>(Mesh))
	{
		int32 TotalVertices = 0;
		TArray<FRealtimeMeshLODKey> LODs = RealtimeMeshSimple->GetLODs();
		
		for (const FRealtimeMeshLODKey& LODKey : LODs)
		{
			TArray<FRealtimeMeshSectionGroupKey> SectionGroups = RealtimeMeshSimple->GetSectionGroups(LODKey);
			
			for (const FRealtimeMeshSectionGroupKey& SectionGroupKey : SectionGroups)
			{
				RealtimeMeshSimple->ProcessMesh(SectionGroupKey, [&](const RealtimeMesh::FRealtimeMeshStreamSet& StreamSet)
				{
					if (const auto* PositionStream = StreamSet.Find(RealtimeMesh::FRealtimeMeshStreams::Position))
					{
						TotalVertices += PositionStream->Num();
					}
				});
			}
			break; // Only count LOD 0 for now
		}
		
		return FText::AsNumber(TotalVertices);
	}
	
	return LOCTEXT("UnsupportedMeshType", "N/A");
}

FText FRealtimeMeshComponentDetailsCustomization::GetTriangleCountText() const
{
	if (!ComponentPtr.IsValid())
	{
		return LOCTEXT("NoComponent", "N/A");
	}

	URealtimeMeshComponent* Component = ComponentPtr.Get();
	URealtimeMesh* Mesh = Component->GetRealtimeMesh();
	
	if (!IsValid(Mesh))
	{
		return LOCTEXT("NoMesh", "N/A");
	}

	// Try to cast to URealtimeMeshSimple to get mesh data
	if (URealtimeMeshSimple* RealtimeMeshSimple = Cast<URealtimeMeshSimple>(Mesh))
	{
		int32 TotalTriangles = 0;
		TArray<FRealtimeMeshLODKey> LODs = RealtimeMeshSimple->GetLODs();
		
		for (const FRealtimeMeshLODKey& LODKey : LODs)
		{
			TArray<FRealtimeMeshSectionGroupKey> SectionGroups = RealtimeMeshSimple->GetSectionGroups(LODKey);
			
			for (const FRealtimeMeshSectionGroupKey& SectionGroupKey : SectionGroups)
			{
				RealtimeMeshSimple->ProcessMesh(SectionGroupKey, [&](const RealtimeMesh::FRealtimeMeshStreamSet& StreamSet)
				{
					if (const auto* TriangleStream = StreamSet.Find(RealtimeMesh::FRealtimeMeshStreams::Triangles))
					{
						TotalTriangles += TriangleStream->Num() / 3; // 3 indices per triangle
					}
				});
			}
			break; // Only count LOD 0 for now
		}
		
		return FText::AsNumber(TotalTriangles);
	}
	
	return LOCTEXT("UnsupportedMeshType", "N/A");
}

FText FRealtimeMeshComponentDetailsCustomization::GetMemoryUsageText() const
{
	if (!ComponentPtr.IsValid())
	{
		return LOCTEXT("NoComponent", "N/A");
	}

	URealtimeMeshComponent* Component = ComponentPtr.Get();
	SIZE_T ComponentSize = Component->GetResourceSizeBytes(EResourceSizeMode::EstimatedTotal);
	
	if (ComponentSize > 0)
	{
		if (ComponentSize > 1024 * 1024)
		{
			return FText::FromString(FString::Printf(TEXT("%.1f MB"), ComponentSize / (1024.0f * 1024.0f)));
		}
		else if (ComponentSize > 1024)
		{
			return FText::FromString(FString::Printf(TEXT("%.1f KB"), ComponentSize / 1024.0f));
		}
		else
		{
			return FText::FromString(FString::Printf(TEXT("%d B"), (int32)ComponentSize));
		}
	}
	
	return LOCTEXT("MemoryNA", "N/A");
}

/*FReply FRealtimeMeshComponentDetailsCustomization::OnCreateStaticMesh()
{
	if (!ComponentPtr.IsValid())
	{
		return FReply::Handled();
	}

	URealtimeMeshComponent* Component = ComponentPtr.Get();
	URealtimeMesh* RealtimeMesh = Component->GetRealtimeMesh();
	
	if (!IsValid(RealtimeMesh))
	{
		return FReply::Handled();
	}

	// Try to cast to URealtimeMeshSimple for conversion
	URealtimeMeshSimple* RealtimeMeshSimple = Cast<URealtimeMeshSimple>(RealtimeMesh);
	if (!RealtimeMeshSimple)
	{
		// Show notification that this RealtimeMesh type is not supported
		FNotificationInfo NotificationInfo(LOCTEXT("UnsupportedMeshType", "Static Mesh creation is currently only supported for RealtimeMeshSimple"));
		NotificationInfo.ExpireDuration = 5.0f;
		NotificationInfo.bUseLargeFont = false;
		FSlateNotificationManager::Get().AddNotification(NotificationInfo);
		return FReply::Handled();
	}

	// Show dialog to choose path and name
	FString PackagePath;
	FString AssetName;
	if (!ShowCreateStaticMeshDialog(PackagePath, AssetName))
	{
		return FReply::Handled(); // User cancelled
	}

	// Get asset tools for creating the static mesh
	FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");
	IAssetTools& AssetTools = AssetToolsModule.Get();

	FString FinalPackageName = PackagePath + TEXT("/") + AssetName;
	FString FinalMeshName = AssetName;

	// Create the static mesh asset
	UStaticMesh* StaticMesh = Cast<UStaticMesh>(AssetTools.CreateAsset(FinalMeshName, FPaths::GetPath(FinalPackageName), UStaticMesh::StaticClass(), nullptr));
	
	if (!StaticMesh)
	{
		FNotificationInfo NotificationInfo(LOCTEXT("FailedToCreateAsset", "Failed to create Static Mesh asset"));
		NotificationInfo.ExpireDuration = 5.0f;
		NotificationInfo.bUseLargeFont = false;
		FSlateNotificationManager::Get().AddNotification(NotificationInfo);
		return FReply::Handled();
	}

	// Convert RealtimeMesh to StaticMesh
	FRealtimeMeshStaticMeshConversionOptions ConversionOptions;
	ConversionOptions.bWantTangents = true;
	ConversionOptions.bWantUVs = true;
	ConversionOptions.bWantVertexColors = true;
	ConversionOptions.bWantsDistanceField = true;
	ConversionOptions.bWantsMaterials = true;
	ConversionOptions.MinLODIndex = 0;
	ConversionOptions.MaxLODIndex = 0; // Just convert LOD 0 for now

	ERealtimeMeshOutcomePins Outcome;
	URealtimeMeshStaticMeshConverter::CopyRealtimeMeshToStaticMesh(
		RealtimeMeshSimple,
		StaticMesh,
		ConversionOptions,
		Outcome
	);

	if (Outcome == ERealtimeMeshOutcomePins::Success)
	{
		// Mark the package as dirty and save
		StaticMesh->MarkPackageDirty();
		
		// Show success notification
		FNotificationInfo NotificationInfo(FText::Format(
			LOCTEXT("StaticMeshCreated", "Static Mesh '{0}' created successfully!"),
			FText::FromString(FinalMeshName)
		));
		NotificationInfo.ExpireDuration = 5.0f;
		NotificationInfo.bUseLargeFont = false;
		FSlateNotificationManager::Get().AddNotification(NotificationInfo);

		// Open content browser and select the new asset
		FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
		TArray<FAssetData> AssetsToSync;
		AssetsToSync.Add(FAssetData(StaticMesh));
		ContentBrowserModule.Get().SyncBrowserToAssets(AssetsToSync);
	}
	else
	{
		// Show failure notification
		FNotificationInfo NotificationInfo(LOCTEXT("ConversionFailed", "Failed to convert RealtimeMesh to Static Mesh"));
		NotificationInfo.ExpireDuration = 5.0f;
		NotificationInfo.bUseLargeFont = false;
		FSlateNotificationManager::Get().AddNotification(NotificationInfo);
	}

	return FReply::Handled();
}

bool FRealtimeMeshComponentDetailsCustomization::IsCreateStaticMeshEnabled() const
{
	if (!ComponentPtr.IsValid())
	{
		return false;
	}

	URealtimeMeshComponent* Component = ComponentPtr.Get();
	URealtimeMesh* RealtimeMesh = Component->GetRealtimeMesh();
	
	// Only enable if we have a valid RealtimeMesh assigned
	return IsValid(RealtimeMesh);
}

bool FRealtimeMeshComponentDetailsCustomization::ShowCreateStaticMeshDialog(FString& OutPackagePath, FString& OutAssetName) const
{
	// Default values
	FString DefaultPath = TEXT("/Game/GeneratedMeshes");
	FString DefaultName = TEXT("RealtimeMeshComponent_StaticMesh");
	
	if (ComponentPtr.IsValid())
	{
		URealtimeMeshComponent* Component = ComponentPtr.Get();
		DefaultName = FString::Printf(TEXT("%s_StaticMesh"), *Component->GetName());
	}

	// Create dialog content
	TSharedPtr<SEditableTextBox> PackagePathTextBox;
	TSharedPtr<SEditableTextBox> AssetNameTextBox;

	TSharedRef<SVerticalBox> DialogContent = SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0, 0, 0, 8)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("CreateStaticMeshDialogTitle", "Create Static Mesh Asset"))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0, 4)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0, 0, 8, 0)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("PackagePath", "Package Path:"))
				.MinDesiredWidth(80)
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SAssignNew(PackagePathTextBox, SEditableTextBox)
				.Text(FText::FromString(DefaultPath))
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0, 4)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0, 0, 8, 0)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("AssetName", "Asset Name:"))
				.MinDesiredWidth(80)
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SAssignNew(AssetNameTextBox, SEditableTextBox)
				.Text(FText::FromString(DefaultName))
			]
		];

	// Show modal dialog
	TSharedRef<SWindow> DialogWindow = SNew(SWindow)
		.Title(LOCTEXT("CreateStaticMeshDialogWindowTitle", "Create Static Mesh"))
		.SizingRule(ESizingRule::UserSized)
		.ClientSize(FVector2D(400, 150))
		.SupportsMaximize(false)
		.SupportsMinimize(false);

	TSharedPtr<SButton> OkButton;
	TSharedPtr<SButton> CancelButton;

	TSharedRef<SWidget> ButtonRow = SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		[
			SNullWidget::NullWidget
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(4, 0)
		[
			SAssignNew(OkButton, SButton)
			.Text(LOCTEXT("OK", "OK"))
			.OnClicked_Lambda([&DialogWindow]() -> FReply
			{
				DialogWindow->RequestDestroyWindow();
				return FReply::Handled();
			})
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SAssignNew(CancelButton, SButton)
			.Text(LOCTEXT("Cancel", "Cancel"))
			.OnClicked_Lambda([&DialogWindow]() -> FReply
			{
				DialogWindow->SetRequestDestroyWindowOverride(FRequestDestroyWindowOverride::CreateLambda([](const TSharedRef<SWindow>&)
				{
					// Set a flag or return value to indicate cancellation
				}));
				DialogWindow->RequestDestroyWindow();
				return FReply::Handled();
			})
		];

	TSharedRef<SVerticalBox> FullContent = SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		.Padding(16)
		[
			DialogContent
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(16, 8)
		[
			ButtonRow
		];

	DialogWindow->SetContent(FullContent);

	bool bUserConfirmed = true;
	bool bDialogClosed = false;

	DialogWindow->SetOnWindowClosed(FOnWindowClosed::CreateLambda([&bDialogClosed, &bUserConfirmed](const TSharedRef<SWindow>& Window)
	{
		bDialogClosed = true;
		// If the dialog was closed without clicking OK, treat as cancel
		// This is a simplified approach - in practice you'd want more robust state tracking
	}));

	// Override for cancel button
	CancelButton->SetOnClicked(FOnClicked::CreateLambda([&DialogWindow, &bUserConfirmed]() -> FReply
	{
		bUserConfirmed = false;
		DialogWindow->RequestDestroyWindow();
		return FReply::Handled();
	}));

	GEditor->EditorAddModalWindow(DialogWindow);

	if (bUserConfirmed && PackagePathTextBox.IsValid() && AssetNameTextBox.IsValid())
	{
		OutPackagePath = PackagePathTextBox->GetText().ToString();
		OutAssetName = AssetNameTextBox->GetText().ToString();
		return true;
	}

	return false;
}*/

#undef LOCTEXT_NAMESPACE