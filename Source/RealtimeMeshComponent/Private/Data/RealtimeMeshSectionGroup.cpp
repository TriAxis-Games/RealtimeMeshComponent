// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#include "Data/RealtimeMeshSectionGroup.h"
#include "RealtimeMeshGuard.h"
#include "Data/RealtimeMeshShared.h"
#include "Data/RealtimeMeshSection.h"
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
		FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources->GetGuard());
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

		return NewRange.IsSet() ? *NewRange : FRealtimeMeshStreamRange::Empty;
	}

	FBoxSphereBounds3f FRealtimeMeshSectionGroup::GetLocalBounds() const
	{
		FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources->GetGuard());
		return Bounds.GetBounds([&]() { return CalculateBounds(); });
	}

	bool FRealtimeMeshSectionGroup::HasSections() const
	{
		FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources->GetGuard());
		return Sections.Num() > 0;
	}

	int32 FRealtimeMeshSectionGroup::NumSections() const
	{
		FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources->GetGuard());
		return Sections.Num();
	}

	bool FRealtimeMeshSectionGroup::HasStreams() const
	{
		FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources->GetGuard());
		return Streams.Num() > 0;
	}

	TSet<FRealtimeMeshStreamKey> FRealtimeMeshSectionGroup::GetStreamKeys() const
	{
		FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources->GetGuard());
		return Streams;
	}

	FRealtimeMeshSectionPtr FRealtimeMeshSectionGroup::GetSection(const FRealtimeMeshSectionKey& SectionKey) const
	{
		FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources->GetGuard());
		if (SectionKey.IsPartOf(Key) && Sections.Contains(SectionKey))
		{
			return *Sections.Find(SectionKey);
		}
		return nullptr;
	}

	void FRealtimeMeshSectionGroup::Initialize(FRealtimeMeshProxyCommandBatch& Commands)
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources->GetGuard());
		Bounds.Reset();

		if (Commands)
		{
			InitializeProxy(Commands);
		}
	}

	TFuture<ERealtimeMeshProxyUpdateStatus> FRealtimeMeshSectionGroup::Reset()
	{
		FRealtimeMeshProxyCommandBatch Commands(SharedResources->GetOwner());
		Reset(Commands);
		return Commands.Commit();
	}

	void FRealtimeMeshSectionGroup::Reset(FRealtimeMeshProxyCommandBatch& Commands)
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources->GetGuard());

		const auto StreamsToRemove = GetStreamKeys();
		const auto SectionsToRemove = GetSectionKeys();

		Streams.Empty();
		Sections.Empty();
		Bounds.Reset();

		if (Commands)
		{
			Commands.AddSectionGroupTask(Key, [](FRealtimeMeshSectionGroupProxy& Proxy)
			{
				Proxy.Reset();
			});

			InitializeProxy(Commands);
		}

		SharedResources->BroadcastSectionGroupBoundsChanged(Key);
		SharedResources->BroadcastSectionGroupInUseRangeChanged(Key);
		SharedResources->BroadcastSectionChanged(SectionsToRemove, ERealtimeMeshChangeType::Removed);
		SharedResources->BroadcastStreamChanged(Key, StreamsToRemove, ERealtimeMeshChangeType::Removed);
	}

	void FRealtimeMeshSectionGroup::SetOverrideBounds(const FBoxSphereBounds3f& InBounds)
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources->GetGuard());
		Bounds.SetUserSetBounds(InBounds);
		SharedResources->BroadcastSectionGroupBoundsChanged(Key);
	}

	void FRealtimeMeshSectionGroup::ClearOverrideBounds()
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources->GetGuard());
		Bounds.ClearUserSetBounds();
		SharedResources->BroadcastSectionGroupBoundsChanged(Key);
	}

	TFuture<ERealtimeMeshProxyUpdateStatus> FRealtimeMeshSectionGroup::CreateOrUpdateStream(FRealtimeMeshStream&& Stream)
	{
		FRealtimeMeshProxyCommandBatch Commands(SharedResources->GetOwner());
		CreateOrUpdateStream(Commands, MoveTemp(Stream));
		return Commands.Commit();
	}

	void FRealtimeMeshSectionGroup::CreateOrUpdateStream(FRealtimeMeshProxyCommandBatch& Commands, FRealtimeMeshStream&& Stream)
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources->GetGuard());

		const auto StreamKey = Stream.GetStreamKey();
		bool bAlreadyExisted = false;

		// Make sure we have the stream registered
		Streams.FindOrAdd(StreamKey, &bAlreadyExisted);

		// Create the update data for the GPU
		if (Commands && SharedResources->WantsStreamOnGPU(StreamKey))
		{
			const auto UpdateData = MakeShared<FRealtimeMeshSectionGroupStreamUpdateData>(MoveTemp(Stream));
			UpdateData->ConfigureBuffer(EBufferUsageFlags::Static, true);

			Commands.AddSectionGroupTask(Key, [UpdateData = UpdateData](FRealtimeMeshSectionGroupProxy& Proxy)
			{
				Proxy.CreateOrUpdateStream(UpdateData);
			}, ShouldRecreateProxyOnStreamChange());
		}

		SharedResources->BroadcastStreamChanged(Key, StreamKey, bAlreadyExisted ? ERealtimeMeshChangeType::Updated : ERealtimeMeshChangeType::Added);
	}

	TFuture<ERealtimeMeshProxyUpdateStatus> FRealtimeMeshSectionGroup::RemoveStream(const FRealtimeMeshStreamKey& StreamKey)
	{
		FRealtimeMeshProxyCommandBatch Commands(SharedResources->GetOwner());
		RemoveStream(Commands, StreamKey);
		return Commands.Commit();
	}

	void FRealtimeMeshSectionGroup::RemoveStream(FRealtimeMeshProxyCommandBatch& Commands, const FRealtimeMeshStreamKey& StreamKey)
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources->GetGuard());

		if (Streams.Remove(StreamKey))
		{
			if (Commands && SharedResources->WantsStreamOnGPU(StreamKey))
			{
				Commands.AddSectionGroupTask(Key, [StreamKey](FRealtimeMeshSectionGroupProxy& Proxy)
				{
					Proxy.RemoveStream(StreamKey);
				}, ShouldRecreateProxyOnStreamChange());
			}
		}

		SharedResources->BroadcastStreamChanged(Key, StreamKey, ERealtimeMeshChangeType::Removed);
	}

	TFuture<ERealtimeMeshProxyUpdateStatus> FRealtimeMeshSectionGroup::SetAllStreams(const FRealtimeMeshStreamSet& InStreams)
	{
		FRealtimeMeshProxyCommandBatch Commands(SharedResources->GetOwner());
		FRealtimeMeshStreamSet CopySet(InStreams);
		SetAllStreams(Commands, MoveTemp(CopySet));
		return Commands.Commit();
	}

	void FRealtimeMeshSectionGroup::SetAllStreams(FRealtimeMeshProxyCommandBatch& Commands, const FRealtimeMeshStreamSet& InStreams)
	{
		FRealtimeMeshStreamSet CopySet(InStreams);
		SetAllStreams(Commands, MoveTemp(CopySet));
	}

	TFuture<ERealtimeMeshProxyUpdateStatus> FRealtimeMeshSectionGroup::SetAllStreams(FRealtimeMeshStreamSet&& InStreams)
	{
		FRealtimeMeshProxyCommandBatch Commands(SharedResources->GetOwner());
		SetAllStreams(Commands, InStreams);
		return Commands.Commit();
	}

	void FRealtimeMeshSectionGroup::SetAllStreams(FRealtimeMeshProxyCommandBatch& Commands, FRealtimeMeshStreamSet&& InStreams)
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources->GetGuard());

		TSet<FRealtimeMeshStreamKey> ExistingStreamKeys = Streams;

		// Remove all old streams
		for (const FRealtimeMeshStreamKey& StreamKey : ExistingStreamKeys)
		{
			if (!InStreams.Contains(StreamKey))
			{
				RemoveStream(Commands, StreamKey);
			}
		}

		// Create/Update streams
		InStreams.ForEach([&](FRealtimeMeshStream& Stream)
		{
			CreateOrUpdateStream(Commands, MoveTemp(Stream));			
		});
	}

	TFuture<ERealtimeMeshProxyUpdateStatus> FRealtimeMeshSectionGroup::CreateOrUpdateSection(const FRealtimeMeshSectionKey& SectionKey, const FRealtimeMeshSectionConfig& InConfig,
	                                                                                         const FRealtimeMeshStreamRange& InStreamRange)
	{
		FRealtimeMeshProxyCommandBatch Commands(SharedResources->GetOwner());
		CreateOrUpdateSection(Commands, SectionKey, InConfig, InStreamRange);
		return Commands.Commit();
	}

	void FRealtimeMeshSectionGroup::CreateOrUpdateSection(FRealtimeMeshProxyCommandBatch& Commands, const FRealtimeMeshSectionKey& SectionKey,
	                                                      const FRealtimeMeshSectionConfig& InConfig, const FRealtimeMeshStreamRange& InStreamRange)
	{
		check(SectionKey.IsPartOf(Key));

		FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources->GetGuard());

		const bool bExisted = Sections.Contains(SectionKey);

		if (!bExisted)
		{
			const auto Section = SharedResources->CreateSection(SectionKey);
			Sections.Add(Section);

			Commands.AddSectionGroupTask(Key, [SectionKey](FRealtimeMeshSectionGroupProxy& Proxy)
			{
				Proxy.CreateSectionIfNotExists(SectionKey);
			});
		}

		const auto& Section = *Sections.Find(SectionKey);
		Section->Initialize(Commands, InConfig, InStreamRange);

		SharedResources->BroadcastSectionChanged(SectionKey, bExisted ? ERealtimeMeshChangeType::Updated : ERealtimeMeshChangeType::Added);
	}

	TFuture<ERealtimeMeshProxyUpdateStatus> FRealtimeMeshSectionGroup::RemoveSection(const FRealtimeMeshSectionKey& SectionKey)
	{
		FRealtimeMeshProxyCommandBatch Commands(SharedResources->GetOwner());
		RemoveSection(Commands, SectionKey);
		return Commands.Commit();
	}

	void FRealtimeMeshSectionGroup::RemoveSection(FRealtimeMeshProxyCommandBatch& Commands, const FRealtimeMeshSectionKey& SectionKey)
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources->GetGuard());

		if (Sections.Remove(SectionKey) && Commands)
		{
			Commands.AddSectionGroupTask(Key, [SectionKey](FRealtimeMeshSectionGroupProxy& Proxy)
			{
				Proxy.RemoveSection(SectionKey);
			}, ShouldRecreateProxyOnStreamChange());
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

		Ar << Bounds;
		if (Ar.CustomVer(FRealtimeMeshVersion::GUID) < FRealtimeMeshVersion::DataRestructure)
		{
			FRealtimeMeshStreamRange OldRange;
			Ar << OldRange;
		}
		return true;
	}

	void FRealtimeMeshSectionGroup::InitializeProxy(FRealtimeMeshProxyCommandBatch& Commands)
	{
		FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources->GetGuard());

		// We only send sections here, we rely on the derived to setup the streams
		for (const auto& Section : Sections)
		{
			Commands.AddSectionGroupTask(Key, [SectionKey = Section->GetKey()](FRealtimeMeshSectionGroupProxy& Proxy)
			{
				Proxy.CreateSectionIfNotExists(SectionKey);
			}, ShouldRecreateProxyOnStreamChange());

			Section->InitializeProxy(Commands);
		}
	}

	void FRealtimeMeshSectionGroup::InvalidateBounds() const
	{
		Bounds.ClearCachedValue();
		SharedResources->BroadcastSectionGroupBoundsChanged(Key);
	}

	/*void FRealtimeMeshSectionGroup::ApplyStateUpdate(FRealtimeMeshProxyCommandBatch& Commands, FRealtimeMeshSectionGroupUpdateContext& Update)
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources->GetGuard());
		
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
		FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources->GetGuard());

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
			FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources->GetGuard());
			Bounds.ClearCachedValue();
			SharedResources->BroadcastSectionGroupBoundsChanged(Key);
		}
	}

	void FRealtimeMeshSectionGroup::HandleSectionBoundsChanged(const FRealtimeMeshSectionKey& RealtimeMeshSectionKey)
	{
		if (RealtimeMeshSectionKey.IsPartOf(Key))
		{
			FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources->GetGuard());
			Bounds.ClearCachedValue();
			SharedResources->BroadcastSectionGroupBoundsChanged(Key);
		}
	}

	/*void FRealtimeMeshSectionGroup::HandleSectionStreamRangeChanged(const FRealtimeMeshSectionKey& RealtimeMeshSectionKey)
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources->GetGuard());

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

	bool FRealtimeMeshSectionGroup::ShouldRecreateProxyOnStreamChange() const
	{
		return true;
	}
}

#undef LOCTEXT_NAMESPACE
