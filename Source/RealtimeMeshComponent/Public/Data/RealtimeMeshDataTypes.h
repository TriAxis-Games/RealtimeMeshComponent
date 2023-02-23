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
		union
		{
			struct
			{			
				ERealtimeMeshDatumType Type;
				uint8 NumDatums : 3;
				uint8 bNormalized : 1;
				uint8 bShouldConvertToFloat : 1;
				uint8 Unused : 3;
			};
			uint16 Packed;
		} Data;


		static_assert(sizeof(Data) == sizeof(uint16), "Data must always be 16bit");
	public:
		constexpr FRealtimeMeshElementType() { Data.Packed = 0; }
		constexpr FRealtimeMeshElementType(ERealtimeMeshDatumType InType, int32 InNumDatums, bool bInNormalized, bool bInShouldConvertToFloat)
		{
			Data.Type = InType;
			Data.NumDatums = InNumDatums;
			Data.bNormalized = bInNormalized;
			Data.bShouldConvertToFloat = bInShouldConvertToFloat;
			Data.Unused = 0;
		}

		constexpr bool IsValid() const { return Data.Type != ERealtimeMeshDatumType::Unknown && Data.NumDatums > 0; }
		constexpr ERealtimeMeshDatumType GetDatumType() const { return Data.Type; }
		constexpr uint8 GetNumDatums() const { return Data.NumDatums; }
		constexpr bool IsNormalized() const { return Data.bNormalized; }
		constexpr bool ShouldConvertToFloat() const { return Data.bShouldConvertToFloat; }

		
		bool operator==(const FRealtimeMeshElementType& Other) const { return Data.Packed == Other.Data.Packed; }
		bool operator!=(const FRealtimeMeshElementType& Other) const { return Data.Packed != Other.Data.Packed; }

		FString ToString() const
		{
			return FString::Printf(TEXT("Type=%d NumDatums=%d IsNormalized=%d ConvertToFloat=%d"), Data.Type, Data.NumDatums, Data.bNormalized, Data.bShouldConvertToFloat);
		}
		
		friend FORCEINLINE uint32 GetTypeHash(const FRealtimeMeshElementType& Element)
		{
			return ::GetTypeHash(Element.Data.Packed);
		}
		
		friend FArchive& operator<<(FArchive& Ar, FRealtimeMeshElementType& ElementType)
		{
			Ar << ElementType.Data.Packed;
			return Ar;
		}

		static const FRealtimeMeshElementType Invalid;
	};

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
		uint8 Alignment;

	public:
		FRealtimeMeshElementTypeDefinition(EVertexElementType InVertexType, EIndexElementType InIndexType, EPixelFormat InPixelFormat, uint8 InStride, uint8 InAlignment)
			: VertexType(InVertexType), IndexType(InIndexType), PixelFormat(InPixelFormat), Stride(InStride), Alignment(InAlignment)
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

	struct REALTIMEMESHCOMPONENT_API FRealtimeMeshBufferLayoutUtilities
	{
	private:
		static const TMap<FRealtimeMeshElementType, FRealtimeMeshElementTypeDefinition> SupportedTypeDefinitions;
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
		
	};


	template<typename Type> constexpr FRealtimeMeshElementType GetRealtimeMeshDataElementType() { return FRealtimeMeshElementType(ERealtimeMeshDatumType::Unknown, 0, false, false); }
	
	template<> constexpr FRealtimeMeshElementType GetRealtimeMeshDataElementType<uint16>() { return FRealtimeMeshElementType(ERealtimeMeshDatumType::UInt16, 1, false, false); }
	template<> constexpr FRealtimeMeshElementType GetRealtimeMeshDataElementType<int16>() { return FRealtimeMeshElementType(ERealtimeMeshDatumType::Int16, 1, false, false); }
	template<> constexpr FRealtimeMeshElementType GetRealtimeMeshDataElementType<uint32>() { return FRealtimeMeshElementType(ERealtimeMeshDatumType::UInt32, 1, false, false); }
	template<> constexpr FRealtimeMeshElementType GetRealtimeMeshDataElementType<int32>() { return FRealtimeMeshElementType(ERealtimeMeshDatumType::Int32, 1, false, false); }


	template<> constexpr FRealtimeMeshElementType GetRealtimeMeshDataElementType<float>() { return FRealtimeMeshElementType(ERealtimeMeshDatumType::Float, 1, false, true); }
	template<> constexpr FRealtimeMeshElementType GetRealtimeMeshDataElementType<FVector2f>() { return FRealtimeMeshElementType(ERealtimeMeshDatumType::Float, 2, false, true); }
	template<> constexpr FRealtimeMeshElementType GetRealtimeMeshDataElementType<FVector3f>() { return FRealtimeMeshElementType(ERealtimeMeshDatumType::Float, 3, false, true); }
	template<> constexpr FRealtimeMeshElementType GetRealtimeMeshDataElementType<FVector4f>() { return FRealtimeMeshElementType(ERealtimeMeshDatumType::Float, 4, false, true); }

	template<> constexpr FRealtimeMeshElementType GetRealtimeMeshDataElementType<FFloat16>() { return FRealtimeMeshElementType(ERealtimeMeshDatumType::Half, 1, false, true); }
	template<> constexpr FRealtimeMeshElementType GetRealtimeMeshDataElementType<FVector2DHalf>() { return FRealtimeMeshElementType(ERealtimeMeshDatumType::Half, 2, false, true); }

	template<> constexpr FRealtimeMeshElementType GetRealtimeMeshDataElementType<FColor>() { return FRealtimeMeshElementType(ERealtimeMeshDatumType::UInt8, 4, true, true); }

	template<> constexpr FRealtimeMeshElementType GetRealtimeMeshDataElementType<FPackedNormal>() { return FRealtimeMeshElementType(ERealtimeMeshDatumType::Int8, 4, true, true); }
	template<> constexpr FRealtimeMeshElementType GetRealtimeMeshDataElementType<FPackedRGBA16N>() { return FRealtimeMeshElementType(ERealtimeMeshDatumType::Int16, 4, true, true); }

	template <typename Type>
	struct TIsRealtimeMeshBaseElementType
	{
		static constexpr bool Value = GetRealtimeMeshDataElementType<Type>().IsValid();
	};

	
	static_assert(GetRealtimeMeshDataElementType<float>().IsValid());
	static_assert(GetRealtimeMeshDataElementType<FPackedRGBA16N>().IsValid());
	static_assert(GetRealtimeMeshDataElementType<FVector2f>().IsValid());
	static_assert(GetRealtimeMeshDataElementType<FColor>().IsValid());
	static_assert(!GetRealtimeMeshDataElementType<void>().IsValid());
	static_assert(!GetRealtimeMeshDataElementType<double>().IsValid());
	


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
	
	
	
}
