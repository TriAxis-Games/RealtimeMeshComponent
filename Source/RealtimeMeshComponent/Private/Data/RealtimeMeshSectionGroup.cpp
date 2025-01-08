// Copyright (c) 2015-2025 TriAxis Games, L.L.C. All Rights Reserved.

#include "Data/RealtimeMeshSectionGroup.h"
#include "Data/RealtimeMeshShared.h"
#include "Data/RealtimeMeshSection.h"
#include "Data/RealtimeMeshUpdateBuilder.h"
#include "RenderProxy/RealtimeMeshGPUBuffer.h"
#include "RenderProxy/RealtimeMeshProxyCommandBatch.h"
#include "RenderProxy/RealtimeMeshSectionGroupProxy.h"
#if RMC_ENGINE_ABOVE_5_2
#include "Logging/MessageLog.h"
#endif

#define LOCTEXT_NAMESPACE "RealtimeMesh"

namespace RealtimeMesh
{
	FRealtimeMeshSectionGroup::FRealtimeMeshSectionGroup(const FRealtimeMeshSharedResourcesRef& InSharedResources, const FRealtimeMeshSectionGroupKey& InKey)
		: SharedResources(InSharedResources)
		, Key(InKey)
	{
	}

	FRealtimeMeshStreamRange FRealtimeMeshSectionGroup::GetInUseRange(const FRealtimeMeshLockContext& LockContext) const
	{
		TOptional<FRealtimeMeshStreamRange> NewRange;
		for (const auto& Section : Sections)
		{
			if (!NewRange.IsSet())
			{
				NewRange = Section->GetStreamRange(LockContext);
				continue;
			}
			NewRange = NewRange->Hull(Section->GetStreamRange(LockContext));
		}

		return NewRange.IsSet() ? *NewRange : FRealtimeMeshStreamRange::Empty();
	}

	TOptional<FBoxSphereBounds3f> FRealtimeMeshSectionGroup::GetLocalBounds(const FRealtimeMeshLockContext& LockContext) const
	{
		return Bounds.Get();
	}

	bool FRealtimeMeshSectionGroup::HasSections(const FRealtimeMeshLockContext& LockContext) const
	{
		return Sections.Num() > 0;
	}

	int32 FRealtimeMeshSectionGroup::NumSections(const FRealtimeMeshLockContext& LockContext) const
	{
		return Sections.Num();
	}

	bool FRealtimeMeshSectionGroup::HasStreams(const FRealtimeMeshLockContext& LockContext) const
	{
		return Streams.Num() > 0;
	}

	TSet<FRealtimeMeshStreamKey> FRealtimeMeshSectionGroup::GetStreamKeys(const FRealtimeMeshLockContext& LockContext) const
	{
		return Streams;
	}

	TSet<FRealtimeMeshSectionKey> FRealtimeMeshSectionGroup::GetSectionKeys(const FRealtimeMeshLockContext& LockContext) const
	{
		TSet<FRealtimeMeshSectionKey> SectionKeys;
		for (const auto& Section : Sections)
		{
			SectionKeys.Add(Section->GetKey(LockContext));
		}
		return SectionKeys;
	}

	FRealtimeMeshSectionPtr FRealtimeMeshSectionGroup::GetSection(const FRealtimeMeshLockContext& LockContext, const FRealtimeMeshSectionKey& SectionKey) const
	{
		if (SectionKey.IsPartOf(Key) && Sections.Contains(SectionKey))
		{
			return *Sections.Find(SectionKey);
		}
		return nullptr;
	}

	void FRealtimeMeshSectionGroup::Initialize(FRealtimeMeshUpdateContext& UpdateContext, const FRealtimeMeshSectionGroupConfig& InConfig)
	{
		Config = InConfig;
		Streams.Empty();
		Sections.Empty();
		Bounds.Reset();

		InitializeProxy(UpdateContext);

		UpdateContext.GetState().ConfigDirtyTree.Flag(Key);
		UpdateContext.GetState().BoundsDirtyTree.Flag(Key);
	}

	void FRealtimeMeshSectionGroup::Reset(FRealtimeMeshUpdateContext& UpdateContext)
	{
		Initialize(UpdateContext, FRealtimeMeshSectionGroupConfig());
		
		/*
		const auto StreamsToRemove = GetStreamKeys(UpdateContext);
		const auto SectionsToRemove = GetSectionKeys(UpdateContext);

		Config = FRealtimeMeshSectionGroupConfig();
		Streams.Empty();
		Sections.Empty();
		Bounds.Reset();

		if (auto ProxyBuilder = UpdateContext.GetProxyBuilder())
		{
			ProxyBuilder->AddSectionGroupTask(Key, [](FRHICommandListBase& RHICmdList, FRealtimeMeshSectionGroupProxy& Proxy)
			{
				Proxy.Reset();
			});

			InitializeProxy(UpdateContext);
		}

		UpdateContext.GetState().ConfigDirtyTree.Flag(Key);
		UpdateContext.GetState().BoundsDirtyTree.Flag(Key);

		SharedResources->BroadcastSectionGroupBoundsChanged(Key);
		SharedResources->BroadcastSectionGroupInUseRangeChanged(Key);
		SharedResources->BroadcastSectionChanged(SectionsToRemove, ERealtimeMeshChangeType::Removed);
		SharedResources->BroadcastStreamChanged(Key, StreamsToRemove, ERealtimeMeshChangeType::Removed);*/
	}

	void FRealtimeMeshSectionGroup::SetOverrideBounds(FRealtimeMeshUpdateContext& UpdateContext, const FBoxSphereBounds3f& InBounds)
	{
		Bounds.SetUserSetBounds(InBounds);
		UpdateContext.GetState().BoundsDirtyTree.Flag(Key);
	}

	void FRealtimeMeshSectionGroup::ClearOverrideBounds(FRealtimeMeshUpdateContext& UpdateContext)
	{
		Bounds.ClearUserSetBounds();
		UpdateContext.GetState().BoundsDirtyTree.Flag(Key);
	}

	void FRealtimeMeshSectionGroup::UpdateConfig(FRealtimeMeshUpdateContext& UpdateContext, const FRealtimeMeshSectionGroupConfig& InConfig)
	{
		UpdateConfig(UpdateContext, [InConfig](FRealtimeMeshSectionGroupConfig& ExistingConfig) { ExistingConfig = InConfig; });
	}

	void FRealtimeMeshSectionGroup::UpdateConfig(FRealtimeMeshUpdateContext& UpdateContext, TFunction<void(FRealtimeMeshSectionGroupConfig&)> EditFunc)
	{
		bool bShouldRecreateProxy = ShouldRecreateProxyOnChange(UpdateContext);
		EditFunc(Config);
		bShouldRecreateProxy |= ShouldRecreateProxyOnChange(UpdateContext);

		if (auto ProxyBuilder = UpdateContext.GetProxyBuilder())
		{
			ProxyBuilder->AddSectionGroupTask(Key, [Config = Config](FRHICommandListBase& RHICmdList, FRealtimeMeshSectionGroupProxy& Proxy)
			{
				Proxy.UpdateConfig(Config);
			}, bShouldRecreateProxy);
		}

		UpdateContext.GetState().ConfigDirtyTree.Flag(Key);
	}

	void FRealtimeMeshSectionGroup::CreateOrUpdateStream(FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshStream&& Stream)
	{
		const auto StreamKey = Stream.GetStreamKey();
		bool bAlreadyExisted = false;

		// Make sure we have the stream registered
		Streams.FindOrAdd(StreamKey, &bAlreadyExisted);

		// Create the update data for the GPU
		if (SharedResources->WantsStreamOnGPU(StreamKey))
		{
			if (auto ProxyBuilder = UpdateContext.GetProxyBuilder())
			{
				if (Stream.Num() > 0)
				{
					FRealtimeMeshStream StreamCopy(Stream);
					const auto UpdateData = MakeShared<FRealtimeMeshSectionGroupStreamUpdateData>(MoveTemp(StreamCopy), EBufferUsageFlags::Static);
					UpdateData->CreateBufferAsyncIfPossible(UpdateContext);

					ProxyBuilder->AddSectionGroupTask(Key, [UpdateData = UpdateData](FRHICommandListBase& RHICmdList, FRealtimeMeshSectionGroupProxy& Proxy)
					{
						Proxy.CreateOrUpdateStream(RHICmdList, UpdateData);
					}, ShouldRecreateProxyOnChange(UpdateContext));
				}
				else
				{
					ProxyBuilder->AddSectionGroupTask(Key, [StreamKey](FRHICommandListBase& RHICmdList, FRealtimeMeshSectionGroupProxy& Proxy)
					{
						Proxy.RemoveStream(StreamKey);
					}, ShouldRecreateProxyOnChange(UpdateContext));
				}
			}
		}

		UpdateContext.GetState().StreamDirtyTree.Flag(Key, StreamKey);
	}

	void FRealtimeMeshSectionGroup::RemoveStream(FRealtimeMeshUpdateContext& UpdateContext, const FRealtimeMeshStreamKey& StreamKey)
	{
		if (Streams.Remove(StreamKey))
		{
			if (SharedResources->WantsStreamOnGPU(StreamKey))
			{
				if (auto ProxyBuilder = UpdateContext.GetProxyBuilder())
				{
					ProxyBuilder->AddSectionGroupTask(Key, [StreamKey](FRHICommandListBase& RHICmdList, FRealtimeMeshSectionGroupProxy& Proxy)
					{
						Proxy.RemoveStream(StreamKey);
					}, ShouldRecreateProxyOnChange(UpdateContext));
				}
			}
		}

		UpdateContext.GetState().StreamDirtyTree.Flag(Key, StreamKey);
	}

	void FRealtimeMeshSectionGroup::SetAllStreams(FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshStreamSet&& InStreams)
	{
		TSet<FRealtimeMeshStreamKey> ExistingStreamKeys = Streams;

		// Remove all old streams
		for (const FRealtimeMeshStreamKey& StreamKey : ExistingStreamKeys)
		{
			if (!InStreams.Contains(StreamKey))
			{
				RemoveStream(UpdateContext, StreamKey);
			}
		}

		// Create/Update streams
		InStreams.ForEach([&](FRealtimeMeshStream& Stream)
		{
			CreateOrUpdateStream(UpdateContext, MoveTemp(Stream));			
		});
	}

	void FRealtimeMeshSectionGroup::CreateOrUpdateSection(FRealtimeMeshUpdateContext& UpdateContext, const FRealtimeMeshSectionKey& SectionKey,
		const FRealtimeMeshSectionConfig& InConfig, const FRealtimeMeshStreamRange& InStreamRange)
	{
		check(SectionKey.IsPartOf(Key));

		const bool bExisted = Sections.Contains(SectionKey);

		if (!bExisted)
		{
			const auto Section = SharedResources->CreateSection(SectionKey);
			Sections.Add(Section);

			if (auto ProxyBuilder = UpdateContext.GetProxyBuilder())
			{
				ProxyBuilder->AddSectionGroupTask(Key, [SectionKey](FRHICommandListBase& RHICmdList, FRealtimeMeshSectionGroupProxy& Proxy)
				{
					Proxy.CreateSectionIfNotExists(SectionKey);
				});
			}
		}

		const auto& Section = *Sections.Find(SectionKey);
		Section->Initialize(UpdateContext, InConfig, InStreamRange);
	}

	void FRealtimeMeshSectionGroup::RemoveSection(FRealtimeMeshUpdateContext& UpdateContext, const FRealtimeMeshSectionKey& SectionKey)
	{
		if (Sections.Remove(SectionKey))
		{
			if (auto ProxyBuilder = UpdateContext.GetProxyBuilder())
			{
				ProxyBuilder->AddSectionGroupTask(Key, [SectionKey](FRHICommandListBase& RHICmdList, FRealtimeMeshSectionGroupProxy& Proxy)
				{
					Proxy.RemoveSection(SectionKey);
				}, ShouldRecreateProxyOnChange(UpdateContext));
			}
		}
	}

	bool FRealtimeMeshSectionGroup::Serialize(FArchive& Ar)
	{
		int32 NumSections = Sections.Num();
		Ar << NumSections;

		if (Ar.IsLoading())
		{
			Sections.Empty();
			for (int32 Index = 0; Index < NumSections; Index++)
			{
				FRealtimeMeshSectionKey SectionKey;
				if (Ar.CustomVer(RealtimeMesh::FRealtimeMeshVersion::GUID) < FRealtimeMeshVersion::DataRestructure)
				{
					int32 SectionIndex;
					Ar << SectionIndex;
					SectionKey = FRealtimeMeshSectionKey::Create(Key, SectionIndex);
				}
				else
				{
					FName SectionName;
					Ar << SectionName;
					SectionKey = FRealtimeMeshSectionKey::Create(Key, SectionName);
				}

				auto Section = SharedResources->CreateSection(SectionKey);
				Section->Serialize(Ar);
				Sections.Add(Section);
			}
		}
		else
		{
			for (const auto& Section : Sections)
			{
				FName SectionName = Section->GetKey_AssumesLocked().Name();
				Ar << SectionName;
				Section->Serialize(Ar);
			}
		}

		Ar << Config;
		Ar << Bounds;
		if (Ar.CustomVer(FRealtimeMeshVersion::GUID) < FRealtimeMeshVersion::DataRestructure)
		{
			FRealtimeMeshStreamRange OldRange;
			Ar << OldRange;
		}
		return true;
	}

	void FRealtimeMeshSectionGroup::InitializeProxy(FRealtimeMeshUpdateContext& UpdateContext)
	{
		if (auto ProxyBuilder = UpdateContext.GetProxyBuilder())
		{
			// We only send sections here, we rely on the derived to setup the streams
			for (const auto& Section : Sections)
			{
				ProxyBuilder->AddSectionGroupTask(Key, [SectionKey = Section->GetKey(UpdateContext), Config = Config](FRHICommandListBase& RHICmdList, FRealtimeMeshSectionGroupProxy& Proxy)
				{
					Proxy.CreateSectionIfNotExists(SectionKey);
					Proxy.UpdateConfig(Config);
				}, ShouldRecreateProxyOnChange(UpdateContext));

				Section->InitializeProxy(UpdateContext);
			}
		}
	}

	void FRealtimeMeshSectionGroup::FinalizeUpdate(FRealtimeMeshUpdateContext& UpdateContext)
	{
		for (const auto& Section : Sections)
		{
			Section->FinalizeUpdate(UpdateContext);
		}
		
		// Update bounds
		if (UpdateContext.GetState().BoundsDirtyTree.IsDirty(Key) && !Bounds.HasUserSetBounds())
		{
			TOptional<FBoxSphereBounds3f> NewBounds;
			for (const auto& Section : Sections)
			{
				auto SectionBounds = Section->GetLocalBounds(UpdateContext);
				if (SectionBounds.IsSet())
				{
					if (!NewBounds.IsSet())
					{
						NewBounds = *SectionBounds;
						continue;
					}
					NewBounds = *NewBounds + *SectionBounds;
				}
			}
			if (NewBounds.IsSet())
			{
				Bounds.SetComputedBounds(*NewBounds);
			}
			else
			{
				Bounds.ClearCachedValue();
			}

			UpdateContext.GetState().BoundsDirtyTree.Flag(Key.LOD());
		}
	}

	void FRealtimeMeshSectionGroup::MarkBoundsDirtyIfNotOverridden(FRealtimeMeshUpdateContext& UpdateContext)
	{	
		Bounds.ClearCachedValue();
		if (!Bounds.HasUserSetBounds())
		{
			UpdateContext.GetState().BoundsDirtyTree.Flag(Key);
		}
	}




}

#undef LOCTEXT_NAMESPACE
