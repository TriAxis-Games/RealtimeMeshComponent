// Copyright 2016-2020 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HAL/PlatformAtomics.h"
#include "HAL/ThreadSafeBool.h"
#include "Templates/SharedPointer.h"
#include "Templates/SharedPointerInternals.h"

template<typename ObjectType>
struct FRuntimeMeshReference3;
template<typename ObjectType>
struct FRuntimeMeshPinnedReference3;
template<typename ObjectType>
struct FRuntimeMeshReferenceAnchor3;


template<typename ObjectType>
struct FRuntimeMeshReferenceObject
{
public:
	ObjectType* Object;

	volatile int32 ReferenceCount;
	volatile int32 PinnedReferences;
	volatile int32 bMarkedForCollection;

	explicit FRuntimeMeshReferenceObject(ObjectType* InObject)
		: Object(InObject)
		, ReferenceCount(1)
		, PinnedReferences(0)
		, bMarkedForCollection(0)
	{
	}


	// Non-copyable
	FRuntimeMeshReferenceObject(const FRuntimeMeshReferenceObject&) = delete;
	FRuntimeMeshReferenceObject& operator=(const FRuntimeMeshReferenceObject&) = delete;
};


template<typename ObjectType>
struct FRuntimeMeshReferenceOps
{
	using FReferencer = FRuntimeMeshReferenceObject<ObjectType>;

	static FORCEINLINE const int32 GetReferenceCount(FReferencer* Reference)
	{
		return FPlatformAtomics::AtomicRead(&Reference->ReferenceCount);
	}
	static FORCEINLINE const int32 GetPinnedReferenceCount(FReferencer* Reference)
	{
		return FPlatformAtomics::AtomicRead(&Reference->PinnedReferences);
	}
	static FORCEINLINE const bool IsPinnedByAny(FReferencer* Reference)
	{
		return GetPinnedReferenceCount(Reference) != 0;
	}

	static FORCEINLINE const void MarkForCollection(FReferencer* Reference)
	{
		FPlatformAtomics::InterlockedExchange(&Reference->bMarkedForCollection, 1);
	}
	static FORCEINLINE const bool IsMarkedForCollection(FReferencer* Reference)
	{
		return FPlatformAtomics::AtomicRead(&Reference->bMarkedForCollection) != 0;
	}


	static FORCEINLINE const int32 AddReference(FReferencer* Reference)
	{
		FPlatformAtomics::InterlockedIncrement(&Reference->ReferenceCount);
	}

	static FORCEINLINE const int32 ReleaseReference(FReferencer* Reference)
	{
		checkSlow(FPlatformAtomics::AtomicRead(&Reference->ReferenceCount) > 0);

		if (FPlatformAtomics::InterlockedDecrement(&Reference->ReferenceCount) == 0)
		{
			// No more references to this referencer object, destroy it
			delete Reference;
		}
	}



	static FORCEINLINE const bool TryAquiredPinnedReference(FReferencer* Reference)
	{
		// Are we marked for collection
		if (FPlatformAtomics::AtomicRead(&Reference->bMarkedForCollection) != 0)
		{
			return false;
		}

		// Add the pinned ref
		FPlatformAtomics::InterlockedIncrement(&Reference->PinnedReferences);

		// Are we marked for collection after having pinned the ref?
		if (FPlatformAtomics::AtomicRead(&Reference->bMarkedForCollection) != 0)
		{
			// Remove our pinned ref and return false;
			FPlatformAtomics::InterlockedDecrement(&Reference->PinnedReferences);
			return false;
		}

		AddReference(Reference);
		return true;
	}

	static FORCEINLINE const void AddPinnedReference(FReferencer* Reference)
	{
		AddReference(Reference);
		FPlatformAtomics::InterlockedIncrement(&Reference->PinnedReferences);
	}

	static FORCEINLINE const void ReleasePinnedReference(FReferencer* Reference)
	{
		FPlatformAtomics::InterlockedDecrement(&Reference->PinnedReferences);
		ReleaseReference(Reference);
	}

};


template<typename ObjectType>
struct FRuntimeMeshReference3
{
private:
	using TOps = FRuntimeMeshReferenceOps<ObjectType>;

	FRuntimeMeshReferenceObject<ObjectType>* Controller;

	FORCEINLINE FRuntimeMeshReference3(FRuntimeMeshReferenceObject<ObjectType>* InController)
		: Controller(InController)
	{
		if (Controller != nullptr)
		{
			TOps::AddReference(Controller);
		}
	}

public:

	FORCEINLINE FRuntimeMeshReference3()
		: Controller(nullptr)
	{

	}

	FORCEINLINE FRuntimeMeshReference3(const FRuntimeMeshReference3& InReference)
		: Controller(InReference.Controller)
	{
		if (Controller != nullptr)
		{
			TOps::AddReference(Controller);
		}
	}

	FORCEINLINE FRuntimeMeshReference3(FRuntimeMeshReference3&& InReference)
		: Controller(InReference.Controller)
	{
		InReference.Controller = nullptr;
	}

	FORCEINLINE FRuntimeMeshReference3& operator=(const FRuntimeMeshReference3& InReference)
	{
		auto OldController = Controller;
		auto NewController = InReference.Controller;

		if (OldController != NewController)
		{
			if (OldController)
			{
				TOps::ReleaseReference(OldController);
			}

			if (NewController)
			{
				TOps::AddReference(NewController);
			}

			Controller = NewController;
		}

		return *this;
	}

	FORCEINLINE FRuntimeMeshReference3& operator=(FRuntimeMeshReference3&& InReference)
	{
		if (this != &InReference)
		{
			auto OldController = Controller;

			Controller = InReference.Controller;
			InReference.Controller = nullptr;

			if (OldController)
			{
				TOps::ReleaseReference(OldController);
			}
		}
		return *this;
	}

	FORCEINLINE ~FRuntimeMeshReference3()
	{
		if (Controller != nullptr)
		{
			TOps::ReleaseReference(Controller);
		}
	}

	FORCEINLINE FRuntimeMeshPinnedReference3<ObjectType> Pin() const
	{
		return FRuntimeMeshReference3<ObjectType>(*this);
	}

	FORCEINLINE const bool IsValid() const
	{
		return Controller != nullptr && !TOps::IsMarkedForCollection(Controller);
	}

	FORCEINLINE const int32 GetReferenceCount() const
	{
		return Controller ? TOps::GetReferenceCount(Controller) : 0;
	}

	FORCEINLINE void Reset()
	{
		if (Controller)
		{
			TOps::ReleaseReference(Controller);
			Controller = nullptr;
		}
	}

	friend uint32 GetTypeHash(const FRuntimeMeshReference3& InReference)
	{
		ObjectType* Ptr = InReference.Controller ? InReference.Controller->Object : nullptr;
		return ::PointerHash(Ptr);
	}

	friend struct FRuntimeMeshPinnedReference3<ObjectType>;
	friend struct FRuntimeMeshReferenceAnchor3<ObjectType>;
};


template<typename ObjectType>
struct FRuntimeMeshPinnedReference3
{
private:
	using TOps = FRuntimeMeshReferenceOps<ObjectType>;

	FRuntimeMeshReferenceObject<ObjectType>* Controller;
	ObjectType* Object;

	FORCEINLINE FRuntimeMeshPinnedReference3(const FRuntimeMeshReference3<ObjectType>& InReference)
		: Controller(InReference.Controller)
		, Object(nullptr)
	{
		if (Controller != nullptr)
		{
			if (TOps::TryAquiredPinnedReference(Controller))
			{
				Object = Controller->Object;
			}
			else
			{
				Controller = nullptr;
			}
		}
	}


public:

	FORCEINLINE FRuntimeMeshPinnedReference3(const FRuntimeMeshPinnedReference3& InReference)
		: Controller(InReference.Controller)
		, Object(nullptr)
	{
		if (Controller != nullptr)
		{
			TOps::AddPinnedReference(Controller);
			Object = Controller->Object;
		}
	}

	FORCEINLINE FRuntimeMeshPinnedReference3& operator=(const FRuntimeMeshPinnedReference3& InReference)
	{
		auto OldController = Controller;
		auto NewController = InReference->Controller;

		if (OldController != NewController)
		{
			if (OldController)
			{
				TOps::ReleasePinnedReference(OldController);
			}

			if (NewController)
			{
				TOps::AddPinnedReference(NewController);
			}

			Controller = NewController;
		}

		Object = Controller ? Controller->Object : nullptr;

		return *this;
	}

	FORCEINLINE FRuntimeMeshPinnedReference3& operator=(FRuntimeMeshPinnedReference3&& InReference)
	{
		auto OldController = Controller;
		auto NewController = InReference->Controller;

		if (OldController != NewController)
		{
			if (OldController)
			{
				TOps::ReleasePinnedReference(OldController);
			}

			InReference.Controller = nullptr;
			InReference.Object = nullptr;

			Controller = NewController;
		}

		Object = Controller ? Controller->Object : nullptr;

		return *this;
	}




	FORCEINLINE ~FRuntimeMeshPinnedReference3()
	{
		if (Controller != nullptr)
		{
			TOps::ReleasePinnedReference(Controller);
		}
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
		return Object != nullptr;
	}

	/**
	* Checks to see if this shared pointer is actually pointing to an object
	*
	* @return  True if the shared pointer is valid and can be dereferenced
	*/
	FORCEINLINE const bool IsValid() const
	{
		return Object != nullptr;
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
		if (Controller)
		{
			TOps::ReleasePinnedReference(Controller);
			Controller = nullptr;
			Object = nullptr;
		}
	}

	FORCEINLINE const int32 GetReferenceCount() const
	{
		return Controller ? TOps::GetReferenceCount(Controller) : 0;
	}

	FORCEINLINE const int32 GetPinnedReferenceCount() const
	{
		return Controller ? TOps::GetPinnedReferenceCount(Controller) : 0;
	}

	friend uint32 GetTypeHash(const FRuntimeMeshPinnedReference3<ObjectType>& InReference)
	{
		ObjectType* Ptr = InReference.Controller ? InReference.Controller->Object : nullptr;
		return ::PointerHash(Ptr);
	}

	friend struct FRuntimeMeshReference3<ObjectType>;
	friend struct FRuntimeMeshReferenceAnchor3<ObjectType>;
};


template<typename ObjectType>
struct FRuntimeMeshReferenceAnchor3
{
private:
	using TOps = FRuntimeMeshReferenceOps<ObjectType>;
	FRuntimeMeshReferenceObject<ObjectType>* CurrentReferenceController;
	TArray<FRuntimeMeshReferenceObject<ObjectType>*> WeakRefs;
public:
	FRuntimeMeshReferenceAnchor3()
		: CurrentReferenceController(nullptr)
	{
	}
	FRuntimeMeshReferenceAnchor3(ObjectType* InObject)
		: CurrentReferenceController(new FRuntimeMeshReferenceObject<ObjectType>(InObject))
	{
	}

	void BeginNewState()
	{
		ObjectType* Object = CurrentReferenceController ? CurrentReferenceController->Object : nullptr;
		check(Object);

		// Destroy the old ref
		BeginDestroy();

		// Create our new ref
		CurrentReferenceController = new FRuntimeMeshReferenceObject<ObjectType>(Object);
	}

	FRuntimeMeshReference3<ObjectType> GetReference()
	{
		return FRuntimeMeshReference3<ObjectType>(CurrentReferenceController);
	}

	void BeginDestroy()
	{
		// If we still have a reference we flag it as marked for deletion.
		// and drop the shared ref, we use the weak reference to tell when it's safe to delete
		if (CurrentReferenceController)
		{
			// Make for collection and add it to the old refs list
			TOps::MarkForCollection(CurrentReferenceController);
			WeakRefs.Add(CurrentReferenceController);
			CurrentReferenceController = nullptr;

			// Clear refs that aren't pinned at all after being marked
			ClearStaleRefs();
		}
	}

	bool IsFree()
	{
		if (CurrentReferenceController && TOps::GetPinnedReferenceCount(CurrentReferenceController) > 0)
		{
			return false;
		}

		ClearStaleRefs();

		return WeakRefs.Num() == 0;
	}

private:
	void ClearStaleRefs()
	{
		for (int32 Index = WeakRefs.Num() - 1; Index >= 0; Index--)
		{
			if (TOps::GetPinnedReferenceCount(WeakRefs[Index]) == 0)
			{
				// Release our reference to the tracker and remove it
				TOps::ReleaseReference(WeakRefs[Index]);
				WeakRefs.RemoveAtSwap(Index);
			}
		}
	}

};



class URuntimeMesh;
class URuntimeMeshProvider;

using FRuntimeMeshSharedRef = FRuntimeMeshPinnedReference3<URuntimeMesh>;
using FRuntimeMeshWeakRef = FRuntimeMeshReference3<URuntimeMesh>;

using FRuntimeMeshProviderSharedRef = FRuntimeMeshPinnedReference3<URuntimeMeshProvider>;
using FRuntimeMeshProviderWeakRef = FRuntimeMeshReference3<URuntimeMeshProvider>;





//
//
//template<typename ObjectType>
//struct FRuntimeMeshReference2
//{
//private:
//	FRuntimeMeshReferenceController* Controller;
//	ObjectType* Object;
//
//	FORCEINLINE FRuntimeMeshReference2(const FRuntimeMeshWeakReference2<ObjectType>& InWeakRef)
//		: Controller(InWeakRef.Controller)
//		, Object(nullptr)
//	{
//		if (Controller != nullptr)
//		{
//			// Sleep a small slice of time until the GC has acquired the lock, then we can wait on the lock event.
//			// ideally there'd be another event within the FGCCSyncObject to allow waiting on it for the GC to run, 
//			// but it doesn't exist and that'd require changing engine code
//			// TODO: Look into adding this and PR'ing it
//			//FPlatformProcess::ConditionalSleep([]() { return FGCCSyncObject::Get()->IsGCWaiting(); }, 1.0f / 120.0f);
//
//			//FGCCSyncObject::Get()->LockAsync();
//			if (Controller->ConditionallyAddSharedReference())
//			{
//				Object = InWeakRef.Object;
//			}
//			else
//			{
//				Controller = nullptr;
//				//FGCCSyncObject::Get()->UnlockAsync();
//			}
//		}
//	}
//
//
//public:
//
//	FORCEINLINE FRuntimeMeshReference2(const FRuntimeMeshReference2& InReference)
//		: Controller(InReference.Controller)
//		, Object(nullptr)
//	{
//		if (Controller != nullptr)
//		{
//			Controller->AddSharedReference();
//			Object = InReference->Object;
//		}
//	}
//
//
//
//	FORCEINLINE FRuntimeMeshReference2& operator=(const FRuntimeMeshReference2& InReference)
//	{
//		auto NewReferenceController = InReference.Controller;
//		if (NewReferenceController != Controller)
//		{
//			// First, add a shared reference to the new object
//			if (NewReferenceController != nullptr)
//			{
//				NewReferenceController->AddSharedReference();
//			}
//
//			// Release shared reference to the old object
//			if (Controller != nullptr)
//			{
//				Controller->ReleaseSharedReference();
//			}
//
//			// Assume ownership of the assigned reference counter
//			Controller = NewReferenceController;
//			Object = InReference->Object;
//		}
//		else
//		{
//			check(Object == InReference->Object;)
//		}
//
//		return *this;
//	}
//
//	FORCEINLINE FRuntimeMeshReference2& operator=(FRuntimeMeshReference2&& InReference)
//	{
//		// Make sure we're not be reassigned to ourself!
//		auto NewReferenceController = InReference.Controller;
//		auto OldReferenceController = Controller;
//		if (NewReferenceController != OldReferenceController)
//		{
//			Controller = NewReferenceController;
//			Object = InReference.Object;
//
//			// Assume ownership of the assigned reference counter
//			InReference.Controller = nullptr;
//			InReference.Object = nullptr;
//
//			// Release shared reference to the old object
//			if (OldReferenceController != nullptr)
//			{
//				OldReferenceController->ReleaseSharedReference();
//			}
//		}
//		return *this;
//	}
//
//
//
//
//	FORCEINLINE ~FRuntimeMeshReference2()
//	{
//		if (Controller != nullptr)
//		{
//			Controller->ReleaseSharedReference();
//			//FGCCSyncObject::Get()->UnlockAsync();
//		}
//	}
//
//
//
//	/**
//	* Returns the object referenced by this pointer, or nullptr if no object is reference
//	*
//	* @return  The object owned by this shared pointer, or nullptr
//	*/
//	FORCEINLINE ObjectType* Get() const
//	{
//		return Object;
//	}
//
//	/**
//	* Checks to see if this shared pointer is actually pointing to an object
//	*
//	* @return  True if the shared pointer is valid and can be dereferenced
//	*/
//	FORCEINLINE explicit operator bool() const
//	{
//		return Object != nullptr;
//	}
//
//	/**
//	* Checks to see if this shared pointer is actually pointing to an object
//	*
//	* @return  True if the shared pointer is valid and can be dereferenced
//	*/
//	FORCEINLINE const bool IsValid() const
//	{
//		return Object != nullptr;
//	}
//
//	/**
//	* Dereference operator returns a reference to the object this shared pointer points to
//	*
//	* @return  Reference to the object
//	*/
//	FORCEINLINE typename FMakeReferenceTo<ObjectType>::Type operator*() const
//	{
//		check(IsValid());
//		return *Object;
//	}
//
//	/**
//	* Arrow operator returns a pointer to the object this shared pointer references
//	*
//	* @return  Returns a pointer to the object referenced by this shared pointer
//	*/
//	FORCEINLINE ObjectType* operator->() const
//	{
//		check(IsValid());
//		return Object;
//	}
//
//	/**
//	* Resets this shared pointer, removing a reference to the object.  If there are no other shared
//	* references to the object then it will be destroyed.
//	*/
//	FORCEINLINE void Reset()
//	{
//		if (Controller)
//		{
//			Controller->ReleaseSharedReference();
//			Controller = nullptr;
//			Object = nullptr;
//		}
//	}
//
//	/**
//	* Returns the number of shared references to this object (including this reference.)
//	* IMPORTANT: Not necessarily fast!  Should only be used for debugging purposes!
//	*
//	* @return  Number of shared references to the object (including this reference.)
//	*/
//	FORCEINLINE const int32 GetSharedReferenceCount() const
//	{
//		return Controller ? Controller->GetSharedReferenceCount() : 0;
//	}
//
//	/**
//	* Returns true if this is the only shared reference to this object.  Note that there may be
//	* outstanding weak references left.
//	* IMPORTANT: Not necessarily fast!  Should only be used for debugging purposes!
//	*
//	* @return  True if there is only one shared reference to the object, and this is it!
//	*/
//	FORCEINLINE const bool IsUnique() const
//	{
//		return GetSharedReferenceCount() == 1;
//	}
//
//	/**
//	* Computes a hash code for this object
//	*
//	* @param  InSharedPtr  Shared pointer to compute hash code for
//	*
//	* @return  Hash code value
//	*/
//	friend uint32 GetTypeHash(const FRuntimeMeshReference2& InReference)
//	{
//		return ::PointerHash(InReference.Object);
//	}
//
//	friend struct FRuntimeMeshWeakReference2<ObjectType>;
//	friend struct FRuntimeMeshReferenceAnchor2<ObjectType>;
//};
//
//
//
//template<typename ObjectType>
//struct FRuntimeMeshWeakReference2
//{
//private:
//	FRuntimeMeshReferenceController* Controller;
//	ObjectType* Object;
//
//	FORCEINLINE FRuntimeMeshWeakReference2(FRuntimeMeshReferenceController* InController, ObjectType* InObject)
//		: Controller(InController)
//		, Object(InObject)
//	{
//		if (Controller != nullptr)
//		{
//			Controller->AddWeakReference();
//		}
//	}
//
//public:
//
//	FORCEINLINE FRuntimeMeshWeakReference2()
//		: Controller(nullptr)
//		, Object(nullptr)
//	{
//
//	}
//
//	FORCEINLINE FRuntimeMeshWeakReference2(const FRuntimeMeshWeakReference2& InWeakPtr)
//		: Controller(InWeakPtr.Controller)
//		, Object(InWeakPtr.Object)
//	{
//		if (Controller != nullptr)
//		{
//			Controller->AddWeakReference();
//		}
//	}
//
//	FORCEINLINE FRuntimeMeshWeakReference2(FRuntimeMeshWeakReference2&& InWeakPtr)
//		: Controller(InWeakPtr.Controller)
//		, Object(InWeakPtr.Object)
//	{
//		InWeakPtr.Controller = nullptr;
//		InWeakPtr.Object = nullptr;
//	}
//
//	FORCEINLINE FRuntimeMeshWeakReference2& operator=(const FRuntimeMeshWeakReference2& InWeakPtr)
//	{
//		auto OldController = Controller;
//		if (OldController != InWeakPtr.Controller)
//		{
//			Controller = InWeakPtr.Controller;
//			Object = InWeakPtr.Object;
//
//			if (Controller)
//			{
//				Controller->AddWeakReference();
//			}
//
//			if (OldController)
//			{
//				Controller->ReleaseWeakReference();
//			}
//		}
//
//		return *this;
//	}
//
//	FORCEINLINE FRuntimeMeshWeakReference2& operator=(FRuntimeMeshWeakReference2&& InWeakPtr)
//	{
//		if (this != &InWeakPtr)
//		{
//			auto OldController = Controller;
//
//			Controller = InWeakPtr.Controller;
//			Object = InWeakPtr.Object;
//
//			InWeakPtr.Controller = nullptr;
//			InWeakPtr.Object = nullptr;
//
//			if (OldController)
//			{
//				OldController->ReleaseWeakReference();
//			}
//		}
//		return *this;
//	}
//
//	FORCEINLINE ~FRuntimeMeshWeakReference2()
//	{
//		if (Controller != nullptr)
//		{
//			Controller->ReleaseWeakReference();
//		}
//	}
//
//	FORCEINLINE FRuntimeMeshReference2<ObjectType> Pin() const
//	{
//		return FRuntimeMeshReference2<ObjectType>(*this);
//	}
//
//	FORCEINLINE const bool IsValid() const
//	{
//		return Object != nullptr &&
//			Controller != nullptr &&
//			Controller->GetSharedReferenceCount() > 0 &&
//			!Controller->IsMarkedForDeletion();
//	}
//
//	FORCEINLINE void Reset()
//	{
//		if (Controller)
//		{
//			Controller->ReleaseWeakReference();
//			Controller = nullptr;
//		}
//
//		Object = nullptr;
//	}
//
//private:
//
//	friend uint32 GetTypeHash(const FRuntimeMeshWeakReference2& InWeakPtr)
//	{
//		return ::PointerHash(InWeakPtr.Object);
//	}
//
//
//	// Declare ourselves as a friend of FRuntimeMeshReference2 so we can access members as needed
//
//	friend struct FRuntimeMeshReference2<ObjectType>;
//	friend struct FRuntimeMeshReferenceAnchor2<ObjectType>;
//
//private:
//};
//
//
//
//
//
//
//
//template<typename ObjectType>
//struct FRuntimeMeshReferenceAnchor2
//{
//
//private:
//	FRuntimeMeshReferenceController* CurrentReferenceController;
//	TArray<FRuntimeMeshReferenceController*> WeakRefs;
//
//	ObjectType* Object;
//public:
//	FRuntimeMeshReferenceAnchor2()
//		: CurrentReferenceController(nullptr)
//		, Object(nullptr)
//	{
//	}
//	FRuntimeMeshReferenceAnchor2(ObjectType* InObject)
//		: CurrentReferenceController(new FRuntimeMeshReferenceController())
//		, Object(InObject)
//	{
//		CurrentReferenceController->AddWeakReference();
//		WeakRefs.Add(CurrentReferenceController);
//	}
//
//	void BeginNewState()
//	{
//		if (CurrentReferenceController)
//		{
//			// Mark for deletion and then drop the shared ref so we can start the process of dropping references
//			CurrentReferenceController->MarkForDeletion();
//			CurrentReferenceController->ReleaseSharedReference();
//
//			ClearStaleRefs();
//
//			// Create a new reference controller
//			CurrentReferenceController = new FRuntimeMeshReferenceController();
//			CurrentReferenceController->AddWeakReference();
//			WeakRefs.Add(CurrentReferenceController);
//		}
//	}
//
//	FRuntimeMeshWeakReference2<ObjectType> GetReference()
//	{
//		return FRuntimeMeshWeakReference2<ObjectType>(CurrentReferenceController, Object);
//	}
//
//	void BeginDestroy()
//	{
//		// If we still have a reference we flag it as marked for deletion.
//		// and drop the shared ref, we use the weak reference to tell when it's safe to delete
//		if (CurrentReferenceController)
//		{
//			CurrentReferenceController->MarkForDeletion();
//			CurrentReferenceController->ReleaseSharedReference();
//			CurrentReferenceController = nullptr;
//
//			ClearStaleRefs();
//		}
//	}
//
//	bool IsFree()
//	{
//		if (CurrentReferenceController && CurrentReferenceController->GetSharedReferenceCount() > 0)
//		{
//			return false;
//		}
//
//		ClearStaleRefs();
//
//		return WeakRefs.Num() == 0;
//	}
//
//private:
//	void ClearStaleRefs()
//	{
//		for (int32 Index = WeakRefs.Num() - 1; Index >= 0; Index--)
//		{
//			if (WeakRefs[Index]->GetSharedReferenceCount() == 0)
//			{
//				// Drop our weak reference to the controller then remove it from the list
//				WeakRefs[Index]->ReleaseWeakReference();
//				WeakRefs.RemoveAtSwap(Index);
//			}
//		}
//	}
//};
























//
//
//
//
//
//
//
//class FRuntimeMeshReferencer
//{
//public:
//	FThreadSafeBool MarkedForDeletion;
//
//	FRuntimeMeshReferencer()
//		: MarkedForDeletion(false)
//	{ }
//
//	void MarkForCollection()
//	{
//		MarkedForDeletion.AtomicSet(true);
//	}
//};
//
//template<typename ObjectType>
//struct FRuntimeMeshReferenceAnchor
//{
//	using FReferencer = FRuntimeMeshReferencer;
//
//private:
//	TSharedPtr<FReferencer, ESPMode::ThreadSafe> SharedRef;
//	TWeakPtr<FReferencer, ESPMode::ThreadSafe> WeakRef;
//	TArray<TWeakPtr<FReferencer, ESPMode::ThreadSafe>> OldRefs;
//	ObjectType* Object;
//public:
//	FRuntimeMeshReferenceAnchor()
//	{
//	}
//	FRuntimeMeshReferenceAnchor(ObjectType* InObject)
//		: SharedRef(MakeShared<FReferencer, ESPMode::ThreadSafe>())
//		, WeakRef(SharedRef)
//		, Object(InObject)
//	{
//	}
//
//	void BeginNewState()
//	{
//		if (SharedRef)
//		{
//			SharedRef->MarkForCollection();
//			SharedRef.Reset();
//
//			OldRefs.Add(WeakRef);
//
//			// Clean any stale refs
//			for (int32 Index = OldRefs.Num() - 1; Index >= 0; Index--)
//			{
//				if (!OldRefs[Index].IsValid())
//				{
//					OldRefs.RemoveAtSwap(Index);
//				}
//			}
//
//			SharedRef = MakeShared<FReferencer, ESPMode::ThreadSafe>();
//			WeakRef = SharedRef;
//		}
//	}
//
//	FRuntimeMeshObjectWeakRef<ObjectType> GetReference()
//	{
//		return FRuntimeMeshObjectWeakRef<ObjectType>(WeakRef, Object);
//	}
//
//	void BeginDestroy()
//	{
//		// If we still have a reference we flag it as marked for deletion.
//		// and drop the shared ref, we use the weak reference to tell when it's safe to delete
//		if (SharedRef)
//		{
//			SharedRef->MarkForCollection();
//			SharedRef.Reset();
//		}
//	}
//
//	bool IsFree() 
//	{
//		if (WeakRef.IsValid())
//		{
//			return false;
//		}
//
//		for (int32 Index = OldRefs.Num() - 1; Index >= 0; Index--)
//		{
//			if (OldRefs[Index].IsValid())
//			{
//				return false;
//			}
//			else
//			{
//				// Clean stale refs
//				OldRefs.RemoveAtSwap(Index);
//			}
//		}
//
//		return true;
//	}
//
//};
//
//
//
//template<typename ObjectType>
//struct FRuntimeMeshReferenceAliasAnchor
//{
//	using FReferencer = FRuntimeMeshReferencer;
//
//private:
//	TWeakPtr<FReferencer, ESPMode::ThreadSafe> WeakRef;
//	ObjectType* Object;
//public:
//	FRuntimeMeshReferenceAliasAnchor()
//	{
//	}
//	FRuntimeMeshReferenceAliasAnchor(ObjectType* InObject)
//		: Object(InObject)
//	{
//	}
//
//	template<typename OtherType>
//	void BindToAssociated(const FRuntimeMeshObjectWeakRef<OtherType>& OtherAnchor)
//	{
//		check(!WeakRef.IsValid());
//
//		FRuntimeMeshObjectSharedRef<OtherType> OtherAnchorPinned = OtherAnchor.Pin();
//		if (OtherAnchorPinned)
//		{
//			WeakRef = OtherAnchorPinned.Ref;
//		}
//	}
//
//	FRuntimeMeshObjectWeakRef<ObjectType> GetReference()
//	{
//		return FRuntimeMeshObjectWeakRef<ObjectType>(WeakRef, Object);
//	}
//
//	bool IsFree()
//	{
//		return !WeakRef.IsValid();
//// 		TSharedPtr<FReferencer, ESPMode::ThreadSafe> Pinned = WeakRef.Pin();
//// 		if (Pinned.IsValid() && !Pinned->MarkedForDeletion && Pinned.GetSharedReferenceCount() == 2)
//// 		{
//// 			// Special case for editor where we get marked for deletion, but the parent mesh hasn't been
//// 			// So only we and the RM should have a shared ref but it's not marked for deletion
//// 			return true;
//// 		}
//// 
//// 		return !Pinned.IsValid();
//	}
//
//};
//
//template<typename ObjectType>
//struct FRuntimeMeshObjectSharedRef
//{
//	using FReferencer = FRuntimeMeshReferencer;
//private:
//	TSharedPtr<FReferencer, ESPMode::ThreadSafe> Ref;
//	ObjectType* Object;
//
//	FORCEINLINE FRuntimeMeshObjectSharedRef(const TSharedPtr<FReferencer, ESPMode::ThreadSafe>& InRef, ObjectType* InObject)
//		: Ref(InRef), Object(InObject) 
//	{ 
//		if (!Ref.IsValid())
//		{
//			Object = nullptr;
//		}
//	}
//
//public:
//	FORCEINLINE FRuntimeMeshObjectSharedRef() { }
//	~FRuntimeMeshObjectSharedRef()
//	{
//		if (IsValid())
//		{
//			Ref.Reset();
//		}
//	}
//
//	FORCEINLINE FRuntimeMeshObjectSharedRef(const FRuntimeMeshObjectSharedRef<ObjectType>& InRef)
//		: Ref(InRef.Ref), Object(InRef.Object)
//	{
//		if (!Ref.IsValid())
//		{
//			Object = nullptr;
//		}
//	}
//
//	FORCEINLINE FRuntimeMeshObjectSharedRef(FRuntimeMeshObjectSharedRef<ObjectType>&& InRef)
//		: Ref(MoveTemp(InRef.Ref)), Object(MoveTemp(InRef.Object))
//	{
//		
//	}
//
//
//	FORCEINLINE FRuntimeMeshObjectSharedRef& operator=(const FRuntimeMeshObjectSharedRef<ObjectType>& InRef)
//	{
//		Ref = InRef.Ref;
//		Object = InRef.Object;
//
//		if (!Ref.IsValid())
//		{
//			Object = nullptr;
//		}
//		return *this;
//	}
//
//	FORCEINLINE FRuntimeMeshObjectSharedRef& operator=(FRuntimeMeshObjectSharedRef<ObjectType>&& InRef)
//	{
//		Ref = MoveTemp(InRef.Ref);
//		Object = MoveTemp(InRef.Object);
//		return *this;
//	}
//
//	/**
//	* Returns the object referenced by this pointer, or nullptr if no object is reference
//	*
//	* @return  The object owned by this shared pointer, or nullptr
//	*/
//	FORCEINLINE ObjectType* Get() const
//	{
//		return Object;
//	}
//
//	/**
//	* Checks to see if this shared pointer is actually pointing to an object
//	*
//	* @return  True if the shared pointer is valid and can be dereferenced
//	*/
//	FORCEINLINE explicit operator bool() const
//	{
//		return Ref.IsValid();
//	}
//
//	/**
//	* Checks to see if this shared pointer is actually pointing to an object
//	*
//	* @return  True if the shared pointer is valid and can be dereferenced
//	*/
//	FORCEINLINE const bool IsValid() const
//	{
//		return Ref.IsValid();
//	}
//
//	/**
//	* Dereference operator returns a reference to the object this shared pointer points to
//	*
//	* @return  Reference to the object
//	*/
//	FORCEINLINE typename FMakeReferenceTo<ObjectType>::Type operator*() const
//	{
//		check(IsValid());
//		return *Object;
//	}
//
//	/**
//	* Arrow operator returns a pointer to the object this shared pointer references
//	*
//	* @return  Returns a pointer to the object referenced by this shared pointer
//	*/
//	FORCEINLINE ObjectType* operator->() const
//	{
//		check(IsValid());
//		return Object;
//	}
//
//	/**
//	* Resets this shared pointer, removing a reference to the object.  If there are no other shared
//	* references to the object then it will be destroyed.
//	*/
//	FORCEINLINE void Reset()
//	{
//		if (Ref.IsValid())
//		{
//			Ref.Reset();
//		}
//	}
//
//	friend struct FRuntimeMeshObjectWeakRef<ObjectType>;
//
//	template<typename OtherObjectType>
//	friend struct FRuntimeMeshReferenceAnchor;
//	template<typename OtherObjectType>
//	friend struct FRuntimeMeshReferenceAliasAnchor;
//};
//
//template<typename ObjectType>
//struct FRuntimeMeshObjectWeakRef
//{
//	using FReferencer = FRuntimeMeshReferencer;
//private:
//	TWeakPtr<FReferencer, ESPMode::ThreadSafe> Ref;	
//	ObjectType* Object;
//	
//	FORCEINLINE FRuntimeMeshObjectWeakRef(const TWeakPtr<FReferencer, ESPMode::ThreadSafe>& InRef, ObjectType* InObject)
//		: Ref(InRef), Object(InObject) { }
//
//public:
//	FORCEINLINE FRuntimeMeshObjectWeakRef() { }
//
//	FORCEINLINE FRuntimeMeshObjectWeakRef(const FRuntimeMeshObjectSharedRef<ObjectType>& InRef)
//		: Ref(InRef.Ref), Object(InRef.Object) { }
//
//	FORCEINLINE FRuntimeMeshObjectWeakRef(const FRuntimeMeshObjectWeakRef<ObjectType>& InRef)
//		: Ref(InRef.Ref), Object(InRef.Object) { }
//
//	FORCEINLINE FRuntimeMeshObjectWeakRef(FRuntimeMeshObjectWeakRef<ObjectType>&& InRef)
//		: Ref(MoveTemp(InRef.Ref)), Object(MoveTemp(InRef.Object)) { }
//
//	FORCEINLINE FRuntimeMeshObjectWeakRef& operator=(const FRuntimeMeshObjectSharedRef<ObjectType>& InRef)
//	{
//		Ref = InRef.Ref;
//		Object = InRef.Object;
//		return *this;
//	}
//
//	FORCEINLINE FRuntimeMeshObjectWeakRef& operator=(const FRuntimeMeshObjectWeakRef<ObjectType>& InRef)
//	{
//		Ref = MoveTemp(InRef.Ref);
//		Object = MoveTemp(InRef.Object);
//		return *this;
//	}
//
//	FORCEINLINE FRuntimeMeshObjectWeakRef& operator=(FRuntimeMeshObjectWeakRef<ObjectType>&& InRef)
//	{
//		Ref = InRef.Ref;
//		Object = InRef.Object;
//		return *this;
//	}
//
//	
//	FORCEINLINE FRuntimeMeshObjectSharedRef<ObjectType> Pin(bool bShouldIgnoreGCMark = false) const
//	{
//		FRuntimeMeshObjectSharedRef<ObjectType> NewRef(Ref.Pin(), Object);
//
//		if (!bShouldIgnoreGCMark && NewRef.Ref.IsValid() && NewRef.Ref->MarkedForDeletion)
//		{
//			NewRef.Ref.Reset();
//		}
//
//		return NewRef;
//	}
//
//	FORCEINLINE const bool IsValid(bool bShouldCheckGCMark = false) const
//	{
//		if (bShouldCheckGCMark)
//		{
//			TSharedPtr<FRuntimeMeshReferencer, ESPMode::ThreadSafe> TestRef = Ref.Pin();
//
//			return TestRef.IsValid() && !TestRef->MarkedForDeletion;
//		}
//		return Ref.IsValid();
//	}
//
//	FORCEINLINE void Reset()
//	{
//		Ref.Reset();
//	}
//
//	friend struct FRuntimeMeshObjectSharedRef<ObjectType>;
//
//	template<typename OtherObjectType>
//	friend struct FRuntimeMeshReferenceAnchor;
//	template<typename OtherObjectType>
//	friend struct FRuntimeMeshReferenceAliasAnchor;
//};

