// Copyright (c) 2015-2025 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "RealtimeMeshCore.h"
#include "RealtimeMeshSectionProxy.h"


namespace RealtimeMesh
{
	struct FRealtimeMeshCommandBatchIntermediateFuture : public TSharedFromThis<FRealtimeMeshCommandBatchIntermediateFuture>
	{
		TSharedRef<TPromise<ERealtimeMeshProxyUpdateStatus>> FinalPromise;
		ERealtimeMeshProxyUpdateStatus Result;
		uint8 bRenderThreadReady : 1;
		uint8 bGameThreadReady : 1;
		uint8 bFinalized : 1;

		FRealtimeMeshCommandBatchIntermediateFuture();
		void FinalizeRenderThread(ERealtimeMeshProxyUpdateStatus Status);
		void FinalizeGameThread();
	};

	struct REALTIMEMESHCOMPONENT_API FRealtimeMeshProxyUpdateBuilder
	{
	public:
		using TaskFunctionType = TUniqueFunction<void(FRHICommandListBase&, FRealtimeMeshProxy&)>;
	private:
		TArray<TaskFunctionType> Tasks;
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

		void AddSectionGroupTask(const FRealtimeMeshSectionGroupKey& SectionGroupKey, TUniqueFunction<void(FRHICommandListBase&, FRealtimeMeshSectionGroupProxy&)>&& Function,
		                         bool bInRequiresProxyRecreate = true);

		template <typename SectionGroupProxyType>
		void AddSectionGroupTask(const FRealtimeMeshSectionGroupKey& SectionGroupKey, TUniqueFunction<void(FRHICommandListBase&, SectionGroupProxyType&)>&& Function,
		                         bool bInRequiresProxyRecreate = true)
		{
			AddSectionGroupTask(SectionGroupKey, [Func = MoveTemp(Function)](FRHICommandListBase& RHICmdList, FRealtimeMeshSectionGroupProxy& SectionGroup)
			{
				Func(RHICmdList, static_cast<SectionGroupProxyType&>(SectionGroup));
			}, bInRequiresProxyRecreate);
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
