// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#include "Data/RealtimeMeshLOD.h"
#include "Data/RealtimeMeshShared.h"
#include "Data/RealtimeMeshBufferSet.h"
#include "Core/RealtimeMeshLODConfig.h"
#include "Data/RealtimeMeshUpdateBuilder.h"
#include "RenderProxy/RealtimeMeshLODProxy.h"
#include "RenderProxy/RealtimeMeshProxyCommandBatch.h"
#include "Logging/MessageLog.h"

namespace RealtimeMesh
{
	FRealtimeMeshLOD::FRealtimeMeshLOD(const FRealtimeMeshContextRef& InContext, const FRealtimeMeshLODKey& InKey)
		: Context(InContext)
		, Key(InKey)
	{
	}

	bool FRealtimeMeshLOD::HasSectionGroups(const FRealtimeMeshLockContext& LockContext) const
	{
		return SectionGroups.Num() > 0;
	}

	FRealtimeMeshSectionGroupPtr FRealtimeMeshLOD::GetSectionGroup(const FRealtimeMeshLockContext& LockContext, const FRealtimeMeshBufferSetKey& SectionGroupKey) const
	{
		if (const FRealtimeMeshSectionGroupRef* Found = SectionGroups.Find(SectionGroupKey))
		{
			return *Found;
		}
		return FRealtimeMeshSectionGroupPtr();
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

		// Queue removal of the now-orphaned section groups from the render proxy,
		// otherwise the proxy keeps their stale buffer sets until an unrelated recreate.
		if (auto ProxyBuilder = UpdateContext.GetProxyBuilder())
		{
			for (const FRealtimeMeshBufferSetKey& SectionGroupKey : SectionGroupsToRemove)
			{
				ProxyBuilder->AddLODTask(Key, [SectionGroupKey](FRHICommandListBase& RHICmdList, FRealtimeMeshLODProxy& Proxy)
				{
					Proxy.RemoveSectionGroup(SectionGroupKey);
				}, true);
			}
		}

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
			}, true);
		}

		UpdateContext.GetState().ConfigDirtyTree.Flag(Key);
	}
	
	void FRealtimeMeshLOD::CreateOrUpdateSectionGroup(FRealtimeMeshUpdateContext& UpdateContext, const FRealtimeMeshBufferSetKey& SectionGroupKey, const FRealtimeMeshBufferSetConfig& InConfig)
	{
		if (!SectionGroups.Contains(SectionGroupKey))
		{
			SectionGroups.Add(Context->CreateSectionGroup(SectionGroupKey));

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

	void FRealtimeMeshLOD::RemoveSectionGroup(FRealtimeMeshUpdateContext& UpdateContext, const FRealtimeMeshBufferSetKey& SectionGroupKey)
	{
		if (SectionGroups.Remove(SectionGroupKey))
		{
			if (auto ProxyBuilder = UpdateContext.GetProxyBuilder())
			{
				ProxyBuilder->AddLODTask(Key, [SectionGroupKey](FRHICommandListBase& RHICmdList, FRealtimeMeshLODProxy& Proxy)
				{
					Proxy.RemoveSectionGroup(SectionGroupKey);
				}, true);
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

				FRealtimeMeshBufferSetKey SectionGroupKey = FRealtimeMeshBufferSetKey::Create(Key, SectionGroupIndex);
				auto SectionGroup = Context->CreateSectionGroup(SectionGroupKey);
				SectionGroup->Serialize(Ar);
				SectionGroups.Add(SectionGroup);
			}
		}
		else if (Ar.IsLoading())
		{
			// Group-key identity is (LODIndex, SlotIndex); the friendly Name is metadata.
			// Pre-SerializeSectionGroupKeySlotIndex assets stored only the Name, and the
			// SlotIndex was re-derived from FCrc::StrCrc32(Name) via the FName Create factory.
			// That silently collapsed distinct groups that shared a name (e.g. every procedural
			// group is named "PMC") down to one identical key, so the TSet kept only the last.
			// From v16 on we also read the SlotIndex directly so distinct groups keep distinct
			// keys and survive the round trip.
			const bool bHasDirectSlotIndex =
				Ar.CustomVer(RealtimeMesh::FRealtimeMeshVersion::GUID) >= FRealtimeMeshVersion::SerializeSectionGroupKeySlotIndex;

			SectionGroups.Empty();
			for (int32 Index = 0; Index < NumSectionGroups; Index++)
			{
				FName SectionGroupName;
				Ar << SectionGroupName;

				FRealtimeMeshBufferSetKey SectionGroupKey = FRealtimeMeshBufferSetKey::Create(Key, SectionGroupName);
				if (bHasDirectSlotIndex)
				{
					int32 SectionGroupSlotIndex;
					Ar << SectionGroupSlotIndex;
					SectionGroupKey = FRealtimeMeshBufferSetKey::Create(Key, SectionGroupSlotIndex, SectionGroupName);
				}

				auto SectionGroup = Context->CreateSectionGroup(SectionGroupKey);
				SectionGroup->Serialize(Ar);
				SectionGroups.Add(SectionGroup);
			}
		}
		else
		{
			// Always write the SlotIndex directly (this save path is only reached at the
			// current version, which is >= SerializeSectionGroupKeySlotIndex). See the load
			// branch for why name-only would collapse same-named groups.
			for (const auto& SectionGroup : SectionGroups)
			{
				const FRealtimeMeshBufferSetKey SectionGroupKey = SectionGroup->GetKey_AssumesLocked();
				FName SectionGroupName = SectionGroupKey.Name();
				Ar << SectionGroupName;
				int32 SectionGroupSlotIndex = SectionGroupKey.Index();
				Ar << SectionGroupSlotIndex;
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
			}, true);
		
			for (const auto& SectionGroup : SectionGroups)
			{
				ProxyBuilder->AddLODTask(Key, [SectionGroupKey = SectionGroup->GetKey(UpdateContext)](FRHICommandListBase& RHICmdList, FRealtimeMeshLODProxy& Proxy)
				{
					Proxy.CreateSectionGroupIfNotExists(SectionGroupKey);
				}, true);

				SectionGroup->InitializeProxy(UpdateContext);
			}
		}
	}


	TSet<FRealtimeMeshBufferSetKey> FRealtimeMeshLOD::GetSectionGroupKeys(const FRealtimeMeshLockContext& LockContext) const
	{
		TSet<FRealtimeMeshBufferSetKey> SectionGroupKeys;
		for (const auto& SectionGroup : SectionGroups)
		{
			SectionGroupKeys.Add(SectionGroup->GetKey(LockContext));
		}
		return SectionGroupKeys;
	}

	void FRealtimeMeshLOD::FinalizeUpdate(FRealtimeMeshUpdateContext& UpdateContext)
	{
		// Only descend into section groups the update touched (see FRealtimeMesh::FinalizeUpdate).
		// A group with only stream edits doesn't flag the bounds tree, so consult both.
		FRealtimeMeshUpdateState& State = UpdateContext.GetState();
		for (const auto& SectionGroup : SectionGroups)
		{
			const FRealtimeMeshBufferSetKey SectionGroupKey = SectionGroup->GetKey(UpdateContext);
			if (State.BoundsDirtyTree.IsDirty(SectionGroupKey) || State.StreamRangeDirtyTree.IsDirty(SectionGroupKey) || State.StreamDirtyTree.HasDirtyStreams(SectionGroupKey))
			{
				SectionGroup->FinalizeUpdate(UpdateContext);
			}
		}

		// Update bounds
		if (UpdateContext.GetState().BoundsDirtyTree.IsDirty(Key) && !Bounds.HasUserSetBounds())
		{
			// DUP-008: shared accumulate-hull helper (RealtimeMeshShared.h).
			const TOptional<FBoxSphereBounds3f> NewBounds = AccumulateBounds(SectionGroups,
				[&UpdateContext](const FRealtimeMeshSectionGroupRef& SectionGroup) { return SectionGroup->GetLocalBounds(UpdateContext); });

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
