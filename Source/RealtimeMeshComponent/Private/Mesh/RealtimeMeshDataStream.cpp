// Copyright TriAxis Games, L.L.C. All Rights Reserved.


#include "Mesh/RealtimeMeshDataStream.h"

#include <string>

#include "Algo/AllOf.h"


namespace RealtimeMesh::NatVis
{
	std::string GetRowElementAsString(const FRealtimeMeshStream& Stream, int32 Row, int32 Element) noexcept
	{
		TFunction<std::string(const FRealtimeMeshStream&, int32, int32 , int32)> DatumReader;
		switch(Stream.GetLayout().GetElementType().GetDatumType())
		{		
			case ERealtimeMeshDatumType::UInt8:
				DatumReader = [](const FRealtimeMeshStream& Stream, int32 Row, int32 ElementIndex, int32 DatumIndex) -> std::string
				{
					return std::to_string(*(reinterpret_cast<const uint8*>(Stream.GetDataRawAtVertex(Row, ElementIndex)) + sizeof(uint8) * DatumIndex));
				};
				break;
			case ERealtimeMeshDatumType::Int8:
				DatumReader = [](const FRealtimeMeshStream& Stream, int32 Row, int32 ElementIndex, int32 DatumIndex) -> std::string
				{
					return std::to_string(*(reinterpret_cast<const int8*>(Stream.GetDataRawAtVertex(Row, ElementIndex)) + sizeof(int8) * DatumIndex));
				};
				break;
			case ERealtimeMeshDatumType::UInt16:
				DatumReader = [](const FRealtimeMeshStream& Stream, int32 Row, int32 ElementIndex, int32 DatumIndex) -> std::string
				{
					return std::to_string(*(reinterpret_cast<const uint16*>(Stream.GetDataRawAtVertex(Row, ElementIndex)) + sizeof(uint16) * DatumIndex));
				};
				break;
			case ERealtimeMeshDatumType::Int16:
				DatumReader = [](const FRealtimeMeshStream& Stream, int32 Row, int32 ElementIndex, int32 DatumIndex) -> std::string
				{
					return std::to_string(*(reinterpret_cast<const int16*>(Stream.GetDataRawAtVertex(Row, ElementIndex)) + sizeof(int16) * DatumIndex));
				};
				break;
			case ERealtimeMeshDatumType::UInt32:
				DatumReader = [](const FRealtimeMeshStream& Stream, int32 Row, int32 ElementIndex, int32 DatumIndex) -> std::string
				{
					return std::to_string(*(reinterpret_cast<const uint32*>(Stream.GetDataRawAtVertex(Row, ElementIndex)) + sizeof(uint32) * DatumIndex));
				};
				break;
			case ERealtimeMeshDatumType::Int32:
				DatumReader = [](const FRealtimeMeshStream& Stream, int32 Row, int32 ElementIndex, int32 DatumIndex) -> std::string
				{
					return std::to_string(*(reinterpret_cast<const int32*>(Stream.GetDataRawAtVertex(Row, ElementIndex)) + sizeof(int32) * DatumIndex));
				};
				break;

			case ERealtimeMeshDatumType::Half:
				DatumReader = [](const FRealtimeMeshStream& Stream, int32 Row, int32 ElementIndex, int32 DatumIndex) -> std::string
				{
					return std::to_string(*(reinterpret_cast<const FFloat16*>(Stream.GetDataRawAtVertex(Row, ElementIndex)) + sizeof(FFloat16) * DatumIndex));
				};
				break;
			case ERealtimeMeshDatumType::Float:
				DatumReader = [](const FRealtimeMeshStream& Stream, int32 Row, int32 ElementIndex, int32 DatumIndex) -> std::string
				{
					return std::to_string(*(reinterpret_cast<const float*>(Stream.GetDataRawAtVertex(Row, ElementIndex)) + sizeof(float) * DatumIndex));
				};
				break;
			case ERealtimeMeshDatumType::Double:
				DatumReader = [](const FRealtimeMeshStream& Stream, int32 Row, int32 ElementIndex, int32 DatumIndex) -> std::string
				{
					return std::to_string(*(reinterpret_cast<const double*>(Stream.GetDataRawAtVertex(Row, ElementIndex)) + sizeof(double) * DatumIndex));
				};
				break;

			/*case ERealtimeMeshDatumType::RGB10A2:
				DatumReader = [](const FRealtimeMeshStream& Stream, int32 Row, int32 ElementIndex, int32 DatumIndex) -> std::string
				{
					return std::to_string(*(reinterpret_cast<const FPackedRGB10A2N*>(Stream.GetDataRawAtVertex(Row, ElementIndex)) + sizeof(FPackedRGB10A2N) * DatumIndex));
				};*/
		default:
			return "{ Unable to view data type }";
			break;
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
	ForEachStream([&] (FRealtimeMeshStream& CurrentStream, const FRealtimeMeshStreamDefaultRowValue& DefaultValue)
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
	ForEachStream([&] (FRealtimeMeshStream& CurrentStream, const FRealtimeMeshStreamDefaultRowValue& DefaultValue)
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

void RealtimeMesh::FRealtimeMeshStreamLinkage::MatchSizesOnBind()
{
	int32 MaxSize = 0;
	int32 MaxNum = 0;
	
	ForEachStream([&] (FRealtimeMeshStream& CurrentStream, const FRealtimeMeshStreamDefaultRowValue& DefaultValue)
	{
		check(CurrentStream.Linkage == this);
		MaxSize = FMath::Max(MaxSize, CurrentStream.ArrayMax);
		MaxNum = FMath::Max(MaxNum, CurrentStream.ArrayNum);
	});

	check(MaxNum <= MaxSize);
	
	ForEachStream([&] (FRealtimeMeshStream& CurrentStream, const FRealtimeMeshStreamDefaultRowValue& DefaultValue)
	{
		check(CurrentStream.Linkage == this);
		
		if (CurrentStream.ArrayMax != MaxSize)
		{
			CurrentStream.ResizeAllocation(MaxSize);
		}

		if (CurrentStream.ArrayNum != MaxNum)
		{			
			const int32 ExistingNum = CurrentStream.ArrayNum;
			const int32 NumAdded = MaxNum - ExistingNum;
			CurrentStream.ArrayNum = MaxNum;

			if (NumAdded > 0)
			{
				CurrentStream.FillRange(ExistingNum, NumAdded, DefaultValue);
			}
		}
	});
}

void RealtimeMesh::FRealtimeMeshStreamLinkage::CheckStreams()
{
	bool bIsFirst = true;
	int32 MaxSize = INDEX_NONE;
	int32 Num = INDEX_NONE;
	
	ForEachStream([&] (FRealtimeMeshStream& CurrentStream, const FRealtimeMeshStreamDefaultRowValue& DefaultValue)
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
	return LinkedStreams.FindByPredicate([&](const FStreamLinkageInfo& Linkage) { return Linkage.Stream == Stream; }) != nullptr;
}

bool RealtimeMesh::FRealtimeMeshStreamLinkage::ContainsStream(const FRealtimeMeshStream& Stream) const
{
	return ContainsStream(&Stream);
}

void RealtimeMesh::FRealtimeMeshStreamLinkage::BindStream(FRealtimeMeshStream* Stream, const FRealtimeMeshStreamDefaultRowValue& DefaultValue)
{
	check(Stream && Stream->Linkage == nullptr);
	
	check(LinkedStreams.FindByPredicate([&](const FStreamLinkageInfo& Linkage) { return Linkage.Stream == Stream; }) == nullptr);
	LinkedStreams.Emplace(Stream, DefaultValue);
	Stream->Linkage = this;
	MatchSizesOnBind();
	CheckStreams();
}

void RealtimeMesh::FRealtimeMeshStreamLinkage::BindStream(FRealtimeMeshStream& Stream, const FRealtimeMeshStreamDefaultRowValue& DefaultValue)
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
