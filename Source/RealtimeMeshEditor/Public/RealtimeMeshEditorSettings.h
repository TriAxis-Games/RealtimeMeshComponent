// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "RealtimeMeshEditorSettings.generated.h"

/**
 * Editor-only settings for the Realtime Mesh Component, surfaced under
 * Project Settings > Plugins > Realtime Mesh. Stored per user, per project.
 *
 * These only affect the free Core plugin: when RMC-Pro is installed the upgrade
 * notification is never shown regardless of what is set here.
 */
UCLASS(config = EditorPerProjectUserSettings, meta = (DisplayName = "Realtime Mesh"))
class REALTIMEMESHEDITOR_API URealtimeMeshEditorSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/**
	 * Show an editor notification when a Realtime Mesh is placed in a scene using Lumen,
	 * pointing at RMC-Pro for runtime Lumen support. Ignored entirely when Pro is installed.
	 */
	UPROPERTY(config, EditAnywhere, Category = "Notifications",
		meta = (DisplayName = "Enable Pro upgrade notifications"))
	bool bEnableProUpgradeNotifications = true;

	/** Set when the notification is dismissed with "Don't show again", or after Pro is purchased. */
	UPROPERTY(config, EditAnywhere, Category = "Notifications",
		meta = (DisplayName = "Suppress Pro upgrade notification"))
	bool bSuppressProUpgradeNotification = false;

	/** Earliest time the notification may appear again, as a Unix timestamp. Managed automatically. */
	UPROPERTY(config)
	int64 NextProUpgradeNotificationTime = 0;

	/** How many times the notification has been shown. Managed automatically. */
	UPROPERTY(config)
	int32 ProUpgradeNotificationShowCount = 0;

	virtual FName GetCategoryName() const override { return TEXT("Plugins"); }
};
