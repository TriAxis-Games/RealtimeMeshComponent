// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#include "RealtimeMeshComponentModule.h"
#include "Serialization/CustomVersion.h"
#include "Interfaces/IPluginManager.h"
#include "ShaderCore.h"
#include "RealtimeMeshCore.h"


// Register the custom version with core
FCustomVersionRegistration GRegisterRealtimeMeshCustomVersion(RealtimeMesh::FRealtimeMeshVersion::GUID, RealtimeMesh::FRealtimeMeshVersion::LatestVersion, TEXT("RealtimeMesh"));


class FRealtimeMeshComponentPlugin : public IRealtimeMeshComponentPlugin
{
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

IMPLEMENT_MODULE(FRealtimeMeshComponentPlugin, RealtimeMeshComponent)


void FRealtimeMeshComponentPlugin::StartupModule()
{
	const FString PluginShaderDir = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("RealtimeMeshComponent"))->GetBaseDir(), TEXT("Shaders"));
	if (FPaths::DirectoryExists(PluginShaderDir))
	{
		AddShaderSourceDirectoryMapping(TEXT("/Plugin/RealtimeMeshComponent"), PluginShaderDir);
	}
}

void FRealtimeMeshComponentPlugin::ShutdownModule()
{
}

DEFINE_LOG_CATEGORY(RealtimeMeshLog);
