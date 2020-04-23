// Copyright 2016-2020 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HAL/PlatformAtomics.h"
#include "HAL/ThreadSafeBool.h"
#include "Templates/SharedPointer.h"

template<typename ObjectType>
struct FRuntimeMeshObjectSharedRef;
template<typename ObjectType>
struct FRuntimeMeshObjectWeakRef;


class FRuntimeMeshReferencer
{
public:
	FThreadSafeBool MarkedForDeletion;

	FRuntimeMeshReferencer()
		: MarkedForDeletion(false)
	{ }

	void MarkForCollection()
	{
		MarkedForDeletion.AtomicSet(true);
	}
};

template<typename ObjectType>
struct FRuntimeMeshReferenceAnchor
{
	using FReferencer = FRuntimeMeshReferencer;

private:
	TSharedPtr<FReferencer, ESPMode::ThreadSafe> SharedRef;
	TWeakPtr<FReferencer, ESPMode::ThreadSafe> WeakRef;
	TArray<TWeakPtr<FReferencer, ESPMode::ThreadSafe>> OldRefs;
	ObjectType* Object;
public:
	FRuntimeMeshReferenceAnchor()
	{
	}
	FRuntimeMeshReferenceAnchor(ObjectType* InObject)
		: SharedRef(MakeShared<FReferencer, ESPMode::ThreadSafe>())
		, WeakRef(SharedRef)
		, Object(InObject)
	{
	}

	void BeginNewState()
	{
		if (SharedRef)
		{
			SharedRef->MarkForCollection();
			SharedRef.Reset();

			OldRefs.Add(WeakRef);

			// Clean any stale refs
			for (int32 Index = OldRefs.Num() - 1; Index >= 0; Index--)
			{
				if (!OldRefs[Index].IsValid())
				{
					OldRefs.RemoveAtSwap(Index);
				}
			}

			SharedRef = MakeShared<FReferencer, ESPMode::ThreadSafe>();
			WeakRef = SharedRef;
		}

	}

	FRuntimeMeshObjectSharedRef<ObjectType> GetReference(bool bEvenIfMarked = false)
	{
		return FRuntimeMeshObjectSharedRef<ObjectType>(bEvenIfMarked ? WeakRef.Pin() : SharedRef, Object);
	}

	void BeginDestroy()
	{
		// If we still have a reference we flag it as marked for deletion.
		// and drop the shared ref, we use the weak reference to tell when it's safe to delete
		if (SharedRef)
		{
			SharedRef->MarkForCollection();
			SharedRef.Reset();
		}
	}

	bool IsFree() 
	{
		if (WeakRef.IsValid())
		{
			return false;
		}

		for (int32 Index = OldRefs.Num() - 1; Index >= 0; Index--)
		{
			if (OldRefs[Index].IsValid())
			{
				return false;
			}
			else
			{
				// Clean stale refs
				OldRefs.RemoveAtSwap(Index);
			}
		}

		return true;
	}

};



template<typename ObjectType>
struct FRuntimeMeshReferenceAliasAnchor
{
	using FReferencer = FRuntimeMeshReferencer;

private:
	TWeakPtr<FReferencer, ESPMode::ThreadSafe> WeakRef;
	ObjectType* Object;
public:
	FRuntimeMeshReferenceAliasAnchor()
	{
	}
	FRuntimeMeshReferenceAliasAnchor(ObjectType* InObject)
		: Object(InObject)
	{
	}

	template<typename OtherType>
	void BindToAssociated(const FRuntimeMeshObjectWeakRef<OtherType>& OtherAnchor)
	{
		check(!WeakRef.IsValid());

		FRuntimeMeshObjectSharedRef<OtherType> OtherAnchorPinned = OtherAnchor.Pin();
		if (OtherAnchorPinned)
		{
			WeakRef = OtherAnchorPinned.Ref;
		}
	}

	FRuntimeMeshObjectSharedRef<ObjectType> GetReference()
	{
		return FRuntimeMeshObjectSharedRef<ObjectType>(WeakRef.Pin(), Object);
	}

	bool IsFree()
	{
		return !WeakRef.IsValid();
// 		TSharedPtr<FReferencer, ESPMode::ThreadSafe> Pinned = WeakRef.Pin();
// 		if (Pinned.IsValid() && !Pinned->MarkedForDeletion && Pinned.GetSharedReferenceCount() == 2)
// 		{
// 			// Special case for editor where we get marked for deletion, but the parent mesh hasn't been
// 			// So only we and the RM should have a shared ref but it's not marked for deletion
// 			return true;
// 		}
// 
// 		return !Pinned.IsValid();
	}

};

template<typename ObjectType>
struct FRuntimeMeshObjectSharedRef
{
	using FReferencer = FRuntimeMeshReferencer;
private:
	TSharedPtr<FReferencer, ESPMode::ThreadSafe> Ref;
	ObjectType* Object;
	int32 Id;
	static FThreadSafeCounter Counter;
	static FThreadSafeCounter ActivePointers;

	FORCEINLINE FRuntimeMeshObjectSharedRef(const TSharedPtr<FReferencer, ESPMode::ThreadSafe>& InRef, ObjectType* InObject)
		: Ref(InRef), Object(InObject) 
	{ 
		Id = Counter.Increment();
		int32 active = ActivePointers.Increment();
		UE_LOG(LogTemp, Error, TEXT("Creating shared ref: %d  Active: %d"), Id, active);
	}

public:
	FORCEINLINE FRuntimeMeshObjectSharedRef() { }
	~FRuntimeMeshObjectSharedRef()
	{
		if (IsValid())
		{
			int32 active = ActivePointers.Decrement();
			UE_LOG(LogTemp, Error, TEXT("Destroying shared ref: %d  Active: %d"), Id, active);
		}
	}

	FORCEINLINE FRuntimeMeshObjectSharedRef(const FRuntimeMeshObjectSharedRef<ObjectType>& InRef)
		: Ref(InRef.Ref), Object(InRef.Object)
	{
		Id = Counter.Increment();
		int32 active = ActivePointers.Increment();
		UE_LOG(LogTemp, Error, TEXT("Creating shared ref: %d  Active: %d"), Id, active);
	}

	FORCEINLINE FRuntimeMeshObjectSharedRef(FRuntimeMeshObjectSharedRef<ObjectType>&& InRef)
		: Ref(MoveTemp(InRef.Ref)), Object(MoveTemp(InRef.Object)), Id(InRef.Id)
	{
	}


	FORCEINLINE FRuntimeMeshObjectSharedRef& operator=(const FRuntimeMeshObjectSharedRef<ObjectType>& InRef)
	{
		Ref = InRef.Ref;
		Object = InRef.Object;
		Id = Counter.Increment();
		int32 active = ActivePointers.Increment();
		UE_LOG(LogTemp, Error, TEXT("Creating shared ref: %d  Active: %d"), Id, active);
		return *this;
	}

	FORCEINLINE FRuntimeMeshObjectSharedRef& operator=(FRuntimeMeshObjectSharedRef<ObjectType>&& InRef)
	{
		Ref = MoveTemp(InRef.Ref);
		Object = MoveTemp(InRef.Object);
		Id = InRef.Id;
		return *this;
	}

	/**
	* Returns the object referenced by this pointer, or nullptr if no object is reference
	*
	* @return  The object owned by this shared pointer, or nullptr
	*/
	FORCEINLINE ObjectType* Get() const
	{
		return Object;
	}

	/**
	* Checks to see if this shared pointer is actually pointing to an object
	*
	* @return  True if the shared pointer is valid and can be dereferenced
	*/
	FORCEINLINE explicit operator bool() const
	{
		return Ref.IsValid();
	}

	/**
	* Checks to see if this shared pointer is actually pointing to an object
	*
	* @return  True if the shared pointer is valid and can be dereferenced
	*/
	FORCEINLINE const bool IsValid() const
	{
		return Ref.IsValid();
	}

	/**
	* Dereference operator returns a reference to the object this shared pointer points to
	*
	* @return  Reference to the object
	*/
	FORCEINLINE typename FMakeReferenceTo<ObjectType>::Type operator*() const
	{
		check(IsValid());
		return *Object;
	}

	/**
	* Arrow operator returns a pointer to the object this shared pointer references
	*
	* @return  Returns a pointer to the object referenced by this shared pointer
	*/
	FORCEINLINE ObjectType* operator->() const
	{
		check(IsValid());
		return Object;
	}

	/**
	* Resets this shared pointer, removing a reference to the object.  If there are no other shared
	* references to the object then it will be destroyed.
	*/
	FORCEINLINE void Reset()
	{
		Ref.Reset();
	}

	friend struct FRuntimeMeshObjectWeakRef<ObjectType>;

	template<typename OtherObjectType>
	friend struct FRuntimeMeshReferenceAnchor;
	template<typename OtherObjectType>
	friend struct FRuntimeMeshReferenceAliasAnchor;
};

template<typename ObjectType>
__declspec(selectany) FThreadSafeCounter FRuntimeMeshObjectSharedRef<ObjectType>::ActivePointers;

template<typename ObjectType>
__declspec(selectany) FThreadSafeCounter FRuntimeMeshObjectSharedRef<ObjectType>::Counter;

template<typename ObjectType>
struct FRuntimeMeshObjectWeakRef
{
	using FReferencer = FRuntimeMeshReferencer;
private:
	TWeakPtr<FReferencer, ESPMode::ThreadSafe> Ref;	
	ObjectType* Object;
	
	FORCEINLINE FRuntimeMeshObjectWeakRef(const TSharedPtr<FReferencer, ESPMode::ThreadSafe>& InRef, ObjectType* InObject)
		: Ref(InRef), Object(InObject) { }

public:
	FORCEINLINE FRuntimeMeshObjectWeakRef() { }

	FORCEINLINE FRuntimeMeshObjectWeakRef(const FRuntimeMeshObjectSharedRef<ObjectType>& InRef)
		: Ref(InRef.Ref), Object(InRef.Object) { }

	FORCEINLINE FRuntimeMeshObjectWeakRef(const FRuntimeMeshObjectWeakRef<ObjectType>& InRef)
		: Ref(InRef.Ref), Object(InRef.Object) { }

	FORCEINLINE FRuntimeMeshObjectWeakRef(FRuntimeMeshObjectWeakRef<ObjectType>&& InRef)
		: Ref(MoveTemp(InRef.Ref)), Object(MoveTemp(InRef.Object)) { }

	FORCEINLINE FRuntimeMeshObjectWeakRef& operator=(const FRuntimeMeshObjectSharedRef<ObjectType>& InRef)
	{
		Ref = InRef.Ref;
		Object = InRef.Object;
		return *this;
	}

	FORCEINLINE FRuntimeMeshObjectWeakRef& operator=(const FRuntimeMeshObjectWeakRef<ObjectType>& InRef)
	{
		Ref = MoveTemp(InRef.Ref);
		Object = MoveTemp(InRef.Object);
		return *this;
	}

	FORCEINLINE FRuntimeMeshObjectWeakRef& operator=(FRuntimeMeshObjectWeakRef<ObjectType>&& InRef)
	{
		Ref = InRef.Ref;
		Object = InRef.Object;
		return *this;
	}

	
	FORCEINLINE FRuntimeMeshObjectSharedRef<ObjectType> Pin(bool bShouldIgnoreGCMark = false) const
	{
		FRuntimeMeshObjectSharedRef<ObjectType> NewRef(Ref.Pin(), Object);

		if (!bShouldIgnoreGCMark && NewRef.Ref.IsValid() && NewRef.Ref->MarkedForDeletion)
		{
			NewRef.Ref.Reset();
		}

		return NewRef;
	}

	FORCEINLINE const bool IsValid(bool bShouldCheckGCMark = false) const
	{
		if (bShouldCheckGCMark)
		{
			TSharedPtr<FRuntimeMeshReferencer, ESPMode::ThreadSafe> TestRef = Ref.Pin();

			return TestRef.IsValid() && !TestRef->MarkedForDeletion;
		}
		return Ref.IsValid();
	}

	FORCEINLINE void Reset()
	{
		Ref.Reset();
	}
};


class URuntimeMesh;
class URuntimeMeshProvider;

using FRuntimeMeshSharedRef = FRuntimeMeshObjectSharedRef<URuntimeMesh>;
using FRuntimeMeshWeakRef = FRuntimeMeshObjectWeakRef<URuntimeMesh>;

using FRuntimeMeshProviderSharedRef = FRuntimeMeshObjectSharedRef<URuntimeMeshProvider>;
using FRuntimeMeshProviderWeakRef = FRuntimeMeshObjectWeakRef<URuntimeMeshProvider>;

