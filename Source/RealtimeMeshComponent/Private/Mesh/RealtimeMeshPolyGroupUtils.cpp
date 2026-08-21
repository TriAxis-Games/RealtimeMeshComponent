// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#include "Mesh/RealtimeMeshPolyGroupUtils.h"

#include "Algo/StableSort.h"
#include "Core/RealtimeMeshDataStream.h"
#include "Core/RealtimeMeshDataTypes.h"
#include "Core/RealtimeMeshLayoutDispatch.h"

using namespace RealtimeMesh;

// Internal plumbing for the public entry points in RealtimeMeshPolyGroupUtils.h.
// Deliberately not part of the public surface: the remap-table helpers feed
// OrganizeTrianglesByPolygonGroup, and the segment/range gatherers feed
// GetStreamRangesFromPolyGroups[DepthOnly].
namespace RealtimeMeshAlgo::Private
{
	// Single copy of the per-segment min/max vertex-index scan (DUP-013). Scans the
	// triangles of one polygon-group segment for their vertex-index span and, when the
	// segment is non-degenerate, records the resulting stream range under its group index.
	template <typename IndexType>
	void AccumulateStreamRangeForPolyGroup(const FRealtimeMeshPolygonGroupRange& PolyGroup, TConstArrayView<const IndexType> Indices,
	                                       TMap<int32, FRealtimeMeshStreamRange>& OutStreamRanges)
	{
		int32 MinVertexIndex = Indices[PolyGroup.StartIndex * 3];
		int32 MaxVertexIndex = MinVertexIndex;
		const int32 MinTriangleIndex = PolyGroup.StartIndex;
		const int32 MaxTriangleIndex = PolyGroup.StartIndex + PolyGroup.Count;

		for (int32 Index = 0; Index < PolyGroup.Count; Index++)
		{
			const int32 FinalIndex = (PolyGroup.StartIndex + Index) * 3;
			MinVertexIndex = FMath::Min<IndexType>(MinVertexIndex, Indices[FinalIndex + 0]);
			MinVertexIndex = FMath::Min<IndexType>(MinVertexIndex, Indices[FinalIndex + 1]);
			MinVertexIndex = FMath::Min<IndexType>(MinVertexIndex, Indices[FinalIndex + 2]);

			MaxVertexIndex = FMath::Max<IndexType>(MaxVertexIndex, Indices[FinalIndex + 0]);
			MaxVertexIndex = FMath::Max<IndexType>(MaxVertexIndex, Indices[FinalIndex + 1]);
			MaxVertexIndex = FMath::Max<IndexType>(MaxVertexIndex, Indices[FinalIndex + 2]);
		}

		if (MaxVertexIndex != MinVertexIndex && MaxTriangleIndex != MinTriangleIndex)
		{
			OutStreamRanges.Add(PolyGroup.PolygonGroupIndex, FRealtimeMeshStreamRange(MinVertexIndex, MaxVertexIndex + 1, PolyGroup.StartIndex * 3, (PolyGroup.StartIndex + PolyGroup.Count) * 3));
		}
	}

	template <typename PolygonGroupType>
	void GenerateSortedRemapTable(TConstArrayView<const PolygonGroupType> PolygonGroups, TArrayView<uint32> OutRemapTable)
	{
		check(PolygonGroups.Num() == OutRemapTable.Num());

		// Fill with starting data 0...N
		for (int32 Index = 0; Index < OutRemapTable.Num(); Index++)
		{
			OutRemapTable[Index] = Index;
		}

		// Run stable sort on the remap table, using the polygon group indices as the sorting index
		Algo::StableSortBy(OutRemapTable, [&PolygonGroups](int32 Index)
		{
			return PolygonGroups[Index];
		});
	}

	bool GenerateSortedRemapTable(const FRealtimeMeshStream& PolygonGroups, TArrayView<uint32> OutRemapTable)
	{
		return TElementDispatch<uint16, int16, uint32, int32>::Visit(PolygonGroups,
			[&](auto View)
			{
				using T = typename decltype(View)::ElementType;
				GenerateSortedRemapTable(TConstArrayView<const T>(View.GetData(), View.Num()), OutRemapTable);
			});
	}

	void ApplyRemapTableToStream(TArrayView<uint32> RemapTable, FRealtimeMeshStream& Stream)
	{
		check(RemapTable.Num() == Stream.Num());
		FRealtimeMeshStream NewData(Stream.GetStreamKey(), Stream.GetLayout());
		NewData.SetNumUninitialized(Stream.Num());

		for (int32 Index = 0; Index < RemapTable.Num(); Index++)
		{
			const int32 OldIndex = RemapTable[Index];
			FMemory::Memcpy(NewData.GetData() + Index * Stream.GetStride(), Stream.GetData() + OldIndex * Stream.GetStride(), Stream.GetStride());
		}

		Stream = MoveTemp(NewData);
	}

	// Walks a run-length view of the polygroup index list, invoking the callback once
	// per contiguous segment.
	template <typename PolygonGroupType>
	void GatherSegmentsFromPolygonGroupIndices(TConstArrayView<const PolygonGroupType> PolygonGroupIndices,
	                                           const TFunctionRef<void(const FRealtimeMeshPolygonGroupRange& NewSegment)>& OnAddNewSegmentFunction)
	{
		if (PolygonGroupIndices.Num() < 1)
		{
			return;
		}

		FRealtimeMeshPolygonGroupRange NextSegment;
		NextSegment.PolygonGroupIndex = PolygonGroupIndices[0];
		NextSegment.StartIndex = 0;
		for (int32 Index = 1; Index < PolygonGroupIndices.Num(); Index++)
		{
			const int32 CurrentMatIndex = PolygonGroupIndices[Index];

			if (CurrentMatIndex != NextSegment.PolygonGroupIndex)
			{
				NextSegment.Count = Index - NextSegment.StartIndex;
				OnAddNewSegmentFunction(NextSegment);
				NextSegment.PolygonGroupIndex = CurrentMatIndex;
				NextSegment.StartIndex = Index;
			}
		}
		NextSegment.Count = PolygonGroupIndices.Num() - NextSegment.StartIndex;
		OnAddNewSegmentFunction(NextSegment);
	}

	template <typename IndexType>
	void GatherStreamRangesFromPolyGroupRanges(TConstArrayView<const FRealtimeMeshPolygonGroupRange> PolygonGroupSegments, TConstArrayView<const IndexType> Indices,
	                                           TMap<int32, FRealtimeMeshStreamRange>& OutStreamRanges)
	{
		for (const auto& PolyGroup : PolygonGroupSegments)
		{
			// Bail if this segment has no triangles
			if (PolyGroup.Count < 1)
			{
				continue;
			}

			// DUP-013: shared per-segment scan (see AccumulateStreamRangeForPolyGroup)
			AccumulateStreamRangeForPolyGroup(PolyGroup, Indices, OutStreamRanges);
		}
	}

	void GatherStreamRangesFromPolyGroupRanges(TConstArrayView<const FRealtimeMeshPolygonGroupRange> PolygonGroupSegments,
	                                           const FRealtimeMeshStream& Triangles, TMap<int32, FRealtimeMeshStreamRange>& OutStreamRanges)
	{
		const bool bDispatched = TElementDispatch<uint16, int16, uint32, int32>::Visit(Triangles,
			[&](auto View)
			{
				using T = typename decltype(View)::ElementType;
				GatherStreamRangesFromPolyGroupRanges(PolygonGroupSegments, TConstArrayView<const T>(View.GetData(), View.Num()), OutStreamRanges);
			});
		checkf(bDispatched, TEXT("Unsupported format for Triangles"));
	}

	void GatherStreamRangesFromPolyGroupRanges(const FRealtimeMeshStream& PolygonGroupSegments,
	                                           const FRealtimeMeshStream& Triangles, TMap<int32, FRealtimeMeshStreamRange>& OutStreamRanges)
	{
		const TConstArrayView<const FRealtimeMeshPolygonGroupRange> PolyGroupRanges = PolygonGroupSegments.GetArrayView<FRealtimeMeshPolygonGroupRange>();
		GatherStreamRangesFromPolyGroupRanges(PolyGroupRanges, Triangles, OutStreamRanges);
	}

	template <typename PolygonGroupType, typename IndexType>
	void GatherStreamRangesFromPolyGroupIndices(TConstArrayView<const PolygonGroupType> PolygonGroupIndices, TConstArrayView<const IndexType> Indices,
	                                            TMap<int32, FRealtimeMeshStreamRange>& OutStreamRanges)
	{
		ensure(Indices.Num() >= PolygonGroupIndices.Num() * 3);
		if (PolygonGroupIndices.Num() < 1 || Indices.Num() < 3)
		{
			return;
		}

		const int32 MaxTriangleCount = FMath::Min(PolygonGroupIndices.Num(), Indices.Num() / 3);

		GatherSegmentsFromPolygonGroupIndices(PolygonGroupIndices.Slice(0, MaxTriangleCount), [&](const FRealtimeMeshPolygonGroupRange& PolyGroup)
		{
			if (!OutStreamRanges.Contains(PolyGroup.PolygonGroupIndex))
			{
				// DUP-013: shared per-segment scan (see AccumulateStreamRangeForPolyGroup)
				AccumulateStreamRangeForPolyGroup(PolyGroup, Indices, OutStreamRanges);
			}
		});
	}

	template <typename PolygonGroupType>
	void GatherStreamRangesFromPolyGroupIndices(TConstArrayView<const PolygonGroupType> PolygonGroupIndices, const FRealtimeMeshStream& Indices,
	                                            TMap<int32, FRealtimeMeshStreamRange>& OutStreamRanges)
	{
		// DUP-013: converged the hand-rolled uint16/int16/uint32/int32 if-chain onto the
		// shared TElementDispatch mechanism (same type order, same per-type code path).
		const bool bDispatched = TElementDispatch<uint16, int16, uint32, int32>::Visit(Indices,
			[&](auto View)
			{
				using T = typename decltype(View)::ElementType;
				GatherStreamRangesFromPolyGroupIndices(PolygonGroupIndices, TConstArrayView<const T>(View.GetData(), View.Num()), OutStreamRanges);
			});
		checkf(bDispatched, TEXT("Unsupported format for Indices"));
	}

	void GatherStreamRangesFromPolyGroupIndices(const FRealtimeMeshStream& PolygonGroupIndices, const FRealtimeMeshStream& Indices,
	                                            TMap<int32, FRealtimeMeshStreamRange>& OutStreamRanges)
	{
		const bool bDispatched = TElementDispatch<uint16, int16, uint32, int32>::Visit(PolygonGroupIndices,
			[&](auto View)
			{
				using T = typename decltype(View)::ElementType;
				GatherStreamRangesFromPolyGroupIndices(TConstArrayView<const T>(View.GetData(), View.Num()), Indices, OutStreamRanges);
			});
		checkf(bDispatched, TEXT("Unsupported format for PolygonGroupIndices"));
	}
}

bool RealtimeMeshAlgo::OrganizeTrianglesByPolygonGroup(FRealtimeMeshStream& IndexStream, FRealtimeMeshStream& PolygonGroupStream,
                                                       TArrayView<uint32> OutRemapTable)
{
	// Make sure triangle count and polygon group indices length are the same
	if ((IndexStream.Num() * IndexStream.GetNumElements() / 3) != PolygonGroupStream.Num())
	{
		return false;
	}

	// Make sure the remap table is the same length as the polygon group stream
	if (OutRemapTable.Num() != PolygonGroupStream.Num())
	{
		return false;
	}

	if (Private::GenerateSortedRemapTable(PolygonGroupStream, OutRemapTable))
	{
		Private::ApplyRemapTableToStream(OutRemapTable, IndexStream);
		Private::ApplyRemapTableToStream(OutRemapTable, PolygonGroupStream);
		return true;
	}
	return false;
}

bool RealtimeMeshAlgo::OrganizeTrianglesByPolygonGroup(FRealtimeMeshStreamSet& InStreamSet, const FRealtimeMeshStreamKey& IndexStreamKey,
                                                       const FRealtimeMeshStreamKey& PolygonGroupStreamKey, TArray<uint32>* OutRemapTable)
{
	FRealtimeMeshStream* IndexStream = InStreamSet.Find(IndexStreamKey);
	FRealtimeMeshStream* PolygonGroupStream = InStreamSet.Find(PolygonGroupStreamKey);

	// Do we have both streams?
	if (!IndexStream || !PolygonGroupStream)
	{
		return false;
	}

	TArray<uint32> Temp;
	TArray<uint32>* RemapTable = OutRemapTable ? OutRemapTable : &Temp;
	RemapTable->SetNumUninitialized(PolygonGroupStream->Num());

	return OrganizeTrianglesByPolygonGroup(*IndexStream, *PolygonGroupStream, *RemapTable);
}

TOptional<TMap<int32, FRealtimeMeshStreamRange>> RealtimeMeshAlgo::GetStreamRangesFromPolyGroups(const FRealtimeMeshStreamSet& Streams,
                                                                                                 const FRealtimeMeshStreamKey& TrianglesKey,
                                                                                                 const FRealtimeMeshStreamKey& PolyGroupsKey,
                                                                                                 const FRealtimeMeshStreamKey& PolyGroupSegmentsKey)
{
	if (const auto Triangles = Streams.Find(TrianglesKey))
	{
		if (const auto PolyGroupSegments = Streams.Find(PolyGroupSegmentsKey))
		{
			TMap<int32, FRealtimeMeshStreamRange> Ranges;
			Private::GatherStreamRangesFromPolyGroupRanges(*PolyGroupSegments, *Triangles, Ranges);
			return MoveTemp(Ranges);
		}

		if (const auto PolyGroupIndices = Streams.Find(PolyGroupsKey))
		{
			TMap<int32, FRealtimeMeshStreamRange> Ranges;
			Private::GatherStreamRangesFromPolyGroupIndices(*PolyGroupIndices, *Triangles, Ranges);
			return MoveTemp(Ranges);
		}
	}

	return TOptional<TMap<int32, FRealtimeMeshStreamRange>>();
}

TOptional<TMap<int32, FRealtimeMeshStreamRange>> RealtimeMeshAlgo::GetStreamRangesFromPolyGroupsDepthOnly(const FRealtimeMeshStreamSet& Streams)
{
	return GetStreamRangesFromPolyGroups(Streams, FRealtimeMeshStreams::DepthOnlyTriangles, FRealtimeMeshStreams::DepthOnlyPolyGroups,
	                                     FRealtimeMeshStreams::DepthOnlyPolyGroupSegments);
}
