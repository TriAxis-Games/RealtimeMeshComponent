// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "RealtimeMeshCore.h"
#include "RealtimeMeshData.h"
#include "RealtimeMeshLOD.h"
#include "RenderProxy/RealtimeMeshProxyCommandBatch.h"


namespace RealtimeMesh
{

	struct REALTIMEMESHCOMPONENT_API FRealtimeMeshLockContext
	{
	protected:
		FRealtimeMeshLockContext() = default;
		UE_NONCOPYABLE(FRealtimeMeshLockContext);
	};


	struct REALTIMEMESHCOMPONENT_API FRealtimeMeshAccessContext : public FRealtimeMeshLockContext
	{
		FRealtimeMeshScopeGuardRead ReadGuard;
		FRealtimeMeshContextRef Resources;

		FRealtimeMeshAccessContext(const TSharedRef<const FRealtimeMesh>& InMesh);

		FRealtimeMeshAccessContext(const FRealtimeMeshContextRef& InResources);

		UE_NONCOPYABLE(FRealtimeMeshAccessContext);
	};


	struct REALTIMEMESHCOMPONENT_API FRealtimeMeshStructuredDirtyTree
	{
	private:
		struct SectionGroupEntry
		{
			TSet<int32> Sections;        // section slot indices
			bool bDirty;
		};

		struct LODEntry
		{
			TMap<int32, SectionGroupEntry> SectionGroups;  // keyed by buffer-set slot index
			bool bDirty;
		};


		TFixedLODArray<LODEntry> DirtyTree;

	public:
		FRealtimeMeshStructuredDirtyTree()
		{
			DirtyTree.SetNum(REALTIME_MESH_MAX_LODS);
		}

		void Flag(const FRealtimeMeshLODKey& LODKey)
		{
			DirtyTree[LODKey].bDirty = true;
		}

		void Flag(const FRealtimeMeshBufferSetKey& SectionGroupKey)
		{
			DirtyTree[SectionGroupKey.LOD()].SectionGroups.FindOrAdd(SectionGroupKey.Index()).bDirty = true;
		}

		void Flag(const FRealtimeMeshSectionKey& SectionKey)
		{
			// Bucket by the section's recorded buffer-set slot. SectionGroup() returns
			// a key built from the section key's cached BufferSetSlotIndex.
			DirtyTree[SectionKey.LOD()].SectionGroups.FindOrAdd(SectionKey.SectionGroup().Index()).Sections.Add(SectionKey.Index());
		}

		bool IsDirty(const FRealtimeMeshLODKey& LODKey, bool bIncludeChildren = true) const
		{
			auto& LODEntry = DirtyTree[LODKey];
			return LODEntry.bDirty || (bIncludeChildren && !LODEntry.SectionGroups.IsEmpty());
		}

		bool IsDirty(const FRealtimeMeshBufferSetKey& SectionGroup, bool bIncludeChildren = true) const
		{
			if (auto* SectionGroupEntry = DirtyTree[SectionGroup.LOD()].SectionGroups.Find(SectionGroup.Index()))
			{
				return SectionGroupEntry->bDirty || (bIncludeChildren && !SectionGroupEntry->Sections.IsEmpty());
			}
			return false;
		}

		bool IsDirty(const FRealtimeMeshSectionKey& SectionKey) const
		{
			if (auto* SectionGroupEntry = DirtyTree[SectionKey.LOD()].SectionGroups.Find(SectionKey.SectionGroup().Index()))
			{
				return SectionGroupEntry->Sections.Contains(SectionKey.Index());
			}
			return false;
		}

		/**
		 * Visits every dirty entry: SectionGroupSlot is INDEX_NONE for LOD-level dirt, otherwise
		 * the buffer-set slot index of a dirty section group (flagged directly or via a dirty
		 * section). Slot indices must be resolved back to full keys by the caller (the tree only
		 * stores indices, not names).
		 */
		void ForEachDirtyEntry(TFunctionRef<void(int32 LODIndex, int32 SectionGroupSlot)> Visitor) const
		{
			for (int32 LODIndex = 0; LODIndex < DirtyTree.Num(); LODIndex++)
			{
				const LODEntry& Entry = DirtyTree[LODIndex];
				if (Entry.bDirty)
				{
					Visitor(LODIndex, INDEX_NONE);
				}
				for (const auto& GroupPair : Entry.SectionGroups)
				{
					if (GroupPair.Value.bDirty || !GroupPair.Value.Sections.IsEmpty())
					{
						Visitor(LODIndex, GroupPair.Key);
					}
				}
			}
		}

	};

	struct REALTIMEMESHCOMPONENT_API FRealtimeMeshStructuredStreamDirtyTree
	{
	private:
		TMap<FRealtimeMeshBufferSetKey, TSet<FRealtimeMeshStreamKey>> DirtyStreams;

	public:

		void Flag(const FRealtimeMeshBufferSetKey& SectionGroupKey, const FRealtimeMeshStreamKey& StreamKey)
		{
			DirtyStreams.FindOrAdd(SectionGroupKey).Add(StreamKey);
		}

		bool HasDirtyStreams(const FRealtimeMeshBufferSetKey& SectionGroup) const
		{
			const TSet<FRealtimeMeshStreamKey>* Found = DirtyStreams.Find(SectionGroup);
			return Found != nullptr && !Found->IsEmpty();
		}

		bool HasAnyDirtyStreams(const FRealtimeMeshLODKey& LODKey) const
		{
			for (const auto& Pair : DirtyStreams)
			{
				if (Pair.Key.LOD() == LODKey && !Pair.Value.IsEmpty())
				{
					return true;
				}
			}
			return false;
		}

		const TSet<FRealtimeMeshStreamKey>& GetDirtyStreams(const FRealtimeMeshBufferSetKey& SectionGroup) const
		{
			return DirtyStreams.FindChecked(SectionGroup);
		}

		/** Visits every section group that has at least one dirty stream this batch. */
		void ForEachDirtyGroup(TFunctionRef<void(const FRealtimeMeshBufferSetKey&)> Visitor) const
		{
			for (const auto& Pair : DirtyStreams)
			{
				if (!Pair.Value.IsEmpty())
				{
					Visitor(Pair.Key);
				}
			}
		}
	};

	struct REALTIMEMESHCOMPONENT_API FRealtimeMeshSectionRangeDirtyTree
	{
	private:
		TSet<FRealtimeMeshSectionKey> Dirty;

	public:

		void Flag(const FRealtimeMeshSectionKey& SectionKey)
		{
			Dirty.Add(SectionKey);
		}

		bool IsDirty(const FRealtimeMeshSectionKey& SectionKey) const
		{
			return Dirty.Contains(SectionKey);
		}

		// Hierarchical queries used by the FinalizeUpdate descent gates. Linear scans are fine
		// here: the set only holds sections whose range changed in the current batch.
		bool IsDirty(const FRealtimeMeshBufferSetKey& SectionGroupKey) const
		{
			for (const FRealtimeMeshSectionKey& SectionKey : Dirty)
			{
				if (SectionKey.SectionGroup() == SectionGroupKey)
				{
					return true;
				}
			}
			return false;
		}

		bool IsDirty(const FRealtimeMeshLODKey& LODKey) const
		{
			for (const FRealtimeMeshSectionKey& SectionKey : Dirty)
			{
				if (SectionKey.LOD() == LODKey)
				{
					return true;
				}
			}
			return false;
		}

		/** Visits every section whose stream range changed this batch. */
		void ForEach(TFunctionRef<void(const FRealtimeMeshSectionKey&)> Visitor) const
		{
			for (const FRealtimeMeshSectionKey& SectionKey : Dirty)
			{
				Visitor(SectionKey);
			}
		}
	};


	struct REALTIMEMESHCOMPONENT_API FRealtimeMeshUpdateState
	{
		FRealtimeMeshStructuredDirtyTree BoundsDirtyTree;
		FRealtimeMeshStructuredDirtyTree ConfigDirtyTree;
		FRealtimeMeshStructuredStreamDirtyTree StreamDirtyTree;
		FRealtimeMeshSectionRangeDirtyTree StreamRangeDirtyTree;

		uint32 bNeedsBoundsUpdate : 1;
		uint32 bNeedsCollisionUpdate : 1;
		uint32 bNeedsRenderProxyUpdate : 1;
		// Set by the Set/Clear DistanceField / CardRepresentation paths so commit observers
		// (OnUpdateCommitted subscribers, e.g. net sync) can see the change; the payloads
		// themselves live on the mesh, not in the update state.
		uint32 bDistanceFieldDirty : 1;
		uint32 bCardRepresentationDirty : 1;

		FRealtimeMeshUpdateState()
			: bNeedsBoundsUpdate(false)
			, bNeedsCollisionUpdate(false)
			, bNeedsRenderProxyUpdate(false)
			, bDistanceFieldDirty(false)
			, bCardRepresentationDirty(false)
		{ }
	};


	struct REALTIMEMESHCOMPONENT_API FRealtimeMeshUpdateContext : public FRealtimeMeshLockContext
	{
	private:
		FRealtimeMeshScopeGuardWrite WriteGuard;
		FRealtimeMeshProxyUpdateBuilder ProxyBuilder;
		FRealtimeMeshContextRef Resources;
		FRealtimeMeshUpdateStateRef UpdateState;
		TUniquePtr<FRHICommandList> RHICmdList;
		// The RHI command list is created lazily on first GetRHICmdList() use, so its
		// validity can no longer double as the "not yet committed" sentinel. Track commit
		// state explicitly so the destructor finalizes exactly once even when nothing was
		// recorded (config-only updates and the in-place fast path never touch the list).
		bool bCommitted;

	public:
		FRealtimeMeshUpdateContext(const TSharedRef<FRealtimeMesh>& InMesh);

		FRealtimeMeshUpdateContext(const FRealtimeMeshContextRef& InResources);

		~FRealtimeMeshUpdateContext();

		UE_NONCOPYABLE(FRealtimeMeshUpdateContext)

		bool ShouldUpdateProxy() const { return ProxyBuilder.IsValid(); }

		FRealtimeMeshProxyUpdateBuilder* GetProxyBuilder() { return ProxyBuilder.IsValid()? &ProxyBuilder : nullptr; }
		operator FRealtimeMeshProxyUpdateBuilder& () { return ProxyBuilder; }

		FRHICommandList& GetRHICmdList();

		FRealtimeMeshUpdateState& GetState() { return *UpdateState; }
		template<typename UpdateStateType>
		UpdateStateType& GetState() { return static_cast<UpdateStateType&>(GetState()); }


		TFuture<ERealtimeMeshProxyUpdateStatus> Commit();
	};


	/**
	 * Shared base for the two task accumulator flavors. Holds the task list and
	 * the per-element add helpers (walk to LOD / SectionGroup / Section with
	 * ensure + error log). Doesn't own a lock — that's deliberate: builders
	 * are sometimes long-lived (e.g. the async-pipeline state in
	 * RealtimeMeshExt's factory accumulates tasks across milliseconds-scale
	 * async work, and holding the write lock that long would deadlock
	 * everything else). Concrete builders construct their LockContext locally
	 * at Execute / Commit time.
	 *
	 * Tasks captured here are shaped as
	 *     void(FRealtimeMeshLockContext&, const FRealtimeMesh&)
	 * with const navigation through the mesh tree. Element accessors
	 * (Mesh.GetLOD, LOD.GetSectionGroup, BufferSet.GetSection) return mutable
	 * shared_ptrs even from a const parent, so write tasks added via the
	 * FRealtimeMeshUpdateBuilder wrapper safely downcast the context and
	 * navigate to mutable elements at run time.
	 */
	struct REALTIMEMESHCOMPONENT_API FRealtimeMeshTaskBuilderBase
	{
	public:
		using TaskFunctionType = TUniqueFunction<void(FRealtimeMeshLockContext&, const FRealtimeMesh&)>;

	protected:
		TArray<TaskFunctionType> Tasks;

		FRealtimeMeshTaskBuilderBase() = default;
		UE_NONCOPYABLE(FRealtimeMeshTaskBuilderBase)

		// Helper for derived classes to invoke from Execute / Commit once
		// they've constructed their concrete LockContext.
		void RunTasks(FRealtimeMeshLockContext& LockContext, const FRealtimeMesh& Mesh);

	public:
		~FRealtimeMeshTaskBuilderBase() = default;

		void AddMeshTask(TaskFunctionType&& Function);

		template <typename MeshType>
		void AddMeshTask(TUniqueFunction<void(FRealtimeMeshLockContext&, const MeshType&)>&& Function)
		{
			AddMeshTask([Func = MoveTemp(Function)](FRealtimeMeshLockContext& LockContext, const FRealtimeMesh& Mesh)
			{
				Func(LockContext, static_cast<const MeshType&>(Mesh));
			});
		}

		void AddLODTask(const FRealtimeMeshLODKey& LODKey, TUniqueFunction<void(FRealtimeMeshLockContext&, const FRealtimeMeshLOD&)>&& Function);

		template <typename LODType>
		void AddLODTask(const FRealtimeMeshLODKey& LODKey, TUniqueFunction<void(FRealtimeMeshLockContext&, const LODType&)>&& Function)
		{
			AddLODTask(LODKey, [Func = MoveTemp(Function)](FRealtimeMeshLockContext& LockContext, const FRealtimeMeshLOD& LOD)
			{
				Func(LockContext, static_cast<const LODType&>(LOD));
			});
		}

		void AddSectionGroupTask(const FRealtimeMeshBufferSetKey& SectionGroupKey, TUniqueFunction<void(FRealtimeMeshLockContext&, const FRealtimeMeshBufferSet&)>&& Function);

		template <typename SectionGroupType>
		void AddSectionGroupTask(const FRealtimeMeshBufferSetKey& SectionGroupKey, TUniqueFunction<void(FRealtimeMeshLockContext&, const SectionGroupType&)>&& Function)
		{
			AddSectionGroupTask(SectionGroupKey, [Func = MoveTemp(Function)](FRealtimeMeshLockContext& LockContext, const FRealtimeMeshBufferSet& SectionGroup)
			{
				Func(LockContext, static_cast<const SectionGroupType&>(SectionGroup));
			});
		}

		void AddSectionTask(const FRealtimeMeshSectionKey& SectionKey, TUniqueFunction<void(FRealtimeMeshLockContext&, const FRealtimeMeshSection&)>&& Function);

		template <typename SectionType>
		void AddSectionTask(const FRealtimeMeshSectionKey& SectionKey, TUniqueFunction<void(FRealtimeMeshLockContext&, const SectionType&)>&& Function)
		{
			AddSectionTask(SectionKey, [Func = MoveTemp(Function)](FRealtimeMeshLockContext& LockContext, const FRealtimeMeshSection& Section)
			{
				Func(LockContext, static_cast<const SectionType&>(Section));
			});
		}
	};


	/**
	 * Read-only task accumulator. Execute(Mesh) acquires the read lock,
	 * runs every queued task against it, releases on exit.
	 */
	struct REALTIMEMESHCOMPONENT_API FRealtimeMeshAccessor : public FRealtimeMeshTaskBuilderBase
	{
	public:
		FRealtimeMeshAccessor() = default;

		void Execute(const TSharedRef<const FRealtimeMesh>& Mesh);
	};


	/*
	 *	Helper for creating a batch update to the RMC. This is used to group multiple things
	 *	like updating several lods, section groups, sections, or distancefield/cards as well as collision into
	 *	a single update that will lock once and apply everything at once.
	 *	This will also make it easy to have those calls batch render thread actions so those all get applied at once as well.
	 *
	 *	Commit(Mesh) acquires the write lock, runs every queued task, publishes
	 *	the proxy update, releases on exit. Inherits the const "read task"
	 *	overloads from FRealtimeMeshTaskBuilderBase (useful for inspect-then-
	 *	conditionally-update patterns); also exposes write-flavored Add*Task
	 *	overloads taking an FRealtimeMeshUpdateContext& and a mutable element ref.
	 */
	struct REALTIMEMESHCOMPONENT_API FRealtimeMeshUpdateBuilder : public FRealtimeMeshTaskBuilderBase
	{
	public:
		FRealtimeMeshUpdateBuilder() = default;

		TFuture<ERealtimeMeshProxyUpdateStatus> Commit(const TSharedRef<FRealtimeMesh>& Mesh);

		// Keep the inherited const-task overloads visible alongside the
		// write-flavored ones below (C++ name hiding rules).
		using FRealtimeMeshTaskBuilderBase::AddMeshTask;
		using FRealtimeMeshTaskBuilderBase::AddLODTask;
		using FRealtimeMeshTaskBuilderBase::AddSectionGroupTask;
		using FRealtimeMeshTaskBuilderBase::AddSectionTask;

		// Write-flavored entry points. Caller's lambda gets a concrete
		// FRealtimeMeshUpdateContext& and a mutable element ref. Internally
		// these wrap into the base's read-task shape and downcast at run time;
		// the downcast is safe because UpdateBuilder::Commit always constructs
		// an UpdateContext, and only this class stores these wrapped lambdas.
		void AddMeshTask(TUniqueFunction<void(FRealtimeMeshUpdateContext&, FRealtimeMesh&)>&& Function);

		template <typename MeshType>
		void AddMeshTask(TUniqueFunction<void(FRealtimeMeshUpdateContext&, MeshType&)>&& Function)
		{
			AddMeshTask([Func = MoveTemp(Function)](FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMesh& Mesh)
			{
				Func(UpdateContext, static_cast<MeshType&>(Mesh));
			});
		}

		void AddLODTask(const FRealtimeMeshLODKey& LODKey, TUniqueFunction<void(FRealtimeMeshUpdateContext&, FRealtimeMeshLOD&)>&& Function);

		template <typename LODType>
		void AddLODTask(const FRealtimeMeshLODKey& LODKey, TUniqueFunction<void(FRealtimeMeshUpdateContext&, LODType&)>&& Function)
		{
			AddLODTask(LODKey, [Func = MoveTemp(Function)](FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshLOD& LOD)
			{
				Func(UpdateContext, static_cast<LODType&>(LOD));
			});
		}

		void AddSectionGroupTask(const FRealtimeMeshBufferSetKey& SectionGroupKey, TUniqueFunction<void(FRealtimeMeshUpdateContext&, FRealtimeMeshBufferSet&)>&& Function);

		template <typename SectionGroupType>
		void AddSectionGroupTask(const FRealtimeMeshBufferSetKey& SectionGroupKey, TUniqueFunction<void(FRealtimeMeshUpdateContext&, SectionGroupType&)>&& Function)
		{
			AddSectionGroupTask(SectionGroupKey, [Func = MoveTemp(Function)](FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshBufferSet& SectionGroup)
			{
				Func(UpdateContext, static_cast<SectionGroupType&>(SectionGroup));
			});
		}

		void AddSectionTask(const FRealtimeMeshSectionKey& SectionKey, TUniqueFunction<void(FRealtimeMeshUpdateContext&, FRealtimeMeshSection&)>&& Function);

		template <typename SectionType>
		void AddSectionTask(const FRealtimeMeshSectionKey& SectionKey, TUniqueFunction<void(FRealtimeMeshUpdateContext&, SectionType&)>&& Function)
		{
			AddSectionTask(SectionKey, [Func = MoveTemp(Function)](FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshSection& Section)
			{
				Func(UpdateContext, static_cast<SectionType&>(Section));
			});
		}
	};
}
