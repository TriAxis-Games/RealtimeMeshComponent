// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.


#include "Core/RealtimeMeshDataConversion.h"
#include "Misc/ScopeRWLock.h"


namespace RealtimeMesh
{
	TMap<FRealtimeMeshElementConversionKey, FRealtimeMeshElementConverters> FRealtimeMeshTypeConversionUtilities::TypeConversionMap;

	// The conversion registry is populated during static init / module startup and read
	// concurrently at runtime. Guard structural access so a Register/Unregister rehash
	// can't tear a concurrent lookup. Callers reach a converter via ProcessTypeConverter,
	// which holds this read lock across the caller's use of the reference - so no reference
	// into the map ever escapes the lock (a rehash can't invalidate it mid-use).
	static FRWLock GTypeConversionMapLock;

	bool FRealtimeMeshTypeConversionUtilities::CanConvert(const FRealtimeMeshElementType& FromType, const FRealtimeMeshElementType& ToType)
	{
		FRWScopeLock ScopeLock(GTypeConversionMapLock, SLT_ReadOnly);
		return TypeConversionMap.Contains(FRealtimeMeshElementConversionKey(FromType, ToType));
	}

	void FRealtimeMeshTypeConversionUtilities::ProcessTypeConverter(const FRealtimeMeshElementType& FromType, const FRealtimeMeshElementType& ToType, TFunctionRef<void(const FRealtimeMeshElementConverters&)> Func)
	{
		FRWScopeLock ScopeLock(GTypeConversionMapLock, SLT_ReadOnly);
		const FRealtimeMeshElementConverters& Converters = TypeConversionMap.FindChecked(FRealtimeMeshElementConversionKey(FromType, ToType));
		Func(Converters);
	}

	void FRealtimeMeshTypeConversionUtilities::RegisterTypeConverter(const FRealtimeMeshElementType& FromType, const FRealtimeMeshElementType& ToType,
	                                                                 const FRealtimeMeshElementConverters& Converters)
	{
		FRWScopeLock ScopeLock(GTypeConversionMapLock, SLT_Write);
		TypeConversionMap.Add(FRealtimeMeshElementConversionKey(FromType, ToType), Converters);
	}

	void FRealtimeMeshTypeConversionUtilities::UnregisterTypeConverter(const FRealtimeMeshElementType& FromType, const FRealtimeMeshElementType& ToType)
	{
		FRWScopeLock ScopeLock(GTypeConversionMapLock, SLT_Write);
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
	RMC_DEFINE_ELEMENT_TYPE_CONVERTER(FPackedNormal, FVector4f, { Destination = Source.ToFVector4f(); });
	RMC_DEFINE_ELEMENT_TYPE_CONVERTER(FPackedNormal, FVector4d, { Destination = Source.ToFVector4(); });
	RMC_DEFINE_ELEMENT_TYPE_CONVERTER_TRIVIAL(FPackedNormal, FPackedNormal);
	RMC_DEFINE_ELEMENT_TYPE_CONVERTER(FPackedNormal, FPackedRGBA16N, { Destination = Source.ToFVector4f(); });

	// FPackedRGBA16N	
	RMC_DEFINE_ELEMENT_TYPE_CONVERTER(FPackedRGBA16N, FVector4f, { Destination = Source.ToFVector4f(); });
	RMC_DEFINE_ELEMENT_TYPE_CONVERTER(FPackedRGBA16N, FVector4d, { Destination = Source.ToFVector4(); });
	RMC_DEFINE_ELEMENT_TYPE_CONVERTER(FPackedRGBA16N, FPackedNormal, { Destination = Source.ToFVector4f(); });
	RMC_DEFINE_ELEMENT_TYPE_CONVERTER_TRIVIAL(FPackedRGBA16N, FPackedRGBA16N);

	// FColor
	RMC_DEFINE_ELEMENT_TYPE_CONVERTER_TRIVIAL(FColor, FColor);
	RMC_DEFINE_ELEMENT_TYPE_CONVERTER(FColor, FLinearColor, { Destination = FLinearColor::FromSRGBColor(Source); })

	// FLinearColor
	RMC_DEFINE_ELEMENT_TYPE_CONVERTER_TRIVIAL(FLinearColor, FLinearColor);
	RMC_DEFINE_ELEMENT_TYPE_CONVERTER(FLinearColor, FColor, { Destination = Source.ToFColorSRGB(); })
}
