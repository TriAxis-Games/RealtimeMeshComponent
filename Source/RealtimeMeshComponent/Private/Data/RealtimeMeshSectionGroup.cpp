// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#include "Data/RealtimeMeshSectionGroup.h"
#include "Data/RealtimeMeshData.h"
#include "Data/RealtimeMeshSection.h"
#include "RenderProxy/RealtimeMeshLODProxy.h"
#include "RenderProxy/RealtimeMeshSectionGroupProxy.h"
#include "RenderProxy/RealtimeMeshProxy.h"

#define LOCTEXT_NAMESPACE "RealtimeMesh"

namespace RealtimeMesh
{
	FRealtimeMeshSectionGroup::FRealtimeMeshSectionGroup(const FRealtimeMeshClassFactoryRef& InClassFactory,
		const FRealtimeMeshRef& InMesh, FRealtimeMeshSectionGroupKey InID)
		: ClassFactory(InClassFactory)
		, MeshWeak(InMesh)
		, Key(InID)
		, LocalBounds(FSphere3f(FVector3f::ZeroVector, 1.0f))
		, InUseRange(FRealtimeMeshStreamRange::Empty)
	{
	}

	FName FRealtimeMeshSectionGroup::GetMeshName() const
	{
		if (const auto Mesh = MeshWeak.Pin())
		{
			return Mesh->GetMeshName();
		}
		return NAME_None;
	}

	FRealtimeMeshStreamRange FRealtimeMeshSectionGroup::GetInUseRange() const
	{
		FReadScopeLock ScopeLock(Lock);
		return InUseRange;
	}

	FBoxSphereBounds3f FRealtimeMeshSectionGroup::GetLocalBounds() const
	{
		FReadScopeLock ScopeLock(Lock);
		return LocalBounds;
	}

	bool FRealtimeMeshSectionGroup::HasSections() const
	{
		FReadScopeLock ScopeLock(Lock);
		return Sections.Num() > 0;
	}

	int32 FRealtimeMeshSectionGroup::NumSections() const
	{
		FReadScopeLock ScopeLock(Lock);
		return Sections.Num();
	}

	FRealtimeMeshSectionDataPtr FRealtimeMeshSectionGroup::GetSection(FRealtimeMeshSectionKey SectionKey) const
	{
		FReadScopeLock ScopeLock(Lock);
		if (SectionKey.IsPartOf(Key) && Sections.IsValidIndex(FRealtimeMeshKeyHelpers::GetSectionIndex(SectionKey)))
		{
			return Sections[FRealtimeMeshKeyHelpers::GetSectionIndex(SectionKey)];
		}
		return nullptr;
	}


	void FRealtimeMeshSectionGroup::CreateOrUpdateStream(FRealtimeMeshStreamKey StreamKey, const FRealtimeMeshSectionGroupStreamUpdateDataRef& InStream)
	{
		DoOnValidProxy([InStream = InStream](const FRealtimeMeshSectionGroupProxyRef& Proxy)
		{
			Proxy->CreateOrUpdateStreams({InStream});
		});

		// Broadcast stream updated event
		BroadcastStreamsChanged({ StreamKey }, {});
		
		MarkRenderStateDirty(true);
	}

	void FRealtimeMeshSectionGroup::ClearStream(FRealtimeMeshStreamKey StreamKey)
	{
		DoOnValidProxy([StreamKey](const FRealtimeMeshSectionGroupProxyRef& Proxy)
		{
			Proxy->RemoveStream({StreamKey});
		});

		// Broadcast stream updated event
		BroadcastStreamsChanged({ StreamKey }, {});
		
		MarkRenderStateDirty(true);
	}

	void FRealtimeMeshSectionGroup::RemoveStream(FRealtimeMeshStreamKey StreamKey)
	{
		DoOnValidProxy([StreamKey](const FRealtimeMeshSectionGroupProxyRef& Proxy)
		{
			Proxy->RemoveStream({StreamKey});
		});
			
		// Broadcast stream removed event
		BroadcastStreamsChanged({}, { StreamKey });
		
		MarkRenderStateDirty(true);
	}

	void FRealtimeMeshSectionGroup::SetAllStreams(const TArray<FRealtimeMeshStreamKey>& UpdatedStreamKeys, const TArray<FRealtimeMeshStreamKey>& RemovedStreamKeys,
			TArray<FRealtimeMeshSectionGroupStreamUpdateDataRef>&& StreamUpdateData)
	{
		DoOnValidProxy([RemovedStreams = RemovedStreamKeys, NewStreams = MoveTemp(StreamUpdateData)](const FRealtimeMeshSectionGroupProxyRef& Proxy)
		{
			Proxy->RemoveStream(RemovedStreams);
			Proxy->CreateOrUpdateStreams(NewStreams);
		});
		
		// Broadcast all stream added/removed events
		BroadcastStreamsChanged(UpdatedStreamKeys, RemovedStreamKeys);
		
		MarkRenderStateDirty(true);
	}

	FRealtimeMeshSectionKey FRealtimeMeshSectionGroup::CreateSection(const FRealtimeMeshSectionConfig& InConfig, const FRealtimeMeshStreamRange& InStreamRange)
	{
		FRWScopeLockEx ScopeLock(Lock, SLT_Write);
		const auto Allocation = Sections.AddUninitialized();
		check(Allocation.Index <= UINT16_MAX);
		FRealtimeMeshSectionKey SectionKey = FRealtimeMeshSectionKey(Key, Allocation.Index);
		new(Allocation) FRealtimeMeshSectionDataRef(ClassFactory->CreateSection(MeshWeak.Pin().ToSharedRef(), SectionKey, InConfig, InStreamRange));
		const auto& Section = Sections[FRealtimeMeshKeyHelpers::GetSectionIndex(SectionKey)];
		ScopeLock.Release();

		Section->OnBoundsUpdated().AddThreadSafeSP(this, &FRealtimeMeshSectionGroup::HandleSectionBoundsChanged);
		Section->OnStreamRangeUpdated().AddThreadSafeSP(this, &FRealtimeMeshSectionGroup::HandleStreamRangeChanged);

		Section->OnStreamsChanged({}, {});
		
		FRealtimeMeshSectionProxyInitializationParametersRef InitParams = Section->GetInitializationParams();

		DoOnValidProxy([SectionKey, InitParams = InitParams](const FRealtimeMeshSectionGroupProxyRef& SectionGroupProxy) mutable
		{
			SectionGroupProxy->CreateSection(SectionKey, InitParams);
		});

		BroadcastSectionsChanged({SectionKey}, {});

		MarkRenderStateDirty(true);

		return SectionKey;
	}

	void FRealtimeMeshSectionGroup::RemoveSection(FRealtimeMeshSectionKey SectionKey)
	{
		FRWScopeLockEx ScopeLock(Lock, SLT_Write);
		if (SectionKey.IsPartOf(Key) && Sections.IsValidIndex(FRealtimeMeshKeyHelpers::GetSectionIndex(SectionKey)))
		{
			Sections.RemoveAt(FRealtimeMeshKeyHelpers::GetSectionIndex(SectionKey));
			ScopeLock.Release();

			DoOnValidProxy([SectionKey](const FRealtimeMeshSectionGroupProxyRef& SectionGroupProxy) mutable
			{
				SectionGroupProxy->RemoveSection(SectionKey);
			});

			BroadcastSectionsChanged({}, {SectionKey});
		
			MarkRenderStateDirty(true);
		}
		else
		{
			FMessageLog("RealtimeMesh").Error(
				FText::Format(LOCTEXT("RemoveSectionInvalid", "Attempted to remove invalid section {0} in Mesh:{1}"),
				              FText::FromString(SectionKey.ToString()), FText::FromName(GetParentName())));
		}
	}

	void FRealtimeMeshSectionGroup::RemoveAllSections()
	{
		FRWScopeLockEx ScopeLock(Lock, SLT_Write);

		TArray<FRealtimeMeshSectionKey> RemovedSections;
		for (const auto& Section : Sections)
		{
			RemovedSections.Add(Section->GetKey());
		}

		DoOnValidProxy([](const FRealtimeMeshSectionGroupProxyRef& Proxy)
		{
			Proxy->RemoveAllSections();
		});

		// Broadcast all stream removed events
		BroadcastSectionsChanged({}, RemovedSections);
		
		MarkRenderStateDirty(true);
	}

	void FRealtimeMeshSectionGroup::MarkRenderStateDirty(bool bShouldRecreateProxies)
	{
		if (const auto Mesh = MeshWeak.Pin())
		{
			Mesh->MarkRenderStateDirty(bShouldRecreateProxies);
		}
	}


	FRealtimeMeshSectionGroupProxyInitializationParametersRef FRealtimeMeshSectionGroup::GetInitializationParams() const
	{
		FReadScopeLock ScopeLock(Lock);

		auto InitParams = MakeShared<FRealtimeMeshSectionGroupProxyInitializationParameters>();

		// Get the init params for all existing sections
		InitParams->Sections.Reserve(Sections.Num());
		for (TSparseArray<FRealtimeMeshSectionDataRef>::TConstIterator It(Sections); It; ++It)
		{
			InitParams->Sections.Insert(It.GetIndex(), (*It)->GetInitializationParams());
		}

		return InitParams;
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
				int32 SectionIndex;
				Ar << SectionIndex;

				Sections.Insert(SectionIndex, ClassFactory->CreateSection(MeshWeak.Pin().ToSharedRef(),
					FRealtimeMeshSectionKey(Key, SectionIndex), FRealtimeMeshSectionConfig(), FRealtimeMeshStreamRange()));

				Sections[SectionIndex]->Serialize(Ar);
				Sections[SectionIndex]->OnBoundsUpdated().AddThreadSafeSP(this, &FRealtimeMeshSectionGroup::HandleSectionBoundsChanged);
				Sections[SectionIndex]->OnStreamRangeUpdated().AddThreadSafeSP(this, &FRealtimeMeshSectionGroup::HandleStreamRangeChanged);
			}
		}
		else
		{
			for (TSparseArray<FRealtimeMeshSectionDataRef>::TConstIterator It(Sections); It; ++It)
			{
				int32 Index = It.GetIndex();
				Ar << Index;
				(*It)->Serialize(Ar);
			}
		}
		
		Ar << LocalBounds;
		Ar << InUseRange;
		return true;
	}

	FName FRealtimeMeshSectionGroup::GetParentName() const
	{
		if (const FRealtimeMeshPtr Parent = MeshWeak.Pin())
		{
			return Parent->GetMeshName();
		}
		return "UnknownMesh";
	}

	void FRealtimeMeshSectionGroup::DoOnValidProxy(TUniqueFunction<void(const FRealtimeMeshSectionGroupProxyRef&)>&& Function) const
	{
		if (const FRealtimeMeshPtr Parent = MeshWeak.Pin())
		{
			Parent->DoOnRenderProxy([Key = Key, Function = MoveTemp(Function)](const FRealtimeMeshProxyRef& Proxy)
			{
				if (const FRealtimeMeshLODProxyPtr LODProxy = Proxy->GetLOD(Key.GetLODKey()))
				{
					if (const FRealtimeMeshSectionGroupProxyPtr SectionGroupProxy = LODProxy->GetSectionGroup(Key))
					{
						Function(SectionGroupProxy.ToSharedRef());
					}
				}
			});
		}
	}

	void FRealtimeMeshSectionGroup::UpdateBounds()
	{
		FRWScopeLockEx ScopeLock(Lock, SLT_Write);

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

		// Update with new bounds or a unit sphere if we don't have bounds
		LocalBounds = NewBounds.IsSet() ? *NewBounds : FBoxSphereBounds3f(FSphere3f(FVector3f::ZeroVector, 1.0f));

		ScopeLock.Release();

		BoundsUpdatedEvent.Broadcast(this->AsShared());
	}

	void FRealtimeMeshSectionGroup::UpdateInUseStreamRange()
	{
		FRWScopeLockEx ScopeLock(Lock, SLT_Write);

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
		InUseRange = NewRange.IsSet() ? *NewRange : FRealtimeMeshStreamRange::Empty;

		ScopeLock.Release();

		InUseStreamRangeUpdatedEvent.Broadcast(this->AsShared());
	}

	void FRealtimeMeshSectionGroup::HandleStreamRangeChanged(const FRealtimeMeshSectionDataRef& InSection)
	{
		UpdateInUseStreamRange();
	}

	void FRealtimeMeshSectionGroup::HandleSectionBoundsChanged(const FRealtimeMeshSectionDataRef& InSection)
	{
		UpdateBounds();
	}


	void FRealtimeMeshSectionGroup::BroadcastSectionsChanged(const TArray<FRealtimeMeshSectionKey>& AddedOrUpdatedSections,	const TArray<FRealtimeMeshSectionKey>& RemovedSections)
	{
		SectionsUpdatedEvent.Broadcast(this->AsShared(), AddedOrUpdatedSections, RemovedSections);
	}
	
	void FRealtimeMeshSectionGroup::BroadcastStreamsChanged(const TArray<FRealtimeMeshStreamKey>& AddedOrUpdatedStreams, const TArray<FRealtimeMeshStreamKey>& RemovedStreams)
	{		
		for (const auto& Section : Sections)
		{
			Section->OnStreamsChanged(AddedOrUpdatedStreams, RemovedStreams);
		}

		StreamsUpdatedEvent.Broadcast(this->AsShared(), AddedOrUpdatedStreams, RemovedStreams);
	}
}

#undef LOCTEXT_NAMESPACE
