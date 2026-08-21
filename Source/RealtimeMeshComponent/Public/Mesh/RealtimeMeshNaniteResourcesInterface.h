// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include <atomic>

#include "RealtimeMeshCore.h"
#include "Rendering/NaniteResources.h"
#include "Rendering/NaniteStreamingManager.h"
#include "RealtimeMeshComponentModule.h"

// ================================================================================================
// FRealtimeMeshNaniteResources — the RMC runtime Nanite resource, in three build tiers (see
// RMC-Plan-Nanite-Stock-Engine-Fallback). The public API is byte-for-byte identical across tiers so
// no caller changes: Create / CreateFromCopy / Clone / Consume / HasValidData / GetBounds /
// InitResources(const UObject*) / ReleaseResources() / ClearRuntimeState / GetNaniteProvider().
//
//   Tier A (RMC_NANITE_ENGINE_PROVIDER):  provider fork. Subclass ::Nanite::FResourcesProvider, own
//                                         the provision arrays, serve pages via UploadPageBytesForProvider.
//   Tier B (RMC_NANITE_AVAILABLE only):   stock 5.7. Wrap a real ::Nanite::FResources; streaming pages
//                                         become in-memory bulk data served by the stock streaming path.
//   Tier C (neither):                     unavailable. Stubs; Create* return null with a one-time warning.
// ================================================================================================

namespace RealtimeMesh
{
	struct FRealtimeMeshNaniteResources;

	struct FRealtimeMeshNaniteResourcesDeleter
	{
		void operator()(FRealtimeMeshNaniteResources* Resources) const
		{
			Destroy(Resources);
		}

		static void Destroy(FRealtimeMeshNaniteResources* Resources);
	};

	using FRealtimeMeshNaniteResourcesPtr = TUniquePtr<FRealtimeMeshNaniteResources, FRealtimeMeshNaniteResourcesDeleter>;

	// The one visible seam between tiers (§4.1). Field-access code (RuntimeResourceID, HierarchyOffset,
	// AssemblyTransformOffset, ImposterIndex, NumClusters, ...) compiles identically under both aliases
	// because the fork hoisted those identity fields onto the base unchanged and stock keeps them on
	// FResources. Only array access differs (getters on the fork base vs fields on stock FResources).
#if RMC_NANITE_ENGINE_PROVIDER
	// Fork: the polymorphic base every Nanite resource implements.
	using FRealtimeMeshNaniteEngineResources = ::Nanite::FResourcesProvider;
#else
	// Stock: the concrete cooked resource type (which carries the same identity fields).
	using FRealtimeMeshNaniteEngineResources = ::Nanite::FResources;
#endif


#if RMC_NANITE_ENGINE_PROVIDER
	// ============================================================================================
	// Tier A — provider fork.
	// ============================================================================================
	// Inherits FResourcesProvider directly — the polymorphic base for Nanite resources — so we don't carry
	// the cooked FResources subobject's empty arrays, FByteBulkData StreamablePages, or WITH_EDITOR DDC
	// machinery. RMC owns its provision arrays (Own*) and exposes them via the const virtual getters;
	// streaming pages are served from a plain TArray<uint8> via the GetPageFixupChunk / InstallPage overrides.
	struct FRealtimeMeshNaniteResources : public ::Nanite::FResourcesProvider
	{
		friend struct FRealtimeMeshNaniteResourcesDeleter;
	private:
		TArray<uint8>							OwnRootData;
		// Concatenated streaming-page bytes. Each page begins with its FFixupChunk; PageStreamingStates[i]
		// .BulkOffset / .BulkSize index into this array. BulkSize - PageSize == FixupChunkSize.
		TArray<uint8>							OwnStreamingPagesData;
		TArray<::Nanite::FPackedHierarchyNode>	OwnHierarchyNodes;
		TArray<uint32>							OwnHierarchyRootOffsets;
		TArray<::Nanite::FPageStreamingState>	OwnPageStreamingStates;
		TArray<uint16>							OwnPageDependencies;
		TArray<::Nanite::FPageRangeKey>			OwnPageRangeLookup;
		TArray<FMatrix3x4>						OwnAssemblyTransforms;
		TArray<uint32>							OwnAssemblyBoneAttachmentData;
		TArray<uint16>							OwnImposterAtlas;

		FBoxSphereBounds3f Bounds;
		// Nanite resource-sharing: written on the game thread (InitResources) and read by the custom
		// deleter's ReleaseResources() on whatever thread drops the last shared reference (game thread for
		// the mesh's ref, render thread for a scene-proxy ref). Atomic (relaxed) to avoid a formal data
		// race — it only gates whether ReleaseResources runs, with no happens-before dependency, mirroring
		// the bHasNaniteData precedent (PROXY-F17). With E2 the engine no longer writes provider hierarchy
		// memory, so a shared immutable payload can safely back many registrations.
		std::atomic<bool> bIsInitialized;

#if !UE_BUILD_SHIPPING
		// Cached owner identity so the register/unregister diagnostic pairs can be matched per mesh in logs
		// (ReleaseResources has no OwningMesh param). Non-Shipping only; not an audit — E2 removed the
		// hierarchy-hash audit machinery entirely (the engine no longer writes provider memory).
		FString AuditOwnerName;
#endif


		FRealtimeMeshNaniteResources(::Nanite::FResources&& InResources, TArray<uint8>&& InStreamingPagesData, const FBoxSphereBounds3f& InBounds)
			: OwnRootData(MoveTemp(InResources.RootData))
			, OwnStreamingPagesData(MoveTemp(InStreamingPagesData))
			, OwnHierarchyNodes(MoveTemp(InResources.HierarchyNodes))
			, OwnHierarchyRootOffsets(MoveTemp(InResources.HierarchyRootOffsets))
			, OwnPageStreamingStates(MoveTemp(InResources.PageStreamingStates))
			, OwnPageDependencies(MoveTemp(InResources.PageDependencies))
			, OwnPageRangeLookup(MoveTemp(InResources.PageRangeLookup))
			, OwnAssemblyTransforms(MoveTemp(InResources.AssemblyTransforms))
			, OwnAssemblyBoneAttachmentData(MoveTemp(InResources.AssemblyBoneAttachmentData))
			, OwnImposterAtlas(MoveTemp(InResources.ImposterAtlas))
			, Bounds(InBounds)
			, bIsInitialized(false)
		{
			// Copy cached scalars from the builder's temp into our FResourcesProvider base.
			NumRootPages		= InResources.NumRootPages;
			NumClusters			= InResources.NumClusters;
			ResourceFlags		= InResources.ResourceFlags;
			MeshBounds			= InResources.MeshBounds;
			NumInputTriangles	= InResources.NumInputTriangles;
			NumInputVertices	= InResources.NumInputVertices;
			PositionPrecision	= InResources.PositionPrecision;
			NormalPrecision		= InResources.NormalPrecision;
			TangentPrecision	= InResources.TangentPrecision;
			VoxelMaterialsMask	= InResources.VoxelMaterialsMask;
			ClearRuntimeState();
		}
		FRealtimeMeshNaniteResources(const FRealtimeMeshNaniteResources& Other)
			: OwnRootData(Other.OwnRootData)
			, OwnStreamingPagesData(Other.OwnStreamingPagesData)
			, OwnHierarchyNodes(Other.OwnHierarchyNodes)
			, OwnHierarchyRootOffsets(Other.OwnHierarchyRootOffsets)
			, OwnPageStreamingStates(Other.OwnPageStreamingStates)
			, OwnPageDependencies(Other.OwnPageDependencies)
			, OwnPageRangeLookup(Other.OwnPageRangeLookup)
			, OwnAssemblyTransforms(Other.OwnAssemblyTransforms)
			, OwnAssemblyBoneAttachmentData(Other.OwnAssemblyBoneAttachmentData)
			, OwnImposterAtlas(Other.OwnImposterAtlas)
			, Bounds(Other.Bounds)
			, bIsInitialized(false)
		{
			// Copying a registered (initialized) instance is a lifetime bug: the copy holds no engine
			// registration yet duplicates identity state. Guard identically to the move ctor.
			check(!Other.bIsInitialized.load(std::memory_order_relaxed));
			NumRootPages		= Other.NumRootPages;
			NumClusters			= Other.NumClusters;
			ResourceFlags		= Other.ResourceFlags;
			MeshBounds			= Other.MeshBounds;
			NumInputTriangles	= Other.NumInputTriangles;
			NumInputVertices	= Other.NumInputVertices;
			PositionPrecision	= Other.PositionPrecision;
			NormalPrecision		= Other.NormalPrecision;
			TangentPrecision	= Other.TangentPrecision;
			VoxelMaterialsMask	= Other.VoxelMaterialsMask;
			ClearRuntimeState();
		}
		FRealtimeMeshNaniteResources(FRealtimeMeshNaniteResources&& Other)
			: OwnRootData(MoveTemp(Other.OwnRootData))
			, OwnStreamingPagesData(MoveTemp(Other.OwnStreamingPagesData))
			, OwnHierarchyNodes(MoveTemp(Other.OwnHierarchyNodes))
			, OwnHierarchyRootOffsets(MoveTemp(Other.OwnHierarchyRootOffsets))
			, OwnPageStreamingStates(MoveTemp(Other.OwnPageStreamingStates))
			, OwnPageDependencies(MoveTemp(Other.OwnPageDependencies))
			, OwnPageRangeLookup(MoveTemp(Other.OwnPageRangeLookup))
			, OwnAssemblyTransforms(MoveTemp(Other.OwnAssemblyTransforms))
			, OwnAssemblyBoneAttachmentData(MoveTemp(Other.OwnAssemblyBoneAttachmentData))
			, OwnImposterAtlas(MoveTemp(Other.OwnImposterAtlas))
			, Bounds(Other.Bounds)
			, bIsInitialized(false)
		{
			// Moving a registered instance is a lifetime bug (see the copy ctor).
			check(!Other.bIsInitialized.load(std::memory_order_relaxed));
			NumRootPages		= Other.NumRootPages;
			NumClusters			= Other.NumClusters;
			ResourceFlags		= Other.ResourceFlags;
			MeshBounds			= Other.MeshBounds;
			NumInputTriangles	= Other.NumInputTriangles;
			NumInputVertices	= Other.NumInputVertices;
			PositionPrecision	= Other.PositionPrecision;
			NormalPrecision		= Other.NormalPrecision;
			TangentPrecision	= Other.TangentPrecision;
			VoxelMaterialsMask	= Other.VoxelMaterialsMask;
			ClearRuntimeState();
		}

	public:
		// Polymorphic data-getter overrides — return const views into RMC-owned storage. All const: with E2
		// the engine patches hierarchy fixups into upload staging, never back into this memory.
		virtual TConstArrayView<uint8>							GetRootData() const override					{ return OwnRootData; }
		virtual TConstArrayView<::Nanite::FPackedHierarchyNode>	GetHierarchyNodes() const override				{ return OwnHierarchyNodes; }
		virtual TConstArrayView<uint32>							GetHierarchyRootOffsets() const override		{ return OwnHierarchyRootOffsets; }
		virtual TConstArrayView<::Nanite::FPageStreamingState>	GetPageStreamingStates() const override			{ return OwnPageStreamingStates; }
		virtual TConstArrayView<uint16>							GetPageDependencies() const override			{ return OwnPageDependencies; }
		virtual TConstArrayView<::Nanite::FPageRangeKey>		GetPageRangeLookup() const override				{ return OwnPageRangeLookup; }
		virtual TConstArrayView<FMatrix3x4>						GetAssemblyTransforms() const override			{ return OwnAssemblyTransforms; }
		virtual TConstArrayView<uint32>							GetAssemblyBoneAttachmentData() const override	{ return OwnAssemblyBoneAttachmentData; }
		virtual TConstArrayView<uint16>							GetImposterAtlas() const override				{ return OwnImposterAtlas; }

		// Streaming hooks. Bytes are always in memory, so RequestPage is always Available; GetPageFixupChunk
		// returns a pointer into OwnStreamingPagesData at the page's BulkOffset; InstallPage hands the
		// post-fixup-chunk bytes to FStreamingManager::UploadPageBytesForProvider which queues them in the
		// shared transcode upload (same path cooked uses).
		virtual ::Nanite::EStreamingPageRequestResult RequestPage(::Nanite::FStreamingHookContext, const ::Nanite::FPageKey&, uint32) override
		{
			return ::Nanite::EStreamingPageRequestResult::Available;
		}

		virtual const ::Nanite::FFixupChunk* GetPageFixupChunk(::Nanite::FStreamingHookContext, const ::Nanite::FPageKey& Key) override
		{
			const ::Nanite::FPageStreamingState& State = OwnPageStreamingStates[Key.PageIndex];
			return reinterpret_cast<const ::Nanite::FFixupChunk*>(OwnStreamingPagesData.GetData() + State.BulkOffset);
		}

		virtual void InstallPage(::Nanite::FStreamingHookContext, const ::Nanite::FPageKey& Key, const ::Nanite::FStreamingPageProduceTarget& Target) override
		{
			const ::Nanite::FPageStreamingState& State = OwnPageStreamingStates[Key.PageIndex];
			const uint32 FixupChunkSize = State.BulkSize - State.PageSize;
			const uint8* SrcDataBytes = OwnStreamingPagesData.GetData() + State.BulkOffset + FixupChunkSize;
			::Nanite::GStreamingManager.UploadPageBytesForProvider(*this, Key, Target, SrcDataBytes, State.PageSize);
		}

		virtual void OnPageSkipped(::Nanite::FStreamingHookContext, const ::Nanite::FPageKey&) override {}

	public:
		FRealtimeMeshNaniteResources()
			: bIsInitialized(false)
		{
			ClearRuntimeState();
		}

		FRealtimeMeshNaniteResources& operator=(const FRealtimeMeshNaniteResources&) = delete;
		FRealtimeMeshNaniteResources& operator=(FRealtimeMeshNaniteResources&&) = delete;

		static FRealtimeMeshNaniteResourcesPtr Create(::Nanite::FResources&& InResources, TArray<uint8>&& InStreamingPagesData, const FBoxSphereBounds3f& InBounds)
		{
			return FRealtimeMeshNaniteResourcesPtr(new FRealtimeMeshNaniteResources(MoveTemp(InResources), MoveTemp(InStreamingPagesData), InBounds));
		}

		// Compatibility wrapper for callers that produce a `::Nanite::FResources` with bytes in StreamablePages
		// (e.g. the prebuilt-data import path). Locks the bulk data, copies into a plain TArray, unlocks.
		static FRealtimeMeshNaniteResourcesPtr CreateFromCopy(const ::Nanite::FResources& InResources, const FBoxSphereBounds3f& InBounds)
		{
			::Nanite::FResources ResourcesCopy = InResources;
			TArray<uint8> StreamingPagesData;
			::Nanite::FResources& MutableSource = const_cast<::Nanite::FResources&>(InResources);
			if (MutableSource.StreamablePages.GetBulkDataSize() > 0 && MutableSource.StreamablePages.IsBulkDataLoaded())
			{
				const int64 NumBytes = MutableSource.StreamablePages.GetBulkDataSize();
				StreamingPagesData.SetNumUninitialized(NumBytes);
				const void* SrcBytes = MutableSource.StreamablePages.LockReadOnly();
				FMemory::Memcpy(StreamingPagesData.GetData(), SrcBytes, NumBytes);
				MutableSource.StreamablePages.Unlock();
			}
			return FRealtimeMeshNaniteResourcesPtr(new FRealtimeMeshNaniteResources(MoveTemp(ResourcesCopy), MoveTemp(StreamingPagesData), InBounds));
		}

		FRealtimeMeshNaniteResourcesPtr Clone() const
		{
			return FRealtimeMeshNaniteResourcesPtr(new FRealtimeMeshNaniteResources(*this));
		}

		FRealtimeMeshNaniteResourcesPtr Consume()
		{
			return FRealtimeMeshNaniteResourcesPtr(new FRealtimeMeshNaniteResources(MoveTemp(*this)));
		}

		bool HasValidData() const { return OwnRootData.Num() > 0; }

		const FBoxSphereBounds3f& GetBounds() const { return Bounds; }

		// Note: matches the FResourcesProvider::InitResources signature so it overrides; we add RMC's
		// data-validity checks before delegating to the base body.
		virtual void InitResources(const UObject* OwningMesh) override
		{
			check(IsValid(OwningMesh));
			if (bIsInitialized.load(std::memory_order_relaxed))
			{
				return;
			}

			if (!HasValidData())
			{
				UE_LOG(LogRealtimeMesh, Warning, TEXT("Attempting to initialize Nanite resources with invalid data for mesh: %s"),
					OwningMesh ? *OwningMesh->GetName() : TEXT("NULL"));
				return;
			}

			if (OwnHierarchyNodes.IsEmpty())
			{
				UE_LOG(LogRealtimeMesh, Warning, TEXT("Missing hierarchy nodes for Nanite mesh: %s"),
					OwningMesh ? *OwningMesh->GetName() : TEXT("NULL"));
				return;
			}

			if (NumClusters == 0)
			{
				UE_LOG(LogRealtimeMesh, Warning, TEXT("Nanite mesh has 0 clusters for mesh: %s - this may cause render issues"),
					OwningMesh ? *OwningMesh->GetName() : TEXT("NULL"));
			}

			::Nanite::FResourcesProvider::InitResources(OwningMesh);
			bIsInitialized.store(true, std::memory_order_relaxed);
#if !UE_BUILD_SHIPPING
			// Cache owner identity for the ReleaseResources diagnostic (which has no OwningMesh param).
			AuditOwnerName = OwningMesh ? OwningMesh->GetName() : FString(TEXT("NULL"));
#endif
			// Registration diagnostic (Nanite resource-sharing): one line per streaming-manager Add.
			// With instance sharing this should fire once per Nanite *data change*, not once per proxy
			// recreate. Enable with: Log LogRealtimeMesh Verbose.
			// NOTE: RuntimeResourceID is intentionally NOT logged here — it is assigned later on the render
			// thread by FStreamingManager::Add and is still MAX_uint32 at this point.
			UE_LOG(LogRealtimeMesh, Verbose, TEXT("Nanite InitResources (register) for mesh: %s"),
				OwningMesh ? *OwningMesh->GetName() : TEXT("NULL"));
		}

		virtual bool ReleaseResources() override
		{
			if (!bIsInitialized.load(std::memory_order_relaxed))
			{
				return false;
			}
#if !UE_BUILD_SHIPPING
			const uint32 DiagRuntimeResourceID = RuntimeResourceID;
#endif
			const bool bResult = ::Nanite::FResourcesProvider::ReleaseResources();
			bIsInitialized.store(false, std::memory_order_relaxed);
			// Registration diagnostic (Nanite resource-sharing): one line per streaming-manager Remove,
			// i.e. when the last shared reference to a registered instance drops.
#if !UE_BUILD_SHIPPING
			UE_LOG(LogRealtimeMesh, Verbose, TEXT("Nanite ReleaseResources (unregister) for mesh: %s [RuntimeResourceID=%u]"),
				*AuditOwnerName, DiagRuntimeResourceID);
#else
			UE_LOG(LogRealtimeMesh, Verbose, TEXT("Nanite ReleaseResources (unregister)"));
#endif
			return bResult;
		}

		// Polymorphic pointer for engine APIs that take FResourcesProvider* (aliased as
		// FRealtimeMeshNaniteEngineResources across tiers).
		FRealtimeMeshNaniteEngineResources* GetNaniteProvider() { return this; }
		const FRealtimeMeshNaniteEngineResources* GetNaniteProvider() const { return this; }

		void ClearRuntimeState()
		{
			if (!ensure(!bIsInitialized.load(std::memory_order_relaxed)))
			{
				return;
			}

			static const ::Nanite::FResources NullResources;
			RuntimeResourceID = NullResources.RuntimeResourceID;
			HierarchyOffset = NullResources.HierarchyOffset;
			RootPageIndex = NullResources.RootPageIndex;
#if RMC_ENGINE_BELOW_5_8
			// 5.8 removed FResources::ImposterIndex (Nanite imposters dropped); the proxy guards its reads
			// the same way (RealtimeMeshNaniteProxy.cpp).
			ImposterIndex = NullResources.ImposterIndex;
#endif
			NumHierarchyNodes = NullResources.NumHierarchyNodes;
			NumResidentClusters = NullResources.NumResidentClusters;
			PersistentHash = NullResources.PersistentHash;
#if RMC_ENGINE_ABOVE_5_6
			AssemblyTransformOffset = NullResources.AssemblyTransformOffset;
			NumHierarchyDwords = NullResources.NumHierarchyDwords;
#endif
#if WITH_EDITOR
			// ResourceName lives on the FResourcesProvider base now (the cooked path sets it in InitResources);
			// nothing else editor-only lives on the base, so the DDC clears are gone.
			ResourceName = FString();
#endif
		}
	};

#elif RMC_NANITE_AVAILABLE
	// ============================================================================================
	// Tier B — stock 5.7. Wrap a real ::Nanite::FResources.
	// ============================================================================================
	// Public API byte-for-byte identical to tier A. The builder's streaming-page blob becomes in-memory
	// bulk data; the stock streaming manager serves those pages straight from resident bulk (the
	// IsBulkDataLoaded() memory path) — full streaming, no disk/DDC backing. The shared-live-instance
	// ownership model (TSharedPtr master, refcount per proxy generation, deleter = ReleaseResources +
	// RT-deferred delete) carries over unchanged: stock FResources::InitResources/ReleaseResources have the
	// same enqueue-to-render-thread contract, and one registration serving many proxies is the engine's own
	// static-mesh model.
	struct FRealtimeMeshNaniteResources
	{
		friend struct FRealtimeMeshNaniteResourcesDeleter;
	private:
		::Nanite::FResources	Resources;			// engine-owned lifecycle from here on
		FBoxSphereBounds3f		Bounds;
		std::atomic<bool>		bIsInitialized;		// same semantics/comment as tier A
		// Cached because the engine empties RootData after root upload in non-editor builds — reading
		// RootData.Num() post-registration would flip HasValidData() to false and proxies would drop the mesh.
		bool					bHasData = false;

#if !UE_BUILD_SHIPPING
		FString AuditOwnerName;		// register/unregister diagnostic pairing (same role as tier A)
#endif

		FRealtimeMeshNaniteResources(::Nanite::FResources&& InResources, TArray<uint8>&& InStreamingPagesData, const FBoxSphereBounds3f& InBounds)
			: Resources(MoveTemp(InResources))
			, Bounds(InBounds)
			, bIsInitialized(false)
		{
			// One unavoidable copy (bulk data has no adopt-buffer API); steady-state memory is the same once
			// the source TArray frees. BULKDATA_ForceInlinePayload is belt-and-braces on never-serialized
			// runtime bulk data (the data is memory-resident by construction).
			if (InStreamingPagesData.Num() > 0)
			{
				Resources.StreamablePages.Lock(LOCK_READ_WRITE);
				void* Dst = Resources.StreamablePages.Realloc(InStreamingPagesData.Num());
				FMemory::Memcpy(Dst, InStreamingPagesData.GetData(), InStreamingPagesData.Num());
				Resources.StreamablePages.Unlock();
				Resources.StreamablePages.SetBulkDataFlags(BULKDATA_ForceInlinePayload);
			}
			bHasData = Resources.RootData.Num() > 0;
			// The incoming FResources may carry live registration identity (CreateFromCopy from a
			// registered cooked resource, e.g. the static-mesh converter) — reset it like tier A's ctor
			// does, or InitResources would hand the streaming manager a stale RuntimeResourceID.
			ClearRuntimeState();
		}

		FRealtimeMeshNaniteResources(const FRealtimeMeshNaniteResources& Other)
			: Resources(Other.Resources)	// FResources copy carries bulk data with it
			, Bounds(Other.Bounds)
			, bIsInitialized(false)
			, bHasData(Other.bHasData)
		{
			// Copying a registered (initialized) instance is a lifetime bug — guard as tier A does.
			check(!Other.bIsInitialized.load(std::memory_order_relaxed));
			// Match tier A: a released-then-cloned instance must not inherit stale registration identity.
			ClearRuntimeState();
		}

		FRealtimeMeshNaniteResources(FRealtimeMeshNaniteResources&& Other)
			: Resources(MoveTemp(Other.Resources))
			, Bounds(Other.Bounds)
			, bIsInitialized(false)
			, bHasData(Other.bHasData)
		{
			check(!Other.bIsInitialized.load(std::memory_order_relaxed));
			ClearRuntimeState();
		}

	public:
		FRealtimeMeshNaniteResources()
			: bIsInitialized(false)
		{
			ClearRuntimeState();
		}

		FRealtimeMeshNaniteResources& operator=(const FRealtimeMeshNaniteResources&) = delete;
		FRealtimeMeshNaniteResources& operator=(FRealtimeMeshNaniteResources&&) = delete;

		static FRealtimeMeshNaniteResourcesPtr Create(::Nanite::FResources&& InResources, TArray<uint8>&& InStreamingPagesData, const FBoxSphereBounds3f& InBounds)
		{
			return FRealtimeMeshNaniteResourcesPtr(new FRealtimeMeshNaniteResources(MoveTemp(InResources), MoveTemp(InStreamingPagesData), InBounds));
		}

		// Simpler than tier A: copy the FResources whole (bulk data copies with it) — no bulk→TArray unpacking.
		static FRealtimeMeshNaniteResourcesPtr CreateFromCopy(const ::Nanite::FResources& InResources, const FBoxSphereBounds3f& InBounds)
		{
			::Nanite::FResources ResourcesCopy = InResources;
			return FRealtimeMeshNaniteResourcesPtr(new FRealtimeMeshNaniteResources(MoveTemp(ResourcesCopy), TArray<uint8>(), InBounds));
		}

		FRealtimeMeshNaniteResourcesPtr Clone() const
		{
			return FRealtimeMeshNaniteResourcesPtr(new FRealtimeMeshNaniteResources(*this));
		}

		FRealtimeMeshNaniteResourcesPtr Consume()
		{
			return FRealtimeMeshNaniteResourcesPtr(new FRealtimeMeshNaniteResources(MoveTemp(*this)));
		}

		bool HasValidData() const { return bHasData; }		// NOT RootData.Num(): engine empties it post-upload

		const FBoxSphereBounds3f& GetBounds() const { return Bounds; }

		void InitResources(const UObject* OwningMesh)
		{
			check(IsValid(OwningMesh));
			if (bIsInitialized.load(std::memory_order_relaxed))
			{
				return;
			}

			if (!HasValidData())
			{
				UE_LOG(LogRealtimeMesh, Warning, TEXT("Attempting to initialize Nanite resources with invalid data for mesh: %s"),
					OwningMesh ? *OwningMesh->GetName() : TEXT("NULL"));
				return;
			}

			if (Resources.HierarchyNodes.IsEmpty())
			{
				UE_LOG(LogRealtimeMesh, Warning, TEXT("Missing hierarchy nodes for Nanite mesh: %s"),
					OwningMesh ? *OwningMesh->GetName() : TEXT("NULL"));
				return;
			}

			if (Resources.NumClusters == 0)
			{
				UE_LOG(LogRealtimeMesh, Warning, TEXT("Nanite mesh has 0 clusters for mesh: %s - this may cause render issues"),
					OwningMesh ? *OwningMesh->GetName() : TEXT("NULL"));
			}

			Resources.InitResources(OwningMesh);
			bIsInitialized.store(true, std::memory_order_relaxed);
#if !UE_BUILD_SHIPPING
			AuditOwnerName = OwningMesh ? OwningMesh->GetName() : FString(TEXT("NULL"));
#endif
			UE_LOG(LogRealtimeMesh, Verbose, TEXT("Nanite InitResources (register) for mesh: %s"),
				OwningMesh ? *OwningMesh->GetName() : TEXT("NULL"));
		}

		bool ReleaseResources()
		{
			if (!bIsInitialized.load(std::memory_order_relaxed))
			{
				return false;
			}
#if !UE_BUILD_SHIPPING
			const uint32 DiagRuntimeResourceID = Resources.RuntimeResourceID;
#endif
			const bool bResult = Resources.ReleaseResources();
			bIsInitialized.store(false, std::memory_order_relaxed);
#if !UE_BUILD_SHIPPING
			UE_LOG(LogRealtimeMesh, Verbose, TEXT("Nanite ReleaseResources (unregister) for mesh: %s [RuntimeResourceID=%u]"),
				*AuditOwnerName, DiagRuntimeResourceID);
#else
			UE_LOG(LogRealtimeMesh, Verbose, TEXT("Nanite ReleaseResources (unregister)"));
#endif
			return bResult;
		}

		FRealtimeMeshNaniteEngineResources* GetNaniteProvider() { return &Resources; }
		const FRealtimeMeshNaniteEngineResources* GetNaniteProvider() const { return &Resources; }

		void ClearRuntimeState()
		{
			if (!ensure(!bIsInitialized.load(std::memory_order_relaxed)))
			{
				return;
			}

			static const ::Nanite::FResources NullResources;
			Resources.RuntimeResourceID = NullResources.RuntimeResourceID;
			Resources.HierarchyOffset = NullResources.HierarchyOffset;
			Resources.RootPageIndex = NullResources.RootPageIndex;
#if RMC_ENGINE_BELOW_5_8
			// 5.8 removed FResources::ImposterIndex (Nanite imposters dropped); the proxy guards its reads
			// the same way (RealtimeMeshNaniteProxy.cpp).
			Resources.ImposterIndex = NullResources.ImposterIndex;
#endif
			Resources.NumHierarchyNodes = NullResources.NumHierarchyNodes;
			Resources.NumResidentClusters = NullResources.NumResidentClusters;
			Resources.PersistentHash = NullResources.PersistentHash;
#if RMC_ENGINE_ABOVE_5_6
			Resources.AssemblyTransformOffset = NullResources.AssemblyTransformOffset;
			Resources.NumHierarchyDwords = NullResources.NumHierarchyDwords;
#endif
#if RMC_ENGINE_ABOVE_5_8
			// 5.8 added the NumAssemblyTransforms runtime field alongside AssemblyTransformOffset. Reset it
			// with the other runtime identity fields so a CreateFromCopy of a registered resource can't leave
			// a stale count; RMC meshes carry no assembly transforms, so NullResources' 0 matches
			// AssemblyTransforms.Num().
			Resources.NumAssemblyTransforms = NullResources.NumAssemblyTransforms;
#endif
		}
	};

#else
	// ============================================================================================
	// Tier C — runtime Nanite unavailable (stock engine outside the validated GPU-layout window).
	// ============================================================================================
	// Same public API; everything degrades to null/no-op. ShouldUseNanite() and the builder entry points
	// also gate on RMC_NANITE_AVAILABLE, so components fall back to the normal non-Nanite RMC path.
	struct FRealtimeMeshNaniteResources
	{
		friend struct FRealtimeMeshNaniteResourcesDeleter;
	private:
		FBoxSphereBounds3f Bounds = FBoxSphereBounds3f(ForceInit);

		static void LogUnavailableOnce()
		{
			static bool bLogged = false;
			if (!bLogged)
			{
				bLogged = true;
				UE_LOG(LogRealtimeMesh, Warning,
					TEXT("Runtime Nanite requires UE 5.7 or the provider engine fork; Nanite resources are unavailable on this engine."));
			}
		}

	public:
		FRealtimeMeshNaniteResources() = default;
		FRealtimeMeshNaniteResources(const FRealtimeMeshNaniteResources&) = delete;
		FRealtimeMeshNaniteResources(FRealtimeMeshNaniteResources&&) = delete;
		FRealtimeMeshNaniteResources& operator=(const FRealtimeMeshNaniteResources&) = delete;
		FRealtimeMeshNaniteResources& operator=(FRealtimeMeshNaniteResources&&) = delete;

		static FRealtimeMeshNaniteResourcesPtr Create(::Nanite::FResources&&, TArray<uint8>&&, const FBoxSphereBounds3f&)
		{
			LogUnavailableOnce();
			return nullptr;
		}
		static FRealtimeMeshNaniteResourcesPtr CreateFromCopy(const ::Nanite::FResources&, const FBoxSphereBounds3f&)
		{
			LogUnavailableOnce();
			return nullptr;
		}
		FRealtimeMeshNaniteResourcesPtr Clone() const { return nullptr; }
		FRealtimeMeshNaniteResourcesPtr Consume() { return nullptr; }

		bool HasValidData() const { return false; }
		const FBoxSphereBounds3f& GetBounds() const { return Bounds; }

		void InitResources(const UObject*) {}
		bool ReleaseResources() { return false; }
		void ClearRuntimeState() {}

		FRealtimeMeshNaniteEngineResources* GetNaniteProvider() { return nullptr; }
		const FRealtimeMeshNaniteEngineResources* GetNaniteProvider() const { return nullptr; }
	};

#endif // tier selection


	inline void FRealtimeMeshNaniteResourcesDeleter::Destroy(FRealtimeMeshNaniteResources* Resources)
	{
		if (Resources)
		{
			Resources->ReleaseResources();

			ENQUEUE_RENDER_COMMAND(DestroyRealtimeMeshNaniteResources)(
				[Resources](FRHICommandListImmediate&)
				{
					delete Resources;
				}
			);
		}
	}

	// Nanite resource-sharing: wrap a resources instance in a TSharedPtr carrying the custom deleter, so
	// the last reference triggers ReleaseResources() + the RT-deferred delete. This lets a single live,
	// registered instance be shared (refcount bump) across the game-thread master and every proxy version
	// instead of being deep-cloned on each proxy recreate. Tier-agnostic (deleter + TSharedPtr conversion).
	inline TSharedPtr<FRealtimeMeshNaniteResources> MakeShareableNaniteResources(FRealtimeMeshNaniteResourcesPtr&& InResources)
	{
		if (!InResources.IsValid())
		{
			return nullptr;
		}
		return MakeShareable(InResources.Release(), [](FRealtimeMeshNaniteResources* Resources)
		{
			FRealtimeMeshNaniteResourcesDeleter::Destroy(Resources);
		});
	}
}
