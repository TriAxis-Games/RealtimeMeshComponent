// Copyright (c) 2015-2024 TriAxis Games, L.L.C. All Rights Reserved.

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
		FRealtimeMeshSharedResourcesRef Resources;

		FRealtimeMeshAccessContext(const TSharedRef<const FRealtimeMesh>& InMesh);

		FRealtimeMeshAccessContext(const FRealtimeMeshSharedResourcesRef& InResources);

		UE_NONCOPYABLE(FRealtimeMeshAccessContext);
	};


	struct REALTIMEMESHCOMPONENT_API FRealtimeMeshStructuredDirtyTree
	{
	private:
		struct SectionGroupEntry
		{
			TSet<FName> Sections;
			bool bDirty;
		};

		struct LODEntry
		{
			TMap<FName, SectionGroupEntry> SectionGroups;
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

		void Flag(const FRealtimeMeshSectionGroupKey& SectionGroupKey)
		{
			DirtyTree[SectionGroupKey.LOD()].SectionGroups.FindOrAdd(SectionGroupKey.Name()).bDirty = true;
		}

		void Flag(const FRealtimeMeshSectionKey& SectionKey)
		{
			DirtyTree[SectionKey.LOD()].SectionGroups.FindOrAdd(SectionKey.SectionGroup().Name()).Sections.Add(SectionKey.Name());
		}

		bool IsDirty(const FRealtimeMeshLODKey& LODKey, bool bIncludeChildren = true) const
		{
			auto& LODEntry = DirtyTree[LODKey];
			return LODEntry.bDirty || (bIncludeChildren && !LODEntry.SectionGroups.IsEmpty());
		}

		bool IsDirty(const FRealtimeMeshSectionGroupKey& SectionGroup, bool bIncludeChildren = true) const
		{
			if (auto* SectionGroupEntry = DirtyTree[SectionGroup.LOD()].SectionGroups.Find(SectionGroup.Name()))
			{
				return SectionGroupEntry->bDirty || (bIncludeChildren && !SectionGroupEntry->Sections.IsEmpty());
			}
			return false;
		}

		bool IsDirty(const FRealtimeMeshSectionKey& SectionKey) const
		{
			if (auto* SectionGroupEntry = DirtyTree[SectionKey.LOD()].SectionGroups.Find(SectionKey.SectionGroup().Name()))
			{
				return SectionGroupEntry->Sections.Contains(SectionKey.Name());
			}
			return false;			
		}
		
	};

	struct REALTIMEMESHCOMPONENT_API FRealtimeMeshStructuredStreamDirtyTree
	{		
	private:
		TMap<FRealtimeMeshSectionGroupKey, TSet<FRealtimeMeshStreamKey>> DirtyStreams;

	public:
		
		void Flag(const FRealtimeMeshSectionGroupKey& SectionGroupKey, const FRealtimeMeshStreamKey& StreamKey)
		{
			DirtyStreams.FindOrAdd(SectionGroupKey).Add(StreamKey);
		}

		bool HasDirtyStreams(const FRealtimeMeshSectionGroupKey& SectionGroup) const
		{
			return DirtyStreams.Contains(SectionGroup) && !DirtyStreams[SectionGroup].IsEmpty();
		}

		const TSet<FRealtimeMeshStreamKey>& GetDirtyStreams(const FRealtimeMeshSectionGroupKey& SectionGroup) const
		{
			return DirtyStreams.FindChecked(SectionGroup);
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

		FRealtimeMeshUpdateState()
			: bNeedsBoundsUpdate(false)
			, bNeedsCollisionUpdate(false)
			, bNeedsRenderProxyUpdate(false)
		{ }
	};


	struct REALTIMEMESHCOMPONENT_API FRealtimeMeshUpdateContext : public FRealtimeMeshLockContext
	{
	private:
		FRealtimeMeshScopeGuardWrite WriteGuard;
		FRealtimeMeshProxyUpdateBuilder ProxyBuilder;
		FRealtimeMeshSharedResourcesRef Resources;
		FRealtimeMeshUpdateStateRef UpdateState;
#if RMC_ENGINE_ABOVE_5_5
		TUniquePtr<FRHICommandList> RHICmdList;
#else
		TOptional<FRHIAsyncCommandList> RHICmdList;
#endif

	public:
		FRealtimeMeshUpdateContext(const TSharedRef<FRealtimeMesh>& InMesh);

		FRealtimeMeshUpdateContext(const FRealtimeMeshSharedResourcesRef& InResources);

		~FRealtimeMeshUpdateContext();

		UE_NONCOPYABLE(FRealtimeMeshUpdateContext)

		bool ShouldUpdateProxy() const { return ProxyBuilder.IsValid(); }
		
		FRealtimeMeshProxyUpdateBuilder* GetProxyBuilder() { return ProxyBuilder.IsValid()? &ProxyBuilder : nullptr; }
		operator FRealtimeMeshProxyUpdateBuilder& () { return ProxyBuilder; }

#if RMC_ENGINE_ABOVE_5_5
		FRHICommandList& GetRHICmdList();
#else
		FRHIAsyncCommandList& GetRHICmdList();
#endif
		
		FRealtimeMeshUpdateState& GetState() { return *UpdateState; }
		template<typename UpdateStateType>
		UpdateStateType& GetState() { return static_cast<UpdateStateType&>(GetState()); }


		TFuture<ERealtimeMeshProxyUpdateStatus> Commit();
	};

	
	/*
	 *	Helper for creating a batch update to the RMC. This is used to group multiple things
	 *	like updating several lods, section groups, sections, or distancefield/cards as well as collision into
	 *	a single update that will lock once and apply everything at once.
	 *	This will also make it easy to have those calls batch render thread actions so those all get applied at once as well.
	 */
	struct REALTIMEMESHCOMPONENT_API FRealtimeMeshUpdateBuilder
	{
	public:
		using TaskFunctionType = TUniqueFunction<void(FRealtimeMeshUpdateContext&, FRealtimeMesh&)>;
	private:
		TArray<TaskFunctionType> Tasks;

	public:
		FRealtimeMeshUpdateBuilder() = default;
		UE_NONCOPYABLE(FRealtimeMeshUpdateBuilder)
		
		TFuture<ERealtimeMeshProxyUpdateStatus> Commit(const TSharedRef<FRealtimeMesh>& Mesh);

		void AddMeshTask(TUniqueFunction<void(FRealtimeMeshUpdateContext&, FRealtimeMesh&)>&& Function);

		template <typename MeshType>
		void AddMeshTask(TUniqueFunction<void(FRealtimeMeshUpdateContext&, MeshType&)>&& Function)
		{
			AddMeshTask([Func = MoveTemp(Function)](FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMesh& LOD)
			{
				Func(UpdateContext, static_cast<MeshType&>(LOD));
			});
		}

		void AddLODTask(const FRealtimeMeshLODKey& LODKey, TUniqueFunction<void(FRealtimeMeshUpdateContext&, FRealtimeMeshLOD&)>&& Function);

		template <typename LODProxyType>
		void AddLODTask(const FRealtimeMeshLODKey& LODKey, TUniqueFunction<void(FRealtimeMeshUpdateContext&, LODProxyType&)>&& Function)
		{
			AddLODTask(LODKey, [Func = MoveTemp(Function)](FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshLOD& LOD)
			{
				Func(UpdateContext, static_cast<LODProxyType&>(LOD));
			});
		}

		void AddSectionGroupTask(const FRealtimeMeshSectionGroupKey& SectionGroupKey, TUniqueFunction<void(FRealtimeMeshUpdateContext&, FRealtimeMeshSectionGroup&)>&& Function);

		template <typename SectionGroupProxyType>
		void AddSectionGroupTask(const FRealtimeMeshSectionGroupKey& SectionGroupKey, TUniqueFunction<void(FRealtimeMeshUpdateContext&, SectionGroupProxyType&)>&& Function)
		{
			AddSectionGroupTask(SectionGroupKey, [Func = MoveTemp(Function)](FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshSectionGroup& SectionGroup)
			{
				Func(UpdateContext, static_cast<SectionGroupProxyType&>(SectionGroup));
			});
		}

		void AddSectionTask(const FRealtimeMeshSectionKey& SectionKey, TUniqueFunction<void(FRealtimeMeshUpdateContext&, FRealtimeMeshSection&)>&& Function);

		template <typename SectionProxyType>
		void AddSectionTask(const FRealtimeMeshSectionKey& SectionKey, TUniqueFunction<void(FRealtimeMeshUpdateContext&, SectionProxyType&)>&& Function)
		{
			AddSectionTask(SectionKey, [Func = MoveTemp(Function)](FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshSection& Section)
			{
				Func(UpdateContext, static_cast<SectionProxyType&>(Section));
			});
		}
	};




	struct REALTIMEMESHCOMPONENT_API FRealtimeMeshAccessor
	{
	public:
		using TaskFunctionType = TUniqueFunction<void(const FRealtimeMeshAccessContext&, const FRealtimeMesh&)>;
	private:
		TArray<TaskFunctionType> Tasks;

	public:
		FRealtimeMeshAccessor() = default;
		UE_NONCOPYABLE(FRealtimeMeshAccessor)
		
		void Execute(const TSharedRef<const FRealtimeMesh>& Mesh);

		void AddMeshTask(TUniqueFunction<void(const FRealtimeMeshAccessContext&, const FRealtimeMesh&)>&& Function);

		template <typename MeshType>
		void AddMeshTask(TUniqueFunction<void(const FRealtimeMeshAccessContext&, const MeshType&)>&& Function)
		{
			AddMeshTask([Func = MoveTemp(Function)](const FRealtimeMeshAccessContext& LockContext, const FRealtimeMesh& LOD)
			{
				Func(LockContext, static_cast<const MeshType&>(LOD));
			});
		}

		void AddLODTask(const FRealtimeMeshLODKey& LODKey, TUniqueFunction<void(const FRealtimeMeshAccessContext&, const FRealtimeMeshLOD&)>&& Function);

		template <typename LODProxyType>
		void AddLODTask(const FRealtimeMeshLODKey& LODKey, TUniqueFunction<void(const FRealtimeMeshAccessContext&, const LODProxyType&)>&& Function)
		{
			AddLODTask(LODKey, [Func = MoveTemp(Function)](const FRealtimeMeshAccessContext& LockContext, const FRealtimeMeshLOD& LOD)
			{
				Func(LockContext, static_cast<const LODProxyType&>(LOD));
			});
		}

		void AddSectionGroupTask(const FRealtimeMeshSectionGroupKey& SectionGroupKey, TUniqueFunction<void(const FRealtimeMeshAccessContext&, const FRealtimeMeshSectionGroup&)>&& Function);

		template <typename SectionGroupProxyType>
		void AddSectionGroupTask(const FRealtimeMeshSectionGroupKey& SectionGroupKey, TUniqueFunction<void(const FRealtimeMeshAccessContext&, const SectionGroupProxyType&)>&& Function)
		{
			AddSectionGroupTask(SectionGroupKey, [Func = MoveTemp(Function)](const FRealtimeMeshAccessContext& LockContext, const FRealtimeMeshSectionGroup& SectionGroup)
			{
				Func(LockContext, static_cast<const SectionGroupProxyType&>(SectionGroup));
			});
		}

		void AddSectionTask(const FRealtimeMeshSectionKey& SectionKey, TUniqueFunction<void(const FRealtimeMeshAccessContext&, const FRealtimeMeshSection&)>&& Function);

		template <typename SectionProxyType>
		void AddSectionTask(const FRealtimeMeshSectionKey& SectionKey, TUniqueFunction<void(const FRealtimeMeshAccessContext&, const SectionProxyType&)>&& Function)
		{
			AddSectionTask(SectionKey, [Func = MoveTemp(Function)](const FRealtimeMeshAccessContext& LockContext, const FRealtimeMeshSection& Section)
			{
				Func(LockContext, static_cast<const SectionProxyType&>(Section));
			});
		}
	};
}
