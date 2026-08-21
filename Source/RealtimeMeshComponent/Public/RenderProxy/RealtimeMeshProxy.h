// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "RealtimeMeshComponentProxy.h"
#include "RealtimeMeshCore.h"
#include "RealtimeMeshProxyCommandBatch.h"
#include "RealtimeMeshProxyShared.h"
#include "HAL/ThreadSafeBool.h"
#include "Mesh/RealtimeMeshCardRepresentation.h"
#include "Mesh/RealtimeMeshDistanceField.h"
#include "PhysicsEngine/AggregateGeom.h"
#include "Mesh/RealtimeMeshNaniteResourcesInterface.h"

#include <atomic>


struct FRealtimeMeshDistanceField;
enum class ERealtimeMeshSectionDrawType : uint8;

namespace RealtimeMesh
{
	struct IRealtimeMeshNaniteMeshResourcesImplementation;

	static_assert(REALTIME_MESH_MAX_LODS <= FBitSet::BitsPerWord, "REALTIME_MESH_MAX_LODS must be less than or equal to FBitSet::BitsPerWord");
	using FRealtimeMeshLODMask = TBitArray<TFixedAllocator<1>>;


	/**
	 * Immutable-after-publish, COW'd snapshot of the render-thread mesh state.
	 *
	 * Each FRealtimeMeshProxy instance IS a single version of the mesh's render
	 * state. Scene proxies (FRealtimeMeshComponentSceneProxy, Nanite scene proxy,
	 * etc.) capture a TSharedRef<const FRealtimeMeshProxy> at construction and
	 * hold it for their entire lifetime — they never observe in-progress changes.
	 *
	 * Updates produce a NEW version: an RT command takes the latest version,
	 * clones it (shallow LOD-array copy; TCowPtr lazy-clones children only along
	 * the touched path), applies the batch of tasks to the clone, finalizes it
	 * (UpdatedCachedState), and atomically publishes it as the new latest via
	 * FRealtimeMesh::PublishProxy_RenderThread. Old versions stay alive as long
	 * as any scene proxy still references them and die naturally afterward.
	 *
	 * Reads through TSharedRef<const FRealtimeMeshProxy> hit the proxy's fields
	 * directly — there is no snapshot indirection. const-correctness is what
	 * enforces "scene proxies can only read, not mutate."
	 */
	class REALTIMEMESHCOMPONENT_API FRealtimeMeshProxy : public TSharedFromThis<FRealtimeMeshProxy>
	{
	protected:
		const FRealtimeMeshContextRef Context;

		// LODs is TCowPtr<...> so subsequent versions can share unchanged subtrees
		// with this one — see TCowPtr / FRealtimeMeshLODProxy::Clone. Mutating any
		// LOD beyond a refcount-1 slot triggers a per-node deep copy, leaving this
		// version's view untouched.
		TFixedLODArray<TCowPtr<FRealtimeMeshLODProxy>> LODs;

		FRealtimeMeshDrawMask DrawMask;
		FRealtimeMeshLODMask ActiveLODMask;
		FRealtimeMeshLODMask ScreenPercentageNextLODMask;
		FRealtimeMeshLODMask ActiveStaticLODMask;
		FRealtimeMeshLODMask ActiveDynamicLODMask;

		// Flat lists of active LOD indices for fast iteration. Computed by
		// UpdatedCachedState; read directly by scene proxies — no mask walking
		// required at draw time.
		TArray<int32, TInlineAllocator<REALTIME_MESH_MAX_LODS>> ActiveLODIndices;
		TArray<int32, TInlineAllocator<REALTIME_MESH_MAX_LODS>> ActiveStaticLODIndices;
		TArray<int32, TInlineAllocator<REALTIME_MESH_MAX_LODS>> ActiveDynamicLODIndices;

		// PROXY-F21: per-LOD screen-size ranges, precomputed by UpdatedCachedState.
		// Ranges are immutable after publish, so the per-frame helper reads this flat
		// array by index instead of pinning shared ptrs and rescanning the mask.
		// Indexed by LOD index; slots for LODs without a valid range are default.
		TArray<TRange<float>, TInlineAllocator<REALTIME_MESH_MAX_LODS>> ScreenSizeRanges;

		// Auxiliary render data. Held by TSharedPtr so versions can share the
		// unchanged data and old scene proxies keep pinning what they saw at
		// construction even after a newer version replaces it.
		TSharedPtr<const FDistanceFieldVolumeData> DistanceField;
		TSharedPtr<const FCardRepresentationData> CardRepresentation;
		TSharedPtr<FRealtimeMeshNaniteResources> NaniteResources;

		// Draft-only scratch: which LODs were touched while this proxy was being
		// built as a draft. Cleared by Clone() and by UpdatedCachedState. Not
		// meaningful after publish — readers should never look at it.
		TSet<int32> TouchedLODIndices;

		/* Tracks whether we have nanite data set/pending, so that the GT side can know what type of render proxy to use.
		 * PROXY-F17: written on the game thread (SetHasNaniteData_GT) on the published proxy while the render thread
		 * may concurrently read it (Clone / HasNaniteResources_GT), so it's std::atomic to avoid a formal data race.
		 * Relaxed ordering is sufficient — this flag gates only which scene-proxy type the component picks and
		 * carries no happens-before dependency on other proxy state. */
		std::atomic<bool> bHasNaniteData = false;

#if UE_ENABLE_DEBUG_DRAWING
		/** Whether the collision data has been set up for rendering */
		bool bHasCollisionData = false;

		/** Collision trace flags */
		ECollisionTraceFlag CollisionTraceFlag;
		/** Collision Response of this component */
		FCollisionResponseContainer CollisionResponse;
		/** Cached AggGeom holding the collision shapes to render */
		FKAggregateGeom CachedAggGeom;
#endif

	public:
		FRealtimeMeshProxy(const FRealtimeMeshContextRef& InContext);
		virtual ~FRealtimeMeshProxy();

		const FRealtimeMeshContextRef& GetContext() const { return Context; }

		ERHIFeatureLevel::Type GetRHIFeatureLevel() const;

		// ===== Read accessors (const — usable through TSharedRef<const FRealtimeMeshProxy>) =====

		const FRealtimeMeshDrawMask& GetDrawMask() const { return DrawMask; }
		const FRealtimeMeshLODMask& GetActiveLODMask() const { return ActiveLODMask; }
		const FRealtimeMeshLODMask& GetActiveStaticLODMask() const { return ActiveStaticLODMask; }
		const FRealtimeMeshLODMask& GetActiveDynamicLODMask() const { return ActiveDynamicLODMask; }
		const FRealtimeMeshLODMask& GetScreenPercentageNextLODMask() const { return ScreenPercentageNextLODMask; }

		int32 GetFirstLODIndex() const { return ActiveLODMask.Find(true); }
		int32 GetLastLODIndex() const { return ActiveLODMask.FindLast(true); }
		int8 GetNumLODs() const { return static_cast<int8>(LODs.Num()); }

		TArrayView<const int32> GetActiveLODIndices() const { return ActiveLODIndices; }
		TArrayView<const int32> GetActiveStaticLODIndices() const { return ActiveStaticLODIndices; }
		TArrayView<const int32> GetActiveDynamicLODIndices() const { return ActiveDynamicLODIndices; }

		// Returns a const-pointing TSharedPtr so callers cannot accidentally
		// mutate this version via this path.
		FRealtimeMeshLODProxyConstPtr GetLOD(FRealtimeMeshLODKey LODKey) const;
		const TFixedLODArray<TCowPtr<FRealtimeMeshLODProxy>>& GetLODs() const { return LODs; }

		TRange<float> GetScreenSizeRangeForLOD(const FRealtimeMeshLODKey& LODKey) const;

		bool HasDistanceFieldData() const { return DistanceField.IsValid(); }
		const FDistanceFieldVolumeData* GetDistanceFieldData() const { return DistanceField.Get(); }

		bool HasCardRepresentation() const { return CardRepresentation.IsValid(); }
		const FCardRepresentationData* GetCardRepresentation() const { return CardRepresentation.Get(); }

		bool HasNaniteResources_GT() const { return bHasNaniteData.load(std::memory_order_relaxed); }
		bool HasNaniteResources_RT() const { return NaniteResources.IsValid(); }
		TSharedPtr<FRealtimeMeshNaniteResources> GetNaniteResources() const { return NaniteResources; }

		// ===== Mutation API (non-const — called only on a draft that has not yet been published) =====

		void SetHasNaniteData_GT(bool bNewValue) { bHasNaniteData.store(bNewValue, std::memory_order_relaxed); }
		void SetNaniteResources_RT(FRealtimeMeshNaniteResourcesPtr&& InNaniteResources);
		// Nanite resource-sharing: assign an already-shared, already-initialized instance (refcount bump).
		// The custom deleter is installed game-thread-side when the shared instance is created, so this path
		// installs no deleter — every proxy version simply pins the same live registration.
		void SetNaniteResources_RT(const TSharedPtr<FRealtimeMeshNaniteResources>& InNaniteResources);
		void ClearNaniteResources_RT();

		void SetDistanceField(FRealtimeMeshDistanceField&& InDistanceField);
		// API-L8: overload taking a shared immutable snapshot so proxy recreates avoid a
		// game-thread deep copy of the DF payload. Derives the rendering data by copy
		// (CreateRenderingData) since the snapshot must not be mutated.
		void SetDistanceField(const TSharedRef<const FRealtimeMeshDistanceField>& InDistanceField);
		void ClearDistanceFieldData() { DistanceField.Reset(); }

		void SetCardRepresentation(FRealtimeMeshCardRepresentation&& InCardRepresentation);
		// API-L8: shared-snapshot overload; see SetDistanceField above.
		void SetCardRepresentation(const TSharedRef<const FRealtimeMeshCardRepresentation>& InCardRepresentation);
		void ClearCardRepresentation() { CardRepresentation.Reset(); }

		/**
		 * Look up an LOD as a *mutable* reference. The slot's TCowPtr is COW'd if
		 * it's still shared with an older published version (refcount > 1) so the
		 * returned LOD is private to this draft. Marks the LOD as touched so the
		 * subsequent UpdatedCachedState pass refreshes its cached state.
		 *
		 * Called only by the proxy update builder while applying a batch of tasks
		 * to a draft. The returned pointer is non-owning — workspace storage owns
		 * lifetime.
		 */
		FRealtimeMeshLODProxy* FindWorkspaceLOD(FRealtimeMeshLODKey LODKey);

		/**
		 * In-place fast-update entry (render thread only). Resolves the buffer set for the
		 * given key only when BOTH its LOD node and the buffer set node are uniquely owned
		 * (no published snapshot shares them), so its GPU buffers can be safely overwritten
		 * in place without cloning or publishing. Returns nullptr otherwise, signalling the
		 * caller to fall back to the clone+publish path.
		 */
		FRealtimeMeshBufferSetProxy* FindUniqueBufferSetForInPlace(const FRealtimeMeshBufferSetKey& BufferSetKey);

		void AddLODIfNotExists(const FRealtimeMeshLODKey& LODKey);
		void RemoveLOD(const FRealtimeMeshLODKey& LODKey);

#if UE_ENABLE_DEBUG_DRAWING
		void SetCollisionRenderData(const FKAggregateGeom& InAggGeom, ECollisionTraceFlag InCollisionTraceFlag, const FCollisionResponseContainer& InCollisionResponse);
#endif

		/**
		 * Refresh aggregate state (DrawMask, ActiveLOD masks, flat active-LOD
		 * lists) from the per-LOD cached state. Called once at the end of each
		 * batch, after all touched LODs have run their own UpdateCachedState. Only
		 * touched LODs are walked for the per-LOD pass; the aggregate pass reads
		 * every LOD's now-current draw mask.
		 */
		void UpdatedCachedState(FRHICommandListBase& RHICmdList);
		void Reset();

		/**
		 * Shallow clone for COW. Produces a fresh FRealtimeMeshProxy that
		 * initially shares every LOD TCowPtr with this one. The returned proxy
		 * starts with empty TouchedLODIndices so the next batch's applied tasks
		 * track their own touched paths.
		 *
		 * Used by FRealtimeMesh::ApplyAndPublish_RenderThread to build a draft
		 * from the current published version.
		 */
		TSharedRef<FRealtimeMeshProxy> Clone() const;

	private:
		// PROXY-F21: computes the screen-size range for a single LOD from the current
		// LOD screen sizes and ScreenPercentageNextLODMask. Called by UpdatedCachedState
		// to populate the cached ScreenSizeRanges array; the per-frame accessor
		// GetScreenSizeRangeForLOD then just indexes that array.
		TRange<float> ComputeScreenSizeRangeForLOD(int32 LODIndex) const;
	};
}
