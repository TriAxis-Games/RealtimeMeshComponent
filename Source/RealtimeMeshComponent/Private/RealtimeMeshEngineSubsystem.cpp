// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#include "RealtimeMeshEngineSubsystem.h"

#include "RealtimeMeshActor.h"
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
}

void URealtimeMeshSubsystem::Deinitialize()
{
	bInitialized = false;
	Super::Deinitialize();
}

bool URealtimeMeshSubsystem::IsTickable() const
{
	return ActiveGeneratedActors.Num() > 0;
}

bool URealtimeMeshSubsystem::IsTickableInEditor() const
{
	return true;
}

void URealtimeMeshSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Rebuild all valid generated actors, if necessary
	for (TWeakObjectPtr<ARealtimeMeshActor>& Actor : ActiveGeneratedActors)
	{
		if (Actor.IsValid() && IsValid(Actor->GetLevel()))
		{
			Actor->ExecuteRebuildGeneratedMeshIfPending();
		}
	}
}

TStatId URealtimeMeshSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(URealtimeMeshSubsystem, STATGROUP_Tickables);
}

bool URealtimeMeshSubsystem::RegisterGeneratedMeshActor(ARealtimeMeshActor* Actor)
{
	if (GetWorld() && bInitialized)
	{
		ActiveGeneratedActors.Add(Actor);
		return true;
	}
	return false;
}

void URealtimeMeshSubsystem::UnregisterGeneratedMeshActor(ARealtimeMeshActor* Actor)
{
	if (GetWorld() && bInitialized)
	{
		ActiveGeneratedActors.Remove(Actor);
	}
}

URealtimeMeshSubsystem* URealtimeMeshSubsystem::GetInstance(UWorld* World)
{
	return World ? World->GetSubsystem<URealtimeMeshSubsystem>() : nullptr;
}

