// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#include "RealtimeMeshEngineSubsystem.h"


bool URealtimeMeshEngineSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	return false;
}

inline URealtimeMeshEngineSubsystem* URealtimeMeshEngineSubsystem::GetInstance()
{
	return GEngine? GEngine->GetEngineSubsystem<URealtimeMeshEngineSubsystem>() : nullptr;
}