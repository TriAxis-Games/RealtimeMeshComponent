// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "RealtimeMeshStreamRange.h"
#include "RealtimeMeshDataTypes.h"
#include "RealtimeMeshDataConversion.h"
#include "RealtimeMeshStreamInterface.h"
#include "Containers/StridedView.h"
#include "Templates/MakeUnsigned.h"


struct FRealtimeMeshStreamKey;

namespace RealtimeMesh
{
	class FRealtimeMeshGPUBuffer;

	struct FRealtimeMeshStreams
	{
		inline static const FName PositionStreamName = FName(TEXT("Position"));
		// Previous-frame positions (same layout as Position). When a section group has this stream the
		// render proxy emits motion vectors for it, so a deforming mesh (compute- or CPU-driven) is
		// correct under TAA/TSR. Holds where each vertex was last frame.
		inline static const FName PositionPrevStreamName = FName(TEXT("PositionPrev"));
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
		inline static const FRealtimeMeshStreamKey PositionPrev = FRealtimeMeshStreamKey(ERealtimeMeshStreamType::Vertex, PositionPrevStreamName);
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

	// Convert a single element row through the appropriate converter entry point. Multi-element
	// rows go through the contiguous-array converter; single-element rows use the cheaper single-element
	// path.
	FORCEINLINE void ConvertElementRow(const FRealtimeMeshElementConverters& Converter, const void* Source, void* Destination, int32 NumElements)
	{
		if (NumElements > 1)
		{
			Converter.ConvertContiguousArray(Source, Destination, NumElements);
		}
		else
		{
			Converter.ConvertSingleElement(Source, Destination);
		}
	}

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
		FRealtimeMeshStreamDefaultRowValue(const FRealtimeMeshBufferLayout& InLayout, TArray<uint8, TInlineAllocator<64>>&& InData)
			: Layout(InLayout)
			, Data(MoveTemp(InData))
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
				const uint8* SourceData = reinterpret_cast<const uint8*>(&Value);
				TArray<uint8, TInlineAllocator<64>> DataCopy(SourceData, SourceMemoryLayout.GetStride());
				return FRealtimeMeshStreamDefaultRowValue(SourceLayout, MoveTemp(DataCopy));
			}
			
			const FRealtimeMeshElementType FromType = SourceLayout.GetElementType();
			const FRealtimeMeshElementType ToType = FinalLayout.GetElementType();
			const bool bSameNumElements = FinalLayout.GetNumElements() == SourceLayout.GetNumElements();

			const FRealtimeMeshBufferMemoryLayout FinalMemoryLayout = FRealtimeMeshBufferLayoutUtilities::GetBufferLayoutMemoryLayout(FinalLayout);

			// If this isn't a convertible type, it's invalid to append
			check(FRealtimeMeshTypeConversionUtilities::CanConvert(FromType, ToType) && bSameNumElements);
						
			TArray<uint8, TInlineAllocator<64>> ConvertedData;
			ConvertedData.SetNumUninitialized(FinalMemoryLayout.GetStride());

			FRealtimeMeshTypeConversionUtilities::ProcessTypeConverter(FromType, ToType,
				[&](const FRealtimeMeshElementConverters& Converter)
				{
					// Convert the single default-row value; callers blind-copy it for the rest.
					ConvertElementRow(Converter, &Value, ConvertedData.GetData(), SourceLayout.GetNumElements());
				});

			return FRealtimeMeshStreamDefaultRowValue(FinalLayout, MoveTemp(ConvertedData));
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

#if DO_CHECK
		void CheckStreams();
#else
		FORCEINLINE void CheckStreams() {}
#endif

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

		// ---- Batch mutation API ----
		//
		// Linkage-driven counterpart to the per-stream AddUninitialized / SetNum* /
		// RemoveAt path. The per-stream operations still work and still broadcast
		// through the linkage as a safety net for direct stream mutation, but for
		// builder-style code that knows it wants N rows added across every linked
		// stream, these methods are dramatically cheaper: one growth-math call
		// (in row units, stride-agnostic), one capacity apply pass per stream,
		// and no per-stream broadcast round-trip.

		// Adds N rows to every linked stream and returns the starting index. New
		// rows are uninitialized — the caller is responsible for writing them.
		// Use this from builder paths where every linked stream will be filled
		// immediately after.
		int32 AddRowsUninitialized(int32 NumRows);

		// Like AddRowsUninitialized but fills new rows with each member stream's
		// default row value (set at BindStream time).
		int32 AddRowsZeroed(int32 NumRows);

		// Forces all linked streams to NewNum rows, growing (with defaults) or
		// shrinking as needed. Returns the previous row count.
		int32 SetNumRows(int32 NewNum);

		// Removes Count rows starting at StartIdx from every linked stream,
		// shifting later rows down. Optionally shrinks the allocation slack.
		void RemoveRows(int32 StartIdx, int32 Count, bool bAllowShrinking = true);

		// Grows allocation to hold at least NumRows rows on every linked stream.
		// Does not change ArrayNum. No-op if all streams already have capacity.
		void ReserveRows(int32 NumRows);

		// Current shared row count (same across every linked stream by invariant).
		// Returns 0 when no streams are bound.
		int32 GetNumRows() const;

	private:
		// Common helper: grow/shrink every linked stream's allocation to hold
		// exactly NewCapacityRows, preserving existing data. Bypasses the public
		// ResizeAllocation broadcast path since we're already iterating all
		// members ourselves.
		void ResizeAllStreamAllocations(int32 NewCapacityRows);
	};
	
	struct REALTIMEMESHCOMPONENT_API FRealtimeMeshStream final : public FResourceArrayInterface, public IRealtimeMeshStream
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

		// Row stride = ElementStride * NumElements. uint16 because large layouts
		// (e.g. FVector4d x 8 = 256 bytes) overflow a uint8 to 0.
		uint16 Stride;
		uint8 ElementStride;
		uint8 Alignment;

		FORCEINLINE void CacheStrides()
		{
			ElementStride = FRealtimeMeshBufferLayoutUtilities::GetElementStride(Layout.GetElementType());
			Alignment = FRealtimeMeshBufferLayoutUtilities::GetElementAlignment(Layout.GetElementType());
			Stride = static_cast<uint16>(ElementStride) * Layout.GetNumElements();
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
			
			Other.UnLink();
			Allocator.MoveToEmpty(Other.Allocator);

			Other.Layout = FRealtimeMeshBufferLayout::Invalid;
			Other.ArrayNum = 0;
			Other.ArrayMax = 0;
			Other.StreamKey = FRealtimeMeshStreamKey(ERealtimeMeshStreamType::Unknown, NAME_None);
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
			if (this == &Other)
			{
				return *this;
			}

			const int32 OldStride = Stride;

			StreamKey = Other.StreamKey;
			Layout = Other.Layout;
			CacheStrides();

			UnLink();

			// If the stride changed, ResizeAllocation could early-out when the row count
			// matches ArrayMax, leaving an allocation sized for the old (potentially smaller)
			// stride. Reset ArrayNum first and force a reallocation at the new stride.
			ArrayNum = 0;
			if (Stride != OldStride)
			{
				Allocator.ResizeAllocation(0, Other.Num(), Stride, Alignment);
				ArrayMax = Other.Num();
				BroadcastAllocatedSizeChanged();
			}
			else
			{
				ResizeAllocation(Other.Num(), false);
			}
			ArrayNum = Other.Num();

			FMemory::Memcpy(Allocator.GetAllocation(), Other.Allocator.GetAllocation(), Other.Num() * GetStride());

			return *this;
		}

		FRealtimeMeshStream& operator=(FRealtimeMeshStream&& Other) noexcept
		{
			UnLink();
			Other.UnLink();
			
			StreamKey = MoveTemp(Other.StreamKey);
			Layout = MoveTemp(Other.Layout);		
			CacheStrides();
			
			ArrayNum = Other.ArrayNum;
			ArrayMax = Other.ArrayMax;
			Allocator.MoveToEmpty(Other.Allocator);

			Other.ArrayNum = 0;
			Other.ArrayMax = Other.Allocator.GetInitialCapacity();

			return *this;
		}


	public:
		FName GetName() const { return StreamKey.GetName(); }

		ERealtimeMeshStreamType GetStreamType() const { return StreamKey.GetStreamType(); }

		const FRealtimeMeshStreamKey& GetStreamKey() const { return StreamKey; }

		const FRealtimeMeshBufferLayout& GetLayout() const { return Layout; }

		const FRealtimeMeshElementType& GetElementType() const { return Layout.GetElementType(); }


		void SetStreamKey(const FRealtimeMeshStreamKey& InStreamKey) { StreamKey = InStreamKey; }

		// Bytes per row (ElementStride * NumElements).
		int32 GetStride() const { return Stride; }

		// Bytes of a single element within a row.
		int32 GetElementStride() const { return ElementStride; }

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
				// Empty stream can just change types. Any retained slack was sized for the
				// old stride, so drop it to avoid ArrayMax overstating capacity at the new stride.
				if (ArrayMax != 0)
				{
					Allocator.ResizeAllocation(0, 0, Stride, Alignment);
					ArrayMax = 0;
					BroadcastAllocatedSizeChanged();
				}
				Layout = NewLayout;
				CacheStrides();
				return true;
			}

			if (FRealtimeMeshTypeConversionUtilities::CanConvert(FromType, ToType) && bSameNumElements)
			{
				// Number of rows to convert must be captured before we drop the old allocation.
				const SizeType NumRows = ArrayNum;

				// Move existing data to temp allocator
				ElementAllocatorType OldData;
				OldData.MoveToEmpty(Allocator);

				// Update the layout/strides FIRST so Stride reflects the destination type, then
				// allocate the new buffer at the new stride sized to the live rows. Allocating
				// with the old stride (or against stale ArrayMax) would heap-overflow when
				// converting to a wider type.
				Layout = NewLayout;
				CacheStrides();

				Allocator.ResizeAllocation(0, NumRows, Stride, Alignment);
				ArrayMax = NumRows;
				BroadcastAllocatedSizeChanged();

				// Now convert data from the temp array into the new allocation
				const SIZE_T ElementCount = NumRows * GetNumElements();
				FRealtimeMeshTypeConversionUtilities::ProcessTypeConverter(FromType, ToType,
					[&](const FRealtimeMeshElementConverters& Converter)
					{
						Converter.ConvertContiguousArray(OldData.GetAllocation(), Allocator.GetAllocation(), ElementCount);
					});
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
			ElementCheck(ElementIndex);

			// Compute the element base without RangeCheck(0) so an empty stream yields an empty view.
			DataType* Base = reinterpret_cast<DataType*>(GetData() + GetElementStride() * ElementIndex);
			return MakeStridedView(GetStride(), Base, Num());
		}

		template <typename DataType>
		TStridedView<const DataType> GetElementArrayView(int32 ElementIndex) const
		{
			check(sizeof(DataType) == GetElementStride());
			check(GetRealtimeMeshDataElementType<DataType>() == GetLayout().GetElementType());
			ElementCheck(ElementIndex);

			// Const-correct: read through GetData() const, no const_cast, empty view when empty.
			const DataType* Base = reinterpret_cast<const DataType*>(GetData() + GetElementStride() * ElementIndex);
			return MakeStridedView(GetStride(), Base, Num());
		}

		template <typename DataType>
		TArrayView<DataType> GetElementArrayView()
		{
			check(sizeof(DataType) == GetElementStride());
			check(GetRealtimeMeshDataElementType<DataType>() == GetLayout().GetElementType());

			return MakeArrayView(reinterpret_cast<DataType*>(GetData()), Num() * GetNumElements());
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

		// IRealtimeMeshStream — Num/GetStride/GetLayout/GetStreamKey already match
		// the interface signatures and become virtual overrides implicitly via
		// inheritance.
		virtual bool IsCPUBacked() const override { return true; }
		virtual FResourceArrayInterface* GetCPUResourceArray() override { return this; }
		virtual const FResourceArrayInterface* GetCPUResourceArray() const override { return this; }

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

				// When linked, a per-stream removal must be applied to every member
				// atomically: memmove all streams, decrement each ArrayNum, then shrink
				// the whole pool. Broadcasting the shrink/num-change from here (as the
				// unlinked fast path does) would resize siblings below their live row
				// count and only shift the tail of this one stream, misaligning rows
				// across the pool. Route through the batch path which does this right.
				if (Linkage)
				{
					Linkage->RemoveRows(Index, Count, bAllowShrinking);
					return;
				}

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
					ResizeAllocationShrink(ArrayNum - Count);
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
			if (Num <= 0)
			{
				return;
			}

			const uint8* SrcRow = Value.GetDataPtr();
			const int32 RowStride = GetStride();

			// Write the first row, then replicate it across the remainder by doubling.
			uint8* const Base = GetDataRawAtVertex(StartIndex);
			FMemory::Memcpy(Base, SrcRow, RowStride);
			ReplicateFirstRow(StartIndex, Num, Base, RowStride);
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
						
			FRealtimeMeshTypeConversionUtilities::ProcessTypeConverter(FromType, ToType,
				[&](const FRealtimeMeshElementConverters& Converter)
				{
					// Convert the first row; the remaining rows are replicated below.
					ConvertElementRow(Converter, &Value, GetDataRawAtVertex(StartIndex), GetLayout().GetNumElements());
				});

			// Replicate the converted first row across the remainder by doubling.
			if (Num > 1)
			{
				const int32 RowStride = GetStride();
				uint8* const Base = GetDataRawAtVertex(StartIndex);
				ReplicateFirstRow(StartIndex, Num, Base, RowStride);
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

			// If this is empty and the layouts match exactly, we can steal the source's
			// payload (allocation + counts) directly. We must preserve THIS stream's identity
			// (StreamKey), layout and linkage — only the underlying data is moved. When the
			// layouts differ we fall back to the converting append path so layout-conversion
			// semantics are honored.
			if (Num() == 0 && Source.GetLayout() == GetLayout())
			{
				// The source is being emptied/consumed here, so detach it from any linkage
				// first — otherwise its linkage siblings would still believe it holds the old
				// row count after we steal its payload below, desyncing it. UnLink() is a
				// no-op when the source isn't linked. (THIS stream's linkage is untouched.)
				Source.UnLink();

				// Steal the source's allocation and counts without touching StreamKey/Layout/linkage.
				ArrayNum = Source.ArrayNum;
				ArrayMax = Source.ArrayMax;
				Allocator.MoveToEmpty(Source.Allocator);

				Source.ArrayNum = 0;
				Source.ArrayMax = Source.Allocator.GetInitialCapacity();

				BroadcastAllocatedSizeChanged();
				BroadcastNumChanged();
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
			Append(GetRealtimeMeshBufferLayout<SourceType>(), reinterpret_cast<const uint8*>(NewElements.GetData()), NewElements.Num());
		}

		template <typename VertexType, typename InAllocatorType = FDefaultAllocator>
		void Append(const TArray<VertexType, InAllocatorType>& NewElements)
		{
			Append(GetRealtimeMeshBufferLayout<VertexType>(), reinterpret_cast<const uint8*>(NewElements.GetData()), NewElements.Num());
		}

		template <typename VertexType>
		void Append(std::initializer_list<VertexType> NewElements)
		{
			Append(GetRealtimeMeshBufferLayout<VertexType>(), reinterpret_cast<const uint8*>(NewElements.begin()), NewElements.size());
		}

		template <typename VertexType>
		void Append(int32 Count, const VertexType* NewElements)
		{
			Append(GetRealtimeMeshBufferLayout<VertexType>(), reinterpret_cast<const uint8*>(NewElements), Count);
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
			if (SourceCount > 0)
			{
				RangeCheck(DestinationIndex + SourceCount - 1);
			}
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
						
			FRealtimeMeshTypeConversionUtilities::ProcessTypeConverter(FromType, ToType,
				[&](const FRealtimeMeshElementConverters& Converter)
				{
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
				});
		}

		
		template <typename ElementType, typename GeneratorFunc>
		void SetGeneratedElement(int32 ElementIndex, int32 StartIndex, int32 Count, GeneratorFunc Generator)
		{
			check(GetRealtimeMeshDataElementType<ElementType>() == GetLayout().GetElementType());
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
		void CopyRange(int32 StartIndex, TArrayView<VertexType> OutputElements) const
		{
			CopyRange(StartIndex, GetRealtimeMeshBufferLayout<VertexType>(), OutputElements.GetData(), OutputElements.Num());
		}
		
		template <typename VertexType>
		void CopyRange(int32 StartIndex, int32 Count, TArray<VertexType>& OutputElements) const
		{
			const SizeType DestinationIndex = OutputElements.AddUninitialized(Count);
			CopyRange(StartIndex, GetRealtimeMeshBufferLayout<VertexType>(), reinterpret_cast<uint8*>(&OutputElements[DestinationIndex]), Count);
		}
		
		template <typename VertexType>
		void CopyTo(TArray<VertexType>& OutElements) const
		{
			const SizeType DestinationIndex = OutElements.AddUninitialized(ArrayNum);
			CopyRange(0, GetRealtimeMeshBufferLayout<VertexType>(), reinterpret_cast<uint8*>(&OutElements[DestinationIndex]), ArrayNum);
		}


		

		

		void CountBytes(FArchive& Ar) const
		{
			Ar.CountBytes(ArrayNum * GetStride(), ArrayMax * GetStride());
		}

		friend REALTIMEMESHCOMPONENT_API FArchive& operator<<(FArchive& Ar, FRealtimeMeshStream& Stream);

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
				UE_LOG(LogRealtimeMeshInterface, Fatal, TEXT("Invalid value for %s, must not be negative..."), ParameterName);
			}
		}

		// With the first row already written at FirstRow, fill rows [StartIndex+1, StartIndex+Num)
		// by repeatedly doubling the already-filled block. This turns Num single-row Memcpy calls into
		// ~log2(Num) growing block copies. (No-op when Num <= 1.)
		void ReplicateFirstRow(int32 StartIndex, int32 Num, const uint8* FirstRow, int32 RowStride)
		{
			for (int32 Filled = 1; Filled < Num; )
			{
				const int32 CopyCount = FMath::Min(Filled, Num - Filled);
				FMemory::Memcpy(GetDataRawAtVertex(StartIndex + Filled), FirstRow, static_cast<size_t>(CopyCount) * RowStride);
				Filled += CopyCount;
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
			
			// The remaining options all have to do data conversion, so grab the converter for this type pair.
			// The conversion runs inside ProcessTypeConverter so the registry read lock is held across all uses.
			FRealtimeMeshTypeConversionUtilities::ProcessTypeConverter(FromType, ToType,
				[&](const FRealtimeMeshElementConverters& Converter)
				{
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
				});
		}

		void CopyRange(uint32 SourceIndex, const FRealtimeMeshBufferLayout& DestinationLayout, uint8* DestinationData, uint32 DestinationCount) const
		{
			if (DestinationCount > 0)
			{
				RangeCheck(SourceIndex + DestinationCount - 1);
			}
			CopyStreamDataIntoStream(DestinationLayout, DestinationData, 0, Layout, GetDataRawAtVertex(SourceIndex), DestinationCount);
		}

		friend struct FRealtimeMeshStreamLinkage;
	};


	struct REALTIMEMESHCOMPONENT_API FRealtimeMeshStreamSet
	{
	private:
		TMap<FRealtimeMeshStreamKey, TUniquePtr<FRealtimeMeshStream>> Streams;

	public:
		FRealtimeMeshStreamSet() = default;
		// Non-virtual dtor: nothing derives from this struct (the URealtimeMeshStreamSet
		// UObject is unrelated) and it is never deleted through a base pointer, so the
		// vtable would be unearned.
		~FRealtimeMeshStreamSet() = default;

		
		// We don't allow implicit copying as it leads to unnecessary stream copies.
		// bIncludeLinkages is preserved for source-level back-compat with prior
		// versions but is now a no-op: stream linkage state lives on standalone
		// FRealtimeMeshStreamLinkage instances owned by the caller (typically the
		// mesh builder), not on the StreamSet, so there is nothing to clone here.
		explicit FRealtimeMeshStreamSet(const FRealtimeMeshStreamSet& Other, bool bIncludeLinkages = false, const TSet<FRealtimeMeshStreamKey>& DesiredStreams = TSet<FRealtimeMeshStreamKey>())
		{
			CopyFrom(Other, bIncludeLinkages, DesiredStreams);
		}
		FRealtimeMeshStreamSet(FRealtimeMeshStreamSet&&) = default;

		// We don't allow implicit copying as it leads to unnecessary stream copies
		// To copy a stream set use CopyFrom or the explicit copy constructor
		FRealtimeMeshStreamSet& operator=(const FRealtimeMeshStreamSet&) = delete;

		// We do allow move operation
		FRealtimeMeshStreamSet& operator=(FRealtimeMeshStreamSet&&) = default;

		// bIncludeLinkages is retained for source-level back-compat but ignored —
		// linkage is no longer owned by the StreamSet.
		void CopyFrom(const FRealtimeMeshStreamSet& Other, bool /*bIncludeLinkages*/ = false, const TSet<FRealtimeMeshStreamKey>& DesiredStreams = TSet<FRealtimeMeshStreamKey>())
		{
			Streams.Empty(Other.Streams.Num());
			for (auto SetIt = Other.Streams.CreateConstIterator(); SetIt; ++SetIt)
			{
				if (DesiredStreams.IsEmpty() || DesiredStreams.Contains(SetIt.Value()->GetStreamKey()))
				{
					Streams.Emplace(SetIt.Value()->GetStreamKey(), MakeUnique<FRealtimeMeshStream>(*SetIt->Value));
				}
			}
		}

		int32 Num() const { return Streams.Num(); }
		void Empty() { Streams.Empty(); }
		bool IsEmpty() const { return Streams.IsEmpty(); }

		int32 Remove(const FRealtimeMeshStreamKey& StreamKey)
		{
			// Streams self-unlink from any FRealtimeMeshStreamLinkage they belong to
			// in their destructor. We just need to release ownership here.
			return Streams.Remove(StreamKey);
		}

		int32 RemoveAll(const TSet<FRealtimeMeshStreamKey>& StreamKeys)
		{
			int32 RemovedCount = 0;
			for (const FRealtimeMeshStreamKey& StreamKey : StreamKeys)
			{
				RemovedCount += Streams.Remove(StreamKey);
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
					// We do not remove/replace the stream here - the existing stream
					// (still in its previous layout) is returned as-is so the caller's
					// data is preserved.
					UE_LOG(LogRealtimeMeshInterface, Warning, TEXT("Failed to convert stream %s to requested layout; keeping existing data in its current layout"), *StreamKey.ToString());
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


		friend REALTIMEMESHCOMPONENT_API FArchive& operator<<(FArchive& Ar, FRealtimeMeshStreamSet& StreamSet);
	};

	using FRealtimeMeshStreamProxyMap = TMap<FRealtimeMeshStreamKey, TSharedPtr<FRealtimeMeshGPUBuffer>>;


}
