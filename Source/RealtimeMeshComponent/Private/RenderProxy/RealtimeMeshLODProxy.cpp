// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#include "RenderProxy/RealtimeMeshLODProxy.h"

#include "Algo/IndexOf.h"
#include "Data/RealtimeMeshShared.h"
#include "Core/RealtimeMeshLODConfig.h"
#include "RenderProxy/RealtimeMeshBufferSetProxy.h"
#include "RenderProxy/RealtimeMeshSectionProxy.h"

namespace RealtimeMesh
{
	FRealtimeMeshLODProxy::FRealtimeMeshLODProxy(const FRealtimeMeshContextRef& InContext, const FRealtimeMeshLODKey& InKey)
		: Context(InContext)
		  , Key(InKey)
	{
	}

	FRealtimeMeshLODProxy::~FRealtimeMeshLODProxy()
	{
		check(IsInRenderingThread());
		Reset();
	}

	TSharedRef<FRealtimeMeshLODProxy> FRealtimeMeshLODProxy::Clone() const
	{
		// Allocate via shared resources so subclasses get the correct concrete type.
		// This is a SHALLOW clone — BufferSets and Sections arrays share TSharedRefs
		// with the source. Individual children are COW'd on demand by
		// FindMutableBufferSet / FindMutableSection. Touched-tracking sets are NOT
		// carried across; the new workspace LOD starts with no pending mutations.
		const TSharedRef<FRealtimeMeshLODProxy> Cloned = Context->CreateLODProxy(Key);
		Cloned->Config = Config;
		Cloned->DrawMask = DrawMask;
#if RHI_RAYTRACING
		Cloned->StaticRayTraceBufferSet = StaticRayTraceBufferSet;
#endif
		Cloned->BufferSets = BufferSets;
		Cloned->BufferSetMap = BufferSetMap;
		Cloned->Sections = Sections;
		Cloned->SectionMap = SectionMap;
		Cloned->ActiveSectionIndicesByBufferSet = ActiveSectionIndicesByBufferSet;
		Cloned->AllRenderEntries = AllRenderEntries;
		Cloned->StaticRenderEntries = StaticRenderEntries;
		Cloned->DynamicRenderEntries = DynamicRenderEntries;
		Cloned->RayTracingRenderEntries = RayTracingRenderEntries;
		Cloned->RayTracingBufferSetIndices = RayTracingBufferSetIndices;
		Cloned->ActiveBufferSetIndices = ActiveBufferSetIndices;
		return Cloned;
	}

	FRealtimeMeshBufferSetProxy* FRealtimeMeshLODProxy::FindMutableBufferSet(const FRealtimeMeshBufferSetKey& BufferSetKey)
	{
		const uint32* IndexPtr = BufferSetMap.Find(BufferSetKey);
		if (!IndexPtr)
		{
			return nullptr;
		}
		const int32 Index = static_cast<int32>(*IndexPtr);
		TCowPtr<FRealtimeMeshBufferSetProxy>& Slot = BufferSets[Index];
		// TCowPtr::Write() clones via FRealtimeMeshBufferSetProxy::Clone() if the
		// underlying TSharedPtr is shared with anyone else (typically a published
		// snapshot). Slot rebinds to the clone in place.
		const bool bWasShared = !Slot.IsUnique();
		FRealtimeMeshBufferSetProxy& BS = Slot.Write();
		if (bWasShared)
		{
			// The clone needs its ParentLOD wired to this LOD so its forwarding
			// methods resolve against our section storage.
			BS.SetParentLOD(StaticCastSharedRef<FRealtimeMeshLODProxy>(AsShared()));
		}
		TouchedBufferSetIndices.Add(Index);
		return &BS;
	}

	FRealtimeMeshBufferSetProxy* FRealtimeMeshLODProxy::FindUniqueBufferSetForInPlace(const FRealtimeMeshBufferSetKey& BufferSetKey)
	{
		const uint32* IndexPtr = BufferSetMap.Find(BufferSetKey);
		if (!IndexPtr)
		{
			return nullptr;
		}

		TCowPtr<FRealtimeMeshBufferSetProxy>& Slot = BufferSets[static_cast<int32>(*IndexPtr)];

		// Shared with a published snapshot → an in-place write would corrupt that view.
		// Bail so the caller takes the reallocating clone+publish path instead.
		if (!Slot.IsUnique())
		{
			return nullptr;
		}

		// Unique: return the existing object without ever cloning (GetUniqueUnchecked, not
		// Write(), so a concurrent GT holder appearing after IsUnique() can't cause a
		// clone-and-rebind of this published slot), and we deliberately skip the
		// touched-marking (an in-place update needs no UpdateCachedState).
		return Slot.GetUniqueUnchecked();
	}

	FRealtimeMeshSectionProxy* FRealtimeMeshLODProxy::FindMutableSection(const FRealtimeMeshSectionKey& SectionKey)
	{
		const uint32* IndexPtr = SectionMap.Find(SectionKey);
		if (!IndexPtr)
		{
			return nullptr;
		}
		const int32 Index = static_cast<int32>(*IndexPtr);
		TCowPtr<FRealtimeMeshSectionProxy>& Slot = Sections[Index];
		FRealtimeMeshSectionProxy& Section = Slot.Write();
		TouchedSectionIndices.Add(Index);
		// Mutating a section marks its buffer set touched so UpdateCachedState re-evaluates it.
		// This does NOT by itself reallocate the vertex factory: BS::UpdateCachedState reinitializes
		// the VF only when bVertexFactoryDirty (a stream buffer object was replaced) or a section's
		// stream range actually changed — a pure config-only edit (visibility, material) leaves the
		// VF untouched and allocates nothing. The re-evaluation's real remaining cost is the
		// ray-tracing BLAS refresh in UpdateCachedState step 4, which is itself gated by the
		// PROXY-F11 build-signature compare, so a config/material-only touch rebuilds nothing.
		const int32 BufferSetArrayIdx = BufferSetSlotToArrayIndex(Section.GetBufferSetIndex());
		if (BufferSetArrayIdx != INDEX_NONE)
		{
			TouchedBufferSetIndices.Add(BufferSetArrayIdx);
		}
		return &Section;
	}

	FRealtimeMeshBufferSetProxyConstPtr FRealtimeMeshLODProxy::GetBufferSet(const FRealtimeMeshBufferSetKey& BufferSetKey) const
	{
		check(BufferSetKey.IsPartOf(Key));

		if (const uint32* Index = BufferSetMap.Find(BufferSetKey))
		{
			return BufferSets[*Index].ToSharedPtrConst();
		}
		return FRealtimeMeshBufferSetProxyConstPtr();
	}

	FRealtimeMeshSectionProxyConstPtr FRealtimeMeshLODProxy::GetSection(const FRealtimeMeshSectionKey& SectionKey) const
	{
		if (const uint32* Index = SectionMap.Find(SectionKey))
		{
			return Sections[*Index].ToSharedPtrConst();
		}
		return FRealtimeMeshSectionProxyConstPtr();
	}

	void FRealtimeMeshLODProxy::UpdateConfig(const FRealtimeMeshLODConfig& NewConfig)
	{
		Config = NewConfig;
	}

	void FRealtimeMeshLODProxy::CreateBufferSetIfNotExists(const FRealtimeMeshBufferSetKey& BufferSetKey)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(FRealtimeMeshLODProxy::CreateBufferSetIfNotExists);

		check(BufferSetKey.IsPartOf(Key));

		if (!BufferSetMap.Contains(BufferSetKey))
		{
			const TSharedRef<FRealtimeMeshBufferSetProxy> NewSG = Context->CreateSectionGroupProxy(BufferSetKey);
			NewSG->SetParentLOD(StaticCastSharedRef<FRealtimeMeshLODProxy>(AsShared()));
			const int32 BufferSetArrayIdx = BufferSets.Add(TCowPtr<FRealtimeMeshBufferSetProxy>(NewSG));
			BufferSetMap.Add(BufferSetKey, BufferSetArrayIdx);
			TouchedBufferSetIndices.Add(BufferSetArrayIdx);
		}
	}

	void FRealtimeMeshLODProxy::RemoveBufferSet(const FRealtimeMeshBufferSetKey& BufferSetKey)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(FRealtimeMeshLODProxy::RemoveBufferSet);

		check(BufferSetKey.IsPartOf(Key));

		// Deferred compaction (see CompactPendingRemovals): drop the map entry now so
		// lookups fail correctly and tombstone the slot; the array is compacted and the
		// map rebuilt once in UpdateCachedState. Leaving the slot in place keeps every
		// other index — and the maps — valid for the rest of the batch, so batched
		// removals no longer pay an O(count) map rebuild per removal.
		if (const uint32* IndexPtr = BufferSetMap.Find(BufferSetKey))
		{
			PendingRemovedBufferSetIndices.Add(static_cast<int32>(*IndexPtr));
			BufferSetMap.Remove(BufferSetKey);
		}

		// Also tombstone any sections that belonged to that group — they would now be
		// orphans referencing a missing buffer set. Single pass, no per-removal shift.
		// Match on the runtime buffer-set binding, not key ancestry: sections can be
		// rebound via SetBufferSet, so their key may no longer reflect which buffer set
		// they actually reference.
		for (int32 Index = 0; Index < Sections.Num(); ++Index)
		{
			if (PendingRemovedSectionIndices.Contains(Index))
			{
				continue;
			}
			if (Sections[Index]->GetBufferSetIndex() == BufferSetKey.Index())
			{
				SectionMap.Remove(Sections[Index]->GetKey());
				PendingRemovedSectionIndices.Add(Index);
			}
		}
		ActiveSectionIndicesByBufferSet.Remove(BufferSetKey);
	}

	void FRealtimeMeshLODProxy::CreateSectionIfNotExists(const FRealtimeMeshSectionKey& SectionKey, int32 BufferSetIndex)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(FRealtimeMeshLODProxy::CreateSectionIfNotExists);

		check(SectionKey.LOD() == Key);

		int32 SectionIndex = INDEX_NONE;
		if (!SectionMap.Contains(SectionKey))
		{
			const TSharedRef<FRealtimeMeshSectionProxy> NewSection = Context->CreateSectionProxy(SectionKey);
			if (BufferSetIndex != INDEX_NONE)
			{
				NewSection->SetBufferSetIndex(BufferSetIndex);
			}
			SectionIndex = Sections.Add(TCowPtr<FRealtimeMeshSectionProxy>(NewSection));
			SectionMap.Add(SectionKey, SectionIndex);
		}
		else
		{
			SectionIndex = SectionMap[SectionKey];
			// TCowPtr::Write() clones if the section is shared with a snapshot,
			// so Reset / SetBufferSetIndex don't leak into the snapshot.
			FRealtimeMeshSectionProxy& Section = Sections[SectionIndex].Write();
			Section.Reset();
			if (BufferSetIndex != INDEX_NONE)
			{
				Section.SetBufferSetIndex(BufferSetIndex);
			}
		}
		TouchedSectionIndices.Add(SectionIndex);
		// Adding a section also changes its buffer set's active-section bucket,
		// so the BS needs a re-evaluation. BufferSetIndex here is the BS's slot
		// index (from BS.Key.Index()); resolve through BufferSetMap to get
		// the array index that TouchedBufferSetIndices uses.
		const int32 BufferSetArrayIdx = BufferSetSlotToArrayIndex(BufferSetIndex);
		if (BufferSetArrayIdx != INDEX_NONE)
		{
			TouchedBufferSetIndices.Add(BufferSetArrayIdx);
		}
	}

	void FRealtimeMeshLODProxy::RemoveSection(const FRealtimeMeshSectionKey& SectionKey)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(FRealtimeMeshLODProxy::RemoveSection);

		const uint32* IndexPtr = SectionMap.Find(SectionKey);
		if (!IndexPtr)
		{
			return;
		}
		const int32 SectionIndex = static_cast<int32>(*IndexPtr);

		// Mark the section's buffer set touched before we drop the section, so
		// the BS's active-section bucket gets recomputed.
		if (Sections.IsValidIndex(SectionIndex))
		{
			const int32 BufferSetArrayIdx = BufferSetSlotToArrayIndex(Sections[SectionIndex]->GetBufferSetIndex());
			if (BufferSetArrayIdx != INDEX_NONE)
			{
				TouchedBufferSetIndices.Add(BufferSetArrayIdx);
			}
		}

		// Deferred compaction (see CompactPendingRemovals): drop the map entry now,
		// tombstone the slot, and compact + rebuild the map once in UpdateCachedState.
		// Batched removals thus cost a single O(sections) rebuild instead of one per removal.
		SectionMap.Remove(SectionKey);
		PendingRemovedSectionIndices.Add(SectionIndex);
	}

#if RHI_RAYTRACING
	FRayTracingGeometry* FRealtimeMeshLODProxy::GetStaticRayTracingGeometry() const
	{
		if (!BufferSets.IsValidIndex(StaticRayTraceBufferSet))
		{
			return nullptr;
		}
		// The engine ray-tracing path takes a mutable FRayTracingGeometry* (BVH
		// build/update operations are non-const). The underlying TSharedPtr is the
		// same across snapshot and workspace; mutation here is shared, but engine
		// BVH ops are idempotent rebuilds from the initializer.
		const FRayTracingGeometry* ConstGeom = BufferSets[StaticRayTraceBufferSet]->GetRayTracingGeometry();
		return const_cast<FRayTracingGeometry*>(ConstGeom);
	}
#endif

	void FRealtimeMeshLODProxy::UpdateCachedState(FRHICommandListBase& RHICmdList)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(FRealtimeMeshLODProxy::UpdateCachedState);

		// Compact any sections / buffer sets removed this batch (deferred by
		// RemoveSection / RemoveBufferSet) before the passes below walk the arrays.
		CompactPendingRemovals();

		// Step 1: touched buffer sets handle their own VertexFactory + ray-tracing
		// geometry rebuilds. Untouched BSs are still shared with snapshots from
		// prior batches — their state is correct from when they were last touched.
		//
		// A touched BS may have arrived here in two ways: explicitly COW'd by
		// FindMutableBufferSet (which already wired ParentLOD), or touched via a
		// section mutation that ran FindMutableSection (which marks the BS touched
		// but does NOT COW the BS itself). In the latter case Write() below is the
		// first time the BS gets cloned this batch — and BS::Clone leaves
		// ParentLOD null because it doesn't know which LOD will own the clone.
		// We're that owning LOD, so wire it up here so the BS's section forwarders
		// (GetSection etc.) resolve against our section storage once published.
		for (const int32 BSIndex : TouchedBufferSetIndices)
		{
			if (!BufferSets.IsValidIndex(BSIndex))
			{
				continue;
			}
			TCowPtr<FRealtimeMeshBufferSetProxy>& Slot = BufferSets[BSIndex];
			const bool bWasShared = !Slot.IsUnique();
			FRealtimeMeshBufferSetProxy& BS = Slot.Write();
			if (bWasShared)
			{
				BS.SetParentLOD(StaticCastSharedRef<FRealtimeMeshLODProxy>(AsShared()));
			}
			BS.UpdateCachedState(RHICmdList);
		}

		// Step 2: build "effectively touched" section set = directly-touched sections
		// plus every section whose buffer set was touched (a BS reinit may have
		// invalidated its sections' stream-range validity, so they need a re-check).
		// Section::BufferSetIndex is the BS's *slot index* (from its key) — resolve
		// through BufferSetMap to the array index to compare against
		// TouchedBufferSetIndices (which stores array indices).
		TSet<int32> EffectivelyTouchedSections = TouchedSectionIndices;
		if (TouchedBufferSetIndices.Num() > 0)
		{
			for (int32 SectionIndex = 0; SectionIndex < Sections.Num(); ++SectionIndex)
			{
				const int32 BSArrayIdx = BufferSetSlotToArrayIndex(Sections[SectionIndex]->GetBufferSetIndex());
				if (BSArrayIdx != INDEX_NONE && TouchedBufferSetIndices.Contains(BSArrayIdx))
				{
					EffectivelyTouchedSections.Add(SectionIndex);
				}
			}
		}
		for (const int32 SectionIndex : EffectivelyTouchedSections)
		{
			if (!Sections.IsValidIndex(SectionIndex))
			{
				continue;
			}
			// .Write() COWs if the section is still shared with a snapshot.
			FRealtimeMeshSectionProxy& Section = Sections[SectionIndex].Write();
			const int32 BufferSetArrayIdx = BufferSetSlotToArrayIndex(Section.GetBufferSetIndex());
			if (BufferSetArrayIdx == INDEX_NONE)
			{
				continue;
			}
			// Section::UpdateCachedState only reads the BS's vertex factory; it doesn't
			// mutate the BS — so passing the const view via Read() is fine.
			Section.UpdateCachedState(const_cast<FRealtimeMeshBufferSetProxy&>(BufferSets[BufferSetArrayIdx].Read()));
		}

		// Step 3: rebuild ActiveSectionIndicesByBufferSet. Cheap (O(sections)) and
		// guarantees correctness whether sections were rebound, removed, or just
		// flipped visibility — no need to track partial invalidations. The bucket
		// key is the BS's slot-index-based SGKey (matches how BufferSetMap is
		// keyed and how the BS itself reports its own key).
		ActiveSectionIndicesByBufferSet.Reset();
		for (int32 SectionIndex = 0; SectionIndex < Sections.Num(); ++SectionIndex)
		{
			const FRealtimeMeshSectionProxy& Section = Sections[SectionIndex].Read();
			if (!Section.GetDrawMask().ShouldRender())
			{
				continue;
			}
			const FRealtimeMeshBufferSetKey BufferSetKey = FRealtimeMeshBufferSetKey::Create(Key, Section.GetBufferSetIndex());
			if (!BufferSetMap.Contains(BufferSetKey))
			{
				continue;
			}
			ActiveSectionIndicesByBufferSet.FindOrAdd(BufferSetKey).Add(SectionIndex);
		}

		// Step 4: each touched buffer set folds its (potentially new) active-section
		// bucket into its DrawMask and rebuilds ray-tracing geometry. Untouched BSs
		// keep their previous DrawMask — by construction nothing in their bucket
		// changed.
		for (const int32 BSIndex : TouchedBufferSetIndices)
		{
			if (!BufferSets.IsValidIndex(BSIndex))
			{
				continue;
			}
			FRealtimeMeshBufferSetProxy& BufferSet = BufferSets[BSIndex].Write();
			const TArray<int32>* Bucket = ActiveSectionIndicesByBufferSet.Find(BufferSet.GetKey());
			static const TArray<int32> EmptyBucket;
			BufferSet.SetDrawMaskFromActiveSections(RHICmdList, Bucket ? *Bucket : EmptyBucket, Sections);
		}

		// Step 4b: aggregate group draw masks into the LOD draw mask and ray-tracing flags.
		// Read-only walk of every BS.
		DrawMask = FRealtimeMeshDrawMask();
		if (Config.bIsVisible && Config.ScreenSize >= 0)
		{
			uint32 RayTracingRelevantBufferSetCount = 0;

			for (const TCowPtr<FRealtimeMeshBufferSetProxy>& BufferSetSlot : BufferSets)
			{
				const auto BufferSetDrawMask = BufferSetSlot->GetDrawMask();
				DrawMask |= BufferSetDrawMask;

				if (BufferSetDrawMask.ShouldRenderInRayTracing())
				{
					RayTracingRelevantBufferSetCount++;
				}
			}

			if (RayTracingRelevantBufferSetCount > 1 || DrawMask.IsSet(ERealtimeMeshDrawMask::DrawDynamic))
			{
				DrawMask.SetFlag(ERealtimeMeshDrawMask::DynamicRayTracing);
			}
		}

#if RHI_RAYTRACING
		if (DrawMask.CanRenderInStaticRayTracing())
		{
			StaticRayTraceBufferSet = Algo::IndexOfByPredicate(BufferSets, [](const TCowPtr<FRealtimeMeshBufferSetProxy>& Slot)
			{
				return Slot->GetDrawMask().CanRenderInStaticRayTracing();
			});
		}
		else
		{
			StaticRayTraceBufferSet = INDEX_NONE;
		}
#endif

		// Step 5: precompute flat (buffer set, section) lists for the various render
		// paths. Scene proxies walk these directly instead of doing nested mask
		// iteration. Bucketed sections per BS already live in
		// ActiveSectionIndicesByBufferSet, so this is just a fan-out per draw category.
		AllRenderEntries.Reset();
		StaticRenderEntries.Reset();
		DynamicRenderEntries.Reset();
		RayTracingRenderEntries.Reset();
		RayTracingBufferSetIndices.Reset();
		ActiveBufferSetIndices.Reset();

		for (int32 BufferSetIndex = 0; BufferSetIndex < BufferSets.Num(); ++BufferSetIndex)
		{
			const TCowPtr<FRealtimeMeshBufferSetProxy>& BufferSet = BufferSets[BufferSetIndex];
			const FRealtimeMeshDrawMask BufferSetMask = BufferSet->GetDrawMask();
			if (!BufferSetMask.ShouldRender())
			{
				continue;
			}

			const TArray<int32>* SectionIndices = ActiveSectionIndicesByBufferSet.Find(BufferSet->GetKey());
			if (!SectionIndices || SectionIndices->Num() == 0)
			{
				continue;
			}

			ActiveBufferSetIndices.Add(BufferSetIndex);

			const bool bRendersStatic = BufferSetMask.ShouldRenderStaticPath();
			const bool bRendersDynamic = BufferSetMask.ShouldRenderDynamicPath();
			const bool bRendersRayTracing = BufferSetMask.ShouldRenderInRayTracing();

			bool bRayTracingRecorded = false;
			for (int32 SectionIndex : *SectionIndices)
			{
				const FRealtimeMeshRenderEntry Entry{ BufferSetIndex, SectionIndex };
				AllRenderEntries.Add(Entry);
				if (bRendersStatic)
				{
					StaticRenderEntries.Add(Entry);
				}
				if (bRendersDynamic)
				{
					DynamicRenderEntries.Add(Entry);
				}
				if (bRendersRayTracing && Sections[SectionIndex]->GetDrawMask().ShouldRenderMainPass())
				{
					RayTracingRenderEntries.Add(Entry);
					if (!bRayTracingRecorded)
					{
						RayTracingBufferSetIndices.Add(BufferSetIndex);
						bRayTracingRecorded = true;
					}
				}
			}
		}

		// Touched-tracking is consumed by this pass — clear so the next batch starts
		// fresh. (BuildAndPublishSnapshot doesn't carry these into the snapshot; they
		// only matter for the workspace's incremental update pipeline.)
		TouchedBufferSetIndices.Reset();
		TouchedSectionIndices.Reset();
	}

	void FRealtimeMeshLODProxy::RebuildBufferSetMap()
	{
		BufferSetMap.Empty(BufferSets.Num());
		for (auto It = BufferSets.CreateIterator(); It; ++It)
		{
			// TCowPtr::operator-> returns const T*, GetKey() is const — read-only walk.
			BufferSetMap.Add(It->Get()->GetKey(), It.GetIndex());
		}
	}

	void FRealtimeMeshLODProxy::RebuildSectionMap()
	{
		SectionMap.Empty(Sections.Num());
		for (auto It = Sections.CreateIterator(); It; ++It)
		{
			SectionMap.Add(It->Get()->GetKey(), It.GetIndex());
		}
	}

	namespace
	{
		// DUP-006: single-pass stable compaction of tombstoned slots + touched-index
		// remap, shared by the buffer-set and section passes of CompactPendingRemovals.
		// The touched set stores array indices, so it is remapped to the post-compaction
		// indices (dropping any that were removed). RebuildMap is invoked (only when work
		// was done) at the same point the original inline blocks rebuilt their map.
		template <typename ElementType, typename RebuildMapFuncType>
		void CompactTombstonedSlots(TArray<ElementType>& Slots, TSet<int32>& PendingRemovedIndices,
			TSet<int32>& TouchedIndices, RebuildMapFuncType&& RebuildMap)
		{
			if (PendingRemovedIndices.Num() == 0)
			{
				return;
			}

			TArray<int32> OldToNew;
			OldToNew.SetNumUninitialized(Slots.Num());
			int32 WriteIndex = 0;
			for (int32 ReadIndex = 0; ReadIndex < Slots.Num(); ++ReadIndex)
			{
				if (PendingRemovedIndices.Contains(ReadIndex))
				{
					OldToNew[ReadIndex] = INDEX_NONE;
					continue;
				}
				OldToNew[ReadIndex] = WriteIndex;
				if (WriteIndex != ReadIndex)
				{
					Slots[WriteIndex] = MoveTemp(Slots[ReadIndex]);
				}
				++WriteIndex;
			}
			Slots.SetNum(WriteIndex);

			TSet<int32> Remapped;
			Remapped.Reserve(TouchedIndices.Num());
			for (const int32 OldIndex : TouchedIndices)
			{
				if (OldToNew.IsValidIndex(OldIndex) && OldToNew[OldIndex] != INDEX_NONE)
				{
					Remapped.Add(OldToNew[OldIndex]);
				}
			}
			TouchedIndices = MoveTemp(Remapped);

			RebuildMap();
			PendingRemovedIndices.Reset();
		}
	}

	void FRealtimeMeshLODProxy::CompactPendingRemovals()
	{
		// Buffer sets: stable compaction of the tombstoned slots, then one map rebuild.
		CompactTombstonedSlots(BufferSets, PendingRemovedBufferSetIndices, TouchedBufferSetIndices,
			[this]() { RebuildBufferSetMap(); });

		// Sections: same compaction + remap. Independent of the buffer-set pass above —
		// section slots hold their owning BS's slot index (not an array index), so
		// buffer-set compaction does not disturb them.
		CompactTombstonedSlots(Sections, PendingRemovedSectionIndices, TouchedSectionIndices,
			[this]() { RebuildSectionMap(); });
	}

	void FRealtimeMeshLODProxy::Reset()
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(FRealtimeMeshLODProxy::Reset);

		// Sections are about to be dropped entirely — no need to call Reset() on each
		// when the TSharedPtr destruction (via FRealtimeMeshRenderResourceDeleter) will
		// release any RHI state anyway. Skipping the per-section Reset also avoids a
		// gratuitous COW when the LOD is part of a snapshot being torn down.
		Sections.Empty();
		SectionMap.Empty();
		ActiveSectionIndicesByBufferSet.Empty();
		AllRenderEntries.Empty();
		StaticRenderEntries.Empty();
		DynamicRenderEntries.Empty();
		RayTracingRenderEntries.Empty();
		RayTracingBufferSetIndices.Empty();
		ActiveBufferSetIndices.Empty();
		TouchedBufferSetIndices.Reset();
		TouchedSectionIndices.Reset();
		PendingRemovedSectionIndices.Reset();
		PendingRemovedBufferSetIndices.Reset();

		BufferSets.Empty();
		BufferSetMap.Empty();

		Config = FRealtimeMeshLODConfig();
		DrawMask = FRealtimeMeshDrawMask();

#if RHI_RAYTRACING
		StaticRayTraceBufferSet = INDEX_NONE;
#endif
	}
}
