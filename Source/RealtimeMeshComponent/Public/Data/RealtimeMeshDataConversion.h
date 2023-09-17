// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "RealtimeMeshDataTypes.h"

namespace RealtimeMesh
{
	using FRealtimeMeshSingleElementDataConverter = TFunction<void(void*, void*)>;
	using FRealtimeMeshMultiElementDataConverter = TFunction<void(void*, void*, uint32 Num, uint32 Stride)>;

	
	struct REALTIMEMESHCOMPONENT_API FRealtimeMeshElementConversionKey
	{
		const FRealtimeMeshElementType FromType;
		const FRealtimeMeshElementType ToType;

		FRealtimeMeshElementConversionKey(const FRealtimeMeshElementType& InFromType, const FRealtimeMeshElementType& InToType)
			: FromType(InFromType), ToType(InToType)
		{
		}

		bool operator==(const FRealtimeMeshElementConversionKey& OtherKey) const
		{
			return FromType == OtherKey.FromType && ToType == OtherKey.ToType;
		}

		friend uint32 GetTypeHash(const FRealtimeMeshElementConversionKey& Key)
		{
			return HashCombine(GetTypeHash(Key.FromType), GetTypeHash(Key.ToType));
		}
	};

	struct REALTIMEMESHCOMPONENT_API FRealtimeMeshElementConverters
	{
		const FRealtimeMeshSingleElementDataConverter SingleElementConverter;
		const FRealtimeMeshMultiElementDataConverter ArrayElementConverter;

		FRealtimeMeshElementConverters(const FRealtimeMeshSingleElementDataConverter& InSingleElementConverter,
		                               const FRealtimeMeshMultiElementDataConverter& InArrayElementConverter)
			: SingleElementConverter(InSingleElementConverter), ArrayElementConverter(InArrayElementConverter)
		{
		}
	};

	struct FRealtimeMeshTypeConversionUtilities
	{
	private:
		static TMap<FRealtimeMeshElementConversionKey, FRealtimeMeshElementConverters> TypeConversionMap;

	public:
		static const FRealtimeMeshElementConverters& GetTypeConverter(const FRealtimeMeshElementType& FromType, const FRealtimeMeshElementType& ToType);
		static void RegisterTypeConverter(const FRealtimeMeshElementType& FromType, const FRealtimeMeshElementType& ToType,
		                                  const FRealtimeMeshElementConverters& Converters);
		static void UnregisterTypeConverter(const FRealtimeMeshElementType& FromType, const FRealtimeMeshElementType& ToType);
	};

	template <typename FromType, typename ToType>
	class FRealtimeMeshTypeConverterRegistration : FNoncopyable
	{
		static_assert(FRealtimeMeshElementTypeTraits<FromType>::IsValid);
		static_assert(FRealtimeMeshElementTypeTraits<ToType>::IsValid);

	public:
		FRealtimeMeshTypeConverterRegistration(FRealtimeMeshElementConverters Converters)
		{
			FRealtimeMeshTypeConversionUtilities::RegisterTypeConverter(GetRealtimeMeshDataElementType<FromType>(), GetRealtimeMeshDataElementType<ToType>(), Converters);
		}

		~FRealtimeMeshTypeConverterRegistration()
		{
			FRealtimeMeshTypeConversionUtilities::UnregisterTypeConverter(GetRealtimeMeshDataElementType<FromType>(), GetRealtimeMeshDataElementType<ToType>());
		}
	};


#define RMC_DEFINE_ELEMENT_TYPE_CONVERTER(FromElementType, ToElementType, ElementConverter) \
	FRealtimeMeshTypeConverterRegistration<FromElementType, ToElementType> GRegister##FromElementType##To##ToElementType(FRealtimeMeshElementConverters( \
			[](void* Source, void* Destination) ElementConverter, \
			[](void* SourceArr, void* DestinationArr, uint32 Count, uint32 Stride) { \
				for (uint32 Index = 0; Index < Count; Index++) \
				{ \
					void* Source = static_cast<FromElementType*>(SourceArr) + Index * Stride; \
					void* Destination = static_cast<ToElementType*>(DestinationArr) + Index * Stride; \
					ElementConverter; \
				} \
			} \
		) \
	);

#define RMC_DEFINE_ELEMENT_TYPE_CONVERTER_TRIVIAL(FromElementType, ToElementType) \
	RMC_DEFINE_ELEMENT_TYPE_CONVERTER(FromElementType, ToElementType, { *static_cast<ToElementType*>(Destination) = ToElementType(*static_cast<FromElementType*>(Source)); });
}
