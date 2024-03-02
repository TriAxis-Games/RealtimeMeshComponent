// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#include "Data/RealtimeMeshLOD.h"
#include "Data/RealtimeMeshShared.h"
#include "Data/RealtimeMeshSectionGroup.h"
#include "RenderProxy/RealtimeMeshLODProxy.h"
#include "RenderProxy/RealtimeMeshProxyCommandBatch.h"
#if RMC_ENGINE_ABOVE_5_2
#include "Logging/MessageLog.h"
#endif

#define LOCTEXT_NAMESPACE "RealtimeMesh"

namespace RealtimeMesh
{
	FRealtimeMeshLODData::FRealtimeMeshLODData(const FRealtimeMeshSharedResourcesRef& InSharedResources, const FRealtimeMeshLODKey& InKey)
		: SharedResources(InSharedResources)
		  , Key(InKey)
	{
		SharedResources->OnSectionGroupBoundsChanged().AddRaw(this, &FRealtimeMeshLODData::HandleSectionGroupBoundsChanged);
	}

	FRealtimeMeshLODData::~FRealtimeMeshLODData()
	{
		SharedResources->OnSectionGroupBoundsChanged().RemoveAll(this);
	}

	bool FRealtimeMeshLODData::HasSectionGroups() const
	{
		FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources->GetGuard());
		return SectionGroups.Num() > 0;
	}

	FRealtimeMeshSectionGroupPtr FRealtimeMeshLODData::GetSectionGroup(const FRealtimeMeshSectionGroupKey& SectionGroupKey) const
	{
		FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources->GetGuard());
		return SectionGroups.Contains(SectionGroupKey) ? *SectionGroups.Find(SectionGroupKey) : FRealtimeMeshSectionGroupPtr();
	}

	FBoxSphereBounds3f FRealtimeMeshLODData::GetLocalBounds() const
	{
		FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources->GetGuard());
		return Bounds.GetBounds([&]() { return CalculateBounds(); });
	}

	void FRealtimeMeshLODData::Initialize(FRealtimeMeshProxyCommandBatch& Commands, const FRealtimeMeshLODConfig& InConfig)
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources->GetGuard());
		Config = InConfig;
		Bounds.Reset();

		if (Commands)
		{
			InitializeProxy(Commands);
		}
	}

	TFuture<ERealtimeMeshProxyUpdateStatus> FRealtimeMeshLODData::Reset()
	{
		FRealtimeMeshProxyCommandBatch Commands(SharedResources->GetOwner());
		Reset(Commands);
		return Commands.Commit();
	}

	void FRealtimeMeshLODData::Reset(FRealtimeMeshProxyCommandBatch& Commands)
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources->GetGuard());

		const auto SectionGroupsToRemove = GetSectionGroupKeys();

		Config = FRealtimeMeshLODConfig();
		SectionGroups.Empty();
		Bounds.Reset();

		if (Commands)
		{
			InitializeProxy(Commands);
		}

		SharedResources->BroadcastLODConfigChanged(Key);
		SharedResources->BroadcastLODBoundsChanged(Key);
		SharedResources->BroadcastSectionGroupChanged(SectionGroupsToRemove, ERealtimeMeshChangeType::Removed);
	}

	TFuture<ERealtimeMeshProxyUpdateStatus> FRealtimeMeshLODData::UpdateConfig(const FRealtimeMeshLODConfig& InConfig)
	{
		FRealtimeMeshProxyCommandBatch Commands(SharedResources->GetOwner());
		UpdateConfig(Commands, InConfig);
		return Commands.Commit();
	}

	void FRealtimeMeshLODData::UpdateConfig(FRealtimeMeshProxyCommandBatch& Commands, const FRealtimeMeshLODConfig& InConfig)
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources->GetGuard());
		Config = InConfig;

		if (Commands)
		{
			Commands.AddLODTask(Key, [Config = Config](FRealtimeMeshLODProxy& Proxy)
			{
				Proxy.UpdateConfig(Config);
			}, ShouldRecreateProxyOnChange());
		}

		SharedResources->BroadcastLODConfigChanged(Key);
	}

	TFuture<ERealtimeMeshProxyUpdateStatus> FRealtimeMeshLODData::CreateOrUpdateSectionGroup(const FRealtimeMeshSectionGroupKey& SectionGroupKey)
	{
		FRealtimeMeshProxyCommandBatch Commands(SharedResources->GetOwner());
		CreateOrUpdateSectionGroup(Commands, SectionGroupKey);
		return Commands.Commit();
	}

	void FRealtimeMeshLODData::CreateOrUpdateSectionGroup(FRealtimeMeshProxyCommandBatch& Commands, const FRealtimeMeshSectionGroupKey& SectionGroupKey)
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources->GetGuard());

		if (!SectionGroups.Contains(SectionGroupKey))
		{
			SectionGroups.Add(SharedResources->CreateSectionGroup(SectionGroupKey));

			if (Commands)
			{
				Commands.AddLODTask(Key, [SectionGroupKey](FRealtimeMeshLODProxy& Proxy)
				{
					Proxy.CreateSectionGroupIfNotExists(SectionGroupKey);
				});

				(*SectionGroups.Find(SectionGroupKey))->InitializeProxy(Commands);
			}
		}
		else
		{
			const auto& SectionGroup = *SectionGroups.Find(SectionGroupKey);
			SectionGroup->Reset(Commands);
		}

		SharedResources->BroadcastSectionGroupChanged(SectionGroupKey, ERealtimeMeshChangeType::Added);
		SharedResources->BroadcastSectionGroupBoundsChanged(SectionGroupKey);
	}

	TFuture<ERealtimeMeshProxyUpdateStatus> FRealtimeMeshLODData::RemoveSectionGroup(const FRealtimeMeshSectionGroupKey& SectionGroupKey)
	{
		FRealtimeMeshProxyCommandBatch Commands(SharedResources->GetOwner());
		RemoveSectionGroup(Commands, SectionGroupKey);
		return Commands.Commit();
	}

	void FRealtimeMeshLODData::RemoveSectionGroup(FRealtimeMeshProxyCommandBatch& Commands, const FRealtimeMeshSectionGroupKey& SectionGroupKey)
	{
		FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources->GetGuard());

		if (SectionGroups.Remove(SectionGroupKey))
		{
			if (Commands)
			{
				Commands.AddLODTask(Key, [SectionGroupKey](FRealtimeMeshLODProxy& Proxy)
				{
					Proxy.RemoveSectionGroup(SectionGroupKey);
				}, ShouldRecreateProxyOnChange());
			}
		}

		SharedResources->BroadcastSectionGroupChanged(SectionGroupKey, ERealtimeMeshChangeType::Removed);
	}


	bool FRealtimeMeshLODData::Serialize(FArchive& Ar)
	{
		int32 NumSectionGroups = SectionGroups.Num();
		Ar << NumSectionGroups;

		if (Ar.CustomVer(RealtimeMesh::FRealtimeMeshVersion::GUID) < FRealtimeMeshVersion::DataRestructure)
		{
			SectionGroups.Empty();
			for (int32 Index = 0; Index < NumSectionGroups; Index++)
			{
				int32 SectionGroupIndex;
				Ar << SectionGroupIndex;

				FRealtimeMeshSectionGroupKey SectionGroupKey = FRealtimeMeshSectionGroupKey::Create(Key, SectionGroupIndex);
				auto SectionGroup = SharedResources->CreateSectionGroup(SectionGroupKey);
				SectionGroup->Serialize(Ar);
				SectionGroups.Add(SectionGroup);
			}
		}
		else if (Ar.IsLoading())
		{
			SectionGroups.Empty();
			for (int32 Index = 0; Index < NumSectionGroups; Index++)
			{
				FName SectionGroupName;
				Ar << SectionGroupName;

				FRealtimeMeshSectionGroupKey SectionGroupKey = FRealtimeMeshSectionGroupKey::Create(Key, SectionGroupName);
				auto SectionGroup = SharedResources->CreateSectionGroup(SectionGroupKey);
				SectionGroup->Serialize(Ar);
				SectionGroups.Add(SectionGroup);
			}
		}
		else
		{
			for (const auto& SectionGroup : SectionGroups)
			{
				FName SectionGroupName = SectionGroup->GetKey().Name();
				Ar << SectionGroupName;
				SectionGroup->Serialize(Ar);
			}
		}

		Ar << Config;
		Ar << Bounds;
		return true;
	}

	void FRealtimeMeshLODData::InitializeProxy(FRealtimeMeshProxyCommandBatch& Commands)
	{
		FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources->GetGuard());

		Commands.AddLODTask(Key, [Config = Config](FRealtimeMeshLODProxy& Proxy)
		{
			Proxy.UpdateConfig(Config);
		}, ShouldRecreateProxyOnChange());
		
		for (const auto& SectionGroup : SectionGroups)
		{
			Commands.AddLODTask(Key, [SectionGroupKey = SectionGroup->GetKey()](FRealtimeMeshLODProxy& Proxy)
			{
				Proxy.CreateSectionGroupIfNotExists(SectionGroupKey);
			}, ShouldRecreateProxyOnChange());

			SectionGroup->InitializeProxy(Commands);
		}
	}


	TSet<FRealtimeMeshSectionGroupKey> FRealtimeMeshLODData::GetSectionGroupKeys() const
	{
		FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources->GetGuard());
		TSet<FRealtimeMeshSectionGroupKey> SectionGroupKeys;
		for (const auto& SectionGroup : SectionGroups)
		{
			SectionGroupKeys.Add(SectionGroup->GetKey());
		}
		return SectionGroupKeys;
	}


	FBoxSphereBounds3f FRealtimeMeshLODData::CalculateBounds() const
	{
		FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources->GetGuard());
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

		return NewBounds.IsSet() ? *NewBounds : FBoxSphereBounds3f(FSphere3f(FVector3f::ZeroVector, 1.0f));
	}

	void FRealtimeMeshLODData::HandleSectionGroupBoundsChanged(const FRealtimeMeshSectionGroupKey& RealtimeMeshSectionGroupKey)
	{
		if (RealtimeMeshSectionGroupKey.IsPartOf(Key))
		{
			FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources->GetGuard());
			Bounds.ClearCachedValue();
			SharedResources->BroadcastLODBoundsChanged(Key);
		}
	}

}

#undef LOCTEXT_NAMESPACE
