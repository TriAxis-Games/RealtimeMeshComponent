// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#include "RealtimeMeshSubsystem.h"
#include "RealtimeMeshActor.h"
#include "RealtimeMeshSceneViewExtension.h"
#include "SceneViewExtension.h"
#include "Engine/Engine.h"
#include "Engine/Level.h"
#include "Engine/World.h"
#include "Misc/LazySingleton.h"
#include "Misc/CoreDelegates.h"


URealtimeMeshSubsystem::URealtimeMeshSubsystem()
	: bInitialized(false)
{
}

bool URealtimeMeshSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	if (!Super::ShouldCreateSubsystem(Outer))
	{
		return false;
	}

	// Only create the subsystem (and its per-world scene view extension) for worlds that can
	// actually run mesh generation. Inactive/None (and other non-rendered) worlds never generate
	// meshes, so spinning up a subsystem + scene view extension for each of them is pure waste
	// (API-L10). The idiomatic hook would be DoesSupportWorldType, but that requires a new header
	// declaration; restricting here in the already-overridden ShouldCreateSubsystem achieves the
	// same result cpp-side.
	//
	// EditorPreview IS included intentionally: ARealtimeMeshActor-derived Blueprints must rebuild
	// their generated meshes (and compute providers must dispatch) in the Blueprint-editor preview
	// viewport. Excluding it regressed BP authoring, so it is explicitly allowed here while
	// Inactive/None remain excluded.
	if (const UWorld* World = Cast<UWorld>(Outer))
	{
		switch (World->WorldType)
		{
		case EWorldType::Game:
		case EWorldType::Editor:
		case EWorldType::PIE:
		case EWorldType::EditorPreview:
			return true;
		default:
			return false;
		}
	}

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
	// Only tick when at least one actor actually has a pending deferred rebuild. Registered actors
	// that can't rebuild (non-deferred, or already up to date) impose no per-frame cost.
	return PendingRebuildActors.Num() > 0;
}

bool URealtimeMeshSubsystem::IsTickableInEditor() const
{
	return true;
}

void URealtimeMeshSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (PendingRebuildActors.Num() == 0)
	{
		return;
	}

	// Snapshot only the (small) pending set before iterating: ExecuteRebuildGeneratedMeshIfPending()
	// fires user OnGenerateMesh callbacks which can spawn/destroy ARealtimeMeshActors and re-enter
	// MarkActorRebuildPending, mutating PendingRebuildActors mid-iteration. Iterating a local copy
	// keeps those mutations safe; newly-marked actors are picked up on the next tick.
	TArray<TWeakObjectPtr<ARealtimeMeshActor>> ActorsToRebuild = PendingRebuildActors.Array();

	for (const TWeakObjectPtr<ARealtimeMeshActor>& WeakActor : ActorsToRebuild)
	{
		ARealtimeMeshActor* Actor = WeakActor.Get();
		if (!Actor)
		{
			// Prune stale weak pointers.
			PendingRebuildActors.Remove(WeakActor);
			continue;
		}

		if (IsValid(Actor->GetLevel()))
		{
			Actor->ExecuteRebuildGeneratedMeshIfPending();
		}

		// Re-resolve after the (potentially re-entrant) rebuild callback, which may have destroyed
		// this actor. Drop it from the pending set once it no longer wants a rebuild (it rebuilt, is
		// no longer deferred, went away, or its component was cleared). Deferred-but-frozen or
		// not-yet-ready actors stay queued so they retry on a later tick, matching prior behavior.
		Actor = WeakActor.Get();
		if (!Actor || !Actor->WantsGeneratedMeshRebuild())
		{
			PendingRebuildActors.Remove(WeakActor);
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
		// If the actor already has a pending rebuild queued (e.g. its OnConstruction ran before it
		// managed to register), make sure it lands on the pending list now that it is registered.
		if (Actor && Actor->WantsGeneratedMeshRebuild())
		{
			PendingRebuildActors.Add(Actor);
		}
		return true;
	}
	return false;
}

void URealtimeMeshSubsystem::UnregisterGeneratedMeshActor(ARealtimeMeshActor* Actor)
{
	if (GetWorld() && bInitialized)
	{
		PendingRebuildActors.Remove(Actor);
	}
}

void URealtimeMeshSubsystem::MarkActorRebuildPending(ARealtimeMeshActor* Actor)
{
	if (Actor && GetWorld() && bInitialized)
	{
		PendingRebuildActors.Add(Actor);
	}
}

URealtimeMeshSubsystem* URealtimeMeshSubsystem::GetInstance(UWorld* World)
{
	return World ? World->GetSubsystem<URealtimeMeshSubsystem>() : nullptr;
}

void RealtimeMesh::FRealtimeMeshEndOfFrameUpdateManager::OnPreSendAllEndOfFrameUpdates(UWorld* World)
{
	// OnWorldPostActorTick fires once per ticking world. Only process meshes that belong to the
	// world currently ticking, otherwise a PIE mesh could be updated during the editor world's tick
	// (and vice versa). Meshes are bucketed by their (mark-time cached) owning world, so this only
	// swaps out the ticking world's bucket under the lock instead of rescanning and re-resolving the
	// world of every mesh in every world on every tick. The null/invalid-world bucket holds
	// world-agnostic meshes (standalone assets) and meshes whose world has been destroyed; those are
	// processed on whichever world ticks next.
	TArray<FRealtimeMeshWeakPtr> MeshesForThisWorld;
	{
		FScopeLock Lock(&SyncRoot);

		// Bucket meshes that were marked from worker threads (world resolution needs the game
		// thread — see MarkComponentForUpdate). This costs one outer-chain walk per fresh
		// worker-side mark, not one per registered mesh per ticking world.
		for (const FRealtimeMeshWeakPtr& MeshWeak : UnresolvedMeshes)
		{
			TWeakObjectPtr<UWorld> OwningWorld;
			if (FRealtimeMeshPtr Mesh = MeshWeak.Pin())
			{
				if (URealtimeMesh* OwningMesh = Mesh->GetContext()->GetOwningMesh())
				{
					OwningWorld = OwningMesh->GetWorld();
				}
				MeshesToUpdateByWorld.FindOrAdd(OwningWorld).Add(MeshWeak);
			}
		}
		UnresolvedMeshes.Reset();

		TSet<FRealtimeMeshWeakPtr> ThisWorldBucket;
		if (MeshesToUpdateByWorld.RemoveAndCopyValue(TWeakObjectPtr<UWorld>(World), ThisWorldBucket))
		{
			MeshesForThisWorld.Append(ThisWorldBucket.Array());
		}

		// Also drain world-agnostic meshes and any buckets whose owning world has been destroyed
		// (both present as an invalid world key), so they are never stranded.
		for (auto It = MeshesToUpdateByWorld.CreateIterator(); It; ++It)
		{
			if (!It->Key.IsValid())
			{
				MeshesForThisWorld.Append(It->Value.Array());
				It.RemoveCurrent();
			}
		}
	}

	for (const auto& MeshWeak : MeshesForThisWorld)
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
	// If the TLazySingleton has already been torn down (OnEnginePreExit fired during final
	// shutdown), a component GC-purged afterwards can still reach here via the shutdown-safe
	// fallback returned by Get(). No more ticks will run, so safely no-op instead of touching
	// engine delegates or the mesh set. TryGet() returns nullptr only post-teardown; during
	// normal runtime it returns the live singleton and behavior is unchanged.
	if (TLazySingleton<FRealtimeMeshEndOfFrameUpdateManager>::TryGet() == nullptr)
	{
		return;
	}

	// Resolve and cache the owning world now, so OnPreSendAllEndOfFrameUpdates doesn't have to walk
	// each mesh's outer chain under the lock. Resolving means pinning a weak UObject pointer and
	// walking its outer chain, which is only safe against GC on the game thread — marks arriving
	// from worker-thread commits park in UnresolvedMeshes and are resolved by the next game-thread
	// tick instead. A mesh with no owning world (standalone asset) buckets under a null world key
	// and is processed on whichever world ticks next.
	const bool bCanResolveWorld = IsInGameThread();
	TWeakObjectPtr<UWorld> OwningWorld;
	if (bCanResolveWorld)
	{
		if (FRealtimeMeshPtr Mesh = InMesh.Pin())
		{
			if (URealtimeMesh* OwningMesh = Mesh->GetContext()->GetOwningMesh())
			{
				OwningWorld = OwningMesh->GetWorld();
			}
		}
	}

	FScopeLock Lock(&SyncRoot);
	if (!EndOfFrameUpdateHandle.IsValid())
	{

		// Uses OnWorldPostActorTick rather than OnWorldPreSendAllEndOfFrameUpdates: on
		// dedicated servers the latter only fired roughly every ~60 seconds.
		EndOfFrameUpdateHandle = FWorldDelegates::OnWorldPostActorTick.AddLambda([this](UWorld* World, ELevelTick TickType, float DeltaSeconds) { OnPreSendAllEndOfFrameUpdates(World); });

		// The manager is a TLazySingleton that is never otherwise torn down, so its OnWorldPostActorTick
		// delegate would dangle at shutdown. Tear it down on engine exit (destructor removes the handle).
		FCoreDelegates::OnEnginePreExit.AddStatic([]() { TLazySingleton<FRealtimeMeshEndOfFrameUpdateManager>::TearDown(); });
	}

	if (bCanResolveWorld)
	{
		MeshesToUpdateByWorld.FindOrAdd(OwningWorld).Add(InMesh);
	}
	else
	{
		UnresolvedMeshes.Add(InMesh);
	}
}

void RealtimeMesh::FRealtimeMeshEndOfFrameUpdateManager::ClearComponentForUpdate(const RealtimeMesh::FRealtimeMeshWeakPtr& InMesh)
{
	// See MarkComponentForUpdate: no-op if the singleton was already torn down at shutdown so a
	// late GC-purge caller (reaching us via Get()'s fallback) never crashes.
	if (TLazySingleton<FRealtimeMeshEndOfFrameUpdateManager>::TryGet() == nullptr)
	{
		return;
	}

	// The mesh's owning world may have changed (or already been resolved differently) since it was
	// marked, so remove it from whichever bucket holds it and drop any bucket left empty.
	FScopeLock Lock(&SyncRoot);
	UnresolvedMeshes.Remove(InMesh);
	for (auto It = MeshesToUpdateByWorld.CreateIterator(); It; ++It)
	{
		if (It->Value.Remove(InMesh) > 0 && It->Value.Num() == 0)
		{
			It.RemoveCurrent();
		}
	}
}

RealtimeMesh::FRealtimeMeshEndOfFrameUpdateManager& RealtimeMesh::FRealtimeMeshEndOfFrameUpdateManager::Get()
{
	// Use TryGet() rather than TLazySingleton::Get(): after OnEnginePreExit tears the singleton
	// down, a late caller during final GC purge would otherwise hit the FATAL check(Ptr) inside
	// TLazySingleton::Get(). When the singleton is gone we hand back a shutdown-only fallback whose
	// Mark/Clear entries no-op (guarded by the same TryGet check), so late callers never crash.
	// During normal runtime TryGet() returns the live singleton and behavior is identical.
	if (FRealtimeMeshEndOfFrameUpdateManager* Existing = TLazySingleton<FRealtimeMeshEndOfFrameUpdateManager>::TryGet())
	{
		return *Existing;
	}

	static FRealtimeMeshEndOfFrameUpdateManager ShutdownFallback;
	return ShutdownFallback;
}









