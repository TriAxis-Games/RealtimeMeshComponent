// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RealtimeMeshCore.h"
#include "Modules/ModuleManager.h"

class FRealtimeMeshEditorModule : public IModuleInterface
{
private:
	TSharedPtr<class FUICommandList> PluginCommands;

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
	void DismissLumenNotification(int32 DeferDays, bool bSuppressPermanently);
	void HandleLumenNotificationBuyNowClicked();
	void HandleLumenNotificationLaterClicked();
	void HandleLumenNotificationIgnoreClicked();
	
	void CheckLumenUseTimer();
};

