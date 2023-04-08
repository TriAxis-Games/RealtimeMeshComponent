// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#include "Data/RealtimeMeshLOD.h"
#include "Data/RealtimeMeshData.h"
#include "Data/RealtimeMeshSectionGroup.h"
#include "RenderProxy/RealtimeMeshLODProxy.h"
#include "RenderProxy/RealtimeMeshProxy.h"

#define LOCTEXT_NAMESPACE "RealtimeMesh"

namespace RealtimeMesh
{
	FRealtimeMeshLODData::FRealtimeMeshLODData(const FRealtimeMeshClassFactoryRef& InClassFactory, const FRealtimeMeshRef& InMesh,
		FRealtimeMeshLODKey InID, const FRealtimeMeshLODConfig& InConfig)
		: ClassFactory(InClassFactory)
		, MeshWeak(InMesh)
		, Key(InID)
		, Config(InConfig)
		, LocalBounds(FSphere3f(FVector3f::ZeroVector, 1.0f))
	{
		TypeName = "RealtimeMeshLOD-Base";
	}

	FName FRealtimeMeshLODData::GetMeshName() const
	{
		if (const auto Mesh = MeshWeak.Pin())
		{
			return Mesh->GetMeshName();
		}
		return NAME_None;
	}

	bool FRealtimeMeshLODData::HasSectionGroups() const
	{
		FReadScopeLock ScopeLock(Lock);
		return SectionGroups.Num() > 0;
	}

	FRealtimeMeshSectionGroupPtr FRealtimeMeshLODData::GetSectionGroup(FRealtimeMeshSectionGroupKey SectionGroupKey) const
	{
		FReadScopeLock ScopeLock(Lock);
		return SectionGroups.IsValidIndex(FRealtimeMeshKeyHelpers::GetSectionGroupIndex(SectionGroupKey))
			       ? SectionGroups[FRealtimeMeshKeyHelpers::GetSectionGroupIndex(SectionGroupKey)]
			       : FRealtimeMeshSectionGroupPtr();
	}

	FBoxSphereBounds3f FRealtimeMeshLODData::GetLocalBounds() const
	{
		FReadScopeLock ScopeLock(Lock);
		return LocalBounds;
	}

	void FRealtimeMeshLODData::UpdateConfig(const FRealtimeMeshLODConfig& InConfig)
	{
		Lock.WriteLock();
		Config = InConfig;
		Lock.WriteUnlock();

		DoOnValidProxy([InConfig](const FRealtimeMeshLODProxyRef& Proxy)
		{
			Proxy->UpdateConfig(InConfig);
		});

		ConfigUpdatedEvent.Broadcast(this->AsShared());

		MarkRenderStateDirty(true);
	}

	FRealtimeMeshSectionGroupKey FRealtimeMeshLODData::CreateSectionGroup()
	{
		FRWScopeLockEx ScopeLock(Lock, SLT_Write);
		const auto Allocation = SectionGroups.AddUninitialized();
		FRealtimeMeshSectionGroupKey SectionGroupKey = FRealtimeMeshSectionGroupKey(Key, Allocation.Index);
		new(Allocation) FRealtimeMeshSectionGroupRef(ClassFactory->CreateSectionGroup(MeshWeak.Pin().ToSharedRef(), SectionGroupKey));
		const auto& SectionGroup = SectionGroups[FRealtimeMeshKeyHelpers::GetSectionGroupIndex(SectionGroupKey)];
		ScopeLock.Release();

		SectionGroup->OnBoundsUpdated().AddThreadSafeSP(this, &FRealtimeMeshLODData::HandleSectionGroupBoundsChanged);
		FRealtimeMeshSectionGroupProxyInitializationParametersRef InitParams = SectionGroup->GetInitializationParams();

		DoOnValidProxy([SectionGroupKey, InitParams = InitParams](const FRealtimeMeshLODProxyRef& Proxy) mutable
		{
			Proxy->CreateSectionGroup(SectionGroupKey, InitParams);
		});

		SectionGroupAddedEvent.Broadcast(this->AsShared(), SectionGroupKey);

		MarkRenderStateDirty(true);
		return SectionGroupKey;
	}

	void FRealtimeMeshLODData::RemoveSectionGroup(FRealtimeMeshSectionGroupKey SectionGroupKey)
	{
		FRWScopeLockEx ScopeLock(Lock, SLT_Write);

		if (SectionGroupKey.IsPartOf(Key) && SectionGroups.IsValidIndex(FRealtimeMeshKeyHelpers::GetSectionGroupIndex(SectionGroupKey)))
		{
			SectionGroups.RemoveAt(FRealtimeMeshKeyHelpers::GetSectionGroupIndex(SectionGroupKey));
			ScopeLock.Release();

			DoOnValidProxy([SectionGroupKey](const FRealtimeMeshLODProxyRef& Proxy) mutable
			{
				Proxy->RemoveSectionGroup(SectionGroupKey);
			});

			SectionGroupRemovedEvent.Broadcast(this->AsShared(), SectionGroupKey);

			MarkRenderStateDirty(true);
		}
		else
		{
			FMessageLog("RealtimeMesh").Error(FText::Format(LOCTEXT("RemoveSectionGroup_InvalidKey", "Invalid section group key {0}"), FText::FromString(SectionGroupKey.ToString())));
		}
	}

	void FRealtimeMeshLODData::RemoveAllSectionGroups()
	{
		FRWScopeLockEx ScopeLock(Lock, SLT_Write);

		TArray<FRealtimeMeshSectionGroupKey> RemovedSectionGroups;
		for (TSparseArray<FRealtimeMeshSectionGroupRef>::TConstIterator It(SectionGroups); It; ++It)
		{
			RemovedSectionGroups.Add(FRealtimeMeshSectionGroupKey(Key, It.GetIndex()));
		}
		SectionGroups.Empty();

		ScopeLock.Release();

		DoOnValidProxy([](const FRealtimeMeshLODProxyRef& Proxy) mutable
		{
			Proxy->RemoveAllSectionGroups();
		});

		// Broadcast all stream removed events
		for (const auto SectionGroup : RemovedSectionGroups)
		{
			SectionGroupRemovedEvent.Broadcast(this->AsShared(), SectionGroup);
		}

		MarkRenderStateDirty(true);
	}

	void FRealtimeMeshLODData::MarkRenderStateDirty(bool bShouldRecreateProxies)
	{
		if (const auto Mesh = MeshWeak.Pin())
		{
			Mesh->MarkRenderStateDirty(bShouldRecreateProxies);
		}
	}

	FRealtimeMeshLODProxyInitializationParametersRef FRealtimeMeshLODData::GetInitializationParams() const
	{
		FReadScopeLock ScopeLock(Lock);

		auto InitParams = MakeShared<FRealtimeMeshLODProxyInitializationParameters>();
		InitParams->Config = Config;

		// Get the init params for all existing buffer sets;
		for (TSparseArray<FRealtimeMeshSectionGroupRef>::TConstIterator It(SectionGroups); It; ++It)
		{
			InitParams->SectionGroups.Insert(It.GetIndex(), (*It)->GetInitializationParams());
		}

		return InitParams;
	}

	bool FRealtimeMeshLODData::Serialize(FArchive& Ar)
	{
		int32 NumSectionGroups = SectionGroups.Num();
		Ar << NumSectionGroups;

		if (Ar.IsLoading())			
		{
			SectionGroups.Empty();
			for (int32 Index = 0; Index < NumSectionGroups; Index++)
			{
				int32 SectionGroupIndex;
				Ar << SectionGroupIndex;

				SectionGroups.Insert(SectionGroupIndex, ClassFactory->CreateSectionGroup(MeshWeak.Pin().ToSharedRef(),
					FRealtimeMeshSectionGroupKey(Key, SectionGroupIndex)));

				SectionGroups[SectionGroupIndex]->Serialize(Ar);
				SectionGroups[SectionGroupIndex]->OnBoundsUpdated().AddThreadSafeSP(this, &FRealtimeMeshLODData::HandleSectionGroupBoundsChanged);
			}			
		}
		else
		{
			for (TSparseArray<FRealtimeMeshSectionGroupRef>::TConstIterator It(SectionGroups); It; ++It)
			{
				int32 Index = It.GetIndex();
				Ar << Index;
				(*It)->Serialize(Ar);
			}
		}
		
		Ar << Config;
		Ar << LocalBounds;
		return true;
	}

	FName FRealtimeMeshLODData::GetParentName() const
	{
		if (const FRealtimeMeshPtr Parent = MeshWeak.Pin())
		{
			return Parent->GetMeshName();
		}
		return "UnknownMesh";
	}

	void FRealtimeMeshLODData::DoOnValidProxy(TUniqueFunction<void(const FRealtimeMeshLODProxyRef&)>&& Function) const
	{
		if (const FRealtimeMeshPtr Parent = MeshWeak.Pin())
		{
			Parent->DoOnRenderProxy([ID = Key, Function = MoveTemp(Function)](const FRealtimeMeshProxyRef& Proxy)
			{
				if (const FRealtimeMeshLODProxyPtr LODProxy = Proxy->GetLOD(ID))
				{
					Function(LODProxy.ToSharedRef());
				}
			});
		}
	}

	void FRealtimeMeshLODData::UpdateBounds()
	{
		FRWScopeLockEx ScopeLock(Lock, SLT_Write);

		TOptional<FBoxSphereBounds3f> NewBounds;
		for (const auto& SectionGroup : SectionGroups)
		{
			if (!NewBounds.IsSet())
			{
				NewBounds = SectionGroup->GetLocalBounds();
				continue;
			}
			NewBounds = *NewBounds + SectionGroup->GetLocalBounds();
		}

		// Update with new bounds or a unit sphere if we don't have bounds
		LocalBounds = NewBounds.IsSet() ? *NewBounds : FBoxSphereBounds3f(FSphere3f(FVector3f::ZeroVector, 1.0f));

		ScopeLock.Release();

		BoundsUpdatedEvent.Broadcast(this->AsShared());
	}

	void FRealtimeMeshLODData::HandleSectionGroupBoundsChanged(const FRealtimeMeshSectionGroupRef& InSectionGroup)
	{
		UpdateBounds();
	}
}

#undef LOCTEXT_NAMESPACE