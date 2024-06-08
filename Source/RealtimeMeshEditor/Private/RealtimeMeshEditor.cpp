// Copyright (c) 2015-2024 TriAxis Games, L.L.C. All Rights Reserved.

#include "RealtimeMeshEditor.h"

#include "AssetToolsModule.h"
#include "ComponentAssetBroker.h"
#include "IAssetTools.h"
#include "RealtimeMeshComponent.h"

#define LOCTEXT_NAMESPACE "RealtimeMeshEditorModule"

void FRealtimeMeshEditorModule::StartupModule()
{

}

void FRealtimeMeshEditorModule::ShutdownModule()
{
    
}

#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(FRealtimeMeshEditorModule, RealtimeMeshEditor)