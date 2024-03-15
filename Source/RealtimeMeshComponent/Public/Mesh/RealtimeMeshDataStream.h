// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "RealtimeMeshComponentModule.h"
#include "RealtimeMeshConfig.h"
#include "RealtimeMeshDataTypes.h"
#include "RealtimeMeshDataConversion.h"
#include "Containers/StridedView.h"

#if RMC_ENGINE_BELOW_5_1
// Included for TMakeUnsigned in 5.0
#include "Containers/RingBuffer.h"
#endif

// included primarily for NatVis helpers
#include <string>

namespace RealtimeMesh
{
	struct FRealtimeMeshStreams
	{
		inline static const FName PositionStreamName = FName(TEXT("Position"));
		inline static const FName TangentsStreamName = FName(TEXT("Tangents"));
		inline static const FName TexCoordsStreamName = FName(TEXT("TexCoords"));
		inline static const FName ColorStreamName = FName(TEXT("Color"));

		inline static const FName TrianglesStreamName = FName(TEXT("Triangles"));
		inline static const FName DepthOnlyTrianglesStreamName = FName(TEXT("DepthOnlyTriangles"));
		inline static const FName ReversedTrianglesStreamName = FName(TEXT("ReversedTriangles"));
		inline static const FName ReversedDepthOnlyTrianglesStreamName = FName(TEXT("ReversedDepthOnlyTriangles"));

		inline static const FName PolyGroupStreamName = FName(TEXT("PolyGroups"));
		inline static const FName DepthOnlyPolyGroupStreamName = FName(TEXT("DepthOnlyPolyGroups"));
		
		inline static const FName PolyGroupSegmentsStreamName = FName(TEXT("PolyGroupSegments"));
		inline static const FName DepthOnlyPolyGroupSegmentsStreamName = FName(TEXT("DepthOnlyPolyGroupSegments"));
		
		inline static const FRealtimeMeshStreamKey Position = FRealtimeMeshStreamKey(ERealtimeMeshStreamType::Vertex, PositionStreamName);
		inline static const FRealtimeMeshStreamKey Tangents = FRealtimeMeshStreamKey(ERealtimeMeshStreamType::Vertex, TangentsStreamName);
		inline static const FRealtimeMeshStreamKey TexCoords = FRealtimeMeshStreamKey(ERealtimeMeshStreamType::Vertex, TexCoordsStreamName);
		inline static const FRealtimeMeshStreamKey Color = FRealtimeMeshStreamKey(ERealtimeMeshStreamType::Vertex, ColorStreamName);

		inline static const FRealtimeMeshStreamKey Triangles = FRealtimeMeshStreamKey(ERealtimeMeshStreamType::Index, TrianglesStreamName);
		inline static const FRealtimeMeshStreamKey DepthOnlyTriangles = FRealtimeMeshStreamKey(ERealtimeMeshStreamType::Index, DepthOnlyTrianglesStreamName);
		inline static const FRealtimeMeshStreamKey ReversedTriangles = FRealtimeMeshStreamKey(ERealtimeMeshStreamType::Index, ReversedTrianglesStreamName);
		inline static const FRealtimeMeshStreamKey ReversedDepthOnlyTriangles = FRealtimeMeshStreamKey(ERealtimeMeshStreamType::Index, ReversedDepthOnlyTrianglesStreamName);
		
		inline static const FRealtimeMeshStreamKey PolyGroups = FRealtimeMeshStreamKey(ERealtimeMeshStreamType::Index, PolyGroupStreamName);
		inline static const FRealtimeMeshStreamKey DepthOnlyPolyGroups = FRealtimeMeshStreamKey(ERealtimeMeshStreamType::Index, DepthOnlyPolyGroupStreamName);
		
		inline static const FRealtimeMeshStreamKey PolyGroupSegments = FRealtimeMeshStreamKey(ERealtimeMeshStreamType::Index, PolyGroupSegmentsStreamName);
		inline static const FRealtimeMeshStreamKey DepthOnlyPolyGroupSegments = FRealtimeMeshStreamKey(ERealtimeMeshStreamType::Index, DepthOnlyPolyGroupSegmentsStreamName);
	};
	
	struct FRealtimeMeshStream;

	struct FRealtimeMeshStreamDefaultRowValue
	{
	private:
		const FRealtimeMeshBufferLayout Layout;
		const TArray<uint8, TInlineAllocator<64>> Data;

	public:
		FRealtimeMeshStreamDefaultRowValue() : Layout(FRealtimeMeshBufferLayout::Invalid) { }
		FRealtimeMeshStreamDefaultRowValue(const FRealtimeMeshBufferLayout& InLayout, const TArray<uint8, TInlineAllocator<64>>& InData)
			: Layout(InLayout)
			, Data(InData)
		{
		}

		template<typename ValueType>
		static FRealtimeMeshStreamDefaultRowValue Create(const ValueType& Value, const FRealtimeMeshBufferLayout& FinalLayout = FRealtimeMeshBufferLayout::Invalid)
		{
			const auto SourceLayout = GetRealtimeMeshBufferLayout<ValueType>();
			const FRealtimeMeshBufferMemoryLayout SourceMemoryLayout = FRealtimeMeshBufferLayoutUtilities::GetBufferLayoutMemoryLayout(SourceLayout);

			// If the final layout is the current one, just return the new value
			if (FinalLayout == FRealtimeMeshBufferLayout::Invalid || FinalLayout == SourceLayout)
			{
				return FRealtimeMeshStreamDefaultRowValue(SourceLayout, TArray<uint8, TInlineAllocator<64>>{reinterpret_cast<const uint8*>(&Value), SourceMemoryLayout.GetStride()});
			}
			
			const FRealtimeMeshElementType FromType = SourceLayout.GetElementType();
			const FRealtimeMeshElementType ToType = FinalLayout.GetElementType();
			const bool bSameNumElements = FinalLayout.GetNumElements() == SourceLayout.GetNumElements();

			const FRealtimeMeshBufferMemoryLayout FinalMemoryLayout = FRealtimeMeshBufferLayoutUtilities::GetBufferLayoutMemoryLayout(FinalLayout);

			// If this isn't a convertible type, it's invalid to append
			check(FRealtimeMeshTypeConversionUtilities::CanConvert(FromType, ToType) && bSameNumElements);
						
			const auto& Converter = FRealtimeMeshTypeConversionUtilities::GetTypeConverter(FromType, ToType);

			TArray<uint8, TInlineAllocator<64>> ConvertedData;
			ConvertedData.SetNumUninitialized(FinalMemoryLayout.GetStride());

			// Convert the first element, but then we can blind copy that converted value for the rest to reduce the overhead of the conversion
			if (SourceLayout.GetNumElements() > 1)
			{
				// Multi element streams, we use a contiguous array conversion per row to convert all the elements
				const int32 NumElements = SourceLayout.GetNumElements();

				Converter.ConvertContiguousArray(&Value, ConvertedData.GetData(), NumElements);
			}
			else // Single element stream, we don't need the added complexity of multi element conversion per row
			{
				Converter.ConvertSingleElement(&Value, ConvertedData.GetData());
			}

			return FRealtimeMeshStreamDefaultRowValue(FinalLayout, ConvertedData);
		}

		bool HasData() const { return Layout.IsValid() && Data.Num() > 0; }
		const FRealtimeMeshBufferLayout& GetLayout() const { return Layout; }
		const TArray<uint8, TInlineAllocator<64>>& GetData() const { return Data; }
		const uint8* GetDataPtr() const { return Data.GetData(); }
	};
	

	struct REALTIMEMESHCOMPONENT_API FRealtimeMeshStreamLinkage
	{
	public:
		struct FStreamLinkageInfo
		{
			FRealtimeMeshStream* const Stream;
			const FRealtimeMeshStreamDefaultRowValue DefaultRowValue;

			FStreamLinkageInfo(FRealtimeMeshStream* InStream, const FRealtimeMeshStreamDefaultRowValue& InDefaultRowValue)
				: Stream(InStream)
				, DefaultRowValue(InDefaultRowValue)
			{
				check(Stream);
			}
		};
	private:		
		TArray<FStreamLinkageInfo> LinkedStreams;

		void HandleStreamRemoved(FRealtimeMeshStream* Stream);
		void HandleAllocatedSizeChanged(FRealtimeMeshStream* Stream, int32 NewSize);
		void HandleNumChanged(FRealtimeMeshStream* Stream, int32 NewNum);

		void MatchSizesOnBind();

		void CheckStreams();

		friend struct FRealtimeMeshStream;
	public:
		FRealtimeMeshStreamLinkage() = default;
		FRealtimeMeshStreamLinkage(FRealtimeMeshStreamLinkage&&) = delete;
		FRealtimeMeshStreamLinkage(const FRealtimeMeshStreamLinkage&) = delete;
		FRealtimeMeshStreamLinkage& operator=(FRealtimeMeshStreamLinkage&&) = delete;
		FRealtimeMeshStreamLinkage& operator=(const FRealtimeMeshStreamLinkage&) = delete;
		~FRealtimeMeshStreamLinkage();

		int32 NumStreams() const { return LinkedStreams.Num(); }

		bool ContainsStream(const FRealtimeMeshStream* Stream) const;
		bool ContainsStream(const FRealtimeMeshStream& Stream) const;
		
		void BindStream(FRealtimeMeshStream* Stream, const FRealtimeMeshStreamDefaultRowValue& DefaultValue);
		void BindStream(FRealtimeMeshStream& Stream, const FRealtimeMeshStreamDefaultRowValue& DefaultValue);

		void RemoveStream(const FRealtimeMeshStream* Stream);
		void RemoveStream(const FRealtimeMeshStream& Stream);

		void ForEachStream(const TFunctionRef<void(FRealtimeMeshStream&, const FRealtimeMeshStreamDefaultRowValue&)>& Func)
		{
			for (auto It = LinkedStreams.CreateConstIterator(); It; ++It)
			{
				check(It->Stream);
				Func(*It->Stream, It->DefaultRowValue);
			}
		}
	};
	
	
	struct REALTIMEMESHCOMPONENT_API FRealtimeMeshStream : FResourceArrayInterface
	{
		using AllocatorType = TSizedHeapAllocator<32>;
		using SizeType = AllocatorType::SizeType;

	private:
		using ElementAllocatorType = AllocatorType::ForAnyElementType;
		using USizeType = TMakeUnsigned<SizeType>::Type;

		FRealtimeMeshBufferLayout Layout;

		ElementAllocatorType Allocator;
		SizeType ArrayNum;
		SizeType ArrayMax;
		FRealtimeMeshStreamLinkage* Linkage;
		
		FRealtimeMeshStreamKey StreamKey;

		uint8 Stride;
		uint8 ElementStride;
		uint8 Alignment;

		FORCEINLINE void CacheStrides()
		{
			ElementStride = FRealtimeMeshBufferLayoutUtilities::GetElementStride(Layout.GetElementType());
			Alignment = FRealtimeMeshBufferLayoutUtilities::GetElementAlignment(Layout.GetElementType());
			Stride = ElementStride * Layout.GetNumElements();
		}
	public:
		FRealtimeMeshStream()
			: Layout(FRealtimeMeshBufferLayout::Invalid)
			, ArrayNum(0)
			, ArrayMax(Allocator.GetInitialCapacity())
			, Linkage(nullptr)
			, StreamKey(ERealtimeMeshStreamType::Unknown, NAME_None)
		{
			CacheStrides();
		}
		
		FRealtimeMeshStream(const FRealtimeMeshStreamKey& InStreamKey, const FRealtimeMeshBufferLayout& InLayout)
			: Layout(InLayout)
			, ArrayNum(0)
			, ArrayMax(Allocator.GetInitialCapacity())
			, Linkage(nullptr)
			, StreamKey(InStreamKey)
		{
			CacheStrides();
		}

		explicit FRealtimeMeshStream(const FRealtimeMeshStream& Other) noexcept
			: Layout(Other.Layout)
			, ArrayNum(0)
			, ArrayMax(Allocator.GetInitialCapacity())
			, Linkage(nullptr)
			, StreamKey(Other.StreamKey)
		{
			CacheStrides();
			
			ResizeAllocation(Other.Num());
			ArrayNum = Other.Num();
			FMemory::Memcpy(Allocator.GetAllocation(), Other.Allocator.GetAllocation(), Other.Num() * GetStride());
		}
		
		explicit FRealtimeMeshStream(FRealtimeMeshStream&& Other) noexcept
			: Layout(Other.Layout)
			, ArrayNum(Other.ArrayNum)
			, ArrayMax(Other.ArrayMax)
			, Linkage(nullptr)
			, StreamKey(Other.StreamKey)
		{
			CacheStrides();
			
			Allocator.MoveToEmpty(Other.Allocator);

			Other.Layout = FRealtimeMeshBufferLayout::Invalid;
			Other.ArrayNum = 0;
			Other.ArrayMax = 0;
			Other.StreamKey = FRealtimeMeshStreamKey(ERealtimeMeshStreamType::Unknown, NAME_None);

			Other.UnLink();
		}

		virtual ~FRealtimeMeshStream() override
		{
			UnLink();
		}

		static FRealtimeMeshStream Create(const FRealtimeMeshStreamKey& InStreamKey, const FRealtimeMeshBufferLayout& Layout)
		{
			return FRealtimeMeshStream(InStreamKey, Layout);
		}
		
		template<typename StreamType>
		static FRealtimeMeshStream Create(const FRealtimeMeshStreamKey& InStreamKey)
		{
			return FRealtimeMeshStream(InStreamKey, GetRealtimeMeshBufferLayout<StreamType>());
		}

		FRealtimeMeshStream& operator=(const FRealtimeMeshStream& Other)
		{
			Layout = Other.Layout;	
			CacheStrides();
			UnLink();
			ResizeAllocation(Other.Num(), false);			
			ArrayNum = Other.Num();
			BroadcastNumChanged();
			FMemory::Memcpy(Allocator.GetAllocation(), Other.Allocator.GetAllocation(), Other.Num() * GetStride());
			return *this;
		}

		FRealtimeMeshStream& operator=(FRealtimeMeshStream&& Other) noexcept
		{
			Layout = MoveTemp(Other.Layout);		
			CacheStrides();
			UnLink();
			ArrayNum = Other.ArrayNum;
			Other.ArrayNum = 0;
			Other.UnLink();
			BroadcastNumChanged();
			Allocator.MoveToEmpty(Other.Allocator);
			return *this;
		}


	public:
		/**
		 * @brief Get the name of this RealtimeMeshStream.
		 *
		 * @return The name of this RealtimeMeshStream.
		 */
		FName GetName() const { return StreamKey.GetName(); }
		
		/**
		 * @brief Get the data type of this RealtimeMeshStream.
		 *
		 * @return The data type of this RealtimeMeshStream.
		 */
		ERealtimeMeshStreamType GetStreamType() const { return StreamKey.GetStreamType(); }
		
		/**
		 * @brief Returns the stream key associated with the RealtimeMeshStream.
		 *
		 * @return The stream key associated with the RealtimeMeshStream.
		 */
		const FRealtimeMeshStreamKey& GetStreamKey() const { return StreamKey; }
		
		/**
		 * @brief Returns the layout of this RealtimeMeshStream.
		 *
		 * @return The layout of this RealtimeMeshStream.
		 */
		const FRealtimeMeshBufferLayout& GetLayout() const { return Layout; }

		/**
		 * @brief Get the element type of this RealtimeMeshStream.
		 *
		 * @details This method returns the element type of the RealtimeMeshStream.
		 * The element type determines the format of the data stored in the stream.
		 *
		 * @return The element type of the RealtimeMeshStream.
		 */
		const FRealtimeMeshElementType& GetElementType() const { return Layout.GetElementType(); }


		/**
		 * @brief Set the stream key associated with the RealtimeMeshStream.
		 *
		 * @param InStreamKey The stream key to set.
		 */
		void SetStreamKey(const FRealtimeMeshStreamKey& InStreamKey) { StreamKey = InStreamKey; }

		/**
		 * @brief Get the stride, or number of bytes for each row, of the RealtimeMeshStream.
		 *
		 * @return The stride of the RealtimeMeshStream.
		 */
		int32 GetStride() const { return Stride; }
		
		/**
		 * @brief Get the element stride, or the number of bytes for a single element within a row, of the RealtimeMeshStream.
		 *
		 * @return The element stride of the RealtimeMeshStream.
		 */
		int32 GetElementStride() const { return ElementStride; }
		
		/**
		 * @brief Get the number of elements in the RealtimeMeshStream.
		 *
		 * @return The number of elements in the RealtimeMeshStream.
		 */
		int32 GetNumElements() const { return Layout.GetNumElements(); }

		bool IsOfType(const FRealtimeMeshBufferLayout& NewLayout) const
		{
			return GetLayout() == NewLayout;
		}
		
		template<typename NewDataType>
		bool IsOfType() const
		{
			return IsOfType(GetRealtimeMeshBufferLayout<NewDataType>());
		}

		bool CanConvertTo(const FRealtimeMeshBufferLayout& NewLayout) const
		{
			const FRealtimeMeshElementType FromType = GetLayout().GetElementType();
			const FRealtimeMeshElementType ToType = NewLayout.GetElementType();
			const bool bSameNumElements = GetLayout().GetNumElements() == NewLayout.GetNumElements();

			return bSameNumElements && (FromType == ToType || FRealtimeMeshTypeConversionUtilities::CanConvert(FromType, ToType));
		}
		
		template<typename NewDataType>
		bool CanConvertTo() const
		{
			return CanConvertTo(GetRealtimeMeshBufferLayout<NewDataType>());
		}

		bool ConvertTo(const FRealtimeMeshBufferLayout& NewLayout)
		{
			if (Layout == NewLayout)
			{
				// Already in this format
				return true;
			}

			const FRealtimeMeshElementType FromType = GetLayout().GetElementType();
			const FRealtimeMeshElementType ToType = NewLayout.GetElementType();
			const bool bSameNumElements = GetLayout().GetNumElements() == NewLayout.GetNumElements();

			if (Num() == 0)
			{
				// Empty stream can just change types
				Layout = NewLayout;
				CacheStrides();
				return true;
			}
			
			if (FRealtimeMeshTypeConversionUtilities::CanConvert(FromType, ToType) && bSameNumElements)
			{
				const auto& Converter = FRealtimeMeshTypeConversionUtilities::GetTypeConverter(FromType, ToType);

				// Move existing data to temp allocator
				ElementAllocatorType OldData;				
				OldData.MoveToEmpty(Allocator);

				// Resize allocator to correct size for new data type
				Allocator.ResizeAllocation(0, ArrayMax, GetStride());

				// Now convert data from the temp array into the new allocation
				const SIZE_T ElementCount = ArrayNum * GetNumElements();
				Converter.ConvertContiguousArray(OldData.GetAllocation(), Allocator.GetAllocation(), ElementCount);
				Layout = NewLayout;
				CacheStrides();
				return true;
			}

			return false;
		}
		
		template<typename NewDataType>
		bool ConvertTo()
		{
			return ConvertTo(GetRealtimeMeshBufferLayout<NewDataType>());
		}
		
		void CopyToArray(TArray<uint8>& OutData) const
		{
			OutData.SetNumUninitialized(GetStride() * Num());
			FMemory::Memcpy(OutData.GetData(), GetData(), OutData.Num());
		}

		template <typename DataType>
		TArrayView<DataType> GetArrayView()
		{
			check(sizeof(DataType) == GetStride());
			check(GetRealtimeMeshBufferLayout<DataType>() == GetLayout());

			return MakeArrayView(reinterpret_cast<DataType*>(GetData()), Num());
		}

		template <typename DataType>
		TConstArrayView<const DataType> GetArrayView() const
		{
			check(sizeof(DataType) == GetStride());
			check(GetRealtimeMeshBufferLayout<DataType>() == GetLayout());

			return MakeArrayView(reinterpret_cast<const DataType*>(GetData()), Num());
		}


		template <typename DataType>
		TStridedView<DataType> GetElementArrayView(int32 ElementIndex)
		{
			check(sizeof(DataType) == GetElementStride());
			check(GetRealtimeMeshDataElementType<DataType>() == GetLayout().GetElementType());

			return MakeStridedView(GetStride(), reinterpret_cast<DataType*>(GetDataRawAtVertex(0, ElementIndex)), Num());
		}

		template <typename DataType>
		TStridedView<const DataType> GetElementArrayView(int32 ElementIndex) const
		{
			check(sizeof(DataType) == GetElementStride());
			check(GetRealtimeMeshDataElementType<DataType>() == GetLayout().GetElementType());

			return MakeStridedView(GetStride(), reinterpret_cast<DataType*>(GetDataRawAtVertex(0, ElementIndex)), Num());
		}
				
		template <typename DataType>
		TArrayView<const DataType> GetElementArrayView()
		{
			check(sizeof(DataType) == GetElementStride());
			check(GetRealtimeMeshDataElementType<DataType>() == GetLayout().GetElementType());

			return MakeArrayView(reinterpret_cast<const DataType*>(GetData()), Num() * GetNumElements());
		}
		
		template <typename DataType>
		TConstArrayView<const DataType> GetElementArrayView() const
		{
			check(sizeof(DataType) == GetElementStride());
			check(GetRealtimeMeshDataElementType<DataType>() == GetLayout().GetElementType());

			return MakeArrayView(reinterpret_cast<const DataType*>(GetData()), Num() * GetNumElements());
		}

		virtual const void* GetResourceData() const override { return Allocator.GetAllocation(); }
		virtual uint32 GetResourceDataSize() const override { return Num() * GetStride(); }
		virtual void Discard() override	{ }
		virtual bool IsStatic() const override { return false; }
		virtual bool GetAllowCPUAccess() const override { return false; }
		virtual void SetAllowCPUAccess(bool bInNeedsCPUAccess) override { }

		const uint8* GetData() const { return reinterpret_cast<const uint8*>(Allocator.GetAllocation()); }
		uint8* GetData() { return reinterpret_cast<uint8*>(Allocator.GetAllocation()); }


		const uint8* GetDataRawAtVertex(int32 VertexIndex) const
		{
			RangeCheck(VertexIndex);
			return GetData() + (GetStride() * VertexIndex);
		}

		uint8* GetDataRawAtVertex(int32 VertexIndex)
		{
			RangeCheck(VertexIndex);
			return GetData() + (GetStride() * VertexIndex);
		}

		const uint8* GetDataRawAtVertex(int32 VertexIndex, int32 ElementIndex) const
		{
			RangeCheck(VertexIndex);
			ElementCheck(ElementIndex);
			return GetData() + (GetStride() * VertexIndex + GetElementStride() * ElementIndex);
		}

		uint8* GetDataRawAtVertex(int32 VertexIndex, int32 ElementIndex)
		{
			RangeCheck(VertexIndex);
			ElementCheck(ElementIndex);
			return GetData() + (GetStride() * VertexIndex + GetElementStride() * ElementIndex);
		}

		template <typename ElementType>
		const ElementType* GetData() const { return reinterpret_cast<const ElementType*>(Allocator.GetAllocation()); }

		template <typename ElementType>
		ElementType* GetData() { return reinterpret_cast<ElementType*>(Allocator.GetAllocation()); }


		template <typename ElementType>
		const ElementType* GetDataAtVertex(int32 VertexIndex) const
		{
			return reinterpret_cast<const ElementType*>(GetDataRawAtVertex(VertexIndex));
		}

		template <typename ElementType>
		ElementType* GetDataAtVertex(int32 VertexIndex)
		{
			return reinterpret_cast<ElementType*>(GetDataRawAtVertex(VertexIndex));
		}

		template <typename ElementType>
		const ElementType* GetDataAtVertex(int32 VertexIndex, int32 ElementIndex) const
		{
			return reinterpret_cast<const ElementType*>(GetDataRawAtVertex(VertexIndex, ElementIndex));
		}

		template <typename ElementType>
		ElementType* GetDataAtVertex(int32 VertexIndex, int32 ElementIndex)
		{
			return reinterpret_cast<ElementType*>(GetDataRawAtVertex(VertexIndex, ElementIndex));
		}


		SizeType Num() const { return ArrayNum; }

		FORCEINLINE SIZE_T GetAllocatedSize() const { return Allocator.GetAllocatedSize(ArrayMax, GetStride()); }
		FORCEINLINE SizeType GetSlack() const
		{
			return ArrayMax - ArrayNum;
		}

		FORCEINLINE bool IsValidIndex(int32 Index) const { return Index >= 0 && Index < ArrayNum; }
		FORCEINLINE bool IsEmpty() const { return ArrayNum == 0; }
		FORCEINLINE SizeType Max() const { return ArrayMax; }

		FORCEINLINE SizeType AddUninitialized()
		{
			CheckInvariants();

			const USizeType OldNum = static_cast<USizeType>(ArrayNum);
			const USizeType NewNum = OldNum + static_cast<USizeType>(1);
			if (NewNum > static_cast<USizeType>(ArrayMax))
			{
				ResizeAllocationGrow(NewNum);
			}
			ArrayNum = static_cast<SizeType>(NewNum);			
			BroadcastNumChanged();
			return OldNum;
		}

		FORCEINLINE SizeType AddUninitialized(SizeType Count)
		{
			CheckInvariants();
			checkSlow(Count >= 0);

			const USizeType OldNum = static_cast<USizeType>(ArrayNum);
			const USizeType NewNum = OldNum + static_cast<USizeType>(Count);

			// SECURITY - This check will guard against negative counts too, in case the checkSlow(Count >= 0) above is compiled out.
			// However, it results in slightly worse code generation.
			if (static_cast<USizeType>(Count) > static_cast<USizeType>(ArrayMax) - OldNum)
			{
				ResizeAllocationGrow(NewNum);
			}
			ArrayNum = static_cast<SizeType>(NewNum);
			BroadcastNumChanged();
			return OldNum;
		}

		FORCEINLINE SizeType AddZeroed(SizeType Count = 1)
		{
			CheckInvariants();

			const SizeType Index = AddUninitialized(Count);
			FMemory::Memzero(reinterpret_cast<uint8*>(Allocator.GetAllocation()) + Index * GetStride(), Count * GetStride());
			return Index;
		}

		FORCEINLINE void Shrink()
		{
			CheckInvariants();

			if (ArrayNum != ArrayMax)
			{
				ResizeAllocation(ArrayNum);
			}
		}

		FORCEINLINE void Empty(SizeType ExpectedUseSize = 0, SizeType MaxSlack = 0)
		{
			CheckInvariants();

			CheckNotNegative(ExpectedUseSize, TEXT("ExpectedUseSize"));
			CheckNotNegative(MaxSlack, TEXT("MaxSlack"));

			ArrayNum = 0;
			BroadcastNumChanged();

			if (ExpectedUseSize > ArrayMax || ArrayMax > (ExpectedUseSize + MaxSlack))
			{
				ResizeAllocation(ExpectedUseSize);
			}
		}

		FORCEINLINE void Reserve(SizeType Number)
		{
			CheckInvariants();

			CheckNotNegative(Number, TEXT("Number"));

			if (Number > ArrayMax)
			{
				ResizeAllocation(Number);
			}
		}

		FORCEINLINE void SetNumUninitialized(SizeType NewNum, bool bAllowShrinking = true)
		{
			CheckInvariants();

			CheckNotNegative(NewNum, TEXT("NewNum"));

			if (NewNum > ArrayNum)
			{
				AddUninitialized(NewNum - ArrayNum);
			}
			else if (NewNum < ArrayNum)
			{
				RemoveAt(NewNum, ArrayNum - NewNum, bAllowShrinking);
			}
		}

		FORCEINLINE void SetNumZeroed(int32 NewNum, bool bAllowShrinking = true)
		{
			CheckInvariants();

			CheckNotNegative(NewNum, TEXT("NewNum"));

			if (NewNum > ArrayNum)
			{
				AddZeroed(NewNum - ArrayNum);
			}
			else if (NewNum < ArrayNum)
			{
				RemoveAt(NewNum, ArrayNum - NewNum, bAllowShrinking);
			}
		}

		FORCEINLINE void RemoveAt(SizeType Index, SizeType Count = 1, bool bAllowShrinking = true)
		{
			if (Count)
			{
				CheckInvariants();
				CheckNotNegative(Count, TEXT("Count"));
				CheckNotNegative(Index, TEXT("Index"));
				checkSlow(Index + Count <= ArrayNum);

				// Skip memmove in the common case that there is nothing to move.
				if (const SizeType NumToMove = ArrayNum - Index - Count)
				{
					FMemory::Memmove
					(
						reinterpret_cast<uint8*>(Allocator.GetAllocation()) + (Index) * GetStride(),
						reinterpret_cast<uint8*>(Allocator.GetAllocation()) + (Index + Count) * GetStride(),
						NumToMove * GetStride()
					);
				}

				if (bAllowShrinking)
				{
					ResizeAllocationShrink(ArrayNum);
				}

				ArrayNum -= Count;
				BroadcastNumChanged();
			}
		}

		void ZeroRange(int32 StartIndex, int32 Num)
		{
			CheckNotNegative(StartIndex, TEXT("StartIndex"));
			CheckNotNegative(Num, TEXT("Num"));
			checkSlow(StartIndex + Num <= ArrayNum);

			FMemory::Memzero(reinterpret_cast<uint8*>(Allocator.GetAllocation()) + StartIndex * GetStride(), Num * GetStride());
		}

		void FillRange(int32 StartIndex, int32 Num, const FRealtimeMeshStreamDefaultRowValue& Value)
		{
			// If we have no data, we can just zero the range
			if (!Value.HasData())
			{
				ZeroRange(StartIndex, Num);
				return;
			}
			
			check(Value.HasData());
			check(Value.GetLayout() == GetLayout());
			CheckNotNegative(StartIndex, TEXT("StartIndex"));
			CheckNotNegative(Num, TEXT("Num"));
			checkSlow(StartIndex + Num <= ArrayNum);

			const uint8* SrcRow = Value.GetDataPtr();
			
			uint8* Dst = GetDataRawAtVertex(StartIndex);
			for (int32 Index = 0; Index < Num; Index++)
			{
				FMemory::Memcpy(Dst, SrcRow, GetStride());
				Dst += GetStride();
			}			
		}
		

		template<typename DataType>
		void FillRange(int32 StartIndex, int32 Num, const DataType& Value)
		{
			CheckNotNegative(StartIndex, TEXT("StartIndex"));
			CheckNotNegative(Num, TEXT("Num"));
			checkSlow(StartIndex + Num <= ArrayNum);
			if (Num <= 0)
			{
				return;
			}

			const auto SourceLayout = GetRealtimeMeshBufferLayout<DataType>();
			
			// Can we do a simple bitwise copy? This is the fastest option, but only works if the types line up exactly
			if (SourceLayout == GetLayout())
			{
				DataType* DataPtr = GetDataAtVertex<DataType>(StartIndex);
				for (int32 Index = 0; Index < Num; ++Index)
				{
					DataPtr[Index] = Value;
				}
				return;
			}

			const FRealtimeMeshElementType FromType = SourceLayout.GetElementType();
			const FRealtimeMeshElementType ToType = GetLayout().GetElementType();
			const bool bSameNumElements = GetLayout().GetNumElements() == SourceLayout.GetNumElements();

			// If this isn't a convertible type, it's invalid to append
			check(FRealtimeMeshTypeConversionUtilities::CanConvert(FromType, ToType) && bSameNumElements);
						
			const auto& Converter = FRealtimeMeshTypeConversionUtilities::GetTypeConverter(FromType, ToType);

			// Convert the first element, but then we can blind copy that converted value for the rest to reduce the overhead of the conversion
			if (GetLayout().GetNumElements() > 1)
			{
				// Multi element streams, we use a contiguous array conversion per row to convert all the elements
				const int32 NumElements = GetLayout().GetNumElements();

				Converter.ConvertContiguousArray(&Value, GetDataRawAtVertex(StartIndex), NumElements);
			}
			else // Single element stream, we don't need the added complexity of multi element conversion per row
			{
				Converter.ConvertSingleElement(&Value, GetDataRawAtVertex(StartIndex));
			}

			// If we're filling more than one row, copy to remaining rows
			if (Num > 1)
			{
				const void* SrcRow = GetDataRawAtVertex(StartIndex);
				for (int32 Index = 1; Index < Num; Index++)
				{
					FMemory::Memcpy(GetDataRawAtVertex(StartIndex + Index), SrcRow, GetStride());
				}
			}			
		}
		


		template <typename VertexType>
		void Add(const VertexType& InVertex)
		{
			check(Layout == GetRealtimeMeshBufferLayout<VertexType>());
			const SizeType Index = AddUninitialized();
			*GetDataAtVertex<VertexType>(Index) = InVertex;
		}

		void Append(const FRealtimeMeshStream& Source)
		{
			// Don't allow appending to self
			check(this != &Source);

			Append(Source.GetLayout(), Source.GetData(), Source.Num());
		}

		void Append(FRealtimeMeshStream&& Source)
		{
			CheckInvariants();
			check(this != &Source);

			// Does the other stream have nothing to append?
			if (Source.Num() == 0)
			{
				return;
			}

			// We can only really move if this is empty
			if (Num() == 0)
			{
				*this = MoveTemp(Source);
			}
			else
			{
				Append(Source.GetLayout(), Source.GetData(), Source.Num());
				Source.Empty(0, 0);
			}
		}

		template <typename SourceType>
		void Append(TArrayView<SourceType> NewElements)
		{
			Append(GetRealtimeMeshBufferLayout<SourceType>(), NewElements.GetData(), NewElements.Num());
		}

		template <typename VertexType, typename InAllocatorType = FDefaultAllocator>
		void Append(const TArray<VertexType, InAllocatorType>& NewElements)
		{
			Append(GetRealtimeMeshBufferLayout<VertexType>(), reinterpret_cast<const uint8*>(NewElements.GetData()), NewElements.Num());
		}

		template <typename VertexType>
		void Append(std::initializer_list<VertexType> NewElements)
		{
			Append(GetRealtimeMeshBufferLayout<VertexType>(), NewElements.begin(), NewElements.size());
		}

		template <typename VertexType>
		void Append(int32 Count, VertexType* NewElements)
		{
			Append(GetRealtimeMeshBufferLayout<VertexType>(), NewElements, Count);
		}

		template <typename VertexType, typename GeneratorFunc>
		void AppendGenerated(int32 Count, GeneratorFunc Generator)
		{
			const SizeType Index = AddUninitialized(Count);
			SetGenerated<VertexType, GeneratorFunc>(Index, Count, Forward<GeneratorFunc>(Generator));
		}

		template <typename VertexType>
		void SetRange(int32 StartIndex, TArrayView<VertexType> NewElements)
		{
			SetRange(StartIndex, GetRealtimeMeshBufferLayout<VertexType>(), NewElements.GetData(), NewElements.Num());
		}

		template <typename VertexType, typename InAllocatorType = FDefaultAllocator>
		void SetRange(int32 StartIndex, const TArray<VertexType, InAllocatorType>& NewElements)
		{
			SetRange(StartIndex, GetRealtimeMeshBufferLayout<VertexType>(), NewElements.GetData(), NewElements.Num());
		}

		template <typename VertexType>
		void SetRange(int32 StartIndex, std::initializer_list<VertexType> NewElements)
		{
			SetRange(StartIndex, GetRealtimeMeshBufferLayout<VertexType>(), NewElements.begin(), NewElements.size());
		}

		template <typename VertexType>
		void SetRange(int32 StartIndex, int32 Count, VertexType* NewElements)
		{
			SetRange(StartIndex, GetRealtimeMeshBufferLayout<VertexType>(), NewElements, Count);
		}

		void SetRange(uint32 DestinationIndex, const FRealtimeMeshBufferLayout& SourceLayout, const uint8* const SourceData, uint32 SourceCount)
		{
			CopyStreamDataIntoStream(Layout, GetDataRawAtVertex(DestinationIndex), 0, SourceLayout, SourceData, SourceCount);
		}
		
		template <typename VertexType, typename GeneratorFunc>
		void SetGenerated(int32 StartIndex, int32 Count, GeneratorFunc Generator)
		{
			if (Count == 0)
			{
				return;
			}
			
			RangeCheck(StartIndex + Count - 1);

			const auto SourceLayout = GetRealtimeMeshBufferLayout<VertexType>();
			
			// Can we do a simple bitwise copy? This is the fastest option, but only works if the types line up exactly
			if (SourceLayout == GetLayout())
			{
				VertexType* DataPtr = GetDataAtVertex<VertexType>(StartIndex);
				for (int32 Index = 0; Index < Count; ++Index)
				{
					DataPtr[Index] = Generator(Index);
				}
				return;
			}

			const FRealtimeMeshElementType FromType = GetLayout().GetElementType();
			const FRealtimeMeshElementType ToType = SourceLayout.GetElementType();
			const bool bSameNumElements = GetLayout().GetNumElements() == SourceLayout.GetNumElements();

			// If this isn't a convertible type, it's invalid to append
			check(FRealtimeMeshTypeConversionUtilities::CanConvert(FromType, ToType) && bSameNumElements);
						
			const auto& Converter = FRealtimeMeshTypeConversionUtilities::GetTypeConverter(FromType, ToType);

			if (GetLayout().GetNumElements() > 1)
			{
				// Multi element streams, we use a contiguous array conversion per row to convert all the elements
				const int32 NumElements = GetLayout().GetNumElements();
				
				for (int32 Index = 0; Index < Count; ++Index)
				{
					VertexType NewElement = Generator(Index);				
					Converter.ConvertContiguousArray(&NewElement, GetDataRawAtVertex(StartIndex + Index), NumElements);
				}
			}
			else // Single element stream, we don't need the added complexity of multi element conversion per row
			{
				for (int32 Index = 0; Index < Count; ++Index)
				{
					VertexType NewElement = Generator(Index);	
					Converter.ConvertSingleElement(&NewElement, GetDataRawAtVertex(StartIndex + Index));
				}
			}
		}

		
		template <typename ElementType, typename GeneratorFunc>
		void SetGeneratedElement(int32 ElementIndex, int32 StartIndex, int32 Count, GeneratorFunc Generator)
		{
			// TODO: Upgrade this like SetGenerated
			RangeCheck(StartIndex + Count - 1);
			ElementCheck(ElementIndex);
			
			ElementType* DataPtr = GetDataAtVertex<ElementType>(StartIndex, ElementIndex);

			for (int32 Index = 0; Index < Count; ++Index)
			{				
				*DataPtr = Generator(Index);
				DataPtr += GetNumElements();
			}
		}
		
		template <typename VertexType>
		void CopyRange(int32 StartIndex, TArrayView<VertexType> OutputElements)
		{
			CopyRange(StartIndex, GetRealtimeMeshBufferLayout<VertexType>(), OutputElements.GetData(), OutputElements.Num());
		}
		
		template <typename VertexType>
		void CopyRange(int32 StartIndex, int32 Count, TArray<VertexType>& OutputElements)
		{
			const SizeType DestinationIndex = OutputElements.AddUninitialized(Count);
			CopyRange(StartIndex, GetRealtimeMeshBufferLayout<VertexType>(), reinterpret_cast<uint8*>(&OutputElements[DestinationIndex]), Count);
		}


		

		

		void CountBytes(FArchive& Ar) const
		{
			Ar.CountBytes(ArrayNum * GetStride(), ArrayMax * GetStride());
		}

		friend FArchive& operator<<(FArchive& Ar, FRealtimeMeshStream& Stream)
		{
			if (Ar.CustomVer(FRealtimeMeshVersion::GUID) >= FRealtimeMeshVersion::StreamsNowHoldEntireKey)
			{
				Ar << Stream.StreamKey;
			}
			else
			{
				FName StreamName;
				Ar << StreamName;
				Stream.StreamKey = FRealtimeMeshStreamKey(ERealtimeMeshStreamType::Unknown, StreamName);
			}

			Ar << Stream.Layout;
			Stream.CacheStrides();
			
			if (Ar.IsLoading())
			{
				//Stream.LayoutDefinition = FRealtimeMeshBufferLayoutDefinition(FRealtimeMeshBufferLayoutUtilities::GetBufferLayoutDefinition(BufferLayout));

				if (Ar.CustomVer(FRealtimeMeshVersion::GUID) < FRealtimeMeshVersion::StreamsNowHoldEntireKey)
				{
					ERealtimeMeshStreamType StreamType =
					(Stream.Layout == GetRealtimeMeshBufferLayout<uint32>() ||
						Stream.Layout == GetRealtimeMeshBufferLayout<int32>() ||
						Stream.Layout == GetRealtimeMeshBufferLayout<uint16>())
						? ERealtimeMeshStreamType::Index
						: ERealtimeMeshStreamType::Vertex;
					Stream.StreamKey = FRealtimeMeshStreamKey(ERealtimeMeshStreamType::Unknown, Stream.StreamKey.GetName());
				}
			}

			Stream.CountBytes(Ar);

			SizeType SerializedNum = Ar.IsLoading() ? 0 : Stream.ArrayNum;;
			Ar << SerializedNum;

			if (SerializedNum > 0)
			{
				Stream.ArrayNum = 0;

				// Serialize simple bytes which require no construction or destruction.
				if (SerializedNum && Ar.IsLoading())
				{
					Stream.ResizeAllocation(SerializedNum);
				}

				// TODO: This will not handle endianness of the vertex data for say a network archive.
				Ar.Serialize(Stream.GetData(), SerializedNum * Stream.GetStride());
				Stream.ArrayNum = SerializedNum;

				if (Ar.IsLoading())
				{					
					Stream.BroadcastNumChanged();
				}
			}
			else if (Ar.IsLoading())
			{
				Stream.Empty();
			}
			return Ar;
		}

		bool IsLinked() const { return Linkage != nullptr; }
		void UnLink()
		{
			if (Linkage)
			{
				Linkage->HandleStreamRemoved(this);
				Linkage = nullptr;
			}
		}

	private:
		FORCEINLINE void CheckInvariants() const
		{
			checkSlow((ArrayNum >= 0) & (ArrayMax >= ArrayNum)); // & for one branch
		}

		FORCEINLINE void RangeCheck(SizeType Index) const
		{
			CheckInvariants();

			// Template property, branch will be optimized out
			if constexpr (AllocatorType::RequireRangeCheck)
			{
				checkf((Index >= 0) & (Index < ArrayNum), TEXT("Array index out of bounds: %lld from an array of size %lld"), static_cast<int64>(Index),
					   static_cast<int64>(ArrayNum)); // & for one branch
			}
		}

		FORCEINLINE void ElementCheck(SizeType ElementIndex) const
		{
			checkf((ElementIndex >= 0) & (ElementIndex < GetNumElements()), TEXT("Element index out of bounds: %lld from an element list of size %lld"),
				   static_cast<int64>(ElementIndex), static_cast<int64>(GetNumElements())); // & for one branch
		}

		void BroadcastAllocatedSizeChanged()
		{			
			if (Linkage)
			{
				Linkage->HandleAllocatedSizeChanged(this, ArrayMax);
			}
		}

		void BroadcastNumChanged()
		{
			if (Linkage)
			{
				Linkage->HandleNumChanged(this, ArrayNum);
			}
		}

		void ResizeAllocation(USizeType NewNum, bool bKeepElements = true)
		{
			if (NewNum != ArrayMax)
			{
				Allocator.ResizeAllocation(bKeepElements? ArrayNum : 0, NewNum, Stride, Alignment);
				ArrayMax = NewNum;
				BroadcastAllocatedSizeChanged();
			}
		}

		void ResizeAllocationGrow(USizeType NewMinNum)
		{
			const SizeType NewAllocationSize = Allocator.CalculateSlackGrow(NewMinNum, ArrayMax, Stride, Alignment);

			if (NewAllocationSize != ArrayMax)
			{
				Allocator.ResizeAllocation(ArrayNum, NewAllocationSize, Stride, Alignment);
				ArrayMax = NewAllocationSize;
				BroadcastAllocatedSizeChanged();
			}
		}

		void ResizeAllocationShrink(USizeType NewNum)
		{
			const SizeType NewAllocationSize = Allocator.CalculateSlackShrink(NewNum, ArrayMax, Stride, Alignment);

			if (NewAllocationSize != ArrayMax)
			{
				Allocator.ResizeAllocation(ArrayNum, NewAllocationSize, Stride, Alignment);
				ArrayMax = NewAllocationSize;
				BroadcastAllocatedSizeChanged();
			}
		}


		FORCEINLINE static void CheckNotNegative(SizeType InValue, const TCHAR* ParameterName)
		{
			if (InValue < 0)
			{
				UE_LOG(LogCore, Fatal, TEXT("Invalid value for %s, must not be negative..."), ParameterName);
			}
		}


		
		void Append(const FRealtimeMeshBufferLayout& SourceLayout, const uint8* const SourceData, uint32 SourceCount)
		{
			const SizeType DestinationIndex = AddUninitialized(SourceCount);
			SetRange(DestinationIndex, SourceLayout, SourceData, SourceCount);
		}


		static void CopyStreamDataIntoStream(const FRealtimeMeshBufferLayout& DestinationLayout, uint8* DestinationData, uint32 ElementOffset, const FRealtimeMeshBufferLayout& SourceLayout, const uint8* const SourceData, uint32 SourceCount)
		{
			if (SourceCount == 0)
			{
				return;
			}
			
			// Do we have enough space to copy this chunk?
			//RangeCheck(DestinationIndex + SourceCount - 1);
			
			const FRealtimeMeshElementType FromType = SourceLayout.GetElementType();
			const FRealtimeMeshElementType ToType = DestinationLayout.GetElementType();
			const uint32 NumElementsInDestination = DestinationLayout.GetNumElements();
			const uint32 NumElementsInSource = SourceLayout.GetNumElements();

			// Can we fit the number of supplied elements
			check((ElementOffset + NumElementsInSource) <= NumElementsInDestination);
			
			// If this isn't a convertible type, it's invalid to append
			check(FromType == ToType || FRealtimeMeshTypeConversionUtilities::CanConvert(FromType, ToType));

			const uint32 SourceStride = FRealtimeMeshBufferLayoutUtilities::GetElementStride(SourceLayout.GetElementType()) * SourceLayout.GetNumElements();
			const uint32 DestinationStride = FRealtimeMeshBufferLayoutUtilities::GetElementStride(DestinationLayout.GetElementType()) * DestinationLayout.GetNumElements();
			
			// If the full stream types are the same and the offset is zero we can do a simple buffer copy
			if (SourceLayout == DestinationLayout && ElementOffset == 0)
			{
				check(SourceStride == DestinationStride);
				FMemory::Memcpy(DestinationData, SourceData, SourceCount * DestinationStride);
				return;
			}

			// If the types are the same then we can interleave it by simple copying.
			if (FromType == ToType)
			{
				// We do a straight copy one row at a time.
				for (uint32 Index = 0; Index < SourceCount; Index++)
				{
					FMemory::Memcpy(DestinationData + (DestinationStride * Index), SourceData + (SourceStride * Index), SourceStride);
				}
				return;
			}
			
			// The remaining options all have to do data conversion, so grab the converter for this type pair
			const auto& Converter = FRealtimeMeshTypeConversionUtilities::GetTypeConverter(FromType, ToType);
			
			// If we have an offset of zero and the num elements in source and destination are the same we
			// can do a contiguous array conversion which is the fastest option for conversion
			if (ElementOffset == 0 && NumElementsInDestination == NumElementsInSource)
			{
				Converter.ConvertContiguousArray(SourceData, DestinationData, SourceCount * NumElementsInSource);
				return;
			}

			// Our last options is to do a interleaved conversion.
			// We can still do a contiguous conversion if there's more than one element.
			// But if not we'd rather do a single element conversion as it avoids the overhead of the inner loop
			if (SourceLayout.GetNumElements() > 1)
			{				
				// We can do a contiguous conversion for the elements within each row to avoid the overhead of the function call in the inner loop
				for (uint32 Index = 0; Index < SourceCount; Index++)
				{
					Converter.ConvertContiguousArray(SourceData + (SourceStride * Index), DestinationData + (DestinationStride * Index), NumElementsInSource);
				}
			}
			else
			{				
				// Here we do a simple single element conversion for each row
				for (uint32 Index = 0; Index < SourceCount; Index++)
				{
					Converter.ConvertSingleElement(SourceData + (SourceStride * Index), DestinationData + (DestinationStride * Index));
				}				
			}
		}

		void CopyRange(uint32 SourceIndex, const FRealtimeMeshBufferLayout& DestinationLayout, uint8* DestinationData, uint32 DestinationCount) const
		{
			CopyStreamDataIntoStream(DestinationLayout, DestinationData, 0, Layout, GetDataRawAtVertex(SourceIndex), DestinationCount);
		}

		friend struct FRealtimeMeshStreamLinkage;
	};


	struct REALTIMEMESHCOMPONENT_API FRealtimeMeshStreamSet
	{
	private:
		TMap<FRealtimeMeshStreamKey, TUniquePtr<FRealtimeMeshStream>> Streams;
		TMap<FName, TUniquePtr<FRealtimeMeshStreamLinkage>> StreamLinkages;

		void CleanUpLinkages()
		{
			for (auto It = StreamLinkages.CreateIterator(); It; ++It)
			{
				if (It.Value()->NumStreams() == 0)
				{
					It.RemoveCurrent();
				}
			}
		
		}
	public:
		FRealtimeMeshStreamSet() = default;
		virtual ~FRealtimeMeshStreamSet() = default;

		
		// We don't allow implicit copying as it leads to unnecessary stream copies
		explicit FRealtimeMeshStreamSet(const FRealtimeMeshStreamSet& Other)
		{
			CopyFrom(Other);
		}
		FRealtimeMeshStreamSet(FRealtimeMeshStreamSet&&) = default;

		// We don't allow implicit copying as it leads to unnecessary stream copies
		// To copy a stream set use CopyFrom or the explicit copy constructor
		FRealtimeMeshStreamSet& operator=(const FRealtimeMeshStreamSet&) = delete;

		// We do allow move operation
		FRealtimeMeshStreamSet& operator=(FRealtimeMeshStreamSet&&) = default;

		void CopyFrom(const FRealtimeMeshStreamSet& Other)
		{
			Streams.Empty(Other.Streams.Num());
			for (auto SetIt = Other.Streams.CreateConstIterator(); SetIt; ++SetIt)
			{
				Streams.Emplace(SetIt.Value()->GetStreamKey(), MakeUnique<FRealtimeMeshStream>(*SetIt->Value));
			}

			StreamLinkages.Empty(Other.StreamLinkages.Num());
			for (auto SetIt = Other.StreamLinkages.CreateConstIterator(); SetIt; ++SetIt)
			{
				auto& NewLinkage = StreamLinkages.Emplace(SetIt.Key(), MakeUnique<FRealtimeMeshStreamLinkage>());

				SetIt.Value()->ForEachStream([&](const FRealtimeMeshStream& SourceStream, const FRealtimeMeshStreamDefaultRowValue& DefaultValue)
				{
					NewLinkage->BindStream(*Streams.FindChecked(SourceStream.GetStreamKey()), DefaultValue);
				});
			}
		}

		int32 Num() const { return Streams.Num(); }
		void Empty() { Streams.Empty(); StreamLinkages.Empty(); }
		bool IsEmpty() const { return Streams.IsEmpty(); }

		int32 Remove(const FRealtimeMeshStreamKey& StreamKey)
		{
			const auto* Stream = Streams.Find(StreamKey);
			if (Stream && (*Stream)->IsLinked())
			{
				(*Stream)->UnLink();
				CleanUpLinkages();
			}			
			return Streams.Remove(StreamKey);
		}

		int32 RemoveAll(const TSet<FRealtimeMeshStreamKey>& StreamKeys)
		{
			int32 RemovedCount = 0;
			for (const FRealtimeMeshStreamKey& StreamKey : StreamKeys)
			{
				RemovedCount += Streams.Remove(StreamKey);
			}
			if  (RemovedCount)
			{
				CleanUpLinkages();				
			}
			return RemovedCount;
		}
		
		FRealtimeMeshStream* Find(const FRealtimeMeshStreamKey& StreamKey)
		{
			if (const auto* Result = Streams.Find(StreamKey))
			{
				return Result->Get();
			}
			return nullptr;
		}
		
		const FRealtimeMeshStream* Find(const FRealtimeMeshStreamKey& StreamKey) const
		{
			if (const auto* Result = Streams.Find(StreamKey))
			{
				return Result->Get();
			}
			return nullptr;
		}
		
		FRealtimeMeshStream& FindOrAdd(const FRealtimeMeshStreamKey& StreamKey, const FRealtimeMeshBufferLayout& NewLayout, bool bKeepData = true)
		{
			if (auto* Result = Streams.Find(StreamKey))
			{
				if (!bKeepData)
				{
					(*Result)->Empty();
				}
				if (!(*Result)->ConvertTo(NewLayout))
				{
					UE_LOG(LogCore, Warning, TEXT("Failed to convert stream %s to new layout: Removing Data"), *StreamKey.ToString());
				}
				return *Result->Get();
			}
			
			auto& Entry = Streams.FindOrAdd(StreamKey);
			Entry = MakeUnique<FRealtimeMeshStream>(StreamKey, NewLayout);
			return *Entry.Get();
		}

		FRealtimeMeshStream& FindChecked(const FRealtimeMeshStreamKey& StreamKey)
		{
			const auto* Result = Streams.Find(StreamKey);
			check(Result);
			return *Result->Get();
		}
		const FRealtimeMeshStream& FindChecked(const FRealtimeMeshStreamKey& StreamKey) const
		{
			const auto* Result = Streams.Find(StreamKey);
			check(Result);
			return *Result->Get();
		}

		bool Contains(const FRealtimeMeshStreamKey& StreamKey) const
		{
			return Streams.Contains(StreamKey);
		}

		TSet<FRealtimeMeshStreamKey> GetStreamKeys() const
		{
			TSet<FRealtimeMeshStreamKey> Keys;
			Streams.GetKeys(Keys);
			return Keys;
		}

		TSet<FRealtimeMeshStreamKey> FindDifference(const FRealtimeMeshStreamSet& Other) const
		{
			TSet<FRealtimeMeshStreamKey> Result;
			Result.Reserve(Streams.Num());

			for (auto SetIt = Streams.CreateConstIterator(); SetIt; ++SetIt)
			{
				if (!Other.Streams.Contains(SetIt->Key))
				{
					Result.Add(SetIt->Key);
				}
			}
			return Result;
		}

		FRealtimeMeshStream& AddStream(ERealtimeMeshStreamType StreamType, FName StreamName, const FRealtimeMeshBufferLayout& InLayout)
		{
			const FRealtimeMeshStreamKey StreamKey(StreamType, StreamName);
			auto& Entry = Streams.FindOrAdd(StreamKey);
			Entry = MakeUnique<FRealtimeMeshStream>(StreamKey, InLayout);
			return *Entry.Get();
		}
		
		FRealtimeMeshStream& AddStream(const FRealtimeMeshStreamKey& StreamKey, const FRealtimeMeshBufferLayout& InLayout)
		{
			auto& Entry = Streams.FindOrAdd(StreamKey);
			Entry = MakeUnique<FRealtimeMeshStream>(StreamKey, InLayout);
			return *Entry.Get();
		}

		template <typename StreamLayout>
		FRealtimeMeshStream& AddStream(ERealtimeMeshStreamType StreamType, FName StreamName)
		{
			const FRealtimeMeshStreamKey StreamKey(StreamType, StreamName);
			auto& Entry = Streams.FindOrAdd(StreamKey);
			Entry = MakeUnique<FRealtimeMeshStream>(StreamKey, GetRealtimeMeshBufferLayout<StreamLayout>());
			return *Entry.Get();
		}

		template <typename StreamLayout>
		FRealtimeMeshStream& AddStream(const FRealtimeMeshStreamKey& StreamKey)
		{
			auto& Entry = Streams.FindOrAdd(StreamKey);
			Entry = MakeUnique<FRealtimeMeshStream>(StreamKey, GetRealtimeMeshBufferLayout<StreamLayout>());
			return *Entry.Get();
		}

		FRealtimeMeshStream& AddStream(const FRealtimeMeshStream& Stream)
		{
			auto& Entry = Streams.FindOrAdd(Stream.GetStreamKey());
			Entry = MakeUnique<FRealtimeMeshStream>(Stream);
			return *Entry.Get();
		}

		FRealtimeMeshStream& AddStream(FRealtimeMeshStream&& Stream)
		{
			auto& Entry = Streams.FindOrAdd(Stream.GetStreamKey());
			Entry = MakeUnique<FRealtimeMeshStream>(MoveTemp(Stream));
			return *Entry.Get();
		}


		void ForEach(const TFunctionRef<void(FRealtimeMeshStream&)>& Func)
		{
			for (auto SetIt = Streams.CreateConstIterator(); SetIt; ++SetIt)
			{
				Func(*SetIt->Value.Get());
			}
		}
		
		void ForEach(const TFunctionRef<void(const FRealtimeMeshStream&)>& Func) const
		{
			for (auto SetIt = Streams.CreateConstIterator(); SetIt; ++SetIt)
			{
				Func(*SetIt->Value.Get());
			}
		}


		void AddStreamToLinkPool(FName LinkPool, const FRealtimeMeshStreamKey& StreamKey, const FRealtimeMeshStreamDefaultRowValue& DefaultInitializer)
		{
			const auto* Stream = Streams.Find(StreamKey);
			checkf(Stream, TEXT("Stream %s not found in the stream set"), *StreamKey.ToString());
			auto& Entry = StreamLinkages.FindOrAdd(LinkPool);

			// If we're already linked, skip it
			if (Entry.IsValid() && Entry->ContainsStream(Stream->Get()))
			{
				return;
			}
			
			checkf(!(*Stream)->IsLinked(), TEXT("Stream %s is already linked but not to this pool"), *StreamKey.ToString());
			if (!Entry)
			{
				Entry = MakeUnique<FRealtimeMeshStreamLinkage>();
			}
			Entry->BindStream(Stream->Get(), DefaultInitializer);
		}
		
		void AddStreamToLinkPool(FName LinkPool, const FRealtimeMeshStreamKey& StreamKey)
		{
			AddStreamToLinkPool(LinkPool, StreamKey, FRealtimeMeshStreamDefaultRowValue());
		}

		void RemoveStreamFromLinkPool(FName LinkPool, const FRealtimeMeshStreamKey& StreamKey)
		{
			const auto* Stream = Streams.Find(StreamKey);
			checkf(Stream, TEXT("Stream %s not found in the stream set"), *StreamKey.ToString());
			auto* Linkage = StreamLinkages.Find(LinkPool);
			if (Linkage)
			{
				(*Linkage)->RemoveStream(Stream->Get());
				if ((*Linkage)->NumStreams() == 0)
				{
					StreamLinkages.Remove(LinkPool);
				}
			}
		}



	};

	using FRealtimeMeshStreamProxyMap = TMap<FRealtimeMeshStreamKey, TSharedPtr<FRealtimeMeshGPUBuffer>>;


	namespace NatVis
	{
		REALTIMEMESHCOMPONENT_API std::string GetRowElementAsString(const FRealtimeMeshStream& Stream, int32 Row, int32 Element) noexcept;
	
		REALTIMEMESHCOMPONENT_API std::string GetRowAsString(const FRealtimeMeshStream& Stream, int32 Row, int32 Element) noexcept;
	}
}

