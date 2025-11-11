// Copyright (c) 2015-2025 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "RealtimeMeshCore.h"

namespace RealtimeMesh
{
	namespace Threading::Private
	{
		// We use 32 bits to store our depths (16 read and 16 write) allowing a maximum
		// recursive lock of depth 65,536. This unions to whatever the platform ptr size
		// is so we can store this directly into TLS without allocating more storage
		class FRealtimeMeshGuardThreadState
		{
		public:
			FRealtimeMeshGuardThreadState()
				: WriteDepth(0)
				, ReadDepth(0)
			{
			}

			uint16 WriteDepth;
			uint16 ReadDepth;
		};
	}


	
	/**
	 * Recursive Read/Write lock object for protecting realtime mesh data.
	 *
	 * The lock also allows a thread to recursively lock data to avoid deadlocks on repeated writes
	 * or undefined behavior for nesting read locks.
	 *
	 * Fairness is determined by the underlying platform FRWLock type as this lock uses FRWLock
	 * as it's internal primitive
	 */
	class REALTIMEMESHCOMPONENT_API FRealtimeMeshGuard
	{
	public:
		FRealtimeMeshGuard()
			: CurrentWriterThreadId(0)
		{ }

		~FRealtimeMeshGuard() { }

		FRealtimeMeshGuard(const FRealtimeMeshGuard& InOther) = delete;
		FRealtimeMeshGuard(FRealtimeMeshGuard&& InOther) = delete;
		FRealtimeMeshGuard& operator=(const FRealtimeMeshGuard& InOther) = delete;
		FRealtimeMeshGuard& operator=(FRealtimeMeshGuard&& InOther) = delete;

		void ReadLock();
		void WriteLock();
		void ReadUnlock();
		void WriteUnlock();

		bool IsWriteLocked();
		bool IsReadLocked();

	private:
		TAtomic<uint32> CurrentWriterThreadId;
		FRWLock InnerLock;
	};


	struct REALTIMEMESHCOMPONENT_API FRealtimeMeshScopeGuardRead
	{
	public:
		UE_NODISCARD_CTOR explicit FRealtimeMeshScopeGuardRead(FRealtimeMeshGuard& InGuard, bool bLockImmediately = true);
		UE_NODISCARD_CTOR explicit FRealtimeMeshScopeGuardRead(const FRealtimeMeshSharedResourcesRef& InSharedResources, bool bLockImmediately = true);
		UE_NODISCARD_CTOR explicit FRealtimeMeshScopeGuardRead(const FRealtimeMeshPtr& InMesh, bool bLockImmediately = true);

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


	struct REALTIMEMESHCOMPONENT_API FRealtimeMeshScopeGuardWrite
	{
	public:
		UE_NODISCARD_CTOR explicit FRealtimeMeshScopeGuardWrite(FRealtimeMeshGuard& InGuard, bool bLockImmediately = true);
		UE_NODISCARD_CTOR explicit FRealtimeMeshScopeGuardWrite(const FRealtimeMeshSharedResourcesRef& InSharedResources, bool bLockImmediately = true);
		UE_NODISCARD_CTOR explicit FRealtimeMeshScopeGuardWrite(const FRealtimeMeshPtr& InMesh, bool bLockImmediately = true);

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

	struct REALTIMEMESHCOMPONENT_API FRealtimeMeshScopeGuardReadWrite
	{
	public:
		UE_NODISCARD_CTOR explicit FRealtimeMeshScopeGuardReadWrite(FRealtimeMeshGuard& InGuard, ERealtimeMeshGuardLockType InLockType)
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


	
	

	struct REALTIMEMESHCOMPONENT_API FRealtimeMeshScopeGuardWriteCheck
	{		
		UE_NODISCARD_CTOR explicit FRealtimeMeshScopeGuardWriteCheck(FRealtimeMeshGuard& InGuard)
			: Guard(InGuard)
		{
			check(Guard.IsWriteLocked());
		}
		UE_NODISCARD_CTOR explicit FRealtimeMeshScopeGuardWriteCheck(const FRealtimeMeshSharedResourcesRef& SharedResources);

		~FRealtimeMeshScopeGuardWriteCheck()
		{
			check(Guard.IsWriteLocked());
		}
		
	private:
		UE_NONCOPYABLE(FRealtimeMeshScopeGuardWriteCheck);

		FRealtimeMeshGuard& Guard;
	};
	
	struct REALTIMEMESHCOMPONENT_API FRealtimeMeshScopeGuardReadCheck
	{		
		UE_NODISCARD_CTOR explicit FRealtimeMeshScopeGuardReadCheck(FRealtimeMeshGuard& InGuard)
			: Guard(InGuard)
		{
			check(Guard.IsReadLocked());
		}
		UE_NODISCARD_CTOR explicit FRealtimeMeshScopeGuardReadCheck(const FRealtimeMeshSharedResourcesRef& SharedResources);

		~FRealtimeMeshScopeGuardReadCheck()
		{
			check(Guard.IsReadLocked());
		}
		
	private:
		UE_NONCOPYABLE(FRealtimeMeshScopeGuardReadCheck);

		FRealtimeMeshGuard& Guard;
	};
}
