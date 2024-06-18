// Copyright (c) 2015-2024 TriAxis Games, L.L.C. All Rights Reserved.

#include "RealtimeMeshEditor.h"

#include "AssetToolsModule.h"
#include "ComponentAssetBroker.h"
#include "IAssetTools.h"
#include "RealtimeMeshComponent.h"
#include "RealtimeMeshMenuExtension.h"

#define LOCTEXT_NAMESPACE "RealtimeMeshEditorModule"

void FRealtimeMeshEditorModule::StartupModule()
{
	FRealtimeMeshEditorStyle::Initialize();
	FRealtimeMeshEditorStyle::ReloadTextures();
	FRealtimeMeshEditorCommands::Register();
	
	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FRealtimeMeshEditorCommands::Get().MarketplaceProAction,
		FExecuteAction::CreateRaw(this, &FRealtimeMeshEditorModule::MarketplaceProButtonClicked),
		FCanExecuteAction());
	PluginCommands->MapAction(
		FRealtimeMeshEditorCommands::Get().MarketplaceCoreAction,
		FExecuteAction::CreateRaw(this, &FRealtimeMeshEditorModule::MarketplaceCoreButtonClicked),
		FCanExecuteAction());
	PluginCommands->MapAction(
		FRealtimeMeshEditorCommands::Get().DiscordAction,
		FExecuteAction::CreateRaw(this, &FRealtimeMeshEditorModule::DiscordButtonClicked),
		FCanExecuteAction());
	PluginCommands->MapAction(
		FRealtimeMeshEditorCommands::Get().DocumentationAction,
		FExecuteAction::CreateRaw(this, &FRealtimeMeshEditorModule::DocumentationButtonClicked),
		FCanExecuteAction());
	PluginCommands->MapAction(
		FRealtimeMeshEditorCommands::Get().IssuesAction,
		FExecuteAction::CreateRaw(this, &FRealtimeMeshEditorModule::IssuesButtonClicked),
		FCanExecuteAction());

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FRealtimeMeshEditorModule::RegisterMenus));
}

void FRealtimeMeshEditorModule::ShutdownModule()
{    
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);
	FRealtimeMeshEditorStyle::Shutdown();
	FRealtimeMeshEditorCommands::Unregister();
}


void FRealtimeMeshEditorModule::RegisterMenus()
{
	// Owner will be used for cleanup in call to UToolMenus::UnregisterOwner
	FToolMenuOwnerScoped OwnerScoped(this);

	{		
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("MainFrame.MainMenu");
		Menu = Menu->AddSubMenu("MainMenu", NAME_None, "RealtimeMesh", LOCTEXT("RealtimeMesh", "Realtime Mesh"), LOCTEXT("RealtimeMesh_Tooltip", "Open the Realtime Mesh menu"));

		FToolMenuSection& Section = Menu->FindOrAddSection("Tools");
		
		{
			Section.AddMenuEntryWithCommandList(FRealtimeMeshEditorCommands::Get().MarketplaceProAction, PluginCommands);
			Section.AddMenuEntryWithCommandList(FRealtimeMeshEditorCommands::Get().MarketplaceCoreAction, PluginCommands);
			Section.AddMenuEntryWithCommandList(FRealtimeMeshEditorCommands::Get().DiscordAction, PluginCommands);
			Section.AddMenuEntryWithCommandList(FRealtimeMeshEditorCommands::Get().DocumentationAction, PluginCommands);
			Section.AddMenuEntryWithCommandList(FRealtimeMeshEditorCommands::Get().IssuesAction, PluginCommands);
		}
	}

	{
		UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.PlayToolBar");
		{
			FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("RealtimeMeshTools");
			{
				FToolMenuEntry MenuEntry = FToolMenuEntry::InitComboButton(
					"RealtimeMeshTools",
					FUIAction(),
					FOnGetContent::CreateStatic(&FRealtimeMeshEditorModule::GenerateToolbarMenuContent, PluginCommands),
					LOCTEXT("RealtimeMeshTools_Label", "Realtime Mesh"),
					LOCTEXT("RealtimeMeshTools_Tooltip", "Open the Realtime Mesh menu"),
					FSlateIcon(FRealtimeMeshEditorStyle::Get().GetStyleSetName(), "RealtimeMesh.MenuAction")
				);
				Section.AddEntry(MenuEntry);
			}
		}
	}
}

TSharedRef<SWidget> FRealtimeMeshEditorModule::GenerateToolbarMenuContent(TSharedPtr<FUICommandList> Commands)
{
	const bool bShouldCloseWindowAfterMenuSelection = true;
	FMenuBuilder MenuBuilder(bShouldCloseWindowAfterMenuSelection, Commands);

	MenuBuilder.BeginSection("RealtimeMesh", LOCTEXT("RealtimeMesh", "Realtime Mesh"));
	{
		MenuBuilder.AddMenuEntry(FRealtimeMeshEditorCommands::Get().MarketplaceProAction);
		MenuBuilder.AddMenuEntry(FRealtimeMeshEditorCommands::Get().MarketplaceCoreAction);
		MenuBuilder.AddMenuEntry(FRealtimeMeshEditorCommands::Get().DiscordAction);
		MenuBuilder.AddMenuEntry(FRealtimeMeshEditorCommands::Get().DocumentationAction);
		MenuBuilder.AddMenuEntry(FRealtimeMeshEditorCommands::Get().IssuesAction);
	}
	MenuBuilder.EndSection();
	return MenuBuilder.MakeWidget();
}

void FRealtimeMeshEditorModule::MarketplaceProButtonClicked()
{
	FPlatformProcess::LaunchURL(TEXT("https://www.unrealengine.com/marketplace/en-US/product/realtime-mesh-component-pro"), nullptr, nullptr);
}

void FRealtimeMeshEditorModule::MarketplaceCoreButtonClicked()
{
	FPlatformProcess::LaunchURL(TEXT("https://www.unrealengine.com/marketplace/en-US/product/runtime-mesh-component"), nullptr, nullptr);
}

void FRealtimeMeshEditorModule::DiscordButtonClicked()
{
	FPlatformProcess::LaunchURL(TEXT("https://discord.gg/KGvBBTv"), nullptr, nullptr);
}

void FRealtimeMeshEditorModule::DocumentationButtonClicked()
{
	FPlatformProcess::LaunchURL(TEXT("https://rmc.triaxis.games/"), nullptr, nullptr);
}

void FRealtimeMeshEditorModule::IssuesButtonClicked()
{
	FPlatformProcess::LaunchURL(TEXT("https://github.com/TriAxis-Games/RealtimeMeshComponent/issues"), nullptr, nullptr);
}

#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(FRealtimeMeshEditorModule, RealtimeMeshEditor)