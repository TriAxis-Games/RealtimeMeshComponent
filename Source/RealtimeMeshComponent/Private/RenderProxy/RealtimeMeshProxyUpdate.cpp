// Copyright TriAxis Games, L.L.C. All Rights Reserved.


#include "Data/RealtimeMeshShared.h"
#include "Data/RealtimeMeshData.h"
#include "RenderProxy/RealtimeMeshProxyCommandBatch.h"
#include "RenderProxy/RealtimeMeshLODProxy.h"
#include "RenderProxy/RealtimeMeshProxy.h"
#include "RenderProxy/RealtimeMeshSectionGroupProxy.h"

namespace RealtimeMesh
{
	FRealtimeMeshProxyCommandBatch::FRealtimeMeshProxyCommandBatch(const FRealtimeMeshSharedResourcesPtr& InSharedResources, bool bInRequiresProxyRecreate):
		Mesh(InSharedResources.IsValid() ? InSharedResources->GetOwner() : nullptr)
		, bRequiresProxyRecreate(bInRequiresProxyRecreate)
	{
	}

	FRealtimeMeshProxyCommandBatch::FRealtimeMeshProxyCommandBatch(const FRealtimeMeshPtr& InMesh, bool bInRequiresProxyRecreate): Mesh(InMesh)
		, bRequiresProxyRecreate(bInRequiresProxyRecreate)
	{
	}

	TFuture<ERealtimeMeshProxyUpdateStatus> FRealtimeMeshProxyCommandBatch::Commit()
	{
		if (!Mesh.IsValid())
		{
			return MakeFulfilledPromise<ERealtimeMeshProxyUpdateStatus>(ERealtimeMeshProxyUpdateStatus::NoProxy).GetFuture();
		}

		if (Tasks.IsEmpty())
		{
			return MakeFulfilledPromise<ERealtimeMeshProxyUpdateStatus>(ERealtimeMeshProxyUpdateStatus::NoUpdate).GetFuture();
		}

		auto Promise = MakeShared<TPromise<ERealtimeMeshProxyUpdateStatus>>();

		ENQUEUE_RENDER_COMMAND(FRealtimeMeshProxy_Update)([Promise, ProxyWeak = FRealtimeMeshProxyWeakPtr(Mesh->GetRenderProxy(true)), Tasks = MoveTemp(Tasks)](FRHICommandListImmediate&)
		{
			if (const auto& Proxy = ProxyWeak.Pin())
			{
				for (const auto& Task : Tasks)
				{
					Task(*Proxy.Get());
				}
				Proxy->UpdatedCachedState(false);
			}
			Promise->SetValue(ERealtimeMeshProxyUpdateStatus::Updated);
		});

		Mesh->MarkRenderStateDirty(bRequiresProxyRecreate);

		Tasks.Empty();
		bRequiresProxyRecreate = false;

		return Promise->GetFuture();
	}

	void FRealtimeMeshProxyCommandBatch::AddMeshTask(TUniqueFunction<void(FRealtimeMeshProxy&)>&& Function, bool bInRequiresProxyRecreate)
	{
		check(Mesh.IsValid());
		bRequiresProxyRecreate |= bInRequiresProxyRecreate;
		Tasks.Add(MoveTemp(Function));
	}

	void FRealtimeMeshProxyCommandBatch::AddLODTask(const FRealtimeMeshLODKey& LODKey, TUniqueFunction<void(FRealtimeMeshLODProxy&)>&& Function, bool bInRequiresProxyRecreate)
	{
		AddMeshTask([LODKey, Func = MoveTemp(Function)](const FRealtimeMeshProxy& MeshProxy)
		{
			const FRealtimeMeshLODProxyPtr LOD = MeshProxy.GetLOD(LODKey);

			if (LOD.IsValid())
			{
				Func(*LOD.Get());
			}
		}, bInRequiresProxyRecreate);
	}

	void FRealtimeMeshProxyCommandBatch::AddSectionGroupTask(const FRealtimeMeshSectionGroupKey& SectionGroupKey, TUniqueFunction<void(FRealtimeMeshSectionGroupProxy&)>&& Function,
	                                                         bool bInRequiresProxyRecreate)
	{
		AddLODTask(SectionGroupKey.LOD(), [SectionGroupKey, Func = MoveTemp(Function)](const FRealtimeMeshLODProxy& LOD)
		{
			const FRealtimeMeshSectionGroupProxyPtr SectionGroup = LOD.GetSectionGroup(SectionGroupKey);

			if (SectionGroup.IsValid())
			{
				Func(*SectionGroup.Get());
			}
		}, bInRequiresProxyRecreate);
	}

	void FRealtimeMeshProxyCommandBatch::AddSectionTask(const FRealtimeMeshSectionKey& SectionKey, TUniqueFunction<void(FRealtimeMeshSectionProxy&)>&& Function,
	                                                    bool bInRequiresProxyRecreate)
	{
		AddSectionGroupTask(SectionKey.SectionGroup(), [SectionKey, Func = MoveTemp(Function)](const FRealtimeMeshSectionGroupProxy& SectionGroup)
		{
			const FRealtimeMeshSectionProxyPtr Section = SectionGroup.GetSection(SectionKey);

			if (Section.IsValid())
			{
				Func(*Section.Get());
			}
		}, bInRequiresProxyRecreate);
	}
}
