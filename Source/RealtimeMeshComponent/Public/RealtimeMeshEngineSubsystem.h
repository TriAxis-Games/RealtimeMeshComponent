// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"
#include "RealtimeMeshEngineSubsystem.generated.h"


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
class REALTIMEMESHCOMPONENT_API URealtimeMeshEngineSubsystem : public UEngineSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

	virtual bool RegisterGeneratedMeshActor(ARealtimeMeshActor* Actor) { return false; }
	virtual void UnregisterGeneratedMeshActor(ARealtimeMeshActor* Actor) { }

	static URealtimeMeshEngineSubsystem* GetInstance();
private:
};









