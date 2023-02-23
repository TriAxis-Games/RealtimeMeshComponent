// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#if WITH_EDITOR

#include "TickableEditorObject.h"
#include "RealtimeMeshEngineSubsystem.h"
#include "RealtimeMeshEditorSubsystem.generated.h"

class ARealtimeMeshActor;
class URealtimeMeshEditorGenerationManager;

UCLASS()
class REALTIMEMESHEDITOR_API URealtimeMeshEditorEngineSubsystem : public URealtimeMeshEngineSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override { return true; }
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;


	virtual bool RegisterGeneratedMeshActor(ARealtimeMeshActor* Actor) override;
	virtual void UnregisterGeneratedMeshActor(ARealtimeMeshActor* Actor) override;


protected:
	// Callback connected to engine/editor shutdown events to set bIsShuttingDown, which disables the subsystem static functions above
	virtual void OnShutdown();

private:
	UPROPERTY()
	TObjectPtr<URealtimeMeshEditorGenerationManager> GenerationManager;
	
	static bool bIsShuttingDown;
};



/**
 * URealtimeMeshEditorGenerationManager is a class used by URealtimeMeshEditorSubsystem to
 * store registrations and provide a Tick()
 */
UCLASS()
class REALTIMEMESHEDITOR_API URealtimeMeshEditorGenerationManager : public UObject, public FTickableEditorObject
{
	GENERATED_BODY()

public:
	virtual void Shutdown();

	virtual void RegisterGeneratedMeshActor(ARealtimeMeshActor* Actor);
	virtual void UnregisterGeneratedMeshActor(ARealtimeMeshActor* Actor);

public:

	//~ Begin FTickableEditorObject interface
	virtual void Tick(float DeltaTime) override;
	virtual ETickableTickType GetTickableTickType() const override { return ETickableTickType::Conditional; }
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override;
	//~ End FTickableEditorObject interface

	protected:
	TSet<ARealtimeMeshActor*> ActiveGeneratedActors;
};

#endif









