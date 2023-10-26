// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "RealtimeMeshEngineSubsystem.generated.h"


class UWorld;
class ARealtimeMeshActor;

/**
 * URealtimeMeshEditorSubsystem manages recomputation of "generated" mesh actors, eg
 * to provide procedural mesh generation in-Editor. Generally such procedural mesh generation
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

	static URealtimeMeshSubsystem* GetInstance(UWorld* World);

private:
	
	TSet<TWeakObjectPtr<ARealtimeMeshActor>> ActiveGeneratedActors;
	bool bInitialized;
};
