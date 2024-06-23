// Copyright (c) 2015-2024 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RealtimeMeshCore.h"

#if RMC_ENGINE_ABOVE_5_4
struct FRealtimeMeshEditorSettings
{
	bool bShouldIgnoreLumenNotification = false;
	bool bShouldIgnoreGeneralNotification = false;
	int64 LastLumenNotificationTime = 0;
	int64 LastGeneralNotificationTime = 0;
};
#endif


class FRealtimeMeshEditorModule : public IModuleInterface
{
private:
	TSharedPtr<class FUICommandList> PluginCommands;

#if RMC_ENGINE_ABOVE_5_4
	FRealtimeMeshEditorSettings Settings;	
	
	TWeakPtr<SNotificationItem> LumenNotification;	
	FTimerHandle LumenUseCheckHandle;
	bool bUserOwnsPro = false;
#endif
	
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

#if RMC_ENGINE_ABOVE_5_4
	bool IsProVersion();
	bool UserOwnsPro();

	void SetupEditorTimer();
	
	void ShowLumenNotification();
	void HandleLumenNotificationBuyNowClicked();
	void HandleLumenNotificationLaterClicked();
	void HandleLumenNotificationIgnoreClicked();
	
	void CheckUserOwnsPro();
	void CheckLumenUseTimer();

	void LoadSettings();
	void SaveSettings();
#endif
};

