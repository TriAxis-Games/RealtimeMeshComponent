// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#include "RealtimeMeshComponentDetailsCustomization.h"
#include "RealtimeMeshComponent.h"
#include "RealtimeMesh.h"
#include "RealtimeMeshSimple.h"
#include "RealtimeMeshStaticMeshConverter.h"
#include "Editor.h"
#include "Widgets/SWindow.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailGroup.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Framework/Text/SlateTextLayout.h"
#include "Styling/SlateColor.h"
#include "Styling/AppStyle.h"
#include "Core/RealtimeMeshDataTypes.h"
#include "Core/RealtimeMeshDataStream.h"
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
	TArray<TWeakObjectPtr<UObject>> ObjectsBeingCustomized;
	DetailBuilder.GetObjectsBeingCustomized(ObjectsBeingCustomized);

	if (ObjectsBeingCustomized.Num() > 0)
	{
		ComponentPtr = Cast<URealtimeMeshComponent>(ObjectsBeingCustomized[0].Get());
	}

	// Native-style details layout: each section is an IDetailGroup with
	// NameContent/ValueContent rows. Looks and styles like the engine's own
	// StaticMesh / Component panels — uniform alignment, hover, expand/collapse,
	// no custom border or hand-built Slate grid.
	IDetailCategoryBuilder& MeshStateCategory = DetailBuilder.EditCategory("Mesh Stats", LOCTEXT("MeshStatsCategory", "Mesh Stats"));

	const auto AddRow = [this](IDetailGroup& Group, FName RowId, const FText& Label,
		FText (FRealtimeMeshComponentDetailsCustomization::*Getter)() const,
		TAttribute<EVisibility> Visibility = EVisibility::Visible)
	{
		FDetailWidgetRow& Row = Group.AddWidgetRow();
		Row.FilterString(Label);
		Row.Visibility(Visibility);
		Row.NameContent()
		[
			SNew(STextBlock)
			.Text(Label)
			.Font(IDetailLayoutBuilder::GetDetailFont())
		];
		Row.ValueContent()
		.MinDesiredWidth(180.0f)
		[
			SNew(STextBlock)
			.Text(TAttribute<FText>::CreateRaw(this, Getter))
			.Font(IDetailLayoutBuilder::GetDetailFont())
		];
	};

	// Component group (always shown).
	IDetailGroup& ComponentGroup = MeshStateCategory.AddGroup(TEXT("Component"), LOCTEXT("ComponentGroup", "Component"), false, true);
	AddRow(ComponentGroup, TEXT("MeshType"),     LOCTEXT("MeshTypeLabel",      "Mesh Type"),     &FRealtimeMeshComponentDetailsCustomization::GetMeshTypeText);
	AddRow(ComponentGroup, TEXT("Bounds"),       LOCTEXT("BoundsLabel",        "Bounds"),        &FRealtimeMeshComponentDetailsCustomization::GetBoundsText);
	AddRow(ComponentGroup, TEXT("Visible"),      LOCTEXT("VisibleLabel",       "Visible"),       &FRealtimeMeshComponentDetailsCustomization::GetVisibilityText);
	AddRow(ComponentGroup, TEXT("CastShadow"),   LOCTEXT("CastShadowLabel",    "Cast Shadow"),   &FRealtimeMeshComponentDetailsCustomization::GetCastShadowText);
	AddRow(ComponentGroup, TEXT("Materials"),    LOCTEXT("MaterialSlotsLabel", "Materials"),     &FRealtimeMeshComponentDetailsCustomization::GetMaterialSlotsText);

	// Geometry / Nanite groups are conditional on the current mesh state. Header
	// visibility is bound so the group folds out of the layout when its rows don't
	// apply — toggling Nanite on the mesh at runtime takes effect on next reselect.
	IDetailGroup& GeometryGroup = MeshStateCategory.AddGroup(TEXT("Geometry"), LOCTEXT("GeometryGroup", "Geometry"), false, true);
	GeometryGroup.HeaderRow().Visibility(TAttribute<EVisibility>::CreateRaw(this, &FRealtimeMeshComponentDetailsCustomization::GetStandardSectionVisibility));
	const TAttribute<EVisibility> StandardVis = TAttribute<EVisibility>::CreateRaw(this, &FRealtimeMeshComponentDetailsCustomization::GetStandardSectionVisibility);
	AddRow(GeometryGroup, TEXT("LODs"),          LOCTEXT("LODsLabel",         "LODs"),              &FRealtimeMeshComponentDetailsCustomization::GetLODCountText,      StandardVis);
	AddRow(GeometryGroup, TEXT("Vertices"),      LOCTEXT("VerticesLabel",     "Vertices (LOD 0)"),  &FRealtimeMeshComponentDetailsCustomization::GetVertexCountText,   StandardVis);
	AddRow(GeometryGroup, TEXT("Triangles"),     LOCTEXT("TrianglesLabel",    "Triangles (LOD 0)"), &FRealtimeMeshComponentDetailsCustomization::GetTriangleCountText, StandardVis);

	IDetailGroup& NaniteGroup = MeshStateCategory.AddGroup(TEXT("Nanite"), LOCTEXT("NaniteGroup", "Nanite"), false, true);
	NaniteGroup.HeaderRow().Visibility(TAttribute<EVisibility>::CreateRaw(this, &FRealtimeMeshComponentDetailsCustomization::GetNaniteSectionVisibility));
	const TAttribute<EVisibility> NaniteVis = TAttribute<EVisibility>::CreateRaw(this, &FRealtimeMeshComponentDetailsCustomization::GetNaniteSectionVisibility);
	AddRow(NaniteGroup, TEXT("NaniteTris"),     LOCTEXT("NaniteTrisLabel",     "Triangles"),         &FRealtimeMeshComponentDetailsCustomization::GetNaniteInputTrianglesText, NaniteVis);
	AddRow(NaniteGroup, TEXT("NaniteVerts"),    LOCTEXT("NaniteVertsLabel",    "Vertices"),          &FRealtimeMeshComponentDetailsCustomization::GetNaniteInputVerticesText,  NaniteVis);
	AddRow(NaniteGroup, TEXT("NaniteClusters"), LOCTEXT("NaniteClustersLabel", "Clusters"),          &FRealtimeMeshComponentDetailsCustomization::GetNaniteClustersText,       NaniteVis);
	AddRow(NaniteGroup, TEXT("NaniteHierNodes"),LOCTEXT("NaniteHierNodesLabel","Hierarchy Nodes"),   &FRealtimeMeshComponentDetailsCustomization::GetNaniteHierarchyNodesText, NaniteVis);
	AddRow(NaniteGroup, TEXT("NaniteHierDepth"),LOCTEXT("NaniteHierDepthLabel","Max Depth"),         &FRealtimeMeshComponentDetailsCustomization::GetNaniteHierarchyDepthText, NaniteVis);
	AddRow(NaniteGroup, TEXT("NanitePages"),    LOCTEXT("NanitePagesLabel",    "Pages"),             &FRealtimeMeshComponentDetailsCustomization::GetNanitePagesText,          NaniteVis);
	AddRow(NaniteGroup, TEXT("NaniteRoot"),     LOCTEXT("NaniteRootLabel",     "Root Data"),         &FRealtimeMeshComponentDetailsCustomization::GetNaniteRootDataText,       NaniteVis);
	AddRow(NaniteGroup, TEXT("NaniteStream"),   LOCTEXT("NaniteStreamLabel",   "Streaming Data"),    &FRealtimeMeshComponentDetailsCustomization::GetNaniteStreamingDataText,  NaniteVis);

	IDetailCategoryBuilder& RealtimeMeshCategory = DetailBuilder.EditCategory("RealtimeMesh", LOCTEXT("RealtimeMeshCategory", "Realtime Mesh"));

	RealtimeMeshCategory.AddCustomRow(LOCTEXT("CreateStaticMeshRow", "Create Static Mesh"))
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

bool FRealtimeMeshComponentDetailsCustomization::IsNaniteActive() const
{
	if (!ComponentPtr.IsValid())
	{
		return false;
	}
	URealtimeMeshComponent* Component = ComponentPtr.Get();
	URealtimeMeshSimple* RealtimeMeshSimple = Cast<URealtimeMeshSimple>(Component->GetRealtimeMesh());
	if (!RealtimeMeshSimple)
	{
		return false;
	}
	RealtimeMesh::FRealtimeMeshAccessContext LockContext(RealtimeMeshSimple->GetMesh()->GetContext());
	return RealtimeMeshSimple->GetMeshData()->HasNaniteResources(LockContext);
}

EVisibility FRealtimeMeshComponentDetailsCustomization::GetNaniteSectionVisibility() const
{
	return IsNaniteActive() ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility FRealtimeMeshComponentDetailsCustomization::GetStandardSectionVisibility() const
{
	return IsNaniteActive() ? EVisibility::Collapsed : EVisibility::Visible;
}

FText FRealtimeMeshComponentDetailsCustomization::GetMeshTypeText() const
{
	if (!ComponentPtr.IsValid())
	{
		return LOCTEXT("NoComponent", "N/A");
	}
	URealtimeMeshComponent* Component = ComponentPtr.Get();
	URealtimeMesh* Mesh = Component->GetRealtimeMesh();
	if (!IsValid(Mesh))
	{
		return LOCTEXT("NoMesh", "No Mesh");
	}
	if (Cast<URealtimeMeshSimple>(Mesh))
	{
		return IsNaniteActive()
			? LOCTEXT("MeshTypeNanite",   "Nanite (RealtimeMeshSimple)")
			: LOCTEXT("MeshTypeStandard", "Standard (RealtimeMeshSimple)");
	}
	return LOCTEXT("MeshTypeOther", "Custom URealtimeMesh");
}

namespace
{
	// Pretty-print a byte count as B / KB / MB.
	FString FormatBytes(int64 Bytes)
	{
		if (Bytes <= 0)
		{
			return TEXT("0 B");
		}
		const double KB = 1024.0;
		const double MB = 1024.0 * 1024.0;
		if (Bytes >= static_cast<int64>(MB))
		{
			return FString::Printf(TEXT("%.2f MB"), static_cast<double>(Bytes) / MB);
		}
		if (Bytes >= static_cast<int64>(KB))
		{
			return FString::Printf(TEXT("%.1f KB"), static_cast<double>(Bytes) / KB);
		}
		return FString::Printf(TEXT("%lld B"), Bytes);
	}
}

// Resolve the active Nanite resource pointer (polymorphic). Returns nullptr if the component isn't
// backed by a RealtimeMeshSimple with Nanite enabled — callers surface "N/A" in that case.
static const RealtimeMesh::FRealtimeMeshNaniteEngineResources* ResolveNaniteResources(URealtimeMeshComponent* Component)
{
	if (!Component) { return nullptr; }
	URealtimeMeshSimple* RealtimeMeshSimple = Cast<URealtimeMeshSimple>(Component->GetRealtimeMesh());
	if (!RealtimeMeshSimple) { return nullptr; }
	RealtimeMesh::FRealtimeMeshAccessContext LockContext(RealtimeMeshSimple->GetMesh()->GetContext());
	const RealtimeMesh::FRealtimeMeshNaniteResources* NaniteResources = RealtimeMeshSimple->GetMeshData()->GetNaniteResources(LockContext);
	if (!NaniteResources)
	{
		return nullptr;
	}
	// GetNaniteProvider() here resolves to the const overload, returning a const provider off the
	// live, initialized managed-mesh instance — RuntimeResourceID / residency values are real.
	return NaniteResources->GetNaniteProvider();
}

// Provision-array accessors that work across build tiers: on the fork the arrays are behind the provider's
// const getters; on a stock engine they are plain fields on FResources. Identity scalars (NumRootPages,
// NumInputTriangles, ...) are fields on both types, so those reads need no branch.
namespace
{
	using FNaniteResObj = RealtimeMesh::FRealtimeMeshNaniteEngineResources;
#if RMC_NANITE_ENGINE_PROVIDER
	static TConstArrayView<uint8>							NaniteRootDataView(const FNaniteResObj* R)			{ return R->GetRootData(); }
	static TConstArrayView<::Nanite::FPackedHierarchyNode>	NaniteHierarchyNodesView(const FNaniteResObj* R)	{ return R->GetHierarchyNodes(); }
	static TConstArrayView<::Nanite::FPageStreamingState>	NanitePageStatesView(const FNaniteResObj* R)		{ return R->GetPageStreamingStates(); }
#else
	static TConstArrayView<uint8>							NaniteRootDataView(const FNaniteResObj* R)			{ return R->RootData; }
	static TConstArrayView<::Nanite::FPackedHierarchyNode>	NaniteHierarchyNodesView(const FNaniteResObj* R)	{ return R->HierarchyNodes; }
	static TConstArrayView<::Nanite::FPageStreamingState>	NanitePageStatesView(const FNaniteResObj* R)		{ return R->PageStreamingStates; }
#endif
}

FText FRealtimeMeshComponentDetailsCustomization::GetLODCountText() const
{
	if (!ComponentPtr.IsValid())
	{
		return LOCTEXT("NoComponent", "N/A");
	}
	URealtimeMeshSimple* RealtimeMeshSimple = Cast<URealtimeMeshSimple>(ComponentPtr->GetRealtimeMesh());
	if (!RealtimeMeshSimple)
	{
		return LOCTEXT("NoMesh", "N/A");
	}
	return FText::AsNumber(RealtimeMeshSimple->GetLODs().Num());
}

FText FRealtimeMeshComponentDetailsCustomization::GetNaniteInputTrianglesText() const
{
	if (const RealtimeMesh::FRealtimeMeshNaniteEngineResources* R = ResolveNaniteResources(ComponentPtr.Get()))
	{
		return FText::AsNumber(R->NumInputTriangles);
	}
	return LOCTEXT("NoNanite", "—");
}

FText FRealtimeMeshComponentDetailsCustomization::GetNaniteInputVerticesText() const
{
	if (const RealtimeMesh::FRealtimeMeshNaniteEngineResources* R = ResolveNaniteResources(ComponentPtr.Get()))
	{
		return FText::AsNumber(R->NumInputVertices);
	}
	return LOCTEXT("NoNanite", "—");
}

FText FRealtimeMeshComponentDetailsCustomization::GetNaniteClustersText() const
{
	if (const RealtimeMesh::FRealtimeMeshNaniteEngineResources* R = ResolveNaniteResources(ComponentPtr.Get()))
	{
		return FText::AsNumber(R->NumClusters);
	}
	return LOCTEXT("NoNanite", "—");
}

FText FRealtimeMeshComponentDetailsCustomization::GetNaniteHierarchyNodesText() const
{
	if (const RealtimeMesh::FRealtimeMeshNaniteEngineResources* R = ResolveNaniteResources(ComponentPtr.Get()))
	{
		return FText::AsNumber(NaniteHierarchyNodesView(R).Num());
	}
	return LOCTEXT("NoNanite", "—");
}

FText FRealtimeMeshComponentDetailsCustomization::GetNaniteHierarchyDepthText() const
{
	const RealtimeMesh::FRealtimeMeshNaniteEngineResources* R = ResolveNaniteResources(ComponentPtr.Get());
	if (!R)
	{
		return LOCTEXT("NoNanite", "—");
	}
	// PageStreamingState.MaxHierarchyDepth is the deepest BVH path each page can
	// reach; the deepest across all pages is the mesh's overall hierarchy depth.
	uint8 MaxDepth = 0;
	for (const ::Nanite::FPageStreamingState& Pss : NanitePageStatesView(R))
	{
		MaxDepth = FMath::Max(MaxDepth, Pss.MaxHierarchyDepth);
	}
	return FText::AsNumber(static_cast<uint32>(MaxDepth));
}

FText FRealtimeMeshComponentDetailsCustomization::GetNanitePagesText() const
{
	const RealtimeMesh::FRealtimeMeshNaniteEngineResources* R = ResolveNaniteResources(ComponentPtr.Get());
	if (!R)
	{
		return LOCTEXT("NoNanite", "—");
	}
	const uint32 NumPages = static_cast<uint32>(NanitePageStatesView(R).Num());
	const uint32 NumRoot = R->NumRootPages;
	const uint32 NumStream = NumPages > NumRoot ? NumPages - NumRoot : 0u;
	return FText::FromString(FString::Printf(TEXT("%u total  (%u root + %u streaming)"),
		NumPages, NumRoot, NumStream));
}

FText FRealtimeMeshComponentDetailsCustomization::GetNaniteRootDataText() const
{
	if (const RealtimeMesh::FRealtimeMeshNaniteEngineResources* R = ResolveNaniteResources(ComponentPtr.Get()))
	{
		return FText::FromString(FormatBytes(NaniteRootDataView(R).Num()));
	}
	return LOCTEXT("NoNanite", "—");
}

FText FRealtimeMeshComponentDetailsCustomization::GetNaniteStreamingDataText() const
{
	if (const RealtimeMesh::FRealtimeMeshNaniteEngineResources* R = ResolveNaniteResources(ComponentPtr.Get()))
	{
		// Sum BulkSize across streaming pages (indices >= NumRootPages); polymorphic, doesn't depend on
		// the cooked FByteBulkData being present.
		const TConstArrayView<::Nanite::FPageStreamingState> Pages = NanitePageStatesView(R);
		int64 StreamingBytes = 0;
		for (int32 i = static_cast<int32>(R->NumRootPages); i < Pages.Num(); ++i)
		{
			StreamingBytes += Pages[i].BulkSize;
		}
		return FText::FromString(FormatBytes(StreamingBytes));
	}
	return LOCTEXT("NoNanite", "—");
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

// Shared by GetVertexCountText/GetTriangleCountText, which differ only in the stream
// key and the per-element divisor.
FText FRealtimeMeshComponentDetailsCustomization::SumLOD0StreamElements(const FRealtimeMeshStreamKey& StreamKey, int32 Divisor) const
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

	if (URealtimeMeshSimple* RealtimeMeshSimple = Cast<URealtimeMeshSimple>(Mesh))
	{
		int32 TotalElements = 0;
		TArray<FRealtimeMeshLODKey> LODs = RealtimeMeshSimple->GetLODs();

		// Only count LOD 0 for now
		if (LODs.Num() > 0)
		{
			TArray<FRealtimeMeshBufferSetKey> SectionGroups = RealtimeMeshSimple->GetBufferSets(LODs[0]);

			for (const FRealtimeMeshBufferSetKey& SectionGroupKey : SectionGroups)
			{
				RealtimeMeshSimple->ProcessMesh(SectionGroupKey, [&](const RealtimeMesh::FRealtimeMeshStreamSet& StreamSet)
				{
					if (const auto* Stream = StreamSet.Find(StreamKey))
					{
						TotalElements += Stream->Num() / Divisor;
					}
				});
			}
		}

		return FText::AsNumber(TotalElements);
	}

	return LOCTEXT("UnsupportedMeshType", "N/A");
}

FText FRealtimeMeshComponentDetailsCustomization::GetVertexCountText() const
{
	return SumLOD0StreamElements(RealtimeMesh::FRealtimeMeshStreams::Position, 1);
}

FText FRealtimeMeshComponentDetailsCustomization::GetTriangleCountText() const
{
	return SumLOD0StreamElements(RealtimeMesh::FRealtimeMeshStreams::Triangles, 3); // 3 indices per triangle
}

FReply FRealtimeMeshComponentDetailsCustomization::OnCreateStaticMesh()
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

	URealtimeMeshSimple* RealtimeMeshSimple = Cast<URealtimeMeshSimple>(RealtimeMesh);
	if (!RealtimeMeshSimple)
	{
		FNotificationInfo NotificationInfo(LOCTEXT("StaticMeshCreationUnsupported", "Static Mesh creation is currently only supported for RealtimeMeshSimple"));
		NotificationInfo.ExpireDuration = 5.0f;
		NotificationInfo.bUseLargeFont = false;
		FSlateNotificationManager::Get().AddNotification(NotificationInfo);
		return FReply::Handled();
	}

	FString PackagePath;
	FString AssetName;
	if (!ShowCreateStaticMeshDialog(PackagePath, AssetName))
	{
		return FReply::Handled(); // User cancelled
	}

	FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");
	IAssetTools& AssetTools = AssetToolsModule.Get();

	FString FinalPackageName = PackagePath + TEXT("/") + AssetName;
	FString FinalMeshName = AssetName;

	UStaticMesh* StaticMesh = Cast<UStaticMesh>(AssetTools.CreateAsset(FinalMeshName, FPaths::GetPath(FinalPackageName), UStaticMesh::StaticClass(), nullptr));
	
	if (!StaticMesh)
	{
		FNotificationInfo NotificationInfo(LOCTEXT("FailedToCreateAsset", "Failed to create Static Mesh asset"));
		NotificationInfo.ExpireDuration = 5.0f;
		NotificationInfo.bUseLargeFont = false;
		FSlateNotificationManager::Get().AddNotification(NotificationInfo);
		return FReply::Handled();
	}

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
		StaticMesh->MarkPackageDirty();

		FNotificationInfo NotificationInfo(FText::Format(
			LOCTEXT("StaticMeshCreated", "Static Mesh '{0}' created successfully!"),
			FText::FromString(FinalMeshName)
		));
		NotificationInfo.ExpireDuration = 5.0f;
		NotificationInfo.bUseLargeFont = false;
		FSlateNotificationManager::Get().AddNotification(NotificationInfo);

		FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
		TArray<FAssetData> AssetsToSync;
		AssetsToSync.Add(FAssetData(StaticMesh));
		ContentBrowserModule.Get().SyncBrowserToAssets(AssetsToSync);
	}
	else
	{
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

	return IsValid(RealtimeMesh);
}

bool FRealtimeMeshComponentDetailsCustomization::ShowCreateStaticMeshDialog(FString& OutPackagePath, FString& OutAssetName) const
{
	FString DefaultPath = TEXT("/Game/GeneratedMeshes");
	FString DefaultName = TEXT("RealtimeMeshComponent_StaticMesh");
	
	if (ComponentPtr.IsValid())
	{
		URealtimeMeshComponent* Component = ComponentPtr.Get();
		DefaultName = FString::Printf(TEXT("%s_StaticMesh"), *Component->GetName());
	}

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
			// No OnClicked here: the live handler is installed below via
			// CancelButton->SetOnClicked(), once DialogWindow exists to capture by reference.
			SAssignNew(CancelButton, SButton)
			.Text(LOCTEXT("Cancel", "Cancel"))
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

	// bUserConfirmed defaults to true: closing the dialog via the window's X (rather than
	// Cancel) is intentionally treated as OK/confirm. Do not change this to default-false.
	bool bUserConfirmed = true;

	DialogWindow->SetOnWindowClosed(FOnWindowClosed::CreateLambda([&bUserConfirmed](const TSharedRef<SWindow>& Window)
	{
		// Intentional no-op: closing via the window's X leaves bUserConfirmed at its
		// default of true (see note above).
	}));

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
}

#undef LOCTEXT_NAMESPACE