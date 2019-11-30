// Copyright 2016-2019 Chris Conway (Koderz). All Rights Reserved.

#include "RealtimeMeshComponentPlugin.h"
#include "CustomVersion.h"
#include "RealtimeMeshCore.h"

// Register the custom version with core
FCustomVersionRegistration GRegisterRealtimeMeshCustomVersion(FRealtimeMeshVersion::GUID, FRealtimeMeshVersion::LatestVersion, TEXT("RealtimeMesh"));


class FRealtimeMeshComponentPlugin : public IRealtimeMeshComponentPlugin
{
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

IMPLEMENT_MODULE(FRealtimeMeshComponentPlugin, RealtimeMeshComponent)


void FRealtimeMeshComponentPlugin::StartupModule()
{

}

void FRealtimeMeshComponentPlugin::ShutdownModule()
{

}

DEFINE_LOG_CATEGORY(RealtimeMeshLog);