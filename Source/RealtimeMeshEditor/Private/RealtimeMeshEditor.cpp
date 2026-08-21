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
#include "Misc/ConfigCacheIni.h"

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
	// Detect the RealtimeMeshExt module to tell if this is the pro version.
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
	const int64 DayStartTimestamp = FDateTime::Today().ToUnixTimestamp();
	const bool bHasBeenAWhileSinceLastNotification = DayStartTimestamp > Settings.LastLumenNotificationTime;

	if (LumenNotification.Pin() || Settings.bShouldIgnoreLumenNotification || !bHasBeenAWhileSinceLastNotification || IsProVersion())
	{
		return;
	}

	if (!GRealtimeMeshNotifyLumenUseInCore)
	{
		return;
	}

	FNotificationInfo Notification(LOCTEXT("RealtimeMeshToast", "For Lumen support in the RealtimeMesh, please considering purchasing the Pro version!"));

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

// Shared by the notification handlers below: stamps the dismiss time, saves settings,
// and fades out the notification. Each handler adds its own distinct action on top
// (open URL / set the ignore flag) before calling this.
void FRealtimeMeshEditorModule::DismissLumenNotification()
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

void FRealtimeMeshEditorModule::HandleLumenNotificationBuyNowClicked()
{
	MarketplaceProButtonClicked();

	DismissLumenNotification();
}

void FRealtimeMeshEditorModule::HandleLumenNotificationLaterClicked()
{
	DismissLumenNotification();
}

void FRealtimeMeshEditorModule::HandleLumenNotificationIgnoreClicked()
{
	// Set the ignore flag before the shared save so it is persisted.
	Settings.bShouldIgnoreLumenNotification = true;

	DismissLumenNotification();
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

		Settings.LastLumenNotificationTime = ReadInt(NotificationSection, TEXT("LastLumenNotificationTime"));
	}
}

void FRealtimeMeshEditorModule::SaveSettings()
{
	const FString ConfigPath = FPaths::EngineUserDir() / TEXT("Saved") / TEXT("RealtimeMesh.ini");

	FConfigFile ConfigFile;

	{
		ConfigFile.AddToSection(TEXT("Notifications"), TEXT("bShouldIgnoreLumenNotification"), Settings.bShouldIgnoreLumenNotification ? TEXT("True") : TEXT("False"));

		ConfigFile.AddToSection(TEXT("Notifications"), TEXT("LastLumenNotificationTime"), FString::FromInt(Settings.LastLumenNotificationTime));
	}

	ConfigFile.Write(ConfigPath);
}


#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FRealtimeMeshEditorModule, RealtimeMeshEditor)
