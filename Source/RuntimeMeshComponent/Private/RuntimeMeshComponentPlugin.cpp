// Copyright 2016 Chris Conway (Koderz). All Rights Reserved.

#include "RuntimeMeshComponentPluginPrivatePCH.h"
#include "RuntimeMeshVersion.h"
#include "RuntimeMeshComponentPlugin.h"
#include "ModuleManager.h"


// Register the custom version with core
FCustomVersionRegistration GRegisterRuntimeMeshCustomVersion(FRuntimeMeshVersion::GUID, FRuntimeMeshVersion::LatestVersion, TEXT("RuntimeMesh"));




class FRuntimeMeshComponentPlugin : public IRuntimeMeshComponentPlugin
{
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

IMPLEMENT_MODULE(FRuntimeMeshComponentPlugin, RuntimeMeshComponent)



void FRuntimeMeshComponentPlugin::StartupModule()
{
	// TODO: Fix this hack in the engine. I shouldn't have to preload it for the engine.
	FModuleManager::Get().LoadModule("PhysXFormats");
}


void FRuntimeMeshComponentPlugin::ShutdownModule()
{

}



DEFINE_LOG_CATEGORY(RuntimeMeshLog);