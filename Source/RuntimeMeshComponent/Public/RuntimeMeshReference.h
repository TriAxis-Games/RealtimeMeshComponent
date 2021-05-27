// Copyright 2016-2020 TriAxis Games L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HAL/PlatformAtomics.h"
#include "HAL/ThreadSafeBool.h"
#include "Templates/SharedPointer.h"
#include "Templates/SharedPointerInternals.h"

template<typename ObjectType>
struct FRuntimeMeshReference;
template<typename ObjectType>
struct FRuntimeMeshPinnedReference;
template<typename ObjectType>
struct FRuntimeMeshReferenceAnchor;


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


	static FORCEINLINE const void AddReference(FReferencer* Reference)
	{
		FPlatformAtomics::InterlockedIncrement(&Reference->ReferenceCount);
	}

	static FORCEINLINE const void ReleaseReference(FReferencer* Reference)
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
struct FRuntimeMeshReference
{
private:
	using TOps = FRuntimeMeshReferenceOps<ObjectType>;

	FRuntimeMeshReferenceObject<ObjectType>* Controller;

	FORCEINLINE FRuntimeMeshReference(FRuntimeMeshReferenceObject<ObjectType>* InController)
		: Controller(InController)
	{
		if (Controller != nullptr)
		{
			TOps::AddReference(Controller);
		}
	}

public:

	FORCEINLINE FRuntimeMeshReference()
		: Controller(nullptr)
	{

	}

	FORCEINLINE FRuntimeMeshReference(const FRuntimeMeshReference& InReference)
		: Controller(InReference.Controller)
	{
		if (Controller != nullptr)
		{
			TOps::AddReference(Controller);
		}
	}

	FORCEINLINE FRuntimeMeshReference(FRuntimeMeshReference&& InReference)
		: Controller(InReference.Controller)
	{
		InReference.Controller = nullptr;
	}

	FORCEINLINE FRuntimeMeshReference& operator=(const FRuntimeMeshReference& InReference)
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

	FORCEINLINE FRuntimeMeshReference& operator=(FRuntimeMeshReference&& InReference)
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

	FORCEINLINE ~FRuntimeMeshReference()
	{
		if (Controller != nullptr)
		{
			TOps::ReleaseReference(Controller);
		}
	}

	FORCEINLINE FRuntimeMeshPinnedReference<ObjectType> Pin() const
	{
		return FRuntimeMeshReference<ObjectType>(*this);
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

	friend uint32 GetTypeHash(const FRuntimeMeshReference& InReference)
	{
		ObjectType* Ptr = InReference.Controller ? InReference.Controller->Object : nullptr;
		return ::PointerHash(Ptr);
	}

	friend struct FRuntimeMeshPinnedReference<ObjectType>;
	friend struct FRuntimeMeshReferenceAnchor<ObjectType>;
};


template<typename ObjectType>
struct FRuntimeMeshPinnedReference
{
private:
	using TOps = FRuntimeMeshReferenceOps<ObjectType>;

	FRuntimeMeshReferenceObject<ObjectType>* Controller;
	ObjectType* Object;

	FORCEINLINE FRuntimeMeshPinnedReference(const FRuntimeMeshReference<ObjectType>& InReference)
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

	FORCEINLINE FRuntimeMeshPinnedReference(const FRuntimeMeshPinnedReference& InReference)
		: Controller(InReference.Controller)
		, Object(nullptr)
	{
		if (Controller != nullptr)
		{
			TOps::AddPinnedReference(Controller);
			Object = Controller->Object;
		}
	}

	FORCEINLINE FRuntimeMeshPinnedReference& operator=(const FRuntimeMeshPinnedReference& InReference)
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

	FORCEINLINE FRuntimeMeshPinnedReference& operator=(FRuntimeMeshPinnedReference&& InReference)
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




	FORCEINLINE ~FRuntimeMeshPinnedReference()
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

	friend uint32 GetTypeHash(const FRuntimeMeshPinnedReference<ObjectType>& InReference)
	{
		ObjectType* Ptr = InReference.Controller ? InReference.Controller->Object : nullptr;
		return ::PointerHash(Ptr);
	}

	friend struct FRuntimeMeshReference<ObjectType>;
	friend struct FRuntimeMeshReferenceAnchor<ObjectType>;
};


template<typename ObjectType>
struct FRuntimeMeshReferenceAnchor
{
private:
	using TOps = FRuntimeMeshReferenceOps<ObjectType>;
	FRuntimeMeshReferenceObject<ObjectType>* CurrentReferenceController;
	TArray<FRuntimeMeshReferenceObject<ObjectType>*> WeakRefs;
public:
	FRuntimeMeshReferenceAnchor()
		: CurrentReferenceController(nullptr)
	{
	}
	FRuntimeMeshReferenceAnchor(ObjectType* InObject)
		: CurrentReferenceController(new FRuntimeMeshReferenceObject<ObjectType>(InObject))
	{
	}

	void BeginNewState()
	{
		if (CurrentReferenceController)
		{
			ObjectType* Object = CurrentReferenceController->Object;
			check(Object);

			// Destroy the old ref
			BeginDestroy();

			CurrentReferenceController = new FRuntimeMeshReferenceObject<ObjectType>(Object);
		}
	}

	FRuntimeMeshReference<ObjectType> GetReference()
	{
		return FRuntimeMeshReference<ObjectType>(CurrentReferenceController);
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

using FRuntimeMeshSharedRef = FRuntimeMeshPinnedReference<URuntimeMesh>;
using FRuntimeMeshWeakRef = FRuntimeMeshReference<URuntimeMesh>;

using FRuntimeMeshProviderSharedRef = FRuntimeMeshPinnedReference<URuntimeMeshProvider>;
using FRuntimeMeshProviderWeakRef = FRuntimeMeshReference<URuntimeMeshProvider>;
