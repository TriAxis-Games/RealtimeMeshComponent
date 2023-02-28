// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "RealtimeMeshDataTypes.h"
#include "RenderProxy/RealtimeMeshGPUBuffer.h"
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION == 0
// Included for TMakeUnsigned in 5.0
#include "Containers/RingBuffer.h"
#endif

namespace RealtimeMesh
{
	struct FRealtimeMeshDataStream;
	using FRealtimeMeshDataStreamRef = TSharedRef<FRealtimeMeshDataStream>;
	using FRealtimeMeshDataStreamPtr = TSharedPtr<FRealtimeMeshDataStream>;
	using FRealtimeMeshDataStreamWeakPtr = TWeakPtr<FRealtimeMeshDataStream>;
	using FRealtimeMeshDataStreamConstRef = TSharedRef<const FRealtimeMeshDataStream>;
	using FRealtimeMeshDataStreamConstPtr = TSharedPtr<const FRealtimeMeshDataStream>;
	using FRealtimeMeshDataStreamConstWeakPtr = TWeakPtr<const FRealtimeMeshDataStream>;
	
	class REALTIMEMESHCOMPONENT_API FRealtimeMeshDataStreamRawData : public FResourceArrayInterface, public TArray<uint8>
	{	
	public:	
		FRealtimeMeshDataStreamRawData() = default;
		FRealtimeMeshDataStreamRawData(FRealtimeMeshDataStreamRawData&&) = default;
		FRealtimeMeshDataStreamRawData(const FRealtimeMeshDataStreamRawData&) = default;
		FRealtimeMeshDataStreamRawData& operator=(FRealtimeMeshDataStreamRawData&&) = default;
		FRealtimeMeshDataStreamRawData& operator=(const FRealtimeMeshDataStreamRawData&) = default;
		
		virtual ~FRealtimeMeshDataStreamRawData() = default;
		virtual const void* GetResourceData() const { return &(*this)[0]; }	
		virtual uint32 GetResourceDataSize() const { return this->Num() * sizeof(uint8); }	
		virtual void Discard() { }
		virtual bool IsStatic() const { return false; }	
		virtual bool GetAllowCPUAccess() const { return false; }	
		virtual void SetAllowCPUAccess(bool bInNeedsCPUAccess) {  }
	};

	struct REALTIMEMESHCOMPONENT_API FRealtimeMeshDataStream : TSharedFromThis<FRealtimeMeshDataStream, ESPMode::ThreadSafe>, public FResourceArrayInterface
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
		FName StreamName;
	public:
		FRealtimeMeshDataStream()
			: LayoutDefinition(FRealtimeMeshBufferLayoutUtilities::GetBufferLayoutDefinition(FRealtimeMeshBufferLayout::Invalid))
			, ArrayNum(0)
			, ArrayMax(Allocator.GetInitialCapacity())
			, StreamName(NAME_None)
		{
		}
		FRealtimeMeshDataStream(FName InStreamName, FRealtimeMeshBufferLayout InLayout)
			: LayoutDefinition(FRealtimeMeshBufferLayoutUtilities::GetBufferLayoutDefinition(InLayout))
			, ArrayNum(0)
			, ArrayMax(Allocator.GetInitialCapacity())
			, StreamName(InStreamName)
		{
		}

		FName GetName() const { return StreamName; }
		const FRealtimeMeshBufferLayout& GetLayout() const { return LayoutDefinition.GetBufferLayout(); }
		const FRealtimeMeshBufferLayoutDefinition& GetLayoutDefinition() const { return LayoutDefinition; }
		const FRealtimeMeshElementType& GetElementType() const { return LayoutDefinition.GetElementType(); }
		const FRealtimeMeshElementTypeDefinition& GetElementTypeDefinition() const { return LayoutDefinition.GetElementTypeDefinition(); }

		int32 GetStride() const { return LayoutDefinition.GetStride(); }
		int32 GetElementStride() const { return LayoutDefinition.GetElementTypeDefinition().GetStride(); }
		int32 GetNumElements() const { return LayoutDefinition.GetBufferLayout().GetNumElements(); };
		int32 GetElementOffset(FName ElementName) const { return LayoutDefinition.GetElementOffset(ElementName); }
		const TMap<FName, uint8>& GetElementOffsets() const { return LayoutDefinition.GetElementOffsets(); }

		void CopyToArray(TArray<uint8>& OutData) const
		{
			OutData.SetNumUninitialized(GetStride() * Num());
			FMemory::Memcpy(OutData.GetData(), GetDataRaw(), OutData.Num());
		}

		template<typename DataType>
		TArrayView<const DataType> GetArrayView() const
		{
			check(sizeof(DataType) == GetStride());

			return MakeArrayView(reinterpret_cast<const DataType*>(GetDataRaw()), Num());
		}

		FRealtimeMeshDataStreamRawData GetResourceCopy() const
		{
			FRealtimeMeshDataStreamRawData OutData;
			CopyToArray(OutData);
			return OutData;
		}

		FRealtimeMeshSectionGroupStreamUpdateDataRef GetStreamUpdateData(const FRealtimeMeshStreamKey& InStreamKey) const
		{
			auto InitData = MakeUnique<FRealtimeMeshDataStreamRawData>();
			CopyToArray(*InitData);			
			return MakeShared<FRealtimeMeshSectionGroupStreamUpdateData>(MoveTemp(InitData), LayoutDefinition, InStreamKey);
		}
		
		virtual const void* GetResourceData() const override { return Allocator.GetAllocation(); }
		virtual uint32 GetResourceDataSize() const override { return Num() * GetStride(); }
		virtual void Discard() override { }
		virtual bool IsStatic() const override { return false; }
		virtual bool GetAllowCPUAccess() const override { return false; }
		virtual void SetAllowCPUAccess(bool bInNeedsCPUAccess) override { }		

		const uint8* GetDataRaw() const { return reinterpret_cast<const uint8*>(Allocator.GetAllocation()); }
		uint8* GetDataRaw() { return reinterpret_cast<uint8*>(Allocator.GetAllocation()); }

		const uint8* GetDataRawAtVertex(int32 VertexIndex) const
		{
			RangeCheck(VertexIndex);
			return GetDataRaw() + (GetStride() * VertexIndex);
		}
		uint8* GetDataRawAtVertex(int32 VertexIndex)
		{
			RangeCheck(VertexIndex);
			return GetDataRaw() + (GetStride() * VertexIndex);
		}
		
		const uint8* GetDataRawAtVertex(int32 VertexIndex, int32 ElementIndex) const
		{
			RangeCheck(VertexIndex);
			ElementCheck(ElementIndex);
			return GetDataRaw() + (GetStride() * VertexIndex + GetElementStride() * ElementIndex);
		}
		uint8* GetDataRawAtVertex(int32 VertexIndex, int32 ElementIndex)
		{
			RangeCheck(VertexIndex);
			ElementCheck(ElementIndex);
			return GetDataRaw() + (GetStride() * VertexIndex + GetElementStride() * ElementIndex);
		}

		template<typename ElementType>
		const ElementType* GetData() const { return reinterpret_cast<const ElementType*>(Allocator.GetAllocation()); }
		
		template<typename ElementType>
		ElementType* GetData() { return reinterpret_cast<ElementType*>(Allocator.GetAllocation()); }


		template<typename ElementType>
		const ElementType* GetDataAtVertex(int32 VertexIndex) const
		{
			return reinterpret_cast<const ElementType*>(GetDataRawAtVertex(VertexIndex));
		}
		
		template<typename ElementType>
		ElementType* GetDataAtVertex(int32 VertexIndex)
		{
			return reinterpret_cast<ElementType*>(GetDataRawAtVertex(VertexIndex));
		}

		template<typename ElementType>
		const ElementType* GetDataAtVertex(int32 VertexIndex, int32 ElementIndex) const
		{
			return reinterpret_cast<const ElementType*>(GetDataRawAtVertex(VertexIndex, ElementIndex));
		}
		
		template<typename ElementType>
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


		FORCEINLINE void RemoveAt(SizeType Index, SizeType Count, bool bAllowShrinking = true)
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
		

		template<typename VertexType>
		void Add(const VertexType& InVertex)
		{
			check(LayoutDefinition.GetBufferLayout() == GetRealtimeMeshBufferLayout<VertexType>());
			const SizeType Index = AddUninitialized();
			*GetDataAtVertex<VertexType>(Index) = InVertex;
		}

		void Append(const FRealtimeMeshDataStream& Source)
		{
			check(this != &Source);
			check(LayoutDefinition.GetBufferLayout() == Source.LayoutDefinition.GetBufferLayout());
			
			const SizeType Count = Source.Num();
			const SizeType Index = AddUninitialized(Count);
			FMemory::Memcpy(GetDataRawAtVertex(Index), Source.Allocator.GetAllocation(), Count * GetStride());			
		}
		
		void Append(FRealtimeMeshDataStream&& Source)
		{
			check(this != &Source);
			check(LayoutDefinition.GetBufferLayout() == Source.LayoutDefinition.GetBufferLayout());

			if (ArrayNum == 0)
			{
				Allocator.MoveToEmpty(Source.Allocator);
				ArrayMax = MoveTemp(Source.ArrayMax);
				ArrayNum = MoveTemp(Source.ArrayNum);
				return;
			}

			Append(Source);			
		}

		template<typename VertexType>
		FORCENOINLINE void Append(TArrayView<VertexType> NewElements)
		{
			check(LayoutDefinition.GetBufferLayout() == GetRealtimeMeshBufferLayout<VertexType>());
			
			const SizeType Count = NewElements.Num();
			const SizeType Index = AddUninitialized(Count);
			FMemory::Memcpy(GetDataRawAtVertex(Index), NewElements.GetData(), Count * GetStride());	
		}
		
		template<typename VertexType, typename InAllocatorType = FDefaultAllocator>
		void Append(const TArray<VertexType, InAllocatorType>& NewElements)
		{			
			check(LayoutDefinition.GetBufferLayout() == GetRealtimeMeshBufferLayout<VertexType>());
			
			const SizeType Count = NewElements.Num();
			const SizeType Index = AddUninitialized(Count);
			FMemory::Memcpy(GetDataRawAtVertex(Index), NewElements.GetData(), Count * GetStride());	
		}
		
		template<typename VertexType>
		void Append(std::initializer_list<VertexType> NewElements)
		{
			Append(MakeArrayView(NewElements.begin(), NewElements.size()));
		}

		template<typename VertexType>
		void Append(int32 Count, VertexType* NewElements)
		{
			Append(MakeArrayView(NewElements, Count));			
		}
		
		template<typename VertexType, typename GeneratorFunc>
		void Append(int32 Count, GeneratorFunc Generator)
		{
			SizeType Index = AddUninitialized(Count);
			VertexType* DataPtr = GetDataAtVertex<VertexType>(Index);
			
			while (Index != ArrayMax)
			{
				DataPtr[Index] = Generator(Index);				
				Index++;
			}
		}
		
		template<typename VertexType, typename GeneratorFunc>
		void AppendBy(int32 Count, GeneratorFunc Generator)
		{
			SizeType Index = AddUninitialized(Count);
			VertexType* DataPtr = GetDataAtVertex<VertexType>(Index);
			while (Index != ArrayMax)
			{
				DataPtr[Index] = Generator(Index);				
				Index++;
			}
		}

		
		void CountBytes(FArchive& Ar) const
		{
			Ar.CountBytes(ArrayNum * GetStride(), ArrayMax * GetStride());
		}
		
		friend FArchive& operator<<(FArchive& Ar, FRealtimeMeshDataStream& Stream)
		{
			Ar << Stream.StreamName;
			
			FRealtimeMeshBufferLayout BufferLayout = Stream.LayoutDefinition.GetBufferLayout();
			Ar << BufferLayout;
			if (Ar.IsLoading())
			{
				Stream.LayoutDefinition = FRealtimeMeshBufferLayoutDefinition(FRealtimeMeshBufferLayoutUtilities::GetBufferLayoutDefinition(BufferLayout));
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
				Ar.Serialize(Stream.GetDataRaw(), SerializedNum * Stream.GetStride());
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
				checkf((Index >= 0) & (Index < ArrayNum), TEXT("Array index out of bounds: %lld from an array of size %lld"), static_cast<int64>(Index), static_cast<int64>(ArrayNum)); // & for one branch
			}
		}
		
		FORCEINLINE void ElementCheck(SizeType ElementIndex) const
		{
			checkf((ElementIndex >= 0) & (ElementIndex < GetNumElements()), TEXT("Element index out of bounds: %lld from an element list of size %lld"), static_cast<int64>(ElementIndex), static_cast<int64>(GetNumElements())); // & for one branch
		}
		
		void ResizeAllocation(USizeType NewNum)
		{
			if (NewNum != ArrayMax)
			{
				Allocator.ResizeAllocation(ArrayNum, NewNum,
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
	


	class REALTIMEMESHCOMPONENT_API FRealtimeMeshVertexDataBuilder : public TSharedFromThis<FRealtimeMeshVertexDataBuilder>
	{
	private:
		TMap<FRealtimeMeshStreamKey, FRealtimeMeshDataStreamPtr> Buffers;
	public:
		virtual ~FRealtimeMeshVertexDataBuilder() = default;

		const TMap<FRealtimeMeshStreamKey, FRealtimeMeshDataStreamPtr>& GetBuffers() const { return Buffers; }
		
		template<typename VertexType>
		FRealtimeMeshDataStreamRef CreateVertexStream(FName StreamName)
		{
			FRealtimeMeshDataStreamRef NewStream = MakeShared<FRealtimeMeshDataStream>(StreamName, GetRealtimeMeshBufferLayout<VertexType>());
			Buffers.FindOrAdd(FRealtimeMeshStreamKey(ERealtimeMeshStreamType::Vertex, StreamName)) = NewStream;
			return NewStream;
		}
	
		template<typename IndexType>
		FRealtimeMeshDataStreamRef CreateIndexStream(FName StreamName)
		{
			FRealtimeMeshDataStreamRef NewStream = MakeShared<FRealtimeMeshDataStream>(StreamName, GetRealtimeMeshBufferLayout<IndexType>());
			Buffers.FindOrAdd(FRealtimeMeshStreamKey(ERealtimeMeshStreamType::Index, StreamName)) = NewStream;
			return NewStream;
		}
	
		//TSharedRef<struct FRealtimeMeshBufferDataSetBase> GetUpdateData();
	};























	
	//
	//
	//
	//
	//
	//
	// template<typename VertexType>
	// struct TRealtimeMeshDataStreamVertexReader
	// {
	// protected:
	// 	const FRealtimeMeshDataStreamConstRef Stream;
	// 	mutable int32 Index;
	// public:
	// 	TRealtimeMeshDataStreamVertexReader(const FRealtimeMeshDataStreamConstRef& InStream)
	// 		: Stream(InStream), Index(0)
	// 	{
	// 		
	// 	}
	// 	
	// 	FORCEINLINE bool IsEmpty() const { return Stream->IsEmpty(); }
	// 	FORCEINLINE void Restart() const { Index = 0; }
	// 	FORCEINLINE bool Seek(int32 InIndex) const
	// 	{
	// 		if (Stream->IsValidIndex(InIndex))
	// 		{
	// 			Index = InIndex;
	// 			return true;
	// 		}
	// 		return false;
	// 	}
	// 	FORCEINLINE void SeekEnd(int32 InIndex) const
	// 	{
	// 		Index = FMath::Max(0, Stream->GetNum());
	// 	}
	// 	FORCEINLINE bool IsAtStart() const { return Index == 0; }
	// 	FORCEINLINE bool IsAtEnd() const { return Index == Stream->GetNum(); }
	// 	
	// 	FORCEINLINE const VertexType& GetAndAdvance() const
	// 	{
	// 		return *static_cast<VertexType*>(Stream->GetDataAtVertex(Index++, 0));
	// 	}
	//
	// 	
	// };
	//
	//
	//
	// template<typename VertexType>
	// struct TRealtimeMeshDataStreamVertexWriter : public TRealtimeMeshDataStreamVertexReader<VertexType>
	// {
	// protected:
	// 	using Super = TRealtimeMeshDataStreamVertexReader<VertexType>;
	// 	FRealtimeMeshDataStreamRef GetWritableStream() { return ConstCastSharedRef<FRealtimeMeshDataStream>(Super::Stream); }
	// public:		
	// 	TRealtimeMeshDataStreamVertexWriter(const FRealtimeMeshDataStreamRef& InStream)
	// 		: Super(InStream)
	// 	{
	// 		
	// 	}
	//
	// 	FORCEINLINE void Shrink() { GetWritableStream()->Shrink(); }
	// 	FORCEINLINE void Reset(int32 NewSize = 0) { GetWritableStream()->Reset(NewSize); Restart(); }
	// 	FORCEINLINE void Empty(int32 Slack = 0) { GetWritableStream()->Empty(Slack); Restart(); }
	// 	FORCEINLINE void Reserve(int32 Number) { GetWritableStream()->Reserve(Number); }
	//
	// 	FORCEINLINE int32 AddUninitialized(int32 Count = 1) { return GetWritableStream()->AddUninitialized(Count); }
	// 	FORCEINLINE void SetNum(int32 NewNum, bool bAllowShrinking = true) { GetWritableStream()->SetNum(NewNum, bAllowShrinking); }
	// 	FORCEINLINE void SetNumZeroed(int32 NewNum, bool bAllowShrinking = true) { GetWritableStream()->SetNumZeroed(NewNum, bAllowShrinking); }
	// 	FORCEINLINE void SetNumUninitialized(int32 NewNum, bool bAllowShrinking = true) { GetWritableStream()->SetNumUninitialized(NewNum, bAllowShrinking); }
	// 		
	// 	void* GetAndAdvance(bool bShouldAppend = false)
	// 	{
	// 		if (Super::IsAtEnd())
	// 		{
	// 			AddUninitialized(1);
	// 			check(GetWritableStream()->IsValidIndex(Index));
	// 		}
	// 		return GetWritableStream()->GetDataAtVertex(Index++, 0);
	// 	}
	//
	// 	void SetRange(int32 StartIndex, std::initializer_list<VertexType> NewElements);
	// 	template<typename GeneratorType>
	// 	void SetRange(int32 StartIndex, int32 Count, GeneratorType Generator);
	//
	// 	void Append(std::initializer_list<VertexType> NewElements);
	// 	template<typename GeneratorType>
	// 	void Append(int32 Count, GeneratorType Generator);
	//
	// 	
	// };
	//
	//
	// struct TRealtimeMeshDataStreamVertexElementReader;
	// struct TRealtimeMeshDataStreamVertexElementWriter;
	//
	// struct FRealtimeMeshBuilder
	// {
	// 	
	// };
	//
	//


















	
}