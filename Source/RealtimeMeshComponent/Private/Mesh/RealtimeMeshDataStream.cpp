// Copyright TriAxis Games, L.L.C. All Rights Reserved.


#include "Mesh/RealtimeMeshDataStream.h"

#include <string>


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

