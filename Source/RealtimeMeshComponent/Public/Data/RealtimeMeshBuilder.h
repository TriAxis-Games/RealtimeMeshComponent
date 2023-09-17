// Copyright TriAxis Games, L.L.C. All Rights Reserved.


#pragma once

#include "RealtimeMeshDataStream.h"
#include "RealtimeMeshDataTypes.h"


// ReSharper disable CppMemberFunctionMayBeConst
namespace RealtimeMesh
{
	template <typename BufferType>
	struct TRealtimeMeshStreamRowReader : FNoncopyable
	{
	public:
		using SizeType = FRealtimeMeshDataStream::SizeType;
		using ElementType = typename FRealtimeMeshBufferTypeTraits<BufferType>::ElementType;
		static constexpr int32 NumElements = FRealtimeMeshBufferTypeTraits<BufferType>::NumElements;

	private:
		const SizeType RowIndex;
		ElementType* RowPtr;

	public:
		TRealtimeMeshStreamRowReader(SizeType InRowIndex, BufferType* InRowPtr)
			: RowIndex(InRowIndex), RowPtr(reinterpret_cast<ElementType*>(InRowPtr))
		{
		}

		operator int32() const { return RowIndex; }
		int32 GetIndex() const { return RowIndex; }

		const ElementType& Get(int32 ElementIdx) const
		{
			return RowPtr[ElementIdx];
		}

		const TRealtimeMeshStreamRowReader& Get(int32 ElementIdx, ElementType& Element) const
		{
			Element = RowPtr[ElementIdx];
			return *this;
		}
	};

	template <typename BufferType>
	struct TRealtimeMeshStreamRowBuilder : FNoncopyable
	{
	public:
		using SizeType = FRealtimeMeshDataStream::SizeType;
		using ElementType = typename FRealtimeMeshBufferTypeTraits<BufferType>::ElementType;
		static constexpr int32 NumElements = FRealtimeMeshBufferTypeTraits<BufferType>::NumElements;

	private:
		const SizeType RowIndex;
		ElementType* RowPtr;

	public:
		TRealtimeMeshStreamRowBuilder(SizeType InRowIndex, BufferType* InRowPtr)
			: RowIndex(InRowIndex), RowPtr(reinterpret_cast<ElementType*>(InRowPtr))
		{
		}

		operator int32() const { return RowIndex; }
		int32 GetIndex() const { return RowIndex; }

		ElementType& Get(int32 ElementIdx)
		{
			return RowPtr[ElementIdx];
		}

		TRealtimeMeshStreamRowBuilder& Set(int32 ElementIdx, const ElementType& Element)
		{
			SetInternal(ElementIdx, Element);
			return *this;
		}

		template <typename... ArgTypes>
		TRealtimeMeshStreamRowBuilder& SetRange(int32 StartElementIdx, ArgTypes&&... RemainingElements)
		{
			static_assert(sizeof...(ArgTypes) <= NumElements, "Too many elements passed to SetRange");
			checkf(sizeof...(ArgTypes) + StartElementIdx <= NumElements, TEXT("Too many elements passed to SetRange"));

			SetInternal(StartElementIdx, RemainingElements...);
			return *this;
		}

		template <typename... ArgTypes>
		TRealtimeMeshStreamRowBuilder& SetAll(const ArgTypes&... RemainingElements)
		{
			static_assert(sizeof...(ArgTypes) == NumElements, "Wrong number of elements passed to SetAll");
			static_assert(std::conjunction_v<std::is_assignable<ElementType, const ArgTypes&>...>, "Unable to convert all parameters to ElementType");

			SetInternal(0, RemainingElements...);
			return *this;
		}

		TRealtimeMeshStreamRowBuilder& Get(int32 ElementIdx, ElementType& Element)
		{
			Element = RowPtr[ElementIdx];
			return *this;
		}

		TRealtimeMeshStreamRowBuilder& Tie(int32 ElementIdx, ElementType*& Element)
		{
			Element = &RowPtr[ElementIdx];
			return *this;
		}

	protected:
		template <typename ArgType>
		void SetInternal(int32 ElementIdx, ArgType&& Element)
		{
			check(ElementIdx < NumElements);
			RowPtr[ElementIdx] = Element;
		}

		template <typename ArgType, typename... ArgTypes>
		void SetInternal(int32 StartElementIdx, ArgType&& FirstElement, ArgTypes&&... RemainingElements)
		{
			SetInternal<ArgType>(StartElementIdx, FirstElement);
			SetInternal(StartElementIdx + 1, Forward<ArgTypes>(RemainingElements)...);
		}
	};


	template <typename BufferType>
	struct TRealtimeMeshStreamBuilderBase
	{
	public:
		using SizeType = FRealtimeMeshDataStream::SizeType;
		using ElementType = typename FRealtimeMeshBufferTypeTraits<BufferType>::ElementType;
		using RowBuilderType = TRealtimeMeshStreamRowBuilder<BufferType>;

	protected:
		// Underlying data stream we're responsible for
		FRealtimeMeshDataStreamRef Stream;

		TArray<FRealtimeMeshDataStreamWeakPtr> LinkedStreams;

	public:
		TRealtimeMeshStreamBuilderBase(const TRealtimeMeshStreamBuilderBase&) = delete;
		TRealtimeMeshStreamBuilderBase& operator=(const TRealtimeMeshStreamBuilderBase&) = delete;
		TRealtimeMeshStreamBuilderBase(TRealtimeMeshStreamBuilderBase&&) = delete;
		TRealtimeMeshStreamBuilderBase& operator=(TRealtimeMeshStreamBuilderBase&&) = delete;

		TRealtimeMeshStreamBuilderBase(FRealtimeMeshDataStreamRef&& InStream)
			: Stream(MoveTemp(InStream))
		{
			check(Stream->GetLayout() == GetRealtimeMeshBufferLayout<BufferType>());
			check(sizeof(BufferType) == Stream->GetStride());
		}

		TRealtimeMeshStreamBuilderBase(const FRealtimeMeshStreamKey& StreamKey)
			: Stream(MakeShared<FRealtimeMeshDataStream>(StreamKey, GetRealtimeMeshBufferLayout<BufferType>()))
		{
		}

		TRealtimeMeshStreamBuilderBase& operator=(FRealtimeMeshDataStreamRef&& InStream)
		{
			check(Stream->GetLayout() == GetRealtimeMeshBufferLayout<BufferType>());
			check(sizeof(BufferType) == Stream->GetStride());
			Stream = MoveTemp(InStream);
			return *this;
		}


		FORCEINLINE FRealtimeMeshDataStreamRef GetStream() const { return Stream; }
		FORCEINLINE const FRealtimeMeshBufferLayout& GetBufferLayout() const { return Stream->GetLayout(); }

		bool HasLinkedStreams() const { return LinkedStreams.Num() > 0; }

		void AddLinkedStream(const FRealtimeMeshDataStreamRef& InStream)
		{
			LinkedStreams.AddUnique(InStream);
			InStream->SetNumZeroed(Stream->Num());
		}

		template <typename OtherBufferType>
		void AddLinkedBuilder(const TRealtimeMeshStreamBuilderBase<OtherBufferType>& Other)
		{
			AddLinkedStream(Other.GetStream());
		}

		void RemoveLinkedStream(const FRealtimeMeshDataStreamRef& InStream)
		{
			LinkedStreams.Remove(InStream);
		}

		template <typename OtherBufferType>
		void RemoveLinkedBuilder(const TRealtimeMeshStreamBuilderBase<OtherBufferType>& Other)
		{
			RemoveLinkedStream(Other.GetStream());
		}

		template <typename FunctionType>
		void DoForAllLinkedStreams(FunctionType Func)
		{
			bool bCleanupLinked = false;
			for (const auto& StreamWeak : LinkedStreams)
			{
				if (const auto StreamPinned = StreamWeak.Pin())
				{
					Func(StreamPinned);
				}
				else
				{
					bCleanupLinked = true;
				}
			}

			if (bCleanupLinked)
			{
				LinkedStreams.RemoveAll([](const FRealtimeMeshDataStreamWeakPtr& WeakPtr) { return !WeakPtr.IsValid(); });
			}
		}


		FORCEINLINE SizeType Num() const { return Stream->Num(); }

		FORCEINLINE SIZE_T GetAllocatedSize() const { return Stream->GetAllocatedSize(); }
		FORCEINLINE SizeType GetSlack() const { return Stream->GetSlack(); }
		FORCEINLINE bool IsValidIndex(int32 Index) const { return Stream->IsValidIndex(Index); }
		FORCEINLINE bool IsEmpty() const { return Stream->IsEmpty(); }
		FORCEINLINE SizeType Max() const { return Stream->Max(); }


		FORCEINLINE SizeType AddUninitialized()
		{
			const SizeType Index = Stream->AddUninitialized();
			DoForAllLinkedStreams([Index](const FRealtimeMeshDataStreamPtr& Stream)
			{
				const SizeType LinkedIndex = Stream->AddUninitialized();
				check(LinkedIndex == Index);
			});
			return Index;
		}

		FORCEINLINE SizeType AddUninitialized(SizeType Count)
		{
			const SizeType Index = Stream->AddUninitialized(Count);
			DoForAllLinkedStreams([Count, Index](const FRealtimeMeshDataStreamPtr& Stream)
			{
				const SizeType LinkedIndex = Stream->AddUninitialized(Count);
				check(LinkedIndex == Index);
			});
			return Index;
		}

		FORCEINLINE SizeType AddZeroed(SizeType Count = 1)
		{
			const SizeType Index = Stream->AddZeroed(Count);
			DoForAllLinkedStreams([Count, Index](const FRealtimeMeshDataStreamPtr& Stream)
			{
				const SizeType LinkedIndex = Stream->AddZeroed(Count);
				check(LinkedIndex == Index);
			});
			return Index;
		}

		FORCEINLINE void Shrink()
		{
			Stream->Shrink();
			DoForAllLinkedStreams([](const FRealtimeMeshDataStreamPtr& Stream)
			{
				Stream->Shrink();
			});
		}

		FORCEINLINE void Empty(SizeType ExpectedUseSize = 0, SizeType MaxSlack = 0)
		{
			Stream->Empty(ExpectedUseSize, MaxSlack);
			DoForAllLinkedStreams([ExpectedUseSize, MaxSlack](const FRealtimeMeshDataStreamPtr& Stream)
			{
				Stream->Empty(ExpectedUseSize, MaxSlack);
			});
		}

		FORCEINLINE void Reserve(SizeType Number)
		{
			Stream->Reserve(Number);
			DoForAllLinkedStreams([Number](const FRealtimeMeshDataStreamPtr& Stream)
			{
				Stream->Reserve(Number);
			});
		}

		FORCEINLINE void SetNumUninitialized(SizeType NewNum, bool bAllowShrinking = true)
		{
			Stream->SetNumUninitialized(NewNum, bAllowShrinking);
			DoForAllLinkedStreams([NewNum, bAllowShrinking](const FRealtimeMeshDataStreamPtr& Stream)
			{
				Stream->SetNumUninitialized(NewNum, bAllowShrinking);
			});
		}

		FORCEINLINE void SetNumZeroed(int32 NewNum, bool bAllowShrinking = true)
		{
			Stream->SetNumZeroed(NewNum, bAllowShrinking);
			DoForAllLinkedStreams([NewNum, bAllowShrinking](const FRealtimeMeshDataStreamPtr& Stream)
			{
				Stream->SetNumZeroed(NewNum, bAllowShrinking);
			});
		}

		FORCEINLINE void RemoveAt(SizeType Index, SizeType Count = 1, bool bAllowShrinking = true)
		{
			Stream->RemoveAt(Index, Count, bAllowShrinking);
			DoForAllLinkedStreams([Index, Count, bAllowShrinking](const FRealtimeMeshDataStreamPtr& Stream)
			{
				Stream->RemoveAt(Index, Count, bAllowShrinking);
			});
		}

		FORCEINLINE BufferType& Get(int32 Index)
		{
			RangeCheck(Index);
			return *reinterpret_cast<BufferType*>(Stream->GetData() + (sizeof(BufferType) * Index));
		}

		FORCEINLINE const BufferType& Get(int32 Index) const
		{
			RangeCheck(Index);
			return *reinterpret_cast<const BufferType*>(Stream->GetData() + (sizeof(BufferType) * Index));
		}

		TConstArrayView<BufferType, SizeType> GetView() const
		{
			return TConstArrayView<BufferType, SizeType>(Stream->GetData(), Num());
		}

		TArrayView<BufferType, SizeType> GetView()
		{
			return TArrayView<BufferType, SizeType>(Stream->GetData(), Num());
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
			BufferType* DataPtr = *Get(StartIndex);
			for (int32 Index = 0; Index < Count; Index++)
			{
				DataPtr[Index] = Generator(Index, StartIndex + Index);
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
			const SizeType StartIndex = AddUninitialized(Count);
			SetGenerator<GeneratorFunc>(StartIndex, Count, Forward<GeneratorFunc>(Generator));
		}

	protected:
		FORCEINLINE void RangeCheck(SizeType Index) const
		{
			checkf((Index >= 0) & (Index < Num()), TEXT("Array index out of bounds: %lld from an array of size %lld"), static_cast<int64>(Index),
			       static_cast<int64>(Num())); // & for one branch
		}
	};


	// Single Element specialization
	template <typename BufferType>
	struct TRealtimeMeshStreamBuilderSingleElementImpl : public TRealtimeMeshStreamBuilderBase<BufferType>
	{
		static_assert(FRealtimeMeshBufferTypeTraits<BufferType>::NumElements == 1, "TRealtimeMeshStreamBuilderSingleElementImpl can only be used with single element buffers");
		using Super = TRealtimeMeshStreamBuilderBase<BufferType>;
		using SizeType = typename Super::SizeType;

		TRealtimeMeshStreamBuilderSingleElementImpl(const TRealtimeMeshStreamBuilderSingleElementImpl&) = default;
		TRealtimeMeshStreamBuilderSingleElementImpl(TRealtimeMeshStreamBuilderSingleElementImpl&&) = default;

		TRealtimeMeshStreamBuilderSingleElementImpl(const FRealtimeMeshDataStreamRef& InStream)
			: Super(InStream)
		{
		}

		TRealtimeMeshStreamBuilderSingleElementImpl(FRealtimeMeshDataStreamRef&& InStream)
			: Super(MoveTemp(InStream))
		{
		}

		TRealtimeMeshStreamBuilderSingleElementImpl(const FRealtimeMeshStreamKey& StreamKey)
			: Super(StreamKey)
		{
		}

		TRealtimeMeshStreamBuilderSingleElementImpl& operator=(const TRealtimeMeshStreamBuilderSingleElementImpl&) = default;
		TRealtimeMeshStreamBuilderSingleElementImpl& operator=(TRealtimeMeshStreamBuilderSingleElementImpl&&) = default;

		TRealtimeMeshStreamBuilderSingleElementImpl& operator=(const FRealtimeMeshDataStreamRef& InStream)
		{
			Super::operator=(InStream);
			return *this;
		}

		TRealtimeMeshStreamBuilderSingleElementImpl& operator=(const FRealtimeMeshDataStreamRef&& InStream)
		{
			Super::operator=(MoveTemp(InStream));
			return *this;
		}

		int32 Add(const BufferType& Entry)
		{
			const SizeType Index = Super::AddUninitialized();
			Super::Get(Index) = Entry;
			return Index;
		}

		/*void Set(int32 Index, const BufferType& Entry)
		{
			Super::Get(Index) = Entry;		
		}*/

		template <typename ArgType>
		void Set(int32 Index, ArgType Entry)
		{
			Super::Get(Index) = BufferType(Entry);
		}
	};

	template <typename BufferType>
	struct TRealtimeMeshStreamBuilderMultiElementImpl : public TRealtimeMeshStreamBuilderBase<BufferType>
	{
		static_assert(FRealtimeMeshBufferTypeTraits<BufferType>::NumElements > 1, "TRealtimeMeshStreamBuilderSingleElementImpl can only be used with multi element buffers");
		using Super = TRealtimeMeshStreamBuilderBase<BufferType>;
		using SizeType = typename Super::SizeType;
		using ElementType = typename FRealtimeMeshBufferTypeTraits<BufferType>::ElementType;
		static constexpr int32 NumElements = FRealtimeMeshBufferTypeTraits<BufferType>::NumElements;

		TRealtimeMeshStreamBuilderMultiElementImpl(const TRealtimeMeshStreamBuilderMultiElementImpl&) = default;
		TRealtimeMeshStreamBuilderMultiElementImpl(TRealtimeMeshStreamBuilderMultiElementImpl&&) = default;

		TRealtimeMeshStreamBuilderMultiElementImpl(const FRealtimeMeshDataStreamRef& InStream)
			: Super(InStream)
		{
		}

		TRealtimeMeshStreamBuilderMultiElementImpl(FRealtimeMeshDataStreamRef&& InStream)
			: Super(MoveTemp(InStream))
		{
		}

		TRealtimeMeshStreamBuilderMultiElementImpl(const FRealtimeMeshStreamKey& StreamKey)
			: Super(StreamKey)
		{
		}

		TRealtimeMeshStreamBuilderMultiElementImpl& operator=(const TRealtimeMeshStreamBuilderMultiElementImpl&) = default;
		TRealtimeMeshStreamBuilderMultiElementImpl& operator=(TRealtimeMeshStreamBuilderMultiElementImpl&&) = default;

		TRealtimeMeshStreamBuilderMultiElementImpl& operator=(const FRealtimeMeshDataStreamRef& InStream)
		{
			Super::operator=(InStream);
			return *this;
		}

		TRealtimeMeshStreamBuilderMultiElementImpl& operator=(const FRealtimeMeshDataStreamRef&& InStream)
		{
			Super::operator=(MoveTemp(InStream));
			return *this;
		}


		ElementType& GetElement(int32 Index, int32 ElementIdx)
		{
			Super::RangeCheck(Index);
			ElementCheck(ElementIdx);
			return *reinterpret_cast<ElementType*>(Super::Stream->GetData() + (sizeof(BufferType) * Index + sizeof(ElementType) * ElementIdx));
		}

		const ElementType& GetElement(int32 Index, int32 ElementIdx) const
		{
			Super::RangeCheck(Index);
			ElementCheck(ElementIdx);
			return *reinterpret_cast<const ElementType*>(Super::Stream->GetData() + (sizeof(BufferType) * Index + sizeof(ElementType) * ElementIdx));
		}

		using Super::Get;

		TRealtimeMeshStreamRowBuilder<BufferType> Add()
		{
			const SizeType Index = Super::AddUninitialized();
			return TRealtimeMeshStreamRowBuilder<BufferType>(Index, &Super::Get(Index));
		}

		int32 Add(const BufferType& Entry)
		{
			const SizeType Index = Super::AddUninitialized();
			FMemory::Memcpy(&Super::Get(Index), &Entry, sizeof(BufferType));
			return Index;
		}

		void Set(int32 Index, const BufferType& Entry)
		{
			FMemory::Memcpy(&Super::Get(Index), &Entry, sizeof(BufferType));
		}


		template <typename... ArgTypes>
		int32 Add(const ArgTypes&... Elements)
		{
			//static_assert(sizeof...(ArgTypes) == NumElements, "Wrong number of elements passed to SetAll");
			//static_assert(std::conjunction_v<std::is_assignable<ElementType, const ArgTypes&>...>, "Unable to convert all parameters to ElementType");

			TRealtimeMeshStreamRowBuilder<BufferType> Writer = Add();
			Writer.SetAll(Elements...);
			return Writer;
		}

		TRealtimeMeshStreamRowBuilder<BufferType> Edit(int32 Index)
		{
			return TRealtimeMeshStreamRowBuilder<BufferType>(Index, &Get(Index));
		}

		void SetElement(int32 Index, int32 ElementIndex, const ElementType& Element)
		{
			GetElement(Index, ElementIndex) = Element;
		}

		template <typename ArgType>
		void SetElement(int32 Index, int32 ElementIndex, const ArgType& Element)
		{
			GetElement(Index, ElementIndex) = ElementType(Element);
		}

		template <typename... ArgTypes>
		void SetElements(int32 Index, const ArgTypes&... Elements)
		{
			static_assert(sizeof...(ArgTypes) <= NumElements, "Wrong number of elements passed to SetElements");
			checkf(sizeof...(ArgTypes) + Index <= NumElements, "Wrong number of elements passed to SetElements");
			static_assert(std::conjunction_v<std::is_assignable<ElementType, const ArgTypes&>...>, "Unable to convert all parameters to ElementType");

			TRealtimeMeshStreamRowBuilder<BufferType> Writer = Edit(Index);
			Writer.SetRange(0, Elements...);
		}

	protected:
		FORCEINLINE void ElementCheck(int32 ElementIndex) const
		{
			checkf((ElementIndex >= 0) & (ElementIndex < NumElements), TEXT("Element index out of bounds: %d from an element list of size %d"), ElementIndex,
			       NumElements); // & for one branch
		}
	};


	template <typename BufferType>
	using TRealtimeMeshStreamBuilder = std::conditional_t<(FRealtimeMeshBufferTypeTraits<BufferType>::NumElements > 1), TRealtimeMeshStreamBuilderMultiElementImpl<BufferType>,
	                                                      TRealtimeMeshStreamBuilderSingleElementImpl<BufferType>>;


	template <typename TriangleType = uint32, typename TangentType = FPackedNormal, typename TexCoordType = FVector2DHalf, int32 NumTexCoords = 1>
	struct TRealtimeMeshBuilderLocal;


	template <typename IndexType = uint32, typename TangentType = FPackedNormal, typename TexCoordType = FVector2DHalf, int32 NumTexCoords = 1>
	struct TRealtimeMeshRowBuilderLocal : FNoncopyable
	{
	public:
		using BuilderType = TRealtimeMeshBuilderLocal<IndexType, TangentType, TexCoordType, NumTexCoords>;
		using TriangleType = TIndex3<IndexType>;
		using TangentStreamType = TRealtimeMeshTangents<TangentType>;
		using TexCoordStreamType = TRealtimeMeshTexCoords<TexCoordType, NumTexCoords>;
		using SizeType = TRealtimeMeshStreamBuilder<FVector3f>::SizeType;

	private:
		BuilderType* Builder;
		const SizeType RowIndex;

	public:
		TRealtimeMeshRowBuilderLocal(BuilderType* InBuilder, SizeType InRowIndex)
			: Builder(InBuilder), RowIndex(InRowIndex)
		{
		}

		operator int32() const { return RowIndex; }

		int32 GetIndex() const { return RowIndex; }

		TRealtimeMeshRowBuilderLocal& SetPosition(const FVector3f& Position)
		{
			Builder->GetVerticesBuilder().Set(RowIndex, Position);
			return *this;
		}

		TRealtimeMeshRowBuilderLocal& SetNormal(const FVector3f& Normal)
		{
			Builder->GetTangentsBuilder().Get(RowIndex).SetNormal(Normal);
			return *this;
		}

		TRealtimeMeshRowBuilderLocal& SetTangent(const FVector3f& Tangent)
		{
			Builder->GetTangentsBuilder().Get(RowIndex).SetTangent(Tangent);
			return *this;
		}

		TRealtimeMeshRowBuilderLocal& SetNormalAndTangent(const FVector3f& Normal, const FVector3f& Tangent)
		{
			Builder->GetTangentsBuilder().Get(RowIndex).SetNormalAndTangent(Normal, Tangent);
			return *this;
		}

		TRealtimeMeshRowBuilderLocal& SetTangents(const FVector3f& Normal, const FVector3f& Binormal, const FVector3f& Tangent)
		{
			Builder->GetTangentsBuilder().Get(RowIndex).SetTangents(Normal, Binormal, Tangent);
			return *this;
		}

		template <typename ArgType, typename T = TexCoordStreamType>
		typename TEnableIf<(FRealtimeMeshBufferTypeTraits<T>::NumElements == 1), TRealtimeMeshRowBuilderLocal&>::Type SetTexCoord(const ArgType& TexCoord)
		{
			Builder->GetTexCoordsBuilder().Set(RowIndex, TexCoord);
			return *this;
		}

		template <typename ArgType, typename T = TexCoordStreamType>
		typename TEnableIf<(FRealtimeMeshBufferTypeTraits<T>::NumElements > 1), TRealtimeMeshRowBuilderLocal&>::Type SetTexCoord(int32 TexCoordIdx, const ArgType& TexCoord)
		{
			Builder->GetTexCoordsBuilder().SetElement(RowIndex, TexCoordIdx, TexCoord);
			return *this;
		}

		template <typename ArgType, typename T = TexCoordStreamType>
		typename TEnableIf<(FRealtimeMeshBufferTypeTraits<T>::NumElements == 1), TRealtimeMeshRowBuilderLocal&>::Type SetTexCoords(const ArgType& TexCoord)
		{
			Builder->GetTexCoordsBuilder().Set(RowIndex, TexCoord);
			return *this;
		}

		template <typename... ArgTypes, typename T = TexCoordStreamType>
		typename TEnableIf<(FRealtimeMeshBufferTypeTraits<T>::NumElements > 1), TRealtimeMeshRowBuilderLocal&>::Type SetTexCoords(const ArgTypes&... TexCoords)
		{
			Builder->GetTexCoordsBuilder().SetElements(RowIndex, TexCoords...);
			return *this;
		}

		TRealtimeMeshRowBuilderLocal& SetColor(FColor VertexColor)
		{
			check(Builder->HasVertexColors());
			Builder->GetcolorsBuilder().Set(RowIndex, VertexColor);
			return *this;
		}

		TRealtimeMeshRowBuilderLocal& SetColor(const FLinearColor& VertexColor)
		{
			check(Builder->HasVertexColors());
			Builder->GetcolorsBuilder().Set(RowIndex, VertexColor.ToFColor(true));
			return *this;
		}
	};


	template <typename IndexType, typename TangentType, typename TexCoordType, int32 NumTexCoords>
	struct TRealtimeMeshBuilderLocal
	{
		using RowBuilder = TRealtimeMeshRowBuilderLocal<IndexType, TangentType, TexCoordType, NumTexCoords>;
		using TriangleType = TIndex3<IndexType>;
		using TangentStreamType = TRealtimeMeshTangents<TangentType>;
		using TexCoordStreamType = TRealtimeMeshTexCoords<TexCoordType, NumTexCoords>;
		using SizeType = TRealtimeMeshStreamBuilder<FVector3f>::SizeType;

	private:
		TUniquePtr<FRealtimeMeshDataStreamRefSet> StreamStorage;
		FRealtimeMeshDataStreamRefSet& Streams;

		TRealtimeMeshStreamBuilder<FVector3f> Vertices;
		TRealtimeMeshStreamBuilder<TangentStreamType> Tangents;
		TRealtimeMeshStreamBuilder<TexCoordStreamType> TexCoords;
		TRealtimeMeshStreamBuilder<FColor> Colors;

		TRealtimeMeshStreamBuilder<TriangleType> Triangles;
		TRealtimeMeshStreamBuilder<TriangleType> DepthOnlyTriangles;

		bool bWantsFullVertexColors;

		template <typename StreamLayout>
		FRealtimeMeshDataStreamRef AddStream(ERealtimeMeshStreamType StreamType, FName StreamName)
		{
			auto NewStream = MakeShared<FRealtimeMeshDataStream>(StreamType, StreamName, GetRealtimeMeshBufferLayout<StreamLayout>());
			Streams.Add(NewStream);
			return NewStream;
		}

		template <typename StreamLayout>
		FRealtimeMeshDataStreamRef GetStream(ERealtimeMeshStreamType StreamType, FName StreamName)
		{
			return *Streams.Find(FRealtimeMeshStreamKey(StreamType, StreamName));
		}

	public:
		TRealtimeMeshBuilderLocal(bool bInWantsFullVertexColors = false)
			: StreamStorage(MakeUnique<FRealtimeMeshDataStreamRefSet>())
			  , Streams(*StreamStorage)
			  , Vertices(AddStream<FVector3f>(ERealtimeMeshStreamType::Vertex, FRealtimeMeshStreams::PositionStreamName))
			  , Tangents(AddStream<TangentStreamType>(ERealtimeMeshStreamType::Vertex, FRealtimeMeshStreams::TangentsStreamName))
			  , TexCoords(AddStream<TexCoordStreamType>(ERealtimeMeshStreamType::Vertex, FRealtimeMeshStreams::TexCoordsStreamName))
			  , Colors(AddStream<FColor>(ERealtimeMeshStreamType::Vertex, FRealtimeMeshStreams::ColorStreamName))
			  , Triangles(AddStream<TriangleType>(ERealtimeMeshStreamType::Index, FRealtimeMeshStreams::TrianglesStreamName))
			  , DepthOnlyTriangles(AddStream<TriangleType>(ERealtimeMeshStreamType::Index, FRealtimeMeshStreams::DepthOnlyTrianglesStreamName))
			  , bWantsFullVertexColors(bInWantsFullVertexColors)
		{
			Vertices.AddLinkedBuilder(Tangents);
			Vertices.AddLinkedBuilder(TexCoords);
			if (bWantsFullVertexColors)
			{
				Vertices.AddLinkedBuilder(Colors);
			}
			else
			{
				Colors.SetNumZeroed(1);
			}
		}

		TRealtimeMeshBuilderLocal(FRealtimeMeshDataStreamRefSet& InExistingStreams)
			: Streams(InExistingStreams)
			  , Vertices(GetStream<FVector3f>(ERealtimeMeshStreamType::Vertex, FRealtimeMeshStreams::PositionStreamName))
			  , Tangents(GetStream<TangentStreamType>(ERealtimeMeshStreamType::Vertex, FRealtimeMeshStreams::TangentsStreamName))
			  , TexCoords(GetStream<TexCoordStreamType>(ERealtimeMeshStreamType::Vertex, FRealtimeMeshStreams::TexCoordsStreamName))
			  , Colors(GetStream<FColor>(ERealtimeMeshStreamType::Vertex, FRealtimeMeshStreams::ColorStreamName))
			  , Triangles(GetStream<TriangleType>(ERealtimeMeshStreamType::Index, FRealtimeMeshStreams::TrianglesStreamName))
			  , DepthOnlyTriangles(GetStream<TriangleType>(ERealtimeMeshStreamType::Index, FRealtimeMeshStreams::DepthOnlyTrianglesStreamName))
			  , bWantsFullVertexColors(Colors.Num() > 1)
		{
			// TODO: Should we auto convert incoming streams to this layout?

			Tangents.SetNumZeroed(Vertices.Num());
			Vertices.AddLinkedBuilder(Tangents);
			
			TexCoords.SetNumZeroed(Vertices.Num());
			Vertices.AddLinkedBuilder(TexCoords);
			
			if (bWantsFullVertexColors)
			{
				Colors.SetNumZeroed(Vertices.Num());
				Vertices.AddLinkedBuilder(Colors);
			}
			else
			{
				Colors.SetNumZeroed(1);
			}
		}

		FRealtimeMeshDataStreamRefSet&& TakeStreamSet()
		{
			// This is only valid when the builder owns the data.
			check(StreamStorage.IsValid());
			return MoveTemp(*StreamStorage.Get());
		}
		
		FRealtimeMeshDataStreamRefSet CopyStreamSet()
		{
			FRealtimeMeshDataStreamRefSet NewStreams;
			// This is only valid when the builder owns the data.
			for (const auto& Stream : Streams)
			{
				NewStreams.Add(MakeShared<FRealtimeMeshDataStream>(*Stream));	
			}
			return NewStreams;
		}


		TRealtimeMeshStreamBuilder<FVector3f>& GetVerticesBuilder()
		{
			return Vertices;
		}

		TRealtimeMeshStreamBuilder<TangentStreamType>& GetTangentsBuilder()
		{
			return Tangents;
		}

		TRealtimeMeshStreamBuilder<TexCoordStreamType>& GetTexCoordsBuilder()
		{
			return TexCoords;
		}

		TRealtimeMeshStreamBuilder<FColor>& GetColorsBuilder()
		{
			return Colors;
		}

		TRealtimeMeshStreamBuilder<TriangleType>& GetTrianglesBuilder()
		{
			return Triangles;
		}

		TRealtimeMeshStreamBuilder<TriangleType> GetDepthOnlyTrianglesBuilder()
		{
			return DepthOnlyTriangles;
		}

		RowBuilder AddVertex()
		{
			const SizeType Index = Vertices.AddZeroed();
			return RowBuilder(this, Index);
		}

		RowBuilder AddVertex(const FVector3f& InPosition)
		{
			const SizeType Index = Vertices.Add(InPosition);
			return RowBuilder(this, Index);
		}

		RowBuilder EditVertex(int32 VertIdx)
		{
			return RowBuilder(this, VertIdx);
		}


		void SetPosition(int32 VertIdx, const FVector3f& InPosition)
		{
			Vertices.Set(VertIdx, InPosition);
		}

		void SetNormal(int32 VertIdx, const TangentType& Normal)
		{
			Tangents.Get(VertIdx).SetNormal(Normal);
		}

		void SetTangents(int32 VertIdx, const TangentType& Tangent)
		{
			Tangents.Get(VertIdx).SetNormal(Tangent);
		}

		void SetNormalAndTangent(int32 VertIdx, const FVector3f& Normal, const FVector3f& Tangent)
		{
			Tangents.Get(VertIdx).SetNormalAndTangent(Normal, Tangent);
		}

		void SetTangents(int32 VertIdx, const FVector3f& Normal, const FVector3f& Binormal, const FVector3f& Tangent)
		{
			Tangents.Get(VertIdx).SetTangents(Normal, Binormal, Tangent);
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
			if constexpr (FRealtimeMeshBufferTypeTraits<TexCoordStreamType>::NumElements == 1)
			{
				check(TexCoordIdx == 0);
				TexCoords.template Set<ArgType>(VertIdx, TexCoord);
			}
			else
			{
				TexCoords.template SetElement<ArgType>(VertIdx, TexCoordIdx, TexCoord);
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
			check(bWantsFullVertexColors || VertIdx == 0);
			Colors.Set(VertIdx, VertexColor);
		}

		void SetColor(int32 VertIdx, const FLinearColor& VertexColor)
		{
			check(bWantsFullVertexColors || VertIdx == 0);
			Colors.Set(VertIdx, VertexColor.ToFColor(true));
		}


		int32 AddTriangle(const TriangleType& Triangle)
		{
			return Triangles.Add(Triangle);
		}

		int32 AddTriangle(IndexType Vert0, IndexType Vert1, IndexType Vert2)
		{
			return Triangles.Add(TriangleType(Vert0, Vert1, Vert2));
		}

		void SetTriangle(int32 Index, const TriangleType& NewTriangle)
		{
			Triangles.Set(Index, NewTriangle);
		}

		void SetTriangle(int32 Index, IndexType Vert0, IndexType Vert1, IndexType Vert2)
		{
			Triangles.Set(Index, TriangleType(Vert0, Vert1, Vert2));
		}

		int32 AddDepthOnlyTriangle(const TriangleType& Triangle)
		{
			return DepthOnlyTriangles.Add(Triangle);
		}

		int32 AddDepthOnlyTriangle(IndexType Vert0, IndexType Vert1, IndexType Vert2)
		{
			return DepthOnlyTriangles.Add(TriangleType(Vert0, Vert1, Vert2));
		}

		void SetDepthOnlyTriangle(int32 Index, const TriangleType& NewTriangle)
		{
			DepthOnlyTriangles.Set(Index, NewTriangle);
		}

		void SetDepthOnlyTriangle(int32 Index, IndexType Vert0, IndexType Vert1, IndexType Vert2)
		{
			DepthOnlyTriangles.Set(Index, TriangleType(Vert0, Vert1, Vert2));
		}
	};

}
