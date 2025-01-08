// Copyright (c) 2015-2025 TriAxis Games, L.L.C. All Rights Reserved.


#include "RenderProxy/RealtimeMeshProxyCommandBatch.h"

#include "Core/RealtimeMeshFuture.h"
#include "Data/RealtimeMeshShared.h"
#include "Data/RealtimeMeshData.h"
#include "RenderProxy/RealtimeMeshLODProxy.h"
#include "RenderProxy/RealtimeMeshProxy.h"
#include "RenderProxy/RealtimeMeshSectionGroupProxy.h"

namespace RealtimeMesh
{
	FRealtimeMeshCommandBatchIntermediateFuture::FRealtimeMeshCommandBatchIntermediateFuture(): FinalPromise(MakeShared<TPromise<ERealtimeMeshProxyUpdateStatus>>())
	                                                                                            , Result(ERealtimeMeshProxyUpdateStatus::NoUpdate)
	                                                                                            , bRenderThreadReady(false)
	                                                                                            , bGameThreadReady(false)
	                                                                                            , bFinalized(false)
	{
	}

	void FRealtimeMeshCommandBatchIntermediateFuture::FinalizeRenderThread(ERealtimeMeshProxyUpdateStatus Status)
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

	void FRealtimeMeshCommandBatchIntermediateFuture::FinalizeGameThread()
	{
		bGameThreadReady = true;

		if (bRenderThreadReady && !bFinalized)
		{
			FinalPromise->EmplaceValue(Result);
			bFinalized = true;
		}
	}




	
	
	void FRealtimeMeshProxyUpdateBuilder::AddMeshTask(TUniqueFunction<void(FRHICommandListBase&, FRealtimeMeshProxy&)>&& Function, bool bInRequiresProxyRecreate)
	{
		bRequiresProxyRecreate |= bInRequiresProxyRecreate;
		Tasks.Add(MoveTemp(Function));
	}

	void FRealtimeMeshProxyUpdateBuilder::AddLODTask(const FRealtimeMeshLODKey& LODKey, TUniqueFunction<void(FRHICommandListBase&, FRealtimeMeshLODProxy&)>&& Function, bool bInRequiresProxyRecreate)
	{
		AddMeshTask([LODKey, Func = MoveTemp(Function)](FRHICommandListBase& RHICmdList, const FRealtimeMeshProxy& MeshProxy)
		{
			const FRealtimeMeshLODProxyPtr LOD = MeshProxy.GetLOD(LODKey);

			if (ensure(LOD.IsValid()))
			{
				Func(RHICmdList, *LOD.Get());
			}
		}, bInRequiresProxyRecreate);
	}

	void FRealtimeMeshProxyUpdateBuilder::AddSectionGroupTask(const FRealtimeMeshSectionGroupKey& SectionGroupKey, TUniqueFunction<void(FRHICommandListBase&, FRealtimeMeshSectionGroupProxy&)>&& Function, bool bInRequiresProxyRecreate)
	{
		AddLODTask(SectionGroupKey.LOD(), [SectionGroupKey, Func = MoveTemp(Function)](FRHICommandListBase& RHICmdList, const FRealtimeMeshLODProxy& LOD)
		{
			const FRealtimeMeshSectionGroupProxyPtr SectionGroup = LOD.GetSectionGroup(SectionGroupKey);

			if (ensure(SectionGroup.IsValid()))
			{
				Func(RHICmdList, *SectionGroup.Get());
			}
		}, bInRequiresProxyRecreate);
	}

	void FRealtimeMeshProxyUpdateBuilder::AddSectionTask(const FRealtimeMeshSectionKey& SectionKey, TUniqueFunction<void(FRHICommandListBase&, FRealtimeMeshSectionProxy&)>&& Function, bool bInRequiresProxyRecreate)
	{
		AddSectionGroupTask(SectionKey.SectionGroup(), [SectionKey, Func = MoveTemp(Function)](FRHICommandListBase& RHICmdList, const FRealtimeMeshSectionGroupProxy& SectionGroup)
		{
			const FRealtimeMeshSectionProxyPtr Section = SectionGroup.GetSection(SectionKey);

			if (ensure(Section.IsValid()))
			{
				Func(RHICmdList, *Section.Get());
			}
		}, bInRequiresProxyRecreate);
	}


	
	TFuture<ERealtimeMeshProxyUpdateStatus> FRealtimeMeshProxyUpdateBuilder::Commit(const TSharedRef<const FRealtimeMesh>& Mesh)
	{
		// Skip if no tasks
		if (Tasks.IsEmpty())
		{
			return MakeFulfilledPromise<ERealtimeMeshProxyUpdateStatus>(ERealtimeMeshProxyUpdateStatus::NoUpdate).GetFuture();
		}

		// Grab the proxy we are going to update
		const FRealtimeMeshProxyPtr Proxy = Mesh->GetRenderProxy(false);

		// Skip if no proxy
		if (!Proxy.IsValid())
		{
			return MakeFulfilledPromise<ERealtimeMeshProxyUpdateStatus>(ERealtimeMeshProxyUpdateStatus::NoProxy).GetFuture();
		}
		
		auto ThreadState = MakeShared<FRealtimeMeshCommandBatchIntermediateFuture>();

		Proxy->EnqueueCommandBatch(MoveTemp(Tasks), ThreadState);

		DoOnGameThread([ThreadState, MeshWeak = Mesh.ToWeakPtr(), bRecreateProxies = static_cast<bool>(bRequiresProxyRecreate)]()
		{
			if (bRecreateProxies)
			{
				if (const auto MeshToMarkDirty = MeshWeak.Pin())
				{
					if (MeshToMarkDirty->GetSharedResources()->OnRenderProxyRequiresUpdate().IsBound())
					{
						MeshToMarkDirty->GetSharedResources()->OnRenderProxyRequiresUpdate().Broadcast();
					}
				}
			}

			ThreadState->FinalizeGameThread();
		});

		Tasks.Empty();
		bRequiresProxyRecreate = false;

		return ThreadState->FinalPromise->GetFuture();
	}
}
