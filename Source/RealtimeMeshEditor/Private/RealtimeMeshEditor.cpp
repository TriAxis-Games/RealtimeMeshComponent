// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#include "RealtimeMeshEditor.h"

#include "AssetToolsModule.h"
#include "ComponentAssetBroker.h"
#include "EngineUtils.h"
#include "IAssetTools.h"
#include "Editor.h"
#include "ToolMenus.h"
#include "RealtimeMeshComponent.h"
#include "RealtimeMeshMenuExtension.h"
#include "RealtimeMeshComponentDetailsCustomization.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Interfaces/IPluginManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "PropertyEditorModule.h"
#include "SceneInterface.h"
#include "RealtimeMeshEditorSettings.h"

#define LOCTEXT_NAMESPACE "RealtimeMeshEditorModule"

// How long "Later" defers the upgrade notification, and the hard ceiling on how many
// times it may ever be shown. Both are deliberately conservative: this is an upsell,
// not a warning, and it must never become background noise.
static constexpr int32 GRealtimeMeshProNotificationDeferDays = 30;
static constexpr int32 GRealtimeMeshProNotificationMaxShows = 3;

static bool GRealtimeMeshNotifyLumenUseInCore = true;
static FAutoConsoleVariableRef CVarRealtimeMeshEnableProUpgradeNotifications(
	TEXT("RealtimeMesh.EnableProUpgradeNotifications"),
	GRealtimeMeshNotifyLumenUseInCore,
	TEXT("Show the editor notification pointing Core users at RMC-Pro for runtime Lumen support. No effect when Pro is installed."),
	ECVF_Default);
// Legacy spelling, kept so existing ini/console settings keep working. The old name was
// misleading: it gates the notification shown in Core, not anything in Pro.
static FAutoConsoleVariableRef CVarRealtimeMeshNotifyLumenUseInCore_Legacy(
	TEXT("RealtimeMesh.EnableNotificationsForLumenSupportInPro"),
	GRealtimeMeshNotifyLumenUseInCore,
	TEXT("Deprecated alias for RealtimeMesh.EnableProUpgradeNotifications."),
	ECVF_Default);
	
void FRealtimeMeshEditorModule::StartupModule()
{
	FRealtimeMeshEditorStyle::Initialize();
	FRealtimeMeshEditorStyle::ReloadTextures();
	FRealtimeMeshEditorCommands::Register();

	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.RegisterCustomClassLayout(
		URealtimeMeshComponent::StaticClass()->GetFName(),
		FOnGetDetailCustomizationInstance::CreateStatic(&FRealtimeMeshComponentDetailsCustomization::MakeInstance)
	);

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


	FEditorDelegates::OnMapOpened.AddLambda([this](const FString&, bool)
	{
		SetupEditorTimer();
	});
	FEditorDelegates::OnMapLoad.AddLambda([this](const FString&, FCanLoadMap&)
	{		
		SetupEditorTimer();
	});
}

void FRealtimeMeshEditorModule::ShutdownModule()
{	
	if (GEditor && LumenUseCheckHandle.IsValid())
	{
		if (GEditor->IsTimerManagerValid())
		{
			GEditor->GetTimerManager().Get().ClearTimer(LumenUseCheckHandle);
		}
	}

	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.UnregisterCustomClassLayout(URealtimeMeshComponent::StaticClass()->GetFName());
	}

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
			// Store links are for people who don't already own Pro; showing "Get RMC-Pro on Fab"
			// to a Pro user is just noise in their menu.
			if (!IsProVersion())
			{
				Section.AddMenuEntryWithCommandList(FRealtimeMeshEditorCommands::Get().MarketplaceProAction, PluginCommands);
				Section.AddMenuEntryWithCommandList(FRealtimeMeshEditorCommands::Get().MarketplaceCoreAction, PluginCommands);
			}
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
	FPlatformProcess::LaunchURL(TEXT("https://discord.gg/WCgffd3h6r"), nullptr, nullptr);
}

void FRealtimeMeshEditorModule::DocumentationButtonClicked()
{
	FPlatformProcess::LaunchURL(TEXT("https://triaxis.games/realtime-mesh/"), nullptr, nullptr);
}

void FRealtimeMeshEditorModule::IssuesButtonClicked()
{
	FPlatformProcess::LaunchURL(TEXT("https://github.com/TriAxis-Games/RealtimeMeshComponent/issues"), nullptr, nullptr);
}

bool FRealtimeMeshEditorModule::IsProVersion()
{
	// Primary check: is the Pro-only Ext module actually present in this build? Both tiers ship a
	// file named RealtimeMeshComponent.uplugin, so a name-based FindPlugin can resolve to a stale
	// Core copy (e.g. left in Engine/Plugins/Marketplace after upgrading) and wrongly report Core.
	if (FModuleManager::Get().IsModuleLoaded(TEXT("RealtimeMeshExt")) ||
		FModuleManager::Get().ModuleExists(TEXT("RealtimeMeshExt")))
	{
		return true;
	}

	// Fallback: inspect the descriptor.
	if (auto Plugin = IPluginManager::Get().FindPlugin(TEXT("RealtimeMeshComponent")))
	{
		return Plugin->GetDescriptor().FriendlyName.Contains(TEXT("Pro")) ||
			Plugin->GetDescriptor().Modules.ContainsByPredicate([](const FModuleDescriptor& Module)
			{
				return Module.Name == TEXT("RealtimeMeshExt");
			});
	}
	return false;
}

void FRealtimeMeshEditorModule::SetupEditorTimer()
{
	if (!IsProVersion())
	{
		if (GEditor && !LumenUseCheckHandle.IsValid())
		{
			// In editor use the editor manager
			if (GEditor->IsTimerManagerValid())
			{
				GEditor->GetTimerManager().Get().SetTimer(LumenUseCheckHandle,
					FTimerDelegate::CreateRaw(this, &FRealtimeMeshEditorModule::CheckLumenUseTimer), 30.0f, true, 300.0f);
			}
		}
	}
}

void FRealtimeMeshEditorModule::ShowLumenNotification()
{
	if (LumenNotification.Pin() || IsProVersion() || !GRealtimeMeshNotifyLumenUseInCore)
	{
		return;
	}

	const URealtimeMeshEditorSettings* EditorSettings = GetDefault<URealtimeMeshEditorSettings>();
	if (!EditorSettings->bEnableProUpgradeNotifications || EditorSettings->bSuppressProUpgradeNotification)
	{
		return;
	}

	// Hard ceiling, independent of the defer window: after this many showings we stop for good.
	if (EditorSettings->ProUpgradeNotificationShowCount >= GRealtimeMeshProNotificationMaxShows)
	{
		return;
	}

	if (FDateTime::UtcNow().ToUnixTimestamp() < EditorSettings->NextProUpgradeNotificationTime)
	{
		return;
	}

	{
		URealtimeMeshEditorSettings* MutableSettings = GetMutableDefault<URealtimeMeshEditorSettings>();
		MutableSettings->ProUpgradeNotificationShowCount++;
		MutableSettings->SaveConfig();
	}

	FNotificationInfo Notification(LOCTEXT("RealtimeMeshToast", "Realtime Mesh Pro adds runtime Lumen and distance field support for dynamic meshes."));

	Notification.ButtonDetails.Add(FNotificationButtonInfo(
		LOCTEXT("BuyPro", "Buy Pro!"),
		LOCTEXT("BuyProTooltip", "Open the Unreal Engine Marketplace to purchase the Pro version of the RealtimeMesh Component"),
		FSimpleDelegate::CreateRaw(this, &FRealtimeMeshEditorModule::HandleLumenNotificationBuyNowClicked)));
	Notification.ButtonDetails.Add(FNotificationButtonInfo(
		LOCTEXT("RemindMeLater", "Remind Me Later"),
		LOCTEXT("RemindMeLaterTooltip", "Remind me later"),
		FSimpleDelegate::CreateRaw(this, &FRealtimeMeshEditorModule::HandleLumenNotificationLaterClicked)));
	Notification.ButtonDetails.Add(FNotificationButtonInfo(
		LOCTEXT("DontRemindMe", "Ignore"),
		LOCTEXT("DontRemindMeTooltip", "Ignore this warning"),
		FSimpleDelegate::CreateRaw(this, &FRealtimeMeshEditorModule::HandleLumenNotificationIgnoreClicked)));

	// We will be keeping track of this ourselves
	Notification.bFireAndForget = false;
	Notification.ExpireDuration = 0.0f;

	// Set the width so that the notification doesn't resize as its text changes
	Notification.WidthOverride = 450.0f;

	Notification.bUseLargeFont = false;
	Notification.bUseThrobber = false;
	Notification.bUseSuccessFailIcons = false;

	LumenNotification = FSlateNotificationManager::Get().AddNotification(Notification);

	if (LumenNotification.IsValid())
	{
		LumenNotification.Pin()->SetCompletionState(SNotificationItem::CS_Pending);
	}
}

// Shared by the notification handlers below: records when (or whether) the notification may
// appear again, persists that, and fades the toast out.
void FRealtimeMeshEditorModule::DismissLumenNotification(int32 DeferDays, bool bSuppressPermanently)
{
	URealtimeMeshEditorSettings* EditorSettings = GetMutableDefault<URealtimeMeshEditorSettings>();
	EditorSettings->NextProUpgradeNotificationTime =
		(FDateTime::UtcNow() + FTimespan::FromDays(DeferDays)).ToUnixTimestamp();
	if (bSuppressPermanently)
	{
		EditorSettings->bSuppressProUpgradeNotification = true;
	}
	EditorSettings->SaveConfig();

	if (auto Notification = LumenNotification.Pin())
	{
		Notification->SetCompletionState(SNotificationItem::CS_Success);
		Notification->SetExpireDuration(0.0f);
		Notification->ExpireAndFadeout();
	}
}

void FRealtimeMeshEditorModule::HandleLumenNotificationBuyNowClicked()
{
	MarketplaceProButtonClicked();

	// They followed the link; do not ask again regardless of whether they complete the purchase.
	DismissLumenNotification(GRealtimeMeshProNotificationDeferDays, /*bSuppressPermanently*/ true);
}

void FRealtimeMeshEditorModule::HandleLumenNotificationLaterClicked()
{
	DismissLumenNotification(GRealtimeMeshProNotificationDeferDays, /*bSuppressPermanently*/ false);
}

void FRealtimeMeshEditorModule::HandleLumenNotificationIgnoreClicked()
{
	DismissLumenNotification(GRealtimeMeshProNotificationDeferDays, /*bSuppressPermanently*/ true);
}

void FRealtimeMeshEditorModule::CheckLumenUseTimer()
{
	// Shows the notification if any editor world has both Lumen enabled and an active RMC.
	bool bHasActiveRMC = false;
	FGCScopeGuard GCGuard;
	for (TObjectIterator<AActor> It; It; ++It)
	{
		if (IsValid(*It) && !It->IsPendingKillPending() && IsValid(It->GetWorld()))
		{
			if (It->GetWorld()->IsEditorWorld() && !It->GetWorld()->IsPreviewWorld())
			{
				if (DoesPlatformSupportLumenGI(GetFeatureLevelShaderPlatform(It->GetWorld()->Scene->GetFeatureLevel())))
				{
					if (IsValid(It->GetComponentByClass<URealtimeMeshComponent>()))
					{
						bHasActiveRMC = true;
						break;
					}
				}
			}
		}
	}
	
	if (bHasActiveRMC)
	{
		ShowLumenNotification();
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FRealtimeMeshEditorModule, RealtimeMeshEditor)
