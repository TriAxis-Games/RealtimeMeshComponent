// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "RealtimeMeshGPUBuffer.h"
#include "RealtimeMeshProxyShared.h"
#include "RealtimeMeshVertexFactory.h"
#include "RealtimeMeshSectionProxy.h"
#include "Core/RealtimeMeshBufferSetConfig.h"

namespace RealtimeMesh
{
#if RHI_RAYTRACING
	// ============================================================================================
	// PROXY-F11 — the single invalidation contract for the ray-tracing BLAS.
	//
	// There are exactly two ways the acceleration structure can go out of sync with the mesh, and
	// each has ONE trigger. This is the authoritative enumeration; the code paths below implement it
	// and nothing else may rebuild the BLAS:
	//
	//   1. A geometry-affecting *structural* change  →  full rebuild (BuildRayTracingGeometryFromPlan).
	//      Detected by the signature below differing from LastRayTracingSignature. Captured triggers:
	//        - position or index buffer OBJECT replaced (grow/realloc)   → PositionBuffer/IndexBuffer
	//        - a stream reinstalled at the same address (RegisterGPUStream) → StreamGeneration bump
	//        - the active-section segment set / ranges changed (incl. a
	//          visibility toggle or a contiguity flip)                   → Segments / MinIndex / MaxIndex
	//        - vertex count changed                                      → PositionNumVertices
	//        - ray tracing toggled globally                              → bRayTracingEnabled
	//      A config/material-only edit touches NONE of these → identical signature → the rebuild is
	//      skipped and the existing geometry (and its RayTracing draw-mask flag) is reused.
	//
	//   2. An in-place Position *contents* write (the allocation-light fast path)  →  refresh in place
	//      (RefreshRayTracingGeometryInPlace: a rebuild request on the SAME geometry object).
	//      This deliberately CANNOT be caught by the signature — the FRHIBuffer object, segments, and
	//      generation are all unchanged; only the bytes moved. So it is an explicit trigger, not a
	//      compare. Because it changes no signature field, LastRayTracingSignature still describes the
	//      refreshed geometry afterward (coherent by construction — see RefreshRayTracingGeometryInPlace).
	//      In-place writes to non-Position streams (color/texcoord/...) leave the geometry valid and
	//      need no refresh; index CONTENTS can never be updated in place at all
	//      (FRealtimeMeshIndexBuffer inherits CanUpdateInPlace()==false), so any index change always
	//      falls back to the publish path and is caught by case 1.
	//
	// The struct itself is the identity of the inputs case 1 builds from. Kept intentionally
	// lightweight (raw pointers, no ref-counting) so it can be cached cheaply and copied across COW
	// clones.
	// ============================================================================================
	struct FRealtimeMeshRayTracingBuildSignature
	{
		// Raw FRHIBuffer pointers are identity keys, NOT ownership. This is safe against ABA
		// address recycling: the cached FRayTracingGeometry's FRayTracingGeometryInitializer
		// holds FBufferRHIRefs to these same buffers (see BuildRayTracingGeometryFromPlan), so
		// while a signature is live its buffers are kept alive and their addresses cannot be
		// reused underneath us. A ref-counted pointer here would instead keep dead buffers alive
		// past their natural lifetime — deliberately avoided.
		const FRHIBuffer* PositionBuffer = nullptr;
		const FRHIBuffer* IndexBuffer = nullptr;
		int32 PositionNumVertices = 0;
		int32 MinIndex = 0;
		int32 MaxIndex = 0;
		// H2: a stream object can be reinstalled at the same address with new GPU-written contents
		// (RegisterGPUStream), which pointer identity alone would miss. This counter — bumped on
		// every stream install/reinstall/remove — disambiguates that from a true no-op.
		uint32 StreamGeneration = 0;
		// H3: ray tracing can be toggled globally; when off the build is skipped entirely.
		bool bRayTracingEnabled = false;
		// One (FirstPrimitive, NumPrimitives) per emitted segment, in emission order — captures
		// the active-section set, their ranges, and the contiguity decision.
		TArray<TPair<int32, int32>> Segments;

		bool operator==(const FRealtimeMeshRayTracingBuildSignature& Other) const
		{
			return PositionBuffer == Other.PositionBuffer
				&& IndexBuffer == Other.IndexBuffer
				&& PositionNumVertices == Other.PositionNumVertices
				&& MinIndex == Other.MinIndex
				&& MaxIndex == Other.MaxIndex
				&& StreamGeneration == Other.StreamGeneration
				&& bRayTracingEnabled == Other.bRayTracingEnabled
				&& Segments == Other.Segments;
		}
		bool operator!=(const FRealtimeMeshRayTracingBuildSignature& Other) const { return !(*this == Other); }

		// A signature may short-circuit a rebuild only if it actually described a built geometry.
		// The empty / no-geometry case has null buffers and never matches, so it never skips.
		bool DescribesBuiltGeometry() const { return PositionBuffer != nullptr && IndexBuffer != nullptr && Segments.Num() > 0; }
	};

	// Transient per-call working set: the live streams to build from plus the lightweight
	// Signature to cache afterward. Holds TSharedPtrs, so it MUST stay a local — never cache this
	// (it would pin buffers); cache Signature instead.
	struct FRealtimeMeshRayTracingBuildPlan
	{
		bool bShouldGenerate = false;
		TSharedPtr<FRealtimeMeshVertexBuffer> PositionStream;
		TSharedPtr<FRealtimeMeshIndexBuffer> IndexStream;
		FRealtimeMeshRayTracingBuildSignature Signature;
	};
#endif // RHI_RAYTRACING

	class REALTIMEMESHCOMPONENT_API FRealtimeMeshBufferSetProxy : public TSharedFromThis<FRealtimeMeshBufferSetProxy>
	{
	private:
		const FRealtimeMeshContextRef Context;
		const FRealtimeMeshBufferSetKey Key;
		FRealtimeMeshBufferSetConfig Config;
		TSharedPtr<FRealtimeMeshVertexFactory> VertexFactory;
		FRealtimeMeshStreamProxyMap Streams;
#if RHI_RAYTRACING
		// Held via TSharedPtr so a cloned snapshot can share the workspace's most
		// recently-built ray tracing geometry. The workspace allocates a new TSharedPtr
		// whenever it rebuilds — see BuildRayTracingGeometryFromPlan — leaving previous
		// snapshots pointing at the prior geometry until they're released.
		TSharedPtr<FRayTracingGeometry> RayTracingGeometry;
#endif

		FRealtimeMeshDrawMask DrawMask;
		bool bVertexFactoryDirty;

		// PROXY-F11: bumped whenever a stream's underlying GPU buffer object is installed,
		// reinstalled, or removed (CreateOrUpdateStream / RegisterGPUStream / RemoveStream). Feeds
		// the ray-tracing build signature so a same-address buffer reinstall with new contents is
		// not mistaken for a no-op. Copied across Clone (COW).
		uint32 StreamGeneration = 0;
#if RHI_RAYTRACING
		// Identity of the inputs the current RayTracingGeometry was built from. Compared each batch
		// to decide whether the BLAS actually needs rebuilding. Copied across Clone and cleared in
		// Reset — both critical: every batch COWs a touched buffer set off the published snapshot,
		// so without carrying this the compare would never match and the skip would never fire.
		FRealtimeMeshRayTracingBuildSignature LastRayTracingSignature;
#endif

		// Weak ref to the LOD that owns this buffer set. After the flattening pass,
		// sections live on the LOD rather than inside the BS; the BS-level helpers
		// CreateSectionIfNotExists / RemoveSection / GetSection forward through this
		// pointer so existing builder code keeps working without rewriting every
		// callsite. Set by FRealtimeMeshLODProxy::CreateBufferSetIfNotExists on
		// creation, and re-wired to the owning LOD whenever this BS is COW'd
		// (FRealtimeMeshLODProxy::FindMutableBufferSet / UpdateCachedState).
		//
		// PROXY-F13: a BS shared UNCHANGED into a newer LOD version is NOT re-anchored
		// — FRealtimeMeshLODProxy::Clone only shallow-copies the TCowPtr array, so the
		// anchor keeps pointing at the older LOD version and goes stale once that
		// version dies. The forwarders below ensure() and safely no-op on a dead
		// anchor rather than silently mis-resolving; callers should reach sections
		// through the live LOD, where they actually live.
		TWeakPtr<FRealtimeMeshLODProxy> ParentLOD;

	public:
		FRealtimeMeshBufferSetProxy(const FRealtimeMeshContextRef& InContext, const FRealtimeMeshBufferSetKey& InKey);
		virtual ~FRealtimeMeshBufferSetProxy();

		/**
		 * Produce an independent clone for COW. Section storage is on the LOD
		 * post-flattening, so the BS clone is a shallow copy of BS-level state
		 * only. The Streams map is shallow-shared (each stream is a TSharedPtr
		 * to an immutable GPU buffer). VertexFactory is also shared by pointer;
		 * the draft allocates a fresh factory before reinitialization (see
		 * UpdateCachedState) so older versions keep referencing a stable factory.
		 * RayTracingGeometry uses TSharedPtr similarly. ParentLOD is left null
		 * here and re-wired by FRealtimeMeshLODProxy::Clone (or by step 1 of
		 * LOD::UpdateCachedState if the BS was COW'd indirectly via a section
		 * touch).
		 */
		TSharedRef<FRealtimeMeshBufferSetProxy> Clone() const;

		const FRealtimeMeshBufferSetConfig& GetConfig() const { return Config; }
		ERealtimeMeshSectionDrawType GetDrawType() const { return Config.DrawType; }

		const FRealtimeMeshBufferSetKey& GetKey() const { return Key; }
		TSharedPtr<FRealtimeMeshVertexFactory> GetVertexFactory() const { return VertexFactory; }
		FRealtimeMeshDrawMask GetDrawMask() const { return DrawMask; }

		FRealtimeMeshSectionProxyConstPtr GetSection(const FRealtimeMeshSectionKey& SectionKey) const;
		TSharedPtr<FRealtimeMeshGPUBuffer> GetStream(const FRealtimeMeshStreamKey& StreamKey) const;
		void GetStreamKeys(TArray<FRealtimeMeshStreamKey>& OutKeys) const { Streams.GetKeys(OutKeys); }

#if RHI_RAYTRACING
		const FRayTracingGeometry* GetRayTracingGeometry() const { return RayTracingGeometry.Get(); }
#endif

		FRayTracingGeometry* GetRayTracingGeometry();

		void SetParentLOD(const TSharedRef<FRealtimeMeshLODProxy>& InParentLOD) { ParentLOD = InParentLOD; }

		void UpdateConfig(const FRealtimeMeshBufferSetConfig& NewConfig);

		// These forward to the parent LOD where sections actually live.
		void CreateSectionIfNotExists(const FRealtimeMeshSectionKey& SectionKey);
		void RemoveSection(const FRealtimeMeshSectionKey& SectionKey);

		void CreateOrUpdateStream(FRHICommandListBase& RHICmdList, const FRealtimeMeshSectionGroupStreamUpdateDataRef& InStream);

		// In-place fast update of an existing stream's GPU contents over the given element
		// range, without reallocating the RHI buffer or reinitializing the vertex factory.
		// CanUpdateStreamInPlace reports eligibility (stream present, dynamic, matching
		// layout, same element count, range in bounds); UpdateStreamInPlace performs it and
		// returns false if it couldn't. The caller must have already confirmed this buffer
		// set's proxy node is uniquely owned (see FRealtimeMeshProxy::
		// FindUniqueBufferSetForInPlace) so the write cannot leak into a published snapshot.
		bool CanUpdateStreamInPlace(const FRealtimeMeshSectionGroupStreamUpdateDataRef& UpdateData, int32 ElementOffset, int32 NumElements) const;
		bool UpdateStreamInPlace(FRHICommandListBase& RHICmdList, const FRealtimeMeshSectionGroupStreamUpdateDataRef& UpdateData, int32 ElementOffset, int32 NumElements);

		// Adopt a pre-built GPU-only stream. The caller supplies the FBufferRHIRef
		// (typically produced by a compute shader or other GPU pipeline); the proxy
		// wraps it in the appropriate FRealtimeMeshGPUBuffer subclass and registers
		// it in the Streams map alongside CPU-uploaded streams. No CPU bytes are
		// copied. The well-known stream keys (Position/Tangents/TexCoords/Color and
		// the index streams) will flow through the default vertex factory. Custom
		// user keys are stored but not consumed by the default factory.
		void RegisterGPUStream(FRHICommandListBase& RHICmdList, const TSharedRef<FRealtimeMeshGPUStream>& InStream);

		void RemoveStream(const FRealtimeMeshStreamKey& StreamKey);

		bool InitializeMeshBatch(FMeshBatch& MeshBatch, FRealtimeMeshResourceReferenceList& Resources, bool bIsLocalToWorldDeterminantNegative, bool bWantsDepthOnly) const;

		// Handles VertexFactory / RayTracingGeometry / DrawMask only — the per-section
		// state and per-group active-section bookkeeping are driven from the LOD's
		// UpdateCachedState, which calls SetDrawMaskFromActiveSections.
		void UpdateCachedState(FRHICommandListBase& RHICmdList);
		void Reset();

		// Called by FRealtimeMeshLODProxy::UpdateCachedState after it has computed the
		// list of active sections belonging to this group.
		void SetDrawMaskFromActiveSections(FRHICommandListBase& RHICmdList, const TArray<int32>& ActiveSectionIndices, const TArray<TCowPtr<FRealtimeMeshSectionProxy>>& AllSections);

	protected:
#if RHI_RAYTRACING
		// Derive the ray-tracing build plan (segments + identity signature) from the current active
		// sections and streams. Single source of truth: both the rebuild-skip gate and the actual
		// build consume the same computed segment set, so the signature cannot drift from the build.
		FRealtimeMeshRayTracingBuildPlan ComputeRayTracingBuildPlan(const TArray<int32>& ActiveSectionIndices, const TArray<TCowPtr<FRealtimeMeshSectionProxy>>& AllSections) const;
		// Build the BLAS from a precomputed plan (or clear it if the plan generates nothing).
		// Returns true if a geometry was built. This is case 1 of the invalidation contract
		// (structural change → full rebuild, new geometry object).
		bool BuildRayTracingGeometryFromPlan(FRHICommandListBase& RHICmdList, const FRealtimeMeshRayTracingBuildPlan& Plan);

		// Case 2 of the invalidation contract: re-run the BLAS build against the CURRENT contents of
		// the SAME position buffer, without allocating a new geometry. The single definition of the
		// in-place "Position contents changed" refresh — the allocation-light fast path calls this
		// instead of duplicating the geometry-manager request. Keeps LastRayTracingSignature coherent
		// by construction (it touches no field the signature captures). No-op before UE 5.5, where the
		// geometry-manager rebuild-request API does not exist.
		void RefreshRayTracingGeometryInPlace();
#endif
	};
}
