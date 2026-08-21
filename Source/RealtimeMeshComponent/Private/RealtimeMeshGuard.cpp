// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#include "RealtimeMeshGuard.h"

#include "Data/RealtimeMeshData.h"
#include "Data/RealtimeMeshShared.h"

namespace RealtimeMesh
{
	namespace Threading::Private
	{
		static thread_local TMap<FRealtimeMeshGuard*, FRealtimeMeshGuardThreadState> ActiveThreadLocks;

		// Zero-depth entries are kept alive across lock cycles to avoid per-cycle insert/remove
		// churn, but a destroyed guard leaves a dead key behind. Cap how many entries a thread
		// retains so workloads that churn through many transient meshes can't grow the TLS map
		// without bound.
		constexpr int32 PersistentLockEntryBudget = 64;
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
		// Single lookup: resolve the TLS entry once rather than Contains()+FindChecked().
		Threading::Private::FRealtimeMeshGuardThreadState* State = Threading::Private::ActiveThreadLocks.Find(this);
		checkf(State != nullptr, TEXT("ReadUnlock called when the thread doesn't hold the lock."));
		checkf(State->ReadDepth > 0, TEXT("ReadUnlock called when the thread doesn't hold the lock."));
		State->ReadDepth--;

		const uint32 ThisThreadId = FPlatformTLS::GetCurrentThreadId();

		if (CurrentWriterThreadId.Load() != ThisThreadId && State->ReadDepth == 0)
		{
			InnerLock.ReadUnlock();
		}

		// The zero-depth entry is deliberately left in the map: removing it here and
		// re-inserting it on the next lock is pure per-cycle churn, and IsRead/WriteLocked
		// already treat a zero-depth entry as "not held". Evict only past the budget.
		if (State->ReadDepth == 0 && State->WriteDepth == 0 && Threading::Private::ActiveThreadLocks.Num() > Threading::Private::PersistentLockEntryBudget)
		{
			Threading::Private::ActiveThreadLocks.Remove(this);
		}
	}

	void FRealtimeMeshGuard::WriteUnlock()
	{
		const uint32 ThisThreadId = FPlatformTLS::GetCurrentThreadId();

		if (CurrentWriterThreadId.Load() == ThisThreadId)
		{
			// Single lookup: resolve the TLS entry once rather than Contains()+FindChecked().
			Threading::Private::FRealtimeMeshGuardThreadState* State = Threading::Private::ActiveThreadLocks.Find(this);
			checkf(State != nullptr, TEXT("WriteUnlock called when the thread doesn't hold the lock."));
			checkf(State->WriteDepth > 0, TEXT("WriteUnlock called when the thread doesn't hold the lock."));
			State->WriteDepth--;

			if (State->WriteDepth == 0)
			{
				CurrentWriterThreadId.Store(0);
				InnerLock.WriteUnlock();

				// Write->read downgrade (non-LIFO release): if this thread still holds read
				// locks that were taken while it was the writer, they never actually acquired
				// InnerLock.ReadLock() (the exclusive write lock covered them). Genuinely
				// acquire the shared read lock now so the eventual ReadUnlock has real
				// ownership and doesn't call FRWLock::ReadUnlock() without holding it (UB).
				// This must happen after releasing the write lock: FRWLock (SRWLOCK /
				// pthread_rwlock) does not permit the same thread to hold the write lock and
				// then take a read lock, so an acquire-before-release would deadlock.
				if (State->ReadDepth > 0)
				{
					InnerLock.ReadLock();
				}
			}

			// The zero-depth entry is deliberately left in the map (see ReadUnlock).
			if (State->ReadDepth == 0 && State->WriteDepth == 0 && Threading::Private::ActiveThreadLocks.Num() > Threading::Private::PersistentLockEntryBudget)
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
		// Query only: use Find so we never insert a zero-depth TLS entry for a guard this
		// thread doesn't currently hold (FindOrAdd would leave dangling keys behind).
		const Threading::Private::FRealtimeMeshGuardThreadState* State = Threading::Private::ActiveThreadLocks.Find(this);
		const uint32 ThisThreadId = FPlatformTLS::GetCurrentThreadId();
		return State != nullptr && State->WriteDepth > 0 && CurrentWriterThreadId.Load() == ThisThreadId;
	}

	bool FRealtimeMeshGuard::IsReadLocked()
	{
		// Query only: use Find so we never insert a zero-depth TLS entry (see IsWriteLocked).
		const Threading::Private::FRealtimeMeshGuardThreadState* State = Threading::Private::ActiveThreadLocks.Find(this);
		const uint32 ThisThreadId = FPlatformTLS::GetCurrentThreadId();
		return State != nullptr && (State->ReadDepth > 0 || (State->WriteDepth > 0 && CurrentWriterThreadId.Load() == ThisThreadId));
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

	FRealtimeMeshScopeGuardRead::FRealtimeMeshScopeGuardRead(const FRealtimeMeshContextRef& InContext, bool bLockImmediately)
		: FRealtimeMeshScopeGuardRead(InContext->GetGuard(), bLockImmediately)
	{ }

	FRealtimeMeshScopeGuardRead::FRealtimeMeshScopeGuardRead(const FRealtimeMeshPtr& InMesh, bool bLockImmediately)
		: FRealtimeMeshScopeGuardRead(InMesh->GetContext(), bLockImmediately)
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

	FRealtimeMeshScopeGuardWrite::FRealtimeMeshScopeGuardWrite(const FRealtimeMeshContextRef& InContext, bool bLockImmediately)
		: FRealtimeMeshScopeGuardWrite(InContext->GetGuard(), bLockImmediately)
	{ }
	
	FRealtimeMeshScopeGuardWrite::FRealtimeMeshScopeGuardWrite(const FRealtimeMeshPtr& InMesh, bool bLockImmediately)
		: FRealtimeMeshScopeGuardWrite(InMesh->GetContext(), bLockImmediately)
	{ }

	FRealtimeMeshScopeGuardWriteCheck::FRealtimeMeshScopeGuardWriteCheck(const FRealtimeMeshContextRef& Context): FRealtimeMeshScopeGuardWriteCheck(Context->GetGuard())
	{
	}

	FRealtimeMeshScopeGuardReadCheck::FRealtimeMeshScopeGuardReadCheck(const FRealtimeMeshContextRef& Context): FRealtimeMeshScopeGuardReadCheck(Context->GetGuard())
	{
	}
}
