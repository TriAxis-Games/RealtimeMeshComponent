// Copyright (c) 2015-2024 TriAxis Games, L.L.C. All Rights Reserved.

#include "RealtimeMeshEditor.h"

#include "AssetToolsModule.h"
#include "ComponentAssetBroker.h"
#include "EngineUtils.h"
#include "IAssetTools.h"
#include "IPluginWardenModule.h"
#include "RealtimeMeshComponent.h"
#include "RealtimeMeshMenuExtension.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Interfaces/IPluginManager.h"
#include "Widgets/Notifications/SNotificationList.h"

#define LOCTEXT_NAMESPACE "RealtimeMeshEditorModule"

static bool GRealtimeMeshNotifyLumenUseInCore = true;
static FAutoConsoleVariableRef CVarRealtimeMeshNotifyLumenUseInCore(
	TEXT("RealtimeMesh.EnableNotificationsForLumenSupportInPro"),
	GRealtimeMeshNotifyLumenUseInCore,
	TEXT("Should we notify the user when they have an RMC in a scene with Lumen active but don't have RMC-Pro."),
	ECVF_Default);
	
void FRealtimeMeshEditorModule::StartupModule()
{
	LoadSettings();
	CheckUserOwnsPro();
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


	WorldPostInitializeDelegateHandle = FWorldDelegates::OnPostWorldInitialization.AddRaw(this, &FRealtimeMeshEditorModule::SetupWorldNotifications);
	for (TObjectIterator<UWorld> World; World; ++World)
	{
		if (IsValid(*World))
		{
			SetupWorldNotifications(*World, FWorldInitializationValues());
		}
	}
}

void FRealtimeMeshEditorModule::ShutdownModule()
{
	FWorldDelegates::OnPostWorldInitialization.Remove(WorldPostInitializeDelegateHandle);
	WorldPostInitializeDelegateHandle.Reset();

	for (auto CurrentTimer : WorldTimers)
	{
		if (IsValid(CurrentTimer.Key))
		{
			CurrentTimer.Key->GetTimerManager().ClearTimer(CurrentTimer.Value);
		}
	}
	WorldTimers.Empty();

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

bool FRealtimeMeshEditorModule::IsProVersion()
{
	// Detect the RealtimeMeshExt module to tell if this is the pro version.
	if (auto Plugin = IPluginManager::Get().FindPlugin(TEXT("RealtimeMeshComponent")))
	{
		return Plugin->GetDescriptor().Modules.ContainsByPredicate([](const FModuleDescriptor& Module)
		{
			return Module.Name == TEXT("RealtimeMeshExt");
		});
	}
	return false;
}

bool FRealtimeMeshEditorModule::UserOwnsPro()
{
	return bUserOwnsPro;
}

void FRealtimeMeshEditorModule::ShowLumenNotification()
{
	const int64 DayStartTimestamp = FDateTime::Today().ToUnixTimestamp();
	const bool bHasBeenAWhileSinceLastNotification = DayStartTimestamp > Settings.LastLumenNotificationTime;
	
	// Bail if we somehow already have this up, or if this is the pro version.
	if (LumenNotification.Pin() || Settings.bShouldIgnoreLumenNotification || !bHasBeenAWhileSinceLastNotification || IsProVersion())
	{
		return;
	}

	// Bail if this is disabled by CVar.
	if (!GRealtimeMeshNotifyLumenUseInCore)
	{
		return;
	}

	FNotificationInfo Notification(LOCTEXT("RealtimeMeshToast", "For Lumen support in the RealtimeMesh, please considering purchasing the Pro version!"));

	// Add the buttons with text, tooltip and callback
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

void FRealtimeMeshEditorModule::HandleLumenNotificationBuyNowClicked()
{
	MarketplaceProButtonClicked();
	
	Settings.LastLumenNotificationTime = FDateTime::Today().ToUnixTimestamp();
	SaveSettings();
	
	if (auto Notification = LumenNotification.Pin())
	{
		Notification->SetCompletionState(SNotificationItem::CS_Success);
		Notification->SetExpireDuration(0.0f);
		Notification->ExpireAndFadeout();
	}
}

void FRealtimeMeshEditorModule::HandleLumenNotificationLaterClicked()
{
	Settings.LastLumenNotificationTime = FDateTime::Today().ToUnixTimestamp();
	SaveSettings();
	
	if (auto Notification = LumenNotification.Pin())
	{
		Notification->SetCompletionState(SNotificationItem::CS_Success);
		Notification->SetExpireDuration(0.0f);
		Notification->ExpireAndFadeout();
	}
}

void FRealtimeMeshEditorModule::HandleLumenNotificationIgnoreClicked()
{
	Settings.LastLumenNotificationTime = FDateTime::Today().ToUnixTimestamp();
	Settings.bShouldIgnoreLumenNotification = true;
	SaveSettings();

	if (auto Notification = LumenNotification.Pin())
	{
		Notification->SetCompletionState(SNotificationItem::CS_Success);
		Notification->SetExpireDuration(0.0f);
		Notification->ExpireAndFadeout();
	}
}

void FRealtimeMeshEditorModule::SetupWorldNotifications(UWorld* World, FWorldInitializationValues WorldInitializationValues)
{
	if (WorldTimers.Contains(World))
	{
		if (IsValid(World))
		{
			World->GetTimerManager().ClearTimer(WorldTimers[World]);
		}
		WorldTimers.Remove(World);
	}

	if (World->IsEditorWorld())
	{
		FTimerHandle NewTimerHandle;
		World->GetTimerManager().SetTimer(NewTimerHandle, FTimerDelegate::CreateRaw(this, &FRealtimeMeshEditorModule::CheckLumenUseTimer, World), 30.0f, true, 300.0f);
		WorldTimers.Add(World, NewTimerHandle);
	}
}

void FRealtimeMeshEditorModule::CheckUserOwnsPro()
{
	if (IPluginWardenModule::IsAvailable())
	{
		IPluginWardenModule::Get().CheckEntitlementForPlugin(
			LOCTEXT("RealtimeMeshComponentPro", "Realtime Mesh Component Pro"),
			"b8fb43b8e89648fbb44797b3851317fb",
			"b8fb43b8e89648fbb44797b3851317fb",
			LOCTEXT("UnauthorizedPro", "You must own the Realtime Mesh Component Pro to use this feature!"),
			IPluginWardenModule::EUnauthorizedErrorHandling::Silent, [&]()
			{
				if (auto* Module = FModuleManager::GetModulePtr<FRealtimeMeshEditorModule>("RealtimeMeshEditor"))
				{
					Module->bUserOwnsPro = true;
				}
			});
	}
}

void FRealtimeMeshEditorModule::CheckLumenUseTimer(UWorld* World)
{
	// Does world have an RMC in it?  Does that world also have Lumen enabled?
	// If so, show the notification.

	if (IsValid(World))
	{
		bool bHasActiveRMC = false;
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			if (It->GetComponentByClass<URealtimeMeshComponent>() != nullptr)
			{
				bHasActiveRMC = true;
				break;
			}
		}

		const bool bLumenSupported = DoesPlatformSupportLumenGI(GetFeatureLevelShaderPlatform(World->Scene->GetFeatureLevel()));
		if (bHasActiveRMC && bLumenSupported)
		{
			ShowLumenNotification();
		}
	}
}

void FRealtimeMeshEditorModule::LoadSettings()
{
	const FString ConfigPath = FPaths::EngineUserDir() / TEXT("Saved") / TEXT("RealtimeMesh.ini");

	FConfigFile ConfigFile;
	ConfigFile.Read(ConfigPath);


	const auto ReadBool = [](const FConfigSection* Section, const TCHAR* Key)
	{
		if (const auto* T = Section->Find(Key))
		{
			return FCString::ToBool(*T->GetValue());
		}
		return false;
	};
	const auto ReadInt = [](const FConfigSection* Section, const TCHAR* Key)
	{
		if (const auto* T = Section->Find(Key))
		{
			if (FCString::IsNumeric(*T->GetValue()))
			{
				return FCString::Atoi64(*T->GetValue());
			}
		}
		return 0ll;
	};

	{
		const FConfigSection* NotificationSection = ConfigFile.FindOrAddConfigSection(TEXT("Notifications"));

		Settings.bShouldIgnoreLumenNotification = ReadBool(NotificationSection, TEXT("bShouldIgnoreLumenNotification"));
		Settings.bShouldIgnoreGeneralNotification = ReadBool(NotificationSection, TEXT("bShouldIgnoreGeneralNotification"));

		Settings.LastLumenNotificationTime = ReadInt(NotificationSection, TEXT("LastLumenNotificationTime"));
		Settings.LastGeneralNotificationTime = ReadInt(NotificationSection, TEXT("LastGeneralNotificationTime"));
	}
}

void FRealtimeMeshEditorModule::SaveSettings()
{
	const FString ConfigPath = FPaths::EngineUserDir() / TEXT("Saved") / TEXT("RealtimeMesh.ini");

	FConfigFile ConfigFile;

	{
		ConfigFile.AddToSection(TEXT("Notifications"), TEXT("bShouldIgnoreLumenNotification"), Settings.bShouldIgnoreLumenNotification ? TEXT("True") : TEXT("False"));
		ConfigFile.AddToSection(TEXT("Notifications"), TEXT("bShouldIgnoreGeneralNotification"), Settings.bShouldIgnoreGeneralNotification ? TEXT("True") : TEXT("False"));

		ConfigFile.AddToSection(TEXT("Notifications"), TEXT("LastLumenNotificationTime"), FString::FromInt(Settings.LastLumenNotificationTime));
		ConfigFile.AddToSection(TEXT("Notifications"), TEXT("LastGeneralNotificationTime"), FString::FromInt(Settings.LastGeneralNotificationTime));
	}

	ConfigFile.Write(ConfigPath);
}


#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FRealtimeMeshEditorModule, RealtimeMeshEditor)
