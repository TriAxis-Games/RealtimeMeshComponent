// Copyright 2016-2020 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"
#include "RuntimeMeshCore.h"
#include "HAL/Runnable.h"
#include "HAL/RunnableThread.h"
#include "HAL/ThreadSafeBool.h"
#include "Tickable.h"
#include "RuntimeMeshComponentEngineSubsystem.generated.h"

class URuntimeMeshComponentSettings;

/**
 * 
 */
UCLASS()
class RUNTIMEMESHCOMPONENT_API URuntimeMeshComponentEngineSubsystem : public UEngineSubsystem
{
	GENERATED_BODY()
private:

	template<typename ItemType>
	struct TThreadSafeQueue
	{
	private:
		TQueue<ItemType> Queue;
		mutable FCriticalSection SyncObject;

	public:
		bool Dequeue(ItemType& OutItem)
		{
			FScopeLock Lock(&SyncObject);
			return Queue.Dequeue(OutItem);
		}
		void Empty()
		{
			FScopeLock Lock(&SyncObject);
			Queue.Empty();
		}
		bool Enqueue(const ItemType& Item)
		{
			FScopeLock Lock(&SyncObject);
			return Queue.Enqueue(Item);
		}
		bool Enqueue(ItemType&& Item)
		{
			FScopeLock Lock(&SyncObject);
			return Queue.Enqueue(MoveTemp(Item));
		}
		bool IsEmpty() const
		{
			FScopeLock Lock(&SyncObject);
			return Queue.IsEmpty();
		}
	};

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

	class FRuntimeMeshDataBackgroundWorker : public FRunnable
	{
		URuntimeMeshComponentEngineSubsystem* ParentSubsystem;
		FThreadSafeBool ShouldRun;
	public:
		FRuntimeMeshDataBackgroundWorker(URuntimeMeshComponentEngineSubsystem* InParent)
			: ParentSubsystem(InParent), ShouldRun(true)
		{}
		uint32 Run() override;
		void Stop() override;
	};


	TUniquePtr<FRuntimeMeshComponentDelayedActionTickObject> TickObject;

	ERuntimeMeshThreadingType CurrentThreadingType;
	float MaxAllowedTimePerTick;

	TThreadSafeQueue<TTuple<FRuntimeMeshDataWeakPtr, FRuntimeMeshProviderThreadExclusiveFunction>> ProxyParamsUpdateList;
	TThreadSafeQueue<FRuntimeMeshDataWeakPtr> GameThreadUpdateList;
	TThreadSafeQueue<FRuntimeMeshDataWeakPtr> AsyncUpdateList;

	TArray<TUniquePtr<FRuntimeMeshDataBackgroundWorker>> Workers;
	TArray<TUniquePtr<FRunnableThread>> WorkerThreads;
public:
	void Initialize(FSubsystemCollectionBase& Collection) override;

	void Deinitialize() override;


	void RegisterForProxyParamsUpdate(FRuntimeMeshDataWeakPtr InMesh, FRuntimeMeshProviderThreadExclusiveFunction Func);

	void RegisterForUpdate(FRuntimeMeshDataWeakPtr InMesh);

	FRuntimeMeshBackgroundWorkDelegate GetUserSuppliedThreadingWorkHandle();
private:
	void UpdateSettings(const FString& InPropertyName, const URuntimeMeshComponentSettings* InSettings);

	void RemoveThreadsToMaxCount(int32 NewNumThreads);
	void InitializeThreadsToCount(int32 NumThreads, int32 StackSize = 0, EThreadPriority ThreadPriority = TPri_BelowNormal);

	EThreadPriority ConvertThreadPriority(ERuntimeMeshThreadingPriority InPriority);

	bool IsTickable() const;
	void Tick(float DeltaTime);

	void UpdateGameThreadTasks();
	bool RunUpdateTasks(double MaxAllowedTime, bool bRunAsyncTaskList);


	friend class FRuntimeMeshData;
};
