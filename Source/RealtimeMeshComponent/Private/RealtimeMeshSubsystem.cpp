// Copyright (c) 2015-2024 TriAxis Games, L.L.C. All Rights Reserved.

#include "RealtimeMeshSubsystem.h"
#include "RealtimeMeshActor.h"
#include "RealtimeMeshSceneViewExtension.h"
#include "SceneViewExtension.h"
#include "Engine/Engine.h"
#include "Engine/Level.h"


URealtimeMeshSubsystem::URealtimeMeshSubsystem()
	: bInitialized(false)
{
}

bool URealtimeMeshSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	return true;
}

void URealtimeMeshSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	bInitialized = true;	
	SceneViewExtension = FSceneViewExtensions::NewExtension<FRealtimeMeshSceneViewExtension>(GetWorld());
}

void URealtimeMeshSubsystem::Deinitialize()
{
	SceneViewExtension.Reset();
	bInitialized = false;
	Super::Deinitialize();
}









