// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RealtimeMeshCore.h"
#include "Subsystems/WorldSubsystem.h"
#include "RealtimeMeshSubsystem.generated.h"

class UWorld;
class ARealtimeMeshActor;

/**
 * Manages recomputation of "generated" mesh actors, eg to provide procedural mesh
 * generation in-Editor. Generally such procedural mesh generation
 * is expensive, and if many objects need to be generated, the regeneration needs to be 
 * managed at a higher level to ensure that the Editor remains responsive/interactive.
 * 
 * ARealtimeMeshActors register themselves with this Subsystem, and
 * allow the Subsystem to tell them when they should regenerate themselves (if necessary).
 * The current behavior is to run all pending generations on a Tick, however in future
 * this regeneration will be more carefully managed via throttling / timeslicing / etc.
 * 
 */
UCLASS()
class REALTIMEMESHCOMPONENT_API URealtimeMeshSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	URealtimeMeshSubsystem();

	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	virtual bool IsTickable() const override;
	virtual bool IsTickableInEditor() const override;
	virtual void Tick(float DeltaTime) override;

	virtual TStatId GetStatId() const override;

	bool RegisterGeneratedMeshActor(ARealtimeMeshActor* Actor);
	void UnregisterGeneratedMeshActor(ARealtimeMeshActor* Actor);

	// Flags an already-registered actor as needing a deferred-generation rebuild. Only actors on the
	// pending list are visited by Tick, so the per-frame cost scales with actors that actually have
	// work rather than with the total registered actor count.
	void MarkActorRebuildPending(ARealtimeMeshActor* Actor);

	static URealtimeMeshSubsystem* GetInstance(UWorld* World);

private:

	TSet<TWeakObjectPtr<ARealtimeMeshActor>> PendingRebuildActors;
	TSharedPtr<class FRealtimeMeshSceneViewExtension> SceneViewExtension;
	bool bInitialized;
};


namespace RealtimeMesh
{
	struct FRealtimeMeshEndOfFrameUpdateManager
	{
	private:
		FCriticalSection SyncRoot;
		// Meshes bucketed by their owning world (cached at mark time). A null/invalid world key holds
		// world-agnostic meshes (standalone assets) and meshes whose world was destroyed; those are
		// processed on whichever world ticks next. Each world tick swaps out only its own bucket under
		// the lock and processes it outside the lock.
		TMap<TWeakObjectPtr<UWorld>, TSet<FRealtimeMeshWeakPtr>> MeshesToUpdateByWorld;
		// Marks that arrived off the game thread, where resolving the owning world (a weak UObject
		// pin + outer-chain walk) is not GC-safe. Resolved into buckets on the next game-thread tick.
		TSet<FRealtimeMeshWeakPtr> UnresolvedMeshes;
		FDelegateHandle EndOfFrameUpdateHandle;

		void OnPreSendAllEndOfFrameUpdates(UWorld* World);

	public:
		~FRealtimeMeshEndOfFrameUpdateManager();

		void MarkComponentForUpdate(const FRealtimeMeshWeakPtr& InMesh);
		void ClearComponentForUpdate(const FRealtimeMeshWeakPtr& InMesh);

		static FRealtimeMeshEndOfFrameUpdateManager& Get();
	};
}