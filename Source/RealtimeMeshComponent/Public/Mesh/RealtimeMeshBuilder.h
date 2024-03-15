// Copyright TriAxis Games, L.L.C. All Rights Reserved.


#pragma once

#include "RealtimeMeshDataTypes.h"
#include "RealtimeMeshDataStream.h"
#include "Templates/Invoke.h"
#include "Traits/IsVoidType.h"

struct FRealtimeMeshSimpleMeshData;


// ReSharper disable CppMemberFunctionMayBeConst
namespace RealtimeMesh
{
	template<typename InType>
	struct TRealtimeMeshStreamDataAccessorTypeTraits
	{
		using Type = std::remove_cv_t<InType>;
		using ElementType = typename FRealtimeMeshBufferTypeTraits<Type>::ElementType;
		using QualifiedType = InType;
		using QualifiedElementType = typename TCopyQualifiersFromTo<InType, ElementType>::Type;
		static constexpr bool IsConst = TIsConst<InType>::Value;
		static constexpr bool IsVoid = std::is_same_v<Type, void>;
		static constexpr int32 NumElements = FRealtimeMeshBufferTypeTraits<Type>::NumElements;

		template<typename OtherType>
		static constexpr bool IsConvertibleTo()
		{
			return !std::is_void_v<decltype(ConvertRealtimeMeshType<Type, std::remove_cv_t<OtherType>>(DeclVal<Type>()))>;
		}
	};

	
	/* This implementation supports a static element conversion */
	template<typename InAccessType, typename InBufferType, bool bAllowSubstreamAccess>
	struct TRealtimeMeshStreamDataAccessor
	{
		using AccessTypeTraits = TRealtimeMeshStreamDataAccessorTypeTraits<InAccessType>;
		using BufferTypeTraits = TRealtimeMeshStreamDataAccessorTypeTraits<InBufferType>;

		using AccessType = typename AccessTypeTraits::Type;
		using BufferType = typename BufferTypeTraits::Type;

		using AccessElementType = typename AccessTypeTraits::ElementType;
		using BufferElementType = typename BufferTypeTraits::ElementType;
		
		static_assert(!AccessTypeTraits::IsVoid, "AccessType cannot be void.");
		static_assert(!BufferTypeTraits::IsVoid, "BufferType cannot be void.");
		static_assert(BufferTypeTraits::NumElements == AccessTypeTraits::NumElements, "Buffer and Access types must have the same number of elements.");
		static_assert(AccessTypeTraits::template IsConvertibleTo<typename BufferTypeTraits::Type>(), "Conversion handler for this type is not implemented.");
		static_assert(BufferTypeTraits::template IsConvertibleTo<typename AccessTypeTraits::Type>(), "Conversion handler for this type is not implemented.");
		
		struct TContextWithoutSubstream
		{
			typename TCopyQualifiersFromTo<InAccessType, FRealtimeMeshStream>::Type& Stream;
		};
		struct TContextWithSubstream
		{
			typename TCopyQualifiersFromTo<InAccessType, FRealtimeMeshStream>::Type& Stream;
			int32 ElementOffset;
		};

		using TContext = typename TChooseClass<bAllowSubstreamAccess, TContextWithSubstream, TContextWithoutSubstream>::Result;
		
		static TContext InitializeContext(typename TCopyQualifiersFromTo<InAccessType, FRealtimeMeshStream>::Type& Stream, int32 ElementOffset)
		{
			checkf(Stream.GetLayout().GetElementType() == GetRealtimeMeshBufferLayout<BufferType>().GetElementType(), TEXT("Supplied stream not correct format for builder."));
			checkf(AccessTypeTraits::template IsConvertibleTo<BufferType>(), TEXT("Conversion handler for this type is not implemented."));
			checkf(BufferTypeTraits::template IsConvertibleTo<AccessType>(), TEXT("Conversion handler for this type is not implemented."));
			
			if constexpr (bAllowSubstreamAccess)
			{
				checkf(Stream.GetLayout().GetNumElements() >= AccessTypeTraits::NumElements + ElementOffset, TEXT("Substream section is outside of the supplied stream."));
				return TContext { Stream, ElementOffset * Stream.GetElementStride() };
			}
			else
			{
				checkf(ElementOffset == 0, TEXT("Cannot read elements with offset using this accessor."));
				return TContext { Stream };
			}
		}
		
		static AccessType GetBufferValue(const TContext& Context, int32 Index)
		{
			if constexpr (bAllowSubstreamAccess)
			{
				BufferType StreamData = *reinterpret_cast<BufferType*>(Context.Stream.GetDataRawAtVertex(Index) + Context.ElementOffset);
				return ConvertRealtimeMeshType<BufferType, AccessType>(StreamData);
			}
			else
			{
				BufferType StreamData = *reinterpret_cast<BufferType*>(Context.Stream.GetDataRawAtVertex(Index));
				return ConvertRealtimeMeshType<BufferType, AccessType>(StreamData);				
			}
		}
		static void SetBufferValue(const TContext& Context, int32 Index, const AccessType& InValue)
		{
			if constexpr (bAllowSubstreamAccess)
			{
				BufferType StreamData = ConvertRealtimeMeshType<AccessType, BufferType>(InValue);
				*reinterpret_cast<BufferType*>(Context.Stream.GetDataRawAtVertex(Index) + Context.ElementOffset) = StreamData;
			}
			else
			{
				BufferType StreamData = ConvertRealtimeMeshType<AccessType, BufferType>(InValue);
				*reinterpret_cast<BufferType*>(Context.Stream.GetDataRawAtVertex(Index)) = StreamData;		
			}
		}
		static AccessElementType GetElementValue(const TContext& Context, int32 Index, int32 ElementIndex)
		{
			if constexpr (bAllowSubstreamAccess)
			{
				BufferElementType StreamData = *reinterpret_cast<BufferElementType*>(Context.Stream.GetDataRawAtVertex(Index) + Context.ElementOffset + Context.Stream.GetElementStride() * ElementIndex);
				return ConvertRealtimeMeshType<BufferElementType, AccessElementType>(StreamData);
			}
			else
			{
				BufferElementType StreamData = *reinterpret_cast<BufferElementType*>(Context.Stream.GetDataRawAtVertex(Index) + Context.Stream.GetElementStride() * ElementIndex);
				return ConvertRealtimeMeshType<BufferElementType, AccessElementType>(StreamData);				
			}
		}
		static void SetElementValue(const TContext& Context, int32 Index, int32 ElementIndex, const AccessElementType& InValue)
		{
			if constexpr (bAllowSubstreamAccess)
			{
				BufferElementType StreamData = ConvertRealtimeMeshType<AccessElementType, BufferElementType>(InValue);
				*reinterpret_cast<BufferElementType*>(Context.Stream.GetDataRawAtVertex(Index) + Context.ElementOffset + Context.Stream.GetElementStride() * ElementIndex) = StreamData;
			}
			else
			{
				BufferElementType StreamData = ConvertRealtimeMeshType<AccessElementType, BufferElementType>(InValue);
				*reinterpret_cast<BufferElementType*>(Context.Stream.GetDataRawAtVertex(Index) + Context.Stream.GetElementStride() * ElementIndex) = StreamData;		
			}
		}
	};
	
	/* This implementation supports a direct read only of the same type */
	template<typename InStreamType, bool bAllowSubstreamAccess>
	struct TRealtimeMeshStreamDataAccessor<InStreamType, InStreamType, bAllowSubstreamAccess>
	{
		using StreamTypeTraits = TRealtimeMeshStreamDataAccessorTypeTraits<InStreamType>;

		using StreamType = typename StreamTypeTraits::Type;
		using StreamElementType = typename StreamTypeTraits::ElementType;
		
		static_assert(!StreamTypeTraits::IsVoid, "AccessType cannot be void.");
		
		struct TContextWithoutSubstream
		{
			typename TCopyQualifiersFromTo<InStreamType, FRealtimeMeshStream>::Type& Stream;
		};
		struct TContextWithSubstream
		{
			typename TCopyQualifiersFromTo<InStreamType, FRealtimeMeshStream>::Type& Stream;
			int32 ElementOffset;
		};

		using TContext = typename TChooseClass<bAllowSubstreamAccess, TContextWithSubstream, TContextWithoutSubstream>::Result;
		
		static TContext InitializeContext(typename TCopyQualifiersFromTo<InStreamType, FRealtimeMeshStream>::Type& Stream, int32 ElementOffset)
		{			
			if constexpr (bAllowSubstreamAccess)
			{
				checkf(Stream.GetLayout().GetNumElements() >= StreamTypeTraits::NumElements + ElementOffset, TEXT("Substream section is outside of the supplied stream."));
				return TContext { Stream, ElementOffset * Stream.GetElementStride() };
			}
			else
			{
				checkf(ElementOffset == 0, TEXT("Cannot read elements with offset using this accessor."));
				return TContext { Stream };
			}
		}
		
		static StreamType GetBufferValue(const TContext& Context, int32 Index)
		{
			if constexpr (bAllowSubstreamAccess)
			{
				return *reinterpret_cast<StreamType*>(Context.Stream.GetDataRawAtVertex(Index) + Context.ElementOffset);
			}
			else
			{
				return *reinterpret_cast<StreamType*>(Context.Stream.GetDataRawAtVertex(Index));		
			}
		}
		static void SetBufferValue(const TContext& Context, int32 Index, const StreamType& InValue)
		{
			if constexpr (bAllowSubstreamAccess)
			{
				*reinterpret_cast<StreamType*>(Context.Stream.GetDataRawAtVertex(Index) + Context.ElementOffset) = InValue;
			}
			else
			{
				*reinterpret_cast<StreamType*>(Context.Stream.GetDataRawAtVertex(Index)) = InValue;		
			}
		}
		static StreamElementType GetElementValue(const TContext& Context, int32 Index, int32 ElementIndex)
		{
			if constexpr (bAllowSubstreamAccess)
			{
				return *reinterpret_cast<StreamElementType*>(Context.Stream.GetDataRawAtVertex(Index) + Context.ElementOffset + Context.Stream.GetElementStride() * ElementIndex);
			}
			else
			{
				return *reinterpret_cast<StreamElementType*>(Context.Stream.GetDataRawAtVertex(Index) + Context.Stream.GetElementStride() * ElementIndex);
			}
		}
		static void SetElementValue(const TContext& Context, int32 Index, int32 ElementIndex, const StreamElementType& InValue)
		{
			if constexpr (bAllowSubstreamAccess)
			{
				*reinterpret_cast<StreamElementType*>(Context.Stream.GetDataRawAtVertex(Index) + Context.ElementOffset + Context.Stream.GetElementStride() * ElementIndex) = InValue;
			}
			else
			{
				*reinterpret_cast<StreamElementType*>(Context.Stream.GetDataRawAtVertex(Index) + Context.Stream.GetElementStride() * ElementIndex) = InValue;		
			}
		}
	};
	
	/* This implementation supports a dynamic conversion from the unknown stream type */
	template<typename InAccessType, bool bAllowSubstreamAccess>
	struct TRealtimeMeshStreamDataAccessor<InAccessType, void, bAllowSubstreamAccess>
	{
		using AccessTypeTraits = TRealtimeMeshStreamDataAccessorTypeTraits<InAccessType>;

		using AccessType = typename AccessTypeTraits::Type;
		using AccessElementType = typename AccessTypeTraits::ElementType;
		
		static_assert(!AccessTypeTraits::IsVoid, "AccessType cannot be void.");
		
		struct TContextWithoutSubstream
		{
			typename TCopyQualifiersFromTo<InAccessType, FRealtimeMeshStream>::Type& Stream;
			const FRealtimeMeshElementConverters& ReadConverters;
			const FRealtimeMeshElementConverters& WriteConverters;
		};
		struct TContextWithSubstream
		{
			typename TCopyQualifiersFromTo<InAccessType, FRealtimeMeshStream>::Type& Stream;
			const FRealtimeMeshElementConverters& ReadConverters;
			const FRealtimeMeshElementConverters& WriteConverters;
			int32 ElementOffset;
		};

		using TContext = typename TChooseClass<bAllowSubstreamAccess, TContextWithSubstream, TContextWithoutSubstream>::Result;
		
		static TContext InitializeContext(typename TCopyQualifiersFromTo<InAccessType, FRealtimeMeshStream>::Type& Stream, int32 ElementOffset)
		{			
			const FRealtimeMeshBufferLayout StreamLayout = Stream.GetLayout();
			const FRealtimeMeshBufferLayout AccessLayout = GetRealtimeMeshBufferLayout<AccessType>();
			
			const FRealtimeMeshElementType StreamElementTypeDef = StreamLayout.GetElementType();
			const FRealtimeMeshElementType AccessElementTypeDef = AccessLayout.GetElementType();

			check(FRealtimeMeshTypeConversionUtilities::CanConvert(StreamElementTypeDef, AccessElementTypeDef));
			check(FRealtimeMeshTypeConversionUtilities::CanConvert(AccessElementTypeDef, StreamElementTypeDef));
			
			if constexpr (bAllowSubstreamAccess)
			{
				checkf(Stream.GetLayout().GetNumElements() >= AccessTypeTraits::NumElements + ElementOffset, TEXT("Substream section is outside of the supplied stream."));
				return TContext
				{
					Stream,
					FRealtimeMeshTypeConversionUtilities::GetTypeConverter(StreamElementTypeDef, AccessElementTypeDef),
					FRealtimeMeshTypeConversionUtilities::GetTypeConverter(AccessElementTypeDef, StreamElementTypeDef),
					ElementOffset * Stream.GetElementStride()
				};
			}
			else
			{
				checkf(ElementOffset == 0, TEXT("Cannot read elements with offset using this accessor."));
				return TContext
				{
					Stream,
					FRealtimeMeshTypeConversionUtilities::GetTypeConverter(StreamElementTypeDef, AccessElementTypeDef),
					FRealtimeMeshTypeConversionUtilities::GetTypeConverter(AccessElementTypeDef, StreamElementTypeDef)
				};
			}		
		}
		
		static AccessType GetBufferValue(const TContext& Context, int32 Index)
		{
			if constexpr (bAllowSubstreamAccess)
			{
				const void* DataPtr = Context.Stream.GetDataRawAtVertex(Index) + Context.ElementOffset;
				TTypeCompatibleBytes<AccessType> Result;
				Context.ReadConverters.ConvertContiguousArray(DataPtr, &Result, AccessTypeTraits::NumElements);
				return *Result.GetTypedPtr();
			}
			else
			{
				const void* DataPtr = Context.Stream.GetDataRawAtVertex(Index);
				TTypeCompatibleBytes<AccessType> Result;
				Context.ReadConverters.ConvertContiguousArray(DataPtr, &Result, AccessTypeTraits::NumElements);
				return *Result.GetTypedPtr();	
			}
		}
		static void SetBufferValue(const TContext& Context, int32 Index, const AccessType& InValue)
		{
			if constexpr (bAllowSubstreamAccess)
			{
				void* DataPtr = Context.Stream.GetDataRawAtVertex(Index) + Context.ElementOffset;
				Context.WriteConverters.ConvertContiguousArray(&InValue, DataPtr, AccessTypeTraits::NumElements);
			}
			else
			{
				void* DataPtr = Context.Stream.GetDataRawAtVertex(Index);
				Context.WriteConverters.ConvertContiguousArray(&InValue, DataPtr, AccessTypeTraits::NumElements);	
			}
		}
		static AccessElementType GetElementValue(const TContext& Context, int32 Index, int32 ElementIndex)
		{
			if constexpr (bAllowSubstreamAccess)
			{
				const void* DataPtr = Context.Stream.GetDataRawAtVertex(Index) + Context.ElementOffset + Context.Stream.GetElementStride() * ElementIndex;
				TTypeCompatibleBytes<AccessElementType> Result;
				Context.ReadConverters.ConvertSingleElement(DataPtr, &Result);
				return *Result.GetTypedPtr();
			}
			else
			{
				const void* DataPtr = Context.Stream.GetDataRawAtVertex(Index) + Context.Stream.GetElementStride() * ElementIndex;
				TTypeCompatibleBytes<AccessElementType> Result;
				Context.ReadConverters.ConvertSingleElement(DataPtr, &Result);
				return *Result.GetTypedPtr();
			}
		}
		static void SetElementValue(const TContext& Context, int32 Index, int32 ElementIndex, const AccessElementType& InValue)
		{
			if constexpr (bAllowSubstreamAccess)
			{
				void* DataPtr = Context.Stream.GetDataRawAtVertex(Index) + Context.ElementOffset + Context.Stream.GetElementStride() * ElementIndex;
				Context.WriteConverters.ConvertSingleElement(&InValue, DataPtr);
			}
			else
			{
				void* DataPtr = Context.Stream.GetDataRawAtVertex(Index) + Context.Stream.GetElementStride() * ElementIndex;
				Context.WriteConverters.ConvertSingleElement(&InValue, DataPtr);
			}
		}
	};


	template<typename BaseType, typename NewType>
	struct TRealtimeMeshPromoteTypeIfNotVoid
	{
		using Type = NewType;
	};

	template<typename NewType>
	struct TRealtimeMeshPromoteTypeIfNotVoid<void, NewType>
	{
		using Type = void;
	};
	

	template<typename OriginalType, typename DefaultType>
	struct TRealtimeMeshUseTypeOrDefaultIfVoid
	{
		using Type = OriginalType;
	};

	template<typename DefaultType>
	struct TRealtimeMeshUseTypeOrDefaultIfVoid<void, DefaultType>
	{
		using Type = DefaultType;
	};
	
	template<typename OriginalType, typename NewType>
	struct TRealtimeMeshIsVoidOrSameType
	{
		static constexpr bool Value = false;
	};

	template<typename DefaultType>
	struct TRealtimeMeshIsVoidOrSameType<void, DefaultType>
	{
		static constexpr bool Value = true;
	};

	template<typename Type>
	struct TRealtimeMeshIsVoidOrSameType<Type, Type>
	{
		static constexpr bool Value = true;
	};


	
	
	enum class FRealtimeMeshStreamBuilderEventType
	{
		NewSizeUninitialized,
		NewSizeZeroed,
		Reserve,
		Shrink,
		Empty,
		RemoveAt,
	};

	
	
	
	template<typename InAccessType, typename InBufferType, bool bAllowSubstreamAccess = false>
	struct TRealtimeMeshElementAccessor
	{	
		using SizeType = FRealtimeMeshStream::SizeType;

		using AccessTypeTraits = TRealtimeMeshStreamDataAccessorTypeTraits<InAccessType>;
		using BufferTypeTraits = TRealtimeMeshStreamDataAccessorTypeTraits<InBufferType>;

		using AccessType = typename AccessTypeTraits::Type;
		using BufferType = typename BufferTypeTraits::Type;

		using AccessElementType = typename AccessTypeTraits::ElementType;
		using BufferElementType = typename BufferTypeTraits::ElementType;

		static constexpr int32 NumElements = AccessTypeTraits::NumElements;
		static constexpr bool IsSingleElement = NumElements == 1;
		static constexpr bool IsWritable = !AccessTypeTraits::IsConst;

		using StreamDataAccessor = TRealtimeMeshStreamDataAccessor<InAccessType, InBufferType, bAllowSubstreamAccess>;

		template<typename RetVal, typename Dummy>
		using TEnableIfWritable = std::enable_if_t<sizeof(Dummy) && IsWritable, RetVal>;
		
	private:
		const typename StreamDataAccessor::TContext& Context;
		SizeType RowIndex;
		SizeType ElementIdx;

	public:
		TRealtimeMeshElementAccessor(const typename StreamDataAccessor::TContext& InContext, SizeType InRowIndex, SizeType InElementIndex)
			: Context(InContext)
			, RowIndex(InRowIndex)
			, ElementIdx(InElementIndex)
		{			
		}

		AccessElementType GetValue() const
		{
			return StreamDataAccessor::GetElementValue(Context, RowIndex, ElementIdx);
		}
		
		template <typename U = AccessElementType>
		FORCEINLINE TEnableIfWritable<void, U> SetValue(const AccessElementType& InNewValue)
		{			
			StreamDataAccessor::SetElementValue(Context, RowIndex, ElementIdx, InNewValue);
		}
		
		FORCEINLINE operator AccessElementType() const
		{
			return GetValue();
		}
		
		template <typename U = AccessElementType>
		FORCEINLINE TEnableIfWritable<TRealtimeMeshElementAccessor&, U> operator=(const AccessElementType& InNewValue)
		{
			SetValue(InNewValue);
			return *this;
		}		

#define DEFINE_BINARY_OPERATOR_VARIATIONS(ret, op) \
		FORCEINLINE friend ret operator op(const TRealtimeMeshElementAccessor& Left, const TRealtimeMeshElementAccessor& Right) { return Left.GetValue() op Right.GetValue(); } \
		FORCEINLINE friend ret operator op(const TRealtimeMeshElementAccessor& Left, const AccessElementType& Right) { return Left.GetValue() op Right; } \
		FORCEINLINE friend ret operator op(const AccessElementType& Left, const TRealtimeMeshElementAccessor& Right) { return Left op Right.GetValue(); }
		DEFINE_BINARY_OPERATOR_VARIATIONS(bool, ==)
		DEFINE_BINARY_OPERATOR_VARIATIONS(bool, !=)
		DEFINE_BINARY_OPERATOR_VARIATIONS(bool, <)
		DEFINE_BINARY_OPERATOR_VARIATIONS(bool, >)
		DEFINE_BINARY_OPERATOR_VARIATIONS(bool, <=)
		DEFINE_BINARY_OPERATOR_VARIATIONS(bool, >=)
		DEFINE_BINARY_OPERATOR_VARIATIONS(AccessElementType, +)
		DEFINE_BINARY_OPERATOR_VARIATIONS(AccessElementType, -)
		DEFINE_BINARY_OPERATOR_VARIATIONS(AccessElementType, *)
		DEFINE_BINARY_OPERATOR_VARIATIONS(AccessElementType, /)
		DEFINE_BINARY_OPERATOR_VARIATIONS(AccessElementType, %)
		DEFINE_BINARY_OPERATOR_VARIATIONS(AccessElementType, &)
		DEFINE_BINARY_OPERATOR_VARIATIONS(AccessElementType, |)
		DEFINE_BINARY_OPERATOR_VARIATIONS(AccessElementType, ^)
		DEFINE_BINARY_OPERATOR_VARIATIONS(AccessElementType, <<)
		DEFINE_BINARY_OPERATOR_VARIATIONS(AccessElementType, >>)
#undef DEFINE_BINARY_OPERATOR_VARIATIONS
		
#define DEFINE_BINARY_OPERATOR_VARIATIONS(op) \
		template <typename U = AccessElementType> \
		FORCEINLINE TEnableIfWritable<TRealtimeMeshElementAccessor&, U> operator op(const AccessElementType& Right) { SetValue(GetValue() op Right); return *this; } \
		template <typename U = AccessElementType> \
		FORCEINLINE TEnableIfWritable<TRealtimeMeshElementAccessor&, U> operator op(const TRealtimeMeshElementAccessor& Right) { SetValue(GetValue() op Right.GetValue()); return *this; }
		DEFINE_BINARY_OPERATOR_VARIATIONS(+=)
		DEFINE_BINARY_OPERATOR_VARIATIONS(-=)
		DEFINE_BINARY_OPERATOR_VARIATIONS(*=)
		DEFINE_BINARY_OPERATOR_VARIATIONS(/=)
		DEFINE_BINARY_OPERATOR_VARIATIONS(%=)
		DEFINE_BINARY_OPERATOR_VARIATIONS(&=)
		DEFINE_BINARY_OPERATOR_VARIATIONS(|=)
		DEFINE_BINARY_OPERATOR_VARIATIONS(^=)
		DEFINE_BINARY_OPERATOR_VARIATIONS(<<=)
		DEFINE_BINARY_OPERATOR_VARIATIONS(>>=)
#undef DEFINE_BINARY_OPERATOR_VARIATIONS		
	};
	
	template<typename InAccessType, typename InBufferType, bool bAllowSubstreamAccess = false>
	struct TRealtimeMeshIndexedBufferAccessor
	{
	public:
		using SizeType = FRealtimeMeshStream::SizeType;

		using AccessTypeTraits = TRealtimeMeshStreamDataAccessorTypeTraits<InAccessType>;
		using BufferTypeTraits = TRealtimeMeshStreamDataAccessorTypeTraits<InBufferType>;

		using AccessType = typename AccessTypeTraits::Type;
		using BufferType = typename BufferTypeTraits::Type;

		using AccessElementType = typename AccessTypeTraits::ElementType;
		using BufferElementType = typename BufferTypeTraits::ElementType;

		static constexpr int32 NumElements = AccessTypeTraits::NumElements;
		static constexpr bool IsSingleElement = NumElements == 1;
		static constexpr bool IsWritable = !AccessTypeTraits::IsConst;

		using StreamDataAccessor = TRealtimeMeshStreamDataAccessor<InAccessType, InBufferType, bAllowSubstreamAccess>;

		template<typename RetVal, typename Dummy>
		using TEnableIfWritable = std::enable_if_t<sizeof(Dummy) && IsWritable, RetVal>;

		using ElementAccessor = TRealtimeMeshElementAccessor<InAccessType, InBufferType, bAllowSubstreamAccess>;
		using ConstElementAccessor = TRealtimeMeshElementAccessor<const InAccessType, InBufferType, bAllowSubstreamAccess>;
		
	protected:
		const typename StreamDataAccessor::TContext& Context;
		SizeType RowIndex;

	public:
		TRealtimeMeshIndexedBufferAccessor(const typename StreamDataAccessor::TContext& InContext, int32 InRowIndex)
			: Context(InContext)
			, RowIndex(InRowIndex)
		{			
		}
		FORCEINLINE SizeType GetIndex() const { return RowIndex; }
		FORCEINLINE operator AccessType() const { return Get(); }
		
		FORCEINLINE AccessType Get() const
		{
			return StreamDataAccessor::GetBufferValue(Context, RowIndex);
		}
		
		template <typename U = AccessType>
		FORCEINLINE TEnableIfWritable<TRealtimeMeshIndexedBufferAccessor&, U> Set(const AccessType& NewValue)
		{
			StreamDataAccessor::SetBufferValue(Context, RowIndex, NewValue);
			return *this;
		}
		
		FORCEINLINE ConstElementAccessor GetElement(SizeType ElementIdx) const
		{
			return ConstElementAccessor(Context, RowIndex, ElementIdx);
		}
		
		template <typename U = AccessType>
		FORCEINLINE TEnableIfWritable<ElementAccessor, U> GetElement(SizeType ElementIdx)
		{
			return ElementAccessor(Context, RowIndex, ElementIdx);
		}
		
		template <typename U = AccessElementType>
		FORCEINLINE TEnableIfWritable<TRealtimeMeshIndexedBufferAccessor&, U> SetElement(SizeType ElementIdx, const AccessElementType& NewElementValue)
		{
			GetElement(ElementIdx) = NewElementValue;
			return *this;
		}

		FORCEINLINE ConstElementAccessor operator[](SizeType ElementIdx) const
		{
			return GetElement(ElementIdx);
		}
		
		template <typename U = AccessElementType>
		FORCEINLINE TEnableIfWritable<ElementAccessor, U> operator[](SizeType ElementIdx)
		{
			return GetElement(ElementIdx);
		}
	
		template <typename U = AccessType>
		FORCEINLINE TEnableIfWritable<TRealtimeMeshIndexedBufferAccessor&, U> operator=(const AccessType& InNewValue)
		{
			Set(InNewValue);
			return *this;
		}		
	

		template <typename U = AccessType>
		FORCEINLINE TEnableIfWritable<TRealtimeMeshIndexedBufferAccessor&, U> SetRange(SizeType StartElementIdx, TArrayView<AccessElementType> Elements)
		{
			checkf(StartElementIdx + Elements.Num() <= NumElements, TEXT("Too many elements passed to SetRange"));
			for (int32 ElementIdx = 0; ElementIdx < Elements.Num(); ++ElementIdx)
			{
				SetElement(StartElementIdx + ElementIdx, Elements[ElementIdx]);
			}
			return *this;
		}
		
		template <typename U = AccessType>
		FORCEINLINE TEnableIfWritable<TRealtimeMeshIndexedBufferAccessor&, U> SetAll(TArrayView<AccessElementType> Elements)
		{
			return SetAll<U>(0, Elements);
		}

		template <typename U = AccessType>
		FORCEINLINE TEnableIfWritable<TRealtimeMeshIndexedBufferAccessor&, U> SetRange(SizeType StartElementIdx, std::initializer_list<AccessElementType> Elements)
		{
			return SetAll<U>(StartElementIdx, MakeArrayView(Elements));
		}

		template <typename U = AccessType>
		FORCEINLINE TEnableIfWritable<TRealtimeMeshIndexedBufferAccessor&, U> SetAll(std::initializer_list<AccessElementType> Elements)
		{
			return SetAll<U>(0, MakeArrayView(Elements));
		}

		template <typename U = AccessType>
		FORCEINLINE TEnableIfWritable<TRealtimeMeshIndexedBufferAccessor&, U> SetRange(SizeType StartElementIdx, const TArray<AccessElementType>& Elements)
		{
			return SetAll<U>(StartElementIdx, MakeArrayView(Elements));
		}

		template <typename U = AccessType>
		FORCEINLINE TEnableIfWritable<TRealtimeMeshIndexedBufferAccessor&, U> SetAll(const TArray<AccessElementType>& Elements)
		{
			return SetAll<U>(0, MakeArrayView(Elements));
		}

#define DEFINE_BINARY_OPERATOR_VARIATIONS(ret, op) \
		FORCEINLINE friend ret operator op(const TRealtimeMeshIndexedBufferAccessor& Left, const TRealtimeMeshIndexedBufferAccessor& Right) { return Left.Get() op Right.Get(); } \
		FORCEINLINE friend ret operator op(const TRealtimeMeshIndexedBufferAccessor& Left, const AccessType& Right) { return Left.Get() op Right; } \
		FORCEINLINE friend ret operator op(const AccessType& Left, const TRealtimeMeshIndexedBufferAccessor& Right) { return Left op Right.Get(); }
		DEFINE_BINARY_OPERATOR_VARIATIONS(bool, ==)
		DEFINE_BINARY_OPERATOR_VARIATIONS(bool, !=)
		DEFINE_BINARY_OPERATOR_VARIATIONS(bool, <)
		DEFINE_BINARY_OPERATOR_VARIATIONS(bool, >)
		DEFINE_BINARY_OPERATOR_VARIATIONS(bool, <=)
		DEFINE_BINARY_OPERATOR_VARIATIONS(bool, >=)
		DEFINE_BINARY_OPERATOR_VARIATIONS(AccessType, +)
		DEFINE_BINARY_OPERATOR_VARIATIONS(AccessType, -)
		DEFINE_BINARY_OPERATOR_VARIATIONS(AccessType, *)
		DEFINE_BINARY_OPERATOR_VARIATIONS(AccessType, /)
		DEFINE_BINARY_OPERATOR_VARIATIONS(AccessType, %)
		DEFINE_BINARY_OPERATOR_VARIATIONS(AccessType, &)
		DEFINE_BINARY_OPERATOR_VARIATIONS(AccessType, |)
		DEFINE_BINARY_OPERATOR_VARIATIONS(AccessType, ^)
		DEFINE_BINARY_OPERATOR_VARIATIONS(AccessType, <<)
		DEFINE_BINARY_OPERATOR_VARIATIONS(AccessType, >>)
#undef DEFINE_BINARY_OPERATOR_VARIATIONS
		
#define DEFINE_BINARY_OPERATOR_VARIATIONS(op, simpleop) \
		template <typename U = AccessType> \
		FORCEINLINE TEnableIfWritable<TRealtimeMeshIndexedBufferAccessor&, U> operator op(const AccessType& Right) { Set(Get() simpleop Right); return *this; } \
		template <typename U = AccessType> \
		FORCEINLINE TEnableIfWritable<TRealtimeMeshIndexedBufferAccessor&, U> operator op(const TRealtimeMeshIndexedBufferAccessor& Right) { Set(Get() simpleop Right.Get()); return *this; }
		DEFINE_BINARY_OPERATOR_VARIATIONS(+=, +)
		DEFINE_BINARY_OPERATOR_VARIATIONS(-=, -)
		DEFINE_BINARY_OPERATOR_VARIATIONS(*=, *)
		DEFINE_BINARY_OPERATOR_VARIATIONS(/=, /)
		DEFINE_BINARY_OPERATOR_VARIATIONS(%=, %)
		DEFINE_BINARY_OPERATOR_VARIATIONS(&=, &)
		DEFINE_BINARY_OPERATOR_VARIATIONS(|=, |)
		DEFINE_BINARY_OPERATOR_VARIATIONS(^=, ^)
		DEFINE_BINARY_OPERATOR_VARIATIONS(<<=, <<)
		DEFINE_BINARY_OPERATOR_VARIATIONS(>>=, >>)
#undef DEFINE_BINARY_OPERATOR_VARIATIONS	
	};





	template <typename InAccessType, typename InBufferType, bool bAllowSubstreamAccess>
	struct TRealtimeMeshStreamBuilderBase
	{
	public:
		using SizeType = FRealtimeMeshStream::SizeType;

		using AccessTypeTraits = TRealtimeMeshStreamDataAccessorTypeTraits<InAccessType>;
		using BufferTypeTraits = TRealtimeMeshStreamDataAccessorTypeTraits<InBufferType>;

		using AccessType = typename AccessTypeTraits::Type;
		using BufferTypeT = typename BufferTypeTraits::Type;

		using AccessElementType = typename AccessTypeTraits::ElementType;
		using BufferElementTypeT = typename BufferTypeTraits::ElementType;

		static constexpr int32 NumElements = AccessTypeTraits::NumElements;
		static constexpr bool IsSingleElement = NumElements == 1;
		static constexpr bool IsWritable = !AccessTypeTraits::IsConst;

		using StreamDataAccessor = TRealtimeMeshStreamDataAccessor<InAccessType, InBufferType, bAllowSubstreamAccess>;

		template<typename RetVal, typename Dummy>
		using TEnableIfWritable = std::enable_if_t<sizeof(Dummy) && IsWritable, RetVal>;

		using ElementAccessor = TRealtimeMeshElementAccessor<InAccessType, InBufferType, bAllowSubstreamAccess>;
		using ConstElementAccessor = TRealtimeMeshElementAccessor<const InAccessType, InBufferType, bAllowSubstreamAccess>;

		using RowAccessor = TRealtimeMeshIndexedBufferAccessor<InAccessType, InBufferType, bAllowSubstreamAccess>;
		using ConstRowAccessor = TRealtimeMeshIndexedBufferAccessor<const InAccessType, InBufferType, bAllowSubstreamAccess>;
	protected:
		// Underlying data stream we're responsible for, as well as any conversion that needs to happen
		typename StreamDataAccessor::TContext Context;

	public:
		TRealtimeMeshStreamBuilderBase(typename TCopyQualifiersFromTo<InAccessType, FRealtimeMeshStream>::Type& InStream, int32 ElementOffset = 0)
			: Context(StreamDataAccessor::InitializeContext(InStream, ElementOffset))
		{
			if constexpr (!std::is_void_v<BufferTypeT>)
			{
				checkf(InStream.GetLayout().GetElementType() == GetRealtimeMeshBufferLayout<BufferTypeT>().GetElementType(),
					TEXT("Supplied stream not correct format for builder."));				
			}
			checkf(bAllowSubstreamAccess || ElementOffset == 0, TEXT("Cannot offset element within stream without substream access enabled."));
		}
		
		TRealtimeMeshStreamBuilderBase(const TRealtimeMeshStreamBuilderBase&) = default;
		TRealtimeMeshStreamBuilderBase& operator=(const TRealtimeMeshStreamBuilderBase&) = delete;
		TRealtimeMeshStreamBuilderBase(TRealtimeMeshStreamBuilderBase&&) = default;
		TRealtimeMeshStreamBuilderBase& operator=(TRealtimeMeshStreamBuilderBase&&) = delete;

		FORCEINLINE const FRealtimeMeshStream& GetStream() const { return Context.Stream; }
		
		template <typename U = AccessType>
		FORCEINLINE TEnableIfWritable<FRealtimeMeshStream&, U> GetStream() { return Context.Stream; }
		FORCEINLINE const FRealtimeMeshBufferLayout& GetBufferLayout() const { return Context.Stream.GetLayout(); }

		FORCEINLINE SizeType Num() const { return Context.Stream.Num(); }

		FORCEINLINE SIZE_T GetAllocatedSize() const { return Context.Stream.GetAllocatedSize(); }
		FORCEINLINE SizeType GetSlack() const { return Context.Stream.GetSlack(); }
		FORCEINLINE bool IsValidIndex(SizeType Index) const { return Context.Stream.IsValidIndex(Index); }
		FORCEINLINE bool IsEmpty() const { return Context.Stream.IsEmpty(); }
		FORCEINLINE SizeType Max() const { return Context.Stream.Max(); }





		

		template <typename U = AccessType>
		FORCEINLINE TEnableIfWritable<RowAccessor, U> AddUninitialized()
		{
			return RowAccessor(Context, Context.Stream.AddUninitialized());
		}

		template <typename U = AccessType>
		FORCEINLINE TEnableIfWritable<SizeType, U> AddUninitialized(SizeType Count)
		{
			return Context.Stream.AddUninitialized(Count);
		}

		template <typename U = AccessType>
		FORCEINLINE TEnableIfWritable<RowAccessor, U> AddZeroed(SizeType Count = 1)
		{
			return RowAccessor(Context, Context.Stream.AddZeroed(Count));
		}

		template <typename U = AccessType>
		FORCEINLINE TEnableIfWritable<void, U> Shrink()
		{
			Context.Stream.Shrink();
		}

		template <typename U = AccessType>
		FORCEINLINE TEnableIfWritable<void, U> Empty(SizeType ExpectedUseSize = 0, SizeType MaxSlack = 0)
		{
			Context.Stream.Empty(ExpectedUseSize, MaxSlack);			
		}

		template <typename U = AccessType>
		FORCEINLINE TEnableIfWritable<void, U> Reserve(SizeType Number)
		{
			Context.Stream.Reserve(Number);
		}

		template <typename U = AccessType>
		FORCEINLINE TEnableIfWritable<void, U> SetNumUninitialized(SizeType NewNum)
		{
			Context.Stream.SetNumUninitialized(NewNum);
		}

		template <typename U = AccessType>
		FORCEINLINE TEnableIfWritable<void, U> SetNumZeroed(SizeType NewNum)
		{
			Context.Stream.SetNumZeroed(NewNum);
		}

		template <typename U = AccessType>
		FORCEINLINE TEnableIfWritable<void, U> SetNumWithFill(SizeType NewNum, const AccessType& FillValue)
		{
			const int32 StartIndex = Context.Stream.Num();
			Context.Stream.SetNumUninitialized(NewNum);
			for (int32 Index = StartIndex; Index < NewNum; Index++)
			{
				Edit(Index) = FillValue;
			}
		}

		template <typename U = AccessType>
		FORCEINLINE TEnableIfWritable<void, U> RemoveAt(SizeType Index, SizeType Count = 1)
		{
			Context.Stream.RemoveAt(Index, Count);
		}

		FORCEINLINE AccessType GetValue(int32 Index) const
		{
			return StreamDataAccessor::GetBufferValue(Context, Index);
		}

		FORCEINLINE AccessElementType GetElementValue(SizeType Index, SizeType ElementIdx) const
		{
			return StreamDataAccessor::GetElementValue(Context, Index, ElementIdx);
		}
		
		FORCEINLINE ConstRowAccessor Get(SizeType Index) const
		{
			return ConstRowAccessor(Context, Index);
		}
		
		FORCEINLINE RowAccessor Get(SizeType Index)
		{
			return RowAccessor(Context, Index);
		}
		
		FORCEINLINE ConstRowAccessor operator[](int32 Index) const
		{
			return Get(Index);
		}
		
		FORCEINLINE RowAccessor operator[](int32 Index)
		{
			return Get(Index);
		}
		
		template <typename U = AccessType>
		FORCEINLINE TEnableIfWritable<RowAccessor, U> Add()
		{
			return AddUninitialized().Set(AccessType());
		}
		
		template <typename U = AccessType>
		FORCEINLINE TEnableIfWritable<SizeType, U> Add(const AccessType& Entry)
		{
			return AddUninitialized().Set(Entry).GetIndex();
		}
		
				
		template <typename U = AccessType>
		FORCEINLINE TEnableIfWritable<SizeType, U> Emplace(TArrayView<AccessElementType> Elements)
		{
			return Add().SetAll(Elements).GetIndex();
		}

		template <typename U = AccessType>
		FORCEINLINE TEnableIfWritable<SizeType, U> Emplace(const TArray<AccessElementType>& Elements)
		{
			return Emplace(MakeArrayView(Elements));
		}

		template <typename U = AccessType>
		FORCEINLINE TEnableIfWritable<SizeType, U> Emplace(std::initializer_list<AccessElementType> Elements)
		{
			return Emplace(MakeArrayView(Elements));
		}

		

		template <typename U = AccessType>
		FORCEINLINE TEnableIfWritable<RowAccessor, U> Edit(SizeType Index)
		{
			return Get(Index);
		}

		template <typename U = AccessType>
		FORCEINLINE TEnableIfWritable<void, U> Set(int32 Index, const AccessType& Entry)
		{
			Edit(Index) = Entry;
		}
		
		template <typename U = AccessType>
		FORCEINLINE TEnableIfWritable<void, U> SetElements(int32 Index, TArrayView<AccessElementType> Elements)
		{			
			Edit(Index).SetAll(Elements);
		}
		
		template <typename U = AccessType>
		FORCEINLINE TEnableIfWritable<void, U> SetElements(int32 Index, const TArray<AccessElementType>& Elements)
		{			
			Edit(Index).SetAll(MakeArrayView(Elements));
		}
		
		template <typename U = AccessType>
		FORCEINLINE TEnableIfWritable<void, U> SetElements(int32 Index, std::initializer_list<AccessElementType> Elements)
		{			
			Edit(Index).SetAll(MakeArrayView(Elements));
		}
		
		

		template <typename U = AccessType>
		FORCEINLINE TEnableIfWritable<void, U> SetElement(int32 Index, int32 ElementIndex, const AccessElementType& Element)
		{
			Edit(Index).SetElement(ElementIndex, Element);
		}


		template <typename GeneratorFunc>
		FORCEINLINE TEnableIfWritable<void, GeneratorFunc> SetElementGenerator(int32 StartIndex, int32 Count, int32 ElementIndex, GeneratorFunc Generator)
		{
			RangeCheck(StartIndex + Count - 1);
			ElementCheck(ElementIndex);
			for (int32 Index = 0; Index < Count; Index++)
			{
				Edit(StartIndex + Index).SetElement(ElementIndex, Generator(Index, StartIndex + Index));
			}
		}

		template <typename U = AccessType>
		FORCEINLINE TEnableIfWritable<void, U> SetRange(int32 StartIndex, TArrayView<AccessType> Elements)
		{
			RangeCheck(StartIndex + Elements.Num() - 1);
			for (int32 Index = 0; Index < Elements.Num(); Index++)
			{
				StreamDataAccessor::SetBufferValue(Context, StartIndex + Index, Elements[Index]);
			}
		}

		template <typename InAllocatorType>
		FORCEINLINE TEnableIfWritable<void, InAllocatorType> SetRange(int32 StartIndex, const TArray<AccessType, InAllocatorType>& Elements)
		{
			SetRange(StartIndex, MakeArrayView(Elements));
		}

		template <typename U = AccessType>
		FORCEINLINE TEnableIfWritable<void, U> SetRange(int32 StartIndex, AccessType* Elements, int32 Count)
		{
			SetRange(StartIndex, MakeArrayView(Elements, Count));
		}

		template <typename U = AccessType>
		FORCEINLINE TEnableIfWritable<void, U> SetRange(int32 StartIndex, std::initializer_list<AccessType> Elements)
		{
			SetRange(StartIndex, MakeArrayView(Elements.begin(), Elements.size()));
		}

		template <typename GeneratorFunc>
		FORCEINLINE TEnableIfWritable<void, GeneratorFunc> SetGenerator(int32 StartIndex, int32 Count, GeneratorFunc Generator)
		{
			RangeCheck(StartIndex + Count - 1);
			for (int32 Index = 0; Index < Count; Index++)
			{
				const int32 FinalIndex = StartIndex + Index;
				Edit(FinalIndex) = Invoke(Generator, Index, FinalIndex);
			}
		}

		template <typename U = AccessType>
		FORCEINLINE TEnableIfWritable<void, U> Append(TArrayView<AccessType> Elements)
		{
			const SizeType StartIndex = AddUninitialized(Elements.Num());
			SetRange(StartIndex, Elements);
		}

		template <typename InAllocatorType>
		FORCEINLINE TEnableIfWritable<void, InAllocatorType> Append(const TArray<AccessType, InAllocatorType>& Elements)
		{
			Append(MakeArrayView(Elements));
		}

		template <typename U = AccessType>
		FORCEINLINE TEnableIfWritable<void, U> Append(AccessType* Elements, int32 Count)
		{
			Append(MakeArrayView(Elements, Count));
		}

		template <typename U = AccessType>
		FORCEINLINE TEnableIfWritable<void, U> Append(std::initializer_list<AccessType> Elements)
		{
			Append(MakeArrayView(Elements.begin(), Elements.size()));
		}

		template <typename GeneratorFunc>
		FORCEINLINE TEnableIfWritable<void, GeneratorFunc> AppendGenerator(int32 Count, GeneratorFunc Generator)
		{
			if (Count > 0)
			{
				const SizeType StartIndex = AddUninitialized<GeneratorFunc>(Count);
				SetGenerator<GeneratorFunc>(StartIndex, Count, Forward<GeneratorFunc>(Generator));
			}
		}

		


	protected:
		static FORCEINLINE void ElementCheck(int32 ElementIndex)
		{
			checkf((ElementIndex >= 0) & (ElementIndex < NumElements), TEXT("Element index out of bounds: %d from an element list of size %d"), ElementIndex,
			       NumElements); // & for one branch
		}
		
		FORCEINLINE void RangeCheck(SizeType Index) const
		{
			checkf((Index >= 0) & (Index < Num()), TEXT("Array index out of bounds: %lld from an array of size %lld"), static_cast<int64>(Index),
			       static_cast<int64>(Num())); // & for one branch
		}
	};



	template<typename InAccessType, typename InBufferType = InAccessType>
	using TRealtimeMeshStreamBuilder = TRealtimeMeshStreamBuilderBase<InAccessType, InBufferType, false>;
	
	template<typename InAccessType, typename InBufferType = InAccessType>
	using TRealtimeMeshStridedStreamBuilder = TRealtimeMeshStreamBuilderBase<InAccessType, InBufferType, true>;

	template <typename MeshBuilderType>
	struct TRealtimeMeshVertexBuilderLocal;


	

	/**
	 * Implements a helper class for building normal realtime mesh data. This is specialized towards the needs of the FLocalVertexFactory
	 * If you implement a custom vertex factory you may want to build your own builder structure, possibly less generic than this one
	 * to accomplish the same goals.
	 * The building blocks of this builder are reusable in other tools
	 * @tparam IndexType Type used for an individual index, usually uint16 or uint32 for 16 or 32 bit indices respectively
	 * @tparam TangentElementType Type used for a single tangent, usually FPackedNormal or FPackedRGBA16N
	 * @tparam TexCoordElementType Type used for a single tex coord, usually FVector2f or FVector2DHalf
	 * @tparam NumTexCoords Number of texture coordinates to use, supported values are between 1 and 8
	 * @tparam PolyGroupIndexType Type used to index the polygroup, usually a uint8, but larger values can be used for more polygroups
	 */
	template <typename IndexType = uint32, typename TangentElementType = FPackedNormal, typename TexCoordElementType = FVector2DHalf, int32 NumTexCoords = 1, typename PolyGroupIndexType = uint16>
	struct TRealtimeMeshBuilderLocal
	{
		using TriangleType = TIndex3<uint32>;
		using TangentAccessType = TRealtimeMeshTangents<FVector4f>;
		using TexCoordAccessType = TRealtimeMeshTexCoords<FVector2f, NumTexCoords>;

		using TriangleStreamType = typename TRealtimeMeshPromoteTypeIfNotVoid<IndexType, TIndex3<IndexType>>::Type;
		using TangentStreamType = typename TRealtimeMeshPromoteTypeIfNotVoid<TangentElementType, TRealtimeMeshTangents<TangentElementType>>::Type;
		using TexCoordStreamType = typename TRealtimeMeshPromoteTypeIfNotVoid<TexCoordElementType, TRealtimeMeshTexCoords<TexCoordElementType, NumTexCoords>>::Type;

		using TriangleStreamBuilder = TRealtimeMeshStreamBuilder<TriangleType, TriangleStreamType>;
		using PositionStreamBuilder = TRealtimeMeshStreamBuilder<FVector3f, FVector3f>;
		using TangentStreamBuilder = TRealtimeMeshStreamBuilder<TangentAccessType, TangentStreamType>;
		using TexCoordStreamBuilder = TRealtimeMeshStridedStreamBuilder<TexCoordAccessType, TexCoordStreamType>;
		using ColorStreamBuilder = TRealtimeMeshStreamBuilder<FColor, FColor>;
		using PolyGroupStreamBuilder = TRealtimeMeshStreamBuilder<uint32, PolyGroupIndexType>;
		
		using VertexBuilder = TRealtimeMeshVertexBuilderLocal<TRealtimeMeshBuilderLocal>;
		using SizeType = PositionStreamBuilder::SizeType;

		static_assert(std::is_void<TexCoordElementType>::value || FRealtimeMeshBufferTypeTraits<TexCoordStreamType>::NumElements == NumTexCoords, "NumTexCoords must match the number of elements in the TexCoordStreamType");

	private:
		FRealtimeMeshStreamSet& Streams;

		PositionStreamBuilder Vertices;
		TOptional<TangentStreamBuilder> Tangents;
		TOptional<TexCoordStreamBuilder> TexCoords;
		TOptional<ColorStreamBuilder> Colors;

		TriangleStreamBuilder Triangles;
		TOptional<TriangleStreamBuilder> DepthOnlyTriangles;
		
		TOptional<PolyGroupStreamBuilder> TrianglePolyGroups;
		TOptional<PolyGroupStreamBuilder> DepthOnlyTrianglePolyGroups;

		template <typename AccessLayout, typename DataLayout = AccessLayout, bool bAllowSubstreamAccess = false>
		TOptional<TRealtimeMeshStreamBuilderBase<AccessLayout, DataLayout, bAllowSubstreamAccess>> GetStreamBuilder(const FRealtimeMeshStreamKey& StreamKey,
			bool bCreateIfNotAvailable = false, const FRealtimeMeshBufferLayout& DefaultLayout = FRealtimeMeshBufferLayout::Invalid, bool bForceConvertToDefaultType = false)
		{			
			if (auto* ExistingStream = Streams.Find(StreamKey))
			{
				if constexpr (std::is_same_v<AccessLayout, DataLayout>)
				{
					// Make sure the desired format matches the format as they should all be equivalent
					check(GetRealtimeMeshBufferLayout<AccessLayout>() == DefaultLayout || DefaultLayout == FRealtimeMeshBufferLayout::Invalid);
					check(ExistingStream->IsOfType(GetRealtimeMeshBufferLayout<AccessLayout>()));
				}
				else if constexpr (!std::is_same_v<DataLayout, void>)
				{
					// If the concrete data type is valid, make sure the stream is in this format
					check(ExistingStream->IsOfType<DataLayout>());
				}
				else
				{
					if (bAllowSubstreamAccess)
					{
						const FRealtimeMeshElementType FromType = ExistingStream->GetLayout().GetElementType();
						const FRealtimeMeshElementType ToType = GetRealtimeMeshBufferLayout<AccessLayout>().GetElementType();
						check(FRealtimeMeshTypeConversionUtilities::CanConvert(FromType, ToType));
						check(FRealtimeMeshTypeConversionUtilities::CanConvert(ToType, FromType));
						check(GetRealtimeMeshBufferLayout<AccessLayout>().GetNumElements() <= ExistingStream->GetNumElements());
					}
					else
					{
						// Convert on the fly, make sure it's convertible to this type
						check(ExistingStream->CanConvertTo<AccessLayout>());
					}
				}

				if (bForceConvertToDefaultType && !ExistingStream->IsOfType(DefaultLayout))
				{
					ExistingStream->ConvertTo(DefaultLayout);
				}
				
				return TRealtimeMeshStreamBuilderBase<AccessLayout, DataLayout, bAllowSubstreamAccess>(*ExistingStream);
			}

			if (bCreateIfNotAvailable)
			{
				FRealtimeMeshBufferLayout FinalLayout;
				if constexpr (!std::is_void_v<DataLayout>)
				{
					FinalLayout = GetRealtimeMeshBufferLayout<DataLayout>();					
				}
				else
				{
					FinalLayout = DefaultLayout;
				}
				FRealtimeMeshStream& NewStream = Streams.AddStream(StreamKey, FinalLayout);
				return TRealtimeMeshStreamBuilderBase<AccessLayout, DataLayout, bAllowSubstreamAccess>(NewStream);
			}
			
			return TOptional<TRealtimeMeshStreamBuilderBase<AccessLayout, DataLayout, bAllowSubstreamAccess>>();
		}

	public:

		TRealtimeMeshBuilderLocal(FRealtimeMeshStreamSet& InExistingStreams)
			: Streams(InExistingStreams)
			, Vertices(GetStreamBuilder<FVector3f>(FRealtimeMeshStreams::Position, true, GetRealtimeMeshBufferLayout<FVector3f>())->GetStream())
			, Tangents(GetStreamBuilder<TangentAccessType, TangentStreamType>(FRealtimeMeshStreams::Tangents))
			, TexCoords(GetStreamBuilder<TexCoordAccessType, TexCoordStreamType, true>(FRealtimeMeshStreams::TexCoords))
			, Colors(GetStreamBuilder<FColor>(FRealtimeMeshStreams::Color))
			, Triangles(GetStreamBuilder<TriangleType, TriangleStreamType>(FRealtimeMeshStreams::Triangles, true, GetRealtimeMeshBufferLayout<TIndex3<uint16>>())->GetStream())
			, DepthOnlyTriangles(GetStreamBuilder<TriangleType, TriangleStreamType>(FRealtimeMeshStreams::DepthOnlyTriangles))
			, TrianglePolyGroups(GetStreamBuilder<uint32, PolyGroupIndexType>(FRealtimeMeshStreams::PolyGroups))
			, DepthOnlyTrianglePolyGroups(GetStreamBuilder<uint32, PolyGroupIndexType>(FRealtimeMeshStreams::DepthOnlyPolyGroups))
		{
			Streams.AddStreamToLinkPool("Vertices", FRealtimeMeshStreams::Position);
			if (Tangents.IsSet())
			{				
				Streams.AddStreamToLinkPool("Vertices", FRealtimeMeshStreams::Tangents,
					FRealtimeMeshStreamDefaultRowValue::Create(TRealtimeMeshTangents<FVector4f>(FVector3f::ZAxisVector, FVector3f::XAxisVector), Tangents->GetBufferLayout()));
			}
			if (TexCoords.IsSet())
			{
				Streams.AddStreamToLinkPool("Vertices", FRealtimeMeshStreams::TexCoords);
			}

			if (Colors.IsSet())
			{
				Streams.AddStreamToLinkPool("Vertices", FRealtimeMeshStreams::Color, FRealtimeMeshStreamDefaultRowValue::Create(FColor::White));
			}

			Streams.AddStreamToLinkPool("Triangles", FRealtimeMeshStreams::Triangles);

			if (TrianglePolyGroups.IsSet())
			{
				Streams.AddStreamToLinkPool("Triangles", FRealtimeMeshStreams::PolyGroups);
			}

			if (DepthOnlyTriangles.IsSet())
			{
				Streams.AddStreamToLinkPool("DepthOnlyTriangles", FRealtimeMeshStreams::DepthOnlyTriangles);
			}
			if (DepthOnlyTrianglePolyGroups.IsSet())
			{
				Streams.AddStreamToLinkPool("DepthOnlyTriangles", FRealtimeMeshStreams::DepthOnlyPolyGroups);
			}

			
		}




		
		FORCEINLINE bool HasTangents() const { return Tangents.IsSet(); }
		FORCEINLINE bool HasTexCoords() const { return TexCoords.IsSet(); }
		FORCEINLINE bool HasVertexColors() const { return Colors.IsSet(); }
		FORCEINLINE SizeType NumTexCoordChannels() const { return HasTexCoords()? TexCoords->NumElements : 0; }
		FORCEINLINE bool HasDepthOnlyTriangles() const { return DepthOnlyTriangles.IsSet(); }		
		FORCEINLINE bool HasPolyGroups() const { return TrianglePolyGroups.IsSet(); }


		void EnableTangents(const FRealtimeMeshElementType& ElementType)
		{
			checkf(ElementType.IsValid(), TEXT("Element type must be a valid type"));
			if constexpr (std::is_void<TangentElementType>::value)
			{
				checkf(FRealtimeMeshTypeConversionUtilities::CanConvert(ElementType, GetRealtimeMeshDataElementType<FVector4f>()),
					TEXT("ElementType must be convertible to FVector4f"));				
			}
			else
			{
				checkf(ElementType == GetRealtimeMeshDataElementType<TangentElementType>(),
					TEXT("ElementType must be the same as the TangentElementType, unless the builder is dynamic by supplying void to the TangentElementType of the builder template"));
			}			
			
			if (!Tangents.IsSet())
			{				
				Tangents = GetStreamBuilder<TangentAccessType, TangentStreamType>(FRealtimeMeshStreams::Tangents, true, GetRealtimeMeshBufferLayout(ElementType, 2), true);
				Streams.AddStreamToLinkPool("Vertices", FRealtimeMeshStreams::Tangents,
				FRealtimeMeshStreamDefaultRowValue::Create(TRealtimeMeshTangents<FVector4f>(FVector3f::ZAxisVector, FVector3f::XAxisVector), Tangents->GetBufferLayout()));
			}
		}		
		template<typename NewTangentElementType = typename TRealtimeMeshUseTypeOrDefaultIfVoid<TangentElementType, FPackedNormal>::Type>
		void EnableTangents()
		{
			static_assert(TRealtimeMeshIsVoidOrSameType<TangentElementType, NewTangentElementType>::Value,
				"New tangent type does not match type the builder assumes. Please match the type or allow the builder to use a dynamic access by passing void to TangentElementType");
			
			EnableTangents(GetRealtimeMeshDataElementType<NewTangentElementType>());
		}
		void DisableTangents(bool bRemoveExistingStreams = true)
		{
			Tangents.Reset();
			if (bRemoveExistingStreams)
			{
				Streams.Remove(FRealtimeMeshStreams::Tangents);
			}
		}

		void EnableColors()
		{
			if (!Colors.IsSet())
			{
				Colors = GetStreamBuilder<FColor>(FRealtimeMeshStreams::Color, true, GetRealtimeMeshBufferLayout<FColor>());
				Streams.AddStreamToLinkPool("Vertices", FRealtimeMeshStreams::Color, FRealtimeMeshStreamDefaultRowValue::Create(FColor::White));
			}
		}
		void DisableColors(bool bRemoveExistingStreams = true)
		{
			Colors.Reset();
			if (bRemoveExistingStreams)
			{
				Streams.Remove(FRealtimeMeshStreams::Color);
			}
		}

		void EnableTexCoords(const FRealtimeMeshElementType& ElementType, int32 NumChannels = 1)
		{
			checkf(ElementType.IsValid(), TEXT("Element type must be a valid type"));
			checkf(NumChannels >= NumTexCoords, TEXT("NumChannels must be greater or equal to the number of tex coords"));

			if constexpr (std::is_void<TexCoordElementType>::value)
			{
				checkf(FRealtimeMeshTypeConversionUtilities::CanConvert(ElementType, GetRealtimeMeshDataElementType<FVector2f>()),
					TEXT("ElementType must be convertible to FVector2f"));	
			}
			else
			{
				checkf(ElementType == GetRealtimeMeshDataElementType<TexCoordElementType>(),
					TEXT("ElementType must be the same as the TexCoordStreamType, unless the builder is dynamic by supplying void to the TexCoordStreamType of the builder template"));
			}
			
			if (!TexCoords.IsSet())
			{
				TexCoords = GetStreamBuilder<TexCoordAccessType, TexCoordStreamType, true>(FRealtimeMeshStreams::TexCoords, true, GetRealtimeMeshBufferLayout(ElementType, NumChannels), true);
				Streams.AddStreamToLinkPool("Vertices", FRealtimeMeshStreams::TexCoords);
			}
		}	
		template<typename NewTexCoordElementType = typename TRealtimeMeshUseTypeOrDefaultIfVoid<TexCoordElementType, FVector2DHalf>::Type>
		void EnableTexCoords(int32 NumChannels = NumTexCoords)
		{			
			static_assert(TRealtimeMeshIsVoidOrSameType<TexCoordElementType, NewTexCoordElementType>::Value,
				"New TexCoord type does not match type the builder assumes. Please match the type or allow the builder to use a dynamic access by passing void to TexCoordElementType");
			
			EnableTexCoords(GetRealtimeMeshDataElementType<NewTexCoordElementType>(), NumChannels);
		}
		void DisableTexCoords(bool bRemoveExistingStreams = true)
		{
			TexCoords.Reset();
			if (bRemoveExistingStreams)
			{
				Streams.Remove(FRealtimeMeshStreams::TexCoords);
			}
		}

		void EnableDepthOnlyTriangles(const FRealtimeMeshElementType& ElementType)
		{
			checkf(ElementType.IsValid(), TEXT("Element type must be a valid type"));
			
			if constexpr (std::is_void<IndexType>::value)
			{
				checkf(FRealtimeMeshTypeConversionUtilities::CanConvert(ElementType, GetRealtimeMeshDataElementType<uint32>()),
					TEXT("ElementType must be convertible to uint32"));
			}
			else
			{
				checkf(ElementType == GetRealtimeMeshDataElementType<IndexType>(),
					TEXT("ElementType must be the same as the IndexType, unless the builder is dynamic by supplying void to the IndexType of the builder template"));
			}
						
			if (!DepthOnlyTriangles.IsSet())
			{
				DepthOnlyTriangles = GetStreamBuilder<TriangleType, TriangleStreamType>(FRealtimeMeshStreams::DepthOnlyTriangles, true, GetRealtimeMeshBufferLayout(ElementType, 3), true);
				Streams.AddStreamToLinkPool("DepthOnlyTriangles", FRealtimeMeshStreams::DepthOnlyTriangles);
			}

			if (HasPolyGroups())
			{
				check(TrianglePolyGroups.IsSet());
				DepthOnlyTrianglePolyGroups = GetStreamBuilder<uint32, PolyGroupIndexType>(FRealtimeMeshStreams::DepthOnlyPolyGroups, true,
					TrianglePolyGroups->GetBufferLayout());
				Streams.AddStreamToLinkPool("DepthOnlyTriangles", FRealtimeMeshStreams::DepthOnlyPolyGroups);				
			}
		}			
		template<typename NewIndexType = typename TRealtimeMeshUseTypeOrDefaultIfVoid<IndexType, uint16>::Type>
		void EnableDepthOnlyTriangles()
		{
			static_assert(TRealtimeMeshIsVoidOrSameType<IndexType, NewIndexType>::Value,
				"New index type does not match type the builder assumes. Please match the type or allow the builder to use a dynamic access by passing void to IndexType");

			EnableDepthOnlyTriangles(GetRealtimeMeshDataElementType<NewIndexType>());
		}
		void DisableDepthOnlyTriangles(bool bRemoveExistingStreams = true)
		{
			DepthOnlyTriangles.Reset();
			if (bRemoveExistingStreams)
			{
				Streams.Remove(FRealtimeMeshStreams::DepthOnlyTriangles);
			}
		}

		
		
		void EnablePolyGroups(const FRealtimeMeshElementType& ElementType)
		{
			checkf(ElementType.IsValid(), TEXT("Element type must be a valid type"));
			
			if constexpr (std::is_void<PolyGroupIndexType>::value)
			{
				checkf(FRealtimeMeshTypeConversionUtilities::CanConvert(ElementType, GetRealtimeMeshDataElementType<uint32>()),
					TEXT("TexCoord ElementType must be convertible to uint32"));
			}
			else
			{
				checkf(ElementType == GetRealtimeMeshDataElementType<PolyGroupIndexType>(),
					TEXT("ElementType must be the same as the PolyGroupIndexType, unless the builder is dynamic by supplying void to the PolyGroupIndexType of the builder template"));
			}
			
			if (!TrianglePolyGroups.IsSet())
			{
				TrianglePolyGroups = GetStreamBuilder<uint32, PolyGroupIndexType>(FRealtimeMeshStreams::PolyGroups, true, GetRealtimeMeshBufferLayout(ElementType, 1), true);
				Streams.AddStreamToLinkPool("Triangles", FRealtimeMeshStreams::PolyGroups);
			}
			if (HasDepthOnlyTriangles() && !DepthOnlyTrianglePolyGroups.IsSet())
			{
				DepthOnlyTrianglePolyGroups = GetStreamBuilder<uint32, PolyGroupIndexType>(FRealtimeMeshStreams::DepthOnlyPolyGroups, true, GetRealtimeMeshBufferLayout(ElementType, 1), true);
				Streams.AddStreamToLinkPool("DepthOnlyTriangles", FRealtimeMeshStreams::DepthOnlyPolyGroups);
			}
		}
		template<typename NewPolyGroupIndexType = typename TRealtimeMeshUseTypeOrDefaultIfVoid<PolyGroupIndexType, uint16>::Type>
		void EnablePolyGroups()
		{
			static_assert(TRealtimeMeshIsVoidOrSameType<PolyGroupIndexType, NewPolyGroupIndexType>::Value,
				"New poly group index type does not match type the builder assumes. Please match the type or allow the builder to use a dynamic access by passing void to PolyGroupIndexType");
			
			EnablePolyGroups(GetRealtimeMeshDataElementType<NewPolyGroupIndexType>());
		}
		void DisablePolyGroups(bool bRemoveExistingStreams = true)
		{
			TrianglePolyGroups.Reset();
			DepthOnlyTrianglePolyGroups.Reset();
			if (bRemoveExistingStreams)
			{
				Streams.Remove(FRealtimeMeshStreams::PolyGroups);
				Streams.Remove(FRealtimeMeshStreams::DepthOnlyPolyGroups);
			}
		}



		SizeType NumVertices() const
		{
			return Vertices.Num();
		}
		void ReserveNumVertices(SizeType TotalNum)
		{
			Vertices.Reserve(TotalNum);
		}
		void ReserveAdditionalVertices(SizeType NumToAdd)
		{
			Vertices.Reserve(Vertices.Num() + NumToAdd);
		}
		void SetNumVertices(SizeType NewNum)
		{
			Vertices.SetNumUninitialized(NewNum);
		}
		void EmptyVertices()
		{
			Vertices.Empty();
		}

		SizeType NumTriangles() const
		{
			return Triangles.Num();
		}
		void ReserveNumTriangles(SizeType TotalNum)
		{
			Triangles.Reserve(TotalNum);
		}
		void ReserveAdditionalTriangles(SizeType NumToAdd)
		{
			Triangles.Reserve(Triangles.Num() + NumToAdd);
		}
		void SetNumTriangles(SizeType NewNum)
		{
			Triangles.SetNumUninitialized(NewNum);
		}
		void EmptyTriangles()
		{
			Triangles.Empty();
		}

		SizeType NumDepthOnlyTriangles() const
		{
			return DepthOnlyTriangles.IsSet() ? DepthOnlyTriangles->Num() : 0;
		}
		void ReserveNumDepthOnlyTriangles(SizeType TotalNum)
		{
			DepthOnlyTriangles->Reserve(TotalNum);
		}
		void ReserveAdditionalDepthOnlyTriangles(SizeType NumToAdd)
		{
			DepthOnlyTriangles->Reserve(DepthOnlyTriangles->Num() + NumToAdd);
		}
		void SetNumDepthOnlyTriangles(SizeType NewNum)
		{
			DepthOnlyTriangles->SetNumUninitialized(NewNum);
		}
		void EmptyDepthOnlyTriangles()
		{
			check(DepthOnlyTriangles.IsSet());
			DepthOnlyTriangles->Empty();
		}


		

		VertexBuilder AddVertex()
		{
			return Vertices.AddZeroed();
		}

		VertexBuilder AddVertex(const FVector3f& InPosition)
		{
			const SizeType Index = Vertices.Add(InPosition);
			return VertexBuilder(*this, Index);
		}

		VertexBuilder EditVertex(int32 VertIdx)
		{
			return VertexBuilder(*this, VertIdx);
		}


		void SetPosition(int32 VertIdx, const FVector3f& InPosition)
		{
			Vertices.Set(VertIdx, InPosition);
		}

		FVector3f GetPosition(int32 VertIdx)
		{
			return Vertices.Get(VertIdx);
		}

		void SetNormal(int32 VertIdx, const FVector3f& Normal)
		{
			checkf(HasTangents(), TEXT("Vertex tangents not enabled"));
			Tangents->SetElement(VertIdx, 1, FVector4f(Normal, Tangents->GetElementValue(VertIdx, 1).W));
		}

		FVector3f GetNormal(int32 VertIdx)
		{
			checkf(HasTangents(), TEXT("Vertex tangents not enabled"));
			return Tangents->GetElementValue(VertIdx, 1);			
		}

		void SetTangent(int32 VertIdx, const FVector3f& Tangent)
		{
			checkf(HasTangents(), TEXT("Vertex tangents not enabled"));
			Tangents->SetElement(VertIdx, 0, FVector4f(Tangent));
		}

		FVector3f GetTangent(int32 VertIdx)
		{			
			checkf(HasTangents(), TEXT("Vertex tangents not enabled"));
			return Tangents->GetElementValue(VertIdx, 0);
		}

		void SetNormalAndTangent(int32 VertIdx, const FVector3f& Normal, const FVector3f& Tangent, bool bShouldFlipBinormal = false)
		{
			checkf(HasTangents(), TEXT("Vertex tangents not enabled"));
			Tangents->Set(VertIdx, TRealtimeMeshTangents<FVector4f>(Normal, Tangent, bShouldFlipBinormal));
		}

		void SetTangents(int32 VertIdx, const FVector3f& Normal, const FVector3f& Binormal, const FVector3f& Tangent)
		{
			checkf(HasTangents(), TEXT("Vertex tangents not enabled"));
			Tangents->Set(VertIdx, TRealtimeMeshTangents<FVector4f>(Normal, Tangent, Tangent));
		}

		void SetTexCoord(int32 VertIdx, int32 TexCoordIdx, const FVector2f& TexCoord)
		{
			checkf(HasTexCoords(), TEXT("Vertex texcoords not enabled"));
			if constexpr (FRealtimeMeshBufferTypeTraits<TexCoordStreamType>::NumElements == 1)
			{
				check(TexCoordIdx == 0);
				TexCoords->Set(VertIdx, TexCoord);
			}
			else
			{
				TexCoords->SetElement(VertIdx, TexCoordIdx, TexCoord);
			}
		}

		FVector2f GetTexCoord(int32 VertIdx, int32 TexCoordIdx)
		{			
			checkf(HasTexCoords(), TEXT("Vertex texcoords not enabled"));
			if constexpr (FRealtimeMeshBufferTypeTraits<TexCoordStreamType>::NumElements == 1)
			{
				check(TexCoordIdx == 0);
				return TexCoords->GetValue(VertIdx);
			}
			else
			{
				return TexCoords->GetElementValue(VertIdx, TexCoordIdx);
			}
		}

		void SetColor(int32 VertIdx, FColor VertexColor)
		{
			checkf(HasVertexColors(), TEXT("Vertex colors not enabled"));
			if (Colors->Num() <= VertIdx)
			{
				Colors->AppendGenerator(VertIdx + 1 - Colors->Num(), [](int32, int32) { return FColor::White; });
			}
			Colors->Set(VertIdx, VertexColor);
		}

		void SetColor(int32 VertIdx, const FLinearColor& VertexColor, bool bSRGB = true)
		{
			checkf(HasVertexColors(), TEXT("Vertex colors not enabled"));
			if (Colors->Num() <= VertIdx)
			{
				Colors->AppendGenerator(VertIdx + 1 - Colors->Num(), [](int32, int32) { return FColor::White; });
			}
			Colors->Set(VertIdx, VertexColor.ToFColor(bSRGB));
		}
		
		FColor GetColor(int32 VertIdx)
		{
			checkf(HasVertexColors(), TEXT("Vertex colors not enabled"));
			return Colors->Get(VertIdx);
		}


		SizeType AddTriangle(const TIndex3<uint32>& Triangle)
		{
			return Triangles.Add(Triangle);
		}
		
		SizeType AddTriangle(const TIndex3<uint32>& Triangle, uint32 MaterialIndex)
		{
			checkf(HasPolyGroups(), TEXT("Triangle material indices not enabled"));
			auto Result = Triangles.Add(Triangle);
			TrianglePolyGroups->Set(Result, MaterialIndex);
			return Result;
		}

		SizeType AddTriangle(uint32 Vert0, uint32 Vert1, uint32 Vert2)
		{
			return Triangles.Add(TIndex3<uint32>(Vert0, Vert1, Vert2));
		}

		SizeType AddTriangle(uint32 Vert0, uint32 Vert1, uint32 Vert2, uint32 MaterialIndex)
		{
			checkf(HasPolyGroups(), TEXT("Triangle material indices not enabled"));
			auto Result = Triangles.Add(TIndex3<uint32>(Vert0, Vert1, Vert2));
			TrianglePolyGroups->Set(Result, MaterialIndex);
			return Result;
		}

		void SetTriangle(SizeType Index, const TIndex3<uint32>& NewTriangle)
		{
			Triangles.Set(Index, NewTriangle);
		}

		void SetTriangle(SizeType Index, const TIndex3<uint32>& NewTriangle, uint32 MaterialIndex)
		{
			checkf(HasPolyGroups(), TEXT("Triangle material indices not enabled"));
			Triangles.Set(Index, NewTriangle);
			TrianglePolyGroups->Set(Index, MaterialIndex);
		}

		void SetTriangle(SizeType Index, uint32 Vert0, uint32 Vert1, uint32 Vert2)
		{
			Triangles.Set(Index, TIndex3<uint32>(Vert0, Vert1, Vert2));
		}

		void SetTriangle(SizeType Index, uint32 Vert0, uint32 Vert1, uint32 Vert2, uint32 MaterialIndex)
		{
			checkf(HasPolyGroups(), TEXT("Triangle material indices not enabled"));
			Triangles.Set(Index, TIndex3<uint32>(Vert0, Vert1, Vert2));
			TrianglePolyGroups->Set(Index, MaterialIndex);
		}

		TIndex3<uint32> GetTriangle(SizeType Index)
		{
			return Triangles.Get(Index);
		}
		
		uint32 GetMaterialIndex(SizeType Index)
		{
			checkf(HasPolyGroups(), TEXT("Triangle material indices not enabled"));
			return TrianglePolyGroups->Get(Index);
		}

		int32 AddDepthOnlyTriangle(const TIndex3<uint32>& Triangle)
		{
			checkf(HasDepthOnlyTriangles(), TEXT("Depth only triangles not enabled"));
			return DepthOnlyTriangles.Add(Triangle);
		}
		int32 AddDepthOnlyTriangle(const TIndex3<uint32>& Triangle, uint32 MaterialIndex)
		{
			checkf(HasDepthOnlyTriangles(), TEXT("Depth only triangles not enabled"));
			checkf(HasPolyGroups(), TEXT("Depth only triangle material indices not enabled"));
			auto Result = DepthOnlyTriangles.Add(Triangle);
			DepthOnlyTrianglePolyGroups->Set(Result, MaterialIndex);
			return Result;
		}

		int32 AddDepthOnlyTriangle(uint32 Vert0, uint32 Vert1, uint32 Vert2)
		{
			checkf(HasDepthOnlyTriangles(), TEXT("Depth only triangles not enabled"));
			return DepthOnlyTriangles.Add(TIndex3<uint32>(Vert0, Vert1, Vert2));
		}
		
		int32 AddDepthOnlyTriangle(uint32 Vert0, uint32 Vert1, uint32 Vert2, uint32 MaterialIndex)
		{
			checkf(HasDepthOnlyTriangles(), TEXT("Depth only triangles not enabled"));
			checkf(HasPolyGroups(), TEXT("Depth only triangle material indices not enabled"));
			auto Result = DepthOnlyTriangles.Add(TIndex3<uint32>(Vert0, Vert1, Vert2));
			DepthOnlyTrianglePolyGroups->Set(Result, MaterialIndex);
			return Result;
		}

		void SetDepthOnlyTriangle(SizeType Index, const TIndex3<uint32>& NewTriangle)
		{
			checkf(HasDepthOnlyTriangles(), TEXT("Depth only triangles not enabled"));
			DepthOnlyTriangles.Set(Index, NewTriangle);
		}
		
		void SetDepthOnlyTriangle(SizeType Index, const TIndex3<uint32>& NewTriangle, uint32 MaterialIndex)
		{
			checkf(HasDepthOnlyTriangles(), TEXT("Depth only triangles not enabled"));
			checkf(HasPolyGroups(), TEXT("Depth only triangle material indices not enabled"));
			DepthOnlyTriangles.Set(Index, NewTriangle);
			DepthOnlyTrianglePolyGroups->Set(Index, MaterialIndex);
		}

		void SetDepthOnlyTriangle(SizeType Index, uint32 Vert0, uint32 Vert1, uint32 Vert2)
		{
			checkf(HasDepthOnlyTriangles(), TEXT("Depth only triangles not enabled"));
			DepthOnlyTriangles.Set(Index, TIndex3<uint32>(Vert0, Vert1, Vert2));
		}
		
		void SetDepthOnlyTriangle(SizeType Index, uint32 Vert0, uint32 Vert1, uint32 Vert2, uint32 MaterialIndex)
		{
			checkf(HasDepthOnlyTriangles(), TEXT("Depth only triangles not enabled"));
			checkf(HasPolyGroups(), TEXT("Depth only triangle material indices not enabled"));
			DepthOnlyTriangles.Set(Index, TIndex3<uint32>(Vert0, Vert1, Vert2));
			DepthOnlyTrianglePolyGroups->Set(Index, MaterialIndex);
		}
		
		TIndex3<uint32> GetDepthOnlyTriangle(SizeType Index)
		{
			checkf(HasDepthOnlyTriangles(), TEXT("Depth only triangles not enabled"));
			return DepthOnlyTriangles.Get(Index);
		}
		
		uint32 GetDepthOnlyMaterialIndex(SizeType Index)
		{
			checkf(HasDepthOnlyTriangles(), TEXT("Depth only triangles not enabled"));
			checkf(HasPolyGroups(), TEXT("Depth only triangle material indices not enabled"));
			return DepthOnlyTrianglePolyGroups->Get(Index);
		}

	};


	template <typename MeshBuilderType>
	struct TRealtimeMeshVertexBuilderLocal : FNoncopyable
	{
		using VertexBuilder = TRealtimeMeshVertexBuilderLocal<MeshBuilderType>;
		using SizeType = typename MeshBuilderType::SizeType;
	private:
		MeshBuilderType& ParentBuilder;
		const SizeType RowIndex;
	public:
		TRealtimeMeshVertexBuilderLocal(MeshBuilderType& InBuilder, SizeType InRowIndex)
			: ParentBuilder(InBuilder), RowIndex(InRowIndex)
		{
		}
		
		SizeType GetIndex() const { return RowIndex; }
		operator SizeType() const { return GetIndex(); }
		
		bool HasTangents() const { return ParentBuilder.HasTangents(); }
		bool HasTexCoords() const { return ParentBuilder.HasTexCoords(); }
		bool HasVertexColors() const { return ParentBuilder.HasVertexColors(); }
		SizeType NumTexCoordChannels() const { return ParentBuilder.NumTexCoordChannels(); }



		VertexBuilder& SetPosition(const FVector3f& Position)
		{
			ParentBuilder.SetPosition(RowIndex, Position);
			return *this;
		}

		FVector3f GetPosition() const
		{
			return ParentBuilder.GetPosition(RowIndex);
		}

		VertexBuilder& SetNormal(const FVector3f& Normal)
		{
			ParentBuilder.SetNormal(RowIndex, Normal);
			return *this;
		}

		FVector3f GetNormal() const
		{
			checkf(HasTangents(), TEXT("Vertex tangents not enabled"));
			return ParentBuilder.GetNormal(RowIndex);
		}

		VertexBuilder& SetTangent(const FVector3f& Tangent)
		{
			ParentBuilder.SetTangent(RowIndex, Tangent);
			return *this;
		}

		FVector3f GetTangent() const
		{
			checkf(HasTangents(), TEXT("Vertex tangents not enabled"));
			return ParentBuilder.GetTangent(RowIndex);
		}

		VertexBuilder& SetNormalAndTangent(const FVector3f& Normal, const FVector3f& Tangent)
		{
			ParentBuilder.SetNormalAndTangent(RowIndex, Normal, Tangent);
			return *this;
		}

		VertexBuilder& SetTangents(const FVector3f& Normal, const FVector3f& Binormal, const FVector3f& Tangent)
		{
			ParentBuilder.SetTangents(RowIndex, Normal, Binormal, Tangent);
			return *this;
		}

		VertexBuilder& SetTexCoord(const FVector2f& TexCoord)
		{
			ParentBuilder.SetTexCoord(RowIndex, 0, TexCoord);
			return *this;
		}

		VertexBuilder& SetTexCoord(int32 TexCoordIdx, const FVector2f& TexCoord)
		{
			ParentBuilder.SetTexCoord(RowIndex, TexCoordIdx, TexCoord);
			return *this;
		}

		FVector2f GetTexCoord(int32 TexCoordIdx = 0) const
		{
			checkf(HasTexCoords(), TEXT("Vertex texcoords not enabled"));
			return ParentBuilder.GetTexCoord(RowIndex, TexCoordIdx);
		}

		VertexBuilder& SetColor(FColor VertexColor)
		{
			ParentBuilder.SetColor(RowIndex, VertexColor);
			return *this;
		}

		VertexBuilder& SetColor(const FLinearColor& VertexColor, bool bSRGB = true)
		{
			ParentBuilder.SetColor(RowIndex, VertexColor.ToFColor(bSRGB));
			return *this;
		}

		FColor GetColor() const
		{
			checkf(HasVertexColors(), TEXT("Vertex colors not enabled"));
			return ParentBuilder.GetColor(RowIndex);
		}

		FLinearColor GetLinearColor() const
		{
			return FLinearColor(GetColor());
		}
		
	};

	
}
