// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "RealtimeMeshCore.h"

namespace RealtimeMesh
{
	/**
	 * Recursive Read/Write lock object for protecting realtime mesh data.
	 *
	 * The lock also allows a thread to recursively lock data to avoid deadlocks on repeated writes
	 * or undefined behavior for nesting read locks.
	 *
	 * Fairness is determined by the underlying platform FRWLock type as this lock uses FRWLock
	 * as it's internal primitive
	 */
	class FRealtimeMeshGuard
	{
	public:
		FRealtimeMeshGuard()
		{
			TlsSlot = FPlatformTLS::AllocTlsSlot();
			CurrentWriterThreadId.Store(0);
		}

		~FRealtimeMeshGuard()
		{
			if (FPlatformTLS::IsValidTlsSlot(TlsSlot))
			{
				check(CurrentWriterThreadId.Load() == 0);
				FPlatformTLS::FreeTlsSlot(TlsSlot);
			}
		}

		FRealtimeMeshGuard(const FRealtimeMeshGuard& InOther) = delete;
		FRealtimeMeshGuard(FRealtimeMeshGuard&& InOther) = delete;
		FRealtimeMeshGuard& operator=(const FRealtimeMeshGuard& InOther) = delete;
		FRealtimeMeshGuard& operator=(FRealtimeMeshGuard&& InOther) = delete;

		void ReadLock()
		{
			const FRealtimeMeshGuardTls ThreadData = ModifyTls([](FRealtimeMeshGuardTls& ThreadDataInner) { ThreadDataInner.ReadDepth++; });

			const uint32 ThisThreadId = FPlatformTLS::GetCurrentThreadId();

			// If we're already writing then don't attempt the lock, we already have exclusive access
			if (CurrentWriterThreadId.Load() != ThisThreadId && ThreadData.ReadDepth == 1)
			{
				InnerLock.ReadLock();
			}
		}

		void WriteLock()
		{
			const FRealtimeMeshGuardTls ThreadData = ModifyTls([](FRealtimeMeshGuardTls& ThreadDataInner) { ThreadDataInner.WriteDepth++; });

			const uint32 ThisThreadId = FPlatformTLS::GetCurrentThreadId();

			if (CurrentWriterThreadId.Load() != ThisThreadId)
			{
				// Ensure we don't already own a read lock where we'd be trying to upgrade the lock
				check(ThreadData.ReadDepth == 0);

				InnerLock.WriteLock();
				CurrentWriterThreadId.Store(ThisThreadId);
			}
		}

		void ReadUnlock()
		{
			const FRealtimeMeshGuardTls ThreadData = ModifyTls([](FRealtimeMeshGuardTls& ThreadDataInner)
			{
				checkf(ThreadDataInner.ReadDepth > 0, TEXT("ReadUnlock called when the thread doesn't hold the lock."));
				ThreadDataInner.ReadDepth--;
			});

			const uint32 ThisThreadId = FPlatformTLS::GetCurrentThreadId();

			if (CurrentWriterThreadId.Load() != ThisThreadId && ThreadData.ReadDepth == 0)
			{
				InnerLock.ReadUnlock();
			}
		}

		void WriteUnlock()
		{
			const uint32 ThisThreadId = FPlatformTLS::GetCurrentThreadId();

			if (CurrentWriterThreadId.Load() == ThisThreadId)
			{
				const FRealtimeMeshGuardTls ThreadData = ModifyTls([](FRealtimeMeshGuardTls& ThreadDataInner) { ThreadDataInner.WriteDepth--; });

				if (ThreadData.WriteDepth == 0)
				{
					CurrentWriterThreadId.Store(0);
					InnerLock.WriteUnlock();
				}
			}
			else
			{
				checkf(false, TEXT("WriteUnlock called when the thread doesn't hold the lock."));
			}
		}

	private:
		// We use 32 bits to store our depths (16 read and 16 write) allowing a maximum
		// recursive lock of depth 65,536. This unions to whatever the platform ptr size
		// is so we can store this directly into TLS without allocating more storage
		class FRealtimeMeshGuardTls
		{
		public:
			FRealtimeMeshGuardTls()
				: WriteDepth(0)
				  , ReadDepth(0)
			{
			}

			union
			{
				struct
				{
					uint16 WriteDepth;
					uint16 ReadDepth;
				};

				void* PtrDummy;
			};
		};

		// Helper for modifying the current TLS data
		template <typename CallableType>
		FRealtimeMeshGuardTls ModifyTls(CallableType Callable)
		{
			checkSlow(FPlatformTLS::IsValidTlsSlot(TlsSlot));

			void* ThreadData = FPlatformTLS::GetTlsValue(TlsSlot);

			FRealtimeMeshGuardTls TlsData;
			TlsData.PtrDummy = ThreadData;

			Callable(TlsData);

			FPlatformTLS::SetTlsValue(TlsSlot, TlsData.PtrDummy);

			return TlsData;
		}

		uint32 TlsSlot;
		TAtomic<uint32> CurrentWriterThreadId;
		FRWLock InnerLock;
	};


	struct FRealtimeMeshScopeGuardRead
	{
	public:
		RMC_NODISCARD_CTOR explicit FRealtimeMeshScopeGuardRead(FRealtimeMeshGuard& InGuard, bool bLockImmediately = true);
		RMC_NODISCARD_CTOR explicit FRealtimeMeshScopeGuardRead(const FRealtimeMeshSharedResourcesRef& InSharedResources, bool bLockImmediately = true);

		void Lock()
		{
			check(!bIsLocked)
			Guard.ReadLock();
			bIsLocked = true;
		}

		void Unlock()
		{
			check(bIsLocked);
			Guard.ReadUnlock();
			bIsLocked = false;
		}

		~FRealtimeMeshScopeGuardRead()
		{
			if (bIsLocked)
			{
				Guard.ReadUnlock();
			}
		}

	private:
		FRealtimeMeshGuard& Guard;
		bool bIsLocked;

		UE_NONCOPYABLE(FRealtimeMeshScopeGuardRead);
	};


	struct FRealtimeMeshScopeGuardWrite
	{
	public:
		RMC_NODISCARD_CTOR explicit FRealtimeMeshScopeGuardWrite(FRealtimeMeshGuard& InGuard, bool bLockImmediately = true);
		RMC_NODISCARD_CTOR explicit FRealtimeMeshScopeGuardWrite(const FRealtimeMeshSharedResourcesRef& InSharedResources, bool bLockImmediately = true);

		void Lock()
		{
			check(!bIsLocked)
			Guard.WriteLock();
			bIsLocked = true;
		}

		void Unlock()
		{
			check(bIsLocked);
			Guard.WriteUnlock();
			bIsLocked = false;
		}

		~FRealtimeMeshScopeGuardWrite()
		{
			if (bIsLocked)
			{
				Guard.WriteUnlock();
			}
		}

	private:
		FRealtimeMeshGuard& Guard;
		bool bIsLocked;

		UE_NONCOPYABLE(FRealtimeMeshScopeGuardWrite);
	};

	enum class ERealtimeMeshGuardLockType
	{
		Unlocked,
		Read,
		Write
	};

	struct FRealtimeMeshScopeGuardReadWrite
	{
	public:
		RMC_NODISCARD_CTOR explicit FRealtimeMeshScopeGuardReadWrite(FRealtimeMeshGuard& InGuard, ERealtimeMeshGuardLockType InLockType)
			: Guard(InGuard)
			  , LockType(ERealtimeMeshGuardLockType::Unlocked)
		{
			Lock(InLockType);
		}

		void Lock(ERealtimeMeshGuardLockType InLockType)
		{
			check(LockType == ERealtimeMeshGuardLockType::Unlocked);
			LockType = InLockType;
			if (LockType == ERealtimeMeshGuardLockType::Write)
			{
				Guard.WriteLock();
			}
			else if (LockType == ERealtimeMeshGuardLockType::Read)
			{
				Guard.ReadLock();
			}
		}

		void Unlock()
		{
			if (LockType == ERealtimeMeshGuardLockType::Write)
			{
				Guard.WriteUnlock();
			}
			else if (LockType == ERealtimeMeshGuardLockType::Read)
			{
				Guard.ReadUnlock();
			}
			LockType = ERealtimeMeshGuardLockType::Unlocked;
		}

		~FRealtimeMeshScopeGuardReadWrite()
		{
			Unlock();
		}

	private:
		UE_NONCOPYABLE(FRealtimeMeshScopeGuardReadWrite);

		FRealtimeMeshGuard& Guard;
		ERealtimeMeshGuardLockType LockType;
	};
}
