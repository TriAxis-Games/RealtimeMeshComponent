// Copyright (c) 2015-2024 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class FRealtimeMeshEditorModule : public IModuleInterface
{
private:
	TSharedPtr<class FUICommandList> PluginCommands;
	
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
};
