// Copyright (c) 2015-2024 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "RealtimeMeshCore.h"
#include "RealtimeMeshData.h"
#include "RealtimeMeshLOD.h"
#include "RenderProxy/RealtimeMeshProxyCommandBatch.h"


namespace RealtimeMesh
{
	struct REALTIMEMESHCOMPONENT_API FRealtimeMeshUpdateContext
	{
		FRealtimeMeshScopeGuardWrite WriteGuard;
		FRealtimeMeshProxyUpdateBuilder ProxyBuilder;
		FRealtimeMeshSharedResourcesRef Resources;

		FRealtimeMeshUpdateContext(const TSharedRef<FRealtimeMesh>& InMesh)
			: WriteGuard(InMesh->GetSharedResources()->GetGuard())
			, ProxyBuilder(!InMesh->GetRenderProxy().IsValid())
			, Resources(InMesh->GetSharedResources())
		{ }
		
		FRealtimeMeshUpdateContext(const FRealtimeMeshSharedResourcesRef& InResources)
			: WriteGuard(InResources->GetGuard())
			, ProxyBuilder(!InResources->GetOwner().IsValid() || !InResources->GetOwner()->GetRenderProxy().IsValid())
			, Resources(InResources)
		{ }

		FRealtimeMeshProxyUpdateBuilder& GetProxyBuilder() { return ProxyBuilder; }
		operator FRealtimeMeshProxyUpdateBuilder& () { return ProxyBuilder; }		
			

		auto Commit()
		{
			if (auto Mesh = Resources->GetOwner())
			{
				return ProxyBuilder.Commit(Mesh.ToSharedRef());
			}

			return MakeFulfilledPromise<ERealtimeMeshProxyUpdateStatus>(ERealtimeMeshProxyUpdateStatus::NoProxy).GetFuture();
		}
		
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
		using TaskFunctionType = TUniqueFunction<void(FRealtimeMeshProxyUpdateBuilder&, FRealtimeMesh&)>;
	private:
		TArray<TaskFunctionType> Tasks;

	public:
		FRealtimeMeshUpdateBuilder() = default;
		UE_NONCOPYABLE(FRealtimeMeshUpdateBuilder)
		
		TFuture<ERealtimeMeshProxyUpdateStatus> Commit(const TSharedRef<FRealtimeMesh>& Mesh);

		void AddMeshTask(TUniqueFunction<void(FRealtimeMeshProxyUpdateBuilder&, FRealtimeMesh&)>&& Function);

		template <typename MeshType>
		void AddMeshTask(TUniqueFunction<void(FRealtimeMeshProxyUpdateBuilder&, MeshType&)>&& Function)
		{
			AddMeshTask([Func = MoveTemp(Function)](FRealtimeMeshProxyUpdateBuilder& ProxyBuilder, FRealtimeMesh& LOD)
			{
				Func(ProxyBuilder, static_cast<MeshType&>(LOD));
			});
		}

		void AddLODTask(const FRealtimeMeshLODKey& LODKey, TUniqueFunction<void(FRealtimeMeshProxyUpdateBuilder&, FRealtimeMeshLOD&)>&& Function);

		template <typename LODProxyType>
		void AddLODTask(const FRealtimeMeshLODKey& LODKey, TUniqueFunction<void(FRealtimeMeshProxyUpdateBuilder&, LODProxyType&)>&& Function)
		{
			AddLODTask(LODKey, [Func = MoveTemp(Function)](FRealtimeMeshProxyUpdateBuilder& ProxyBuilder, FRealtimeMeshLOD& LOD)
			{
				Func(ProxyBuilder, static_cast<LODProxyType&>(LOD));
			});
		}

		void AddSectionGroupTask(const FRealtimeMeshSectionGroupKey& SectionGroupKey, TUniqueFunction<void(FRealtimeMeshProxyUpdateBuilder&, FRealtimeMeshSectionGroup&)>&& Function);

		template <typename SectionGroupProxyType>
		void AddSectionGroupTask(const FRealtimeMeshSectionGroupKey& SectionGroupKey, TUniqueFunction<void(FRealtimeMeshProxyUpdateBuilder&, SectionGroupProxyType&)>&& Function)
		{
			AddSectionGroupTask(SectionGroupKey, [Func = MoveTemp(Function)](FRealtimeMeshProxyUpdateBuilder& ProxyBuilder, FRealtimeMeshSectionGroup& SectionGroup)
			{
				Func(ProxyBuilder, static_cast<SectionGroupProxyType&>(SectionGroup));
			});
		}

		void AddSectionTask(const FRealtimeMeshSectionKey& SectionKey, TUniqueFunction<void(FRealtimeMeshProxyUpdateBuilder&, FRealtimeMeshSection&)>&& Function);

		template <typename SectionProxyType>
		void AddSectionTask(const FRealtimeMeshSectionKey& SectionKey, TUniqueFunction<void(FRealtimeMeshProxyUpdateBuilder&, SectionProxyType&)>&& Function)
		{
			AddSectionTask(SectionKey, [Func = MoveTemp(Function)](FRealtimeMeshProxyUpdateBuilder& ProxyBuilder, FRealtimeMeshSection& Section)
			{
				Func(ProxyBuilder, static_cast<SectionProxyType&>(Section));
			});
		}
	};




	struct REALTIMEMESHCOMPONENT_API FRealtimeMeshAccessor
	{
	public:
		using TaskFunctionType = TUniqueFunction<void(const FRealtimeMesh&)>;
	private:
		TArray<TaskFunctionType> Tasks;

	public:
		FRealtimeMeshAccessor() = default;
		UE_NONCOPYABLE(FRealtimeMeshAccessor)
		
		void Execute(const TSharedRef<const FRealtimeMesh>& Mesh);

		void AddMeshTask(TUniqueFunction<void(const FRealtimeMesh&)>&& Function);

		template <typename MeshType>
		void AddMeshTask(TUniqueFunction<void(const MeshType&)>&& Function)
		{
			AddMeshTask([Func = MoveTemp(Function)](const FRealtimeMesh& LOD)
			{
				Func(static_cast<const MeshType&>(LOD));
			});
		}

		void AddLODTask(const FRealtimeMeshLODKey& LODKey, TUniqueFunction<void(const FRealtimeMeshLOD&)>&& Function);

		template <typename LODProxyType>
		void AddLODTask(const FRealtimeMeshLODKey& LODKey, TUniqueFunction<void(const LODProxyType&)>&& Function)
		{
			AddLODTask(LODKey, [Func = MoveTemp(Function)](const FRealtimeMeshLOD& LOD)
			{
				Func(static_cast<const LODProxyType&>(LOD));
			});
		}

		void AddSectionGroupTask(const FRealtimeMeshSectionGroupKey& SectionGroupKey, TUniqueFunction<void(const FRealtimeMeshSectionGroup&)>&& Function);

		template <typename SectionGroupProxyType>
		void AddSectionGroupTask(const FRealtimeMeshSectionGroupKey& SectionGroupKey, TUniqueFunction<void(const SectionGroupProxyType&)>&& Function)
		{
			AddSectionGroupTask(SectionGroupKey, [Func = MoveTemp(Function)](const FRealtimeMeshSectionGroup& SectionGroup)
			{
				Func(static_cast<const SectionGroupProxyType&>(SectionGroup));
			});
		}

		void AddSectionTask(const FRealtimeMeshSectionKey& SectionKey, TUniqueFunction<void(const FRealtimeMeshSection&)>&& Function);

		template <typename SectionProxyType>
		void AddSectionTask(const FRealtimeMeshSectionKey& SectionKey, TUniqueFunction<void(const SectionProxyType&)>&& Function)
		{
			AddSectionTask(SectionKey, [Func = MoveTemp(Function)](const FRealtimeMeshSection& Section)
			{
				Func(static_cast<const SectionProxyType&>(Section));
			});
		}
	};
}
