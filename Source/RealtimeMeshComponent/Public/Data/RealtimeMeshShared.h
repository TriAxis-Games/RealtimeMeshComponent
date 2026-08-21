// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "RealtimeMeshCore.h"
#include "RealtimeMeshGuard.h"
#include "Core/RealtimeMeshKeys.h"
#include "Core/RealtimeMeshDataStream.h"
#include "Async/Async.h"

struct FRealtimeMeshSimpleGeometry;
struct FRealtimeMeshCollisionConfiguration;
struct FRealtimeMeshCollisionInfo;
enum class ERealtimeMeshCollisionUpdateResult : uint8;

namespace RealtimeMesh
{
	struct FRealtimeMeshUpdateContext;

	// DUP-027: Shared KeyFuncs for TSet<TSharedRef<InElementType>> indexed by a key retrieved
	// via InElementType::GetKey_AssumesLocked(). The owning element type must befriend the
	// concrete specialization so this template can read its (protected) key accessor.
	template <typename InElementType, typename InKeyType>
	struct TRealtimeMeshRefKeyFuncs : BaseKeyFuncs<TSharedRef<InElementType>, InKeyType, false>
	{
		using ElementInitType = typename BaseKeyFuncs<TSharedRef<InElementType>, InKeyType, false>::ElementInitType;
		using KeyInitType = typename BaseKeyFuncs<TSharedRef<InElementType>, InKeyType, false>::KeyInitType;

		/**
		 * @return The key used to index the given element.
		 */
		static KeyInitType GetSetKey(ElementInitType Element)
		{
			return Element->GetKey_AssumesLocked();
		}

		/**
		 * @return True if the keys match.
		 */
		static bool Matches(KeyInitType A, KeyInitType B)
		{
			return A == B;
		}

		/** Calculates a hash index for a key. */
		static uint32 GetKeyHash(KeyInitType Key)
		{
			return GetTypeHash(Key);
		}
	};

	// ReSharper disable CppExpressionWithoutSideEffects
	struct FRealtimeMeshBounds
	{
	private:
		TOptional<FBoxSphereBounds3f> UserSetBounds;
		TOptional<FBoxSphereBounds3f> CalculatedBounds;

	public:
		bool HasUserSetBounds() const { return UserSetBounds.IsSet(); }
		void SetUserSetBounds(const FBoxSphereBounds3f& InBounds) { UserSetBounds = InBounds; }
		void ClearUserSetBounds() { UserSetBounds.Reset(); }

		bool HasComputedBounds() const { return CalculatedBounds.IsSet(); }
		void SetComputedBounds(const FBoxSphereBounds3f& InBounds) { CalculatedBounds = InBounds; }
		void ClearCachedValue() { CalculatedBounds.Reset(); }

		bool HasBounds() const { return UserSetBounds.IsSet() || CalculatedBounds.IsSet(); }
		const FBoxSphereBounds3f& GetBounds() const { check(HasBounds()); return UserSetBounds.IsSet()? UserSetBounds.GetValue() : CalculatedBounds.GetValue(); }

		const FBoxSphereBounds3f& GetComputedBounds() const { check(HasComputedBounds()); return CalculatedBounds.GetValue(); }

		TOptional<FBoxSphereBounds3f> Get() const { return UserSetBounds.IsSet() ? UserSetBounds : CalculatedBounds; }
		
		void Reset()
		{
			UserSetBounds.Reset();
			CalculatedBounds.Reset();
		}

		friend FArchive& operator<<(FArchive& Ar, FRealtimeMeshBounds& Bounds)
		{
			if (Ar.CustomVer(FRealtimeMeshVersion::GUID) >= FRealtimeMeshVersion::DataRestructure)
			{
				Ar << Bounds.UserSetBounds;
				Ar << Bounds.CalculatedBounds;
			}
			else
			{
				FBoxSphereBounds3f TempBounds;
				Ar << TempBounds;
				Bounds.CalculatedBounds = TempBounds;
			}
			return Ar;
		}

	};
	// ReSharper restore CppExpressionWithoutSideEffects


	// DUP-008: shared accumulate-hull of child bounds. FRealtimeMesh (over its LODs),
	// FRealtimeMeshLOD (over its section groups), and FRealtimeMeshBufferSet (over its
	// sections) folded their children's bounds with a verbatim-identical loop in their
	// FinalizeUpdate paths. The iteration order and the left-folded `+` sequence are
	// preserved exactly (the first set child seeds, each subsequent set child is added as
	// `*NewBounds + *ChildBounds`, unset children are skipped) so the accumulated
	// floating-point bounds stay bit-identical to the pre-collapse loops.
	template <typename RangeType, typename ProjectionType>
	TOptional<FBoxSphereBounds3f> AccumulateBounds(const RangeType& Elements, ProjectionType Projection)
	{
		TOptional<FBoxSphereBounds3f> NewBounds;
		for (const auto& Element : Elements)
		{
			const TOptional<FBoxSphereBounds3f> ElementBounds = Projection(Element);
			if (ElementBounds.IsSet())
			{
				if (!NewBounds.IsSet())
				{
					NewBounds = *ElementBounds;
					continue;
				}
				NewBounds = *NewBounds + *ElementBounds;
			}
		}
		return NewBounds;
	}

	// DUP-008: separate overload for the hull variant used by
	// FRealtimeMeshBufferSet::GetInUseRange. Unlike AccumulateBounds this folds plain
	// (never-unset) stream ranges with `Hull` rather than `+`, so it is kept distinct per
	// the §3 duplication map. The first element seeds; each subsequent element is folded as
	// `NewRange->Hull(Projection(Element))`, preserving the exact call order and fold shape.
	template <typename RangeType, typename ProjectionType>
	TOptional<FRealtimeMeshStreamRange> AccumulateStreamRangeHull(const RangeType& Elements, ProjectionType Projection)
	{
		TOptional<FRealtimeMeshStreamRange> NewRange;
		for (const auto& Element : Elements)
		{
			if (!NewRange.IsSet())
			{
				NewRange = Projection(Element);
				continue;
			}
			NewRange = NewRange->Hull(Projection(Element));
		}
		return NewRange;
	}


	enum class ERealtimeMeshChangeType
	{
		Unknown,
		Added,
		Updated,
		Removed
	};

	DECLARE_MULTICAST_DELEGATE_ThreeParams(FRealtimeMeshStreamChangedEvent, const FRealtimeMeshBufferSetKey&, const FRealtimeMeshStreamKey&, ERealtimeMeshChangeType);
	DECLARE_MULTICAST_DELEGATE_TwoParams(FRealtimeMeshStreamPropertyChangedEvent, const FRealtimeMeshBufferSetKey&, const FRealtimeMeshStreamKey&);

	DECLARE_MULTICAST_DELEGATE_TwoParams(FRealtimeMeshSectionChangedEvent, const FRealtimeMeshSectionKey&, ERealtimeMeshChangeType);
	DECLARE_MULTICAST_DELEGATE_OneParam(FRealtimeMeshSectionPropertyChangedEvent, const FRealtimeMeshSectionKey&);

	DECLARE_MULTICAST_DELEGATE_TwoParams(FRealtimeMeshSectionGroupChangedEvent, const FRealtimeMeshBufferSetKey&, ERealtimeMeshChangeType);
	DECLARE_MULTICAST_DELEGATE_OneParam(FRealtimeMeshSectionGroupPropertyChangedEvent, const FRealtimeMeshBufferSetKey&);

	DECLARE_MULTICAST_DELEGATE_TwoParams(FRealtimeMeshLODChangedEvent, const FRealtimeMeshLODKey&, ERealtimeMeshChangeType);
	DECLARE_MULTICAST_DELEGATE_OneParam(FRealtimeMeshLODPropertyChangedEvent, const FRealtimeMeshLODKey&);

	DECLARE_MULTICAST_DELEGATE(FRealtimeMeshPropertyChangedEvent);
	DECLARE_MULTICAST_DELEGATE_TwoParams(FRealtimeMeshRenderDataChangedEvent, bool, int32);


	DECLARE_DELEGATE(FRealtimeMeshRequestEndOfFrameUpdateDelegate);
	DECLARE_DELEGATE_ThreeParams(FRealtimeMeshCollisionUpdateDelegate, const TSharedRef<TPromise<ERealtimeMeshCollisionUpdateResult>>&,
	                             const TSharedRef<FRealtimeMeshCollisionInfo>&, bool);

	DECLARE_MULTICAST_DELEGATE(FRealtimeMeshSimpleEvent);

	/**
	 * Broadcast by FRealtimeMeshUpdateContext::Commit after FinalizeUpdate, while the mesh
	 * write lock is still held, on whichever thread committed the batch. The update context's
	 * GetState() dirty trees describe exactly what this batch touched. Subscribers must copy
	 * out whatever they need and return quickly: no mesh write/edit calls, no blocking, no
	 * assumption of the game thread. Used by optional layers (e.g. net sync) to observe
	 * per-LOD/section-group/stream change granularity that is otherwise discarded.
	 */
	DECLARE_MULTICAST_DELEGATE_OneParam(FRealtimeMeshUpdateCommittedEvent, FRealtimeMeshUpdateContext&);

	/**
	 * Per-mesh shared state: lock, name, weak back-pointers to the UObject layer
	 * and the render-thread proxy, and the mesh-level event broker. No factory
	 * methods — type-tier construction lives on FRealtimeMesh (data side) and
	 * FRealtimeMeshProxy (render side), where the leaf type info actually exists.
	 *
	 * Holds no virtuals; concrete leaves don't subclass this.
	 */
	class REALTIMEMESHCOMPONENT_API FRealtimeMeshContext : public TSharedFromThis<FRealtimeMeshContext>
	{
		mutable FRealtimeMeshGuard Guard;
		FName MeshName;

		TWeakObjectPtr<URealtimeMesh> OwningMesh;
		FRealtimeMeshWeakPtr Owner;
		FRealtimeMeshProxyWeakPtr Proxy;

		FRealtimeMeshSimpleEvent OnRenderProxyRequiresUpdateEvent;
		FRealtimeMeshSimpleEvent OnBoundsChangedEvent;
		FRealtimeMeshUpdateCommittedEvent OnUpdateCommittedEvent;

	public:
		~FRealtimeMeshContext() = default;

		FRealtimeMeshContext()
		{
		}

		void SetOwnerMesh(URealtimeMesh* InOwningMesh, const FRealtimeMeshRef& InOwner);
		void SetProxy(const FRealtimeMeshProxyRef& InProxy) { Proxy = InProxy; }

		FRealtimeMeshGuard& GetGuard() const { return Guard; }
		FName GetMeshName() const { return MeshName; }
		void SetMeshName(FName InName) { MeshName = InName; }

		URealtimeMesh* GetOwningMesh() const { return OwningMesh.Get(); }
		FRealtimeMeshPtr GetOwner() const { return Owner.Pin(); }
		FRealtimeMeshProxyPtr GetProxy() const { return Proxy.Pin(); }

		ERHIFeatureLevel::Type GetFeatureLevel() const;

		bool WantsStreamOnGPU(const FRealtimeMeshStreamKey& StreamKey) const
		{
			static const TSet WantedStreams =
			{
				FRealtimeMeshStreams::Position,
				FRealtimeMeshStreams::Tangents,
				FRealtimeMeshStreams::TexCoords,
				FRealtimeMeshStreams::Color,
				FRealtimeMeshStreams::Triangles
			};
			return WantedStreams.Contains(StreamKey);
		}


		FRealtimeMeshSimpleEvent& OnRenderProxyRequiresUpdate() { return OnRenderProxyRequiresUpdateEvent; }
		FRealtimeMeshSimpleEvent& OnBoundsChanged() { return OnBoundsChangedEvent; }
		FRealtimeMeshUpdateCommittedEvent& OnUpdateCommitted() { return OnUpdateCommittedEvent; }


		// Non-virtual forwarders. The polymorphic dispatch lives on FRealtimeMesh
		// (data side) and FRealtimeMeshProxy (render side); these are thin
		// conveniences so children can still write `Context->CreateX(K)`
		// without manually pinning the back-reference. All cold-path; the Pin
		// cost is negligible vs. the construction itself.
		FRealtimeMeshSectionRef     CreateSection(const FRealtimeMeshSectionKey& InKey) const;
		FRealtimeMeshSectionGroupRef CreateSectionGroup(const FRealtimeMeshBufferSetKey& InKey) const;
		FRealtimeMeshLODRef         CreateLOD(const FRealtimeMeshLODKey& InKey) const;
		FRealtimeMeshUpdateStateRef CreateUpdateState() const;

		FRealtimeMeshSectionProxyRef     CreateSectionProxy(const FRealtimeMeshSectionKey& InKey) const;
		FRealtimeMeshSectionGroupProxyRef CreateSectionGroupProxy(const FRealtimeMeshBufferSetKey& InKey) const;
		FRealtimeMeshLODProxyRef         CreateLODProxy(const FRealtimeMeshLODKey& InKey) const;
		FRealtimeMeshVertexFactoryRef    CreateVertexFactory() const;

	};
}
