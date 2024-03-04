// Copyright TriAxis Games, L.L.C. All Rights Reserved.


#include "Mesh/RealtimeMeshAlgo.h"

#include "Mesh/RealtimeMeshBuilder.h"
#include "Mesh/RealtimeMeshDataStream.h"
#include "Mesh/RealtimeMeshDataTypes.h"

using namespace RealtimeMesh;

bool RealtimeMeshAlgo::GenerateSortedRemapTable(const FRealtimeMeshStream& PolygonGroups, TArrayView<uint32> OutRemapTable)
{
	if (PolygonGroups.GetLayout().GetElementType() == GetRealtimeMeshDataElementType<uint16>())
	{
		GenerateSortedRemapTable(PolygonGroups.GetElementArrayView<uint16>(), OutRemapTable);
		return true;
	}
	if (PolygonGroups.GetLayout().GetElementType() == GetRealtimeMeshDataElementType<int16>())
	{
		GenerateSortedRemapTable(PolygonGroups.GetElementArrayView<int16>(), OutRemapTable);
		return true;
	}
	if (PolygonGroups.GetLayout().GetElementType() == GetRealtimeMeshDataElementType<uint32>())
	{
		GenerateSortedRemapTable(PolygonGroups.GetElementArrayView<uint32>(), OutRemapTable);
		return true;
	}
	if (PolygonGroups.GetLayout().GetElementType() == GetRealtimeMeshDataElementType<int32>())
	{
		GenerateSortedRemapTable(PolygonGroups.GetElementArrayView<int32>(), OutRemapTable);
		return true;
	}

	return false;
}

void RealtimeMeshAlgo::ApplyRemapTableToStream(TArrayView<uint32> RemapTable, FRealtimeMeshStream& Stream)
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
			
	if (GenerateSortedRemapTable(PolygonGroupStream, OutRemapTable))
	{
		ApplyRemapTableToStream(OutRemapTable, IndexStream);
		ApplyRemapTableToStream(OutRemapTable, PolygonGroupStream);
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
	TArray<uint32>* RemapTable = OutRemapTable? OutRemapTable : &Temp;
	RemapTable->SetNumUninitialized(PolygonGroupStream->Num());

	return  OrganizeTrianglesByPolygonGroup(*IndexStream, *PolygonGroupStream, *RemapTable);
}

void RealtimeMeshAlgo::PropagateTriangleSegmentsToPolygonGroups(TArrayView<FRealtimeMeshPolygonGroupRange> TriangleSegments, FRealtimeMeshStream& OutPolygonGroupIndices)
{
	if (OutPolygonGroupIndices.GetLayout().GetElementType() == GetRealtimeMeshDataElementType<uint16>())
	{
		TArrayView<uint16> View = OutPolygonGroupIndices.GetArrayView<uint16>();
		PropagateTriangleSegmentsToPolygonGroups(TriangleSegments, View);
	}
	if (OutPolygonGroupIndices.GetLayout().GetElementType() == GetRealtimeMeshDataElementType<int16>())
	{
		TArrayView<int16> View = OutPolygonGroupIndices.GetArrayView<int16>();
		PropagateTriangleSegmentsToPolygonGroups(TriangleSegments, View);
	}
	if (OutPolygonGroupIndices.GetLayout().GetElementType() == GetRealtimeMeshDataElementType<uint32>())
	{
		TArrayView<uint32> View = OutPolygonGroupIndices.GetArrayView<uint32>();
		PropagateTriangleSegmentsToPolygonGroups(TriangleSegments, View);
	}
	if (OutPolygonGroupIndices.GetLayout().GetElementType() == GetRealtimeMeshDataElementType<int32>())
	{
		TArrayView<int32> View = OutPolygonGroupIndices.GetArrayView<int32>();
		PropagateTriangleSegmentsToPolygonGroups(TriangleSegments, View);
	}
}

bool RealtimeMeshAlgo::ArePolygonGroupIndicesOptimal(TConstArrayView<const FRealtimeMeshPolygonGroupRange> TriangleSegments)
{
	TSet<int32> UniquePolygonGroupIndices;
	for (int32 Index = 0; Index < TriangleSegments.Num(); Index++)
	{
		bool bAlreadyContains;
		UniquePolygonGroupIndices.Add(TriangleSegments[Index].PolygonGroupIndex, &bAlreadyContains);

		if (bAlreadyContains)
		{
			return false;
		}
	}

	return true;
}

bool RealtimeMeshAlgo::ArePolygonGroupIndicesOptimal(const FRealtimeMeshStream& PolygonGroupIndices)
{
	if (PolygonGroupIndices.GetLayout().GetElementType() == GetRealtimeMeshDataElementType<uint16>())
	{
		return ArePolygonGroupIndicesOptimal(PolygonGroupIndices.GetElementArrayView<uint16>());
	}
	if (PolygonGroupIndices.GetLayout().GetElementType() == GetRealtimeMeshDataElementType<int16>())
	{
		return ArePolygonGroupIndicesOptimal(PolygonGroupIndices.GetElementArrayView<int16>());
	}
	if (PolygonGroupIndices.GetLayout().GetElementType() == GetRealtimeMeshDataElementType<uint32>())
	{
		return ArePolygonGroupIndicesOptimal(PolygonGroupIndices.GetElementArrayView<uint32>());
	}
	if (PolygonGroupIndices.GetLayout().GetElementType() == GetRealtimeMeshDataElementType<int32>())
	{
		return ArePolygonGroupIndicesOptimal(PolygonGroupIndices.GetElementArrayView<int32>());
	}

	checkf(false, TEXT("Unsupported format for PolygonGroupIndices"));
	return false;
}

void RealtimeMeshAlgo::GatherSegmentsFromPolygonGroupIndices(const FRealtimeMeshStream& PolygonGroupIndices,
	const TFunctionRef<void(const FRealtimeMeshPolygonGroupRange& NewSegment)>& OnAddNewSegmentFunction)
{
	if (PolygonGroupIndices.GetLayout().GetElementType() == GetRealtimeMeshDataElementType<uint16>())
	{
		GatherSegmentsFromPolygonGroupIndices(PolygonGroupIndices.GetElementArrayView<uint16>(), OnAddNewSegmentFunction);
	}
	else if (PolygonGroupIndices.GetLayout().GetElementType() == GetRealtimeMeshDataElementType<int16>())
	{
		GatherSegmentsFromPolygonGroupIndices(PolygonGroupIndices.GetElementArrayView<int16>(), OnAddNewSegmentFunction);
	}
	else if (PolygonGroupIndices.GetLayout().GetElementType() == GetRealtimeMeshDataElementType<uint32>())
	{
		GatherSegmentsFromPolygonGroupIndices(PolygonGroupIndices.GetElementArrayView<uint32>(), OnAddNewSegmentFunction);
	}
	else if (PolygonGroupIndices.GetLayout().GetElementType() == GetRealtimeMeshDataElementType<int32>())
	{
		GatherSegmentsFromPolygonGroupIndices(PolygonGroupIndices.GetElementArrayView<int32>(), OnAddNewSegmentFunction);
	}
	else
	{
		checkf(false, TEXT("Unsupported format for PolygonGroupIndices"));		
	}
}

void RealtimeMeshAlgo::GatherSegmentsFromPolygonGroupIndices(const FRealtimeMeshStream& PolygonGroupIndices, FRealtimeMeshStream& OutSegments)
{
	TRealtimeMeshStreamBuilder<FRealtimeMeshPolygonGroupRange> SegmentsBuilder(OutSegments);
	SegmentsBuilder.Empty();

	GatherSegmentsFromPolygonGroupIndices(PolygonGroupIndices, [&SegmentsBuilder](const FRealtimeMeshPolygonGroupRange& NewSegment)
	{
		SegmentsBuilder.Add(NewSegment);
	});	
}

void RealtimeMeshAlgo::GatherStreamRangesFromPolyGroupRanges(TConstArrayView<const FRealtimeMeshPolygonGroupRange> PolygonGroupSegments,
	const FRealtimeMeshStream& Triangles, TMap<int32, FRealtimeMeshStreamRange>& OutStreamRanges)
{
	if (Triangles.GetLayout().GetElementType() == GetRealtimeMeshDataElementType<uint16>())
	{
		GatherStreamRangesFromPolyGroupRanges(PolygonGroupSegments, Triangles.GetElementArrayView<uint16>(), OutStreamRanges);
	}
	else if (Triangles.GetLayout().GetElementType() == GetRealtimeMeshDataElementType<int16>())
	{
		GatherStreamRangesFromPolyGroupRanges(PolygonGroupSegments, Triangles.GetElementArrayView<int16>(), OutStreamRanges);
	}
	else if (Triangles.GetLayout().GetElementType() == GetRealtimeMeshDataElementType<uint32>())
	{
		GatherStreamRangesFromPolyGroupRanges(PolygonGroupSegments, Triangles.GetElementArrayView<uint32>(), OutStreamRanges);
	}
	else if (Triangles.GetLayout().GetElementType() == GetRealtimeMeshDataElementType<int32>())
	{
		GatherStreamRangesFromPolyGroupRanges(PolygonGroupSegments, Triangles.GetElementArrayView<int32>(), OutStreamRanges);
	}
	else
	{
		checkf(false, TEXT("Unsupported format for PolygonGroupIndices"));		
	}	
}

void RealtimeMeshAlgo::GatherStreamRangesFromPolyGroupRanges(const FRealtimeMeshStream& PolygonGroupSegments,
	const FRealtimeMeshStream& Triangles, TMap<int32, FRealtimeMeshStreamRange>& OutStreamRanges)
{
	const TConstArrayView<const FRealtimeMeshPolygonGroupRange> PolyGroupRanges = PolygonGroupSegments.GetArrayView<FRealtimeMeshPolygonGroupRange>();
	GatherStreamRangesFromPolyGroupRanges(PolyGroupRanges, Triangles, OutStreamRanges);
}

void RealtimeMeshAlgo::GatherStreamRangesFromPolyGroupIndices(const FRealtimeMeshStream& PolygonGroupIndices, const FRealtimeMeshStream& Indices,
	TMap<int32, FRealtimeMeshStreamRange>& OutStreamRanges)
{
	if (PolygonGroupIndices.GetLayout().GetElementType() == RealtimeMesh::GetRealtimeMeshDataElementType<uint16>())
	{
		GatherStreamRangesFromPolyGroupIndices(PolygonGroupIndices.GetElementArrayView<uint16>(), Indices, OutStreamRanges);
	}
	else if (PolygonGroupIndices.GetLayout().GetElementType() == RealtimeMesh::GetRealtimeMeshDataElementType<int16>())
	{
		GatherStreamRangesFromPolyGroupIndices(PolygonGroupIndices.GetElementArrayView<int16>(), Indices, OutStreamRanges);
	}
	else if (PolygonGroupIndices.GetLayout().GetElementType() == RealtimeMesh::GetRealtimeMeshDataElementType<uint32>())
	{
		GatherStreamRangesFromPolyGroupIndices(PolygonGroupIndices.GetElementArrayView<uint32>(), Indices, OutStreamRanges);
	}
	else if (PolygonGroupIndices.GetLayout().GetElementType() == RealtimeMesh::GetRealtimeMeshDataElementType<int32>())
	{
		GatherStreamRangesFromPolyGroupIndices(PolygonGroupIndices.GetElementArrayView<int32>(), Indices, OutStreamRanges);
	}
	else
	{
		checkf(false, TEXT("Unsupported format for PolygonGroupIndices"));		
	}			
}

