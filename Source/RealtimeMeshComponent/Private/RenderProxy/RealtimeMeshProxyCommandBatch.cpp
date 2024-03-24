// Copyright TriAxis Games, L.L.C. All Rights Reserved.


#include "RenderProxy/RealtimeMeshProxyCommandBatch.h"
#include "Data/RealtimeMeshShared.h"
#include "Data/RealtimeMeshData.h"
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


	struct FRealtimeMeshCommandBatchIntermediateFuture : public TSharedFromThis<FRealtimeMeshCommandBatchIntermediateFuture>
	{
		TSharedRef<TPromise<ERealtimeMeshProxyUpdateStatus>> FinalPromise;
		ERealtimeMeshProxyUpdateStatus Result;
		uint8 bRenderThreadReady : 1;
		uint8 bGameThreadReady : 1;
		uint8 bFinalized : 1;

		FRealtimeMeshCommandBatchIntermediateFuture()
			: FinalPromise(MakeShared<TPromise<ERealtimeMeshProxyUpdateStatus>>())
			  , Result(ERealtimeMeshProxyUpdateStatus::NoUpdate)
			  , bRenderThreadReady(false)
			  , bGameThreadReady(false)
			  , bFinalized(false)
		{
		}

		void FinalizeRenderThread(ERealtimeMeshProxyUpdateStatus Status)
		{
			AsyncTask(ENamedThreads::GameThread, [Status, ThisShared = this->AsShared()]()
			{
				ThisShared->Result = Status;
				ThisShared->bRenderThreadReady = true;

				if (!ThisShared->bFinalized)
				{
					ThisShared->FinalPromise->EmplaceValue(ThisShared->Result);
					ThisShared->bFinalized = true;
				}
			});
		}

		void FinalizeGameThread()
		{
			bGameThreadReady = true;

			if (bRenderThreadReady && !bFinalized)
			{
				FinalPromise->EmplaceValue(Result);
				bFinalized = true;
			}
		}
	};


	TFuture<ERealtimeMeshProxyUpdateStatus> FRealtimeMeshProxyCommandBatch::Commit()
	{
		// Skip if no tasks
		if (Tasks.IsEmpty())
		{
			return MakeFulfilledPromise<ERealtimeMeshProxyUpdateStatus>(ERealtimeMeshProxyUpdateStatus::NoUpdate).GetFuture();
		}

		// Skip if no mesh
		if (!Mesh.IsValid())
		{
			return MakeFulfilledPromise<ERealtimeMeshProxyUpdateStatus>(ERealtimeMeshProxyUpdateStatus::NoProxy).GetFuture();
		}

		// Skip if no proxy
		const FRealtimeMeshProxyPtr Proxy = Mesh->GetRenderProxy(true);

		auto ThreadState = MakeShared<FRealtimeMeshCommandBatchIntermediateFuture>();

		ENQUEUE_RENDER_COMMAND(FRealtimeMeshProxy_Update)([ThreadState, ProxyWeak = FRealtimeMeshProxyWeakPtr(Proxy), Tasks = MoveTemp(Tasks)](FRHICommandListImmediate&)
		{
			if (const auto& Proxy = ProxyWeak.Pin())
			{
				for (const auto& Task : Tasks)
				{
					Task(*Proxy.Get());
				}
				Proxy->UpdatedCachedState(false);
				ThreadState->FinalizeRenderThread(ERealtimeMeshProxyUpdateStatus::Updated);
			}
			else
			{
				ThreadState->FinalizeRenderThread(ERealtimeMeshProxyUpdateStatus::NoProxy);
			}
		});

		AsyncTask(ENamedThreads::GameThread, [ThreadState, MeshWeak = TWeakPtr<FRealtimeMesh>(Mesh), bRecreateProxies = bRequiresProxyRecreate]()
		{
			if (const FRealtimeMeshPtr Mesh = MeshWeak.Pin())
			{
				// TODO: We probably shouldn't always have to do this
				Mesh->MarkRenderStateDirty(bRecreateProxies);
			}

			ThreadState->FinalizeGameThread();
		});

		Tasks.Empty();
		bRequiresProxyRecreate = false;

		return ThreadState->FinalPromise->GetFuture();
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
