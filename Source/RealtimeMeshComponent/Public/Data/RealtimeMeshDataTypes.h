// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "RealtimeMeshCore.h"

namespace RealtimeMesh
{
	
	enum class ERealtimeMeshDatumType : uint8
	{
		Unknown,
		
		UInt8,
		Int8,
		
		UInt16,
		Int16,
		
		UInt32,
		Int32,

		Half,
		Float,
		Double,

		// Specific type for a tightly packed element
		RGB10A2,
	};

	struct REALTIMEMESHCOMPONENT_API FRealtimeMeshElementType
	{
	private:
		ERealtimeMeshDatumType Type;
		uint8 NumDatums : 3;
		uint8 bNormalized : 1;
		uint8 bShouldConvertToFloat : 1;


	public:
		constexpr FRealtimeMeshElementType() : Type(ERealtimeMeshDatumType::Unknown), NumDatums(0), bNormalized(false), bShouldConvertToFloat(false) {  }
		constexpr FRealtimeMeshElementType(ERealtimeMeshDatumType InType, int32 InNumDatums, bool bInNormalized, bool bInShouldConvertToFloat)
			: Type(InType), NumDatums(InNumDatums), bNormalized(bInNormalized), bShouldConvertToFloat(bInShouldConvertToFloat)
		{
		}

		constexpr bool IsValid() const { return Type != ERealtimeMeshDatumType::Unknown && NumDatums > 0; }
		constexpr ERealtimeMeshDatumType GetDatumType() const { return Type; }
		constexpr uint8 GetNumDatums() const { return NumDatums; }
		constexpr bool IsNormalized() const { return bNormalized; }
		constexpr bool ShouldConvertToFloat() const { return bShouldConvertToFloat; }

		
		constexpr bool operator==(const FRealtimeMeshElementType& Other) const { return Type == Other.Type && NumDatums == Other.NumDatums && bNormalized == Other.bNormalized && bShouldConvertToFloat == Other.bShouldConvertToFloat; }
		constexpr bool operator!=(const FRealtimeMeshElementType& Other) const { return Type != Other.Type || NumDatums != Other.NumDatums || bNormalized != Other.bNormalized || bShouldConvertToFloat != Other.bShouldConvertToFloat; }

		FString ToString() const
		{
			return FString::Printf(TEXT("Type=%d NumDatums=%d IsNormalized=%d ConvertToFloat=%d"), Type, NumDatums, bNormalized, bShouldConvertToFloat);
		}
		
		friend FORCEINLINE uint32 GetTypeHash(const FRealtimeMeshElementType& Element)
		{
			return ::HashCombine(::GetTypeHash(Element.Type), ::GetTypeHash(Element.NumDatums << 2 | Element.bNormalized << 1 | Element.bShouldConvertToFloat));
		}
		
		friend FArchive& operator<<(FArchive& Ar, FRealtimeMeshElementType& ElementType)
		{
			Ar << ElementType.Type;
			
			uint8 TempNumDatums = ElementType.NumDatums;
			Ar << TempNumDatums;
			ElementType.NumDatums = TempNumDatums;

			bool bTempNormalized = ElementType.bNormalized;
			Ar << bTempNormalized;
			ElementType.bNormalized = bTempNormalized;

			bool bTempShouldConvertToFloat = ElementType.bShouldConvertToFloat;
			Ar << bTempShouldConvertToFloat;
			ElementType.bShouldConvertToFloat = bTempShouldConvertToFloat;
			
			return Ar;
		}

		static const FRealtimeMeshElementType Invalid;
	};
	
	static_assert(sizeof(FRealtimeMeshElementType) == sizeof(uint16), "Data must always be 16bit");

	struct REALTIMEMESHCOMPONENT_API FRealtimeMeshBufferLayout
	{
	private:
		FRealtimeMeshElementType ElementType;
		TArray<FName, TInlineAllocator<REALTIME_MESH_MAX_STREAM_ELEMENTS>> Elements;
		int32 NumElements;
		uint32 ElementsHash;
	public:
		FRealtimeMeshBufferLayout(FRealtimeMeshElementType InType, int32 InNumElements)
		: ElementType(InType), NumElements(InNumElements)
		{
			InitHash();
		}
		FRealtimeMeshBufferLayout(FRealtimeMeshElementType InType, TArrayView<FName> InElements)
		: ElementType(InType), Elements(InElements), NumElements(InElements.Num())
		{
			InitHash();
		}
		FRealtimeMeshBufferLayout(FRealtimeMeshElementType InType, TArray<FName> InElements)
			: ElementType(InType), Elements(InElements), NumElements(InElements.Num())
		{
			InitHash();
		}
		FRealtimeMeshBufferLayout(FRealtimeMeshElementType InType, std::initializer_list<FName> InitList)
			: ElementType(InType), Elements(InitList), NumElements(InitList.size())
		{
			InitHash();
		}

		

		bool IsValid() const { return ElementType.IsValid() && NumElements > 0; }
		const FRealtimeMeshElementType& GetElementType() const { return ElementType; }
		int32 GetNumElements() const { return NumElements; }
		bool HasElementIdentifiers() const { return Elements.Num() > 0; }
		TConstArrayView<FName> GetElements() const { return MakeArrayView(Elements); }

		bool operator==(const FRealtimeMeshBufferLayout& Other) const { return ElementType == Other.ElementType && NumElements == Other.NumElements; }
		bool operator!=(const FRealtimeMeshBufferLayout& Other) const { return ElementType != Other.ElementType || NumElements != Other.NumElements; }

		FString ToString() const
		{
			return FString::Printf(TEXT("ElementType=(%s) NumElements=%d Hash=%d NumElements=%d Elements=[%s]"), *ElementType.ToString(),
				Elements.Num(), ElementsHash, NumElements, *FString::JoinBy(Elements, TEXT(","), [](FName Name) -> FString { return Name.ToString(); }));
		}
		
		friend FORCEINLINE uint32 GetTypeHash(const FRealtimeMeshBufferLayout& DataType)
		{
			return HashCombine(GetTypeHash(DataType.ElementType), DataType.ElementsHash);
		}

		friend FArchive& operator<<(FArchive& Ar, FRealtimeMeshBufferLayout& Layout)
		{
			Ar << Layout.ElementType;
			Ar << Layout.Elements;
			Ar << Layout.NumElements;
			if (Ar.IsLoading())
			{
				Layout.InitHash();
			}
			return Ar;
		}
		
		static const FRealtimeMeshBufferLayout Invalid;

	private:
		void InitHash()
		{
			uint32 NewHash = ::GetTypeHash(NumElements);
			for (int32 Index = 0; Index < Elements.Num(); Index++)
			{
				NewHash = HashCombine(NewHash, GetTypeHash(Elements[Index]));
			}
			ElementsHash = NewHash;
		}
	};

	enum EIndexElementType
	{
		IET_None,
	
		IET_UInt16,
		IET_Int16,
	
		IET_UInt32,
		IET_Int32,
	
		IET_MAX,
	};

	struct REALTIMEMESHCOMPONENT_API FRealtimeMeshElementTypeDefinition
	{
	private:
		EVertexElementType VertexType;
		EIndexElementType IndexType;
		EPixelFormat PixelFormat;
		uint8 Stride;
		uint8 Alignment : 7;
		uint8 bIsValid : 1;

	public:
		FRealtimeMeshElementTypeDefinition() : VertexType(VET_None), IndexType(IET_None), PixelFormat(PF_Unknown), Stride(0), Alignment(0), bIsValid(false) { }
		FRealtimeMeshElementTypeDefinition(EVertexElementType InVertexType, EIndexElementType InIndexType, EPixelFormat InPixelFormat, uint8 InStride, uint8 InAlignment)
			: VertexType(InVertexType), IndexType(InIndexType), PixelFormat(InPixelFormat), Stride(InStride), Alignment(InAlignment), bIsValid(true)
		{			
		}

		bool IsValid() const { return (IsSupportedVertexType() || IsSupportedIndexType()) && Stride > 0 && Alignment > 0; }
		bool IsSupportedVertexType() const { return VertexType != VET_None; }
		EVertexElementType GetVertexType() const { return VertexType; }
		bool IsSupportedIndexType() const { return IndexType != IET_None; }
		EIndexElementType GetIndexType() const { return IndexType; }
		EPixelFormat GetPixelFormat() const { return PixelFormat; }
		uint8 GetStride() const { return Stride; }
		uint8 GetAlignment() const { return Alignment; }

		constexpr bool operator==(const FRealtimeMeshElementTypeDefinition& Other) const
		{
			return VertexType == Other.VertexType && IndexType == Other.IndexType && PixelFormat == Other.PixelFormat && Stride == Other.Stride && Alignment == Other.Alignment && bIsValid == Other.bIsValid;
		}
		constexpr bool operator!=(const FRealtimeMeshElementTypeDefinition& Other) const
		{
			return VertexType != Other.VertexType || IndexType != Other.IndexType || PixelFormat != Other.PixelFormat || Stride != Other.Stride || Alignment != Other.Alignment || bIsValid != Other.bIsValid;
		}
		
		static const FRealtimeMeshElementTypeDefinition Invalid;
	};

	struct REALTIMEMESHCOMPONENT_API FRealtimeMeshBufferLayoutDefinition
	{
	private:
		FRealtimeMeshBufferLayout Layout;
		FRealtimeMeshElementTypeDefinition TypeDefinition;
		TMap<FName, uint8> ElementOffsets;		
		uint8 Stride;
		bool bIsValid;

	public:
		FRealtimeMeshBufferLayoutDefinition(FRealtimeMeshBufferLayout InLayout, FRealtimeMeshElementTypeDefinition InTypeDefinition)
			: Layout(InLayout), TypeDefinition(InTypeDefinition), Stride(0), bIsValid(false)
		{
			if (Layout.IsValid())
			{
				uint8 ElementOffset = 0;
				for (FName Element : Layout.GetElements())
				{
					ElementOffsets.Add(Element, ElementOffset);
					ElementOffset += TypeDefinition.GetStride();
				}

				Stride = InTypeDefinition.GetStride() * Layout.GetNumElements();
				bIsValid = true;
			}			
		}

		FORCEINLINE const FRealtimeMeshBufferLayout& GetBufferLayout() const { return Layout; }
		FORCEINLINE const FRealtimeMeshElementType& GetElementType() const { return Layout.GetElementType(); }
		FORCEINLINE const FRealtimeMeshElementTypeDefinition& GetElementTypeDefinition() const { return TypeDefinition; }
		FORCEINLINE bool IsValid() const { return bIsValid; }
		FORCEINLINE bool IsValidVertexBuffer() const { return TypeDefinition.IsSupportedVertexType(); }
		FORCEINLINE bool IsValidIndexBuffer() const { return TypeDefinition.IsSupportedIndexType() && Layout.GetNumElements() == 1; }
		FORCEINLINE uint8 GetStride() const { return Stride; }
		FORCEINLINE uint8 GetAlignment() const { return TypeDefinition.GetAlignment(); }
		FORCEINLINE bool ContainsElement(FName ElementName) const { return ElementOffsets.Contains(ElementName); }
		FORCEINLINE uint8 GetElementOffset(FName ElementName) const { return ElementOffsets.FindChecked(ElementName); }
		FORCEINLINE const uint8* FindElementOffset(FName ElementName) const { return ElementOffsets.Find(ElementName); }
		FORCEINLINE const TMap<FName, uint8>& GetElementOffsets() const { return ElementOffsets; }

		static const FRealtimeMeshBufferLayoutDefinition Invalid;
	};

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
		const TFunction<void(void*, void*)> SingleElementConverter;
		const TFunction<void(void*, void*, uint32 Num, uint32 Stride)> ArrayElementConverter;

		FRealtimeMeshElementConverters(const TFunction<void(void*, void*)> InSingleElementConverter, const TFunction<void(void*, void*, uint32 Num, uint32 Stride)> InArrayElementConverter)
			: SingleElementConverter(InSingleElementConverter), ArrayElementConverter(InArrayElementConverter)
		{
		}
	};

	struct REALTIMEMESHCOMPONENT_API FRealtimeMeshBufferLayoutUtilities
	{
	private:
		static const TMap<FRealtimeMeshElementType, FRealtimeMeshElementTypeDefinition> SupportedTypeDefinitions;
		static TMap<FRealtimeMeshElementConversionKey, FRealtimeMeshElementConverters> TypeConversionMap;
	public:
		static FName GetRealtimeMeshDatumTypeName(ERealtimeMeshDatumType Datum);
		static int32 GetRealtimeMeshDatumTypeSize(ERealtimeMeshDatumType Datum);

		static bool IsSupportedType(const FRealtimeMeshElementType& DataType);
		static bool IsSupportedVertexType(const FRealtimeMeshElementType& VertexType);
		static bool IsSupportedIndexType(const FRealtimeMeshElementType& IndexType);

		static bool Is32BitIndex(const FRealtimeMeshElementTypeDefinition& IndexType);

		static bool HasDoubleWidthVertexType(const FRealtimeMeshElementType& VertexType);
		static FRealtimeMeshElementType GetDoubleWidthVertexType(const FRealtimeMeshElementType& VertexType);

		static const FRealtimeMeshElementTypeDefinition& GetTypeDefinition(const FRealtimeMeshElementType& VertexType);
		static FRealtimeMeshBufferLayoutDefinition GetBufferLayoutDefinition(const FRealtimeMeshBufferLayout& BufferLayout);

		static const FRealtimeMeshElementConverters& GetTypeConverter(const FRealtimeMeshElementType& FromType, const FRealtimeMeshElementType& ToType);
		static void RegisterTypeConverter(const FRealtimeMeshElementType& FromType, const FRealtimeMeshElementType& ToType, FRealtimeMeshElementConverters Converters);
		static void UnregisterTypeConverter(const FRealtimeMeshElementType& FromType, const FRealtimeMeshElementType& ToType);
	};


	template<typename ElementType>
	struct FRealtimeMeshElementTypeTraits
	{
		static constexpr FRealtimeMeshElementType ElementTypeDefinition = FRealtimeMeshElementType();
		static constexpr bool IsValid = false;
	};
	
	/*template<>
	struct FRealtimeMeshElementTypeTraits<uint16>
	{
		static constexpr FRealtimeMeshElementType ElementTypeDefinition = FRealtimeMeshElementType(ERealtimeMeshDatumType::UInt16, 1, false, false);
		static constexpr bool IsValid = true;
	};*/



	template<typename ElementType>
	constexpr FRealtimeMeshElementType GetRealtimeMeshDataElementType()
	{
		static_assert(FRealtimeMeshElementTypeTraits<ElementType>::IsValid);
		return FRealtimeMeshElementTypeTraits<ElementType>::ElementTypeDefinition;
	}

	inline TFunctionRef<void(void*, void*)> GetRealtimeMeshElementTypeConverter(const FRealtimeMeshElementType& FromType, const FRealtimeMeshElementType& ToType)
	{
		return FRealtimeMeshBufferLayoutUtilities::GetTypeConverter(FromType, ToType).SingleElementConverter;
	}
	
	inline TFunctionRef<void(void*, void*, uint32 Num, uint32 Stride)> GetRealtimeMeshArrayTypeConverter(const FRealtimeMeshElementType& FromType, const FRealtimeMeshElementType& ToType)
	{
		return FRealtimeMeshBufferLayoutUtilities::GetTypeConverter(FromType, ToType).ArrayElementConverter;		
	}

#define RMC_DEFINE_ELEMENT_TYPE(ElementType, DatumType, NumDatums, bNormalized, bShouldConvertToFloat) \
	template<> struct FRealtimeMeshElementTypeTraits<ElementType> \
	{ \
		static constexpr FRealtimeMeshElementType ElementTypeDefinition = FRealtimeMeshElementType(DatumType, NumDatums, bNormalized, bShouldConvertToFloat); \
		static constexpr bool IsValid = true; \
	};
	

	//template<typename Type> constexpr FRealtimeMeshElementType GetRealtimeMeshDataElementType() { return FRealtimeMeshElementType(); }

	RMC_DEFINE_ELEMENT_TYPE(uint16, ERealtimeMeshDatumType::UInt16, 1, false, false);
	RMC_DEFINE_ELEMENT_TYPE(int16, ERealtimeMeshDatumType::Int16, 1, false, false);
	RMC_DEFINE_ELEMENT_TYPE(uint32, ERealtimeMeshDatumType::UInt32, 1, false, false);
	RMC_DEFINE_ELEMENT_TYPE(int32, ERealtimeMeshDatumType::Int32, 1, false, false);
	
	RMC_DEFINE_ELEMENT_TYPE(float, ERealtimeMeshDatumType::Float, 1, false, true);
	RMC_DEFINE_ELEMENT_TYPE(FVector2f, ERealtimeMeshDatumType::Float, 2, false, true);
	RMC_DEFINE_ELEMENT_TYPE(FVector3f, ERealtimeMeshDatumType::Float, 3, false, true);
	RMC_DEFINE_ELEMENT_TYPE(FVector4f, ERealtimeMeshDatumType::Float, 4, false, true);

	RMC_DEFINE_ELEMENT_TYPE(FFloat16, ERealtimeMeshDatumType::Half, 1, false, true);
	RMC_DEFINE_ELEMENT_TYPE(FVector2DHalf, ERealtimeMeshDatumType::Half, 2, false, true);

	RMC_DEFINE_ELEMENT_TYPE(FColor, ERealtimeMeshDatumType::UInt8, 4, true, true);
	RMC_DEFINE_ELEMENT_TYPE(FLinearColor, ERealtimeMeshDatumType::Float, 4, true, true);

	RMC_DEFINE_ELEMENT_TYPE(FPackedNormal, ERealtimeMeshDatumType::Int8, 4, true, true);
	RMC_DEFINE_ELEMENT_TYPE(FPackedRGBA16N, ERealtimeMeshDatumType::Int16, 4, true, true);

	// These are mostly just used for external data conversion with BP for example. Not actual mesh rendering
	RMC_DEFINE_ELEMENT_TYPE(double, ERealtimeMeshDatumType::Double, 1, false, true);
	RMC_DEFINE_ELEMENT_TYPE(FVector2d, ERealtimeMeshDatumType::Double, 2, false, true);
	RMC_DEFINE_ELEMENT_TYPE(FVector3d, ERealtimeMeshDatumType::Double, 3, false, true);
	RMC_DEFINE_ELEMENT_TYPE(FVector4d, ERealtimeMeshDatumType::Double, 4, false, true);

	template <typename Type>
	struct TIsRealtimeMeshBaseElementType
	{
		static constexpr bool Value = FRealtimeMeshElementTypeTraits<Type>::IsValid;
	};

	
	static_assert(FRealtimeMeshElementTypeTraits<float>::IsValid);
	static_assert(FRealtimeMeshElementTypeTraits<FPackedRGBA16N>::IsValid);
	static_assert(FRealtimeMeshElementTypeTraits<FVector2f>::IsValid);
	static_assert(FRealtimeMeshElementTypeTraits<FColor>::IsValid);
	static_assert(FRealtimeMeshElementTypeTraits<FLinearColor>::IsValid);
	static_assert(!FRealtimeMeshElementTypeTraits<void>::IsValid);


	
	template<typename FromType, typename ToType>
	class FRealtimeMeshTypeConverterRegistration : FNoncopyable
	{
		static_assert(FRealtimeMeshElementTypeTraits<FromType>::IsValid);
		static_assert(FRealtimeMeshElementTypeTraits<ToType>::IsValid);
	public:
		FRealtimeMeshTypeConverterRegistration(FRealtimeMeshElementConverters Converters)
		{
			FRealtimeMeshBufferLayoutUtilities::RegisterTypeConverter(GetRealtimeMeshDataElementType<FromType>(), GetRealtimeMeshDataElementType<ToType>(), Converters);
		}

		~FRealtimeMeshTypeConverterRegistration()
		{
			FRealtimeMeshBufferLayoutUtilities::UnregisterTypeConverter(GetRealtimeMeshDataElementType<FromType>(), GetRealtimeMeshDataElementType<ToType>());
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


	

	template<typename TangentType>
	struct FRealtimeMeshTangents
	{
		TangentType Tangent;
		TangentType Normal;

		FRealtimeMeshTangents(TangentType InNormal, TangentType InTangent)
			: Tangent(InTangent), Normal(InNormal) { }

		FRealtimeMeshTangents(FVector4f InNormal, FVector4f InTangent)
			: Tangent(InTangent), Normal(InNormal) { }
	
		FRealtimeMeshTangents(FVector4 InNormal, FVector4 InTangent)
			: Tangent(InTangent), Normal(InNormal) { }
	};

	template <typename T>
	struct TIsRealtimeMeshTangentType
	{
		enum { Value = false };
	};

	template <typename TangentType>
	struct TIsRealtimeMeshTangentType<FRealtimeMeshTangents<TangentType>>
	{
		enum { Value = true };
		using ElementType = TangentType;
	};

	template<typename ChannelType, int32 ChannelCount>
	struct FRealtimeMeshTexCoord
	{
	private:
		ChannelType UVs[ChannelCount];
	public:
		ChannelType& operator[](int32 Index)
		{
			check(Index >= 0 && Index < ChannelCount);
			return UVs[Index];
		}
	};

	static_assert(sizeof(FRealtimeMeshTexCoord<FVector2f, 1>) == sizeof(FVector2f));
	
	template <typename T>
	struct TIsRealtimeMeshTexCoordType
	{
		enum { Value = false };
	};

	template <typename ChannelType, int32 ChannelCount>
	struct TIsRealtimeMeshTexCoordType<FRealtimeMeshTexCoord<ChannelType, ChannelCount>>
	{
		enum { Value = true };
		using ElementType = ChannelType;
		static constexpr int32 NumChannels = ChannelCount;
	};
	
	static_assert(TIsRealtimeMeshBaseElementType<FVector2DHalf>::Value);
	static_assert(!TIsRealtimeMeshBaseElementType<void>::Value);

	static_assert(TIsRealtimeMeshTexCoordType<FRealtimeMeshTexCoord<FVector2DHalf, 2>>::Value);
	static_assert(!TIsRealtimeMeshTexCoordType<FVector2DHalf>::Value);
	
	static_assert(TIsRealtimeMeshTangentType<FRealtimeMeshTangents<FPackedNormal>>::Value);
	static_assert(!TIsRealtimeMeshTangentType<FPackedNormal>::Value);

	
	//template<typename VertexType> FRealtimeMeshBufferLayout GetRealtimeMeshBufferLayout();
	
	// Handles the case of simple data types being promoted to a full stream
	template<typename VertexType> typename TEnableIf<TIsRealtimeMeshBaseElementType<VertexType>::Value, FRealtimeMeshBufferLayout>::Type GetRealtimeMeshBufferLayout() { return FRealtimeMeshBufferLayout(GetRealtimeMeshDataElementType<VertexType>(), 1); }

	// Handles the case of the template FRealtimeMeshTexCoord
	template<typename VertexType> typename TEnableIf<TIsRealtimeMeshTexCoordType<VertexType>::Value, FRealtimeMeshBufferLayout>::Type GetRealtimeMeshBufferLayout() { return FRealtimeMeshBufferLayout(GetRealtimeMeshDataElementType<typename TIsRealtimeMeshTexCoordType<VertexType>::ElementType>(), TIsRealtimeMeshTexCoordType<VertexType>::NumChannels); }

	// Handles the case of the template FRealtimeMeshTexCoord
	template<typename VertexType> typename TEnableIf<TIsRealtimeMeshTangentType<VertexType>::Value, FRealtimeMeshBufferLayout>::Type GetRealtimeMeshBufferLayout() { return FRealtimeMeshBufferLayout(GetRealtimeMeshDataElementType<typename TIsRealtimeMeshTangentType<VertexType>::ElementType>(), { FName(TEXT("Tangent")), FName(TEXT("Normal")) }); }

	

	using FRealtimeMeshTangentsHighPrecision = FRealtimeMeshTangents<FPackedRGBA16N>;
	using FRealtimeMeshTangentsNormalPrecision = FRealtimeMeshTangents<FPackedNormal>;


	template<typename ChannelType>
	FRealtimeMeshBufferLayout GetRealtimeMeshTexCoordFormatHelper(int32 NumChannels)
	{
		return
			NumChannels >= 8? GetRealtimeMeshBufferLayout<FRealtimeMeshTexCoord<ChannelType, 8>>() :
			NumChannels >= 7? GetRealtimeMeshBufferLayout<FRealtimeMeshTexCoord<ChannelType, 7>>() :
			NumChannels >= 6? GetRealtimeMeshBufferLayout<FRealtimeMeshTexCoord<ChannelType, 6>>() :
			NumChannels >= 5? GetRealtimeMeshBufferLayout<FRealtimeMeshTexCoord<ChannelType, 5>>() :
			NumChannels >= 4? GetRealtimeMeshBufferLayout<FRealtimeMeshTexCoord<ChannelType, 4>>() :
			NumChannels >= 3? GetRealtimeMeshBufferLayout<FRealtimeMeshTexCoord<ChannelType, 3>>() :
			NumChannels >= 2? GetRealtimeMeshBufferLayout<FRealtimeMeshTexCoord<ChannelType, 2>>() :
			GetRealtimeMeshBufferLayout<FRealtimeMeshTexCoord<ChannelType, 1>>();
	}
	

	inline FRealtimeMeshBufferLayout GetRealtimeMeshTexCoordFormatHelper(bool bUseHighPrecision, int32 NumChannels)
	{
		return bUseHighPrecision? GetRealtimeMeshTexCoordFormatHelper<FVector2f>(NumChannels) : GetRealtimeMeshTexCoordFormatHelper<FVector2DHalf>(NumChannels);
	}


	inline FRealtimeMeshBufferLayout GetRealtimeMeshTangentFormatHelper(bool bUseHighPrecision)
	{
		return bUseHighPrecision? GetRealtimeMeshBufferLayout<FRealtimeMeshTangentsHighPrecision>() : GetRealtimeMeshBufferLayout<FRealtimeMeshTangentsNormalPrecision>();
	}
	
	
	
}
