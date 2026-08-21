// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.


#include "RenderProxy/RealtimeMeshProxyCommandBatch.h"

#include "Core/RealtimeMeshFuture.h"
#include "Data/RealtimeMeshShared.h"
#include "Data/RealtimeMeshData.h"
#include "RenderProxy/RealtimeMeshLODProxy.h"
#include "RenderProxy/RealtimeMeshProxy.h"
#include "RenderProxy/RealtimeMeshBufferSetProxy.h"
#include "RenderingThread.h"

namespace RealtimeMesh
{
	namespace
	{
		// DUP-007: schedule the scene-proxy-recreation broadcast back onto the game
		// thread. Shared verbatim by the in-place fallback and the publish paths — the
		// thread hop, captured weak mesh, and broadcast payload are identical in both.
		void BroadcastProxyRequiresUpdateOnGameThread(TWeakPtr<const FRealtimeMesh> MeshWeak)
		{
			AsyncTask(ENamedThreads::GameThread, [MeshWeak = MoveTemp(MeshWeak)]()
			{
				if (const auto MeshToMarkDirty = MeshWeak.Pin())
				{
					if (MeshToMarkDirty->GetContext()->OnRenderProxyRequiresUpdate().IsBound())
					{
						MeshToMarkDirty->GetContext()->OnRenderProxyRequiresUpdate().Broadcast();
					}
				}
			});
		}
	}

	void FRealtimeMeshCommandBatchIntermediateFuture::FinalizeRenderThread(ERealtimeMeshProxyUpdateStatus Status)
	{
		AsyncTask(ENamedThreads::GameThread, [Status, ThisShared = this->AsShared()]()
		{
			ThisShared->FinalPromise.EmplaceValue(Status);
		});
	}

	void FRealtimeMeshProxyUpdateBuilder::AddMeshTask(TUniqueFunction<void(FRHICommandListBase&, FRealtimeMeshProxy&)>&& Function, bool bInRequiresProxyRecreate)
	{
		bRequiresProxyRecreate |= bInRequiresProxyRecreate;
		Tasks.Add(MoveTemp(Function));
	}

	void FRealtimeMeshProxyUpdateBuilder::AddLODTask(const FRealtimeMeshLODKey& LODKey, TUniqueFunction<void(FRHICommandListBase&, FRealtimeMeshLODProxy&)>&& Function, bool bInRequiresProxyRecreate)
	{
		AddMeshTask([LODKey, Func = MoveTemp(Function)](FRHICommandListBase& RHICmdList, FRealtimeMeshProxy& MeshProxy)
		{
			// FindWorkspaceLOD returns a non-owning raw pointer into workspace storage
			// (TCowPtr's underlying TSharedPtr stays put). The pointer is valid for
			// the lifetime of this task within the same ProcessCommands batch.
			FRealtimeMeshLODProxy* LOD = MeshProxy.FindWorkspaceLOD(LODKey);

			if (ensure(LOD != nullptr))
			{
				Func(RHICmdList, *LOD);
			}
		}, bInRequiresProxyRecreate);
	}

	void FRealtimeMeshProxyUpdateBuilder::AddBufferSetTask(const FRealtimeMeshBufferSetKey& BufferSetKey, TUniqueFunction<void(FRHICommandListBase&, FRealtimeMeshBufferSetProxy&)>&& Function, bool bInRequiresProxyRecreate)
	{
		AddLODTask(BufferSetKey.LOD(), [BufferSetKey, Func = MoveTemp(Function)](FRHICommandListBase& RHICmdList, FRealtimeMeshLODProxy& LOD)
		{
			// FindMutableBufferSet COWs the slot if it's still shared with an older
			// published version and marks it touched. Raw pointer is non-owning —
			// the LOD's TCowPtr retains storage ownership.
			FRealtimeMeshBufferSetProxy* BufferSet = LOD.FindMutableBufferSet(BufferSetKey);

			if (ensure(BufferSet != nullptr))
			{
				Func(RHICmdList, *BufferSet);
			}
		}, bInRequiresProxyRecreate);
	}

	void FRealtimeMeshProxyUpdateBuilder::AddSectionTask(const FRealtimeMeshSectionKey& SectionKey, TUniqueFunction<void(FRHICommandListBase&, FRealtimeMeshSectionProxy&)>&& Function, bool bInRequiresProxyRecreate)
	{
		// Resolve sections directly through the LOD (they live there post-flattening).
		// FindMutableSection handles the COW + touched-marking.
		AddLODTask(SectionKey.LOD(), [SectionKey, Func = MoveTemp(Function)](FRHICommandListBase& RHICmdList, FRealtimeMeshLODProxy& LOD)
		{
			FRealtimeMeshSectionProxy* Section = LOD.FindMutableSection(SectionKey);

			if (ensure(Section != nullptr))
			{
				Func(RHICmdList, *Section);
			}
		}, bInRequiresProxyRecreate);
	}


	
	TFuture<ERealtimeMeshProxyUpdateStatus> FRealtimeMeshProxyUpdateBuilder::Commit(const TSharedRef<const FRealtimeMesh>& Mesh)
	{
		if (Tasks.IsEmpty() && InPlaceUpdates.IsEmpty())
		{
			return MakeFulfilledPromise<ERealtimeMeshProxyUpdateStatus>(ERealtimeMeshProxyUpdateStatus::NoUpdate).GetFuture();
		}

		// Grab the proxy we are going to update — needed for the GT-side
		// HasNaniteData flag that gates which scene-proxy type the component picks.
		const FRealtimeMeshProxyPtr Proxy = Mesh->GetRenderProxy(false);

		if (!Proxy.IsValid())
		{
			return MakeFulfilledPromise<ERealtimeMeshProxyUpdateStatus>(ERealtimeMeshProxyUpdateStatus::NoProxy).GetFuture();
		}

		auto ThreadState = MakeShared<FRealtimeMeshCommandBatchIntermediateFuture>();

		if (bNewHasNaniteData.IsSet())
		{
			Proxy->SetHasNaniteData_GT(bNewHasNaniteData.GetValue());
		}

		// Fast path: a batch made up solely of in-place stream updates that requires no
		// proxy recreate can skip the clone + publish entirely. The RT command overwrites
		// the current published version's GPU buffers in place (see
		// FRealtimeMesh::ApplyInPlace_RenderThread). If that turns out to be unsafe (a live
		// snapshot shares a target node), it transparently falls back to a clone+publish
		// and we trigger a scene-proxy recreate so the new version becomes visible.
		if (Tasks.IsEmpty() && !InPlaceUpdates.IsEmpty() && !bRequiresProxyRecreate)
		{
			ENQUEUE_RENDER_COMMAND(RealtimeMeshApplyInPlace)(
				[MeshWeak = Mesh.ToWeakPtr(), UpdatesToRun = MoveTemp(InPlaceUpdates), ThreadState]
				(FRHICommandListImmediate& RHICmdList) mutable
				{
					ERealtimeMeshProxyUpdateStatus Status = ERealtimeMeshProxyUpdateStatus::NoProxy;
					bool bNeedsRecreate = false;

					if (const auto MeshPinned = MeshWeak.Pin())
					{
						const ERealtimeMeshInPlaceApplyResult Result =
							const_cast<FRealtimeMesh&>(*MeshPinned).ApplyInPlace_RenderThread(RHICmdList, UpdatesToRun);

						if (Result != ERealtimeMeshInPlaceApplyResult::NoProxy)
						{
							Status = ERealtimeMeshProxyUpdateStatus::Updated;
						}

						// A fallback published a brand-new version; scene proxies must be
						// recreated to observe it (same rule as the regular publish path).
						bNeedsRecreate = (Result == ERealtimeMeshInPlaceApplyResult::FellBackPublished);
					}

					if (bNeedsRecreate)
					{
						BroadcastProxyRequiresUpdateOnGameThread(MeshWeak);
					}

					ThreadState->FinalizeRenderThread(Status);
				});

			Tasks.Empty();
			InPlaceUpdates.Empty();
			bRequiresProxyRecreate = false;
			return ThreadState->FinalPromise.GetFuture();
		}

		// Mixed or structural batch: fold any queued in-place updates into ordinary
		// reallocating stream tasks so they ride the clone+publish path with everything
		// else (the fast path can't be combined with a publishing batch).
		for (FRealtimeMeshInPlaceStreamUpdate& Update : InPlaceUpdates)
		{
			AddBufferSetTask(Update.BufferSetKey, [UpdateData = Update.UpdateData](FRHICommandListBase& RHICmdList, FRealtimeMeshBufferSetProxy& BufferSet)
			{
				BufferSet.CreateOrUpdateStream(RHICmdList, UpdateData);
			}, false);
		}
		InPlaceUpdates.Empty();

		// Ship the batch to RT. Each Commit becomes one RT command — the RT
		// command clones the latest published proxy, applies these tasks, and
		// atomically publishes the result as the new latest. RT commands
		// serialize naturally, so two concurrent commits don't interleave: the
		// later one sees the earlier one's published state.
		//
		// The broadcast that triggers component scene-proxy recreation MUST fire
		// AFTER the new version is published — otherwise the engine recreates
		// scene proxies that capture the older (pre-batch) version and never
		// see this update. We schedule the broadcast from inside the RT command,
		// dispatched back to the game thread once the publish completes.
		//
		// PROXY-F9: this path ALWAYS publishes a brand-new version. Live scene
		// proxies captured the previous version at construction and never observe
		// in-progress changes, so a successful publish is by itself sufficient
		// reason to recreate them — regardless of the batch's bRequiresProxyRecreate
		// flag. (A batch whose tasks all pass false — e.g. the compute registry
		// binding SetIndirectArgs — would otherwise publish a change no scene proxy
		// could ever see.) We therefore broadcast whenever the publish succeeds.
		ENQUEUE_RENDER_COMMAND(RealtimeMeshApplyAndPublish)(
			[MeshWeak = Mesh.ToWeakPtr(), TasksToRun = MoveTemp(Tasks), ThreadState]
			(FRHICommandListImmediate& RHICmdList) mutable
			{
				ERealtimeMeshProxyUpdateStatus Status = ERealtimeMeshProxyUpdateStatus::NoProxy;
				bool bPublished = false;
				if (const auto MeshPinned = MeshWeak.Pin())
				{
					// const_cast: ApplyAndPublish_RenderThread is a non-const RT
					// method on FRealtimeMesh, but the GT-side Commit signature
					// takes a TSharedRef<const FRealtimeMesh>. The proxy publish
					// is logically a render-thread mutation orthogonal to the
					// GT-side const view of the mesh.
					if (const_cast<FRealtimeMesh&>(*MeshPinned).ApplyAndPublish_RenderThread(RHICmdList, TasksToRun))
					{
						Status = ERealtimeMeshProxyUpdateStatus::Updated;
						bPublished = true;
					}
				}

				if (bPublished)
				{
					BroadcastProxyRequiresUpdateOnGameThread(MeshWeak);
				}

				ThreadState->FinalizeRenderThread(Status);
			});

		Tasks.Empty();
		bRequiresProxyRecreate = false;

		return ThreadState->FinalPromise.GetFuture();
	}
}
