// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "RealtimeMeshCore.h"
#include "RealtimeMeshSectionProxy.h"


namespace RealtimeMesh
{
	struct FRealtimeMeshProxyCommandBatch
	{
	private:
		using TaskFunctionType = TUniqueFunction<void(FRealtimeMeshProxy&)>;
		TArray<TaskFunctionType> Tasks;
		FRealtimeMeshPtr Mesh;
		bool bRequiresProxyRecreate;

	public:
		FRealtimeMeshProxyCommandBatch(const FRealtimeMeshSharedResourcesPtr& InSharedResources, bool bInRequiresProxyRecreate = false);
		FRealtimeMeshProxyCommandBatch(const FRealtimeMeshPtr& InMesh, bool bInRequiresProxyRecreate = false);

		FRealtimeMeshProxyCommandBatch(const FRealtimeMeshProxyCommandBatch&) = delete;
		FRealtimeMeshProxyCommandBatch(FRealtimeMeshProxyCommandBatch&&) = delete;
		FRealtimeMeshProxyCommandBatch& operator=(const FRealtimeMeshProxyCommandBatch&) = delete;
		FRealtimeMeshProxyCommandBatch& operator=(FRealtimeMeshProxyCommandBatch&&) = delete;

		operator bool() const { return Mesh.IsValid(); }

		TFuture<ERealtimeMeshProxyUpdateStatus> Commit();

		void MarkForProxyRecreate() { bRequiresProxyRecreate = true; }
		void ClearProxyRecreate() { bRequiresProxyRecreate = false; }

		void AddMeshTask(TUniqueFunction<void(FRealtimeMeshProxy&)>&& Function, bool bInRequiresProxyRecreate = true);

		template <typename MeshType>
		void AddMeshTask(TUniqueFunction<void(MeshType&)>&& Function, bool bInRequiresProxyRecreate = true)
		{
			AddMeshTask([Func = MoveTemp(Function)](FRealtimeMeshProxy& LOD)
			{
				Func(static_cast<MeshType&>(LOD));
			}, bInRequiresProxyRecreate);
		}

		void AddLODTask(const FRealtimeMeshLODKey& LODKey, TUniqueFunction<void(FRealtimeMeshLODProxy&)>&& Function, bool bInRequiresProxyRecreate = true);

		template <typename LODProxyType>
		void AddLODTask(const FRealtimeMeshLODKey& LODKey, TUniqueFunction<void(LODProxyType&)>&& Function, bool bInRequiresProxyRecreate = true)
		{
			AddLODTask(LODKey, [Func = MoveTemp(Function)](FRealtimeMeshLODProxy& LOD)
			{
				Func(static_cast<LODProxyType&>(LOD));
			}, bInRequiresProxyRecreate);
		}

		void AddSectionGroupTask(const FRealtimeMeshSectionGroupKey& SectionGroupKey, TUniqueFunction<void(FRealtimeMeshSectionGroupProxy&)>&& Function,
		                         bool bInRequiresProxyRecreate = true);

		template <typename SectionGroupProxyType>
		void AddSectionGroupTask(const FRealtimeMeshSectionGroupKey& SectionGroupKey, TUniqueFunction<void(SectionGroupProxyType&)>&& Function,
		                         bool bInRequiresProxyRecreate = true)
		{
			AddSectionGroupTask(SectionGroupKey, [Func = MoveTemp(Function)](FRealtimeMeshSectionGroupProxy& SectionGroup)
			{
				Func(static_cast<SectionGroupProxyType&>(SectionGroup));
			}, bInRequiresProxyRecreate);
		}

		void AddSectionTask(const FRealtimeMeshSectionKey& SectionKey, TUniqueFunction<void(FRealtimeMeshSectionProxy&)>&& Function, bool bInRequiresProxyRecreate = true);

		template <typename SectionProxyType>
		void AddSectionTask(const FRealtimeMeshSectionKey& SectionKey, TUniqueFunction<void(SectionProxyType&)>&& Function, bool bInRequiresProxyRecreate = true)
		{
			AddSectionTask(SectionKey, [Func = MoveTemp(Function)](FRealtimeMeshSectionProxy& Section)
			{
				Func(static_cast<SectionProxyType&>(Section));
			}, bInRequiresProxyRecreate);
		}
	};
}
