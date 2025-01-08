// Copyright (c) 2015-2025 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class FRealtimeMeshEditorStyle
{
public:

	static void Initialize();

	static void Shutdown();

	/** reloads textures used by slate renderer */
	static void ReloadTextures();

	/** @return The Slate style set for the Shooter game */
	static const ISlateStyle& Get();

	static FName GetStyleSetName();

private:

	static TSharedRef< class FSlateStyleSet > Create();

private:

	static TSharedPtr< class FSlateStyleSet > StyleInstance;
};

class FRealtimeMeshEditorCommands : public TCommands<FRealtimeMeshEditorCommands>
{
public:

	FRealtimeMeshEditorCommands() : TCommands<FRealtimeMeshEditorCommands>(TEXT("RealtimeMeshEditor"), NSLOCTEXT("Contexts", "RealtimeMeshEditor", "RealtimeMeshEditor Plugin"), NAME_None, FRealtimeMeshEditorStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr<FUICommandInfo> MarketplaceCoreAction;
	TSharedPtr<FUICommandInfo> MarketplaceProAction;
	TSharedPtr<FUICommandInfo> DiscordAction;
	TSharedPtr<FUICommandInfo> DocumentationAction;
	TSharedPtr<FUICommandInfo> IssuesAction;
};


class REALTIMEMESHEDITOR_API RealtimeMeshMenuExtension
{
public:
	RealtimeMeshMenuExtension();
	~RealtimeMeshMenuExtension();
};
