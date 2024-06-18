// Copyright (c) 2015-2024 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FRealtimeMeshEditorSettings
{
	bool bShouldIgnoreLumenNotification = false;
	bool bShouldIgnoreGeneralNotification = false;
	int64 LastLumenNotificationTime = 0;
	int64 LastGeneralNotificationTime = 0;
};


class FRealtimeMeshEditorModule : public IModuleInterface
{
private:
	TSharedPtr<class FUICommandList> PluginCommands;

	FRealtimeMeshEditorSettings Settings;
	
	FDelegateHandle WorldPostInitializeDelegateHandle;
	TMap<UWorld*, FTimerHandle> WorldTimers;
	
	TWeakPtr<SNotificationItem> LumenNotification;	
	FTimerHandle LumenUseCheckHandle;
	bool bUserOwnsPro = false;
	
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
	
private:
    void RegisterMenus();

	static TSharedRef<SWidget> GenerateToolbarMenuContent(TSharedPtr<FUICommandList> Commands);
	
	void MarketplaceProButtonClicked();
	void MarketplaceCoreButtonClicked();
	void DiscordButtonClicked();
	void DocumentationButtonClicked();
	void IssuesButtonClicked();

	bool IsProVersion();
	bool UserOwnsPro();

	void ShowLumenNotification();
	void HandleLumenNotificationBuyNowClicked();
	void HandleLumenNotificationLaterClicked();
	void HandleLumenNotificationIgnoreClicked();

	void SetupWorldNotifications(UWorld* World, FWorldInitializationValues WorldInitializationValues);
	
	void CheckUserOwnsPro();
	void CheckLumenUseTimer(UWorld* World);

	void LoadSettings();
	void SaveSettings();
};

