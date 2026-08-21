// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Core/RealtimeMeshDataStream.h"

struct FRealtimeMeshStreamKey;

namespace RealtimeMesh
{
	struct FRealtimeMeshStream;
	struct FRealtimeMeshStreamSet;
}

/**
 * Polygon-group utilities: organizing triangles by polygon group and deriving
 * per-group stream ranges (what drives polygroup auto-created sections).
 *
 * The internal plumbing (remap tables, segment gathering, the per-format
 * dispatch chains) lives in Private/Mesh/RealtimeMeshPolyGroupUtils.cpp — this
 * header is only the supported entry points. (Split out of the former
 * RealtimeMeshAlgo.h; the namespace name is retained for source compatibility.)
 */
namespace RealtimeMeshAlgo
{
	/**
	 * Sorts the triangle (and polygon-group) streams so that each polygon group's
	 * triangles are contiguous, which is required for a single section range per
	 * group. Returns false if the streams are missing or their sizes disagree.
	 */
	REALTIMEMESHCOMPONENT_API bool OrganizeTrianglesByPolygonGroup(RealtimeMesh::FRealtimeMeshStream& IndexStream, RealtimeMesh::FRealtimeMeshStream& PolygonGroupStream,
	                                                               TArrayView<uint32> OutRemapTable);

	REALTIMEMESHCOMPONENT_API bool OrganizeTrianglesByPolygonGroup(RealtimeMesh::FRealtimeMeshStreamSet& InStreamSet, const FRealtimeMeshStreamKey& IndexStreamKey,
	                                                               const FRealtimeMeshStreamKey& PolygonGroupStreamKey, TArray<uint32>* OutRemapTable = nullptr);

	/**
	 * Derives the per-polygroup vertex/index stream ranges from a stream set's
	 * triangle + polygroup (or pre-gathered polygroup-segment) streams. Unset when
	 * the required streams are absent.
	 */
	REALTIMEMESHCOMPONENT_API TOptional<TMap<int32, FRealtimeMeshStreamRange>> GetStreamRangesFromPolyGroups(const RealtimeMesh::FRealtimeMeshStreamSet& Streams,
		const FRealtimeMeshStreamKey& TrianglesKey = RealtimeMesh::FRealtimeMeshStreams::Triangles,
		const FRealtimeMeshStreamKey& PolyGroupsKey = RealtimeMesh::FRealtimeMeshStreams::PolyGroups,
		const FRealtimeMeshStreamKey& PolyGroupSegmentsKey = RealtimeMesh::FRealtimeMeshStreams::PolyGroupSegments);

	REALTIMEMESHCOMPONENT_API TOptional<TMap<int32, FRealtimeMeshStreamRange>> GetStreamRangesFromPolyGroupsDepthOnly(const RealtimeMesh::FRealtimeMeshStreamSet& Streams);
}
