// Copyright (c) 2015-2025 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"
#include "Misc/QueuedThreadPool.h"
#include "RealtimeMeshThreadingSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class REALTIMEMESHCOMPONENT_API URealtimeMeshThreadingSubsystem : public UEngineSubsystem
{
	GENERATED_BODY()
private:
	TUniquePtr<FQueuedThreadPool> ThreadPool;
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	static URealtimeMeshThreadingSubsystem* Get();

	FQueuedThreadPool& GetThreadPool();
};
