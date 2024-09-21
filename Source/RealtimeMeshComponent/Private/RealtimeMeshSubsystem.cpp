// Copyright (c) 2015-2024 TriAxis Games, L.L.C. All Rights Reserved.

#include "RealtimeMeshSubsystem.h"
#include "RealtimeMeshActor.h"
#include "RealtimeMeshSceneViewExtension.h"
#include "SceneViewExtension.h"
#include "Engine/Engine.h"
#include "Engine/Level.h"
#include "Misc/LazySingleton.h"


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
















void RealtimeMesh::FRealtimeMeshEndOfFrameUpdateManager::OnPreSendAllEndOfFrameUpdates(UWorld* World)
{
	SyncRoot.Lock();
	auto MeshesCopy = MoveTemp(MeshesToUpdate);
	SyncRoot.Unlock();

	for (const auto& MeshWeak : MeshesCopy)
	{
		if (auto Mesh = MeshWeak.Pin())
		{
			Mesh->ProcessEndOfFrameUpdates();
		}
	}
}

RealtimeMesh::FRealtimeMeshEndOfFrameUpdateManager::~FRealtimeMeshEndOfFrameUpdateManager()
{
	if (EndOfFrameUpdateHandle.IsValid())
	{
		FWorldDelegates::OnWorldPostActorTick.Remove(EndOfFrameUpdateHandle);
		EndOfFrameUpdateHandle.Reset();
	}
}

void RealtimeMesh::FRealtimeMeshEndOfFrameUpdateManager::MarkComponentForUpdate(const RealtimeMesh::FRealtimeMeshWeakPtr& InMesh)
{
	FScopeLock Lock(&SyncRoot);
	if (!EndOfFrameUpdateHandle.IsValid())
	{
			
		// TODO: Moved this to post actor tick from OnWorldPreSendAlLEndOfFrameUpdates... Is this the best option?
		// Servers were not getting events but ever ~60 seconds
		EndOfFrameUpdateHandle = FWorldDelegates::OnWorldPostActorTick.AddLambda([this](UWorld* World, ELevelTick TickType, float DeltaSeconds) { OnPreSendAllEndOfFrameUpdates(World); });
	}
	MeshesToUpdate.Add(InMesh);
}

void RealtimeMesh::FRealtimeMeshEndOfFrameUpdateManager::ClearComponentForUpdate(const RealtimeMesh::FRealtimeMeshWeakPtr& InMesh)
{
	FScopeLock Lock(&SyncRoot);
	MeshesToUpdate.Remove(InMesh);
}

RealtimeMesh::FRealtimeMeshEndOfFrameUpdateManager& RealtimeMesh::FRealtimeMeshEndOfFrameUpdateManager::Get()
{
	return TLazySingleton<FRealtimeMeshEndOfFrameUpdateManager>::Get();
}









