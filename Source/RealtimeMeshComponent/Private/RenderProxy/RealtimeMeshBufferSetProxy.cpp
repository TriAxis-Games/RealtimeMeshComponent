// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#include "RenderProxy/RealtimeMeshBufferSetProxy.h"

#include "RealtimeMeshComponentModule.h"
#include "Data/RealtimeMeshShared.h"
#include "RenderProxy/RealtimeMeshLODProxy.h"
#include "MaterialDomain.h"
#include "RenderProxy/RealtimeMeshSectionProxy.h"
#include "RenderProxy/RealtimeMeshVertexFactory.h"
#include "Materials/Material.h"
#if RHI_RAYTRACING
#include "RayTracingGeometry.h"
#include "RayTracingGeometryManagerInterface.h"
#endif

namespace RealtimeMesh
{
	FRealtimeMeshBufferSetProxy::FRealtimeMeshBufferSetProxy(const FRealtimeMeshContextRef& InContext, const FRealtimeMeshBufferSetKey& InKey)
		: Context(InContext)
		, Key(InKey)
		, VertexFactory(Context->CreateVertexFactory())
		, bVertexFactoryDirty(false)
	{
	}

	FRealtimeMeshBufferSetProxy::~FRealtimeMeshBufferSetProxy()
	{
		check(IsInRenderingThread());
		Reset();
	}

	TSharedRef<FRealtimeMeshBufferSetProxy> FRealtimeMeshBufferSetProxy::Clone() const
	{
		// Allocate via shared resources so subclasses (Nanite, etc.) get the correct type.
		const TSharedRef<FRealtimeMeshBufferSetProxy> Cloned = Context->CreateSectionGroupProxy(Key);
		Cloned->Config = Config;
		// VertexFactory is shared by pointer. UpdateCachedState allocates a fresh
		// VertexFactory whenever reinitialization is required (see that method), so the
		// snapshot's pointer never observes mutations from subsequent batches.
		Cloned->VertexFactory = VertexFactory;
		// Streams (TMap<key, TSharedPtr<FRealtimeMeshGPUBuffer>>) are likewise shared.
		// CreateOrUpdateStream replaces map entries with new buffers rather than mutating
		// existing ones, so sharing the map is safe — but we shallow-copy the map itself
		// so workspace add/remove operations affect the workspace SG only.
		Cloned->Streams = Streams;
		Cloned->DrawMask = DrawMask;
		Cloned->bVertexFactoryDirty = false;
		// Carry the stream generation so a clone that made no stream change compares
		// equal to the geometry it shares below (PROXY-F11).
		Cloned->StreamGeneration = StreamGeneration;
#if RHI_RAYTRACING
		// Share the ray tracing geometry pointer. A rebuild allocates a fresh TSharedPtr, so the
		// snapshot's pointer never observes mutation.
		Cloned->RayTracingGeometry = RayTracingGeometry;
		// The signature must travel with the shared geometry it describes; otherwise a clone
		// starts with an empty signature and the rebuild-skip compare can never match (PROXY-F11).
		Cloned->LastRayTracingSignature = LastRayTracingSignature;
#endif
		// ParentLOD is intentionally left null here: Clone() doesn't know which LOD will
		// own the clone. The owning LOD re-anchors it when the BS is COW'd into that LOD's
		// workspace — in FindMutableBufferSet (explicit COW) or in the UpdateCachedState
		// touched-BS sweep (for BSs first cloned via a section mutation). Note that
		// FRealtimeMeshLODProxy::Clone is a shallow clone that shares the BufferSets array
		// and does not itself invoke this Clone() or set ParentLOD.
		return Cloned;
	}

	FRealtimeMeshSectionProxyConstPtr FRealtimeMeshBufferSetProxy::GetSection(const FRealtimeMeshSectionKey& SectionKey) const
	{
		check(SectionKey.IsPartOf(Key));
		// Sections live on the LOD post-flattening. Resolve through ParentLOD.
		// PROXY-F13: a dead anchor means this BS was shared unchanged into a newer
		// version whose predecessor LOD has since died. ensure() so it's loud, then
		// safely return null — the section still exists on the live LOD.
		const auto LODPinned = ParentLOD.Pin();
		if (!ensure(LODPinned.IsValid()))
		{
			return FRealtimeMeshSectionProxyConstPtr();
		}
		return LODPinned->GetSection(SectionKey);
	}

	TSharedPtr<FRealtimeMeshGPUBuffer> FRealtimeMeshBufferSetProxy::GetStream(const FRealtimeMeshStreamKey& StreamKey) const
	{
		return Streams.FindRef(StreamKey);
	}

	FRayTracingGeometry* FRealtimeMeshBufferSetProxy::GetRayTracingGeometry()
	{
#if RHI_RAYTRACING
		return RayTracingGeometry.IsValid() && RayTracingGeometry->IsValid()? RayTracingGeometry.Get() : nullptr;
#else
		return nullptr;
#endif
	}

	void FRealtimeMeshBufferSetProxy::UpdateConfig(const FRealtimeMeshBufferSetConfig& NewConfig)
	{
		if (Config != NewConfig)
		{
			Config = NewConfig;
			bVertexFactoryDirty = true;
		}
	}

	void FRealtimeMeshBufferSetProxy::CreateSectionIfNotExists(const FRealtimeMeshSectionKey& SectionKey)
	{
		check(SectionKey.IsPartOf(Key));
		// PROXY-F13: ensure() + no-op on a dead anchor rather than silently dropping
		// the create (see ParentLOD doc in the header).
		const auto LODPinned = ParentLOD.Pin();
		if (!ensure(LODPinned.IsValid()))
		{
			return;
		}
		// Wire the new section's BufferSetIndex to this group's slot — preserves
		// the historical implicit binding (section created via SG belongs to SG).
		LODPinned->CreateSectionIfNotExists(SectionKey, Key.Index());
	}

	void FRealtimeMeshBufferSetProxy::RemoveSection(const FRealtimeMeshSectionKey& SectionKey)
	{
		check(SectionKey.IsPartOf(Key));
		// PROXY-F13: ensure() + no-op on a dead anchor rather than silently dropping
		// the removal (see ParentLOD doc in the header).
		const auto LODPinned = ParentLOD.Pin();
		if (!ensure(LODPinned.IsValid()))
		{
			return;
		}
		LODPinned->RemoveSection(SectionKey);
		bVertexFactoryDirty = true;
	}

	void FRealtimeMeshBufferSetProxy::CreateOrUpdateStream(FRHICommandListBase& RHICmdList, const FRealtimeMeshSectionGroupStreamUpdateDataRef& InStream)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(FRealtimeMeshBufferSetProxy::CreateOrUpdateStream);

		// If we didn't create the buffers async, create them now
		InStream->FinalizeInitialization(RHICmdList);

		check(InStream->GetBuffer().IsValid() && InStream->GetBuffer()->GetSize() > 0);

		// Replace any existing stream entry by reassigning the TSharedPtr.
		// COW invariant: snapshots may share the previous buffer's TSharedPtr; explicit
		// ReleaseUnderlyingResource here would corrupt their view. The underlying RHI
		// resource is freed via the FRealtimeMeshRenderResourceDeleter once the last
		// snapshot drops its reference.
		Streams.Remove(InStream->GetStreamKey());

		// Create a new GPU buffer using the RenderResource deleter (see the deleter doc
		// in RealtimeMeshCore.h).
		TSharedPtr<FRealtimeMeshGPUBuffer> GPUBuffer = InStream->GetStreamKey().GetStreamType() == ERealtimeMeshStreamType::Vertex
							? StaticCastSharedPtr<FRealtimeMeshGPUBuffer>(TSharedPtr<FRealtimeMeshVertexBuffer>(MakeShareable(new FRealtimeMeshVertexBuffer(InStream->GetBufferLayout()), FRealtimeMeshRenderResourceDeleter<FRealtimeMeshVertexBuffer>())))
							: StaticCastSharedPtr<FRealtimeMeshGPUBuffer>(TSharedPtr<FRealtimeMeshIndexBuffer>(MakeShareable(new FRealtimeMeshIndexBuffer(InStream->GetBufferLayout()), FRealtimeMeshRenderResourceDeleter<FRealtimeMeshIndexBuffer>())));

		GPUBuffer->InitializeResources(RHICmdList, InStream);
		Streams.Add(InStream->GetStreamKey(), GPUBuffer);

		check(GPUBuffer);
		check(GPUBuffer->IsResourceInitialized());

		bVertexFactoryDirty = true;
		// PROXY-F11 (H2): a new buffer object was installed for this stream — invalidate any
		// pointer-identity match against the previously-built ray-tracing geometry.
		++StreamGeneration;
	}

	bool FRealtimeMeshBufferSetProxy::CanUpdateStreamInPlace(const FRealtimeMeshSectionGroupStreamUpdateDataRef& UpdateData, int32 ElementOffset, int32 NumElements) const
	{
		const TSharedPtr<FRealtimeMeshGPUBuffer>* Found = Streams.Find(UpdateData->GetStreamKey());
		if (!Found || !Found->IsValid())
		{
			return false;
		}

		const FRealtimeMeshGPUBuffer& GPUBuffer = *Found->Get();

		// Structure must be unchanged: same layout and same total element count as what's
		// already resident. Only the element values (whole stream or a sub-range) differ.
		if (GPUBuffer.GetBufferLayout() != UpdateData->GetBufferLayout() || UpdateData->GetNumElements() != GPUBuffer.Num())
		{
			return false;
		}

		return GPUBuffer.CanUpdateInPlace(UpdateData->GetBufferLayout(), ElementOffset, NumElements);
	}

	bool FRealtimeMeshBufferSetProxy::UpdateStreamInPlace(FRHICommandListBase& RHICmdList, const FRealtimeMeshSectionGroupStreamUpdateDataRef& UpdateData, int32 ElementOffset, int32 NumElements)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(FRealtimeMeshBufferSetProxy::UpdateStreamInPlace);

		const TSharedPtr<FRealtimeMeshGPUBuffer>* Found = Streams.Find(UpdateData->GetStreamKey());
		if (!Found || !Found->IsValid())
		{
			return false;
		}

		const void* SrcData = UpdateData->GetResource() ? UpdateData->GetResource()->GetResourceData() : nullptr;
		if (!SrcData)
		{
			return false;
		}

		// Intentionally does NOT set bVertexFactoryDirty: the FRHIBuffer handle and its SRV are
		// unchanged, so the existing vertex factory keeps working as-is without a reinit.
		const bool bUpdated = (*Found)->UpdateInPlace(RHICmdList, SrcData, ElementOffset, NumElements);

#if RHI_RAYTRACING
		// PROXY-F11 case 2 (see the invalidation contract on FRealtimeMeshRayTracingBuildSignature):
		// the in-place fast path skips UpdatedCachedState, so the publish-path signature compare never
		// runs for it — and it couldn't help anyway, because an in-place write reuses the *same*
		// position FRHIBuffer object (only its contents moved), leaving every signature field
		// unchanged. So a contents change is an EXPLICIT refresh trigger, not a compare. Only the
		// Position stream feeds the geometry the BLAS is built from; non-Position in-place writes
		// (color/texcoord/...) leave the acceleration structure valid, and index contents can't be
		// updated in place at all — so nothing but a successful Position write needs a refresh.
		// The shared helper is the single definition of that refresh.
		if (bUpdated && UpdateData->GetStreamKey() == FRealtimeMeshStreams::Position)
		{
			RefreshRayTracingGeometryInPlace();
		}
#endif // RHI_RAYTRACING

		return bUpdated;
	}

	void FRealtimeMeshBufferSetProxy::RegisterGPUStream(FRHICommandListBase& RHICmdList, const TSharedRef<FRealtimeMeshGPUStream>& InStream)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(FRealtimeMeshBufferSetProxy::RegisterGPUStream);

		check(InStream->GetBuffer().IsValid());

		// Mirror CreateOrUpdateStream's COW invariants: any prior snapshot may share
		// the old buffer's TSharedPtr; freeing it here would corrupt them. Just drop
		// our reference — the underlying RHI release happens via the deleter when
		// the last snapshot releases.
		Streams.Remove(InStream->GetStreamKey());

		TSharedPtr<FRealtimeMeshGPUBuffer> GPUBuffer = InStream->GetStreamKey().GetStreamType() == ERealtimeMeshStreamType::Vertex
			? StaticCastSharedPtr<FRealtimeMeshGPUBuffer>(TSharedPtr<FRealtimeMeshVertexBuffer>(MakeShareable(new FRealtimeMeshVertexBuffer(InStream->GetLayout()), FRealtimeMeshRenderResourceDeleter<FRealtimeMeshVertexBuffer>())))
			: StaticCastSharedPtr<FRealtimeMeshGPUBuffer>(TSharedPtr<FRealtimeMeshIndexBuffer>(MakeShareable(new FRealtimeMeshIndexBuffer(InStream->GetLayout()), FRealtimeMeshRenderResourceDeleter<FRealtimeMeshIndexBuffer>())));

		GPUBuffer->InitializeResourcesFromGPUStream(RHICmdList, *InStream);
		Streams.Add(InStream->GetStreamKey(), GPUBuffer);

		check(GPUBuffer);
		check(GPUBuffer->IsResourceInitialized());

		bVertexFactoryDirty = true;
		// PROXY-F11 (H2): RegisterGPUStream can reinstall a stream at the SAME FRHIBuffer address
		// with new GPU-written contents; bump the generation so the ray-tracing signature does not
		// mistake that for an unchanged buffer and wrongly skip the rebuild.
		++StreamGeneration;
	}

	void FRealtimeMeshBufferSetProxy::RemoveStream(const FRealtimeMeshStreamKey& StreamKey)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(FRealtimeMeshBufferSetProxy::RemoveStream);

		if (Streams.Contains(StreamKey))
		{
			Streams.Remove(StreamKey);
			bVertexFactoryDirty = true;
			// PROXY-F11 (H2): stream set changed — invalidate the ray-tracing signature.
			++StreamGeneration;
		}
	}

	bool FRealtimeMeshBufferSetProxy::InitializeMeshBatch(FMeshBatch& MeshBatch, FRealtimeMeshResourceReferenceList& Resources, bool bIsLocalToWorldDeterminantNegative, bool bWantsDepthOnly) const
	{
		if (ensure(VertexFactory && VertexFactory->IsInitialized()) == false)
		{
			return false;
		}
		Resources.AddResource(VertexFactory);
		VertexFactory->GatherVertexBufferResources(Resources);

		FMeshBatchElement& BatchElement = MeshBatch.Elements[0];
		BatchElement.IndexBuffer = &VertexFactory->GetIndexBuffer(bWantsDepthOnly, bIsLocalToWorldDeterminantNegative, Resources);
		MeshBatch.VertexFactory = VertexFactory.Get();

		MeshBatch.LODIndex = Key.LOD();
		MeshBatch.SegmentIndex = 0;

		MeshBatch.ReverseCulling = bIsLocalToWorldDeterminantNegative;
		MeshBatch.bDisableBackfaceCulling = false;

		MeshBatch.CastShadow = DrawMask.ShouldRenderShadow();
		MeshBatch.bUseForMaterial = true;
		MeshBatch.bUseForDepthPass = true;
		MeshBatch.bUseAsOccluder = true;

		MeshBatch.Type = VertexFactory->GetPrimitiveType();
		MeshBatch.DepthPriorityGroup = SDPG_World;

		MeshBatch.bCanApplyViewModeOverrides = false;
		MeshBatch.bDitheredLODTransition = true;

		MeshBatch.bRenderToVirtualTexture = false;
		MeshBatch.RuntimeVirtualTextureMaterialType = 0;
		MeshBatch.bOverlayMaterial = false;

#if RHI_RAYTRACING
		MeshBatch.CastRayTracedShadow = MeshBatch.CastShadow;
#endif

		MeshBatch.bUseForWaterInfoTextureDepth = false;
		MeshBatch.bUseForLumenSurfaceCacheCapture = false;

#if UE_ENABLE_DEBUG_DRAWING
		MeshBatch.VisualizeLODIndex = MeshBatch.LODIndex;
#endif

		return true;
	}

	void FRealtimeMeshBufferSetProxy::UpdateCachedState(FRHICommandListBase& RHICmdList)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(FRealtimeMeshBufferSetProxy::UpdateCachedState);

		// Section storage lives on the LOD; this method only handles VertexFactory
		// reinitialization, which is needed whenever the stream buffer objects change
		// (bVertexFactoryDirty) or no valid factory exists yet.
		//
		// A section range move with unchanged buffers deliberately does NOT force a
		// reinit: FRealtimeMeshLocalVertexFactory's ValidRange is derived solely from the
		// bound buffer extents, so a range change cannot invalidate factory state. Range
		// changes are instead reflected by the section's own DrawMask recompute (its
		// IsValidStreamRange check) and by the ray-tracing build signature.
		const bool bNeedsFactoryInitialization = bVertexFactoryDirty || !VertexFactory.IsValid() || !VertexFactory->IsInitialized();

		if (bNeedsFactoryInitialization)
		{
			// Allocate a fresh VF so any snapshot still referencing the previous one
			// continues to render the prior state cleanly (COW invariant).
			VertexFactory = Context->CreateVertexFactory();
			VertexFactory->Initialize(RHICmdList, Streams);
		}

		bVertexFactoryDirty = false;
		// DrawMask is finalized later via SetDrawMaskFromActiveSections, after the LOD
		// has computed which sections are active for this group.
	}

	void FRealtimeMeshBufferSetProxy::SetDrawMaskFromActiveSections(FRHICommandListBase& RHICmdList, const TArray<int32>& ActiveSectionIndices, const TArray<TCowPtr<FRealtimeMeshSectionProxy>>& AllSections)
	{
		DrawMask = FRealtimeMeshDrawMask();
		for (const int32 SectionIndex : ActiveSectionIndices)
		{
			if (AllSections.IsValidIndex(SectionIndex))
			{
				DrawMask |= AllSections[SectionIndex]->GetDrawMask();
			}
		}

		if (DrawMask.HasAnyFlags())
		{
			DrawMask.SetFlag(Config.DrawType == ERealtimeMeshSectionDrawType::Static ? ERealtimeMeshDrawMask::DrawStatic : ERealtimeMeshDrawMask::DrawDynamic);
		}

#if RHI_RAYTRACING
		// A buffer set is re-evaluated every time it is *touched*, but the BLAS only depends on the
		// position/index buffer objects and the emitted segment set. Derive the build signature and
		// skip the from-scratch rebuild when it matches what the live geometry was already built
		// from — the common material/config-only edit changes none of it (PROXY-F11).
		//
		// NOTE: bVertexFactoryDirty is cleared in UpdateCachedState (step 1) before this (step 4)
		// runs, so it cannot be read here — the persistent StreamGeneration counter carries the
		// "buffer object replaced" signal into the signature instead.
		const FRealtimeMeshRayTracingBuildPlan Plan = ComputeRayTracingBuildPlan(ActiveSectionIndices, AllSections);

		if (RayTracingGeometry.IsValid() && RayTracingGeometry->IsValid()
			&& Plan.Signature.DescribesBuiltGeometry()
			&& Plan.Signature == LastRayTracingSignature)
		{
			// Inputs unchanged and a valid BLAS already exists — reuse it. DrawMask was reset above,
			// so re-assert the RayTracing flag (H3).
			DrawMask.SetFlag(ERealtimeMeshDrawMask::RayTracing);
			return;
		}

		if (BuildRayTracingGeometryFromPlan(RHICmdList, Plan))
		{
			DrawMask.SetFlag(ERealtimeMeshDrawMask::RayTracing);
		}
		LastRayTracingSignature = Plan.Signature;
#endif
	}

	void FRealtimeMeshBufferSetProxy::Reset()
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(FRealtimeMeshBufferSetProxy::Reset);

#if RHI_RAYTRACING
		// Drop our reference; if the snapshot still holds the geometry it stays alive
		// until that snapshot is released. The TSharedPtr destructor handles the RHI
		// release path when the last reference goes away.
		RayTracingGeometry.Reset();
		// PROXY-F11 (H1): the geometry is gone, so its signature must not match a future build.
		LastRayTracingSignature = FRealtimeMeshRayTracingBuildSignature();
#endif
		// VertexFactory and Streams may be shared with published snapshots. We must
		// drop our references but not forcibly release the underlying RHI resources —
		// those are owned by reference count and released by the deleter when the last
		// snapshot drops.
		VertexFactory.Reset();
		Streams.Empty();

		DrawMask = FRealtimeMeshDrawMask();
		// Note: section storage is on the LOD, so we don't iterate / Reset sections here.
	}

#if RHI_RAYTRACING
	FRealtimeMeshRayTracingBuildPlan FRealtimeMeshBufferSetProxy::ComputeRayTracingBuildPlan(const TArray<int32>& ActiveSectionIndices, const TArray<TCowPtr<FRealtimeMeshSectionProxy>>& AllSections) const
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(FRealtimeMeshBufferSetProxy::ComputeRayTracingBuildPlan);

		FRealtimeMeshRayTracingBuildPlan Plan;
		Plan.Signature.StreamGeneration = StreamGeneration;
		Plan.Signature.bRayTracingEnabled = IsRayTracingEnabled();

		bool bShouldGenerateRayTracingGeometry = DrawMask.HasAnyFlags() && VertexFactory.IsValid() && Plan.Signature.bRayTracingEnabled;

		// We need to check if the sections are contiguous with no gaps and using the entire index buffer...
		// If it is not then we'd need to allocate a ray tracing index buffer and pack the active sections down into it.
		// Collect each rendering section's half-open [begin, end) index range, then decide
		// contiguity with one sort and a single linear adjacency sweep — O(N log N) instead
		// of the former TRangeSet's O(N^2) insertion plus separate min/max bound scans.
		TArray<TPair<int32, int32>, TInlineAllocator<16>> SectionRanges;
		SectionRanges.Reserve(ActiveSectionIndices.Num());
		for (const int32 SectionIndex : ActiveSectionIndices)
		{
			if (!AllSections.IsValidIndex(SectionIndex)) continue;
			const TCowPtr<FRealtimeMeshSectionProxy>& Section = AllSections[SectionIndex];
			if (Section->GetDrawMask().ShouldRender() && Section->GetDrawMask().ShouldRenderMainPass())
			{
				SectionRanges.Emplace(Section->GetStreamRange().GetMinIndex(), Section->GetStreamRange().GetMaxIndex() + 1);
			}
		}

		int32 MinIndex = 0;
		int32 MaxIndex = 0;
		bool bAreSectionsContiguous = false;
		if (SectionRanges.Num() > 0)
		{
			SectionRanges.Sort([](const TPair<int32, int32>& A, const TPair<int32, int32>& B) { return A.Key < B.Key; });

			MinIndex = SectionRanges[0].Key;
			int32 RunningEnd = SectionRanges[0].Value;
			bAreSectionsContiguous = true;
			for (int32 RangeIndex = 1; RangeIndex < SectionRanges.Num(); ++RangeIndex)
			{
				// A begin past the running merged end leaves a gap in the coverage of
				// [MinIndex, MaxIndex+1). Overlapping/adjacent ranges keep it contiguous.
				if (SectionRanges[RangeIndex].Key > RunningEnd)
				{
					bAreSectionsContiguous = false;
				}
				RunningEnd = FMath::Max(RunningEnd, SectionRanges[RangeIndex].Value);
			}
			MaxIndex = RunningEnd - 1;
		}

		if (!bAreSectionsContiguous)
		{
			// Only a genuine failure warrants a warning: sections exist but don't cover the
			// index buffer contiguously. An empty section set is the normal "nothing to
			// ray-trace" case and must not spam the log every batch.
			if (SectionRanges.Num() > 0)
			{
				UE_LOG(LogRealtimeMesh, Warning, TEXT("Unable to create ray tracing accelleration structures. Some triangles in buffer are unaccounted for in sections."));
			}
			bShouldGenerateRayTracingGeometry = false;
		}

		if (!bShouldGenerateRayTracingGeometry)
		{
			// Signature keeps its null geometry buffers → DescribesBuiltGeometry() is false, so the
			// gate never treats an empty plan as a reusable match.
			Plan.bShouldGenerate = false;
			return Plan;
		}

		Plan.PositionStream = StaticCastSharedPtr<FRealtimeMeshVertexBuffer>(Streams.FindChecked(FRealtimeMeshStreams::Position));
		Plan.IndexStream = StaticCastSharedPtr<FRealtimeMeshIndexBuffer>(Streams.FindChecked(FRealtimeMeshStreams::Triangles));
		Plan.bShouldGenerate = true;

		Plan.Signature.PositionBuffer = Plan.PositionStream->VertexBufferRHI;
		Plan.Signature.IndexBuffer = Plan.IndexStream->IndexBufferRHI;
		Plan.Signature.PositionNumVertices = Plan.PositionStream->Num();
		Plan.Signature.MinIndex = MinIndex;
		Plan.Signature.MaxIndex = MaxIndex;

		// The emitted segment set is the single source of truth for both the signature (equality
		// gate) and the actual build below — computed exactly once here so they cannot drift.
		for (const int32 SectionIndex : ActiveSectionIndices)
		{
			if (!AllSections.IsValidIndex(SectionIndex)) continue;
			const TCowPtr<FRealtimeMeshSectionProxy>& Section = AllSections[SectionIndex];
			if (Section->GetDrawMask().ShouldRender() && Section->GetDrawMask().ShouldRenderMainPass())
			{
				// FirstPrimitive is relative to IndexBufferOffset, which already skips MinIndex;
				// subtract MinIndex here so we don't apply it twice.
				const int32 FirstPrimitive = (Section->GetStreamRange().GetMinIndex() - MinIndex) / 3;
				const int32 NumPrimitives = Section->GetStreamRange().NumPrimitives(3);
				Plan.Signature.Segments.Add(TPair<int32, int32>(FirstPrimitive, NumPrimitives));
			}
		}

		return Plan;
	}

	bool FRealtimeMeshBufferSetProxy::BuildRayTracingGeometryFromPlan(FRHICommandListBase& RHICmdList, const FRealtimeMeshRayTracingBuildPlan& Plan)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(FRealtimeMeshBufferSetProxy::BuildRayTracingGeometryFromPlan);

		// Allocate a fresh ray tracing geometry. Any snapshot still holding the previous
		// TSharedPtr keeps the prior geometry alive until that snapshot is released.
		RayTracingGeometry.Reset();

		if (!Plan.bShouldGenerate)
		{
			return false;
		}

		check(VertexFactory->IsInitialized());
		check(Plan.PositionStream.IsValid() && Plan.IndexStream.IsValid());

		const FRealtimeMeshRayTracingBuildSignature& Sig = Plan.Signature;

		FRayTracingGeometryInitializer Initializer;
		Initializer.DebugName = *(Context->GetMeshName().ToString() + TEXT("_") + Key.ToString() + " RTGeometry");
		Initializer.OwnerName = Context->GetMeshName();

		Initializer.IndexBuffer = Plan.IndexStream->IndexBufferRHI;
		Initializer.IndexBufferOffset = Plan.IndexStream->IndexBufferRHI->GetStride() * Sig.MinIndex;
		Initializer.TotalPrimitiveCount = ((Sig.MaxIndex - Sig.MinIndex) + 1) / 3;
		Initializer.GeometryType = RTGT_Triangles;
		Initializer.bFastBuild = true;
		Initializer.bAllowUpdate = false;

		for (const TPair<int32, int32>& Seg : Sig.Segments)
		{
			FRayTracingGeometrySegment Segment;
			Segment.VertexBuffer = Plan.PositionStream->VertexBufferRHI;
			Segment.VertexBufferOffset = 0;
			Segment.MaxVertices = Plan.PositionStream->Num();
			Segment.FirstPrimitive = Seg.Key;
			Segment.NumPrimitives = Seg.Value;
			check(Segment.NumPrimitives > 0);
			Initializer.Segments.Add(Segment);
		}

		// Use the RenderResource deleter so shared geometries release on the
		// render thread when the final reference drops.
		RayTracingGeometry = MakeShareable(new FRayTracingGeometry(), FRealtimeMeshRenderResourceDeleter<FRayTracingGeometry>());
		RayTracingGeometry->SetInitializer(Initializer);
		RayTracingGeometry->InitResource(RHICmdList);

		check(RayTracingGeometry->GetRHI()->IsValid());

		return true;
	}

	void FRealtimeMeshBufferSetProxy::RefreshRayTracingGeometryInPlace()
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(FRealtimeMeshBufferSetProxy::RefreshRayTracingGeometryInPlace);

		// Case 2 of the invalidation contract: an in-place Position write changed the buffer's
		// contents but not the geometry it was built from. Re-run the acceleration-structure build
		// against those new positions on the SAME geometry object. A full Build (not a refit) is
		// used intentionally: it needs no bAllowUpdate flag (so RT build flags — and their memory/
		// perf cost — stay unchanged for meshes that never take the in-place path) and no
		// manually-managed scratch buffer, and it is correct regardless of how many in-place writes
		// occurred.
		//
		// LastRayTracingSignature is deliberately NOT touched here: an in-place refresh changes none
		// of the fields the signature captures (same PositionBuffer/IndexBuffer objects, same
		// StreamGeneration, same Segments/MinIndex/MaxIndex), so the cached signature still describes
		// the refreshed geometry. The next publish-path compare therefore still correctly skips when
		// nothing structural changed. This coherence holds by construction — there is nothing to
		// re-cache.
		//
		// The geometry-manager rebuild-request API (FRayTracingGeometry::GetRHI() /
		// IRayTracingGeometryManager / GRayTracingGeometryManager) is a 5.5+ construct, so before
		// 5.5 this is a no-op — the plugin's stated 5.4 minimum has no equivalent request path.
		if (RayTracingGeometry.IsValid()
			&& RayTracingGeometry->IsValid()
			&& RayTracingGeometry->GetRHI() != nullptr
			&& IsRayTracingEnabled()
			// The geometry manager asserts a geometry has no in-flight build request when a new one
			// is queued. A pending request (from the initial build, or from an earlier Position write
			// to this same buffer set in this batch) already rebuilds against the current contents, so
			// skip if one exists — the pending build will pick up these latest positions too.
			&& !RayTracingGeometry->HasPendingBuildRequest())
		{
			GRayTracingGeometryManager->RequestBuildAccelerationStructure(
				RayTracingGeometry.Get(), ERTAccelerationStructureBuildPriority::Normal);
		}
	}
#endif // RHI_RAYTRACING
}
