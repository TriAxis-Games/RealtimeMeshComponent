// Copyright (c) 2015-2024 TriAxis Games, L.L.C. All Rights Reserved.

#include "Data/RealtimeMeshLOD.h"
#include "Data/RealtimeMeshShared.h"
#include "Data/RealtimeMeshSectionGroup.h"
#include "Core/RealtimeMeshLODConfig.h"
#include "Data/RealtimeMeshUpdateBuilder.h"
#include "RenderProxy/RealtimeMeshLODProxy.h"
#include "RenderProxy/RealtimeMeshProxyCommandBatch.h"
#if RMC_ENGINE_ABOVE_5_2
#include "Logging/MessageLog.h"
#endif

#define LOCTEXT_NAMESPACE "RealtimeMesh"

namespace RealtimeMesh
{
	FRealtimeMeshLOD::FRealtimeMeshLOD(const FRealtimeMeshSharedResourcesRef& InSharedResources, const FRealtimeMeshLODKey& InKey)
		: SharedResources(InSharedResources)
		  , Key(InKey)
	{
		SharedResources->OnSectionGroupBoundsChanged().AddRaw(this, &FRealtimeMeshLOD::HandleSectionGroupBoundsChanged);
	}

	FRealtimeMeshLOD::~FRealtimeMeshLOD()
	{
		SharedResources->OnSectionGroupBoundsChanged().RemoveAll(this);
	}

	bool FRealtimeMeshLOD::HasSectionGroups() const
	{
		FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources);
		return SectionGroups.Num() > 0;
	}

	FRealtimeMeshSectionGroupPtr FRealtimeMeshLOD::GetSectionGroup(const FRealtimeMeshSectionGroupKey& SectionGroupKey) const
	{
		FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources);
		return SectionGroups.Contains(SectionGroupKey) ? *SectionGroups.Find(SectionGroupKey) : FRealtimeMeshSectionGroupPtr();
	}

	FBoxSphereBounds3f FRealtimeMeshLOD::GetLocalBounds() const
	{
		FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources);
		return Bounds.GetBounds([&]() { return CalculateBounds(); });
	}

	void FRealtimeMeshLOD::Initialize(FRealtimeMeshProxyUpdateBuilder& ProxyBuilder, const FRealtimeMeshLODConfig& InConfig)
	{
		FRealtimeMeshScopeGuardWriteCheck LockCheck(SharedResources);
		Config = InConfig;
		Bounds.Reset();

		if (ProxyBuilder)
		{
			InitializeProxy(ProxyBuilder);
		}
	}

	TFuture<ERealtimeMeshProxyUpdateStatus> FRealtimeMeshLOD::Reset()
	{
		FRealtimeMeshUpdateContext UpdateContext(SharedResources);
		Reset(UpdateContext);
		return UpdateContext.Commit();
	}

	void FRealtimeMeshLOD::Reset(FRealtimeMeshProxyUpdateBuilder& ProxyBuilder)
	{
		FRealtimeMeshScopeGuardWriteCheck LockCheck(SharedResources);

		const auto SectionGroupsToRemove = GetSectionGroupKeys();

		Config = FRealtimeMeshLODConfig();
		SectionGroups.Empty();
		Bounds.Reset();

		if (ProxyBuilder)
		{
			InitializeProxy(ProxyBuilder);
		}

		SharedResources->BroadcastLODConfigChanged(Key);
		SharedResources->BroadcastLODBoundsChanged(Key);
		SharedResources->BroadcastSectionGroupChanged(SectionGroupsToRemove, ERealtimeMeshChangeType::Removed);
	}

	TFuture<ERealtimeMeshProxyUpdateStatus> FRealtimeMeshLOD::UpdateConfig(const FRealtimeMeshLODConfig& InConfig)
	{
		FRealtimeMeshUpdateContext UpdateContext(SharedResources);
		UpdateConfig(UpdateContext, InConfig);
		return UpdateContext.Commit();
	}

	void FRealtimeMeshLOD::UpdateConfig(FRealtimeMeshProxyUpdateBuilder& ProxyBuilder, const FRealtimeMeshLODConfig& InConfig)
	{
		FRealtimeMeshScopeGuardWriteCheck LockCheck(SharedResources);
		Config = InConfig;

		if (ProxyBuilder)
		{
			ProxyBuilder.AddLODTask(Key, [Config = Config](FRHICommandListBase& RHICmdList, FRealtimeMeshLODProxy& Proxy)
			{
				Proxy.UpdateConfig(Config);
			}, ShouldRecreateProxyOnChange());
		}

		SharedResources->BroadcastLODConfigChanged(Key);
	}

	TFuture<ERealtimeMeshProxyUpdateStatus> FRealtimeMeshLOD::CreateOrUpdateSectionGroup(const FRealtimeMeshSectionGroupKey& SectionGroupKey, const FRealtimeMeshSectionGroupConfig& InConfig)
	{
		FRealtimeMeshUpdateContext UpdateContext(SharedResources);
		CreateOrUpdateSectionGroup(UpdateContext, SectionGroupKey, InConfig);
		return UpdateContext.Commit();
	}

	void FRealtimeMeshLOD::CreateOrUpdateSectionGroup(FRealtimeMeshProxyUpdateBuilder& ProxyBuilder, const FRealtimeMeshSectionGroupKey& SectionGroupKey, const FRealtimeMeshSectionGroupConfig& InConfig)
	{
		FRealtimeMeshScopeGuardWriteCheck LockCheck(SharedResources);

		if (!SectionGroups.Contains(SectionGroupKey))
		{
			SectionGroups.Add(SharedResources->CreateSectionGroup(SectionGroupKey));

			if (ProxyBuilder)
			{
				ProxyBuilder.AddLODTask(Key, [SectionGroupKey](FRHICommandListBase& RHICmdList, FRealtimeMeshLODProxy& Proxy)
				{
					Proxy.CreateSectionGroupIfNotExists(SectionGroupKey);
				});
			}
		}

		const auto& SectionGroup = *SectionGroups.Find(SectionGroupKey);
		SectionGroup->Initialize(ProxyBuilder, InConfig);

		SharedResources->BroadcastSectionGroupChanged(SectionGroupKey, ERealtimeMeshChangeType::Added);
		SharedResources->BroadcastSectionGroupBoundsChanged(SectionGroupKey);
	}

	TFuture<ERealtimeMeshProxyUpdateStatus> FRealtimeMeshLOD::RemoveSectionGroup(const FRealtimeMeshSectionGroupKey& SectionGroupKey)
	{
		FRealtimeMeshUpdateContext UpdateContext(SharedResources);
		RemoveSectionGroup(UpdateContext, SectionGroupKey);
		return UpdateContext.Commit();
	}

	void FRealtimeMeshLOD::RemoveSectionGroup(FRealtimeMeshProxyUpdateBuilder& ProxyBuilder, const FRealtimeMeshSectionGroupKey& SectionGroupKey)
	{
		FRealtimeMeshScopeGuardWriteCheck LockCheck(SharedResources);

		if (SectionGroups.Remove(SectionGroupKey))
		{
			if (ProxyBuilder)
			{
				ProxyBuilder.AddLODTask(Key, [SectionGroupKey](FRHICommandListBase& RHICmdList, FRealtimeMeshLODProxy& Proxy)
				{
					Proxy.RemoveSectionGroup(SectionGroupKey);
				}, ShouldRecreateProxyOnChange());
			}
		}

		SharedResources->BroadcastSectionGroupChanged(SectionGroupKey, ERealtimeMeshChangeType::Removed);
	}


	bool FRealtimeMeshLOD::Serialize(FArchive& Ar)
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

	void FRealtimeMeshLOD::InitializeProxy(FRealtimeMeshProxyUpdateBuilder& ProxyBuilder)
	{
		if (ProxyBuilder)
		{
			ProxyBuilder.AddLODTask(Key, [Config = Config](FRHICommandListBase& RHICmdList, FRealtimeMeshLODProxy& Proxy)
			{
				Proxy.UpdateConfig(Config);
			}, ShouldRecreateProxyOnChange());
		
			for (const auto& SectionGroup : SectionGroups)
			{
				ProxyBuilder.AddLODTask(Key, [SectionGroupKey = SectionGroup->GetKey()](FRHICommandListBase& RHICmdList, FRealtimeMeshLODProxy& Proxy)
				{
					Proxy.CreateSectionGroupIfNotExists(SectionGroupKey);
				}, ShouldRecreateProxyOnChange());

				SectionGroup->InitializeProxy(ProxyBuilder);
			}
		}
	}


	TSet<FRealtimeMeshSectionGroupKey> FRealtimeMeshLOD::GetSectionGroupKeys() const
	{
		FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources);
		TSet<FRealtimeMeshSectionGroupKey> SectionGroupKeys;
		for (const auto& SectionGroup : SectionGroups)
		{
			SectionGroupKeys.Add(SectionGroup->GetKey());
		}
		return SectionGroupKeys;
	}


	FBoxSphereBounds3f FRealtimeMeshLOD::CalculateBounds() const
	{
		FRealtimeMeshScopeGuardRead ScopeGuard(SharedResources);
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

	void FRealtimeMeshLOD::HandleSectionGroupBoundsChanged(const FRealtimeMeshSectionGroupKey& RealtimeMeshSectionGroupKey)
	{
		if (RealtimeMeshSectionGroupKey.IsPartOf(Key))
		{
			FRealtimeMeshScopeGuardWrite ScopeGuard(SharedResources);
			Bounds.ClearCachedValue();
			SharedResources->BroadcastLODBoundsChanged(Key);
		}
	}

}

#undef LOCTEXT_NAMESPACE
