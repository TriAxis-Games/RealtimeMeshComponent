// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "RealtimeMeshCore.h"
#include "RealtimeMeshSectionProxy.h"
#include "Core/RealtimeMeshGPUStream.h"
#include "RealtimeMeshBufferSetProxy.h"


namespace RealtimeMesh
{
	// A single fast in-place stream update: overwrite an existing stream's GPU contents
	// over [ElementOffset, ElementOffset + NumElements) without reallocating. UpdateData
	// carries the full new stream bytes (and can lazily create an RHI buffer if the render
	// thread has to fall back to a reallocating update).
	struct FRealtimeMeshInPlaceStreamUpdate
	{
		FRealtimeMeshBufferSetKey BufferSetKey;
		FRealtimeMeshSectionGroupStreamUpdateDataRef UpdateData;
		int32 ElementOffset = 0;
		int32 NumElements = 0;
	};

	// Outcome of FRealtimeMesh::ApplyInPlace_RenderThread.
	enum class ERealtimeMeshInPlaceApplyResult : uint8
	{
		NoProxy,            // no published proxy to update
		AppliedInPlace,     // mutated the current published version in place; no new version
		FellBackPublished,  // a snapshot shared a target node (or it changed shape); cloned + reallocated + published
	};

	struct FRealtimeMeshCommandBatchIntermediateFuture : public TSharedFromThis<FRealtimeMeshCommandBatchIntermediateFuture>
	{
		// Held by value (not a separate MakeShared) — the shared struct itself owns it, so
		// one allocation covers the whole completion object. The returned TFuture keeps the
		// promise's shared state alive independently of this object.
		TPromise<ERealtimeMeshProxyUpdateStatus> FinalPromise;

		FRealtimeMeshCommandBatchIntermediateFuture() = default;

		// Sole completion path: fulfils FinalPromise on the game thread with the
		// render-thread result. (The former second, game-thread handshake half never
		// actually fulfilled the promise — this method already did so unconditionally —
		// so it was dropped along with its extra per-commit game-thread task.)
		void FinalizeRenderThread(ERealtimeMeshProxyUpdateStatus Status);
	};

	struct REALTIMEMESHCOMPONENT_API FRealtimeMeshProxyUpdateBuilder
	{
	public:
		using TaskFunctionType = TUniqueFunction<void(FRHICommandListBase&, FRealtimeMeshProxy&)>;
	private:
		TArray<TaskFunctionType> Tasks;
		TArray<FRealtimeMeshInPlaceStreamUpdate> InPlaceUpdates;
		TOptional<bool> bNewHasNaniteData;
		uint32 bRequiresProxyRecreate : 1;
		uint32 bIsIgnoringCommands : 1;
	public:
		FRealtimeMeshProxyUpdateBuilder(bool bShouldIgnoreCommands = false)
			: bRequiresProxyRecreate(false)
			, bIsIgnoringCommands(bShouldIgnoreCommands)
		{ }
		UE_NONCOPYABLE(FRealtimeMeshProxyUpdateBuilder);

		FORCEINLINE bool IsValid() const { return !bIsIgnoringCommands; }
		FORCEINLINE operator bool() const { return IsValid(); }

		TFuture<ERealtimeMeshProxyUpdateStatus> Commit(const TSharedRef<const FRealtimeMesh>& Mesh);

		void MarkForProxyRecreate() { bRequiresProxyRecreate = true; }
		void ClearProxyRecreate() { bRequiresProxyRecreate = false; }

		void SetHasNaniteData(bool bHasNaniteData) { bNewHasNaniteData = bHasNaniteData; }

		void AddMeshTask(TUniqueFunction<void(FRHICommandListBase&, FRealtimeMeshProxy&)>&& Function, bool bInRequiresProxyRecreate = true);

		template <typename MeshType>
		void AddMeshTask(TUniqueFunction<void(FRHICommandListBase&, MeshType&)>&& Function, bool bInRequiresProxyRecreate = true)
		{
			AddMeshTask([Func = MoveTemp(Function)](FRHICommandListBase& RHICmdList, FRealtimeMeshProxy& LOD)
			{
				Func(RHICmdList, static_cast<MeshType&>(LOD));
			}, bInRequiresProxyRecreate);
		}

		void AddLODTask(const FRealtimeMeshLODKey& LODKey, TUniqueFunction<void(FRHICommandListBase&, FRealtimeMeshLODProxy&)>&& Function, bool bInRequiresProxyRecreate = true);

		template <typename LODProxyType>
		void AddLODTask(const FRealtimeMeshLODKey& LODKey, TUniqueFunction<void(FRHICommandListBase&, LODProxyType&)>&& Function, bool bInRequiresProxyRecreate = true)
		{
			AddLODTask(LODKey, [Func = MoveTemp(Function)](FRHICommandListBase& RHICmdList, FRealtimeMeshLODProxy& LOD)
			{
				Func(RHICmdList, static_cast<LODProxyType&>(LOD));
			}, bInRequiresProxyRecreate);
		}

		void AddBufferSetTask(const FRealtimeMeshBufferSetKey& BufferSetKey, TUniqueFunction<void(FRHICommandListBase&, FRealtimeMeshBufferSetProxy&)>&& Function,
		                      bool bInRequiresProxyRecreate = true);

		template <typename BufferSetProxyType>
		void AddBufferSetTask(const FRealtimeMeshBufferSetKey& BufferSetKey, TUniqueFunction<void(FRHICommandListBase&, BufferSetProxyType&)>&& Function,
		                      bool bInRequiresProxyRecreate = true)
		{
			AddBufferSetTask(BufferSetKey, [Func = MoveTemp(Function)](FRHICommandListBase& RHICmdList, FRealtimeMeshBufferSetProxy& BufferSet)
			{
				Func(RHICmdList, static_cast<BufferSetProxyType&>(BufferSet));
			}, bInRequiresProxyRecreate);
		}

		// Back-compat shims forwarding to the BufferSet-named API.
		void AddSectionGroupTask(const FRealtimeMeshBufferSetKey& BufferSetKey, TUniqueFunction<void(FRHICommandListBase&, FRealtimeMeshBufferSetProxy&)>&& Function,
		                         bool bInRequiresProxyRecreate = true)
		{
			AddBufferSetTask(BufferSetKey, MoveTemp(Function), bInRequiresProxyRecreate);
		}

		template <typename BufferSetProxyType>
		void AddSectionGroupTask(const FRealtimeMeshBufferSetKey& BufferSetKey, TUniqueFunction<void(FRHICommandListBase&, BufferSetProxyType&)>&& Function,
		                         bool bInRequiresProxyRecreate = true)
		{
			AddBufferSetTask<BufferSetProxyType>(BufferSetKey, MoveTemp(Function), bInRequiresProxyRecreate);
		}

		// Sugar: ship a caller-built FRealtimeMeshGPUStream into the named buffer set
		// without writing the lambda by hand. Wraps an AddBufferSetTask that calls
		// FRealtimeMeshBufferSetProxy::RegisterGPUStream on the render thread.
		void AddGPUStreamRegistration(const FRealtimeMeshBufferSetKey& BufferSetKey, const TSharedRef<FRealtimeMeshGPUStream>& InStream, bool bInRequiresProxyRecreate = true)
		{
			AddBufferSetTask(BufferSetKey, [InStream](FRHICommandListBase& RHICmdList, FRealtimeMeshBufferSetProxy& BufferSet)
			{
				BufferSet.RegisterGPUStream(RHICmdList, InStream);
			}, bInRequiresProxyRecreate);
		}

		// Queue a fast in-place stream update (see FRealtimeMeshInPlaceStreamUpdate). When a
		// committed batch consists solely of in-place updates and requires no proxy
		// recreate, Commit takes the fast render-thread path: mutate the current published
		// version in place, with no clone and no new version. Otherwise these are folded in
		// as ordinary reallocating stream updates so nothing is lost.
		void AddInPlaceStreamUpdate(const FRealtimeMeshBufferSetKey& BufferSetKey, const FRealtimeMeshSectionGroupStreamUpdateDataRef& UpdateData, int32 ElementOffset, int32 NumElements)
		{
			InPlaceUpdates.Add(FRealtimeMeshInPlaceStreamUpdate{ BufferSetKey, UpdateData, ElementOffset, NumElements });
		}

		void AddSectionTask(const FRealtimeMeshSectionKey& SectionKey, TUniqueFunction<void(FRHICommandListBase&, FRealtimeMeshSectionProxy&)>&& Function, bool bInRequiresProxyRecreate = true);

		template <typename SectionProxyType>
		void AddSectionTask(const FRealtimeMeshSectionKey& SectionKey, TUniqueFunction<void(FRHICommandListBase&, SectionProxyType&)>&& Function, bool bInRequiresProxyRecreate = true)
		{
			AddSectionTask(SectionKey, [Func = MoveTemp(Function)](FRHICommandListBase& RHICmdList, FRealtimeMeshSectionProxy& Section)
			{
				Func(RHICmdList, static_cast<SectionProxyType&>(Section));
			}, bInRequiresProxyRecreate);
		}
	};
}
