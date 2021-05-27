// Copyright 2016-2020 TriAxis Games L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"
#include "RuntimeMeshCore.h"
#include "HAL/Runnable.h"
#include "HAL/RunnableThread.h"
#include "HAL/ThreadSafeBool.h"
#include "Tickable.h"
#include "RuntimeMeshReference.h"
#include "RuntimeMeshComponentEngineSubsystem.generated.h"

class URuntimeMeshComponentSettings;
class URuntimeMesh;

/**
 * 
 */
UCLASS()
class RUNTIMEMESHCOMPONENT_API URuntimeMeshComponentEngineSubsystem : public UEngineSubsystem
{
	GENERATED_BODY()
private:

	struct FRuntimeMeshComponentDelayedActionTickObject : FTickableGameObject
	{
		URuntimeMeshComponentEngineSubsystem* ParentSubsystem;
	public:
		FRuntimeMeshComponentDelayedActionTickObject(URuntimeMeshComponentEngineSubsystem* InParent)
			: ParentSubsystem(InParent)
		{}

		virtual void Tick(float DeltaTime);
		virtual bool IsTickable() const;
		virtual bool IsTickableInEditor() const { return true; }
		virtual TStatId GetStatId() const { return TStatId(); }
	};


	TUniquePtr<FRuntimeMeshComponentDelayedActionTickObject> TickObject;
	TQueue<FRuntimeMeshWeakRef, EQueueMode::Mpsc> MeshesToUpdate;
	FQueuedThreadPool* ThreadPool;
public:
	void Initialize(FSubsystemCollectionBase& Collection) override;

	void Deinitialize() override;

	FQueuedThreadPool* GetThreadPool() { return ThreadPool; }
private:
	void QueueMeshForUpdate(const FRuntimeMeshWeakRef& InMesh);

	int32 CalculateNumThreads(const URuntimeMeshComponentSettings* Settings);
	void InitializeThreads(int32 NumThreads, int32 StackSize = 0, EThreadPriority ThreadPriority = TPri_BelowNormal);

	EThreadPriority ConvertThreadPriority(ERuntimeMeshThreadingPriority InPriority);

	bool IsTickable() const;
	void Tick(float DeltaTime);

	friend class URuntimeMesh;
};
