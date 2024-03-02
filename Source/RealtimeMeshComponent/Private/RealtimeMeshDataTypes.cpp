// Copyright TriAxis Games, L.L.C. All Rights Reserved.


#include "Mesh/RealtimeMeshDataTypes.h"


namespace RealtimeMesh
{
	const FRealtimeMeshElementType FRealtimeMeshElementType::Invalid(ERealtimeMeshDatumType::Unknown, 0);
	const FRealtimeMeshBufferLayout FRealtimeMeshBufferLayout::Invalid(FRealtimeMeshElementType::Invalid, 0);

	/*const FRealtimeMeshElementTypeDefinition FRealtimeMeshElementTypeDefinition::Invalid(VET_None, IET_None, PF_Unknown, 0, 0);
	const FRealtimeMeshBufferLayoutDefinition FRealtimeMeshBufferLayoutDefinition::Invalid(FRealtimeMeshBufferLayout::Invalid, FRealtimeMeshElementTypeDefinition::Invalid);
	*/

	const TMap<FRealtimeMeshElementType, FRealtimeMeshElementTypeDetails> FRealtimeMeshBufferLayoutUtilities::SupportedTypeDefinitions =
	{
		{
			FRealtimeMeshElementType(ERealtimeMeshDatumType::Float, 1),
			FRealtimeMeshElementTypeDetails(VET_Float1, IET_None, PF_R32_FLOAT, sizeof(float), alignof(float))
		},
		{
			FRealtimeMeshElementType(ERealtimeMeshDatumType::Float, 2),
			FRealtimeMeshElementTypeDetails(VET_Float2, IET_None, PF_R32_FLOAT, sizeof(FVector2f), alignof(float))
		},
		{
			FRealtimeMeshElementType(ERealtimeMeshDatumType::Float, 3),
			FRealtimeMeshElementTypeDetails(VET_Float3, IET_None, PF_R32_FLOAT, sizeof(FVector3f), alignof(float))
		},
		{
			FRealtimeMeshElementType(ERealtimeMeshDatumType::Float, 4),
			FRealtimeMeshElementTypeDetails(VET_Float4, IET_None, PF_R32_FLOAT, sizeof(FVector4f), alignof(float))
		},
		{
			FRealtimeMeshElementType(ERealtimeMeshDatumType::Int8Float, 4),
			FRealtimeMeshElementTypeDetails(VET_PackedNormal, IET_None, PF_R8G8B8A8_SNORM, sizeof(FPackedNormal), alignof(FPackedNormal))
		},
		{
			FRealtimeMeshElementType(ERealtimeMeshDatumType::UInt8, 4),
			FRealtimeMeshElementTypeDetails(VET_UByte4, IET_None, PF_R8G8B8A8_UINT, sizeof(uint8) * 4, alignof(uint8))
		},
		{
			FRealtimeMeshElementType(ERealtimeMeshDatumType::UInt8, 4),
			FRealtimeMeshElementTypeDetails(VET_UByte4N, IET_None, PF_R8G8B8A8_UINT, sizeof(uint8) * 4, alignof(uint8))
		},
		{
			FRealtimeMeshElementType(ERealtimeMeshDatumType::UInt8, 4),
			FRealtimeMeshElementTypeDetails(VET_Color, IET_None, PF_R8G8B8A8, sizeof(uint8) * 4, alignof(uint8))
		},
		{
			FRealtimeMeshElementType(ERealtimeMeshDatumType::Int16, 2),
			FRealtimeMeshElementTypeDetails(VET_Short2, IET_None, PF_R16G16_UINT, sizeof(int16) * 2, alignof(int16))
		},
		{
			FRealtimeMeshElementType(ERealtimeMeshDatumType::Int16, 4),
			FRealtimeMeshElementTypeDetails(VET_Short4, IET_None, PF_R16G16B16A16_SINT, sizeof(int16) * 4, alignof(int16))
		},
		{
			FRealtimeMeshElementType(ERealtimeMeshDatumType::Int16, 2),
			FRealtimeMeshElementTypeDetails(VET_Short2N, IET_None, PF_G16R16_SNORM, sizeof(int16) * 2, alignof(int16))
		},
		{
			FRealtimeMeshElementType(ERealtimeMeshDatumType::Half, 2),
			FRealtimeMeshElementTypeDetails(VET_Half2, IET_None, PF_G16R16F, sizeof(FFloat16) * 2, alignof(FFloat16))
		},
		{
			FRealtimeMeshElementType(ERealtimeMeshDatumType::Half, 4),
			FRealtimeMeshElementTypeDetails(VET_Half4, IET_None, PF_FloatRGBA, sizeof(FFloat16) * 4, alignof(FFloat16))
		},
		{
			FRealtimeMeshElementType(ERealtimeMeshDatumType::Int16, 4),
			FRealtimeMeshElementTypeDetails(VET_Short4N, IET_None, PF_R16G16B16A16_SNORM, sizeof(int16) * 4, alignof(int16))
		},
		{
			FRealtimeMeshElementType(ERealtimeMeshDatumType::UInt16, 2),
			FRealtimeMeshElementTypeDetails(VET_UShort2, IET_None, PF_R16G16_UINT, sizeof(uint16) * 2, alignof(uint16))
		},
		{
			FRealtimeMeshElementType(ERealtimeMeshDatumType::UInt16, 4),
			FRealtimeMeshElementTypeDetails(VET_UShort4, IET_None, PF_R16G16B16A16_UINT, sizeof(uint16) * 4, alignof(uint16))
		},
		{
			FRealtimeMeshElementType(ERealtimeMeshDatumType::UInt16, 2),
			FRealtimeMeshElementTypeDetails(VET_UShort2N, IET_None, PF_G16R16, sizeof(uint16) * 2, alignof(uint16))
		},
		{
			FRealtimeMeshElementType(ERealtimeMeshDatumType::UInt16, 4),
			FRealtimeMeshElementTypeDetails(VET_UShort4N, IET_None, PF_R16G16B16A16_UNORM, sizeof(uint16) * 4, alignof(uint16))
		},
		{
			FRealtimeMeshElementType(ERealtimeMeshDatumType::RGB10A2, 1),
			FRealtimeMeshElementTypeDetails(VET_URGB10A2N, IET_None, PF_A2B10G10R10, sizeof(uint32), alignof(uint32))
		},
		{
			FRealtimeMeshElementType(ERealtimeMeshDatumType::Int16, 1),
			FRealtimeMeshElementTypeDetails(VET_None, IET_Int16, PF_R16_SINT, sizeof(int16), alignof(int16))
		},
		{
			FRealtimeMeshElementType(ERealtimeMeshDatumType::UInt16, 1),
			FRealtimeMeshElementTypeDetails(VET_None, IET_UInt16, PF_R16_UINT, sizeof(uint16), alignof(uint16))
		},
		{
			FRealtimeMeshElementType(ERealtimeMeshDatumType::Int32, 1),
			FRealtimeMeshElementTypeDetails(VET_None, IET_Int32, PF_R32_SINT, sizeof(int32), alignof(int32))
		},
		{
			FRealtimeMeshElementType(ERealtimeMeshDatumType::UInt32, 1),
			FRealtimeMeshElementTypeDetails(VET_UInt, IET_UInt32, PF_R32_UINT, sizeof(uint32), alignof(uint32))
		},
		{
			FRealtimeMeshElementType(ERealtimeMeshDatumType::Int32, 2),
			FRealtimeMeshElementTypeDetails(VET_None, IET_None, PF_Unknown, sizeof(int32), alignof(int32))
		},
		{
			FRealtimeMeshElementType(ERealtimeMeshDatumType::Int32, 3),
			FRealtimeMeshElementTypeDetails(VET_None, IET_None, PF_R32G32B32_SINT, sizeof(int32), alignof(int32))
		},
	};

	//TMap<FRealtimeMeshElementConversionKey, FRealtimeMeshElementConverters> FRealtimeMeshBufferLayoutUtilities::TypeConversionMap;


	FName FRealtimeMeshBufferLayoutUtilities::GetRealtimeMeshDatumTypeName(ERealtimeMeshDatumType Datum)
	{
		switch (Datum)
		{
		case ERealtimeMeshDatumType::UInt8:
			return "UInt8";
		case ERealtimeMeshDatumType::Int8:
			return "Int8";

		case ERealtimeMeshDatumType::UInt16:
			return "UInt16";
		case ERealtimeMeshDatumType::Int16:
			return "Int16";

		case ERealtimeMeshDatumType::UInt32:
			return "UInt32";
		case ERealtimeMeshDatumType::Int32:
			return "Int32";

		case ERealtimeMeshDatumType::Half:
			return "Half";
		case ERealtimeMeshDatumType::Float:
			return "Float";
		case ERealtimeMeshDatumType::Double:
			return "Double";

		case ERealtimeMeshDatumType::RGB10A2:
			return "RGB10A2";

		case ERealtimeMeshDatumType::Unknown:
		default:
			return "Unknown";
		}
	}

	int32 FRealtimeMeshBufferLayoutUtilities::GetRealtimeMeshDatumTypeSize(ERealtimeMeshDatumType Datum)
	{
		switch (Datum)
		{
		case ERealtimeMeshDatumType::UInt8:
			return sizeof(uint8);
		case ERealtimeMeshDatumType::Int8:
		case ERealtimeMeshDatumType::Int8Float:
			return sizeof(int8);

		case ERealtimeMeshDatumType::UInt16:
			return sizeof(uint16);
		case ERealtimeMeshDatumType::Int16:
			return sizeof(int16);

		case ERealtimeMeshDatumType::UInt32:
			return sizeof(uint32);
		case ERealtimeMeshDatumType::Int32:
			return sizeof(int32);

		case ERealtimeMeshDatumType::Half:
			return sizeof(FFloat16);
		case ERealtimeMeshDatumType::Float:
			return sizeof(float);
		case ERealtimeMeshDatumType::Double:
			return sizeof(double);

		case ERealtimeMeshDatumType::RGB10A2:
			return sizeof(uint32);

		case ERealtimeMeshDatumType::Unknown:
		default:
			return 0;
		}
	}

	int32 FRealtimeMeshBufferLayoutUtilities::GetRealtimeMeshDatumTypeStride(ERealtimeMeshDatumType Datum)
	{
		switch (Datum)
		{
		case ERealtimeMeshDatumType::UInt8:
			return alignof(uint8);
		case ERealtimeMeshDatumType::Int8:
		case ERealtimeMeshDatumType::Int8Float:
			return alignof(int8);

		case ERealtimeMeshDatumType::UInt16:
			return alignof(uint16);
		case ERealtimeMeshDatumType::Int16:
			return alignof(int16);

		case ERealtimeMeshDatumType::UInt32:
			return alignof(uint32);
		case ERealtimeMeshDatumType::Int32:
			return alignof(int32);

		case ERealtimeMeshDatumType::Half:
			return alignof(FFloat16);
		case ERealtimeMeshDatumType::Float:
			return alignof(float);
		case ERealtimeMeshDatumType::Double:
			return alignof(double);

		case ERealtimeMeshDatumType::RGB10A2:
			return alignof(uint32);

		case ERealtimeMeshDatumType::Unknown:
		default:
			return 0;
		}
	}

	/*bool FRealtimeMeshBufferLayoutUtilities::IsSupportedType(const FRealtimeMeshElementType& VertexType)
	{
		return SupportedTypeDefinitions.Contains(VertexType);
	}

	bool FRealtimeMeshBufferLayoutUtilities::IsSupportedVertexType(const FRealtimeMeshElementType& VertexType)
	{
		return SupportedTypeDefinitions.Contains(VertexType) && SupportedTypeDefinitions[VertexType].IsSupportedVertexType();
	}

	bool FRealtimeMeshBufferLayoutUtilities::IsSupportedIndexType(const FRealtimeMeshElementType& IndexType)
	{
		return SupportedTypeDefinitions.Contains(IndexType) && SupportedTypeDefinitions[IndexType].IsSupportedIndexType();
	}*/

	/*bool FRealtimeMeshBufferLayoutUtilities::Is32BitIndex(const FRealtimeMeshElementTypeDefinition& IndexType)
	{
		check(IndexType.IsSupportedIndexType());
		return IndexType.GetIndexType() == IET_UInt32 || IndexType.GetIndexType() == IET_Int32;
	}*/

	/*bool FRealtimeMeshBufferLayoutUtilities::HasDoubleWidthVertexType(const FRealtimeMeshElementType& VertexType)
	{
		// Easy out, we can't double types with more than 2 elements as the max is 4
		if (VertexType.GetNumDatums() > 2)
		{
			return false;
		}

		return SupportedTypeDefinitions.Contains(GetDoubleWidthVertexType(VertexType));
	}

	FRealtimeMeshElementType FRealtimeMeshBufferLayoutUtilities::GetDoubleWidthVertexType(const FRealtimeMeshElementType& VertexType)
	{
		return FRealtimeMeshElementType(VertexType.GetDatumType(), VertexType.GetNumDatums() * 2);
	}*/

	int32 FRealtimeMeshBufferLayoutUtilities::GetElementStride(const FRealtimeMeshElementType& ElementType)
	{
		return GetRealtimeMeshDatumTypeSize(ElementType.GetDatumType()) * ElementType.GetNumDatums();
	}

	int32 FRealtimeMeshBufferLayoutUtilities::GetElementAlignment(const FRealtimeMeshElementType& ElementType)
	{
		return GetRealtimeMeshDatumTypeStride(ElementType.GetDatumType());
	}

	FRealtimeMeshBufferMemoryLayout FRealtimeMeshBufferLayoutUtilities::GetBufferLayoutMemoryLayout(const FRealtimeMeshBufferLayout& BufferLayout)
	{
		FRealtimeMeshElementTypeDetails ElementDetails = GetElementTypeDetails(BufferLayout.GetElementType());
		if (ElementDetails.IsValid())
		{
			return FRealtimeMeshBufferMemoryLayout(ElementDetails.GetStride(), ElementDetails.GetAlignment(), BufferLayout.GetNumElements());
		}
		return FRealtimeMeshBufferMemoryLayout();	
	}

	FRealtimeMeshElementTypeDetails FRealtimeMeshBufferLayoutUtilities::GetElementTypeDetails(const FRealtimeMeshElementType& ElementType)
	{
		if (const auto* FoundEntry = SupportedTypeDefinitions.Find(ElementType))
		{
			return *FoundEntry;
		}
		return FRealtimeMeshElementTypeDetails();
	}

	FRealtimeMeshElementTypeDetails FRealtimeMeshBufferLayoutUtilities::GetElementTypeDetails(const FRealtimeMeshBufferLayout& BufferLayout)
	{
		return GetElementTypeDetails(BufferLayout.GetElementType());
	}

	/*const FRealtimeMeshElementTypeDefinition& FRealtimeMeshBufferLayoutUtilities::GetTypeDefinition(const FRealtimeMeshElementType& Type)
	{
		if (SupportedTypeDefinitions.Contains(Type))
		{
			return SupportedTypeDefinitions.FindChecked(Type);
		}
		return FRealtimeMeshElementTypeDefinition::Invalid;
	}

	FRealtimeMeshBufferLayoutDefinition FRealtimeMeshBufferLayoutUtilities::GetBufferLayoutDefinition(const FRealtimeMeshBufferLayout& BufferLayout)
	{
		return FRealtimeMeshBufferLayoutDefinition(BufferLayout, GetTypeDefinition(BufferLayout.GetElementType()));
	}*/

	/*const FRealtimeMeshElementConverters& FRealtimeMeshBufferLayoutUtilities::GetTypeConverter(const FRealtimeMeshElementType& FromType, const FRealtimeMeshElementType& ToType)
	{
		return TypeConversionMap.FindChecked(FRealtimeMeshElementConversionKey(FromType, ToType));		
	}

	void FRealtimeMeshBufferLayoutUtilities::RegisterTypeConverter(const FRealtimeMeshElementType& FromType, const FRealtimeMeshElementType& ToType,
		FRealtimeMeshElementConverters Converters)
	{
		TypeConversionMap.Add(FRealtimeMeshElementConversionKey(FromType, ToType), Converters);
	}

	void FRealtimeMeshBufferLayoutUtilities::UnregisterTypeConverter(const FRealtimeMeshElementType& FromType, const FRealtimeMeshElementType& ToType)
	{
		TypeConversionMap.Remove(FRealtimeMeshElementConversionKey(FromType, ToType));
	}*/


	/*// UInt16 
	RMC_DEFINE_ELEMENT_TYPE_CONVERTER_TRIVIAL(uint16, uint16);
	RMC_DEFINE_ELEMENT_TYPE_CONVERTER_TRIVIAL(uint16, int16);
	RMC_DEFINE_ELEMENT_TYPE_CONVERTER_TRIVIAL(uint16, uint32);
	RMC_DEFINE_ELEMENT_TYPE_CONVERTER_TRIVIAL(uint16, int32);
	RMC_DEFINE_ELEMENT_TYPE_CONVERTER_TRIVIAL(uint16, float);
	RMC_DEFINE_ELEMENT_TYPE_CONVERTER_TRIVIAL(uint16, FFloat16);

	
	// Int16
	RMC_DEFINE_ELEMENT_TYPE_CONVERTER_TRIVIAL(int16, uint16);
	RMC_DEFINE_ELEMENT_TYPE_CONVERTER_TRIVIAL(int16, int16);
	RMC_DEFINE_ELEMENT_TYPE_CONVERTER_TRIVIAL(int16, uint32);
	RMC_DEFINE_ELEMENT_TYPE_CONVERTER_TRIVIAL(int16, int32);
	RMC_DEFINE_ELEMENT_TYPE_CONVERTER_TRIVIAL(int16, float);
	RMC_DEFINE_ELEMENT_TYPE_CONVERTER_TRIVIAL(int16, FFloat16);

	// UInt32
	RMC_DEFINE_ELEMENT_TYPE_CONVERTER_TRIVIAL(uint32, uint16);
	RMC_DEFINE_ELEMENT_TYPE_CONVERTER_TRIVIAL(uint32, int16);
	RMC_DEFINE_ELEMENT_TYPE_CONVERTER_TRIVIAL(uint32, uint32);
	RMC_DEFINE_ELEMENT_TYPE_CONVERTER_TRIVIAL(uint32, int32);
	RMC_DEFINE_ELEMENT_TYPE_CONVERTER_TRIVIAL(uint32, float);
	RMC_DEFINE_ELEMENT_TYPE_CONVERTER_TRIVIAL(uint32, FFloat16);

	// Int32
	RMC_DEFINE_ELEMENT_TYPE_CONVERTER_TRIVIAL(int32, uint16);
	RMC_DEFINE_ELEMENT_TYPE_CONVERTER_TRIVIAL(int32, int16);
	RMC_DEFINE_ELEMENT_TYPE_CONVERTER_TRIVIAL(int32, uint32);
	RMC_DEFINE_ELEMENT_TYPE_CONVERTER_TRIVIAL(int32, int32);
	RMC_DEFINE_ELEMENT_TYPE_CONVERTER_TRIVIAL(int32, float);
	RMC_DEFINE_ELEMENT_TYPE_CONVERTER_TRIVIAL(int32, FFloat16);

	// float
	RMC_DEFINE_ELEMENT_TYPE_CONVERTER_TRIVIAL(float, float);
	RMC_DEFINE_ELEMENT_TYPE_CONVERTER_TRIVIAL(float, FFloat16);

	// FFloat16
	RMC_DEFINE_ELEMENT_TYPE_CONVERTER_TRIVIAL(FFloat16, float);
	RMC_DEFINE_ELEMENT_TYPE_CONVERTER_TRIVIAL(FFloat16, FFloat16);
	
	// FVector2DHalf
	RMC_DEFINE_ELEMENT_TYPE_CONVERTER_TRIVIAL(FVector2DHalf, FVector2DHalf);
	RMC_DEFINE_ELEMENT_TYPE_CONVERTER_TRIVIAL(FVector2DHalf, FVector2f);
	RMC_DEFINE_ELEMENT_TYPE_CONVERTER_TRIVIAL(FVector2DHalf, FVector2d);
	
	// FVector2f
	RMC_DEFINE_ELEMENT_TYPE_CONVERTER_TRIVIAL(FVector2f, FVector2DHalf);
	RMC_DEFINE_ELEMENT_TYPE_CONVERTER_TRIVIAL(FVector2f, FVector2f);
	RMC_DEFINE_ELEMENT_TYPE_CONVERTER_TRIVIAL(FVector2f, FVector2d);	
	
	// FVector2d
	RMC_DEFINE_ELEMENT_TYPE_CONVERTER_TRIVIAL(FVector2d, FVector2DHalf);
	RMC_DEFINE_ELEMENT_TYPE_CONVERTER_TRIVIAL(FVector2d, FVector2f);
	RMC_DEFINE_ELEMENT_TYPE_CONVERTER_TRIVIAL(FVector2d, FVector2d);

	// FVector3f
	RMC_DEFINE_ELEMENT_TYPE_CONVERTER_TRIVIAL(FVector3f, FVector3f);
	RMC_DEFINE_ELEMENT_TYPE_CONVERTER_TRIVIAL(FVector3f, FVector3d);
	
	// FVector3d
	RMC_DEFINE_ELEMENT_TYPE_CONVERTER_TRIVIAL(FVector3d, FVector3f);
	RMC_DEFINE_ELEMENT_TYPE_CONVERTER_TRIVIAL(FVector3d, FVector3d);

	// FVector4f
	RMC_DEFINE_ELEMENT_TYPE_CONVERTER_TRIVIAL(FVector4f, FVector4f);
	RMC_DEFINE_ELEMENT_TYPE_CONVERTER_TRIVIAL(FVector4f, FVector4d);
	RMC_DEFINE_ELEMENT_TYPE_CONVERTER_TRIVIAL(FVector4f, FPackedNormal);
	RMC_DEFINE_ELEMENT_TYPE_CONVERTER_TRIVIAL(FVector4f, FPackedRGBA16N);
	
	// FVector4d
	RMC_DEFINE_ELEMENT_TYPE_CONVERTER_TRIVIAL(FVector4d, FVector4f);
	RMC_DEFINE_ELEMENT_TYPE_CONVERTER_TRIVIAL(FVector4d, FVector4d);
	RMC_DEFINE_ELEMENT_TYPE_CONVERTER_TRIVIAL(FVector4d, FPackedNormal);
	RMC_DEFINE_ELEMENT_TYPE_CONVERTER_TRIVIAL(FVector4d, FPackedRGBA16N);


	// FPackedNormal
	RMC_DEFINE_ELEMENT_TYPE_CONVERTER(FPackedNormal, FVector4f,	{ *static_cast<FVector4f*>(Destination) = (*static_cast<FPackedNormal*>(Source)).ToFVector4f(); });
	RMC_DEFINE_ELEMENT_TYPE_CONVERTER(FPackedNormal, FVector4d, { *static_cast<FVector4d*>(Destination) = (*static_cast<FPackedNormal*>(Source)).ToFVector4(); });
	RMC_DEFINE_ELEMENT_TYPE_CONVERTER_TRIVIAL(FPackedNormal, FPackedNormal);
	RMC_DEFINE_ELEMENT_TYPE_CONVERTER(FPackedNormal, FPackedRGBA16N, { *static_cast<FPackedRGBA16N*>(Destination) = (*static_cast<FPackedNormal*>(Source)).ToFVector4f(); });
	
	// FPackedRGBA16N	
	RMC_DEFINE_ELEMENT_TYPE_CONVERTER(FPackedRGBA16N, FVector4f, { *static_cast<FVector4f*>(Destination) = (*static_cast<FPackedRGBA16N*>(Source)).ToFVector4f(); });
	RMC_DEFINE_ELEMENT_TYPE_CONVERTER(FPackedRGBA16N, FVector4d, { *static_cast<FVector4d*>(Destination) = (*static_cast<FPackedRGBA16N*>(Source)).ToFVector4(); });
	RMC_DEFINE_ELEMENT_TYPE_CONVERTER(FPackedRGBA16N, FPackedNormal, { *static_cast<FPackedNormal*>(Destination) = (*static_cast<FPackedRGBA16N*>(Source)).ToFVector4f(); });
	RMC_DEFINE_ELEMENT_TYPE_CONVERTER_TRIVIAL(FPackedRGBA16N, FPackedRGBA16N);

	// FColor
	RMC_DEFINE_ELEMENT_TYPE_CONVERTER_TRIVIAL(FColor, FColor);
	RMC_DEFINE_ELEMENT_TYPE_CONVERTER(FColor, FLinearColor, { *static_cast<FLinearColor*>(Destination) = FLinearColor::FromSRGBColor(*static_cast<FColor*>(Source)); })
	
	// FLinearColor
	RMC_DEFINE_ELEMENT_TYPE_CONVERTER_TRIVIAL(FLinearColor, FLinearColor);
	RMC_DEFINE_ELEMENT_TYPE_CONVERTER(FLinearColor, FColor, { *static_cast<FColor*>(Destination) = static_cast<FLinearColor*>(Source)->ToFColorSRGB(); })*/
}
