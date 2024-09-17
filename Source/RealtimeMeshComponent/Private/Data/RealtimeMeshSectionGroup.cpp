// Copyright (c) 2015-2024 TriAxis Games, L.L.C. All Rights Reserved.

#include "Data/RealtimeMeshSectionGroup.h"
#include "RealtimeMeshGuard.h"
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
		SharedResources->OnSectionBoundsChanged().AddRaw(this, &FRealtimeMeshSectionGroup::HandleSectionBoundsChanged);
		SharedResources->OnSectionChanged().AddRaw(this, &FRealtimeMeshSectionGroup::HandleSectionChanged);
	}

	FRealtimeMeshSectionGroup::~FRealtimeMeshSectionGroup()
	{
		SharedResources->OnSectionBoundsChanged().RemoveAll(this);
		SharedResources->OnSectionChanged().RemoveAll(this);
	}


	FRealtimeMeshStreamRange FRealtimeMeshSectionGroup::GetInUseRange() const
	{
		FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources);
		TOptional<FRealtimeMeshStreamRange> NewRange;
		for (const auto& Section : Sections)
		{
			if (!NewRange.IsSet())
			{
				NewRange = Section->GetStreamRange();
				continue;
			}
			NewRange = NewRange->Hull(Section->GetStreamRange());
		}

		return NewRange.IsSet() ? *NewRange : FRealtimeMeshStreamRange::Empty();
	}

	FBoxSphereBounds3f FRealtimeMeshSectionGroup::GetLocalBounds() const
	{
		FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources);
		return Bounds.GetBounds([&]() { return CalculateBounds(); });
	}

	bool FRealtimeMeshSectionGroup::HasSections() const
	{
		FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources);
		return Sections.Num() > 0;
	}

	int32 FRealtimeMeshSectionGroup::NumSections() const
	{
		FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources);
		return Sections.Num();
	}

	bool FRealtimeMeshSectionGroup::HasStreams() const
	{
		FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources);
		return Streams.Num() > 0;
	}

	TSet<FRealtimeMeshStreamKey> FRealtimeMeshSectionGroup::GetStreamKeys() const
	{
		FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources);
		return Streams;
	}

	TSet<FRealtimeMeshSectionKey> FRealtimeMeshSectionGroup::GetSectionKeys() const
	{
		FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources);
		TSet<FRealtimeMeshSectionKey> SectionKeys;
		for (const auto& Section : Sections)
		{
			SectionKeys.Add(Section->GetKey());
		}
		return SectionKeys;
	}

	FRealtimeMeshSectionPtr FRealtimeMeshSectionGroup::GetSection(const FRealtimeMeshSectionKey& SectionKey) const
	{
		FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources);
		if (SectionKey.IsPartOf(Key) && Sections.Contains(SectionKey))
		{
			return *Sections.Find(SectionKey);
		}
		return nullptr;
	}

	void FRealtimeMeshSectionGroup::Initialize(FRealtimeMeshProxyUpdateBuilder& ProxyBuilder, const FRealtimeMeshSectionGroupConfig& InConfig)
	{
		FRealtimeMeshScopeGuardWriteCheck LockCheck(SharedResources);
		Config = InConfig;
		Bounds.Reset();

		if (ProxyBuilder)
		{
			InitializeProxy(ProxyBuilder);
		}
	}

	TFuture<ERealtimeMeshProxyUpdateStatus> FRealtimeMeshSectionGroup::Reset()
	{
		FRealtimeMeshUpdateContext UpdateContext(SharedResources);
		Reset(UpdateContext);
		return UpdateContext.Commit();
	}

	void FRealtimeMeshSectionGroup::Reset(FRealtimeMeshProxyUpdateBuilder& ProxyBuilder)
	{
		FRealtimeMeshScopeGuardWriteCheck LockCheck(SharedResources);

		const auto StreamsToRemove = GetStreamKeys();
		const auto SectionsToRemove = GetSectionKeys();

		Config = FRealtimeMeshSectionGroupConfig();
		Streams.Empty();
		Sections.Empty();
		Bounds.Reset();

		if (ProxyBuilder)
		{
			ProxyBuilder.AddSectionGroupTask(Key, [](FRHICommandListBase& RHICmdList, FRealtimeMeshSectionGroupProxy& Proxy)
			{
				Proxy.Reset();
			});

			InitializeProxy(ProxyBuilder);
		}

		SharedResources->BroadcastSectionGroupBoundsChanged(Key);
		SharedResources->BroadcastSectionGroupInUseRangeChanged(Key);
		SharedResources->BroadcastSectionChanged(SectionsToRemove, ERealtimeMeshChangeType::Removed);
		SharedResources->BroadcastStreamChanged(Key, StreamsToRemove, ERealtimeMeshChangeType::Removed);
	}

	void FRealtimeMeshSectionGroup::SetOverrideBounds(const FBoxSphereBounds3f& InBounds)
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources);
		Bounds.SetUserSetBounds(InBounds);
		SharedResources->BroadcastSectionGroupBoundsChanged(Key);
	}

	void FRealtimeMeshSectionGroup::ClearOverrideBounds()
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources);
		Bounds.ClearUserSetBounds();
		SharedResources->BroadcastSectionGroupBoundsChanged(Key);
	}

	TFuture<ERealtimeMeshProxyUpdateStatus> FRealtimeMeshSectionGroup::UpdateConfig(const FRealtimeMeshSectionGroupConfig& InConfig)
	{
		FRealtimeMeshUpdateContext UpdateContext(SharedResources);
		UpdateConfig(UpdateContext, [InConfig](FRealtimeMeshSectionGroupConfig& ExistingConfig) { ExistingConfig = InConfig; });
		return UpdateContext.Commit();
	}

	void FRealtimeMeshSectionGroup::UpdateConfig(FRealtimeMeshProxyUpdateBuilder& ProxyBuilder, const FRealtimeMeshSectionGroupConfig& InConfig)
	{
		UpdateConfig(ProxyBuilder, [InConfig](FRealtimeMeshSectionGroupConfig& ExistingConfig) { ExistingConfig = InConfig; });
	}

	TFuture<ERealtimeMeshProxyUpdateStatus> FRealtimeMeshSectionGroup::UpdateConfig(TFunction<void(FRealtimeMeshSectionGroupConfig&)> EditFunc)
	{
		FRealtimeMeshUpdateContext UpdateContext(SharedResources);
		UpdateConfig(UpdateContext, EditFunc);
		return UpdateContext.Commit();
	}

	void FRealtimeMeshSectionGroup::UpdateConfig(FRealtimeMeshProxyUpdateBuilder& ProxyBuilder, TFunction<void(FRealtimeMeshSectionGroupConfig&)> EditFunc)
	{
		FRealtimeMeshScopeGuardWriteCheck LockCheck(SharedResources);

		bool bShouldRecreateProxy = ShouldRecreateProxyOnChange();
		EditFunc(Config);
		bShouldRecreateProxy |= ShouldRecreateProxyOnChange();

		if (ProxyBuilder)
		{
			ProxyBuilder.AddSectionGroupTask(Key, [Config = Config](FRHICommandListBase& RHICmdList, FRealtimeMeshSectionGroupProxy& Proxy)
			{
				Proxy.UpdateConfig(Config);
			}, bShouldRecreateProxy);
		}

		SharedResources->BroadcastSectionGroupChanged(Key, ERealtimeMeshChangeType::Updated);
	}

	TFuture<ERealtimeMeshProxyUpdateStatus> FRealtimeMeshSectionGroup::CreateOrUpdateStream(FRealtimeMeshStream&& Stream)
	{
		FRealtimeMeshUpdateContext UpdateContext(SharedResources);
		CreateOrUpdateStream(UpdateContext, MoveTemp(Stream));
		return UpdateContext.Commit();
	}

	void FRealtimeMeshSectionGroup::CreateOrUpdateStream(FRealtimeMeshProxyUpdateBuilder& ProxyBuilder, FRealtimeMeshStream&& Stream)
	{
		FRealtimeMeshScopeGuardWriteCheck LockCheck(SharedResources);

		const auto StreamKey = Stream.GetStreamKey();
		bool bAlreadyExisted = false;

		// Make sure we have the stream registered
		Streams.FindOrAdd(StreamKey, &bAlreadyExisted);

		// Create the update data for the GPU
		if (ProxyBuilder && SharedResources->WantsStreamOnGPU(StreamKey))
		{
			if (Stream.Num() > 0)
			{
				const auto UpdateData = MakeShared<FRealtimeMeshSectionGroupStreamUpdateData>(MoveTemp(Stream));
				UpdateData->ConfigureBuffer(EBufferUsageFlags::Static, true);

				ProxyBuilder.AddSectionGroupTask(Key, [UpdateData = UpdateData](FRHICommandListBase& RHICmdList, FRealtimeMeshSectionGroupProxy& Proxy)
				{
					Proxy.CreateOrUpdateStream(RHICmdList, UpdateData);
				}, ShouldRecreateProxyOnChange());
			}
			else
			{
				ProxyBuilder.AddSectionGroupTask(Key, [StreamKey](FRHICommandListBase& RHICmdList, FRealtimeMeshSectionGroupProxy& Proxy)
				{
					Proxy.RemoveStream(StreamKey);
				});
			}
		}

		SharedResources->BroadcastStreamChanged(Key, StreamKey, bAlreadyExisted ? ERealtimeMeshChangeType::Updated : ERealtimeMeshChangeType::Added);
	}

	TFuture<ERealtimeMeshProxyUpdateStatus> FRealtimeMeshSectionGroup::RemoveStream(const FRealtimeMeshStreamKey& StreamKey)
	{
		FRealtimeMeshUpdateContext UpdateContext(SharedResources);
		RemoveStream(UpdateContext, StreamKey);
		return UpdateContext.Commit();
	}

	void FRealtimeMeshSectionGroup::RemoveStream(FRealtimeMeshProxyUpdateBuilder& ProxyBuilder, const FRealtimeMeshStreamKey& StreamKey)
	{
		FRealtimeMeshScopeGuardWriteCheck LockCheck(SharedResources);

		if (Streams.Remove(StreamKey))
		{
			if (ProxyBuilder && SharedResources->WantsStreamOnGPU(StreamKey))
			{
				ProxyBuilder.AddSectionGroupTask(Key, [StreamKey](FRHICommandListBase& RHICmdList, FRealtimeMeshSectionGroupProxy& Proxy)
				{
					Proxy.RemoveStream(StreamKey);
				}, ShouldRecreateProxyOnChange());
			}
		}

		SharedResources->BroadcastStreamChanged(Key, StreamKey, ERealtimeMeshChangeType::Removed);
	}

	TFuture<ERealtimeMeshProxyUpdateStatus> FRealtimeMeshSectionGroup::SetAllStreams(const FRealtimeMeshStreamSet& InStreams)
	{
		FRealtimeMeshUpdateContext UpdateContext(SharedResources);
		FRealtimeMeshStreamSet CopySet(InStreams);
		SetAllStreams(UpdateContext, MoveTemp(CopySet));
		return UpdateContext.Commit();
	}

	void FRealtimeMeshSectionGroup::SetAllStreams(FRealtimeMeshProxyUpdateBuilder& ProxyBuilder, const FRealtimeMeshStreamSet& InStreams)
	{
		FRealtimeMeshStreamSet CopySet(InStreams);
		SetAllStreams(ProxyBuilder, MoveTemp(CopySet));
	}

	TFuture<ERealtimeMeshProxyUpdateStatus> FRealtimeMeshSectionGroup::SetAllStreams(FRealtimeMeshStreamSet&& InStreams)
	{
		FRealtimeMeshUpdateContext UpdateContext(SharedResources);
		SetAllStreams(UpdateContext, InStreams);
		return UpdateContext.Commit();
	}

	void FRealtimeMeshSectionGroup::SetAllStreams(FRealtimeMeshProxyUpdateBuilder& ProxyBuilder, FRealtimeMeshStreamSet&& InStreams)
	{
		FRealtimeMeshScopeGuardWriteCheck LockCheck(SharedResources);

		TSet<FRealtimeMeshStreamKey> ExistingStreamKeys = Streams;

		// Remove all old streams
		for (const FRealtimeMeshStreamKey& StreamKey : ExistingStreamKeys)
		{
			if (!InStreams.Contains(StreamKey))
			{
				RemoveStream(ProxyBuilder, StreamKey);
			}
		}

		// Create/Update streams
		InStreams.ForEach([&](FRealtimeMeshStream& Stream)
		{
			CreateOrUpdateStream(ProxyBuilder, MoveTemp(Stream));			
		});
	}

	TFuture<ERealtimeMeshProxyUpdateStatus> FRealtimeMeshSectionGroup::CreateOrUpdateSection(const FRealtimeMeshSectionKey& SectionKey, const FRealtimeMeshSectionConfig& InConfig,
	                                                                                         const FRealtimeMeshStreamRange& InStreamRange)
	{
		FRealtimeMeshUpdateContext UpdateContext(SharedResources);
		CreateOrUpdateSection(UpdateContext, SectionKey, InConfig, InStreamRange);
		return UpdateContext.Commit();
	}

	void FRealtimeMeshSectionGroup::CreateOrUpdateSection(FRealtimeMeshProxyUpdateBuilder& ProxyBuilder, const FRealtimeMeshSectionKey& SectionKey,
		const FRealtimeMeshSectionConfig& InConfig, const FRealtimeMeshStreamRange& InStreamRange)
	{
		check(SectionKey.IsPartOf(Key));

		FRealtimeMeshScopeGuardWriteCheck LockCheck(SharedResources);

		const bool bExisted = Sections.Contains(SectionKey);

		if (!bExisted)
		{
			const auto Section = SharedResources->CreateSection(SectionKey);
			Sections.Add(Section);

			ProxyBuilder.AddSectionGroupTask(Key, [SectionKey](FRHICommandListBase& RHICmdList, FRealtimeMeshSectionGroupProxy& Proxy)
			{
				Proxy.CreateSectionIfNotExists(SectionKey);
			});
		}

		const auto& Section = *Sections.Find(SectionKey);
		Section->Initialize(ProxyBuilder, InConfig, InStreamRange);

		SharedResources->BroadcastSectionChanged(SectionKey, bExisted ? ERealtimeMeshChangeType::Updated : ERealtimeMeshChangeType::Added);
	}

	TFuture<ERealtimeMeshProxyUpdateStatus> FRealtimeMeshSectionGroup::RemoveSection(const FRealtimeMeshSectionKey& SectionKey)
	{
		FRealtimeMeshUpdateContext UpdateContext(SharedResources);
		RemoveSection(UpdateContext, SectionKey);
		return UpdateContext.Commit();
	}

	void FRealtimeMeshSectionGroup::RemoveSection(FRealtimeMeshProxyUpdateBuilder& ProxyBuilder, const FRealtimeMeshSectionKey& SectionKey)
	{
		FRealtimeMeshScopeGuardWriteCheck LockCheck(SharedResources);

		if (Sections.Remove(SectionKey) && ProxyBuilder)
		{
			ProxyBuilder.AddSectionGroupTask(Key, [SectionKey](FRHICommandListBase& RHICmdList, FRealtimeMeshSectionGroupProxy& Proxy)
			{
				Proxy.RemoveSection(SectionKey);
			}, ShouldRecreateProxyOnChange());
		}

		SharedResources->BroadcastSectionChanged(SectionKey, ERealtimeMeshChangeType::Removed);
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
				FName SectionName = Section->GetKey().Name();
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

	void FRealtimeMeshSectionGroup::InitializeProxy(FRealtimeMeshProxyUpdateBuilder& ProxyBuilder)
	{
		if (ProxyBuilder)
		{
			// We only send sections here, we rely on the derived to setup the streams
			for (const auto& Section : Sections)
			{
				ProxyBuilder.AddSectionGroupTask(Key, [SectionKey = Section->GetKey(), Config = Config](FRHICommandListBase& RHICmdList, FRealtimeMeshSectionGroupProxy& Proxy)
				{
					Proxy.CreateSectionIfNotExists(SectionKey);
					Proxy.UpdateConfig(Config);
				}, ShouldRecreateProxyOnChange());

				Section->InitializeProxy(ProxyBuilder);
			}
		}
	}

	void FRealtimeMeshSectionGroup::InvalidateBounds() const
	{
		Bounds.ClearCachedValue();
		SharedResources->BroadcastSectionGroupBoundsChanged(Key);
	}

	/*void FRealtimeMeshSectionGroup::ApplyStateUpdate(FRealtimeMeshProxyCommandBatch& Commands, FRealtimeMeshSectionGroupUpdateContext& Update)
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources);
		
		TSet<FRealtimeMeshStreamKey> UpdatedStreams;

		// Apply all stream updates
		if (Update.HasStreamSet())
		{
			for (FRealtimeMeshStreamRef Stream : Update.GetUpdatedStreams())
			{
				UpdatedStreams.Add(Stream->GetStreamKey());				
					
				const auto UpdateData = MakeShared<FRealtimeMeshSectionGroupStreamUpdateData>(MoveTemp(Stream.Get()));
				// TODO: Probably should get usage flags correctly?
				UpdateData->ConfigureBuffer(EBufferUsageFlags::Static, true);

				// Add the task for the proxy to update this stream
				Commands.AddSectionGroupTask(Key, [UpdateData](FRealtimeMeshSectionGroupProxy& SectionGroup)
				{
					SectionGroup.CreateOrUpdateStream(UpdateData);
				}, ShouldRecreateProxyOnStreamChange());					
			}
		}

		// Find the streams we need to remove
		TSet<FRealtimeMeshStreamKey> StreamsToRemove = Update.ShouldReplaceExistingStreams()?
			Streams.Difference(UpdatedStreams) :
			Update.GetStreamsToRemove().Difference(UpdatedStreams);

		if (StreamsToRemove.Num() > 0)
		{
			// Strip all unwanted streams
			for (const auto Stream : StreamsToRemove)
			{
				Streams.Remove(Stream);
			}
			
			// Final stream task to remove all unwanted streams
			Commands.AddSectionGroupTask(Key, [StreamsToRemove = StreamsToRemove.Array()](FRealtimeMeshSectionGroupProxy& SectionGroup)
			{
				for (const auto& Stream : StreamsToRemove)
				{
					SectionGroup.RemoveStream(Stream);
				}
			}, ShouldRecreateProxyOnStreamChange());				
		}



		TSet<FRealtimeMeshSectionKey> UpdatedSections;

		for (auto& SectionData : Update.GetUpdatedSections())
		{
			UpdatedSections.Add(SectionData.Key);
			
			if (!Sections.Contains(SectionData.Key))
			{
				Commands.AddSectionGroupTask(Key, [SectionKey = SectionData.Key](FRealtimeMeshSectionGroupProxy& Proxy)
				{
					Proxy.CreateSectionIfNotExists(SectionKey);
				}, ShouldRecreateProxyOnStreamChange());
				
				auto Section = SharedResources->GetClassFactory().CreateSection(SharedResources, SectionData.Key);
				Section->ApplyStateUpdate(Commands, SectionData.Value);
				Sections.Add(Section);
			}
			else
			{
				auto& Section = *Sections.Find(SectionData.Key);
				Section->ApplyStateUpdate(Commands, SectionData.Value);	
			}
		}

		TSet<FRealtimeMeshSectionKey> ExistingSections;
		for (const auto& Section : Sections)
		{
			ExistingSections.Add(Section->GetKey());
		}

		// Find the sections we need to remove
		TSet<FRealtimeMeshSectionKey> SectionsToRemove = Update.ShouldReplaceExistingSections()?
			ExistingSections.Difference(UpdatedSections) :
			Update.GetSectionsToRemove().Difference(UpdatedSections);

		if (SectionsToRemove.Num() > 0)
		{
			// Strip all unwanted streams
			for (const auto Section : SectionsToRemove)
			{
				Sections.Remove(Section);
			}
			
			// Final section task to remove all unwanted sections
			Commands.AddSectionGroupTask(Key, [SectionsToRemove = SectionsToRemove](FRealtimeMeshSectionGroupProxy& Proxy)
			{
				for (const auto& Section : SectionsToRemove)
				{
					Proxy.RemoveSection(Section);
				}
			}, ShouldRecreateProxyOnStreamChange());				
		}

		// Broadcast all stream added/removed events
		SharedResources->BroadcastSectionGroupStreamsChanged(Key, UpdatedStreams, StreamsToRemove);
	}*/

	FBoxSphereBounds3f FRealtimeMeshSectionGroup::CalculateBounds() const
	{
		FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources);

		TOptional<FBoxSphereBounds3f> NewBounds;
		for (const auto& Section : Sections)
		{
			if (!NewBounds.IsSet())
			{
				NewBounds = Section->GetLocalBounds();
				continue;
			}
			NewBounds = *NewBounds + Section->GetLocalBounds();
		}

		ScopeGuard.Unlock();

		return NewBounds.IsSet() ? *NewBounds : FBoxSphereBounds3f(FSphere3f(FVector3f::ZeroVector, 1.0f));
	}

	void FRealtimeMeshSectionGroup::HandleSectionChanged(const FRealtimeMeshSectionKey& RealtimeMeshSectionKey, ERealtimeMeshChangeType RealtimeMeshChange)
	{
		if (RealtimeMeshSectionKey.IsPartOf(Key))
		{
			FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources);
			Bounds.ClearCachedValue();
			SharedResources->BroadcastSectionGroupBoundsChanged(Key);
		}
	}

	void FRealtimeMeshSectionGroup::HandleSectionBoundsChanged(const FRealtimeMeshSectionKey& RealtimeMeshSectionKey)
	{
		if (RealtimeMeshSectionKey.IsPartOf(Key))
		{
			FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources);
			Bounds.ClearCachedValue();
			SharedResources->BroadcastSectionGroupBoundsChanged(Key);
		}
	}

	/*void FRealtimeMeshSectionGroup::HandleSectionStreamRangeChanged(const FRealtimeMeshSectionKey& RealtimeMeshSectionKey)
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources);

		TOptional<FRealtimeMeshStreamRange> NewRange;
		for (const auto& Section : Sections)
		{
			if (!NewRange.IsSet())
			{
				NewRange = Section->GetStreamRange();
				continue;
			}
			NewRange = NewRange->Hull(Section->GetStreamRange());
		}

		// Update with new segment or a zero length segment
		const auto FinalRange = NewRange.IsSet() ? *NewRange : FRealtimeMeshStreamRange::Empty;
		const bool bRangeChanged = InUseRange != FinalRange;
		InUseRange = FinalRange;
		
		ScopeGuard.Unlock();

		if (bRangeChanged)
		{
			SharedResources->BroadcastSectionGroupInUseRangeChanged(Key);
		}
	}*/
}

#undef LOCTEXT_NAMESPACE
