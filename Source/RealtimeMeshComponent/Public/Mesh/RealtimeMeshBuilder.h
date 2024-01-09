// Copyright TriAxis Games, L.L.C. All Rights Reserved.


#pragma once

#include "RealtimeMeshDataTypes.h"
#include "RealtimeMeshDataStream.h"
#include "Templates/Invoke.h"
#include "RealtimeMeshAlgo.h"

struct FRealtimeMeshSimpleMeshData;

// ReSharper disable CppMemberFunctionMayBeConst
namespace RealtimeMesh
{
	template<typename BufferType>
	struct TRealtimeMeshIndexedBufferAccessor
	{
	public:
		using SizeType = FRealtimeMeshStream::SizeType;
		using BufferTypeRaw = std::remove_const_t<BufferType>;
		using ElementType = typename TCopyQualifiersFromTo<BufferType, typename FRealtimeMeshBufferTypeTraits<BufferTypeRaw>::ElementType>::Type;
		static constexpr int32 NumElements = FRealtimeMeshBufferTypeTraits<BufferTypeRaw>::NumElements;
		static constexpr bool IsSingleElement = NumElements == 1;
		static constexpr bool IsConst = TIsConst<BufferType>::Value;


		template<typename RetVal, typename Dummy>
		using TEnableIfWritable = std::enable_if_t<sizeof(Dummy) && !IsConst, RetVal>;
		
		template<typename RetVal, typename Dummy>
		using TEnableIfMultiElement = std::enable_if_t<sizeof(Dummy) && !IsSingleElement, RetVal>;
		template<typename RetVal, typename Dummy>
		using TEnableIfSingleElement = std::enable_if_t<sizeof(Dummy) && IsSingleElement, RetVal>;
		
		template<typename RetVal, typename Dummy>
		using TEnableIfWritableMultiElement = std::enable_if_t<sizeof(Dummy) && !IsSingleElement && !IsConst, RetVal>;
		template<typename RetVal, typename Dummy>
		using TEnableIfWritableSingleElement = std::enable_if_t<sizeof(Dummy) && IsSingleElement && !IsConst, RetVal>;
		
	protected:
		FRealtimeMeshStream& Stream;
		SizeType RowIndex;

	public:
		TRealtimeMeshIndexedBufferAccessor(FRealtimeMeshStream& InStream, int32 InRowIndex)
			: Stream(InStream)
			, RowIndex(InRowIndex)
		{
			
		}
		SizeType GetIndex() const { return RowIndex; }

		BufferType& Get() { return *Stream.GetDataAtVertex<BufferType>(RowIndex); }

		operator BufferType&() { return Get(); }
				
		template <typename U = BufferType>
		TEnableIfWritable<TRealtimeMeshIndexedBufferAccessor&, U> operator=(const BufferType& InNewValue)
		{
			Get() = InNewValue;
			return *this;
		}
		
		template <typename U = BufferType>
		TEnableIfMultiElement<ElementType&, U> GetElement(SizeType ElementIdx)
		{
			return *Stream.GetDataAtVertex<ElementType>(RowIndex, ElementIdx);
		}	
		
		template <typename U = BufferType>
		TEnableIfSingleElement<BufferType&, U> GetElement(SizeType ElementIdx)
		{
			check(ElementIdx == 0);
			return Get();
		}
		
		template <typename U = BufferType>
		TEnableIfMultiElement<ElementType&, U> operator[](SizeType ElementIdx)
		{
			return GetElement(ElementIdx);
		}
		
		template <typename U = BufferType>
		TEnableIfSingleElement<BufferType&, U> operator[](SizeType ElementIdx)
		{
			check(ElementIdx == 0);
			return Get();
		}
		
		template <typename U = BufferType>
		TEnableIfWritable<TRealtimeMeshIndexedBufferAccessor&, U> Set(const BufferType& NewValue)
		{
			Get() = NewValue;
			return *this;
		}
		
		template <typename U = BufferType>
		TEnableIfWritableMultiElement<TRealtimeMeshIndexedBufferAccessor&, U> SetElement(SizeType ElementIdx, const ElementType& NewElementValue)
		{
			GetElement(ElementIdx) = NewElementValue;
			return *this;
		}
		
		template <typename U = BufferType>
		TEnableIfWritableSingleElement<TRealtimeMeshIndexedBufferAccessor&, U> SetElement(SizeType ElementIdx, const BufferType& NewElementValue)
		{
			check(ElementIdx == 0);
			Get() = NewElementValue;
			return *this;
		}
		
		template <typename U = BufferType>
		TEnableIfMultiElement<TRealtimeMeshIndexedBufferAccessor&, U> Get(SizeType ElementIdx, ElementType& Element)
		{
			Element = GetElement(ElementIdx);
			return *this;
		}
		
		template <typename U = BufferType>
		TEnableIfSingleElement<TRealtimeMeshIndexedBufferAccessor&, U> Get(SizeType ElementIdx, BufferTypeRaw& Element)
		{
			check(ElementIdx == 0);
			Element = Get();
			return *this;
		}

		template <typename U = BufferType>
		TEnableIfWritableMultiElement<TRealtimeMeshIndexedBufferAccessor&, U> Tie(SizeType ElementIdx, ElementType*& Element)
		{
			Element = &GetElement(ElementIdx);
			return *this;
		}
		

		
		template <typename U = BufferType, typename... ArgTypes>
		TEnableIfWritableMultiElement<TRealtimeMeshIndexedBufferAccessor&, U> SetRange(SizeType StartElementIdx, const ArgTypes&... Elements)
		{
			static_assert(sizeof...(ArgTypes) <= NumElements, "Too many elements passed to SetRange");
			//static_assert(std::conjunction_v<std::is_constructible<ElementType, ArgTypes>...>, "Unable to convert all parameters to ElementType");
			checkf(sizeof...(ArgTypes) + StartElementIdx <= NumElements, TEXT("Too many elements passed to SetRange"));

			SetInternal<U, ArgTypes...>(StartElementIdx, Elements...);
			return *this;
		}
		
		template <typename U = BufferType, typename ArgType>
		TEnableIfWritableSingleElement<TRealtimeMeshIndexedBufferAccessor&, U> SetRange(SizeType StartElementIdx, const ArgType& Element)
		{
			static_assert(std::is_constructible_v<ElementType, ArgType>, "Unable to convert parameter to ElementType");
			check(StartElementIdx == 0);
			SetElement(Element);
			return *this;
		}

		template <typename U = BufferType, typename... ArgTypes>
		TEnableIfWritableMultiElement<TRealtimeMeshIndexedBufferAccessor&, U> SetAll(const ArgTypes&... Elements)
		{
			static_assert(sizeof...(ArgTypes) == NumElements, "Wrong number of elements passed to SetAll");
			//static_assert(std::conjunction_v<std::is_constructible<ElementType, ArgTypes>...>, "Unable to convert all parameters to ElementType");

			SetInternal<U, ArgTypes...>(0, Elements...);
			return *this;
		}
		
		template <typename U = BufferType, typename ArgType>
		TEnableIfWritableSingleElement<TRealtimeMeshIndexedBufferAccessor&, U> SetAll(const ArgType& Element)
		{
			static_assert(std::is_constructible_v<ElementType, ArgType>, "Unable to convert parameter to ElementType");

			SetElement(Element);
			return *this;
		}
		

	protected:
		template <typename U = BufferType, typename ArgType>
		TEnableIfWritableMultiElement<void, U> SetInternal(SizeType ElementIdx, const ArgType& Element)
		{
			check(ElementIdx < NumElements);
			GetElement(ElementIdx) = ElementType(Element);
		}

		template <typename U = BufferType, typename ArgType, typename... ArgTypes>
		TEnableIfWritableMultiElement<void, U> SetInternal(SizeType StartElementIdx, const ArgType& FirstElement, const ArgTypes&... RemainingElements)
		{
			SetInternal<U, ArgType>(StartElementIdx, FirstElement);
			SetInternal<U, ArgTypes...>(StartElementIdx + 1, RemainingElements...);
		}		
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
	
	template <typename BufferType>
	struct TRealtimeMeshStreamBuilder
	{
	public:
		using SizeType = FRealtimeMeshStream::SizeType;
		using ElementType = typename TCopyQualifiersFromTo<BufferType, typename FRealtimeMeshBufferTypeTraits<BufferType>::ElementType>::Type;
		using TBufferRowAccessor = TRealtimeMeshIndexedBufferAccessor<BufferType>;
		using TConstBufferRowAccessor = TRealtimeMeshIndexedBufferAccessor<const BufferType>;
		static constexpr int32 NumElements = FRealtimeMeshBufferTypeTraits<BufferType>::NumElements;
		static constexpr bool IsSingleElement = NumElements == 1;

		
		template<typename RetVal, typename Dummy>
		using TEnableIfMultiElement = std::enable_if_t<sizeof(Dummy) && !IsSingleElement, RetVal>;
		template<typename RetVal, typename Dummy>
		using TEnableIfSingleElement = std::enable_if_t<sizeof(Dummy) && IsSingleElement, RetVal>;

		DECLARE_DELEGATE_ThreeParams(FUpdateSizeDelegate, FRealtimeMeshStreamBuilderEventType, SizeType, SizeType);
	protected:
		// Underlying data stream we're responsible for
		FRealtimeMeshStream& Stream;
		FUpdateSizeDelegate UpdateSizeDelegate;

	public:
		TRealtimeMeshStreamBuilder(FRealtimeMeshStream& InStream, bool bConvertDataIfNecessary = false)
			: Stream(InStream)
		{
			if (bConvertDataIfNecessary && !Stream.IsOfType<BufferType>())
			{
				Stream.ConvertTo<BufferType>();
			}

			check(Stream.IsOfType<BufferType>());
		}
		
		TRealtimeMeshStreamBuilder(const TRealtimeMeshStreamBuilder&) = default;
		TRealtimeMeshStreamBuilder& operator=(const TRealtimeMeshStreamBuilder&) = delete;
		TRealtimeMeshStreamBuilder(TRealtimeMeshStreamBuilder&&) = default;
		TRealtimeMeshStreamBuilder& operator=(TRealtimeMeshStreamBuilder&&) = delete;

		FORCEINLINE const FRealtimeMeshStream& GetStream() const { return Stream; }
		FORCEINLINE FRealtimeMeshStream& GetStream() { return Stream; }
		FORCEINLINE const FRealtimeMeshBufferLayout& GetBufferLayout() const { return Stream.GetLayout(); }

		FUpdateSizeDelegate& GetCallbackDelegate() { return UpdateSizeDelegate; }
		void ClearCallbackDelegate() { UpdateSizeDelegate.Unbind(); }

		void SendCallbackNotify(FRealtimeMeshStreamBuilderEventType EventType, SizeType Size, SizeType SecondarySize = 0)
		{
			// ReSharper disable once CppExpressionWithoutSideEffects
			UpdateSizeDelegate.ExecuteIfBound(EventType, Size, SecondarySize);
		}

		FORCEINLINE SizeType Num() const { return Stream.Num(); }

		FORCEINLINE SIZE_T GetAllocatedSize() const { return Stream.GetAllocatedSize(); }
		FORCEINLINE SizeType GetSlack() const { return Stream.GetSlack(); }
		FORCEINLINE bool IsValidIndex(SizeType Index) const { return Stream.IsValidIndex(Index); }
		FORCEINLINE bool IsEmpty() const { return Stream.IsEmpty(); }
		FORCEINLINE SizeType Max() const { return Stream.Max(); }


		FORCEINLINE SizeType AddUninitialized()
		{
			const SizeType Index = Stream.AddUninitialized();
			SendCallbackNotify(FRealtimeMeshStreamBuilderEventType::NewSizeUninitialized, Stream.Num());
			return Index;
		}

		FORCEINLINE SizeType AddUninitialized(SizeType Count)
		{
			const SizeType Index = Stream.AddUninitialized(Count);
			SendCallbackNotify(FRealtimeMeshStreamBuilderEventType::NewSizeUninitialized, Stream.Num());
			return Index;
		}

		FORCEINLINE SizeType AddZeroed(SizeType Count = 1)
		{
			const SizeType Index = Stream.AddZeroed(Count);
			SendCallbackNotify(FRealtimeMeshStreamBuilderEventType::NewSizeZeroed, Stream.Num());
			return Index;
		}

		FORCEINLINE void Shrink()
		{
			Stream.Shrink();
			SendCallbackNotify(FRealtimeMeshStreamBuilderEventType::Shrink, Stream.Num());
		}

		FORCEINLINE void Empty(SizeType ExpectedUseSize = 0, SizeType MaxSlack = 0)
		{
			Stream.Empty(ExpectedUseSize, MaxSlack);			
			SendCallbackNotify(FRealtimeMeshStreamBuilderEventType::Empty, ExpectedUseSize, MaxSlack);
		}

		FORCEINLINE void Reserve(SizeType Number)
		{
			Stream.Reserve(Number);
			SendCallbackNotify(FRealtimeMeshStreamBuilderEventType::Reserve, Number);
		}

		FORCEINLINE void SetNumUninitialized(SizeType NewNum)
		{
			Stream.SetNumUninitialized(NewNum);
			SendCallbackNotify(FRealtimeMeshStreamBuilderEventType::NewSizeUninitialized, NewNum);
		}

		FORCEINLINE void SetNumZeroed(SizeType NewNum)
		{
			Stream.SetNumZeroed(NewNum);
			SendCallbackNotify(FRealtimeMeshStreamBuilderEventType::NewSizeZeroed, NewNum);
		}

		FORCEINLINE void RemoveAt(SizeType Index, SizeType Count = 1)
		{
			Stream.RemoveAt(Index, Count);
			SendCallbackNotify(FRealtimeMeshStreamBuilderEventType::RemoveAt, Index, Count);
		}

		FORCEINLINE BufferType& GetValue(SizeType Index)
		{
			return *Stream.GetDataAtVertex<BufferType>(Index);
		}

		FORCEINLINE const BufferType& GetValue(int32 Index) const
		{
			return *Stream.GetDataAtVertex<BufferType>(Index);
		}
		
		ElementType& GetElementValue(SizeType Index, SizeType ElementIdx)
		{
			return *Stream.GetDataAtVertex<ElementType>(Index, ElementIdx);
		}

		const ElementType& GetElementValue(SizeType Index, SizeType ElementIdx) const
		{
			return *Stream.GetDataAtVertex<ElementType>(Index, ElementIdx);
		}
		
		FORCEINLINE TBufferRowAccessor Get(SizeType Index)
		{
			return TBufferRowAccessor(Stream, Index);
		}

		FORCEINLINE TConstBufferRowAccessor Get(int32 Index) const
		{
			return TConstBufferRowAccessor(Stream, Index);
		}

		TConstBufferRowAccessor operator[](int32 Index) const
		{
			return Get(Index);
		}
		
		TBufferRowAccessor operator[](int32 Index)
		{
			return Get(Index);
		}
		
		TBufferRowAccessor Add()
		{
			const SizeType Index = AddUninitialized();
			return TBufferRowAccessor(Stream, Index);
		}
		
		SizeType Add(const BufferType& Entry)
		{
			const SizeType Index = AddUninitialized();
			Get(Index) = Entry;
			return Index;
		}
		
		template <typename ArgType, SIZE_T ArgCount>
		TEnableIfMultiElement<SizeType, ArgType> Add(ArgType (&Entries)[ArgCount])
		{
			TBufferRowAccessor Writer = Add();
			check(ArgCount < NumElements);
			for (int32 ElementIndex = 0; ElementIndex < ArgCount; ElementIndex++)
			{
				Writer.SetElement(ElementIndex, Entries[ElementIndex]);
			}
			return Writer.GetIndex();
		}
		
				
		template <typename ArgType, typename... ArgTypes>
		TEnableIfMultiElement<SizeType, ArgType> Add(const ArgType& Element, const ArgTypes&... Elements)
		{
			TBufferRowAccessor Writer = Add();
			Writer.SetRange(0, Element, Elements...);
			return Writer.GetIndex();
		}

		TBufferRowAccessor Edit(SizeType Index)
		{
			return Get(Index);
		}

		void Set(int32 Index, const BufferType& Entry)
		{
			Get(Index) = Entry;
		}
		
		template <typename ArgType, SIZE_T ArgCount>
		void Set(int32 Index, ArgType (&Entries)[ArgCount])
		{
			check(ArgCount < NumElements);
			for (int32 ElementIndex = 0; ElementIndex < ArgCount; ElementIndex++)
			{
				GetElementValue(Index, ElementIndex) = Entries[ElementIndex];
			}
		}
		
		template <typename ArgType>
		typename TEnableIf<!TIsArray<ArgType>::Value>::Type Set(int32 Index, const ArgType& Entry)
		{
			Get(Index) = BufferType(Entry);
		}

		void SetElement(int32 Index, int32 ElementIndex, const ElementType& Element)
		{
			GetElementValue(Index, ElementIndex) = Element;
		}

		template <typename ArgType>
		void SetElement(int32 Index, int32 ElementIndex, const ArgType& Element)
		{
			GetElementValue(Index, ElementIndex) = ElementType(Element);
		}

		template <typename... ArgTypes>
		void SetElements(int32 Index, const ArgTypes&... Elements)
		{
			static_assert(sizeof...(ArgTypes) <= NumElements, "Wrong number of elements passed to SetElements");
			checkf(sizeof...(ArgTypes) + Index <= NumElements, TEXT("Wrong number of elements passed to SetElements"));
			static_assert(std::conjunction_v<std::is_assignable<ElementType, const ArgTypes&>...>, "Unable to convert all parameters to ElementType");

			TBufferRowAccessor Writer = Edit(Index);
			Writer.SetRange(0, Elements...);
		}

		template <typename GeneratorFunc>
		void SetElementGenerator(int32 StartIndex, int32 Count, int32 ElementIndex, GeneratorFunc Generator)
		{
			RangeCheck(StartIndex + Count - 1);
			ElementCheck(ElementIndex);
			for (int32 Index = 0; Index < Count; Index++)
			{
				GetElementValue(StartIndex + Index, ElementIndex) = Generator(Index, StartIndex + Index);
			}
		}
		
		TConstArrayView<BufferType, SizeType> GetView() const
		{
			return TConstArrayView<BufferType, SizeType>(Stream.GetData(), Num());
		}

		TArrayView<BufferType, SizeType> GetView()
		{
			return TArrayView<BufferType, SizeType>(Stream.GetData(), Num());
		}

		void SetRange(int32 StartIndex, TArrayView<BufferType> Elements)
		{
			RangeCheck(StartIndex + Elements.Num() - 1);
			FMemory::Memcpy(&Get(StartIndex), Elements.GetData(), sizeof(BufferType) * Elements.Num());
		}

		template <typename InAllocatorType>
		void SetRange(int32 StartIndex, const TArray<BufferType, InAllocatorType>& Elements)
		{
			SetRange(StartIndex, MakeArrayView(Elements));
		}

		void SetRange(int32 StartIndex, BufferType* Elements, int32 Count)
		{
			SetRange(StartIndex, MakeArrayView(Elements, Count));
		}

		void SetRange(int32 StartIndex, std::initializer_list<BufferType> Elements)
		{
			SetRange(StartIndex, MakeArrayView(Elements.begin(), Elements.size()));
		}

		template <typename GeneratorFunc>
		void SetGenerator(int32 StartIndex, int32 Count, GeneratorFunc Generator)
		{
			RangeCheck(StartIndex + Count - 1);
			BufferType* DataPtr = Stream.GetDataAtVertex<BufferType>(StartIndex);
			for (int32 Index = 0; Index < Count; Index++)
			{
				DataPtr[Index] = Invoke(Generator, Index, StartIndex + Index);
			}
		}

		void Append(TArrayView<BufferType> Elements)
		{
			const SizeType StartIndex = AddUninitialized(Elements.Num());
			SetRange(StartIndex, Elements);
		}

		template <typename InAllocatorType>
		void Append(const TArray<BufferType, InAllocatorType>& Elements)
		{
			Append(MakeArrayView(Elements));
		}

		void Append(BufferType* Elements, int32 Count)
		{
			Append(MakeArrayView(Elements, Count));
		}

		void Append(std::initializer_list<BufferType> Elements)
		{
			Append(MakeArrayView(Elements.begin(), Elements.size()));
		}

		template <typename GeneratorFunc>
		void AppendGenerator(int32 Count, GeneratorFunc Generator)
		{
			if (Count > 0)
			{
				const SizeType StartIndex = AddUninitialized(Count);
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



	template <typename IndexType, typename TangentType, typename TexCoordType, int32 NumTexCoords>
	struct TRealtimeMeshVertexBuilderLocal;




	template <typename IndexType = uint32, typename TangentType = FPackedNormal, typename TexCoordType = FVector2DHalf, int32 NumTexCoords = 1>
	struct TRealtimeMeshBuilderLocal
	{
		using VertexBuilder = TRealtimeMeshVertexBuilderLocal<IndexType, TangentType, TexCoordType, NumTexCoords>;
		using TriangleType = TIndex3<IndexType>;
		using TangentStreamType = TRealtimeMeshTangents<TangentType>;
		using TexCoordStreamType = TRealtimeMeshTexCoords<TexCoordType, NumTexCoords>;
		using SizeType = TRealtimeMeshStreamBuilder<FVector3f>::SizeType;

	private:
		FRealtimeMeshStreamSet& Streams;

		TRealtimeMeshStreamBuilder<FVector3f> Vertices;
		TOptional<TRealtimeMeshStreamBuilder<TangentStreamType>> Tangents;
		TOptional<TRealtimeMeshStreamBuilder<TexCoordStreamType>> TexCoords;
		TOptional<TRealtimeMeshStreamBuilder<FColor>> Colors;

		TRealtimeMeshStreamBuilder<TriangleType> Triangles;
		TOptional<TRealtimeMeshStreamBuilder<TriangleType>> DepthOnlyTriangles;
		
		TOptional<TRealtimeMeshStreamBuilder<uint16>> TrianglePolyGroups;
		TOptional<TRealtimeMeshStreamBuilder<uint16>> DepthOnlyTrianglePolyGroups;

		TOptional<TRealtimeMeshStreamBuilder<FRealtimeMeshPolygonGroupRange>> TriangleSegments;
		TOptional<TRealtimeMeshStreamBuilder<FRealtimeMeshPolygonGroupRange>> DepthOnlyTriangleSegments;

		template <typename StreamLayout>
		TOptional<TRealtimeMeshStreamBuilder<StreamLayout>> GetStreamBuilder(const FRealtimeMeshStreamKey& StreamKey, bool bCreateIfNotAvailable = false)
		{
			if (auto* ExistingStream = Streams.Find(StreamKey))
			{
				if (ExistingStream->GetLayout() != GetRealtimeMeshBufferLayout<StreamLayout>())
				{
					// Convert stream if necessary
					ExistingStream->template ConvertTo<StreamLayout>();					
				}

				return TRealtimeMeshStreamBuilder<StreamLayout>(*ExistingStream);
			}

			if (bCreateIfNotAvailable)
			{
				auto NewStream = Streams.AddStream(StreamKey, GetRealtimeMeshBufferLayout<StreamLayout>());
				return TRealtimeMeshStreamBuilder<StreamLayout>(*NewStream);
			}
			
			return TOptional<TRealtimeMeshStreamBuilder<StreamLayout>>();
		}

		
		void OnVerticesSizeChanged(FRealtimeMeshStreamBuilderEventType SizeChangeType, SizeType NewSize, SizeType MaxSize)
		{
			switch(SizeChangeType)
			{
			case FRealtimeMeshStreamBuilderEventType::NewSizeUninitialized:
				if (Tangents.IsSet())
				{
					Tangents->SetNumUninitialized(NewSize);
				}
				if (Colors.IsSet())
				{
					Colors->SetNumUninitialized(NewSize);
				}
				if (TexCoords.IsSet())
				{
					TexCoords->SetNumUninitialized(NewSize);
				}
				break;					
			case FRealtimeMeshStreamBuilderEventType::NewSizeZeroed:
				if (Tangents.IsSet())
				{
					Tangents->SetNumZeroed(NewSize);
				}
				if (Colors.IsSet())
				{
					Colors->SetNumZeroed(NewSize);
				}
				if (TexCoords.IsSet())
				{
					TexCoords->SetNumZeroed(NewSize);
				}
				break;
			case FRealtimeMeshStreamBuilderEventType::Reserve:
				if (Tangents.IsSet())
				{
					Tangents->Reserve(NewSize);
				}
				if (Colors.IsSet())
				{
					Colors->Reserve(NewSize);
				}
				if (TexCoords.IsSet())
				{
					TexCoords->Reserve(NewSize);
				}
				break;
			case FRealtimeMeshStreamBuilderEventType::Shrink:
				if (Tangents.IsSet())
				{
					Tangents->Shrink();
				}
				if (Colors.IsSet())
				{
					Colors->Shrink();
				}
				if (TexCoords.IsSet())
				{
					TexCoords->Shrink();
				}
				break;
			case FRealtimeMeshStreamBuilderEventType::Empty:
				if (Tangents.IsSet())
				{
					Tangents->Empty(NewSize, MaxSize);
				}
				if (Colors.IsSet())
				{
					Colors->Empty(NewSize, MaxSize);
				}
				if (TexCoords.IsSet())
				{
					TexCoords->Empty(NewSize, MaxSize);
				}
				break;
			case FRealtimeMeshStreamBuilderEventType::RemoveAt:
				if (Tangents.IsSet())
				{
					Tangents->RemoveAt(NewSize, MaxSize);
				}
				if (Colors.IsSet())
				{
					Colors->RemoveAt(NewSize, MaxSize);
				}
				if (TexCoords.IsSet())
				{
					TexCoords->RemoveAt(NewSize, MaxSize);
				}
				break;
			default:
				checkf(false, TEXT("We shouldn't have gotten here..."));
			}
		}
		void OnTrianglesSizeChanged(FRealtimeMeshStreamBuilderEventType SizeChangeType, SizeType NewSize, SizeType MaxSize)
		{
			if (TrianglePolyGroups.IsSet())
			{
				switch(SizeChangeType)
				{
				case FRealtimeMeshStreamBuilderEventType::NewSizeUninitialized:
					TrianglePolyGroups->SetNumUninitialized(NewSize);
					break;
				case FRealtimeMeshStreamBuilderEventType::NewSizeZeroed:
					TrianglePolyGroups->SetNumZeroed(NewSize);
					break;
				case FRealtimeMeshStreamBuilderEventType::Reserve:
					TrianglePolyGroups->Reserve(NewSize);
					break;
				case FRealtimeMeshStreamBuilderEventType::Shrink:
					TrianglePolyGroups->Shrink();
					break;
				case FRealtimeMeshStreamBuilderEventType::Empty:
					TrianglePolyGroups->Empty(NewSize, MaxSize);
					break;
				case FRealtimeMeshStreamBuilderEventType::RemoveAt:
					TrianglePolyGroups->RemoveAt(NewSize, MaxSize);
					break;
				default:
					checkf(false, TEXT("We shouldn't have gotten here..."));
				}
			}
		}
		void OnDepthOnlyTrianglesSizeChanged(FRealtimeMeshStreamBuilderEventType SizeChangeType, SizeType NewSize, SizeType MaxSize)
		{
			if (DepthOnlyTrianglePolyGroups.IsSet())
			{
				switch(SizeChangeType)
				{
				case FRealtimeMeshStreamBuilderEventType::NewSizeUninitialized:
					DepthOnlyTrianglePolyGroups->SetNumUninitialized(NewSize);
					break;
				case FRealtimeMeshStreamBuilderEventType::NewSizeZeroed:
					DepthOnlyTrianglePolyGroups->SetNumZeroed(NewSize);
					break;
				case FRealtimeMeshStreamBuilderEventType::Reserve:
					DepthOnlyTrianglePolyGroups->Reserve(NewSize);
					break;
				case FRealtimeMeshStreamBuilderEventType::Shrink:
					DepthOnlyTrianglePolyGroups->Shrink();
					break;
				case FRealtimeMeshStreamBuilderEventType::Empty:
					DepthOnlyTrianglePolyGroups->Empty(NewSize, MaxSize);
					break;
				case FRealtimeMeshStreamBuilderEventType::RemoveAt:
					DepthOnlyTrianglePolyGroups->RemoveAt(NewSize, MaxSize);
					break;
				default:
					checkf(false, TEXT("We shouldn't have gotten here..."));
				}
			}
		}

	public:

		TRealtimeMeshBuilderLocal(FRealtimeMeshStreamSet& InExistingStreams)
			: Streams(InExistingStreams)
			, Vertices(GetStreamBuilder<FVector3f>(FRealtimeMeshStreams::Position, true)->GetStream())
			, Tangents(GetStreamBuilder<TangentStreamType>(FRealtimeMeshStreams::Tangents))
			, TexCoords(GetStreamBuilder<TexCoordStreamType>(FRealtimeMeshStreams::TexCoords))
			, Colors(GetStreamBuilder<FColor>(FRealtimeMeshStreams::Color))
			, Triangles(GetStreamBuilder<TriangleType>(FRealtimeMeshStreams::Triangles, true)->GetStream())
			, DepthOnlyTriangles(GetStreamBuilder<TriangleType>(FRealtimeMeshStreams::DepthOnlyTriangles))
			, TrianglePolyGroups(GetStreamBuilder<uint16>(FRealtimeMeshStreams::PolyGroups))
			, DepthOnlyTrianglePolyGroups(GetStreamBuilder<uint16>(FRealtimeMeshStreams::DepthOnlyPolyGroups))
			, TriangleSegments(GetStreamBuilder<FRealtimeMeshPolygonGroupRange>(FRealtimeMeshStreams::PolyGroupSegments))
		{
			Vertices.GetCallbackDelegate().BindRaw(this, &TRealtimeMeshBuilderLocal::OnVerticesSizeChanged);
			Triangles.GetCallbackDelegate().BindRaw(this, &TRealtimeMeshBuilderLocal::OnTrianglesSizeChanged);
			if (DepthOnlyTriangles.IsSet())
			{
				DepthOnlyTriangles->GetCallbackDelegate().BindRaw(this, &TRealtimeMeshBuilderLocal::OnDepthOnlyTrianglesSizeChanged);
			}
		}




		
		FORCEINLINE bool HasTangents() const { return Tangents.IsSet(); }
		FORCEINLINE bool HasTexCoords() const { return TexCoords.IsSet(); }
		FORCEINLINE bool HasVertexColors() const { return Colors.IsSet(); }
		FORCEINLINE SizeType NumTexCoordChannels() const { return HasTexCoords()? TexCoords->NumElements : 0; }
		FORCEINLINE bool HasDepthOnlyTriangles() const { return DepthOnlyTriangles.IsSet(); }		
		FORCEINLINE bool HasPolyGroups() const { return TrianglePolyGroups.IsSet(); }
		FORCEINLINE bool HasSegments() const { return TriangleSegments.IsSet(); }


		void EnableTangents()
		{
			if (!Tangents.IsSet())
			{
				Tangents = GetStreamBuilder<TangentStreamType>(FRealtimeMeshStreams::Tangents, true);
				Tangents->AppendGenerator(Vertices.Num(), [](int32 Index, int32 VertexIndex)
				{
					return TRealtimeMeshTangents<TangentType>(FVector3f::ZAxisVector, FVector3f::XAxisVector);
				});
			}
		}
		void DisableTangents()
		{
			Tangents.Reset();
		}

		void EnableColors()
		{
			if (!Colors.IsSet())
			{
				Colors = GetStreamBuilder<FColor>(FRealtimeMeshStreams::Color, true);
				Colors->AppendGenerator(Vertices.Num(), [](int32 Index, int32 VertexIndex)
				{
					return FColor::White;
				});
			}
		}
		void DisableColors()
		{
			Colors.Reset();
		}

		void EnableTexCoords()
		{
			if (!TexCoords.IsSet())
			{
				TexCoords = GetStreamBuilder<TexCoordStreamType>(FRealtimeMeshStreams::TexCoords, true);
				TexCoords->SetNumZeroed(Vertices.Num());
			}
		}
		void DisableTexCoords()
		{
			TexCoords.Reset();
		}

		void EnableDepthOnlyTriangles()
		{
			if (!DepthOnlyTriangles.IsSet())
			{
				DepthOnlyTriangles = GetStreamBuilder<TriangleType>(FRealtimeMeshStreams::DepthOnlyTriangles, true);
				DepthOnlyTriangles->GetCallbackDelegate().BindRaw(this, &TRealtimeMeshBuilderLocal::OnDepthOnlyTrianglesSizeChanged);
			}
		}
		void DisableDepthOnlyTriangles()
		{
			DepthOnlyTriangles.Reset();
		}

		
		void EnablePolyGroups()
		{
			if (!TrianglePolyGroups.IsSet())
			{
				TrianglePolyGroups = GetStreamBuilder<uint16>(FRealtimeMeshStreams::PolyGroups, true);
				TrianglePolyGroups->SetNumZeroed(Vertices.Num());
			}
			if (HasDepthOnlyTriangles() && !DepthOnlyTrianglePolyGroups.IsSet())
			{
				DepthOnlyTrianglePolyGroups = GetStreamBuilder<uint16>(FRealtimeMeshStreams::DepthOnlyPolyGroups, true);
				DepthOnlyTrianglePolyGroups->SetNumZeroed(Vertices.Num());
			}

			TriangleSegments.Reset();
			DepthOnlyTriangleSegments.Reset();
		}

		void DisablePolyGroups()
		{
			TrianglePolyGroups.Reset();
			DepthOnlyTrianglePolyGroups.Reset();
		}

		void EnableTriangleSegments()
		{
			if (!TriangleSegments.IsSet())
			{
				TriangleSegments = GetStreamBuilder<FRealtimeMeshPolygonGroupRange>(FRealtimeMeshStreams::PolyGroupSegments, true);		
			}
			if (!DepthOnlyTriangleSegments.IsSet())
			{
				DepthOnlyTriangleSegments = GetStreamBuilder<FRealtimeMeshPolygonGroupRange>(FRealtimeMeshStreams::DepthOnlyPolyGroupSegments, true);		
			}
			TrianglePolyGroups.Reset();
			DepthOnlyTrianglePolyGroups.Reset();
		}

		void DisableTriangleSegments()
		{
			TriangleSegments.Reset();
			DepthOnlyTriangleSegments.Reset();
		}

		
		void ConvertToMaterialSegments()
		{
			if (!TriangleSegments.IsSet())
			{
				TriangleSegments = GetStreamBuilder<FRealtimeMeshPolygonGroupRange>(FRealtimeMeshStreams::PolyGroupSegments);

				// Convert existing material indices to segments
				if (TrianglePolyGroups.IsSet())
				{
					RealtimeMeshAlgo::GatherSegmentsFromPolygonGroupIndices(
						TrianglePolyGroups->GetStream().template GetArrayView<uint16>(),
						[this](const FRealtimeMeshPolygonGroupRange& NewSegment)
						{
							TriangleSegments->Add(NewSegment);
						});

					// Drop the triangle segments as these two ways of doing things are mutually exclusive
					TrianglePolyGroups.Reset();
				}				
			}

			if (HasDepthOnlyTriangles() && !DepthOnlyTriangleSegments.IsSet())
			{
				DepthOnlyTriangleSegments = GetStreamBuilder<FRealtimeMeshPolygonGroupRange>(FRealtimeMeshStreams::PolyGroupSegments);

				// Convert existing material indices to segments
				if (DepthOnlyTrianglePolyGroups.IsSet())
				{
					RealtimeMeshAlgo::GatherSegmentsFromPolygonGroupIndices(
						DepthOnlyTrianglePolyGroups->GetStream().template GetArrayView<uint16>(),
						[this](const FRealtimeMeshPolygonGroupRange& NewSegment)
						{
							DepthOnlyTriangleSegments->Add(NewSegment);
						});

					// Drop the triangle segments as these two ways of doing things are mutually exclusive
					DepthOnlyTrianglePolyGroups.Reset();
				}				
			}
		}

		void ConvertToPolyGroups()
		{
			if (!TrianglePolyGroups.IsSet())
			{
				TrianglePolyGroups = GetStreamBuilder<uint16>(FRealtimeMeshStreams::PolyGroups);

				// If we have existing segments, we need to copy them into the indices and disable them
				if (TriangleSegments.IsSet())
				{
					auto PolyGroups = TrianglePolyGroups->GetStream().template GetArrayView<uint16>();
					RealtimeMeshAlgo::PropagateTriangleSegmentsToPolygonGroups(
						TriangleSegments->GetStream().template GetArrayView<FRealtimeMeshPolygonGroupRange>(),
						PolyGroups);

					// Drop the triangle segments as these two ways of doing things are mutually exclusive
					TriangleSegments.Reset();
				}
			}

			if (HasDepthOnlyTriangles() && !DepthOnlyTrianglePolyGroups.IsSet())
			{
				DepthOnlyTrianglePolyGroups = GetStreamBuilder<uint16>(FRealtimeMeshStreams::PolyGroups);

				// If we have existing segments, we need to copy them into the indices and disable them
				if (DepthOnlyTriangleSegments.IsSet())
				{
					auto PolyGroups = DepthOnlyTrianglePolyGroups->GetStream().template GetArrayView<uint16>();
					RealtimeMeshAlgo::PropagateTriangleSegmentsToPolygonGroups(
						DepthOnlyTriangleSegments->GetStream().template GetArrayView<FRealtimeMeshPolygonGroupRange>(),
						PolyGroups);
					
					// Drop the triangle segments as these two ways of doing things are mutually exclusive
					DepthOnlyTriangleSegments.Reset();
				}
			}
		}
		

		

		/*TSet<FRealtimeMeshStream>&& TakeStreamSet(bool bRemoveEmpty = true)
		{
			// This is only valid when the builder owns the data.
			check(StreamStorage.IsValid());
			
			TSet<FRealtimeMeshStream> NewStreams;
			// This is only valid when the builder owns the data.
			for (auto& Stream : Streams)
			{
				if (!bRemoveEmpty || Stream.Num() > 0)
				{
					NewStreams.Emplace(MoveTemp(Stream));
				}
			}
			Streams.Empty();
			return MoveTemp(NewStreams);
		}*/
		
		/*TSet<FRealtimeMeshStream> CopyStreamSet(bool bRemoveEmpty = true)
		{
			TSet<FRealtimeMeshStream> NewStreams;
			// This is only valid when the builder owns the data.
			for (const auto& Stream : Streams)
			{
				if (!bRemoveEmpty || Stream.Num() > 0)
				{
					NewStreams.Emplace(Stream);
				}
			}
			return NewStreams;
		}*/

		/*void SortTrianglesByMaterial()
		{
			if (TriangleMaterialIndices.Num() == Triangles.Num())
			{
				TArray<uint32> RemapTable;
				RemapTable.SetNumUninitialized(TriangleMaterialIndices.Num());
				for (int32 Index = 0; Index < TriangleMaterialIndices.Num(); Index++)
				{
					RemapTable[Index] = Index;
				}

				Algo::StableSortBy(RemapTable, [this](int32 Index)
				{
					return TriangleMaterialIndices.Get(Index);
				});

				TRealtimeMeshStreamBuilder<TriangleType> NewTriangles(Triangles.GetStream()->GetStreamKey());
				NewTriangles.SetNumUninitialized(Triangles.Num());
				RealtimeMeshAlgo::Reorder(&Triangles[0], &NewTriangles[0], Triangles.Num());
				Triangles = MoveTemp(NewTriangles);

				TRealtimeMeshStreamBuilder<uint16> NewTriangleMaterialIndices(TriangleMaterialIndices.GetStream()->GetStreamKey());
				NewTriangleMaterialIndices.SetNumUninitialized(Triangles.Num());
				RealtimeMeshAlgo::Reorder(&TriangleMaterialIndices[0], &NewTriangleMaterialIndices[0], Triangles.Num());
				TriangleMaterialIndices = MoveTemp(NewTriangleMaterialIndices);
			}
		}*/
		


		VertexBuilder AddVertex()
		{
			const SizeType Index = Vertices.AddZeroed();
			return VertexBuilder(*this, Index);
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

		void SetNormal(int32 VertIdx, const TangentType& Normal)
		{
			checkf(HasTangents(), TEXT("Vertex tangents not enabled"));
			Tangents->GetValue(VertIdx).SetNormal(Normal);
		}

		void SetTangents(int32 VertIdx, const TangentType& Tangent)
		{
			checkf(HasTangents(), TEXT("Vertex tangents not enabled"));
			Tangents->GetValue(VertIdx).SetNormal(Tangent);
		}

		void SetNormalAndTangent(int32 VertIdx, const FVector3f& Normal, const FVector3f& Tangent)
		{
			checkf(HasTangents(), TEXT("Vertex tangents not enabled"));
			Tangents->GetValue(VertIdx).SetNormalAndTangent(Normal, Tangent);
		}

		void SetTangents(int32 VertIdx, const FVector3f& Normal, const FVector3f& Binormal, const FVector3f& Tangent)
		{
			checkf(HasTangents(), TEXT("Vertex tangents not enabled"));
			Tangents->GetValue(VertIdx).SetTangents(Normal, Binormal, Tangent);
		}

		/*void SetTexCoord(int32 VertIdx, int32 TexCoordIdx, const TexCoordType& TexCoord)
		{
			if (FRealtimeMeshBufferTypeTraits<TexCoordStreamType>::NumElements == 1)
			{
				check(TexCoordIdx == 0);
				TexCoords.Set(VertIdx, TexCoord);					
			}
			else
			{
				TexCoords.SetElement(VertIdx, TexCoordIdx, TexCoord);				
			}
		}*/

		template <typename ArgType>
		void SetTexCoord(int32 VertIdx, int32 TexCoordIdx, const ArgType& TexCoord)
		{
			checkf(HasTexCoords(), TEXT("Vertex texcoords not enabled"));
			if constexpr (FRealtimeMeshBufferTypeTraits<TexCoordStreamType>::NumElements == 1)
			{
				check(TexCoordIdx == 0);
				TexCoords->template Set<ArgType>(VertIdx, TexCoord);
			}
			else
			{
				TexCoords->template SetElement<ArgType>(VertIdx, TexCoordIdx, TexCoord);
			}
		}

		/*template<typename... TexCoord>
		void SetTexCoords(int32 VertIdx, const TexCoord& TexCoords...)
		{
			TexCoords.Set(VertIdx, TexCoords...);
			return *this;
		}*/

		void SetColor(int32 VertIdx, FColor VertexColor)
		{
			checkf(HasVertexColors(), TEXT("Vertex colors not enabled"));
			if (Colors->Num() <= VertIdx)
			{
				Colors->AppendGenerator(VertIdx + 1 - Colors->Num(), [](int32, int32) { return FColor::White; });
			}
			Colors->Set(VertIdx, VertexColor);
		}

		void SetColor(int32 VertIdx, const FLinearColor& VertexColor)
		{
			checkf(HasVertexColors(), TEXT("Vertex colors not enabled"));
			if (Colors->Num() <= VertIdx)
			{
				Colors->AppendGenerator(VertIdx + 1 - Colors->Num(), [](int32, int32) { return FColor::White; });
			}
			Colors->Set(VertIdx, VertexColor.ToFColor(true));
		}


		int32 AddTriangle(const TriangleType& Triangle)
		{
			return Triangles.Add(Triangle);
		}
		
		int32 AddTriangle(const TriangleType& Triangle, uint16 MaterialIndex)
		{
			checkf(HasPolyGroups(), TEXT("Triangle material indices not enabled"));
			auto Result = Triangles.Add(Triangle);
			TrianglePolyGroups->Set(Result, MaterialIndex);
			return Result;
		}

		int32 AddTriangle(IndexType Vert0, IndexType Vert1, IndexType Vert2)
		{
			return Triangles.Add(TriangleType(Vert0, Vert1, Vert2));
		}

		int32 AddTriangle(IndexType Vert0, IndexType Vert1, IndexType Vert2, uint16 MaterialIndex)
		{
			checkf(HasPolyGroups(), TEXT("Triangle material indices not enabled"));
			auto Result = Triangles.Add(TriangleType(Vert0, Vert1, Vert2));
			TrianglePolyGroups->Set(Result, MaterialIndex);
			return Result;
		}

		void SetTriangle(int32 Index, const TriangleType& NewTriangle)
		{
			Triangles.Set(Index, NewTriangle);
		}

		void SetTriangle(int32 Index, const TriangleType& NewTriangle, uint16 MaterialIndex)
		{
			checkf(HasPolyGroups(), TEXT("Triangle material indices not enabled"));
			Triangles.Set(Index, NewTriangle);
			TrianglePolyGroups->Set(Index, MaterialIndex);
		}

		void SetTriangle(int32 Index, IndexType Vert0, IndexType Vert1, IndexType Vert2)
		{
			Triangles.Set(Index, TriangleType(Vert0, Vert1, Vert2));
		}

		void SetTriangle(int32 Index, IndexType Vert0, IndexType Vert1, IndexType Vert2, uint16 MaterialIndex)
		{
			checkf(HasPolyGroups(), TEXT("Triangle material indices not enabled"));
			Triangles.Set(Index, TriangleType(Vert0, Vert1, Vert2));
			TrianglePolyGroups->Set(Index, MaterialIndex);
		}

		int32 AddDepthOnlyTriangle(const TriangleType& Triangle)
		{
			checkf(HasDepthOnlyTriangles(), TEXT("Depth only triangles not enabled"));
			return DepthOnlyTriangles.Add(Triangle);
		}
		int32 AddDepthOnlyTriangle(const TriangleType& Triangle, uint16 MaterialIndex)
		{
			checkf(HasDepthOnlyTriangles(), TEXT("Depth only triangles not enabled"));
			checkf(HasPolyGroups(), TEXT("Depth only triangle material indices not enabled"));
			auto Result = DepthOnlyTriangles.Add(Triangle);
			DepthOnlyTrianglePolyGroups->Set(Result, MaterialIndex);
			return Result;
		}

		int32 AddDepthOnlyTriangle(IndexType Vert0, IndexType Vert1, IndexType Vert2)
		{
			checkf(HasDepthOnlyTriangles(), TEXT("Depth only triangles not enabled"));
			return DepthOnlyTriangles.Add(TriangleType(Vert0, Vert1, Vert2));
		}
		
		int32 AddDepthOnlyTriangle(IndexType Vert0, IndexType Vert1, IndexType Vert2, uint16 MaterialIndex)
		{
			checkf(HasDepthOnlyTriangles(), TEXT("Depth only triangles not enabled"));
			checkf(HasPolyGroups(), TEXT("Depth only triangle material indices not enabled"));
			auto Result = DepthOnlyTriangles.Add(TriangleType(Vert0, Vert1, Vert2));
			DepthOnlyTrianglePolyGroups->Set(Result, MaterialIndex);
			return Result;
		}

		void SetDepthOnlyTriangle(int32 Index, const TriangleType& NewTriangle)
		{
			checkf(HasDepthOnlyTriangles(), TEXT("Depth only triangles not enabled"));
			DepthOnlyTriangles.Set(Index, NewTriangle);
		}
		
		void SetDepthOnlyTriangle(int32 Index, const TriangleType& NewTriangle, uint16 MaterialIndex)
		{
			checkf(HasDepthOnlyTriangles(), TEXT("Depth only triangles not enabled"));
			checkf(HasPolyGroups(), TEXT("Depth only triangle material indices not enabled"));
			DepthOnlyTriangles.Set(Index, NewTriangle);
			DepthOnlyTrianglePolyGroups->Set(Index, MaterialIndex);
		}

		void SetDepthOnlyTriangle(int32 Index, IndexType Vert0, IndexType Vert1, IndexType Vert2)
		{
			checkf(HasDepthOnlyTriangles(), TEXT("Depth only triangles not enabled"));
			DepthOnlyTriangles.Set(Index, TriangleType(Vert0, Vert1, Vert2));
		}
		
		void SetDepthOnlyTriangle(int32 Index, IndexType Vert0, IndexType Vert1, IndexType Vert2, uint16 MaterialIndex)
		{
			checkf(HasDepthOnlyTriangles(), TEXT("Depth only triangles not enabled"));
			checkf(HasPolyGroups(), TEXT("Depth only triangle material indices not enabled"));
			DepthOnlyTriangles.Set(Index, TriangleType(Vert0, Vert1, Vert2));
			DepthOnlyTrianglePolyGroups->Set(Index, MaterialIndex);
		}
	};




	template <typename IndexType = uint32, typename TangentType = FPackedNormal, typename TexCoordType = FVector2DHalf, int32 NumTexCoords = 1>
	struct TRealtimeMeshVertexBuilderLocal : FNoncopyable
	{
		using TBuilder = TRealtimeMeshBuilderLocal<IndexType, TangentType, TexCoordType, NumTexCoords>;
		using TRowBuilder = TRealtimeMeshVertexBuilderLocal<IndexType, TangentType, TexCoordType, NumTexCoords>;
		using TriangleType = TIndex3<IndexType>;
		using TangentStreamType = TRealtimeMeshTangents<TangentType>;
		using TexCoordStreamType = TRealtimeMeshTexCoords<TexCoordType, NumTexCoords>;
		using SizeType = TRealtimeMeshBuilderLocal<FVector3f>::SizeType;
	private:
		TBuilder& ParentBuilder;
		const SizeType RowIndex;
	public:
		TRealtimeMeshVertexBuilderLocal(TBuilder& InBuilder, SizeType InRowIndex)
			: ParentBuilder(InBuilder), RowIndex(InRowIndex)
		{
		}
		
		int32 GetIndex() const { return RowIndex; }
		
		bool HasTangents() const { return ParentBuilder.HasTangents(); }
		bool HasTexCoords() const { return ParentBuilder.HasTexCoords(); }
		bool HasVertexColors() const { return ParentBuilder.HasVertexColors(); }
		SizeType NumTexCoordChannels() const { return ParentBuilder.NumTexCoordChannels(); }



		TRowBuilder& SetPosition(const FVector3f& Position)
		{
			ParentBuilder.SetPosition(RowIndex, Position);
			return *this;
		}

		TRowBuilder& SetNormal(const FVector3f& Normal)
		{
			ParentBuilder.SetNormal(RowIndex, Normal);
			return *this;
		}

		TRowBuilder& SetTangent(const FVector3f& Tangent)
		{
			ParentBuilder.SetTangents(RowIndex, Tangent);
			return *this;
		}

		TRowBuilder& SetNormalAndTangent(const FVector3f& Normal, const FVector3f& Tangent)
		{
			ParentBuilder.SetNormalAndTangent(RowIndex, Normal, Tangent);
			return *this;
		}

		TRowBuilder& SetTangents(const FVector3f& Normal, const FVector3f& Binormal, const FVector3f& Tangent)
		{
			ParentBuilder.SetTangents(RowIndex, Normal, Binormal, Tangent);
			return *this;
		}

		template <typename ArgType, typename T = TexCoordStreamType>
		typename TEnableIf<(FRealtimeMeshBufferTypeTraits<T>::NumElements == 1), TRowBuilder&>::Type SetTexCoord(const ArgType& TexCoord)
		{
			ParentBuilder.SetTexCoord(RowIndex, 0, TexCoord);
			return *this;
		}

		template <typename ArgType, typename T = TexCoordStreamType>
		typename TEnableIf<(FRealtimeMeshBufferTypeTraits<T>::NumElements > 1), TRowBuilder&>::Type SetTexCoord(int32 TexCoordIdx, const ArgType& TexCoord)
		{
			ParentBuilder.SetTexCoord(RowIndex, TexCoordIdx, TexCoord);
			return *this;
		}

		template <typename ArgType, typename T = TexCoordStreamType>
		typename TEnableIf<(FRealtimeMeshBufferTypeTraits<T>::NumElements == 1), TRowBuilder&>::Type SetTexCoords(const ArgType& TexCoord)
		{
			ParentBuilder.SetTexCoord(RowIndex, 0, TexCoord);
			return *this;
		}

		template <typename... ArgTypes, typename T = TexCoordStreamType>
		typename TEnableIf<(FRealtimeMeshBufferTypeTraits<T>::NumElements > 1), TRowBuilder&>::Type SetTexCoords(const ArgTypes&... TexCoords)
		{
			ParentBuilder.SetTexCoords(RowIndex, TexCoords...);
			return *this;
		}

		TRowBuilder& SetColor(FColor VertexColor)
		{
			ParentBuilder.SetColor(VertexColor);
			return *this;
		}

		TRowBuilder& SetColor(const FLinearColor& VertexColor)
		{
			ParentBuilder.SetColor(VertexColor.ToFColor(true));
			return *this;
		}
		
	};

	
}
