// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#include "RealtimeMeshGuard.h"
#include "Data/RealtimeMeshShared.h"

namespace RealtimeMesh
{
	FRealtimeMeshScopeGuardRead::FRealtimeMeshScopeGuardRead(FRealtimeMeshGuard& InGuard, bool bLockImmediately)
		: Guard(InGuard)
		  , bIsLocked(false)
	{
		if (bLockImmediately)
		{
			Guard.ReadLock();
			bIsLocked = true;
		}
	}

	FRealtimeMeshScopeGuardRead::FRealtimeMeshScopeGuardRead(const FRealtimeMeshSharedResourcesRef& InSharedResources, bool bLockImmediately)
		: Guard(InSharedResources->GetGuard())
		  , bIsLocked(false)
	{
		if (bLockImmediately)
		{
			Guard.ReadLock();
			bIsLocked = true;
		}
	}

	FRealtimeMeshScopeGuardWrite::FRealtimeMeshScopeGuardWrite(FRealtimeMeshGuard& InGuard, bool bLockImmediately)
		: Guard(InGuard)
		  , bIsLocked(false)
	{
		if (bLockImmediately)
		{
			Guard.WriteLock();
			bIsLocked = true;
		}
	}

	FRealtimeMeshScopeGuardWrite::FRealtimeMeshScopeGuardWrite(const FRealtimeMeshSharedResourcesRef& InSharedResources, bool bLockImmediately)
		: Guard(InSharedResources->GetGuard())
		  , bIsLocked(false)
	{
		if (bLockImmediately)
		{
			Guard.WriteLock();
			bIsLocked = true;
		}
	}
}
