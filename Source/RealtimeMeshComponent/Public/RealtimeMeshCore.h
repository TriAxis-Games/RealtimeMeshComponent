// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Runtime/Launch/Resources/Version.h"
#include "StaticMeshResources.h"
#include "Core/RealtimeMeshCoreFwd.h"

DECLARE_STATS_GROUP(TEXT("RealtimeMesh"), STATGROUP_RealtimeMesh, STATCAT_Advanced);

namespace RealtimeMesh
{
	// Custom version for runtime mesh serialization
	namespace FRealtimeMeshVersion
	{
		enum Type
		{
			Initial = 0,
			StreamsNowHoldEntireKey = 1,
			DataRestructure = 2,
			CollisionUpdateFlowRestructure = 3,
			StreamKeySizeChanged = 4,
			RemovedNamedStreamElements = 5,
			SimpleMeshStoresCollisionConfig = 6,
			ImprovingDataTypes = 7,
			SimpleMeshStoresCustomComplexCollision = 8,
			DistanceFieldAndCardRepresentationSupport = 9,
			SupportOptionalDataSerialization = 10,
			CollisionOverhaul = 11,
			DrawTypeMovedToSectionGroup = 12,
			ActorSupportsOptionalConstructionDefer = 13,
			SectionGroupComputeWritable = 14,
			SerializeSectionKeyBufferSlotAndStreamType = 15,
			SerializeSectionGroupKeySlotIndex = 16,

			// -----<new versions can be added above this line>-------------------------------------------------
			VersionPlusOne,
			LatestVersion = VersionPlusOne - 1
		};

		// The GUID for this custom version
		const static FGuid GUID = FGuid(0xAF3DD80C, 0x4C114B25, 0x9C7A9515, 0x5062D6E9);
	}



	/** Deleter function for TSharedPtrs that only allows the object to be destructed on the render thread. */
	template <typename Type>
	struct FRealtimeMeshRenderThreadDeleter
	{
		void operator()(Type* Object) const
		{
			if (IsInRenderingThread())
			{
				delete Object;
			}
			else
			{
				ENQUEUE_RENDER_COMMAND(FRealtimeMeshProxyDeleterCommand)(
					[Object](FRHICommandListImmediate& RHICmdList)
					{
						delete static_cast<Type*>(Object);
					}
				);
			}
		}
	};

	/**
	 * Deleter for FRenderResource subclasses managed by TSharedPtr in the COW snapshot
	 * chain. FRenderResource asserts at destruction if it was initialized but never
	 * released, so we must call ReleaseResource on the render thread before deleting.
	 * Because resources are shared across snapshots, only the final-reference drop
	 * (this deleter) knows it's safe to release.
	 */
	template <typename Type>
	struct FRealtimeMeshRenderResourceDeleter
	{
		void operator()(Type* Object) const
		{
			auto ReleaseAndDelete = [Object]()
			{
				if (Object && Object->IsInitialized())
				{
					Object->ReleaseResource();
				}
				delete Object;
			};

			if (IsInRenderingThread())
			{
				ReleaseAndDelete();
			}
			else
			{
				ENQUEUE_RENDER_COMMAND(FRealtimeMeshRenderResourceDeleterCommand)(
					[ReleaseAndDelete = MoveTemp(ReleaseAndDelete)](FRHICommandListImmediate& RHICmdList) mutable
					{
						ReleaseAndDelete();
					}
				);
			}
		}
	};

	/**
	 * Copy-on-write smart pointer for nodes stored in the COW snapshot tree.
	 *
	 * Holds a TSharedPtr<T> internally and exposes two distinct access modes:
	 *   - Read access via Read() / operator-> / operator*    returns `const T&`/`const T*`.
	 *     Cannot mutate the underlying object, regardless of whether it's shared
	 *     with a published snapshot or not. The compiler enforces this.
	 *   - Write access via Write()                            triggers COW if shared.
	 *     If the underlying TSharedPtr has any other holder (typically a published
	 *     snapshot that's still alive), Write() clones the object via T::Clone()
	 *     and rebinds this TCowPtr to the clone. After Write() returns the local
	 *     pointer is guaranteed unique and the returned T& is safe to mutate.
	 *
	 * T must expose a `Clone()` method returning `TSharedRef<T>` (the existing
	 * convention for RMC proxy node types). The TCowPtr does not itself track
	 * "was this cloned this batch" — that's still the parent collection's job
	 * via its TouchedXxxIndices set. TCowPtr's responsibility is the COW
	 * mechanic at one slot, plus making it impossible to accidentally mutate
	 * shared state.
	 */
	template <typename T>
	class TCowPtr
	{
	public:
		TCowPtr() = default;
		TCowPtr(std::nullptr_t) {}
		TCowPtr(const TSharedPtr<T>& InPtr) : Ptr(InPtr) {}
		TCowPtr(TSharedPtr<T>&& InPtr) : Ptr(MoveTemp(InPtr)) {}
		TCowPtr(const TSharedRef<T>& InRef) : Ptr(InRef) {}

		bool IsValid() const { return Ptr.IsValid(); }
		explicit operator bool() const { return IsValid(); }

		// ---- Read access — const view ----
		const T& Read() const { check(IsValid()); return *Ptr; }
		const T* operator->() const { return Ptr.Get(); }
		const T& operator*() const { check(IsValid()); return *Ptr; }
		const T* Get() const { return Ptr.Get(); }

		// ---- Write access — clones if shared ----
		// Returns a mutable reference to the underlying object. If anyone else
		// (typically a published snapshot's LOD/buffer-set/section ref) is
		// holding the same TSharedPtr, this clones the object first so the
		// mutation does not leak into that snapshot.
		T& Write()
		{
			check(IsValid());
			if (Ptr.GetSharedReferenceCount() > 1)
			{
				Ptr = TSharedPtr<T>(Ptr->Clone());
			}
			return *Ptr;
		}

		// True iff a Write() call right now would NOT clone (i.e. nobody else
		// is holding the underlying pointer). After a publish this is false
		// for snapshot-shared slots and true for slots that were COW'd during
		// the current batch.
		bool IsUnique() const { return Ptr.IsValid() && Ptr.IsUnique(); }

		// Mutable access that NEVER clones. The caller asserts (and this checks)
		// that the slot is already unique. Unlike Write(), this does not re-test
		// the refcount, so it cannot clone-and-rebind if another holder appeared
		// after a prior IsUnique() check — closing the TOCTOU window on the
		// in-place fast path (a concurrent GT reader must never see this slot
		// mutated or the underlying pointer swapped).
		T* GetUniqueUnchecked()
		{
			check(IsUnique());
			return Ptr.Get();
		}

		void Reset() { Ptr.Reset(); }

		// Const view as a TSharedPtr<const T> — handy when callers need to pass
		// the underlying ref through APIs that take TSharedPtr.
		TSharedPtr<const T> ToSharedPtrConst() const { return Ptr; }

		bool operator==(const TCowPtr& Other) const { return Ptr == Other.Ptr; }
		bool operator!=(const TCowPtr& Other) const { return Ptr != Other.Ptr; }

	private:
		TSharedPtr<T> Ptr;
	};


#define CREATE_RMC_PTR_TYPES(TypeName) \
	using TypeName##Ref = TSharedRef<TypeName, ESPMode::ThreadSafe>; \
	using TypeName##Ptr = TSharedPtr<TypeName, ESPMode::ThreadSafe>; \
	using TypeName##WeakPtr = TWeakPtr<TypeName, ESPMode::ThreadSafe>; \
	using TypeName##ConstRef = TSharedRef<const TypeName, ESPMode::ThreadSafe>; \
	using TypeName##ConstPtr = TSharedPtr<const TypeName, ESPMode::ThreadSafe>; \
	using TypeName##ConstWeakPtr = TWeakPtr<const TypeName, ESPMode::ThreadSafe>;

	struct FRealtimeMeshGPUUpdateBuilder;

	struct FRealtimeMeshStream;

	struct FRealtimeMeshUpdateState;
	CREATE_RMC_PTR_TYPES(FRealtimeMeshUpdateState);
	
	class FRealtimeMeshVertexFactory;
	CREATE_RMC_PTR_TYPES(FRealtimeMeshVertexFactory);

	class FRealtimeMeshBufferSetProxy;
	CREATE_RMC_PTR_TYPES(FRealtimeMeshBufferSetProxy);

	class FRealtimeMeshSectionProxy;
	CREATE_RMC_PTR_TYPES(FRealtimeMeshSectionProxy);

	class FRealtimeMeshLODProxy;
	CREATE_RMC_PTR_TYPES(FRealtimeMeshLODProxy);

	class FRealtimeMeshProxy;
	CREATE_RMC_PTR_TYPES(FRealtimeMeshProxy);

	class FRealtimeMeshContext;
	CREATE_RMC_PTR_TYPES(FRealtimeMeshContext);

	class FRealtimeMeshBufferSet;
	CREATE_RMC_PTR_TYPES(FRealtimeMeshBufferSet);

	class FRealtimeMeshSection;
	CREATE_RMC_PTR_TYPES(FRealtimeMeshSection);

	class FRealtimeMeshLOD;
	CREATE_RMC_PTR_TYPES(FRealtimeMeshLOD);

	class FRealtimeMesh;
	CREATE_RMC_PTR_TYPES(FRealtimeMesh);

#undef CREATE_RMC_PTR_TYPES

	// Back-compat aliases for the previous "SectionGroup" class names. New code
	// should use the FRealtimeMeshBufferSet* names; the old aliases keep external
	// callers and existing user code compiling without modification.
	using FRealtimeMeshSectionGroupProxy             = FRealtimeMeshBufferSetProxy;
	using FRealtimeMeshSectionGroupProxyRef          = FRealtimeMeshBufferSetProxyRef;
	using FRealtimeMeshSectionGroupProxyPtr          = FRealtimeMeshBufferSetProxyPtr;
	using FRealtimeMeshSectionGroupProxyWeakPtr      = FRealtimeMeshBufferSetProxyWeakPtr;
	using FRealtimeMeshSectionGroupProxyConstRef     = FRealtimeMeshBufferSetProxyConstRef;
	using FRealtimeMeshSectionGroupProxyConstPtr     = FRealtimeMeshBufferSetProxyConstPtr;
	using FRealtimeMeshSectionGroupProxyConstWeakPtr = FRealtimeMeshBufferSetProxyConstWeakPtr;

	using FRealtimeMeshSectionGroup                  = FRealtimeMeshBufferSet;
	using FRealtimeMeshSectionGroupRef               = FRealtimeMeshBufferSetRef;
	using FRealtimeMeshSectionGroupPtr               = FRealtimeMeshBufferSetPtr;
	using FRealtimeMeshSectionGroupWeakPtr           = FRealtimeMeshBufferSetWeakPtr;
	using FRealtimeMeshSectionGroupConstRef          = FRealtimeMeshBufferSetConstRef;
	using FRealtimeMeshSectionGroupConstPtr          = FRealtimeMeshBufferSetConstPtr;
	using FRealtimeMeshSectionGroupConstWeakPtr      = FRealtimeMeshBufferSetConstWeakPtr;

	// DUP-015: TFixedLODArray alias-template is defined once in Core/RealtimeMeshCoreFwd.h
	// (included above), so it is intentionally not redefined here.

	class FRealtimeMeshGPUBuffer;
}


class URealtimeMesh;
class URealtimeMeshComponent;
