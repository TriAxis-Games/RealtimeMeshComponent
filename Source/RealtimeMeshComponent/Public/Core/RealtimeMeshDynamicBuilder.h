// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "RealtimeMeshCoreFwd.h"
#include "RealtimeMeshBuilder.h"
#include "RealtimeMeshDataStream.h"
#include "RealtimeMeshKeys.h"

namespace RealtimeMesh
{
	/**
	 * Non-templated, runtime-layout builder for RMC stream sets.
	 *
	 * Where it fits relative to TRealtimeMeshBuilderLocal:
	 *   - TRealtimeMeshBuilderLocal is compile-time-typed. Its Add/Set methods
	 *     are zero-cost format-matched accessors. Use it when the format is
	 *     known at compile time and perf-critical hot loops want devirtualized
	 *     writes.
	 *   - FRealtimeMeshDynamicBuilder is runtime-typed. Format is decided at
	 *     EnableX() time. Per-call writes pay a small layout-dispatch cost.
	 *     Use it when:
	 *       * The format is decided at runtime (e.g., Blueprint exposure).
	 *       * Code needs to pass the builder by reference into non-templated
	 *         utility functions.
	 *       * You want native N-channel UV support without separately-managed
	 *         strided-builder pointers per channel.
	 *
	 * Internally, this is a thin layer over the existing strided stream
	 * builder template with runtime layout (void buffer type), plus three
	 * FRealtimeMeshStreamLinkage instances driving size lockstep via the
	 * batch API.
	 */
	class REALTIMEMESHCOMPONENT_API FRealtimeMeshDynamicBuilder
	{
	private:
		FRealtimeMeshStreamSet& Streams;

		// Required vertex / triangle streams.
		TUniquePtr<TRealtimeMeshStreamBuilder<FVector3f, void>> Vertices;
		TUniquePtr<TRealtimeMeshStreamBuilder<TIndex3<uint32>, void>> Triangles;

		// Optional vertex-side streams.
		TUniquePtr<TRealtimeMeshStreamBuilder<TRealtimeMeshTangents<FVector4f>, void>> Tangents;
		TUniquePtr<TRealtimeMeshStreamBuilder<FColor, void>> Colors;

		// TexCoords: an N-channel multi-element vertex stream + one strided
		// builder per channel. Adding/removing channels resizes both the
		// underlying stream and this array.
		TArray<TUniquePtr<TRealtimeMeshStridedStreamBuilder<FVector2f, void>>> TexCoordChannels;

		// Optional triangle-side streams.
		TUniquePtr<TRealtimeMeshStreamBuilder<TIndex3<uint32>, void>> DepthOnlyTriangles;
		TUniquePtr<TRealtimeMeshStreamBuilder<uint32, void>> TrianglePolyGroups;
		TUniquePtr<TRealtimeMeshStreamBuilder<uint32, void>> DepthOnlyTrianglePolyGroups;

		FRealtimeMeshStreamLinkage VerticesLinkage;
		FRealtimeMeshStreamLinkage TrianglesLinkage;
		FRealtimeMeshStreamLinkage DepthOnlyTrianglesLinkage;

	public:
		// Constructs the builder over an externally-owned stream set. Allocates
		// Position + Triangles streams (the required ones) if they don't already
		// exist, with default layouts (FVector3f position, uint16 indices). To
		// use higher-precision indices, call EnableHighPrecisionIndices() after
		// construction before adding any data.
		explicit FRealtimeMeshDynamicBuilder(FRealtimeMeshStreamSet& InStreams);

		// Non-copyable, non-movable — the contained linkages aren't movable, and
		// the strided-builder TUniquePtrs would need rebinding on move anyway.
		FRealtimeMeshDynamicBuilder(const FRealtimeMeshDynamicBuilder&) = delete;
		FRealtimeMeshDynamicBuilder(FRealtimeMeshDynamicBuilder&&) = delete;
		FRealtimeMeshDynamicBuilder& operator=(const FRealtimeMeshDynamicBuilder&) = delete;
		FRealtimeMeshDynamicBuilder& operator=(FRealtimeMeshDynamicBuilder&&) = delete;

		// ---- Stream enablement / configuration ----

		// Tangents (Normal + Tangent packed as two-element row). ElementType is
		// the per-tangent storage type (FPackedNormal, FPackedRGBA16N, FVector4f).
		void EnableTangents(const FRealtimeMeshElementType& ElementType);
		void EnableTangents() { EnableTangents(GetRealtimeMeshDataElementType<FPackedNormal>()); }
		void DisableTangents();
		bool HasTangents() const { return Tangents.IsValid(); }

		// Vertex colors. Always stored as FColor.
		void EnableColors();
		void DisableColors();
		bool HasColors() const { return Colors.IsValid(); }
		bool HasVertexColors() const { return HasColors(); }  // Compat alias matching TRealtimeMeshBuilderLocal.

		// Tex coords. ElementType is the per-channel storage type
		// (FVector2f, FVector2DHalf). NumChannels in [1, REALTIME_MESH_MAX_TEX_COORDS].
		void EnableTexCoords(const FRealtimeMeshElementType& ElementType, int32 NumChannels);
		void EnableTexCoords(int32 NumChannels = 1) { EnableTexCoords(GetRealtimeMeshDataElementType<FVector2DHalf>(), NumChannels); }
		void DisableTexCoords();
		bool HasTexCoords() const { return TexCoordChannels.Num() > 0; }
		int32 NumTexCoordChannels() const { return TexCoordChannels.Num(); }

		// Replaces the triangle index format. Pre-enabled at construction with
		// uint16. Call this before adding any triangles to switch to uint32.
		void SetTriangleIndexType(const FRealtimeMeshElementType& IndexType);

		void EnableDepthOnlyTriangles(const FRealtimeMeshElementType& IndexType);
		void DisableDepthOnlyTriangles();
		bool HasDepthOnlyTriangles() const { return DepthOnlyTriangles.IsValid(); }

		void EnablePolyGroups(const FRealtimeMeshElementType& IndexType);
		void DisablePolyGroups();
		bool HasPolyGroups() const { return TrianglePolyGroups.IsValid(); }

		// ---- Bulk-add via linkage batch API ----

		// Adds N vertex rows across all vertex-side linked streams (Position,
		// Tangents, TexCoords, Color) in one growth pass. Returns the starting
		// row index. New rows are zero-filled with each stream's default.
		int32 AddVerticesZeroed(int32 NumRows) { return VerticesLinkage.AddRowsZeroed(NumRows); }

		// Adds N vertex rows uninitialized. Caller must write every byte of
		// every linked stream's new rows before the data is read.
		int32 AddVerticesUninitialized(int32 NumRows) { return VerticesLinkage.AddRowsUninitialized(NumRows); }

		int32 AddTrianglesZeroed(int32 NumRows) { return TrianglesLinkage.AddRowsZeroed(NumRows); }
		int32 AddTrianglesUninitialized(int32 NumRows) { return TrianglesLinkage.AddRowsUninitialized(NumRows); }

		int32 AddDepthOnlyTrianglesZeroed(int32 NumRows) { return DepthOnlyTrianglesLinkage.AddRowsZeroed(NumRows); }
		int32 AddDepthOnlyTrianglesUninitialized(int32 NumRows) { return DepthOnlyTrianglesLinkage.AddRowsUninitialized(NumRows); }

		// Pre-allocate capacity for upcoming AddVertices / AddTriangles calls.
		void ReserveAdditionalVertices(int32 N) { VerticesLinkage.ReserveRows(VerticesLinkage.GetNumRows() + N); }
		void ReserveAdditionalTriangles(int32 N) { TrianglesLinkage.ReserveRows(TrianglesLinkage.GetNumRows() + N); }
		void ReserveAdditionalDepthOnlyTriangles(int32 N) { DepthOnlyTrianglesLinkage.ReserveRows(DepthOnlyTrianglesLinkage.GetNumRows() + N); }

		// ---- Empty / Reset ----

		void EmptyVertices();
		void EmptyTriangles();
		void EmptyDepthOnlyTriangles();
		void Reset();

		// ---- Per-row writes (runtime-converting; slow path) ----

		void SetPosition(int32 Row, const FVector3f& Value);
		void SetTangents(int32 Row, const FVector3f& Normal, const FVector3f& Tangent);
		// Three-vector form: computes tangent.W (handedness) from normal x tangent
		// vs. the supplied binormal.
		void SetTangents(int32 Row, const FVector3f& Normal, const FVector3f& Binormal, const FVector3f& Tangent);
		void SetColor(int32 Row, const FColor& Value);
		void SetColor(int32 Row, const FLinearColor& Value);
		void SetTexCoord(int32 Row, int32 Channel, const FVector2f& Value);
		void SetTriangle(int32 Row, uint32 A, uint32 B, uint32 C);
		void SetTriangle(int32 Row, uint32 A, uint32 B, uint32 C, uint32 PolyGroup);
		void SetDepthOnlyTriangle(int32 Row, uint32 A, uint32 B, uint32 C);
		void SetPolyGroup(int32 Row, uint32 PolyGroupIndex);
		void SetDepthOnlyPolyGroup(int32 Row, uint32 PolyGroupIndex);

		// Convenience: add-and-set in one call. Returns the new row index.
		int32 AddVertex(const FVector3f& Position);
		int32 AddTriangle(uint32 A, uint32 B, uint32 C);
		int32 AddTriangle(uint32 A, uint32 B, uint32 C, uint32 PolyGroup);
		int32 AddDepthOnlyTriangle(uint32 A, uint32 B, uint32 C);

		// ---- Per-row reads (runtime-converting; slow path) ----

		FVector3f GetPosition(int32 Row) const;
		FVector3f GetNormal(int32 Row) const;
		FVector3f GetTangent(int32 Row) const;
		FColor GetColor(int32 Row) const;
		FVector2f GetTexCoord(int32 Row, int32 Channel) const;
		TIndex3<uint32> GetTriangle(int32 Row) const;
		TIndex3<uint32> GetDepthOnlyTriangle(int32 Row) const;
		uint32 GetPolyGroup(int32 Row) const;
		uint32 GetDepthOnlyPolyGroup(int32 Row) const;

		// ---- Counts ----

		int32 NumVertices() const { return VerticesLinkage.GetNumRows(); }
		int32 NumTriangles() const { return TrianglesLinkage.GetNumRows(); }
		int32 NumDepthOnlyTriangles() const { return DepthOnlyTrianglesLinkage.GetNumRows(); }

		// ---- Stream access for utility code ----

		FRealtimeMeshStreamSet& GetStreamSet() { return Streams; }
		const FRealtimeMeshStreamSet& GetStreamSet() const { return Streams; }

		// Untyped stream getters — utilities pair these with TLayoutDispatch
		// or per-row converting accessors to write generic code.
		const FRealtimeMeshStream* GetPositionStream() const { return Vertices ? &Vertices->GetStream() : nullptr; }
		const FRealtimeMeshStream* GetTangentsStream() const { return Tangents ? &Tangents->GetStream() : nullptr; }
		const FRealtimeMeshStream* GetColorStream() const { return Colors ? &Colors->GetStream() : nullptr; }
		const FRealtimeMeshStream* GetTexCoordsStream() const;
		const FRealtimeMeshStream* GetTriangleStream() const { return Triangles ? &Triangles->GetStream() : nullptr; }

	private:
		// Rebuild the per-channel strided builders to match a freshly-allocated
		// or replaced TexCoords stream. Called by EnableTexCoords / Reset.
		void RebuildTexCoordChannels();
	};
}
