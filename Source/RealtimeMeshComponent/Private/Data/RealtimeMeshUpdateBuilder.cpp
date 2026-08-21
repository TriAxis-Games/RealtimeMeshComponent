// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#include "Data/RealtimeMeshUpdateBuilder.h"
#include "RealtimeMeshComponentModule.h"
#include "RenderingThread.h"

#define LOCTEXT_NAMESPACE "RealtimeMesh"

namespace RealtimeMesh
{
	FRealtimeMeshAccessContext::FRealtimeMeshAccessContext(const TSharedRef<const FRealtimeMesh>& InMesh)
		: ReadGuard(InMesh->GetContext()->GetGuard())
		, Resources(InMesh->GetContext())
	{ }

	FRealtimeMeshAccessContext::FRealtimeMeshAccessContext(const FRealtimeMeshContextRef& InResources)
		: ReadGuard(InResources->GetGuard())
		, Resources(InResources)
	{ }

	FRealtimeMeshUpdateContext::FRealtimeMeshUpdateContext(const TSharedRef<FRealtimeMesh>& InMesh)
		: WriteGuard(InMesh->GetContext()->GetGuard())
		, ProxyBuilder(!InMesh->GetRenderProxy().IsValid())
		, Resources(InMesh->GetContext())
		, UpdateState(Resources->CreateUpdateState())
		, bCommitted(false)
	{
	}

	FRealtimeMeshUpdateContext::FRealtimeMeshUpdateContext(const FRealtimeMeshContextRef& InResources)
		: FRealtimeMeshUpdateContext(InResources->GetOwner().ToSharedRef()) { }

	FRealtimeMeshUpdateContext::~FRealtimeMeshUpdateContext()
	{
		if (!bCommitted)
		{
			Commit();
		}
	}

	FRHICommandList& FRealtimeMeshUpdateContext::GetRHICmdList()
	{
		// Lazy creation: the command list is only allocated the first time something needs
		// to record onto it (async buffer creation). Config-only updates never get here and
		// so never pay for a command list or its submission.
		if (!RHICmdList.IsValid())
		{
			RHICmdList = MakeUnique<FRHICommandList>();
			RHICmdList->SwitchPipeline(ERHIPipeline::Graphics);
		}
		return *RHICmdList;
	}

	TFuture<ERealtimeMeshProxyUpdateStatus> FRealtimeMeshUpdateContext::Commit()
	{
		bCommitted = true;

		auto SendCmdListIfReady = [&]()
		{
			if (RHICmdList)
			{
				RHICmdList->FinishRecording();

				ENQUEUE_RENDER_COMMAND(RealtimeMeshAsyncSubmission)(
					[RHIAsyncCmdList = RHICmdList.Release()](FRHICommandListImmediate& CmdList)
					{
						CmdList.QueueAsyncCommandListSubmit(RHIAsyncCmdList);
					});
			}
		};

		if (auto Mesh = Resources->GetOwner())
		{
			Mesh->FinalizeUpdate(*this);

			// Write lock still held, committing thread. Subscribers copy dirty state out
			// and defer real work (see FRealtimeMeshUpdateCommittedEvent contract).
			Resources->OnUpdateCommitted().Broadcast(*this);

			SendCmdListIfReady();
			return ProxyBuilder.Commit(Mesh.ToSharedRef());
		}

		SendCmdListIfReady();
		return MakeFulfilledPromise<ERealtimeMeshProxyUpdateStatus>(ERealtimeMeshProxyUpdateStatus::NoProxy).GetFuture();
	}


	// -------- FRealtimeMeshTaskBuilderBase --------

	void FRealtimeMeshTaskBuilderBase::RunTasks(FRealtimeMeshLockContext& LockContext, const FRealtimeMesh& Mesh)
	{
		for (auto& Task : Tasks)
		{
			Task(LockContext, Mesh);
		}
		// Clear the queue so a subsequent Execute/Commit on the same builder
		// doesn't re-run tasks that already applied.
		Tasks.Empty();
	}

	void FRealtimeMeshTaskBuilderBase::AddMeshTask(TaskFunctionType&& Function)
	{
		Tasks.Add(MoveTemp(Function));
	}

	void FRealtimeMeshTaskBuilderBase::AddLODTask(const FRealtimeMeshLODKey& LODKey, TUniqueFunction<void(FRealtimeMeshLockContext&, const FRealtimeMeshLOD&)>&& Function)
	{
		AddMeshTask([LODKey, Func = MoveTemp(Function)](FRealtimeMeshLockContext& LockContext, const FRealtimeMesh& Mesh)
		{
			const FRealtimeMeshLODPtr LOD = Mesh.GetLOD(LockContext, LODKey);

			if (ensure(LOD.IsValid()))
			{
				Func(LockContext, *LOD.Get());
			}
			else
			{
				UE_LOG(LogRealtimeMesh, Error, TEXT("Failed to find LOD %s"), *LODKey.ToString());

				FMessageLog("RealtimeMesh").Error(
				FText::Format(LOCTEXT("RealtimeMeshTaskBuilder_LODTask", "RealtimeMeshTaskBuilder_LODTask: Failed to find LOD {0}"),
					FText::FromString(LODKey.ToString())));
			}
		});
	}

	void FRealtimeMeshTaskBuilderBase::AddSectionGroupTask(const FRealtimeMeshBufferSetKey& SectionGroupKey, TUniqueFunction<void(FRealtimeMeshLockContext&, const FRealtimeMeshBufferSet&)>&& Function)
	{
		AddLODTask(SectionGroupKey.LOD(), [SectionGroupKey, Func = MoveTemp(Function)](FRealtimeMeshLockContext& LockContext, const FRealtimeMeshLOD& LOD)
		{
			const FRealtimeMeshSectionGroupPtr SectionGroup = LOD.GetSectionGroup(LockContext, SectionGroupKey);

			if (ensure(SectionGroup.IsValid()))
			{
				Func(LockContext, *SectionGroup.Get());
			}
			else
			{
				UE_LOG(LogRealtimeMesh, Error, TEXT("Failed to find SectionGroup %s"), *SectionGroupKey.ToString());

				FMessageLog("RealtimeMesh").Error(
				FText::Format(LOCTEXT("RealtimeMeshTaskBuilder_SectionGroupTask", "RealtimeMeshTaskBuilder_SectionGroupTask: Failed to find SectionGroup {0}"),
					FText::FromString(SectionGroupKey.ToString())));
			}
		});
	}

	void FRealtimeMeshTaskBuilderBase::AddSectionTask(const FRealtimeMeshSectionKey& SectionKey, TUniqueFunction<void(FRealtimeMeshLockContext&, const FRealtimeMeshSection&)>&& Function)
	{
		AddSectionGroupTask(SectionKey.SectionGroup(), [SectionKey, Func = MoveTemp(Function)](FRealtimeMeshLockContext& LockContext, const FRealtimeMeshBufferSet& SectionGroup)
		{
			const FRealtimeMeshSectionPtr Section = SectionGroup.GetSection(LockContext, SectionKey);

			if (ensure(Section.IsValid()))
			{
				Func(LockContext, *Section.Get());
			}
			else
			{
				UE_LOG(LogRealtimeMesh, Error, TEXT("Failed to find Section %s"), *SectionKey.ToString());

				FMessageLog("RealtimeMesh").Error(
				FText::Format(LOCTEXT("RealtimeMeshTaskBuilder_Section", "RealtimeMeshTaskBuilder_Section: Failed to find Section {0}"),
					FText::FromString(SectionKey.ToString())));
			}
		});
	}


	// -------- FRealtimeMeshAccessor --------

	void FRealtimeMeshAccessor::Execute(const TSharedRef<const FRealtimeMesh>& Mesh)
	{
		FRealtimeMeshAccessContext LockContext(Mesh);
		RunTasks(LockContext, *Mesh);
	}


	// -------- FRealtimeMeshUpdateBuilder --------
	//
	// Write-flavored Add*Task overloads each wrap the caller's mutable-task
	// into the base's `void(FRealtimeMeshLockContext&, const FRealtimeMesh&)`
	// shape. The runtime casts (LockContext -> UpdateContext, const Element ->
	// mutable Element) are safe: Commit always constructs an UpdateContext
	// (the static_cast target), and the const-narrowing at the base layer is
	// purely a signature-unification trick — the underlying element
	// shared_ptr is mutable regardless of how it was navigated.

	void FRealtimeMeshUpdateBuilder::AddMeshTask(TUniqueFunction<void(FRealtimeMeshUpdateContext&, FRealtimeMesh&)>&& Function)
	{
		FRealtimeMeshTaskBuilderBase::AddMeshTask([Func = MoveTemp(Function)](FRealtimeMeshLockContext& LockContext, const FRealtimeMesh& Mesh)
		{
			Func(static_cast<FRealtimeMeshUpdateContext&>(LockContext), const_cast<FRealtimeMesh&>(Mesh));
		});
	}

	void FRealtimeMeshUpdateBuilder::AddLODTask(const FRealtimeMeshLODKey& LODKey, TUniqueFunction<void(FRealtimeMeshUpdateContext&, FRealtimeMeshLOD&)>&& Function)
	{
		FRealtimeMeshTaskBuilderBase::AddLODTask(LODKey, [Func = MoveTemp(Function)](FRealtimeMeshLockContext& LockContext, const FRealtimeMeshLOD& LOD)
		{
			Func(static_cast<FRealtimeMeshUpdateContext&>(LockContext), const_cast<FRealtimeMeshLOD&>(LOD));
		});
	}

	void FRealtimeMeshUpdateBuilder::AddSectionGroupTask(const FRealtimeMeshBufferSetKey& SectionGroupKey, TUniqueFunction<void(FRealtimeMeshUpdateContext&, FRealtimeMeshBufferSet&)>&& Function)
	{
		FRealtimeMeshTaskBuilderBase::AddSectionGroupTask(SectionGroupKey, [Func = MoveTemp(Function)](FRealtimeMeshLockContext& LockContext, const FRealtimeMeshBufferSet& SectionGroup)
		{
			Func(static_cast<FRealtimeMeshUpdateContext&>(LockContext), const_cast<FRealtimeMeshBufferSet&>(SectionGroup));
		});
	}

	void FRealtimeMeshUpdateBuilder::AddSectionTask(const FRealtimeMeshSectionKey& SectionKey, TUniqueFunction<void(FRealtimeMeshUpdateContext&, FRealtimeMeshSection&)>&& Function)
	{
		FRealtimeMeshTaskBuilderBase::AddSectionTask(SectionKey, [Func = MoveTemp(Function)](FRealtimeMeshLockContext& LockContext, const FRealtimeMeshSection& Section)
		{
			Func(static_cast<FRealtimeMeshUpdateContext&>(LockContext), const_cast<FRealtimeMeshSection&>(Section));
		});
	}

	TFuture<ERealtimeMeshProxyUpdateStatus> FRealtimeMeshUpdateBuilder::Commit(const TSharedRef<FRealtimeMesh>& Mesh)
	{
		FRealtimeMeshUpdateContext UpdateContext(Mesh);
		RunTasks(UpdateContext, *Mesh);
		return UpdateContext.Commit();
	}
}

#undef LOCTEXT_NAMESPACE
