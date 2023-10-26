// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "RealtimeMeshCore.h"
#include "RealtimeMeshDataTypes.generated.h"


USTRUCT(BlueprintType)
struct REALTIMEMESHCOMPONENT_API FRealtimeMeshPolygonGroupRange
{
	GENERATED_BODY()
public:
	FRealtimeMeshPolygonGroupRange()
		: StartIndex(0)
		, Count(0)
		, PolygonGroupIndex(0)
	{ }
	
	FRealtimeMeshPolygonGroupRange(int32 InStartIndex, int32 InCount, int32 InMaterialIndex)
		: StartIndex(InStartIndex)
		, Count(InCount)
		, PolygonGroupIndex(InMaterialIndex)
	{ }
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealtimeMesh")
	int32 StartIndex;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealtimeMesh")
	int32 Count;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealtimeMesh")
	int32 PolygonGroupIndex;


	friend bool operator==(const FRealtimeMeshPolygonGroupRange& Left, const FRealtimeMeshPolygonGroupRange& Right)
	{
		return Left.StartIndex == Right.StartIndex && Left.Count == Right.Count && Left.PolygonGroupIndex == Right.PolygonGroupIndex;
	}
};

namespace RealtimeMesh
{
	template <typename T>
	struct TBoundedArraySize
	{
		enum { Value = 0 };
	};

	template <typename T, uint32 N>
	struct TBoundedArraySize<T[N]>
	{
		enum { Value = N };
	};


	template <typename TangentType>
	struct TRealtimeMeshTangents
	{
	private:
		TangentType Tangent;
		TangentType Normal;

	public:
		TRealtimeMeshTangents(TangentType InNormal, TangentType InTangent)
			: Tangent(InTangent), Normal(InNormal)
		{
		}

		TRealtimeMeshTangents(FVector4f InNormal, FVector4f InTangent)
			: Tangent(InTangent), Normal(InNormal)
		{
		}

		TRealtimeMeshTangents(FVector4d InNormal, FVector4d InTangent)
			: Tangent(InTangent), Normal(InNormal)
		{
		}

		TRealtimeMeshTangents(FVector3f InNormal, FVector3f InTangent)
			: Tangent(InTangent), Normal(InNormal)
		{
		}

		TRealtimeMeshTangents(FVector3d InNormal, FVector3d InTangent)
			: Tangent(InTangent), Normal(InNormal)
		{
		}

		TRealtimeMeshTangents(const FVector3f& InNormal, const FVector3f& InBinormal, const FVector3f& InTangent)
			: Tangent(InTangent), Normal(FVector4f(InNormal, GetBasisDeterminantSign(FVector3d(InTangent), FVector3d(InBinormal), FVector3d(InNormal))))
		{
		}

		TRealtimeMeshTangents(const FVector3d& InNormal, const FVector3d& InBinormal, const FVector3d& InTangent)
			: Tangent(FVector4f(InTangent.X, InTangent.Y, InTangent.Z, 1.0f))
			, Normal(FVector4f(FVector3f(InNormal), GetBasisDeterminantSign(InTangent, InBinormal, InNormal)))
		{
		}

		FVector3f GetNormal() const { return Normal.ToFVector3f(); }
		FVector3f GetTangent() const { return Tangent.ToFVector3f(); }

		FVector3f GetBinormal() const
		{
			const FVector3f TangentX = Tangent.ToFVector3f();
			const FVector4f TangentZ = Normal.ToFVector4f();
			return (FVector3f(TangentZ) ^ TangentX) * TangentZ.W;
		}

		void SetNormal(const FVector3f& NewNormal)
		{
			const FVector3d TangentX = Tangent.ToFVector();
			const FVector4d TangentZ = Normal.ToFVector4();
			const FVector3d TangentY = (FVector3d(TangentZ) ^ TangentX) * TangentZ.W;

			const FVector4f NewTangentZ(NewNormal, GetBasisDeterminantSign(TangentX, TangentY, FVector3d(NewNormal)));
			Normal = NewTangentZ;
		}

		void SetTangent(const FVector3f& NewTangent)
		{
			const FVector3d TangentX = Tangent.ToFVector();
			FVector4d TangentZ = Normal.ToFVector4();
			const FVector3d TangentY = (FVector3d(TangentZ) ^ TangentX) * TangentZ.W;

			TangentZ.W = GetBasisDeterminantSign(FVector3d(NewTangent), TangentY, TangentZ);
			Normal = TangentZ;
			Tangent = NewTangent;
		}

		void SetNormalAndTangent(const FVector3f& InNormal, const FVector3f& InTangent)
		{
			Normal = InNormal;
			Tangent = InTangent;
		}

		void SetTangents(const FVector3f& InNormal, const FVector3f& InBinormal, const FVector3f& InTangent)
		{
			Normal = FVector4f(InNormal, GetBasisDeterminantSign(FVector3d(InTangent), FVector3d(InBinormal), FVector3d(InNormal)));
			Tangent = InTangent;
		}
	};

	using FRealtimeMeshTangentsHighPrecision = TRealtimeMeshTangents<FPackedRGBA16N>;
	using FRealtimeMeshTangentsNormalPrecision = TRealtimeMeshTangents<FPackedNormal>;

	template <typename ChannelType, int32 ChannelCount>
	struct TRealtimeMeshTexCoords
	{
	private:
		ChannelType Channels[ChannelCount];

		template <typename ArgType>
		void Set(int32 Index, const ArgType& Arg)
		{
			static_assert(std::is_constructible<ChannelType, const ArgType&>::value, "Invalid type for TRealtimeMeshTexCoords");
			check(Index < ChannelCount);
			Channels[Index] = ChannelType(Arg);
		}

		template <typename ArgType, typename... ArgTypes>
		void Set(int32 Index, const ArgType& Arg, const ArgTypes&... Args)
		{
			Set(Index, Arg);
			Set(Index + 1, Args...);
		}

	public:
		TRealtimeMeshTexCoords() = default;

		template <typename... ArgTypes>
		TRealtimeMeshTexCoords(const ArgTypes&... Args)
		{
			Set(0, Args...);
		}

		template <typename... ArgTypes>
		void Set(const ArgTypes&... Args)
		{
			SetAll(0, Args...);
		}

		ChannelType& operator[](int32 Index)
		{
			check(Index >= 0 && Index < ChannelCount);
			return Channels[Index];
		}
	};

	template <int32 ChannelCount>
	using TRealtimeMeshTexCoordsHighPri = TRealtimeMeshTexCoords<FVector2f, ChannelCount>;
	template <int32 ChannelCount>
	using TRealtimeMeshTexCoordsNormal = TRealtimeMeshTexCoords<FVector2DHalf, ChannelCount>;

	using FRealtimeMeshTexCoordsHighPri = TRealtimeMeshTexCoordsHighPri<1>;
	using FRealtimeMeshTexCoordsHighPri2 = TRealtimeMeshTexCoordsHighPri<2>;
	using FRealtimeMeshTexCoordsHighPri3 = TRealtimeMeshTexCoordsHighPri<3>;
	using FRealtimeMeshTexCoordsHighPri4 = TRealtimeMeshTexCoordsHighPri<4>;

	using FRealtimeMeshTexCoordsNormal = TRealtimeMeshTexCoordsNormal<1>;
	using FRealtimeMeshTexCoordsNormal2 = TRealtimeMeshTexCoordsNormal<2>;
	using FRealtimeMeshTexCoordsNormal3 = TRealtimeMeshTexCoordsNormal<3>;
	using FRealtimeMeshTexCoordsNormal4 = TRealtimeMeshTexCoordsNormal<4>;

	template <typename IndexType>
	struct TIndex3
	{
		union
		{
			IndexType Indices[3] = {0, 0, 0};

			struct
			{
				IndexType V0;
				IndexType V1;
				IndexType V2;
			};
		};

		constexpr TIndex3() = default;

		constexpr TIndex3(IndexType InV0, IndexType InV1, IndexType InV2)
			: V0(InV0), V1(InV1), V2(InV2)
		{
		}

		operator FIntVector() const
		{
			return FIntVector(V0, V1, V2);
		}

		TIndex3(const FIntVector& Vec)
		{
			V0 = Vec.X;
			V1 = Vec.Y;
			V2 = Vec.Z;
		}

		constexpr static TIndex3 Zero()
		{
			return TIndex3(0, 0, 0);
		}

		constexpr static TIndex3 Max()
		{
			return TIndex3(TNumericLimits<IndexType>::Max(), TNumericLimits<IndexType>::Max(), TNumericLimits<IndexType>::Max());
		}

		IndexType& operator[](int32 Idx)
		{
			return Indices[Idx];
		}

		const IndexType& operator[](int32 Idx) const
		{
			return Indices[Idx];
		}

		bool operator==(const TIndex3& Other) const
		{
			return V0 == Other.V0 && V1 == Other.V1 && V2 == Other.V2;
		}

		bool operator!=(const TIndex3& Other) const
		{
			return V0 != Other.V0 || V1 != Other.V1 || V2 != Other.V2;
		}

		int32 IndexOf(IndexType Value) const
		{
			return (V0 == Value) ? 0 : ((V1 == Value) ? 1 : (V2 == Value ? 2 : -1));
		}

		bool Contains(IndexType Value) const
		{
			return (V0 == Value) || (V1 == Value) || (V2 == Value);
		}

		bool IsDegenerate() const
		{
			return V0 == V1 || V1 == V2 || V2 == V0;
		}

		TIndex3 Clamp(IndexType Min, IndexType Max) const
		{
			return TIndex3(FMath::Clamp(V0, Min, Max), FMath::Clamp(V1, Min, Max), FMath::Clamp(V2, Min, Max));
		}

		friend FArchive& operator<<(FArchive& Ar, TIndex3& I)
		{
			I.Serialize(Ar);
			return Ar;
		}

		void Serialize(FArchive& Ar)
		{
			Ar << V0;
			Ar << V1;
			Ar << V2;
		}
	};

	template <typename IndexType>
	FORCEINLINE uint32 GetTypeHash(const TIndex3<IndexType>& Index)
	{
		// (this is how FIntVector and all the other FVectors do their hash functions)
		// Note: this assumes there's no padding that could contain uncompared data.
		return FCrc::MemCrc_DEPRECATED(&Index, sizeof(TIndex3<IndexType>));
	}

	using FIndex3UI = TIndex3<uint32>;
	using FIndex3US = TIndex3<uint16>;



	

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

	enum EIndexElementType
	{
		IET_None,

		IET_UInt16,
		IET_Int16,

		IET_UInt32,
		IET_Int32,

		IET_MAX,
	};

	struct REALTIMEMESHCOMPONENT_API FRealtimeMeshElementType
	{
	private:
		ERealtimeMeshDatumType Type;
		uint8 NumDatums : 3;
		uint8 bNormalized : 1;
		uint8 bShouldConvertToFloat : 1;

	public:
		constexpr FRealtimeMeshElementType() : Type(ERealtimeMeshDatumType::Unknown), NumDatums(0), bNormalized(false), bShouldConvertToFloat(false)
		{
		}

		constexpr FRealtimeMeshElementType(ERealtimeMeshDatumType InType, int32 InNumDatums, bool bInNormalized, bool bInShouldConvertToFloat)
			: Type(InType), NumDatums(InNumDatums), bNormalized(bInNormalized), bShouldConvertToFloat(bInShouldConvertToFloat)
		{
		}

		constexpr bool IsValid() const { return Type != ERealtimeMeshDatumType::Unknown && NumDatums > 0; }
		constexpr ERealtimeMeshDatumType GetDatumType() const { return Type; }
		constexpr uint8 GetNumDatums() const { return NumDatums; }
		constexpr bool IsNormalized() const { return bNormalized; }
		constexpr bool ShouldConvertToFloat() const { return bShouldConvertToFloat; }


		constexpr bool operator==(const FRealtimeMeshElementType& Other) const
		{
			return Type == Other.Type && NumDatums == Other.NumDatums && bNormalized == Other.bNormalized && bShouldConvertToFloat == Other.bShouldConvertToFloat;
		}

		constexpr bool operator!=(const FRealtimeMeshElementType& Other) const
		{
			return Type != Other.Type || NumDatums != Other.NumDatums || bNormalized != Other.bNormalized || bShouldConvertToFloat != Other.bShouldConvertToFloat;
		}

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

	struct REALTIMEMESHCOMPONENT_API FRealtimeMeshBufferLayout
	{
	private:
		FRealtimeMeshElementType ElementType;
		int32 NumElements;

	public:
		constexpr FRealtimeMeshBufferLayout()
			: ElementType(FRealtimeMeshElementType())
			, NumElements(0)
		{
		}
		constexpr FRealtimeMeshBufferLayout(FRealtimeMeshElementType InType, int32 InNumElements)
			: ElementType(InType)
			, NumElements(InNumElements)
		{
		}

		constexpr bool IsValid() const { return ElementType.IsValid() && NumElements > 0; }
		constexpr const FRealtimeMeshElementType& GetElementType() const { return ElementType; }
		constexpr int32 GetNumElements() const { return NumElements; }
		//bool HasElementIdentifiers() const { return Elements.Num() > 0; }
		//TConstArrayView<FName> GetElements() const { return MakeArrayView(Elements); }

		constexpr bool operator==(const FRealtimeMeshBufferLayout& Other) const { return ElementType == Other.ElementType && NumElements == Other.NumElements; }
		constexpr bool operator!=(const FRealtimeMeshBufferLayout& Other) const { return ElementType != Other.ElementType || NumElements != Other.NumElements; }

		FString ToString() const
		{
			return FString::Printf(TEXT("ElementType=%s NumElements=%d"), *ElementType.ToString(), NumElements);
		}

		friend FORCEINLINE uint32 GetTypeHash(const FRealtimeMeshBufferLayout& DataType)
		{
			return ::HashCombine(GetTypeHash(DataType.ElementType), ::GetTypeHash(DataType.NumElements));
		}

		friend FArchive& operator<<(FArchive& Ar, FRealtimeMeshBufferLayout& Layout)
		{
			Ar << Layout.ElementType;

			if (Ar.CustomVer(FRealtimeMeshVersion::GUID) < FRealtimeMeshVersion::RemovedNamedStreamElements)
			{
				TArray<FName, TInlineAllocator<REALTIME_MESH_MAX_STREAM_ELEMENTS>> Elements;
				Ar << Elements;
			}
			Ar << Layout.NumElements;
			return Ar;
		}

		static const FRealtimeMeshBufferLayout Invalid;

	private:
		/*void InitHash()
		{
			uint32 NewHash = ::GetTypeHash(NumElements);
			for (int32 Index = 0; Index < Elements.Num(); Index++)
			{
				NewHash = HashCombine(NewHash, GetTypeHash(Elements[Index]));
			}
			ElementsHash = NewHash;
		}*/

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
		FRealtimeMeshElementTypeDefinition() : VertexType(VET_None), IndexType(IET_None), PixelFormat(PF_Unknown), Stride(0), Alignment(0), bIsValid(false)
		{
		}

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
			return VertexType == Other.VertexType && IndexType == Other.IndexType && PixelFormat == Other.PixelFormat && Stride == Other.Stride && Alignment == Other.Alignment &&
				bIsValid == Other.bIsValid;
		}

		constexpr bool operator!=(const FRealtimeMeshElementTypeDefinition& Other) const
		{
			return VertexType != Other.VertexType || IndexType != Other.IndexType || PixelFormat != Other.PixelFormat || Stride != Other.Stride || Alignment != Other.Alignment ||
				bIsValid != Other.bIsValid;
		}

		static const FRealtimeMeshElementTypeDefinition Invalid;
	};

	struct REALTIMEMESHCOMPONENT_API FRealtimeMeshBufferLayoutDefinition
	{
	private:
		FRealtimeMeshBufferLayout Layout;
		FRealtimeMeshElementTypeDefinition TypeDefinition;
		uint8 Stride;
		bool bIsValid;

	public:
		FRealtimeMeshBufferLayoutDefinition(FRealtimeMeshBufferLayout InLayout, FRealtimeMeshElementTypeDefinition InTypeDefinition)
			: Layout(InLayout), TypeDefinition(InTypeDefinition), Stride(0), bIsValid(false)
		{
			if (Layout.IsValid())
			{
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
		/*FORCEINLINE bool ContainsElement(FName ElementName) const { return ElementOffsets.Contains(ElementName); }
		FORCEINLINE uint8 GetElementOffset(FName ElementName) const { return ElementOffsets.FindChecked(ElementName); }
		FORCEINLINE const uint8* FindElementOffset(FName ElementName) const { return ElementOffsets.Find(ElementName); }
		FORCEINLINE const TMap<FName, uint8>& GetElementOffsets() const { return ElementOffsets; }*/

		static const FRealtimeMeshBufferLayoutDefinition Invalid;
	};

	template <typename ElementType>
	struct FRealtimeMeshElementTypeTraits
	{
		static constexpr FRealtimeMeshElementType ElementTypeDefinition = FRealtimeMeshElementType();
		static constexpr bool IsValid = false;
	};

#define RMC_DEFINE_ELEMENT_TYPE(ElementType, DatumType, NumDatums, bNormalized, bShouldConvertToFloat) \
	template<> struct FRealtimeMeshElementTypeTraits<ElementType> \
	{ \
		static constexpr FRealtimeMeshElementType ElementTypeDefinition = FRealtimeMeshElementType(DatumType, NumDatums, bNormalized, bShouldConvertToFloat); \
		static constexpr bool IsValid = true; \
	};

	template <typename BufferType, typename Enable = void>
	struct FRealtimeMeshBufferTypeTraits
	{
		using ElementType = void;
		static constexpr int32 NumElements = 0;
		static constexpr bool IsValid = false;
	};

#define RMC_DEFINE_BUFFER_TYPE(BufferType, SingleElementType, ElementCount) \
	template<> struct FRealtimeMeshBufferTypeTraits<BufferType, void> \
	{ \
		using ElementType = SingleElementType; \
		static constexpr int32 NumElements = ElementCount; \
		static constexpr bool IsValid = true; \
		static_assert(sizeof(BufferType) == sizeof(SingleElementType) * ElementCount); \
	};


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


	// BufferTypeTraits specialization for simple element types 
	template <typename BufferType>
	struct FRealtimeMeshBufferTypeTraits<BufferType, typename TEnableIf<FRealtimeMeshElementTypeTraits<BufferType>::IsValid, void>::Type>
	{
		using ElementType = BufferType;
		static constexpr int32 NumElements = 1;
		static constexpr bool IsValid = true;
	};

	// BufferTypeTraits specialization for FRealtimeMeshTangents
	template <typename TangentType>
	struct FRealtimeMeshBufferTypeTraits<TRealtimeMeshTangents<TangentType>>
	{
		using ElementType = TangentType;
		static constexpr int32 NumElements = 2;
		static constexpr bool IsValid = true;
	};

	// BufferTypeTraits specialization for FRealtimeMeshTexCoords
	template <typename ChannelType, int32 ChannelCount>
	struct FRealtimeMeshBufferTypeTraits<TRealtimeMeshTexCoords<ChannelType, ChannelCount>>
	{
		using ElementType = ChannelType;
		static constexpr int32 NumElements = ChannelCount;
		static constexpr bool IsValid = true;
	};

	// BufferTypeTraits specialization for bounded arrays
	template <typename ChannelType, int32 ChannelCount>
	struct FRealtimeMeshBufferTypeTraits<ChannelType[ChannelCount]>
	{
		using ElementType = ChannelType;
		static constexpr int32 NumElements = ChannelCount;
		static constexpr bool IsValid = FRealtimeMeshElementTypeTraits<ChannelType>::IsValid;
	};

	// BufferTypeTraits specialization for TIndex3
	template <typename IndexType>
	struct FRealtimeMeshBufferTypeTraits<TIndex3<IndexType>>
	{
		using ElementType = IndexType;
		static constexpr int32 NumElements = 3;
		static constexpr bool IsValid = FRealtimeMeshElementTypeTraits<IndexType>::IsValid;
	};
	
	template <>
	struct FRealtimeMeshBufferTypeTraits<FRealtimeMeshPolygonGroupRange>
	{
		using ElementType = int32;
		static constexpr int32 NumElements = 3;
		static constexpr bool IsValid = true;
	};

	template <typename ElementType>
	constexpr FRealtimeMeshElementType GetRealtimeMeshDataElementType()
	{
		static_assert(FRealtimeMeshElementTypeTraits<ElementType>::IsValid);
		return FRealtimeMeshElementTypeTraits<ElementType>::ElementTypeDefinition;
	}

	template <typename BufferType>
	constexpr FRealtimeMeshBufferLayout GetRealtimeMeshBufferLayout()
	{
		static_assert(FRealtimeMeshBufferTypeTraits<BufferType>::IsValid);
		return FRealtimeMeshBufferLayout(
			GetRealtimeMeshDataElementType<typename FRealtimeMeshBufferTypeTraits<BufferType>::ElementType>(),
			FRealtimeMeshBufferTypeTraits<BufferType>::NumElements);
	}

	template <typename ElementType>
	constexpr FRealtimeMeshBufferLayout GetRealtimeMeshBufferLayout(int32 NumStreamElements)
	{
		return FRealtimeMeshBufferLayout(GetRealtimeMeshDataElementType<ElementType>(), NumStreamElements);
	}


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

	static_assert(sizeof(FRealtimeMeshElementType) == sizeof(uint16), "Data must always be 16bit");
	static_assert(sizeof(TRealtimeMeshTexCoords<FVector2f, 1>) == sizeof(FVector2f));

	static_assert(!FRealtimeMeshElementTypeTraits<void>::IsValid);
	static_assert(!FRealtimeMeshElementTypeTraits<FRealtimeMeshTexCoordsNormal>::IsValid);
	static_assert(!FRealtimeMeshElementTypeTraits<float[1]>::IsValid);
	static_assert(FRealtimeMeshElementTypeTraits<float>::IsValid);
	static_assert(FRealtimeMeshElementTypeTraits<FPackedRGBA16N>::IsValid);
	static_assert(FRealtimeMeshElementTypeTraits<FVector2f>::IsValid);
	static_assert(FRealtimeMeshElementTypeTraits<FColor>::IsValid);
	static_assert(FRealtimeMeshElementTypeTraits<FLinearColor>::IsValid);

	static_assert(!FRealtimeMeshBufferTypeTraits<void>::IsValid);
	static_assert(FRealtimeMeshBufferTypeTraits<FVector3f>::IsValid);
	static_assert(FRealtimeMeshBufferTypeTraits<FPackedNormal[1]>::IsValid);
	static_assert(FRealtimeMeshBufferTypeTraits<FRealtimeMeshTexCoordsNormal>::IsValid);
	static_assert(FRealtimeMeshBufferTypeTraits<FRealtimeMeshTangentsHighPrecision>::IsValid);
	static_assert(FRealtimeMeshBufferTypeTraits<FIndex3UI>::IsValid);
}
