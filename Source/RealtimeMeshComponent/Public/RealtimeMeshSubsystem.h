// Copyright (c) 2015-2024 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RealtimeMeshCore.h"
#include "Subsystems/WorldSubsystem.h"
#include "RealtimeMeshSubsystem.generated.h"

class UWorld;
class ARealtimeMeshActor;

/**
 * manages the scene view extension used for thread synchronization
 */
UCLASS()
class REALTIMEMESHCOMPONENT_API URealtimeMeshSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:	
	URealtimeMeshSubsystem();
	
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
	
	TSharedPtr<class FRealtimeMeshSceneViewExtension> SceneViewExtension;
	bool bInitialized;
};


namespace RealtimeMesh
{
	struct FRealtimeMeshEndOfFrameUpdateManager
	{
	private:
		FCriticalSection SyncRoot;
		TSet<FRealtimeMeshWeakPtr> MeshesToUpdate;
		FDelegateHandle EndOfFrameUpdateHandle;

		void OnPreSendAllEndOfFrameUpdates(UWorld* World);

	public:
		~FRealtimeMeshEndOfFrameUpdateManager();

		void MarkComponentForUpdate(const FRealtimeMeshWeakPtr& InMesh);
		void ClearComponentForUpdate(const FRealtimeMeshWeakPtr& InMesh);

		static FRealtimeMeshEndOfFrameUpdateManager& Get();
	};
}