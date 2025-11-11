// Copyright (c) 2015-2025 TriAxis Games, L.L.C. All Rights Reserved.


#include "RealtimeMeshMenuExtension.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/SlateStyleRegistry.h"
#include "Slate/SlateGameResources.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleMacros.h"
#include "Framework/Commands/Commands.h"

#define LOCTEXT_NAMESPACE "RealtimeMeshEditorModule"

#define RootToContentDir Style->RootToContentDir

TSharedPtr<FSlateStyleSet> FRealtimeMeshEditorStyle::StyleInstance = nullptr;

void FRealtimeMeshEditorStyle::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FRealtimeMeshEditorStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

FName FRealtimeMeshEditorStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("RealtimeMeshStyle"));
	return StyleSetName;
}


const FVector2D Icon16x16(16.0f, 16.0f);
const FVector2D Icon20x20(20.0f, 20.0f);

TSharedRef< FSlateStyleSet > FRealtimeMeshEditorStyle::Create()
{
	TSharedRef< FSlateStyleSet > Style = MakeShareable(new FSlateStyleSet("RealtimeMeshStyle"));
	Style->SetContentRoot(IPluginManager::Get().FindPlugin("RealtimeMeshComponent")->GetBaseDir() / TEXT("Resources"));

	Style->Set("RealtimeMesh.MenuAction", new IMAGE_BRUSH_SVG(TEXT("MenuIcon"), Icon20x20));
	return Style;
}

void FRealtimeMeshEditorStyle::ReloadTextures()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
	}
}

const ISlateStyle& FRealtimeMeshEditorStyle::Get()
{
	return *StyleInstance;
}

void FRealtimeMeshEditorCommands::RegisterCommands()
{	
	UI_COMMAND(MarketplaceCoreAction, "RMC-Core Marketplace", "Get the core version of the RMC on the Unreal Engine Marketplace!", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(MarketplaceProAction, "RMC-Pro Marketplace", "Get the pro version, with advanced features, of the RMC on the Unreal Engine Marketplace!", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(DiscordAction, "Join RMC Discord", "Join the RMC Discord community to find inspiration or help from like minded users!", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(DocumentationAction, "Open Documentation", "Open the RMC documentation in your web browser!", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(IssuesAction, "Open RMC Issue Tracker", "Open the RMC issue tracker on GitHub if you have found a bug and want to report it!", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
