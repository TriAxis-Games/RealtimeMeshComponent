// Copyright 2016-2020 Chris Conway (Koderz). All Rights Reserved.


#include "RuntimeMeshComponentEngineSubsystem.h"
#include "RuntimeMeshComponentSettings.h"
#include "RuntimeMeshCore.h"

void URuntimeMeshComponentEngineSubsystem::FRuntimeMeshComponentDelayedActionTickObject::Tick(float DeltaTime)
{
	ParentSubsystem->Tick(DeltaTime);
}
bool URuntimeMeshComponentEngineSubsystem::FRuntimeMeshComponentDelayedActionTickObject::IsTickable() const
{
	return ParentSubsystem->IsTickable();
}

uint32 URuntimeMeshComponentEngineSubsystem::FRuntimeMeshDataBackgroundWorker::Run()
{
	while (ShouldRun)
	{
		bool bHasMoreWork = ParentSubsystem->RunUpdateTasks(0, true);

		if (!bHasMoreWork)
		{
			FPlatformProcess::Sleep(1.0 / 60);
		}
	}
	return 0;
}
void URuntimeMeshComponentEngineSubsystem::FRuntimeMeshDataBackgroundWorker::Stop()
{
	ShouldRun = false;
}



void URuntimeMeshComponentEngineSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	CurrentThreadingType = ERuntimeMeshThreadingType::Synchronous;
	MaxAllowedTimePerTick = 1000 / 60;

	// Create the tick object
	TickObject = MakeUnique<FRuntimeMeshComponentDelayedActionTickObject>(this);

	const URuntimeMeshComponentSettings* Settings = GetDefault<URuntimeMeshComponentSettings>();
	check(Settings);

	UpdateSettings(TEXT(""), Settings);


#if WITH_EDITOR
	Settings->OnSettingsChanged().AddUObject(this, &URuntimeMeshComponentEngineSubsystem::UpdateSettings);
#endif
}

void URuntimeMeshComponentEngineSubsystem::Deinitialize()
{
	Super::Deinitialize();

	RemoveThreadsToMaxCount(0);
}

void URuntimeMeshComponentEngineSubsystem::RegisterForProxyParamsUpdate(FRuntimeMeshDataWeakPtr InMesh, FRuntimeMeshProviderThreadExclusiveFunction Func)
{
	FRuntimeMeshDataPtr Mesh = InMesh.Pin();

	if (Mesh)
	{
		bool bIsRegistered = Mesh->AsyncWorkState.SetHasGameThreadWork();

		// Are we already queued?
		if (!bIsRegistered)
		{
			ProxyParamsUpdateList.Enqueue(MakeTuple(InMesh, Func));
		}
	}
}

void URuntimeMeshComponentEngineSubsystem::RegisterForUpdate(FRuntimeMeshDataWeakPtr InMesh)
{
	FRuntimeMeshDataPtr Mesh = InMesh.Pin();

	if (Mesh)
	{
		bool bIsRegistered = Mesh->AsyncWorkState.SetHasAsyncWork();

		// Are we already queued?
		if (!bIsRegistered)
		{
			if (Mesh->IsThreadSafe())
			{
				AsyncUpdateList.Enqueue(Mesh);
			}
			else
			{
				GameThreadUpdateList.Enqueue(Mesh);
			}
		}
	}
}

FRuntimeMeshBackgroundWorkDelegate URuntimeMeshComponentEngineSubsystem::GetUserSuppliedThreadingWorkHandle()
{
	CurrentThreadingType = ERuntimeMeshThreadingType::UserSupplied;

	RemoveThreadsToMaxCount(0);

	TWeakObjectPtr<URuntimeMeshComponentEngineSubsystem> SubsystemWeakPtr = this;

	return FRuntimeMeshBackgroundWorkDelegate::CreateLambda([SubsystemWeakPtr](double MaxAllowedTime)
		{
			if (SubsystemWeakPtr.IsValid())
			{
				SubsystemWeakPtr.Get()->RunUpdateTasks(MaxAllowedTime, true);
			}
		});
}

void URuntimeMeshComponentEngineSubsystem::UpdateSettings(const FString& InPropertyName, const URuntimeMeshComponentSettings* InSettings)
{
	check(InSettings);
	CurrentThreadingType = InSettings->DefaultThreadingModel;
	MaxAllowedTimePerTick = InSettings->MaxAllowedTimePerTick;

	if (CurrentThreadingType == ERuntimeMeshThreadingType::ThreadPool)
	{
		int32 MinThreads = InSettings->MinMaxThreadPoolThreads.GetLowerBoundValue();
		int32 MaxThreads = InSettings->MinMaxThreadPoolThreads.GetUpperBoundValue();
		int32 ThreadDivisor = InSettings->SystemThreadDivisor;

		MaxThreads = FMath::Clamp(MaxThreads, 1, 64);
		MinThreads = FMath::Clamp(MinThreads, 1, MaxThreads);
		ThreadDivisor = FMath::Clamp(ThreadDivisor, 1, MaxThreads);

		int32 CoresWithHyperthreads = FGenericPlatformMisc::NumberOfCoresIncludingHyperthreads();

		int32 NumThreads = FMath::Clamp(
			FMath::RoundToInt(CoresWithHyperthreads / (float)ThreadDivisor),
			MinThreads, MaxThreads);

		InitializeThreadsToCount(NumThreads, InSettings->ThreadStackSize, ConvertThreadPriority(InSettings->ThreadPriority));
		RemoveThreadsToMaxCount(NumThreads);
	}
	else
	{
		RemoveThreadsToMaxCount(0);
	}
}

void URuntimeMeshComponentEngineSubsystem::RemoveThreadsToMaxCount(int32 NewNumThreads)
{
	// Remove excessive threads
	for (int32 Index = Workers.Num() - 1; Index > (NewNumThreads - 1); Index--)
	{
		Workers[Index]->Stop();
	}

	for (int32 Index = Workers.Num() - 1; Index > (NewNumThreads - 1); Index--)
	{
		WorkerThreads[Index]->WaitForCompletion();
		WorkerThreads[Index]->Kill();

		WorkerThreads.RemoveAt(Index);
		Workers.RemoveAt(Index);
	}
}

void URuntimeMeshComponentEngineSubsystem::InitializeThreadsToCount(int32 NumThreads, int32 StackSize /*= 0*/, EThreadPriority ThreadPriority /*= TPri_BelowNormal*/)
{
	// Add new threads			
	for (int32 Index = Workers.Num(); Index < NumThreads; Index++)
	{
		Workers.Add(MakeUnique<FRuntimeMeshDataBackgroundWorker>(this));

		WorkerThreads.Add(TUniquePtr<FRunnableThread>(FRunnableThread::Create(Workers[Index].Get(),
			*FString::Printf(TEXT("RuntimeMeshBackgroundThread: %d"), Index), StackSize, ThreadPriority)));
	}
}

EThreadPriority URuntimeMeshComponentEngineSubsystem::ConvertThreadPriority(ERuntimeMeshThreadingPriority InPriority)
{
	switch (InPriority)
	{
	case ERuntimeMeshThreadingPriority::Normal:
		return TPri_Normal;
	case ERuntimeMeshThreadingPriority::AboveNormal:
		return TPri_AboveNormal;
	case ERuntimeMeshThreadingPriority::BelowNormal:
		return TPri_BelowNormal;
	case ERuntimeMeshThreadingPriority::Highest:
		return TPri_Highest;
	case ERuntimeMeshThreadingPriority::Lowest:
		return TPri_Lowest;
	case ERuntimeMeshThreadingPriority::SlightlyBelowNormal:
		return TPri_SlightlyBelowNormal;
	case ERuntimeMeshThreadingPriority::TimeCritical:
		return TPri_TimeCritical;
	default:
		return TPri_BelowNormal;
	}
}

bool URuntimeMeshComponentEngineSubsystem::IsTickable() const
{
	return (!ProxyParamsUpdateList.IsEmpty()) ||
		(!GameThreadUpdateList.IsEmpty()) ||
		(CurrentThreadingType == ERuntimeMeshThreadingType::Synchronous && !AsyncUpdateList.IsEmpty());
}

void URuntimeMeshComponentEngineSubsystem::Tick(float DeltaTime)
{
	//SCOPE_CYCLE_COUNTER(STAT_RuntimeMeshDelayedActions_Tick);

	UpdateGameThreadTasks();
	RunUpdateTasks(MaxAllowedTimePerTick, false);

	if (CurrentThreadingType == ERuntimeMeshThreadingType::Synchronous)
	{
		RunUpdateTasks(MaxAllowedTimePerTick, true);
	}
}

void URuntimeMeshComponentEngineSubsystem::UpdateGameThreadTasks()
{
	TQueue<TTuple<FRuntimeMeshDataWeakPtr, FRuntimeMeshProviderThreadExclusiveFunction>> RemainingList;


	// Here we handle the game thread only tasks, Each tick we try to handle every tasks
	// If a task is currently doing background work we add it back to the queue for next tick
	TTuple<FRuntimeMeshDataWeakPtr, FRuntimeMeshProviderThreadExclusiveFunction> TempData;
	while (ProxyParamsUpdateList.Dequeue(TempData))
	{
		FRuntimeMeshDataPtr Mesh = TempData.Key.Pin();
		if (!Mesh.IsValid())
		{
			continue;
		}

		if (Mesh->AsyncWorkState.TryLockForGameThread())
		{
			TempData.Value.Execute();
			Mesh->AsyncWorkState.Unlock();
		}
		else
		{
			RemainingList.Enqueue(TempData);
		}
	}

	// Add the remaining list back to the queue for next tick
	while (RemainingList.Dequeue(TempData))
	{
		ProxyParamsUpdateList.Enqueue(TempData);
	}
}

bool URuntimeMeshComponentEngineSubsystem::RunUpdateTasks(double MaxAllowedTime, bool bRunAsyncTaskList)
{
	if (MaxAllowedTime == 0.0f)
	{
		MaxAllowedTime = MaxAllowedTimePerTick;
	}

	// Convert time to seconds
	MaxAllowedTime /= 60.0;

	double StartTime = FPlatformTime::Seconds();

	TThreadSafeQueue<FRuntimeMeshDataWeakPtr>& CurrentTaskList = bRunAsyncTaskList ? AsyncUpdateList : GameThreadUpdateList;


	FRuntimeMeshDataWeakPtr FirstSkippedMesh;
	FRuntimeMeshDataWeakPtr TempMesh;
	while ((FPlatformTime::Seconds() - StartTime) < MaxAllowedTime && CurrentTaskList.Dequeue(TempMesh))
	{
		bool bHandled = false;

		FRuntimeMeshDataPtr Mesh = TempMesh.IsValid() ? TempMesh.Pin() : nullptr;
		if (!Mesh.IsValid())
		{
			continue;
		}

		// This stops the loop when we come full circle. So if we skip any due to inability to lock
		// we don't just keep tight looping to retry 
		if (FirstSkippedMesh.IsValid() && FirstSkippedMesh == TempMesh)
		{
			CurrentTaskList.Enqueue(TempMesh);
			break;
		}

		if (Mesh->AsyncWorkState.TryLockForAsyncThread())
		{
			UE_LOG(LogTemp, Warning, TEXT("Updating RMC In Thread: %d IsGameThread: %d"), FPlatformTLS::GetCurrentThreadId(), IsInGameThread());

			Mesh->HandleUpdate();
			Mesh->AsyncWorkState.Unlock();
		}
		else
		{
			CurrentTaskList.Enqueue(TempMesh);

			if (!FirstSkippedMesh.IsValid())
			{
				FirstSkippedMesh = TempMesh;
			}
		}

	}

	// Is there still more work to do?
	return !CurrentTaskList.IsEmpty();
}

