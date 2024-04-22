// Copyright TriAxis Games, L.L.C. All Rights Reserved.


#include "RealtimeMeshThreadingSubsystem.h"
#include "Engine/Engine.h"

void URealtimeMeshThreadingSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void URealtimeMeshThreadingSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

URealtimeMeshThreadingSubsystem* URealtimeMeshThreadingSubsystem::Get()
{
	return GEngine->GetEngineSubsystem<URealtimeMeshThreadingSubsystem>();
}

FQueuedThreadPool& URealtimeMeshThreadingSubsystem::GetThreadPool()
{
	if (!ThreadPool.IsValid())
	{
		ThreadPool = TUniquePtr<FQueuedThreadPool>(FQueuedThreadPool::Allocate());
		ThreadPool->Create(4,  64 * 1024, TPri_Normal, TEXT("RealtimeMeshThreadPool"));
	}
	
	return *ThreadPool;
}
