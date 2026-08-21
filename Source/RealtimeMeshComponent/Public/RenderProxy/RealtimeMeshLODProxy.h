// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "RealtimeMeshCore.h"
#include "RealtimeMeshProxyShared.h"
#include "RealtimeMeshBufferSetProxy.h"
#include "Core/RealtimeMeshLODConfig.h"

namespace RealtimeMesh
{
	/**
	 * Precomputed (buffer set, section) pair recorded in a LOD's render lists at
	 * publish time. Rendering walks these lists directly instead of doing nested
	 * mask iteration — UpdateCachedState builds the lists once per batch and the
	 * snapshot's LOD clone gets them by copy.
	 *
	 * BufferSetIndex indexes into FRealtimeMeshLODProxy::BufferSets (the buffer
	 * set tier). SectionIndex indexes into FRealtimeMeshLODProxy::Sections.
	 */
	struct FRealtimeMeshRenderEntry
	{
		int32 BufferSetIndex = INDEX_NONE;
		int32 SectionIndex = INDEX_NONE;
	};


	class REALTIMEMESHCOMPONENT_API FRealtimeMeshLODProxy : public TSharedFromThis<FRealtimeMeshLODProxy>
	{
	private:
		const FRealtimeMeshContextRef Context;
		const FRealtimeMeshLODKey Key;

		// Buffer-set tier. Holds streams, vertex factory, ray-tracing geometry.
		// Sections live directly on the LOD below and reference their owning
		// buffer set by key.
		//
		// Stored as TCowPtr so the type itself enforces that read access is
		// const-only; any mutating code must go through Write() (which clones if
		// the slot is shared with a published version). FindMutableBufferSet
		// wraps that for the command-batch task helpers and also marks the slot
		// touched.
		TArray<TCowPtr<FRealtimeMeshBufferSetProxy>> BufferSets;
		TMap<FRealtimeMeshBufferSetKey, uint32> BufferSetMap;

		// Sections live here as a flat array. Each section knows its owning
		// buffer set via its FRealtimeMeshSectionKey (which embeds the BS key).
		// Multiple sections referencing the same BS is supported — storage is at
		// the LOD level rather than inside the BS.
		TArray<TCowPtr<FRealtimeMeshSectionProxy>> Sections;
		TMap<FRealtimeMeshSectionKey, uint32> SectionMap;

		// Indices into Sections, grouped by their owning buffer set key, filtered
		// to only the active ones. Rebuilt by UpdateCachedState. The
		// section-iterator shim on FRealtimeMeshBufferSetProxy walks the entry
		// for its own key.
		TMap<FRealtimeMeshBufferSetKey, TArray<int32>> ActiveSectionIndicesByBufferSet;

		// Flat lists of (buffer set, section) pairs to render, computed once per
		// batch by UpdateCachedState and shipped to every snapshot via Clone.
		// Scene proxies walk these directly without mask iteration.
		// AllRenderEntries covers any pair whose buffer set renders at all — used
		// by the editor "force dynamic" path that draws everything via the
		// dynamic path.
		TArray<FRealtimeMeshRenderEntry> AllRenderEntries;
		TArray<FRealtimeMeshRenderEntry> StaticRenderEntries;
		TArray<FRealtimeMeshRenderEntry> DynamicRenderEntries;
		// RayTracingRenderEntries are ordered so all pairs sharing a buffer set
		// are contiguous — scene proxies can scan-and-group to emit one BVH per
		// BS.
		TArray<FRealtimeMeshRenderEntry> RayTracingRenderEntries;
		// Buffer sets that contributed at least one section to
		// RayTracingRenderEntries — used by GetDynamicRayTracingInstances to emit
		// one BVH per BS.
		TArray<int32> RayTracingBufferSetIndices;
		// Buffer sets with any active rendering — used by debug-visualization
		// paths that iterate per-BS (normals/tangents/etc).
		TArray<int32> ActiveBufferSetIndices;

		FRealtimeMeshLODConfig Config;
		FRealtimeMeshDrawMask DrawMask;

#if RHI_RAYTRACING
		int32 StaticRayTraceBufferSet;
#endif

		// Draft-only scratch (not copied by Clone): tracks which children were
		// COW'd or otherwise mutated during the current batch. UpdateCachedState
		// walks only these instead of every child. Cleared by UpdateCachedState
		// after it runs. On a published LOD these sets are empty.
		TSet<int32> TouchedBufferSetIndices;
		TSet<int32> TouchedSectionIndices;

		// Draft-only scratch (not copied by Clone): indices into Sections /
		// BufferSets tombstoned by RemoveSection / RemoveBufferSet during the
		// current batch. Their array slots stay in place so every unaffected index
		// — and the key maps — remain valid for the rest of the batch; the actual
		// array compaction and single map rebuild happen once at the top of
		// UpdateCachedState (CompactPendingRemovals). Empty on a published LOD.
		TSet<int32> PendingRemovedSectionIndices;
		TSet<int32> PendingRemovedBufferSetIndices;

	public:
		FRealtimeMeshLODProxy(const FRealtimeMeshContextRef& InContext, const FRealtimeMeshLODKey& InKey);
		virtual ~FRealtimeMeshLODProxy();

		/**
		 * Produce a shallow clone for COW. The new LOD shares every BufferSet
		 * and Section TSharedRef with the source — when a child later needs to
		 * be mutated, FindMutableBufferSet / FindMutableSection clone that child
		 * individually. Touched-tracking sets are NOT carried across, so the
		 * clone starts with no pending updates.
		 */
		TSharedRef<FRealtimeMeshLODProxy> Clone() const;

		const FRealtimeMeshLODKey& GetKey() const { return Key; }
		const FRealtimeMeshLODConfig& GetConfig() const { return Config; }
		FRealtimeMeshDrawMask GetDrawMask() const { return DrawMask; }
		float GetScreenSize() const { return Config.ScreenSize; }

		// Read accessors return const-pointing TSharedPtrs / TCowPtrs — callers
		// can inspect the underlying nodes but cannot call mutating methods on
		// them. To mutate, go through FindMutableBufferSet / FindMutableSection.
		FRealtimeMeshBufferSetProxyConstPtr GetBufferSet(const FRealtimeMeshBufferSetKey& BufferSetKey) const;

		// COW-aware mutable accessors used by the command-batch task helpers.
		// Each looks up the child by key and, if the slot's TSharedPtr is shared
		// with an older published version, clones the child into a draft-private
		// copy before returning the raw pointer. The pointer is non-owning —
		// workspace storage owns the lifetime. Marks the child as touched for
		// the next UpdateCachedState pass.
		FRealtimeMeshBufferSetProxy* FindMutableBufferSet(const FRealtimeMeshBufferSetKey& BufferSetKey);
		FRealtimeMeshSectionProxy* FindMutableSection(const FRealtimeMeshSectionKey& SectionKey);

		// In-place fast update: returns the buffer set ONLY if its TCowPtr is uniquely
		// owned (no published snapshot shares it), so its GPU buffers can be mutated in
		// place. Unlike FindMutableBufferSet this never clones and never marks the slot
		// touched. Returns nullptr if the slot is shared or absent.
		FRealtimeMeshBufferSetProxy* FindUniqueBufferSetForInPlace(const FRealtimeMeshBufferSetKey& BufferSetKey);

		// Section accessors (sections live on the LOD post-flattening).
		FRealtimeMeshSectionProxyConstPtr GetSection(const FRealtimeMeshSectionKey& SectionKey) const;
		const TArray<TCowPtr<FRealtimeMeshSectionProxy>>& GetSections() const { return Sections; }
		const TArray<TCowPtr<FRealtimeMeshBufferSetProxy>>& GetBufferSets() const { return BufferSets; }
		const TArray<int32>* FindActiveSectionIndicesForBufferSet(const FRealtimeMeshBufferSetKey& BufferSetKey) const
		{
			return ActiveSectionIndicesByBufferSet.Find(BufferSetKey);
		}

		// Back-compat shims: prior internal name was "SectionGroup". Forward
		// to the BufferSet API so external code that still calls the old names
		// keeps compiling.
		FRealtimeMeshBufferSetProxyConstPtr GetSectionGroup(const FRealtimeMeshBufferSetKey& BufferSetKey) const { return GetBufferSet(BufferSetKey); }
		const TArray<TCowPtr<FRealtimeMeshBufferSetProxy>>& GetSectionGroups() const { return GetBufferSets(); }
		const TArray<int32>* FindActiveSectionIndicesForGroup(const FRealtimeMeshBufferSetKey& BufferSetKey) const { return FindActiveSectionIndicesForBufferSet(BufferSetKey); }

		// Flat render-list accessors. Scene proxies walk these directly to emit
		// FMeshBatch / RT instance entries. Each entry resolves via GetSections()
		// and GetBufferSets() indexed by SectionIndex / BufferSetIndex.
		const TArray<FRealtimeMeshRenderEntry>& GetAllRenderEntries() const { return AllRenderEntries; }
		const TArray<FRealtimeMeshRenderEntry>& GetStaticRenderEntries() const { return StaticRenderEntries; }
		const TArray<FRealtimeMeshRenderEntry>& GetDynamicRenderEntries() const { return DynamicRenderEntries; }
		const TArray<FRealtimeMeshRenderEntry>& GetRayTracingRenderEntries() const { return RayTracingRenderEntries; }
		const TArray<int32>& GetRayTracingBufferSetIndices() const { return RayTracingBufferSetIndices; }
		const TArray<int32>& GetActiveBufferSetIndices() const { return ActiveBufferSetIndices; }

		void UpdateConfig(const FRealtimeMeshLODConfig& NewConfig);

		void CreateBufferSetIfNotExists(const FRealtimeMeshBufferSetKey& BufferSetKey);
		void RemoveBufferSet(const FRealtimeMeshBufferSetKey& BufferSetKey);

		// Back-compat shims for callers still using the old "SectionGroup" names.
		void CreateSectionGroupIfNotExists(const FRealtimeMeshBufferSetKey& BufferSetKey) { CreateBufferSetIfNotExists(BufferSetKey); }
		void RemoveSectionGroup(const FRealtimeMeshBufferSetKey& BufferSetKey) { RemoveBufferSet(BufferSetKey); }

		// Section mutators (moved up from buffer set; the BS-level methods now
		// forward here through their ParentLOD ref to preserve the existing
		// builder task API). BufferSetIndex selects which buffer set the
		// section reads from; INDEX_NONE means "leave the section unbound" —
		// callers that go through the legacy BS-based API supply the BS's slot
		// index here so existing flows work without modification.
		void CreateSectionIfNotExists(const FRealtimeMeshSectionKey& SectionKey, int32 BufferSetIndex = INDEX_NONE);
		void RemoveSection(const FRealtimeMeshSectionKey& SectionKey);

#if RHI_RAYTRACING
		FRayTracingGeometry* GetStaticRayTracingGeometry() const;
#endif

		void UpdateCachedState(FRHICommandListBase& RHICmdList);
		void Reset();

	protected:
		void RebuildBufferSetMap();
		void RebuildSectionMap();

		// Compacts the Section / BufferSet slots tombstoned during the batch (one
		// pass each), remaps the touched-index sets to the new indices, and rebuilds
		// the key maps once. Called at the top of UpdateCachedState so all subsequent
		// passes walk fully-compacted arrays.
		void CompactPendingRemovals();

		// Converts a section's BufferSetIndex (which stores the BS's slot index
		// from its key) into the corresponding array index in BufferSets, or
		// INDEX_NONE if no BS with that slot exists. Necessary because slot
		// indices (typically CRC32 hashes of the SG name for legacy factories)
		// are NOT the same as the array index unless the user assigned them
		// that way explicitly.
		int32 BufferSetSlotToArrayIndex(int32 SlotIndex) const
		{
			if (SlotIndex == INDEX_NONE) return INDEX_NONE;
			const uint32* ArrayIdx = BufferSetMap.Find(FRealtimeMeshBufferSetKey::Create(Key, SlotIndex));
			return ArrayIdx ? static_cast<int32>(*ArrayIdx) : INDEX_NONE;
		}

		friend class FRealtimeMeshActiveSectionIterator;
	};
}
