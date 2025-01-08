// Copyright (c) 2015-2025 TriAxis Games, L.L.C. All Rights Reserved.

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
	}

	bool FRealtimeMeshLOD::HasSectionGroups(const FRealtimeMeshLockContext& LockContext) const
	{
		return SectionGroups.Num() > 0;
	}

	FRealtimeMeshSectionGroupPtr FRealtimeMeshLOD::GetSectionGroup(const FRealtimeMeshLockContext& LockContext, const FRealtimeMeshSectionGroupKey& SectionGroupKey) const
	{
		return SectionGroups.Contains(SectionGroupKey) ? *SectionGroups.Find(SectionGroupKey) : FRealtimeMeshSectionGroupPtr();
	}

	TOptional<FBoxSphereBounds3f> FRealtimeMeshLOD::GetLocalBounds(const FRealtimeMeshLockContext& LockContext) const
	{
		return Bounds.Get();
	}

	void FRealtimeMeshLOD::Initialize(FRealtimeMeshUpdateContext& UpdateContext, const FRealtimeMeshLODConfig& InConfig)
	{
		Config = InConfig;
		Bounds.Reset();

		InitializeProxy(UpdateContext);
	}

	void FRealtimeMeshLOD::Reset(FRealtimeMeshUpdateContext& UpdateContext)
	{
		const auto SectionGroupsToRemove = GetSectionGroupKeys(UpdateContext);

		Config = FRealtimeMeshLODConfig();
		SectionGroups.Empty();
		Bounds.Reset();

		InitializeProxy(UpdateContext);

		UpdateContext.GetState().ConfigDirtyTree.Flag(Key);
		UpdateContext.GetState().BoundsDirtyTree.Flag(Key);
	}

	void FRealtimeMeshLOD::UpdateConfig(FRealtimeMeshUpdateContext& UpdateContext, const FRealtimeMeshLODConfig& InConfig)
	{
		Config = InConfig;

		if (auto ProxyBuilder = UpdateContext.GetProxyBuilder())
		{
			ProxyBuilder->AddLODTask(Key, [Config = Config](FRHICommandListBase& RHICmdList, FRealtimeMeshLODProxy& Proxy)
			{
				Proxy.UpdateConfig(Config);
			}, ShouldRecreateProxyOnChange(UpdateContext));
		}

		UpdateContext.GetState().ConfigDirtyTree.Flag(Key);
	}
	
	void FRealtimeMeshLOD::CreateOrUpdateSectionGroup(FRealtimeMeshUpdateContext& UpdateContext, const FRealtimeMeshSectionGroupKey& SectionGroupKey, const FRealtimeMeshSectionGroupConfig& InConfig)
	{
		if (!SectionGroups.Contains(SectionGroupKey))
		{
			SectionGroups.Add(SharedResources->CreateSectionGroup(SectionGroupKey));

			if (auto ProxyBuilder = UpdateContext.GetProxyBuilder())
			{
				ProxyBuilder->AddLODTask(Key, [SectionGroupKey](FRHICommandListBase& RHICmdList, FRealtimeMeshLODProxy& Proxy)
				{
					Proxy.CreateSectionGroupIfNotExists(SectionGroupKey);
				});
			}
		}

		const auto& SectionGroup = *SectionGroups.Find(SectionGroupKey);
		SectionGroup->Initialize(UpdateContext, InConfig);
	}

	void FRealtimeMeshLOD::RemoveSectionGroup(FRealtimeMeshUpdateContext& UpdateContext, const FRealtimeMeshSectionGroupKey& SectionGroupKey)
	{
		if (SectionGroups.Remove(SectionGroupKey))
		{
			if (auto ProxyBuilder = UpdateContext.GetProxyBuilder())
			{
				ProxyBuilder->AddLODTask(Key, [SectionGroupKey](FRHICommandListBase& RHICmdList, FRealtimeMeshLODProxy& Proxy)
				{
					Proxy.RemoveSectionGroup(SectionGroupKey);
				}, ShouldRecreateProxyOnChange(UpdateContext));
			}
		}
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
				FName SectionGroupName = SectionGroup->GetKey_AssumesLocked().Name();
				Ar << SectionGroupName;
				SectionGroup->Serialize(Ar);
			}
		}

		Ar << Config;
		Ar << Bounds;
		return true;
	}

	void FRealtimeMeshLOD::InitializeProxy(FRealtimeMeshUpdateContext& UpdateContext)
	{
		if (auto ProxyBuilder = UpdateContext.GetProxyBuilder())
		{
			ProxyBuilder->AddLODTask(Key, [Config = Config](FRHICommandListBase& RHICmdList, FRealtimeMeshLODProxy& Proxy)
			{
				Proxy.UpdateConfig(Config);
			}, ShouldRecreateProxyOnChange(UpdateContext));
		
			for (const auto& SectionGroup : SectionGroups)
			{
				ProxyBuilder->AddLODTask(Key, [SectionGroupKey = SectionGroup->GetKey(UpdateContext)](FRHICommandListBase& RHICmdList, FRealtimeMeshLODProxy& Proxy)
				{
					Proxy.CreateSectionGroupIfNotExists(SectionGroupKey);
				}, ShouldRecreateProxyOnChange(UpdateContext));

				SectionGroup->InitializeProxy(UpdateContext);
			}
		}
	}


	TSet<FRealtimeMeshSectionGroupKey> FRealtimeMeshLOD::GetSectionGroupKeys(const FRealtimeMeshLockContext& LockContext) const
	{
		TSet<FRealtimeMeshSectionGroupKey> SectionGroupKeys;
		for (const auto& SectionGroup : SectionGroups)
		{
			SectionGroupKeys.Add(SectionGroup->GetKey(LockContext));
		}
		return SectionGroupKeys;
	}

	void FRealtimeMeshLOD::FinalizeUpdate(FRealtimeMeshUpdateContext& UpdateContext)
	{
		for (const auto& SectionGroup : SectionGroups)
		{
			SectionGroup->FinalizeUpdate(UpdateContext);
		}
		
		// Update bounds
		if (UpdateContext.GetState().BoundsDirtyTree.IsDirty(Key) && !Bounds.HasUserSetBounds())
		{
			TOptional<FBoxSphereBounds3f> NewBounds;
			for (const auto& SectionGroup : SectionGroups)
			{
				auto SectionGroupBounds = SectionGroup->GetLocalBounds(UpdateContext);
				if (SectionGroupBounds.IsSet())
				{
					if (!NewBounds.IsSet())
					{
						NewBounds = *SectionGroupBounds;
						continue;
					}
					NewBounds = *NewBounds + *SectionGroupBounds;
				}
			}

			if (NewBounds)
			{
				Bounds.SetComputedBounds(*NewBounds);
			}
			else
			{
				Bounds.ClearCachedValue();
			}

			UpdateContext.GetState().bNeedsBoundsUpdate = true;
		}
	}

	void FRealtimeMeshLOD::MarkBoundsDirtyIfNotOverridden(FRealtimeMeshUpdateContext& UpdateContext)
	{
		Bounds.ClearCachedValue();
		if (!Bounds.HasUserSetBounds())
		{
			UpdateContext.GetState().BoundsDirtyTree.Flag(Key);
		}
	}


}

#undef LOCTEXT_NAMESPACE
