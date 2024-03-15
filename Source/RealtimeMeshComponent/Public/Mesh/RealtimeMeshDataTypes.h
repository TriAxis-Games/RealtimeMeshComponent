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

	namespace Internal
	{
		template<typename NormalType>
		inline void SetRealtimeMeshNormalWToSign(NormalType& Normal, float Sign)
		{
			Normal.W = Sign;
		}
		template<>
		inline void SetRealtimeMeshNormalWToSign<FPackedNormal>(FPackedNormal& Normal, float Sign)
		{
			Normal.Vector.W = static_cast<int8>(FMath::Clamp<int32>(FMath::RoundToInt(Sign * MAX_int8), MIN_int8, MAX_int8));
		}
		template<>
		inline void SetRealtimeMeshNormalWToSign<FPackedRGBA16N>(FPackedRGBA16N& Normal, float Sign)
		{
			Normal.W = static_cast<int16>(FMath::Clamp<int32>(FMath::RoundToInt(Sign * MIN_int16), MIN_int16, MAX_int16));
		}

		inline FVector4f GetTangentAsVector(const FVector4f& Input)
		{
			return Input;
		}

		inline FVector4f GetTangentAsVector(const FPackedNormal& Input)
		{
			return Input.ToFVector4f();
		}

		inline FVector4f GetTangentAsVector(const FPackedRGBA16N& Input)
		{
			return Input.ToFVector4f();
		}
		
	}


	template <typename TangentType>
	struct TRealtimeMeshTangents
	{
	private:
		TangentType Tangent;
		TangentType Normal;

	public:
		TRealtimeMeshTangents() = default;

		TRealtimeMeshTangents(FVector3f InNormal, FVector3f InTangent, bool bShouldFlipBinormal = false)
			: Tangent(InTangent), Normal(InNormal)
		{
			Internal::SetRealtimeMeshNormalWToSign(Normal, bShouldFlipBinormal? -1.0f : 1.0f);
		}

		TRealtimeMeshTangents(FVector3d InNormal, FVector3d InTangent, bool bShouldFlipBinormal = false)
			: Tangent(InTangent), Normal(InNormal)
		{
			Internal::SetRealtimeMeshNormalWToSign(Normal, bShouldFlipBinormal? -1.0f : 1.0f);
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

		template<typename InputTangentType>
		TRealtimeMeshTangents(const TRealtimeMeshTangents<InputTangentType>& Other)
			: Tangent(Other.Tangent), Normal(Other.Normal)
		{
		}
		
		TRealtimeMeshTangents(const TRealtimeMeshTangents<FVector4f>& Other)
			: Tangent(Other.Tangent), Normal(Other.Normal)
		{
		}
		
		TRealtimeMeshTangents(const TRealtimeMeshTangents<FPackedNormal>& Other)
			: Tangent(Other.Tangent.ToFVector4f()), Normal(Other.Normal.ToFVector4f())
		{
		}
		
		TRealtimeMeshTangents(const TRealtimeMeshTangents<FPackedRGBA16N>& Other)
			: Tangent(Other.Tangent.ToFVector4f()), Normal(Other.Normal.ToFVector4f())
		{
		}

		FVector3f GetNormal() const { return Internal::GetTangentAsVector(Normal); }
		FVector3f GetTangent() const { return Internal::GetTangentAsVector(Tangent); }

		bool IsBinormalFlipped() const { return Normal.W < 0.0f; }

		void SetFlipBinormal(bool bShouldFlipBinormal)
		{
			SetRealtimeMeshNormalWToSign(Normal, bShouldFlipBinormal? -1.0f : 1.0f);
		}

		FVector3f GetBinormal() const
		{
			const FVector4f TangentX = Internal::GetTangentAsVector(Tangent);
			const FVector4f TangentZ = Internal::GetTangentAsVector(Normal);
			return (FVector3f(TangentZ) ^ FVector3f(TangentX)) * TangentZ.W;
		}

		void SetNormal(const FVector3f& NewNormal)
		{
			const FVector4f TangentX = Internal::GetTangentAsVector(Tangent);
			const FVector4f TangentZ = Internal::GetTangentAsVector(Normal);
			const FVector3f TangentY = (FVector3f(TangentZ) ^ FVector3f(TangentX)) * TangentZ.W;

			const FVector4f NewTangentZ(NewNormal, GetBasisDeterminantSign(FVector3d(TangentX), FVector3d(TangentY), FVector3d(NewNormal)));
			Normal = NewTangentZ;
		}

		void SetTangent(const FVector3f& NewTangent)
		{
			const FVector4f TangentX = Internal::GetTangentAsVector(Tangent);
			FVector4f TangentZ = Internal::GetTangentAsVector(Normal);
			const FVector3f TangentY = (FVector3f(TangentZ) ^ TangentX) * TangentZ.W;

			TangentZ.W = GetBasisDeterminantSign(FVector3d(NewTangent), FVector3d(TangentY), FVector3d(TangentZ));
			Normal = TangentZ;
			Tangent = NewTangent;
		}

		void SetNormalAndTangent(const FVector3f& InNormal, const FVector3f& InTangent, bool bShouldFlipBinormal = false)
		{
			Normal = InNormal;
			Internal::SetRealtimeMeshNormalWToSign(Normal, bShouldFlipBinormal? -1.0f : 1.0f);
			Tangent = InTangent;
		}

		void SetTangents(const FVector3f& InNormal, const FVector3f& InBinormal, const FVector3f& InTangent)
		{
			Normal = FVector4f(InNormal, GetBasisDeterminantSign(FVector3d(InTangent), FVector3d(InBinormal), FVector3d(InNormal)));
			Tangent = InTangent;
		}

		template<typename InputTangentType>
		friend struct TRealtimeMeshTangents;

		FORCEINLINE bool operator==(const TRealtimeMeshTangents<TangentType>& Other) const
		{
			return Normal == Other.Normal && Tangent == Other.Tangent;
		}

		FORCEINLINE bool operator!=(const TRealtimeMeshTangents<TangentType>& Other) const
		{
			return Normal != Other.Normal || Tangent != Other.Tangent;
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
			static_assert(std::is_constructible_v<ChannelType, const ArgType&>, "Invalid type for TRealtimeMeshTexCoords");
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
		TRealtimeMeshTexCoords(const TRealtimeMeshTexCoords& Other) = default;
		TRealtimeMeshTexCoords(TRealtimeMeshTexCoords&& Other) = default;
		TRealtimeMeshTexCoords& operator=(const TRealtimeMeshTexCoords& Other) = default;
		TRealtimeMeshTexCoords& operator=(TRealtimeMeshTexCoords&& Other) = default;


		template<typename InputTexCoordType>
		TRealtimeMeshTexCoords(const TRealtimeMeshTexCoords<InputTexCoordType, ChannelCount>& Other)
		{
			for (int32 Index = 0; Index < ChannelCount; Index++)
			{
				Channels[Index] = Other.Channels[Index];
			}
		}
		
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

		FORCEINLINE bool operator==(const TRealtimeMeshTexCoords& Other) const
		{
			for (int32 Index = 0; Index < ChannelCount; Index++)
			{
				if (Channels[Index] != Other.Channels[Index])
				{
					return false;
				}
			}

			return true;
		}

		FORCEINLINE bool operator!=(const TRealtimeMeshTexCoords& Other) const
		{
			for (int32 Index = 0; Index < ChannelCount; Index++)
			{
				if (Channels[Index] != Other.Channels[Index])
				{
					return true;
				}
			}

			return false;
		}

		template<typename OtherChannelType, int32 OtherChannelCount>
		friend struct TRealtimeMeshTexCoords;
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

		// Customized version of Int8 meant to signify a normalized float packed into a int8
		Int8Float,
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
	public:
		constexpr FRealtimeMeshElementType() : Type(ERealtimeMeshDatumType::Unknown), NumDatums(0)
		{
		}

		constexpr FRealtimeMeshElementType(ERealtimeMeshDatumType InType, int32 InNumDatums)
			: Type(InType), NumDatums(InNumDatums)
		{
		}

		constexpr bool IsValid() const { return Type != ERealtimeMeshDatumType::Unknown && NumDatums > 0; }
		constexpr ERealtimeMeshDatumType GetDatumType() const { return Type; }
		constexpr uint8 GetNumDatums() const { return NumDatums; }


		constexpr bool operator==(const FRealtimeMeshElementType& Other) const
		{
			return Type == Other.Type && NumDatums == Other.NumDatums;
		}

		constexpr bool operator!=(const FRealtimeMeshElementType& Other) const
		{
			return Type != Other.Type || NumDatums != Other.NumDatums;
		}

		FString ToString() const
		{
			return FString::Printf(TEXT("Type=%d NumDatums=%d IsNormalized=%d ConvertToFloat=%d"), Type, NumDatums);
		}

		friend FORCEINLINE uint32 GetTypeHash(const FRealtimeMeshElementType& Element)
		{
			return ::HashCombine(::GetTypeHash(Element.Type), ::GetTypeHash(Element.NumDatums));
		}

		friend FArchive& operator<<(FArchive& Ar, FRealtimeMeshElementType& ElementType)
		{
			Ar << ElementType.Type;

			uint8 TempNumDatums = ElementType.NumDatums;
			Ar << TempNumDatums;
			ElementType.NumDatums = TempNumDatums;

			if (Ar.CustomVer(FRealtimeMeshVersion::GUID) < FRealtimeMeshVersion::ImprovingDataTypes)
			{
				// TODO: Remove these
				bool bTempNormalized = false;
				Ar << bTempNormalized;

				bool bTempShouldConvertToFloat = false;
				Ar << bTempShouldConvertToFloat;

				if (Ar.IsLoading())
				{
					if (bTempNormalized && bTempShouldConvertToFloat && ElementType.Type == ERealtimeMeshDatumType::Int8)
					{
						ElementType.Type = ERealtimeMeshDatumType::Int8Float;
					}
				}
			}

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
	};


	struct REALTIMEMESHCOMPONENT_API FRealtimeMeshBufferMemoryLayout
	{
	private:
		const uint8 Stride;
		const uint8 ElementStride;
		const uint8 Alignment;

	public:
		FRealtimeMeshBufferMemoryLayout() : Stride(0), ElementStride(0), Alignment(0) { }
		FRealtimeMeshBufferMemoryLayout(uint8 InElementStride, uint8 InAlignment, uint8 InNumElements)
			: Stride(InElementStride * InNumElements), ElementStride(InElementStride), Alignment(InAlignment) { }
		
		FORCEINLINE uint8 GetStride() const { return Stride; }
		FORCEINLINE uint8 GetElementStride() const { return ElementStride; }
		FORCEINLINE uint8 GetAlignment() const { return Alignment; }
	};

	struct REALTIMEMESHCOMPONENT_API FRealtimeMeshElementTypeDetails
	{
	private:
		const EVertexElementType VertexType;
		const EIndexElementType IndexType;
		const EPixelFormat PixelFormat;
		const uint8 Stride;
		const uint8 Alignment;
	public:

		FRealtimeMeshElementTypeDetails()
			: VertexType(VET_None), IndexType(IET_None), PixelFormat(PF_Unknown), Stride(0), Alignment(0)
		{
		}
		
		FRealtimeMeshElementTypeDetails(EVertexElementType InVertexType, EIndexElementType InIndexType, EPixelFormat InPixelFormat, uint8 InStride, uint8 InAlignment)
			: VertexType(InVertexType), IndexType(InIndexType), PixelFormat(InPixelFormat), Stride(InStride), Alignment(InAlignment)
		{
		}
		
		FORCEINLINE bool IsValid() const { return Stride > 0 && Alignment > 0; }
		FORCEINLINE bool IsSupportedVertexType() const { return VertexType != VET_None; }
		FORCEINLINE EVertexElementType GetVertexType() const { return VertexType; }
		FORCEINLINE bool IsSupportedIndexType() const { return IndexType != IET_None; }
		FORCEINLINE EIndexElementType GetIndexType() const { return IndexType; }
		FORCEINLINE EPixelFormat GetPixelFormat() const { return PixelFormat; }
		FORCEINLINE uint8 GetStride() const { return Stride; }
		FORCEINLINE uint8 GetAlignment() const { return Alignment; }		
	};

	template <typename ElementType>
	struct FRealtimeMeshElementTypeTraits
	{
		static constexpr FRealtimeMeshElementType ElementTypeDefinition = FRealtimeMeshElementType();
		static constexpr bool IsValid = false;
	};

#define RMC_DEFINE_ELEMENT_TYPE(ElementType, DatumType, NumDatums) \
	template<> struct FRealtimeMeshElementTypeTraits<ElementType> \
	{ \
		static constexpr FRealtimeMeshElementType ElementTypeDefinition = FRealtimeMeshElementType(DatumType, NumDatums); \
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


	RMC_DEFINE_ELEMENT_TYPE(uint16, ERealtimeMeshDatumType::UInt16, 1);
	RMC_DEFINE_ELEMENT_TYPE(int16, ERealtimeMeshDatumType::Int16, 1);
	RMC_DEFINE_ELEMENT_TYPE(uint32, ERealtimeMeshDatumType::UInt32, 1);
	RMC_DEFINE_ELEMENT_TYPE(int32, ERealtimeMeshDatumType::Int32, 1);
	RMC_DEFINE_ELEMENT_TYPE(float, ERealtimeMeshDatumType::Float, 1);
	RMC_DEFINE_ELEMENT_TYPE(FVector2f, ERealtimeMeshDatumType::Float, 2);
	RMC_DEFINE_ELEMENT_TYPE(FVector3f, ERealtimeMeshDatumType::Float, 3);
	RMC_DEFINE_ELEMENT_TYPE(FVector4f, ERealtimeMeshDatumType::Float, 4);
	RMC_DEFINE_ELEMENT_TYPE(FFloat16, ERealtimeMeshDatumType::Half, 1);
	RMC_DEFINE_ELEMENT_TYPE(FVector2DHalf, ERealtimeMeshDatumType::Half, 2);
	RMC_DEFINE_ELEMENT_TYPE(FColor, ERealtimeMeshDatumType::UInt8, 4);
	RMC_DEFINE_ELEMENT_TYPE(FLinearColor, ERealtimeMeshDatumType::Float, 4);
	RMC_DEFINE_ELEMENT_TYPE(FPackedNormal, ERealtimeMeshDatumType::Int8Float, 4);
	RMC_DEFINE_ELEMENT_TYPE(FPackedRGBA16N, ERealtimeMeshDatumType::Int16, 4);
	RMC_DEFINE_ELEMENT_TYPE(double, ERealtimeMeshDatumType::Double, 1);
	RMC_DEFINE_ELEMENT_TYPE(FVector2d, ERealtimeMeshDatumType::Double, 2);
	RMC_DEFINE_ELEMENT_TYPE(FVector3d, ERealtimeMeshDatumType::Double, 3);
	RMC_DEFINE_ELEMENT_TYPE(FVector4d, ERealtimeMeshDatumType::Double, 4);
	RMC_DEFINE_ELEMENT_TYPE(FIntVector, ERealtimeMeshDatumType::Int32, 3);
	RMC_DEFINE_ELEMENT_TYPE(FIntPoint, ERealtimeMeshDatumType::Int32, 2);

	

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
	
	constexpr FRealtimeMeshBufferLayout GetRealtimeMeshBufferLayout(const FRealtimeMeshElementType& ElementType, int32 NumElements)
	{
		return FRealtimeMeshBufferLayout(ElementType, NumElements);
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
		static const TMap<FRealtimeMeshElementType, FRealtimeMeshElementTypeDetails> SupportedTypeDefinitions;

	public:
		static FName GetRealtimeMeshDatumTypeName(ERealtimeMeshDatumType Datum);
		static int32 GetRealtimeMeshDatumTypeSize(ERealtimeMeshDatumType Datum);
		static int32 GetRealtimeMeshDatumTypeStride(ERealtimeMeshDatumType Datum);

		static int32 GetElementStride(const FRealtimeMeshElementType& ElementType);
		static int32 GetElementAlignment(const FRealtimeMeshElementType& ElementType);

		static FRealtimeMeshBufferMemoryLayout GetBufferLayoutMemoryLayout(const FRealtimeMeshBufferLayout& BufferLayout);
		static FRealtimeMeshElementTypeDetails GetElementTypeDetails(const FRealtimeMeshElementType& ElementType);
		static FRealtimeMeshElementTypeDetails GetElementTypeDetails(const FRealtimeMeshBufferLayout& BufferLayout);
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

	 
#if WITH_EDITORONLY_DATA
	namespace NatVisHelpers
	{
		template<typename Type, int32 NumDatums>
		struct TRawVisualizer;

		template<typename Type, int32 NumElements>
		struct TRowVisualizer;
		
#define DEFINE_VISUALIZER_SET(Type) \
		template<> struct REALTIMEMESHCOMPONENT_API TRawVisualizer<Type, 2> \
		{ \
			Type X; \
			Type Y; \
		}; \
		using TRawVisualizer_##Type##_2 = TRawVisualizer<Type, 2>; \
		template<> struct REALTIMEMESHCOMPONENT_API TRawVisualizer<Type, 3> \
		{ \
			Type X; \
			Type Y; \
			Type Z; \
		}; \
		using TRawVisualizer_##Type##_3 = TRawVisualizer<Type, 3>; \
		template<> struct REALTIMEMESHCOMPONENT_API TRawVisualizer<Type, 4> \
		{ \
			Type X; \
			Type Y; \
			Type Z; \
			Type W; \
		}; \
		using TRawVisualizer_##Type##_4 = TRawVisualizer<Type, 4>; \

#define DEFINE_VISUALISER_ROW_SET(Type) \
		template<> struct REALTIMEMESHCOMPONENT_API TRowVisualizer<Type, 2> \
		{ \
			Type Elements[2]; \
		}; \
		template<> struct REALTIMEMESHCOMPONENT_API TRowVisualizer<Type, 3> \
		{ \
			Type Elements[3]; \
		}; \
		template<> struct REALTIMEMESHCOMPONENT_API TRowVisualizer<Type, 4> \
		{ \
			Type Elements[4]; \
		}; \
		template<> struct REALTIMEMESHCOMPONENT_API TRowVisualizer<Type, 5> \
		{ \
			Type Elements[5]; \
		}; \
		template<> struct REALTIMEMESHCOMPONENT_API TRowVisualizer<Type, 6> \
		{ \
			Type Elements[6]; \
		}; \
		template<> struct REALTIMEMESHCOMPONENT_API TRowVisualizer<Type, 7> \
		{ \
			Type Elements[7]; \
		}; \
		template<> struct REALTIMEMESHCOMPONENT_API TRowVisualizer<Type, 8> \
		{ \
			Type Elements[8]; \
		}; \
		inline void RegisterVisualizers_##Type() \
		{ \
			[[maybe_unused]] TRowVisualizer<Type, 2> Row2; \
			[[maybe_unused]] TRowVisualizer<Type, 3> Row3; \
			[[maybe_unused]] TRowVisualizer<Type, 4> Row4; \
			[[maybe_unused]] TRowVisualizer<Type, 5> Row5; \
			[[maybe_unused]] TRowVisualizer<Type, 6> Row6; \
			[[maybe_unused]] TRowVisualizer<Type, 7> Row7; \
			[[maybe_unused]] TRowVisualizer<Type, 8> Row8; \
		}


#define DEFINE_TYPE_VISUALIZERS(Type) \
		DEFINE_VISUALIZER_SET(Type) \
		DEFINE_VISUALISER_ROW_SET(TRawVisualizer_##Type##_2) \
		DEFINE_VISUALISER_ROW_SET(TRawVisualizer_##Type##_3) \
		DEFINE_VISUALISER_ROW_SET(TRawVisualizer_##Type##_4)
		

		DEFINE_TYPE_VISUALIZERS(int8);
		DEFINE_TYPE_VISUALIZERS(uint8);

		DEFINE_TYPE_VISUALIZERS(int16);
		DEFINE_TYPE_VISUALIZERS(uint16);

		DEFINE_TYPE_VISUALIZERS(int32);
		DEFINE_TYPE_VISUALIZERS(uint32);

		DEFINE_TYPE_VISUALIZERS(FFloat16);
		DEFINE_TYPE_VISUALIZERS(float);
		DEFINE_TYPE_VISUALIZERS(double);


	}
#endif
}


template<typename ChannelType, int32 ChannelCount> struct TCanBulkSerialize<RealtimeMesh::TRealtimeMeshTexCoords<ChannelType, ChannelCount>> { enum { Value = true }; };
template<typename ChannelType, int32 ChannelCount> struct TIsPODType<RealtimeMesh::TRealtimeMeshTexCoords<ChannelType, ChannelCount>> { enum { Value = true }; };

template<typename IndexType> struct TCanBulkSerialize<RealtimeMesh::TIndex3<IndexType>> { enum { Value = true }; };
template<typename IndexType> struct TIsPODType<RealtimeMesh::TIndex3<IndexType>> { enum { Value = true }; };

template<typename TangentType> struct TCanBulkSerialize<RealtimeMesh::TRealtimeMeshTangents<TangentType>> { enum { Value = true }; };
template<typename TangentType> struct TIsPODType<RealtimeMesh::TRealtimeMeshTangents<TangentType>> { enum { Value = true }; };
