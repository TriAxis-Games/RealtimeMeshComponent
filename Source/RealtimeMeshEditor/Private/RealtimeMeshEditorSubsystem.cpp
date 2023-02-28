// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#include "RealtimeMeshEditorSubsystem.h"
#include "RealtimeMeshActor.h"
#include "Editor.h"

bool URealtimeMeshEditorEngineSubsystem::bIsShuttingDown = false;


void URealtimeMeshEditorEngineSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	GenerationManager = NewObject<URealtimeMeshEditorGenerationManager>(this);

	GEditor->OnEditorClose().AddUObject(this, &URealtimeMeshEditorEngineSubsystem::OnShutdown);
	FCoreDelegates::OnEnginePreExit.AddUObject(this, &URealtimeMeshEditorEngineSubsystem::OnShutdown);
}

void URealtimeMeshEditorEngineSubsystem::Deinitialize()
{
	GEditor->OnEditorClose().RemoveAll(this);
	FCoreDelegates::OnEnginePreExit.RemoveAll(this);
	bIsShuttingDown = true;

	if (GenerationManager)
	{
		GenerationManager->Shutdown();
		GenerationManager = nullptr;
	}
}


void URealtimeMeshEditorEngineSubsystem::OnShutdown()
{
	bIsShuttingDown = true;
}



bool URealtimeMeshEditorEngineSubsystem::RegisterGeneratedMeshActor(ARealtimeMeshActor* Actor)
{
	if (bIsShuttingDown || GEditor == nullptr || GEngine == nullptr)		// subsystem no longer exists
	{
		return false;
	}

	GenerationManager->RegisterGeneratedMeshActor(Actor);
	return true;
}

void URealtimeMeshEditorEngineSubsystem::UnregisterGeneratedMeshActor(ARealtimeMeshActor* Actor)
{
	if (bIsShuttingDown || GEditor == nullptr || GEngine == nullptr)		// subsystem no longer exists
	{
		return;
	}
	
	GenerationManager->UnregisterGeneratedMeshActor(Actor);
}



void URealtimeMeshEditorGenerationManager::Tick(float DeltaTime)
{
	// Rebuild all valid generated actors, if necessary
	for (ARealtimeMeshActor* Actor : ActiveGeneratedActors)
	{
		if (IsValid(Actor) && Actor->IsValidLowLevel() && !Actor->IsUnreachable() && Actor->GetLevel() != nullptr)
		{
			Actor->ExecuteRebuildGeneratedMeshIfPending();
		}
	}
}

bool URealtimeMeshEditorGenerationManager::IsTickable() const
{
	return (ActiveGeneratedActors.Num() > 0);
}

TStatId URealtimeMeshEditorGenerationManager::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(URealtimeMeshEditorGenerationManager, STATGROUP_Tickables);
}


void URealtimeMeshEditorGenerationManager::Shutdown()
{
	ActiveGeneratedActors.Reset();
}

void URealtimeMeshEditorGenerationManager::RegisterGeneratedMeshActor(ARealtimeMeshActor* Actor)
{
	ActiveGeneratedActors.Add(Actor);
}

void URealtimeMeshEditorGenerationManager::UnregisterGeneratedMeshActor(ARealtimeMeshActor* Actor)
{
	ActiveGeneratedActors.Remove(Actor);
}




