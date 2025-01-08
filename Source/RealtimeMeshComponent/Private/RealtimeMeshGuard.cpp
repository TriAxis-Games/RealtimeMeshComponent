// Copyright (c) 2015-2025 TriAxis Games, L.L.C. All Rights Reserved.

#include "RealtimeMeshGuard.h"

#include "Data/RealtimeMeshData.h"
#include "Data/RealtimeMeshShared.h"

namespace RealtimeMesh
{
	namespace Threading::Private
	{		
		static thread_local TMap<FRealtimeMeshGuard*, FRealtimeMeshGuardThreadState> ActiveThreadLocks;
	}
	
	void FRealtimeMeshGuard::ReadLock()
	{
		Threading::Private::FRealtimeMeshGuardThreadState& State = Threading::Private::ActiveThreadLocks.FindOrAdd(this);
		State.ReadDepth++;

		const uint32 ThisThreadId = FPlatformTLS::GetCurrentThreadId();

		// If we're already writing then don't attempt the lock, we already have exclusive access
		if (CurrentWriterThreadId.Load() != ThisThreadId && State.ReadDepth == 1)
		{
			InnerLock.ReadLock();
		}
	}
	
	void FRealtimeMeshGuard::WriteLock()
	{
		Threading::Private::FRealtimeMeshGuardThreadState& State = Threading::Private::ActiveThreadLocks.FindOrAdd(this);
		State.WriteDepth++;
		
		const uint32 ThisThreadId = FPlatformTLS::GetCurrentThreadId();

		if (CurrentWriterThreadId.Load() != ThisThreadId)
		{
			// Ensure we don't already own a read lock where we'd be trying to upgrade the lock
			check(State.ReadDepth == 0);

			InnerLock.WriteLock();
			CurrentWriterThreadId.Store(ThisThreadId);
		}
	}
	
	void FRealtimeMeshGuard::ReadUnlock()
	{
		checkf(Threading::Private::ActiveThreadLocks.Contains(this), TEXT("ReadUnlock called when the thread doesn't hold the lock."));
		Threading::Private::FRealtimeMeshGuardThreadState& State = Threading::Private::ActiveThreadLocks.FindChecked(this);
		checkf(State.ReadDepth > 0, TEXT("ReadUnlock called when the thread doesn't hold the lock."));
		State.ReadDepth--;

		const uint32 ThisThreadId = FPlatformTLS::GetCurrentThreadId();

		if (CurrentWriterThreadId.Load() != ThisThreadId && State.ReadDepth == 0)
		{
			InnerLock.ReadUnlock();
		}

		if (State.ReadDepth == 0 && State.WriteDepth == 0)
		{
			Threading::Private::ActiveThreadLocks.Remove(this);
		}
	}

	void FRealtimeMeshGuard::WriteUnlock()
	{
		const uint32 ThisThreadId = FPlatformTLS::GetCurrentThreadId();

		if (CurrentWriterThreadId.Load() == ThisThreadId)
		{
			checkf(Threading::Private::ActiveThreadLocks.Contains(this), TEXT("WriteUnlock called when the thread doesn't hold the lock."));
			Threading::Private::FRealtimeMeshGuardThreadState& State = Threading::Private::ActiveThreadLocks.FindChecked(this);
			checkf(State.WriteDepth > 0, TEXT("WriteUnlock called when the thread doesn't hold the lock."));
			State.WriteDepth--;

			if (State.WriteDepth == 0)
			{
				CurrentWriterThreadId.Store(0);
				InnerLock.WriteUnlock();
			}

			if (State.ReadDepth == 0 && State.WriteDepth == 0)
			{
				Threading::Private::ActiveThreadLocks.Remove(this);
			}			
		}
		else
		{
			checkf(false, TEXT("WriteUnlock called when the thread doesn't hold the lock."));
		}
	}

	bool FRealtimeMeshGuard::IsWriteLocked()
	{
		Threading::Private::FRealtimeMeshGuardThreadState& State = Threading::Private::ActiveThreadLocks.FindOrAdd(this);		
		const uint32 ThisThreadId = FPlatformTLS::GetCurrentThreadId();
		return State.WriteDepth > 0 && CurrentWriterThreadId.Load() == ThisThreadId;
	}

	bool FRealtimeMeshGuard::IsReadLocked()
	{
		Threading::Private::FRealtimeMeshGuardThreadState& State = Threading::Private::ActiveThreadLocks.FindOrAdd(this);
		const uint32 ThisThreadId = FPlatformTLS::GetCurrentThreadId();
		return State.ReadDepth > 0 || (State.WriteDepth > 0 && CurrentWriterThreadId.Load() == ThisThreadId);
	}


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
		: FRealtimeMeshScopeGuardRead(InSharedResources->GetGuard(), bLockImmediately)
	{ }

	FRealtimeMeshScopeGuardRead::FRealtimeMeshScopeGuardRead(const FRealtimeMeshPtr& InMesh, bool bLockImmediately)
		: FRealtimeMeshScopeGuardRead(InMesh->GetSharedResources(), bLockImmediately)
	{ }


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
		: FRealtimeMeshScopeGuardWrite(InSharedResources->GetGuard(), bLockImmediately)
	{ }
	
	FRealtimeMeshScopeGuardWrite::FRealtimeMeshScopeGuardWrite(const FRealtimeMeshPtr& InMesh, bool bLockImmediately)
		: FRealtimeMeshScopeGuardWrite(InMesh->GetSharedResources(), bLockImmediately)
	{ }

	FRealtimeMeshScopeGuardWriteCheck::FRealtimeMeshScopeGuardWriteCheck(const FRealtimeMeshSharedResourcesRef& SharedResources): FRealtimeMeshScopeGuardWriteCheck(SharedResources->GetGuard())
	{
	}

	FRealtimeMeshScopeGuardReadCheck::FRealtimeMeshScopeGuardReadCheck(const FRealtimeMeshSharedResourcesRef& SharedResources): FRealtimeMeshScopeGuardReadCheck(SharedResources->GetGuard())
	{
	}
}
