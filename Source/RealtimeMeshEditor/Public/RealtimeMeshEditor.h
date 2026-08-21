// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RealtimeMeshCore.h"
#include "Modules/ModuleManager.h"

struct FRealtimeMeshEditorSettings
{
	bool bShouldIgnoreLumenNotification = false;
	int64 LastLumenNotificationTime = 0;
};


class FRealtimeMeshEditorModule : public IModuleInterface
{
private:
	TSharedPtr<class FUICommandList> PluginCommands;

	FRealtimeMeshEditorSettings Settings;	
	
	TWeakPtr<SNotificationItem> LumenNotification;
	FTimerHandle LumenUseCheckHandle;
	
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

	void SetupEditorTimer();
	
	void ShowLumenNotification();
	// Shared stamp-time + save + expire/fadeout body for the notification handlers below.
	void DismissLumenNotification();
	void HandleLumenNotificationBuyNowClicked();
	void HandleLumenNotificationLaterClicked();
	void HandleLumenNotificationIgnoreClicked();
	
	void CheckLumenUseTimer();

	void LoadSettings();
	void SaveSettings();
};

