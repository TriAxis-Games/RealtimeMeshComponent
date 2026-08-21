// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#include "Data/RealtimeMeshBufferSet.h"
#include "Data/RealtimeMeshShared.h"
#include "Data/RealtimeMeshSection.h"
#include "Data/RealtimeMeshUpdateBuilder.h"
#include "RenderProxy/RealtimeMeshGPUBuffer.h"
#include "RenderProxy/RealtimeMeshProxyCommandBatch.h"
#include "RenderProxy/RealtimeMeshBufferSetProxy.h"
#include "Logging/MessageLog.h"

namespace RealtimeMesh
{
	// Kill switch for the in-place GPU fast-update path. When 0, EditMeshInPlace[Ranged]
	// on Dynamic section groups routes through the normal reallocating update instead of
	// the staging-copy in-place path. Defaults on; flip off to isolate rendering issues.
	static int32 GRealtimeMeshEnableInPlaceUpdate = 1;
	static FAutoConsoleVariableRef CVarRealtimeMeshEnableInPlaceUpdate(
		TEXT("RealtimeMesh.InPlaceUpdate.Enabled"),
		GRealtimeMeshEnableInPlaceUpdate,
		TEXT("Enable the in-place GPU stream fast-update path for Dynamic section groups (1, default) or force the full reallocating update (0)."),
		ECVF_Default);

	FRealtimeMeshBufferSet::FRealtimeMeshBufferSet(const FRealtimeMeshContextRef& InContext, const FRealtimeMeshBufferSetKey& InKey)
		: Context(InContext)
		, Key(InKey)
	{
	}

	FRealtimeMeshStreamRange FRealtimeMeshBufferSet::GetInUseRange(const FRealtimeMeshLockContext& LockContext) const
	{
		// DUP-008: shared hull-accumulate overload (RealtimeMeshShared.h).
		const TOptional<FRealtimeMeshStreamRange> NewRange = AccumulateStreamRangeHull(Sections,
			[&LockContext](const FRealtimeMeshSectionRef& Section) { return Section->GetStreamRange(LockContext); });

		return NewRange.IsSet() ? *NewRange : FRealtimeMeshStreamRange::Empty();
	}

	TOptional<FBoxSphereBounds3f> FRealtimeMeshBufferSet::GetLocalBounds(const FRealtimeMeshLockContext& LockContext) const
	{
		return Bounds.Get();
	}

	bool FRealtimeMeshBufferSet::HasSections(const FRealtimeMeshLockContext& LockContext) const
	{
		return Sections.Num() > 0;
	}

	int32 FRealtimeMeshBufferSet::NumSections(const FRealtimeMeshLockContext& LockContext) const
	{
		return Sections.Num();
	}

	bool FRealtimeMeshBufferSet::HasStreams(const FRealtimeMeshLockContext& LockContext) const
	{
		return Streams.Num() > 0;
	}

	TSet<FRealtimeMeshStreamKey> FRealtimeMeshBufferSet::GetStreamKeys(const FRealtimeMeshLockContext& LockContext) const
	{
		return Streams;
	}

	TSet<FRealtimeMeshSectionKey> FRealtimeMeshBufferSet::GetSectionKeys(const FRealtimeMeshLockContext& LockContext) const
	{
		TSet<FRealtimeMeshSectionKey> SectionKeys;
		for (const auto& Section : Sections)
		{
			SectionKeys.Add(Section->GetKey(LockContext));
		}
		return SectionKeys;
	}

	FRealtimeMeshSectionPtr FRealtimeMeshBufferSet::GetSection(const FRealtimeMeshLockContext& LockContext, const FRealtimeMeshSectionKey& SectionKey) const
	{
		if (SectionKey.IsPartOf(Key))
		{
			if (const FRealtimeMeshSectionRef* Found = Sections.Find(SectionKey))
			{
				return *Found;
			}
		}
		return nullptr;
	}

	void FRealtimeMeshBufferSet::Initialize(FRealtimeMeshUpdateContext& UpdateContext, const FRealtimeMeshBufferSetConfig& InConfig)
	{
		Config = InConfig;
		Streams.Empty();
		Sections.Empty();
		Bounds.Reset();

		InitializeProxy(UpdateContext);

		UpdateContext.GetState().ConfigDirtyTree.Flag(Key);
		UpdateContext.GetState().BoundsDirtyTree.Flag(Key);
	}

	void FRealtimeMeshBufferSet::Reset(FRealtimeMeshUpdateContext& UpdateContext)
	{
		Initialize(UpdateContext, FRealtimeMeshBufferSetConfig());
	}

	void FRealtimeMeshBufferSet::SetOverrideBounds(FRealtimeMeshUpdateContext& UpdateContext, const FBoxSphereBounds3f& InBounds)
	{
		Bounds.SetUserSetBounds(InBounds);
		UpdateContext.GetState().BoundsDirtyTree.Flag(Key);
	}

	void FRealtimeMeshBufferSet::ClearOverrideBounds(FRealtimeMeshUpdateContext& UpdateContext)
	{
		Bounds.ClearUserSetBounds();
		UpdateContext.GetState().BoundsDirtyTree.Flag(Key);
	}

	void FRealtimeMeshBufferSet::UpdateConfig(FRealtimeMeshUpdateContext& UpdateContext, const FRealtimeMeshBufferSetConfig& InConfig)
	{
		UpdateConfig(UpdateContext, [InConfig](FRealtimeMeshBufferSetConfig& ExistingConfig) { ExistingConfig = InConfig; });
	}

	void FRealtimeMeshBufferSet::UpdateConfig(FRealtimeMeshUpdateContext& UpdateContext, TFunction<void(FRealtimeMeshBufferSetConfig&)> EditFunc)
	{
		bool bShouldRecreateProxy = ShouldRecreateProxyOnChange(UpdateContext);
		EditFunc(Config);
		bShouldRecreateProxy |= ShouldRecreateProxyOnChange(UpdateContext);

		if (auto ProxyBuilder = UpdateContext.GetProxyBuilder())
		{
			ProxyBuilder->AddSectionGroupTask(Key, [Config = Config](FRHICommandListBase& RHICmdList, FRealtimeMeshBufferSetProxy& Proxy)
			{
				Proxy.UpdateConfig(Config);
			}, bShouldRecreateProxy);
		}

		UpdateContext.GetState().ConfigDirtyTree.Flag(Key);
	}

	void FRealtimeMeshBufferSet::CreateOrUpdateStream(FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshStream&& Stream)
	{
		const auto StreamKey = Stream.GetStreamKey();
		bool bAlreadyExisted = false;

		// Make sure we have the stream registered
		Streams.FindOrAdd(StreamKey, &bAlreadyExisted);

		// Create the update data for the GPU
		if (Context->WantsStreamOnGPU(StreamKey))
		{
			if (auto ProxyBuilder = UpdateContext.GetProxyBuilder())
			{
				if (Stream.Num() > 0)
				{
					// Stream is an rvalue owned exclusively by us here; move its payload into the
					// GPU update data instead of deep-copying (managed path already copied once).
					const auto UpdateData = MakeShared<FRealtimeMeshSectionGroupStreamUpdateData>(MoveTemp(Stream), GetGPUBufferUsageFlags());
					UpdateData->CreateBufferAsyncIfPossible(UpdateContext);

					ProxyBuilder->AddSectionGroupTask(Key, [UpdateData = UpdateData](FRHICommandListBase& RHICmdList, FRealtimeMeshBufferSetProxy& Proxy)
					{
						Proxy.CreateOrUpdateStream(RHICmdList, UpdateData);
					}, ShouldRecreateProxyOnChange(UpdateContext));
				}
				else
				{
					ProxyBuilder->AddSectionGroupTask(Key, [StreamKey](FRHICommandListBase& RHICmdList, FRealtimeMeshBufferSetProxy& Proxy)
					{
						Proxy.RemoveStream(StreamKey);
					}, ShouldRecreateProxyOnChange(UpdateContext));
				}
			}
		}

		UpdateContext.GetState().StreamDirtyTree.Flag(Key, StreamKey);
	}

	void FRealtimeMeshBufferSet::FastUpdateStream(FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshStream&& Stream, const FInt32Range& ElementRange)
	{
		const FRealtimeMeshStreamKey StreamKey = Stream.GetStreamKey();

		// In-place is only viable for an existing vertex stream on a Dynamic buffer set with
		// data to upload. New streams, index/topology streams, and Static (non-lockable)
		// buffer sets use the normal reallocating path.
		const bool bEligible =
			GRealtimeMeshEnableInPlaceUpdate != 0 &&
			Config.DrawType == ERealtimeMeshSectionDrawType::Dynamic &&
			StreamKey.IsVertexStream() &&
			Streams.Contains(StreamKey) &&
			Stream.Num() > 0 &&
			Context->WantsStreamOnGPU(StreamKey);

		if (!bEligible)
		{
			// Note: explicit base call (the CPU stream storage is already updated by the
			// caller, mirroring EditMeshData), avoiding a managed re-add / poly-group pass.
			FRealtimeMeshBufferSet::CreateOrUpdateStream(UpdateContext, MoveTemp(Stream));
			return;
		}

		if (auto ProxyBuilder = UpdateContext.GetProxyBuilder())
		{
			// Resolve the element range (empty = whole stream), clamped to the stream.
			const int32 Num = Stream.Num();
			int32 Offset = 0;
			int32 Count = Num;
			if (!ElementRange.IsEmpty())
			{
				// FInt32Range is treated as a half-open [lower, upper) element span, matching
				// FRealtimeMeshStreamRange usage elsewhere.
				Offset = FMath::Clamp(ElementRange.GetLowerBoundValue(), 0, Num);
				const int32 Span = ElementRange.GetUpperBoundValue() - ElementRange.GetLowerBoundValue();
				Count = FMath::Clamp(Span, 0, Num - Offset);
			}
			if (Count <= 0)
			{
				Offset = 0;
				Count = Num;
			}

			// Stream is an rvalue owned exclusively by us here; move its payload into the
			// GPU update data instead of deep-copying. Num/Offset/Count already captured above.
			const auto UpdateData = MakeShared<FRealtimeMeshSectionGroupStreamUpdateData>(MoveTemp(Stream), GetGPUBufferUsageFlags());
			ProxyBuilder->AddInPlaceStreamUpdate(Key, UpdateData, Offset, Count);
		}

		UpdateContext.GetState().StreamDirtyTree.Flag(Key, StreamKey);

		// Position changes move the mesh's bounds; flag so the bounds pass refreshes them.
		if (StreamKey == FRealtimeMeshStreams::Position)
		{
			UpdateContext.GetState().BoundsDirtyTree.Flag(Key);
		}
	}

	void FRealtimeMeshBufferSet::RemoveStream(FRealtimeMeshUpdateContext& UpdateContext, const FRealtimeMeshStreamKey& StreamKey)
	{
		if (Streams.Remove(StreamKey))
		{
			if (Context->WantsStreamOnGPU(StreamKey))
			{
				if (auto ProxyBuilder = UpdateContext.GetProxyBuilder())
				{
					ProxyBuilder->AddSectionGroupTask(Key, [StreamKey](FRHICommandListBase& RHICmdList, FRealtimeMeshBufferSetProxy& Proxy)
					{
						Proxy.RemoveStream(StreamKey);
					}, ShouldRecreateProxyOnChange(UpdateContext));
				}
			}
		}

		UpdateContext.GetState().StreamDirtyTree.Flag(Key, StreamKey);
	}

	void FRealtimeMeshBufferSet::SetAllStreams(FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshStreamSet&& InStreams)
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

	void FRealtimeMeshBufferSet::CreateOrUpdateSection(FRealtimeMeshUpdateContext& UpdateContext, const FRealtimeMeshSectionKey& SectionKey,
		const FRealtimeMeshSectionConfig& InConfig, const FRealtimeMeshStreamRange& InStreamRange)
	{
		check(SectionKey.IsPartOf(Key));

		const bool bExisted = Sections.Contains(SectionKey);

		if (!bExisted)
		{
			const auto Section = Context->CreateSection(SectionKey);
			Sections.Add(Section);

			if (auto ProxyBuilder = UpdateContext.GetProxyBuilder())
			{
				ProxyBuilder->AddSectionGroupTask(Key, [SectionKey](FRHICommandListBase& RHICmdList, FRealtimeMeshBufferSetProxy& Proxy)
				{
					Proxy.CreateSectionIfNotExists(SectionKey);
				});
			}
		}

		const auto& Section = *Sections.Find(SectionKey);
		Section->Initialize(UpdateContext, InConfig, InStreamRange);
	}

	void FRealtimeMeshBufferSet::RemoveSection(FRealtimeMeshUpdateContext& UpdateContext, const FRealtimeMeshSectionKey& SectionKey)
	{
		if (Sections.Remove(SectionKey))
		{
			if (auto ProxyBuilder = UpdateContext.GetProxyBuilder())
			{
				ProxyBuilder->AddSectionGroupTask(Key, [SectionKey](FRHICommandListBase& RHICmdList, FRealtimeMeshBufferSetProxy& Proxy)
				{
					Proxy.RemoveSection(SectionKey);
				}, ShouldRecreateProxyOnChange(UpdateContext));
			}
		}
	}

	bool FRealtimeMeshBufferSet::Serialize(FArchive& Ar)
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

				auto Section = Context->CreateSection(SectionKey);
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

	void FRealtimeMeshBufferSet::InitializeProxy(FRealtimeMeshUpdateContext& UpdateContext)
	{
		if (auto ProxyBuilder = UpdateContext.GetProxyBuilder())
		{
			// We only send sections here, we rely on the derived to setup the streams
			for (const auto& Section : Sections)
			{
				ProxyBuilder->AddSectionGroupTask(Key, [SectionKey = Section->GetKey(UpdateContext), Config = Config](FRHICommandListBase& RHICmdList, FRealtimeMeshBufferSetProxy& Proxy)
				{
					Proxy.CreateSectionIfNotExists(SectionKey);
					Proxy.UpdateConfig(Config);
				}, ShouldRecreateProxyOnChange(UpdateContext));

				Section->InitializeProxy(UpdateContext);
			}
		}
	}

	void FRealtimeMeshBufferSet::FinalizeUpdate(FRealtimeMeshUpdateContext& UpdateContext)
	{
		// Only descend into sections that could do finalize work. A section's bounds recompute
		// is driven by its own bounds/stream-range dirtiness, or by any of the group's streams
		// changing (which affects every section in the group). Streams are tracked per group,
		// so a group-wide stream edit forces the whole loop; otherwise only individually dirty
		// sections are visited.
		FRealtimeMeshUpdateState& State = UpdateContext.GetState();
		const bool bGroupStreamsDirty = State.StreamDirtyTree.HasDirtyStreams(Key);
		for (const auto& Section : Sections)
		{
			const FRealtimeMeshSectionKey SectionKey = Section->GetKey(UpdateContext);
			if (bGroupStreamsDirty || State.BoundsDirtyTree.IsDirty(SectionKey) || State.StreamRangeDirtyTree.IsDirty(SectionKey))
			{
				Section->FinalizeUpdate(UpdateContext);
			}
		}

		// Update bounds
		if (UpdateContext.GetState().BoundsDirtyTree.IsDirty(Key) && !Bounds.HasUserSetBounds())
		{
			// DUP-008: shared accumulate-hull helper (RealtimeMeshShared.h).
			const TOptional<FBoxSphereBounds3f> NewBounds = AccumulateBounds(Sections,
				[&UpdateContext](const FRealtimeMeshSectionRef& Section) { return Section->GetLocalBounds(UpdateContext); });
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

	void FRealtimeMeshBufferSet::MarkBoundsDirtyIfNotOverridden(FRealtimeMeshUpdateContext& UpdateContext)
	{	
		Bounds.ClearCachedValue();
		if (!Bounds.HasUserSetBounds())
		{
			UpdateContext.GetState().BoundsDirtyTree.Flag(Key);
		}
	}




}
