// Copyright 2016-2020 TriAxis Games L.L.C. All Rights Reserved.

#include "RuntimeMeshComponentEditorCommands.h"

#define LOCTEXT_NAMESPACE "FRuntimeMeshComponentEditorModule"

void FRuntimeMeshComponentEditorCommands::RegisterCommands()
{
#if ENGINE_MAJOR_VERSION >= 4 && ENGINE_MINOR_VERSION >= 26
	UI_COMMAND(DonateAction, "Donate to Support the RMC!", "Support the RMC by donating", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(HelpAction, "Help Documentation", "RMC Help Documentation", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(ForumsAction, "Forums", "RMC Forums", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(IssuesAction, "Issues", "RMC Issue Tracker", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(DiscordAction, "Discord Chat", "RMC Discord Chat", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(MarketplaceAction, "Marketplace Page", "RMC Marketplace Page", EUserInterfaceActionType::Button, FInputChord());
#else
	UI_COMMAND(DonateAction, "Donate to Support the RMC!", "Support the RMC by donating", EUserInterfaceActionType::Button, FInputGesture());
	UI_COMMAND(HelpAction, "Help Documentation", "RMC Help Documentation", EUserInterfaceActionType::Button, FInputGesture());
	UI_COMMAND(ForumsAction, "Forums", "RMC Forums", EUserInterfaceActionType::Button, FInputGesture());
	UI_COMMAND(IssuesAction, "Issues", "RMC Issue Tracker", EUserInterfaceActionType::Button, FInputGesture());
	UI_COMMAND(DiscordAction, "Discord Chat", "RMC Discord Chat", EUserInterfaceActionType::Button, FInputGesture());
	UI_COMMAND(MarketplaceAction, "Marketplace Page", "RMC Marketplace Page", EUserInterfaceActionType::Button, FInputGesture());
#endif
}

#undef LOCTEXT_NAMESPACE
