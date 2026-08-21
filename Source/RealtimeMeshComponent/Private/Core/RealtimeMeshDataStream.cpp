// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.


#include "Core/RealtimeMeshDataStream.h"

#include <string>


namespace RealtimeMesh::NatVis
{
	std::string GetRowElementAsString(const FRealtimeMeshStream& Stream, int32 Row, int32 Element) noexcept
	{
		TFunction<std::string(const FRealtimeMeshStream&, int32, int32, int32)> DatumReader;
		switch (Stream.GetLayout().GetElementType().GetDatumType())
		{
		case ERealtimeMeshDatumType::UInt8:
			DatumReader = [](const FRealtimeMeshStream& Stream, int32 Row, int32 ElementIndex,
			                 int32 DatumIndex) -> std::string
			{
				return std::to_string(
					*(reinterpret_cast<const uint8*>(Stream.GetDataRawAtVertex(Row, ElementIndex)) + DatumIndex));
			};
			break;
		case ERealtimeMeshDatumType::Int8:
			DatumReader = [](const FRealtimeMeshStream& Stream, int32 Row, int32 ElementIndex,
			                 int32 DatumIndex) -> std::string
			{
				return std::to_string(
					*(reinterpret_cast<const int8*>(Stream.GetDataRawAtVertex(Row, ElementIndex)) + DatumIndex));
			};
			break;
		case ERealtimeMeshDatumType::UInt16:
			DatumReader = [](const FRealtimeMeshStream& Stream, int32 Row, int32 ElementIndex,
			                 int32 DatumIndex) -> std::string
			{
				return std::to_string(
					*(reinterpret_cast<const uint16*>(Stream.GetDataRawAtVertex(Row, ElementIndex)) + DatumIndex));
			};
			break;
		case ERealtimeMeshDatumType::Int16:
			DatumReader = [](const FRealtimeMeshStream& Stream, int32 Row, int32 ElementIndex,
			                 int32 DatumIndex) -> std::string
			{
				return std::to_string(
					*(reinterpret_cast<const int16*>(Stream.GetDataRawAtVertex(Row, ElementIndex)) + DatumIndex));
			};
			break;
		case ERealtimeMeshDatumType::UInt32:
			DatumReader = [](const FRealtimeMeshStream& Stream, int32 Row, int32 ElementIndex,
			                 int32 DatumIndex) -> std::string
			{
				return std::to_string(
					*(reinterpret_cast<const uint32*>(Stream.GetDataRawAtVertex(Row, ElementIndex)) + DatumIndex));
			};
			break;
		case ERealtimeMeshDatumType::Int32:
			DatumReader = [](const FRealtimeMeshStream& Stream, int32 Row, int32 ElementIndex,
			                 int32 DatumIndex) -> std::string
			{
				return std::to_string(
					*(reinterpret_cast<const int32*>(Stream.GetDataRawAtVertex(Row, ElementIndex)) + DatumIndex));
			};
			break;

		case ERealtimeMeshDatumType::Half:
			DatumReader = [](const FRealtimeMeshStream& Stream, int32 Row, int32 ElementIndex,
			                 int32 DatumIndex) -> std::string
			{
				return std::to_string(
					static_cast<float>(*(reinterpret_cast<const FFloat16*>(Stream.GetDataRawAtVertex(Row, ElementIndex))
						+ DatumIndex)));
			};
			break;
		case ERealtimeMeshDatumType::Float:
			DatumReader = [](const FRealtimeMeshStream& Stream, int32 Row, int32 ElementIndex,
			                 int32 DatumIndex) -> std::string
			{
				return std::to_string(
					*(reinterpret_cast<const float*>(Stream.GetDataRawAtVertex(Row, ElementIndex)) + DatumIndex));
			};
			break;
		case ERealtimeMeshDatumType::Double:
			DatumReader = [](const FRealtimeMeshStream& Stream, int32 Row, int32 ElementIndex,
			                 int32 DatumIndex) -> std::string
			{
				return std::to_string(
					*(reinterpret_cast<const double*>(Stream.GetDataRawAtVertex(Row, ElementIndex)) + DatumIndex));
			};
			break;

		/*case ERealtimeMeshDatumType::RGB10A2:
		 DatumReader = [](const FRealtimeMeshStream& Stream, int32 Row, int32 ElementIndex, int32 DatumIndex) -> std::string
		 {
		         return std::to_string(*(reinterpret_cast<const FPackedRGB10A2N*>(Stream.GetDataRawAtVertex(Row, ElementIndex)) + DatumIndex));
		 };*/
		default:
			return "{ Unable to view data type }";
		}

		int32 NumDatums = Stream.GetLayout().GetElementType().GetNumDatums();

		std::string Result = "{ X:" + DatumReader(Stream, Row, Element, 0);

		if (NumDatums > 1)
		{
			Result += ", Y:" + DatumReader(Stream, Row, Element, 1);

			if (NumDatums > 2)
			{
				Result += ", Z:" + DatumReader(Stream, Row, Element, 2);

				if (NumDatums > 3)
				{
					Result += ", W:" + DatumReader(Stream, Row, Element, 3);
				}
			}
		}

		Result += " }";
		return Result;
	}

	std::string GetRowAsString(const FRealtimeMeshStream& Stream, int32 Row) noexcept
	{
		if (Stream.GetLayout().GetNumElements() > 1)
		{
			std::string Result = "{ ";
			for (int32 Element = 0; Element < Stream.GetLayout().GetNumElements(); ++Element)
			{
				Result += GetRowElementAsString(Stream, Row, Element);
				if (Element < Stream.GetLayout().GetNumElements() - 1)
				{
					Result += ", ";
				}
			}

			Result += " }";
			return Result;
		}

		return GetRowElementAsString(Stream, Row, 0);
	}
}


void RealtimeMesh::FRealtimeMeshStreamLinkage::HandleStreamRemoved(FRealtimeMeshStream* Stream)
{
	RemoveStream(Stream);
}

void RealtimeMesh::FRealtimeMeshStreamLinkage::HandleAllocatedSizeChanged(FRealtimeMeshStream* Stream, int32 NewSize)
{
	ForEachStream([&](FRealtimeMeshStream& CurrentStream, const FRealtimeMeshStreamDefaultRowValue& DefaultValue)
	{
		if (&CurrentStream != Stream)
		{
			check(CurrentStream.ArrayNum <= NewSize);
			CurrentStream.ResizeAllocation(NewSize);
		}
	});
	CheckStreams();
}

void RealtimeMesh::FRealtimeMeshStreamLinkage::HandleNumChanged(FRealtimeMeshStream* Stream, int32 NewNum)
{
	ForEachStream([&](FRealtimeMeshStream& CurrentStream, const FRealtimeMeshStreamDefaultRowValue& DefaultValue)
	{
		if (&CurrentStream != Stream)
		{
			check(NewNum <= CurrentStream.ArrayMax);
			const int32 ExistingNum = CurrentStream.Num();
			const int32 NumAdded = NewNum - ExistingNum;
			CurrentStream.ArrayNum = NewNum;

			if (NumAdded > 0)
			{
				CurrentStream.FillRange(ExistingNum, NumAdded, DefaultValue);
			}
		}
	});
	CheckStreams();
}

#if DO_CHECK
void RealtimeMesh::FRealtimeMeshStreamLinkage::CheckStreams()
{
	bool bIsFirst = true;
	int32 MaxSize = INDEX_NONE;
	int32 Num = INDEX_NONE;

	ForEachStream([&](FRealtimeMeshStream& CurrentStream, const FRealtimeMeshStreamDefaultRowValue& DefaultValue)
	{
		check(CurrentStream.Linkage == this);

		if (bIsFirst)
		{
			MaxSize = CurrentStream.ArrayMax;
			Num = CurrentStream.ArrayNum;
			bIsFirst = false;
		}
		else
		{
			check(CurrentStream.ArrayMax == MaxSize);
			check(CurrentStream.ArrayNum == Num);
		}
	});
}
#endif // DO_CHECK

RealtimeMesh::FRealtimeMeshStreamLinkage::~FRealtimeMeshStreamLinkage()
{
	CheckStreams();
	while (LinkedStreams.Num() > 0)
	{
		LinkedStreams[0].Stream->UnLink();
	}
}


bool RealtimeMesh::FRealtimeMeshStreamLinkage::ContainsStream(const FRealtimeMeshStream* Stream) const
{
	return LinkedStreams.FindByPredicate([&](const FStreamLinkageInfo& Linkage)
	{
		return Linkage.Stream == Stream;
	}) != nullptr;
}

bool RealtimeMesh::FRealtimeMeshStreamLinkage::ContainsStream(const FRealtimeMeshStream& Stream) const
{
	return ContainsStream(&Stream);
}

void RealtimeMesh::FRealtimeMeshStreamLinkage::BindStream(FRealtimeMeshStream* Stream,
                                                          const FRealtimeMeshStreamDefaultRowValue& DefaultValue)
{
	check(Stream && Stream->Linkage == nullptr);

	check(LinkedStreams.FindByPredicate([&](const FStreamLinkageInfo& Linkage) { return Linkage.Stream == Stream; }) ==
		nullptr);
	if (LinkedStreams.Num() > 0)
	{
		const int32 TargetNum = LinkedStreams[0].Stream->ArrayNum;
		const int32 ExistingNum = Stream->Num();
		Stream->ResizeAllocation(LinkedStreams[0].Stream->ArrayMax);
		Stream->SetNumUninitialized(TargetNum, false);

		// Newly grown rows must adopt this stream's default row value (matching the
		// per-row growth path in HandleNumChanged), not a raw zero-fill. Otherwise a
		// late-bound color stream, whose default is white, would come up as
		// transparent black. FillRange falls back to zeroing when no default is set.
		if (TargetNum > ExistingNum)
		{
			Stream->FillRange(ExistingNum, TargetNum - ExistingNum, DefaultValue);
		}
	}

	LinkedStreams.Emplace(Stream, DefaultValue);
	Stream->Linkage = this;
	CheckStreams();
}

void RealtimeMesh::FRealtimeMeshStreamLinkage::BindStream(FRealtimeMeshStream& Stream,
                                                          const FRealtimeMeshStreamDefaultRowValue& DefaultValue)
{
	BindStream(&Stream, DefaultValue);
}

void RealtimeMesh::FRealtimeMeshStreamLinkage::RemoveStream(const FRealtimeMeshStream* Stream)
{
	check(Stream && Stream->Linkage == this);

	const_cast<FRealtimeMeshStream*>(Stream)->Linkage = nullptr;
	LinkedStreams.RemoveAll([&](const FStreamLinkageInfo& Linkage) { return Linkage.Stream == Stream; });
	CheckStreams();
}

void RealtimeMesh::FRealtimeMeshStreamLinkage::RemoveStream(const FRealtimeMeshStream& Stream)
{
	RemoveStream(&Stream);
}

int32 RealtimeMesh::FRealtimeMeshStreamLinkage::GetNumRows() const
{
	if (LinkedStreams.Num() == 0)
	{
		return 0;
	}
	// All linked streams share Num by invariant.
	return LinkedStreams[0].Stream->ArrayNum;
}

void RealtimeMesh::FRealtimeMeshStreamLinkage::ResizeAllStreamAllocations(int32 NewCapacityRows)
{
	for (FStreamLinkageInfo& Info : LinkedStreams)
	{
		FRealtimeMeshStream* Stream = Info.Stream;
		if (NewCapacityRows != Stream->ArrayMax)
		{
			// Direct byte resize via the stream's allocator. We're the canonical
			// path here — skip the per-stream broadcast that the public
			// FRealtimeMeshStream::ResizeAllocation would do.
			Stream->Allocator.ResizeAllocation(Stream->ArrayNum, NewCapacityRows, Stream->Stride, Stream->Alignment);
			Stream->ArrayMax = NewCapacityRows;
		}
	}
}

int32 RealtimeMesh::FRealtimeMeshStreamLinkage::AddRowsUninitialized(int32 NumRows)
{
	check(NumRows >= 0);
	if (NumRows == 0 || LinkedStreams.Num() == 0)
	{
		return LinkedStreams.Num() ? LinkedStreams[0].Stream->ArrayNum : 0;
	}

	FRealtimeMeshStream* Leader = LinkedStreams[0].Stream;
	const int32 OldNum = Leader->ArrayNum;
	const int32 NewNum = OldNum + NumRows;

	if (NewNum > Leader->ArrayMax)
	{
		// Compute the new capacity once via the leader's allocator slack
		// heuristic, in row units. Same row capacity gets applied to every
		// member (each at its own stride / alignment).
		const int32 NewCapacityRows = Leader->Allocator.CalculateSlackGrow(NewNum, Leader->ArrayMax, Leader->Stride, Leader->Alignment);
		ResizeAllStreamAllocations(NewCapacityRows);
	}

	// Bump ArrayNum on every member in one pass. No broadcasts.
	for (FStreamLinkageInfo& Info : LinkedStreams)
	{
		Info.Stream->ArrayNum = NewNum;
	}

	return OldNum;
}

int32 RealtimeMesh::FRealtimeMeshStreamLinkage::AddRowsZeroed(int32 NumRows)
{
	const int32 StartIdx = AddRowsUninitialized(NumRows);
	if (NumRows > 0)
	{
		for (FStreamLinkageInfo& Info : LinkedStreams)
		{
			Info.Stream->FillRange(StartIdx, NumRows, Info.DefaultRowValue);
		}
	}
	return StartIdx;
}

int32 RealtimeMesh::FRealtimeMeshStreamLinkage::SetNumRows(int32 NewNum)
{
	check(NewNum >= 0);
	if (LinkedStreams.Num() == 0)
	{
		return 0;
	}

	FRealtimeMeshStream* Leader = LinkedStreams[0].Stream;
	const int32 OldNum = Leader->ArrayNum;

	if (NewNum > OldNum)
	{
		AddRowsZeroed(NewNum - OldNum);
	}
	else if (NewNum < OldNum)
	{
		RemoveRows(NewNum, OldNum - NewNum);
	}
	return OldNum;
}

void RealtimeMesh::FRealtimeMeshStreamLinkage::RemoveRows(int32 StartIdx, int32 Count, bool bAllowShrinking)
{
	check(StartIdx >= 0 && Count >= 0);
	if (Count == 0 || LinkedStreams.Num() == 0)
	{
		return;
	}

	FRealtimeMeshStream* Leader = LinkedStreams[0].Stream;
	check(StartIdx + Count <= Leader->ArrayNum);

	const int32 NumToMove = Leader->ArrayNum - StartIdx - Count;

	for (FStreamLinkageInfo& Info : LinkedStreams)
	{
		FRealtimeMeshStream* Stream = Info.Stream;
		if (NumToMove > 0)
		{
			uint8* Base = reinterpret_cast<uint8*>(Stream->Allocator.GetAllocation());
			FMemory::Memmove(
				Base + StartIdx * Stream->Stride,
				Base + (StartIdx + Count) * Stream->Stride,
				NumToMove * Stream->Stride);
		}
		Stream->ArrayNum -= Count;
	}

	if (bAllowShrinking)
	{
		const int32 NewCapacity = Leader->Allocator.CalculateSlackShrink(Leader->ArrayNum, Leader->ArrayMax, Leader->Stride, Leader->Alignment);
		if (NewCapacity != Leader->ArrayMax)
		{
			ResizeAllStreamAllocations(NewCapacity);
		}
	}
}

void RealtimeMesh::FRealtimeMeshStreamLinkage::ReserveRows(int32 NumRows)
{
	check(NumRows >= 0);
	if (LinkedStreams.Num() == 0)
	{
		return;
	}
	FRealtimeMeshStream* Leader = LinkedStreams[0].Stream;
	if (NumRows > Leader->ArrayMax)
	{
		ResizeAllStreamAllocations(NumRows);
	}
}
