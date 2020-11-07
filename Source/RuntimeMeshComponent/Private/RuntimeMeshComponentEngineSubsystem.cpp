// Copyright 2016-2020 TriAxis Games L.L.C. All Rights Reserved.


#include "RuntimeMeshComponentEngineSubsystem.h"
#include "RuntimeMeshComponentSettings.h"
#include "RuntimeMeshCore.h"
#include "RuntimeMesh.h"

void URuntimeMeshComponentEngineSubsystem::FRuntimeMeshComponentDelayedActionTickObject::Tick(float DeltaTime)
{
	ParentSubsystem->Tick(DeltaTime);
}
bool URuntimeMeshComponentEngineSubsystem::FRuntimeMeshComponentDelayedActionTickObject::IsTickable() const
{
	return ParentSubsystem->IsTickable();
}


void URuntimeMeshComponentEngineSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Create the tick object
	TickObject = MakeUnique<FRuntimeMeshComponentDelayedActionTickObject>(this);

	const URuntimeMeshComponentSettings* Settings = GetDefault<URuntimeMeshComponentSettings>();
	check(Settings);

	InitializeThreads(CalculateNumThreads(Settings), Settings->ThreadStackSize, ConvertThreadPriority(Settings->ThreadPriority));
}

void URuntimeMeshComponentEngineSubsystem::Deinitialize()
{
	Super::Deinitialize();

	if (ThreadPool)
	{
		ThreadPool->Destroy();
		delete ThreadPool;
		ThreadPool = nullptr;
	}
}

void URuntimeMeshComponentEngineSubsystem::QueueMeshForUpdate(const FRuntimeMeshWeakRef& InMesh)
{
	MeshesToUpdate.Enqueue(InMesh);
}

int32 URuntimeMeshComponentEngineSubsystem::CalculateNumThreads(const URuntimeMeshComponentSettings* Settings)
{
	int32 MinThreads = Settings->MinMaxThreadPoolThreads.GetLowerBoundValue();
	int32 MaxThreads = Settings->MinMaxThreadPoolThreads.GetUpperBoundValue();
	int32 ThreadDivisor = Settings->SystemThreadDivisor;

	MaxThreads = FMath::Clamp(MaxThreads, 1, 64);
	MinThreads = FMath::Clamp(MinThreads, 1, MaxThreads);
	ThreadDivisor = FMath::Clamp(ThreadDivisor, 1, MaxThreads);

	int32 CoresWithHyperthreads = FGenericPlatformMisc::NumberOfCoresIncludingHyperthreads();

	return FMath::Clamp(
		FMath::RoundToInt(CoresWithHyperthreads / (float)ThreadDivisor),
		MinThreads, MaxThreads);
}


void URuntimeMeshComponentEngineSubsystem::InitializeThreads(int32 NumThreads, int32 StackSize /*= 0*/, EThreadPriority ThreadPriority /*= TPri_BelowNormal*/)
{
	if (ThreadPool == nullptr)
	{
		if (FPlatformProcess::SupportsMultithreading())
		{
			ThreadPool = FQueuedThreadPool::Allocate();

			int32 NumThreadsInThreadPool = NumThreads;

			if (FPlatformProperties::IsServerOnly())
			{
				NumThreadsInThreadPool = 1;
			}

			verify(ThreadPool->Create(NumThreadsInThreadPool, StackSize, ThreadPriority));
		}
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
	return !MeshesToUpdate.IsEmpty();
}

void URuntimeMeshComponentEngineSubsystem::Tick(float DeltaTime)
{
	FRuntimeMeshWeakRef WeakMeshRef;
	while (MeshesToUpdate.Dequeue(WeakMeshRef))
	{
		FRuntimeMeshSharedRef MeshRef = WeakMeshRef.Pin();

		if (MeshRef.IsValid())
		{
			if (MeshRef->MeshProviderPtr)
			{
// 				if (MeshRef->bNeedsInitialization)
// 				{
// 					MeshRef->InitializeInternal();
// 					MeshRef->bNeedsInitialization = false;
// 				}

				if (MeshRef->bCollisionIsDirty)
				{
					MeshRef->UpdateCollision();
					MeshRef->bCollisionIsDirty = false;
				}

				FReadScopeLock Lock(MeshRef->MeshProviderLock);
				if (MeshRef->bQueuedForMeshUpdate && !MeshRef->MeshProviderPtr->IsThreadSafe())
				{
					// we check again when we actually set it just to make sure it hasn't changed
					if (MeshRef->bQueuedForMeshUpdate.AtomicSet(false) == true)
					{
						MeshRef->HandleUpdate();
					}

				}
			}
		}
	}
}
