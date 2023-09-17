// Copyright TriAxis Games, L.L.C. All Rights Reserved.


#include "Data/RealtimeMeshDataConversion.h"


namespace RealtimeMesh
{
	TMap<FRealtimeMeshElementConversionKey, FRealtimeMeshElementConverters> FRealtimeMeshTypeConversionUtilities::TypeConversionMap;

	const FRealtimeMeshElementConverters& FRealtimeMeshTypeConversionUtilities::GetTypeConverter(const FRealtimeMeshElementType& FromType, const FRealtimeMeshElementType& ToType)
	{
		return TypeConversionMap.FindChecked(FRealtimeMeshElementConversionKey(FromType, ToType));
	}

	void FRealtimeMeshTypeConversionUtilities::RegisterTypeConverter(const FRealtimeMeshElementType& FromType, const FRealtimeMeshElementType& ToType,
	                                                                 const FRealtimeMeshElementConverters& Converters)
	{
		TypeConversionMap.Add(FRealtimeMeshElementConversionKey(FromType, ToType), Converters);
	}

	void FRealtimeMeshTypeConversionUtilities::UnregisterTypeConverter(const FRealtimeMeshElementType& FromType, const FRealtimeMeshElementType& ToType)
	{
		TypeConversionMap.Remove(FRealtimeMeshElementConversionKey(FromType, ToType));
	}


	// UInt16 
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
	RMC_DEFINE_ELEMENT_TYPE_CONVERTER(FPackedNormal, FVector4f, { *static_cast<FVector4f*>(Destination) = (*static_cast<FPackedNormal*>(Source)).ToFVector4f(); });
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
	RMC_DEFINE_ELEMENT_TYPE_CONVERTER(FLinearColor, FColor, { *static_cast<FColor*>(Destination) = static_cast<FLinearColor*>(Source)->ToFColorSRGB(); })
}
