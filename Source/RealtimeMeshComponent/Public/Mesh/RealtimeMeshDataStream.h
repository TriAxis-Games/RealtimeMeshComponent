// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "RealtimeMeshComponentModule.h"
#include "RealtimeMeshConfig.h"
#include "RealtimeMeshDataTypes.h"
#include "RealtimeMeshDataConversion.h"

#if RMC_ENGINE_BELOW_5_1
// Included for TMakeUnsigned in 5.0
#include "Containers/RingBuffer.h"
#endif








namespace RealtimeMesh
{
	
	struct FRealtimeMeshStreams
	{
		inline static const FName PositionStreamName = FName(TEXT("Position"));
		inline static const FName TangentsStreamName = FName(TEXT("Tangents"));
		inline static const FName TexCoordsStreamName = FName(TEXT("TexCoords"));
		inline static const FName ColorStreamName = FName(TEXT("Color"));

		/*
		inline static const FName NormalElementName = FName("Normal");
		inline static const FName TangentElementName = FName("Tangent");*/

		/*inline static const FName TexCoord0ElementName = FName("TexCoord", 0);
		inline static const FName TexCoord1ElementName = FName("TexCoord", 1);
		inline static const FName TexCoord2ElementName = FName("TexCoord", 2);
		inline static const FName TexCoord3ElementName = FName("TexCoord", 3);
		inline static const FName TexCoord4ElementName = FName("TexCoord", 4);
		inline static const FName TexCoord5ElementName = FName("TexCoord", 5);
		inline static const FName TexCoord6ElementName = FName("TexCoord", 6);
		inline static const FName TexCoord7ElementName = FName("TexCoord", 7);*/

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
	
	struct REALTIMEMESHCOMPONENT_API FRealtimeMeshStream : FResourceArrayInterface
	{
		using AllocatorType = TSizedHeapAllocator<32>;
		using SizeType = AllocatorType::SizeType;

	private:
		using ElementAllocatorType = AllocatorType::ForAnyElementType;
		using USizeType = TMakeUnsigned<SizeType>::Type;

		FRealtimeMeshBufferLayoutDefinition LayoutDefinition;

		ElementAllocatorType Allocator;
		SizeType ArrayNum;
		SizeType ArrayMax;
		FRealtimeMeshStreamKey StreamKey;

	public:
		FRealtimeMeshStream()
			: LayoutDefinition(FRealtimeMeshBufferLayoutUtilities::GetBufferLayoutDefinition(FRealtimeMeshBufferLayout::Invalid))
			  , ArrayNum(0)
			  , ArrayMax(Allocator.GetInitialCapacity())
			  , StreamKey(ERealtimeMeshStreamType::Unknown, NAME_None)
		{
		}
		
		FRealtimeMeshStream(const FRealtimeMeshStreamKey& InStreamKey, const FRealtimeMeshBufferLayout& Layout)
			: LayoutDefinition(FRealtimeMeshBufferLayoutUtilities::GetBufferLayoutDefinition(Layout))
			  , ArrayNum(0)
			  , ArrayMax(Allocator.GetInitialCapacity())
			  , StreamKey(InStreamKey)
		{
		}

		explicit FRealtimeMeshStream(const FRealtimeMeshStream& Other) noexcept
			: LayoutDefinition(Other.LayoutDefinition)
			  , ArrayNum(0)
			  , ArrayMax(Allocator.GetInitialCapacity())
			  , StreamKey(Other.StreamKey)
		{
			ResizeAllocation(Other.Num());
			ArrayNum = Other.Num();
			FMemory::Memcpy(Allocator.GetAllocation(), Other.Allocator.GetAllocation(), Other.Num() * GetStride());
		}
		
		FRealtimeMeshStream(FRealtimeMeshStream&& Other) noexcept
			: LayoutDefinition(Other.LayoutDefinition)
			  , ArrayNum(Other.ArrayNum)
			  , ArrayMax(Other.ArrayMax)
			  , StreamKey(Other.StreamKey)
		{
			Allocator.MoveToEmpty(Other.Allocator);

			Other.LayoutDefinition = FRealtimeMeshBufferLayoutUtilities::GetBufferLayoutDefinition(FRealtimeMeshBufferLayout::Invalid);
			Other.ArrayNum = 0;
			Other.ArrayMax = 0;
			Other.StreamKey = FRealtimeMeshStreamKey(ERealtimeMeshStreamType::Unknown, NAME_None);
		}

		template<typename StreamType>
		static FRealtimeMeshStream Create(const FRealtimeMeshStreamKey& InStreamKey)
		{
			return FRealtimeMeshStream(InStreamKey, GetRealtimeMeshBufferLayout<StreamType>());
		}

		FRealtimeMeshStream& operator=(const FRealtimeMeshStream& Other)
		{
			LayoutDefinition = Other.LayoutDefinition;
			ResizeAllocation(Other.Num(), false);			
			ArrayNum = Other.Num();
			FMemory::Memcpy(Allocator.GetAllocation(), Other.Allocator.GetAllocation(), Other.Num() * GetStride());
			return *this;
		}

		FRealtimeMeshStream& operator=(FRealtimeMeshStream& Other)
		{
			LayoutDefinition = MoveTemp(Other.LayoutDefinition);
			ArrayNum = Other.ArrayNum;
			Other.ArrayNum = 0;
			Allocator.MoveToEmpty(Other.Allocator);
			return *this;
		}
		

	public:
		const FRealtimeMeshStreamKey& GetStreamKey() const { return StreamKey; }		
		void SetStreamKey(const FRealtimeMeshStreamKey& InStreamKey) { StreamKey = InStreamKey; }
		ERealtimeMeshStreamType GetStreamType() const { return StreamKey.GetStreamType(); }
		FName GetName() const { return StreamKey.GetName(); }
		const FRealtimeMeshBufferLayout& GetLayout() const { return LayoutDefinition.GetBufferLayout(); }
		const FRealtimeMeshBufferLayoutDefinition& GetLayoutDefinition() const { return LayoutDefinition; }
		const FRealtimeMeshElementType& GetElementType() const { return LayoutDefinition.GetElementType(); }
		const FRealtimeMeshElementTypeDefinition& GetElementTypeDefinition() const { return LayoutDefinition.GetElementTypeDefinition(); }

		int32 GetStride() const { return LayoutDefinition.GetStride(); }
		int32 GetElementStride() const { return LayoutDefinition.GetElementTypeDefinition().GetStride(); }
		int32 GetNumElements() const { return LayoutDefinition.GetBufferLayout().GetNumElements(); }
		/*int32 GetElementOffset(FName ElementName) const { return LayoutDefinition.GetElementOffset(ElementName); }
		const TMap<FName, uint8>& GetElementOffsets() const { return LayoutDefinition.GetElementOffsets(); }*/

		
		template<typename NewDataType>
		bool IsOfType() const
		{
			return GetLayout() == GetRealtimeMeshBufferLayout<NewDataType>();
		}

		bool ConvertTo(const FRealtimeMeshBufferLayout& NewLayout)
		{
			if (LayoutDefinition.GetBufferLayout() == NewLayout)
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
				LayoutDefinition = FRealtimeMeshBufferLayoutUtilities::GetBufferLayoutDefinition(NewLayout);
				return true;
			}
			else if (FRealtimeMeshTypeConversionUtilities::CanConvert(FromType, ToType) && bSameNumElements)
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
				LayoutDefinition = FRealtimeMeshBufferLayoutUtilities::GetBufferLayoutDefinition(NewLayout);
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

		/*FRealtimeMeshStreamRawData GetResourceCopy() const
		{
			FRealtimeMeshStreamRawData OutData;
			CopyToArray(OutData);
			return OutData;
		}

		FRealtimeMeshSectionGroupStreamUpdateDataRef GetStreamUpdateData(const FRealtimeMeshStreamKey& InStreamKey) const
		{
			auto InitData = MakeUnique<FRealtimeMeshStreamRawData>();
			CopyToArray(*InitData);			
			return MakeShared<FRealtimeMeshSectionGroupStreamUpdateData>(MoveTemp(InitData), LayoutDefinition, InStreamKey);
		}*/

		virtual const void* GetResourceData() const override { return Allocator.GetAllocation(); }
		virtual uint32 GetResourceDataSize() const override { return Num() * GetStride(); }

		virtual void Discard() override
		{
		}

		virtual bool IsStatic() const override { return false; }
		virtual bool GetAllowCPUAccess() const override { return false; }

		virtual void SetAllowCPUAccess(bool bInNeedsCPUAccess) override
		{
		}

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
			}
		}


		template <typename VertexType>
		void Add(const VertexType& InVertex)
		{
			check(LayoutDefinition.GetBufferLayout() == GetRealtimeMeshBufferLayout<VertexType>());
			const SizeType Index = AddUninitialized();
			*GetDataAtVertex<VertexType>(Index) = InVertex;
		}

		void Append(const FRealtimeMeshStream& Source)
		{
			check(this != &Source);
			check(LayoutDefinition.GetBufferLayout() == Source.LayoutDefinition.GetBufferLayout());

			const SizeType Count = Source.Num();
			const SizeType Index = AddUninitialized(Count);
			FMemory::Memcpy(GetDataRawAtVertex(Index), Source.Allocator.GetAllocation(), Count * GetStride());
		}

		void Append(FRealtimeMeshStream&& Source)
		{
			CheckInvariants();
			check(this != &Source);
			check(LayoutDefinition.GetBufferLayout() == Source.LayoutDefinition.GetBufferLayout());

			if (ArrayNum == 0)
			{
				Allocator.MoveToEmpty(Source.Allocator);
				ArrayMax = MoveTemp(Source.ArrayMax);
				ArrayNum = MoveTemp(Source.ArrayNum);
			}
			else
			{
				Append(Source);
				Source.Empty(0, 0);
			}
		}

		template <typename VertexType>
		FORCENOINLINE void Append(TArrayView<VertexType> NewElements)
		{
			check(LayoutDefinition.GetBufferLayout() == GetRealtimeMeshBufferLayout<VertexType>());

			const SizeType Count = NewElements.Num();
			const SizeType Index = AddUninitialized(Count);
			FMemory::Memcpy(GetDataRawAtVertex(Index), NewElements.GetData(), Count * GetStride());
		}

		template <typename VertexType, typename InAllocatorType = FDefaultAllocator>
		void Append(const TArray<VertexType, InAllocatorType>& NewElements)
		{
			check(LayoutDefinition.GetBufferLayout() == GetRealtimeMeshBufferLayout<VertexType>());

			const SizeType Count = NewElements.Num();
			const SizeType Index = AddUninitialized(Count);
			FMemory::Memcpy(GetDataRawAtVertex(Index), NewElements.GetData(), Count * GetStride());
		}

		template <typename VertexType>
		void Append(std::initializer_list<VertexType> NewElements)
		{
			Append(MakeArrayView(NewElements.begin(), NewElements.size()));
		}

		template <typename VertexType>
		void Append(int32 Count, VertexType* NewElements)
		{
			Append(MakeArrayView(NewElements, Count));
		}

		template <typename VertexType, typename GeneratorFunc>
		void AppendGenerated(int32 Count, GeneratorFunc Generator)
		{
			SizeType Index = AddUninitialized(Count);
			VertexType* DataPtr = GetDataAtVertex<VertexType>(Index);

			while (Index < ArrayNum)
			{
				DataPtr[Index] = Generator(Index);
				Index++;
			}
		}

		template <typename VertexType>
		FORCENOINLINE void SetRange(int32 StartIndex, TArrayView<VertexType> NewElements)
		{
			RangeCheck(StartIndex + NewElements.Num() - 1);
			check(LayoutDefinition.GetBufferLayout() == GetRealtimeMeshBufferLayout<VertexType>());

			FMemory::Memcpy(GetDataRawAtVertex(StartIndex), NewElements.GetData(), NewElements.Num() * GetStride());
		}

		template <typename VertexType, typename InAllocatorType = FDefaultAllocator>
		void SetRange(int32 StartIndex, const TArray<VertexType, InAllocatorType>& NewElements)
		{
			RangeCheck(StartIndex + NewElements.Num() - 1);
			check(LayoutDefinition.GetBufferLayout() == GetRealtimeMeshBufferLayout<VertexType>());
			FMemory::Memcpy(GetDataRawAtVertex(StartIndex), NewElements.GetData(), NewElements.Num() * GetStride());
		}

		template <typename VertexType>
		void SetRange(int32 StartIndex, std::initializer_list<VertexType> NewElements)
		{
			SetRange(StartIndex, MakeArrayView(NewElements.begin(), NewElements.size()));
		}

		template <typename VertexType>
		void SetRange(int32 StartIndex, int32 Count, VertexType* NewElements)
		{
			SetRange(StartIndex, MakeArrayView(NewElements, Count));
		}

		template <typename VertexType, typename GeneratorFunc>
		void SetGenerated(int32 StartIndex, int32 Count, GeneratorFunc Generator)
		{
			RangeCheck(StartIndex + Count - 1);
			VertexType* DataPtr = GetDataAtVertex<VertexType>(StartIndex);

			while (StartIndex < ArrayNum)
			{
				DataPtr[StartIndex] = Generator(StartIndex);
				StartIndex++;
			}
		}

		template <typename ElementType, typename GeneratorFunc>
		void SetGeneratedElement(int32 ElementIndex, int32 StartIndex, int32 Count, GeneratorFunc Generator)
		{
			RangeCheck(StartIndex + Count - 1);
			ElementCheck(ElementIndex);
			
			ElementType* DataPtr = GetDataAtVertex<ElementType>(StartIndex, ElementIndex);

			while (StartIndex < ArrayNum)
			{				
				*DataPtr = Generator(StartIndex);
				DataPtr += GetNumElements();
				StartIndex++;
			}
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

			FRealtimeMeshBufferLayout BufferLayout = Stream.LayoutDefinition.GetBufferLayout();
			Ar << BufferLayout;
			if (Ar.IsLoading())
			{
				Stream.LayoutDefinition = FRealtimeMeshBufferLayoutDefinition(FRealtimeMeshBufferLayoutUtilities::GetBufferLayoutDefinition(BufferLayout));

				if (Ar.CustomVer(FRealtimeMeshVersion::GUID) < FRealtimeMeshVersion::StreamsNowHoldEntireKey)
				{
					ERealtimeMeshStreamType StreamType =
					(BufferLayout == GetRealtimeMeshBufferLayout<uint32>() ||
						BufferLayout == GetRealtimeMeshBufferLayout<int32>() ||
						BufferLayout == GetRealtimeMeshBufferLayout<uint16>())
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
			}
			else if (Ar.IsLoading())
			{
				Stream.Empty();
			}
			return Ar;
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

		void ResizeAllocation(USizeType NewNum, bool bKeepElements = true)
		{
			if (NewNum != ArrayMax)
			{
				Allocator.ResizeAllocation(bKeepElements? ArrayNum : 0, NewNum,
										   LayoutDefinition.GetStride(), LayoutDefinition.GetElementTypeDefinition().GetAlignment());
				ArrayMax = NewNum;
			}
		}

		void ResizeAllocationGrow(USizeType NewMinNum)
		{
			const SizeType NewAllocationSize = Allocator.CalculateSlackGrow(NewMinNum, ArrayMax,
																			LayoutDefinition.GetStride(), LayoutDefinition.GetAlignment());

			if (NewAllocationSize != ArrayMax)
			{
				Allocator.ResizeAllocation(ArrayNum, NewAllocationSize,
										   LayoutDefinition.GetStride(), LayoutDefinition.GetAlignment());
				ArrayMax = NewAllocationSize;
			}
		}

		void ResizeAllocationShrink(USizeType NewNum)
		{
			const SizeType NewAllocationSize = Allocator.CalculateSlackShrink(NewNum, ArrayMax,
																			  LayoutDefinition.GetStride(), LayoutDefinition.GetElementTypeDefinition().GetAlignment());

			if (NewAllocationSize != ArrayMax)
			{
				Allocator.ResizeAllocation(ArrayNum, NewAllocationSize,
										   LayoutDefinition.GetStride(), LayoutDefinition.GetElementTypeDefinition().GetAlignment());
				ArrayMax = NewAllocationSize;
			}
		}


		FORCEINLINE static void CheckNotNegative(SizeType InValue, const TCHAR* ParameterName)
		{
			if (InValue < 0)
			{
				UE_LOG(LogCore, Fatal, TEXT("Invalid value for %s, must not be negative..."), ParameterName);
			}
		}
	};	

	struct REALTIMEMESHCOMPONENT_API FRealtimeMeshStreamSet
	{
	private:
		TMap<FRealtimeMeshStreamKey, TUniquePtr<FRealtimeMeshStream>> Streams;

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
		}

		int32 Num() const { return Streams.Num(); }
		void Empty() { Streams.Empty(); }
		bool IsEmpty() const { return Streams.IsEmpty(); }

		int32 Remove(const FRealtimeMeshStreamKey& StreamKey)
		{
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
		FRealtimeMeshStream* FindOrAdd(const FRealtimeMeshStreamKey& StreamKey, const FRealtimeMeshBufferLayout& NewLayout, bool bKeepData = true)
		{
			if (auto* Result = Streams.Find(StreamKey))
			{
				if (!bKeepData)
				{
					(*Result)->Empty();
				}
				if (!(*Result)->ConvertTo(NewLayout))
				{
					UE_LOG(RealtimeMeshLog, Warning, TEXT("Failed to convert stream %s to new layout: Removing Data"), *StreamKey.ToString());
				}
				return Result->Get();
			}
			
			auto& Entry = Streams.FindOrAdd(StreamKey);
			Entry = MakeUnique<FRealtimeMeshStream>(StreamKey, NewLayout);
			return Entry.Get();
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

		FRealtimeMeshStream* AddStream(ERealtimeMeshStreamType StreamType, FName StreamName, const FRealtimeMeshBufferLayout& InLayout)
		{
			const FRealtimeMeshStreamKey StreamKey(StreamType, StreamName);
			auto& Entry = Streams.FindOrAdd(StreamKey);
			Entry = MakeUnique<FRealtimeMeshStream>(StreamKey, InLayout);
			return Entry.Get();
		}
		
		FRealtimeMeshStream* AddStream(const FRealtimeMeshStreamKey& StreamKey, const FRealtimeMeshBufferLayout& InLayout)
		{
			auto& Entry = Streams.FindOrAdd(StreamKey);
			Entry = MakeUnique<FRealtimeMeshStream>(StreamKey, InLayout);
			return Entry.Get();
		}

		template <typename StreamLayout>
		FRealtimeMeshStream* AddStream(ERealtimeMeshStreamType StreamType, FName StreamName)
		{
			const FRealtimeMeshStreamKey StreamKey(StreamType, StreamName);
			auto& Entry = Streams.FindOrAdd(StreamKey);
			Entry = MakeUnique<FRealtimeMeshStream>(StreamKey, GetRealtimeMeshBufferLayout<StreamLayout>());
			return Entry.Get();
		}

		template <typename StreamLayout>
		FRealtimeMeshStream* AddStream(const FRealtimeMeshStreamKey& StreamKey)
		{
			auto& Entry = Streams.FindOrAdd(StreamKey);
			Entry = MakeUnique<FRealtimeMeshStream>(StreamKey, GetRealtimeMeshBufferLayout<StreamLayout>());
			return Entry.Get();
		}

		FRealtimeMeshStream* AddStream(const FRealtimeMeshStream& Stream)
		{
			auto& Entry = Streams.FindOrAdd(Stream.GetStreamKey());
			Entry = MakeUnique<FRealtimeMeshStream>(Stream);
			return Entry.Get();
		}

		FRealtimeMeshStream* AddStream(FRealtimeMeshStream&& Stream)
		{
			auto& Entry = Streams.FindOrAdd(Stream.GetStreamKey());
			Entry = MakeUnique<FRealtimeMeshStream>(MoveTemp(Stream));
			return Entry.Get();
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
	};



	using FRealtimeMeshStreamProxyMap = TMap<FRealtimeMeshStreamKey, TSharedPtr<FRealtimeMeshGPUBuffer>>;
}
