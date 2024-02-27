// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RealtimeMeshBuilder.h"
#include "RealtimeMeshDataStream.h"
#include "RealtimeMeshDataTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RealtimeMeshBlueprintMeshBuilder.generated.h"

struct FRealtimeMeshStreamKey;


namespace RealtimeMesh
{
	struct FRealtimeMeshStream;
	
	namespace StreamBuilder::Private
	{
		class IDataInterface
		{
		public:
			virtual ~IDataInterface() { }
			virtual FString GetTypeName() = 0;
			virtual void SetInt(FRealtimeMeshStream* Stream, int32 Index, int32 ElementIdx, int32 NewValue);
			virtual void SetLong(FRealtimeMeshStream* Stream, int32 Index, int32 ElementIdx, int64 NewValue);
			virtual void SetFloat(FRealtimeMeshStream* Stream, int32 Index, int32 ElementIdx, float NewValue);
			virtual void SetVector2(FRealtimeMeshStream* Stream, int32 Index, int32 ElementIdx, const FVector2D& NewValue);
			virtual void SetVector3(FRealtimeMeshStream* Stream, int32 Index, int32 ElementIdx, const FVector& NewValue);
			virtual void SetVector4(FRealtimeMeshStream* Stream, int32 Index, int32 ElementIdx, const FVector4& NewValue);
			virtual void SetColor(FRealtimeMeshStream* Stream, int32 Index, int32 ElementIdx, const FLinearColor& NewValue);

			virtual int32 GetInt(FRealtimeMeshStream* Stream, int32 Index, int32 ElementIdx);
			virtual int64 GetLong(FRealtimeMeshStream* Stream, int32 Index, int32 ElementIdx);
			virtual float GetFloat(FRealtimeMeshStream* Stream, int32 Index, int32 ElementIdx);
			virtual FVector2D GetVector2(FRealtimeMeshStream* Stream, int32 Index, int32 ElementIdx);
			virtual FVector GetVector3(FRealtimeMeshStream* Stream, int32 Index, int32 ElementIdx);
			virtual FVector4 GetVector4(FRealtimeMeshStream* Stream, int32 Index, int32 ElementIdx);
			virtual FLinearColor GetColor(FRealtimeMeshStream* Stream, int32 Index, int32 ElementIdx);

		};

		template<typename StreamType, typename = void>
		class TDataInterface : public IDataInterface
		{		
			virtual FString GetTypeName() override { return TEXT("Unknown"); }
		};
	
		template<typename StreamType>
		class TDataInterface<StreamType, typename std::enable_if_t<std::is_integral_v<StreamType>>> : public IDataInterface
		{
			virtual FString GetTypeName() override { return GetRealtimeMeshBufferLayout<StreamType>().ToString(); }
		
			virtual void SetInt(FRealtimeMeshStream* Stream, int32 Index, int32 ElementIdx, int32 NewValue) override
			{
				ensure((StreamType)NewValue >= TNumericLimits<StreamType>::Min() && (StreamType)NewValue <= TNumericLimits<StreamType>::Max());
				*Stream->GetDataAtVertex<StreamType>(Index, ElementIdx) = NewValue;
			}		
			virtual void SetLong(FRealtimeMeshStream* Stream, int32 Index, int32 ElementIdx, int64 NewValue) override
			{
				ensure((StreamType)NewValue >= TNumericLimits<StreamType>::Min() && (StreamType)NewValue <= TNumericLimits<StreamType>::Max());
				*Stream->GetDataAtVertex<StreamType>(Index, ElementIdx) = NewValue;			
			}
			virtual void SetFloat(FRealtimeMeshStream* Stream, int32 Index, int32 ElementIdx, float NewValue) override
			{
				ensure((StreamType)FMath::RoundToInt(NewValue) >= TNumericLimits<StreamType>::Min() && (StreamType)FMath::RoundToInt(NewValue) <= TNumericLimits<StreamType>::Max());
				*Stream->GetDataAtVertex<StreamType>(Index, ElementIdx) = FMath::RoundToInt(NewValue);	
			}
		
			virtual int32 GetInt(FRealtimeMeshStream* Stream, int32 Index, int32 ElementIdx) override
			{
				return *Stream->GetDataAtVertex<StreamType>(Index, ElementIdx);			
			}
			virtual int64 GetLong(FRealtimeMeshStream* Stream, int32 Index, int32 ElementIdx) override
			{
				return *Stream->GetDataAtVertex<StreamType>(Index, ElementIdx);			
			}
			virtual float GetFloat(FRealtimeMeshStream* Stream, int32 Index, int32 ElementIdx) override
			{
				return *Stream->GetDataAtVertex<StreamType>(Index, ElementIdx);
			}
		};

		template<typename StreamType>
		class TDataInterface<StreamType, typename std::enable_if_t<std::is_floating_point_v<StreamType>>> : public IDataInterface
		{
			virtual FString GetTypeName() override { return GetRealtimeMeshBufferLayout<StreamType>().ToString(); }
		
			virtual void SetInt(FRealtimeMeshStream* Stream, int32 Index, int32 ElementIdx, int32 NewValue) override
			{
				ensure((StreamType)NewValue >= TNumericLimits<StreamType>::Min() && (StreamType)NewValue <= TNumericLimits<StreamType>::Max());
				*Stream->GetDataAtVertex<StreamType>(Index, ElementIdx) = NewValue;
			}		
			virtual void SetLong(FRealtimeMeshStream* Stream, int32 Index, int32 ElementIdx, int64 NewValue) override
			{
				ensure((StreamType)NewValue >= TNumericLimits<StreamType>::Min() && (StreamType)NewValue <= TNumericLimits<StreamType>::Max());
				*Stream->GetDataAtVertex<StreamType>(Index, ElementIdx) = NewValue;			
			}
			virtual void SetFloat(FRealtimeMeshStream* Stream, int32 Index, int32 ElementIdx, float NewValue) override
			{
				ensure((StreamType)FMath::RoundToInt(NewValue) >= TNumericLimits<StreamType>::Min() && (StreamType)FMath::RoundToInt(NewValue) <= TNumericLimits<StreamType>::Max());
				*Stream->GetDataAtVertex<StreamType>(Index, ElementIdx) = NewValue;	
			}
		
			virtual int32 GetInt(FRealtimeMeshStream* Stream, int32 Index, int32 ElementIdx) override
			{
				return FMath::RoundToInt(*Stream->GetDataAtVertex<StreamType>(Index, ElementIdx));
			}
			virtual int64 GetLong(FRealtimeMeshStream* Stream, int32 Index, int32 ElementIdx) override
			{
				return FMath::RoundToInt64(*Stream->GetDataAtVertex<StreamType>(Index, ElementIdx));	
			}
			virtual float GetFloat(FRealtimeMeshStream* Stream, int32 Index, int32 ElementIdx) override
			{
				return *Stream->GetDataAtVertex<StreamType>(Index, ElementIdx);
			}
		};
	
		template<typename StreamType>
		class TDataInterface<StreamType, std::enable_if_t<std::is_same_v<StreamType, FVector2f> || std::is_same_v<StreamType, FVector2DHalf>>> : public IDataInterface
		{
			virtual FString GetTypeName() override { return GetRealtimeMeshBufferLayout<StreamType>().ToString(); }
		
			virtual void SetVector2(FRealtimeMeshStream* Stream, int32 Index, int32 ElementIdx, const FVector2D& NewValue) override
			{
				*Stream->GetDataAtVertex<StreamType>(Index, ElementIdx) = FVector2f(NewValue);
			}
		
			virtual FVector2D GetVector2(FRealtimeMeshStream* Stream, int32 Index, int32 ElementIdx) override
			{
				return FVector2D(*Stream->GetDataAtVertex<StreamType>(Index, ElementIdx));		
			}
		};

		template<typename StreamType>
		class TDataInterface<StreamType, std::enable_if_t<std::is_same_v<StreamType, FVector3f>>> : public IDataInterface
		{
			virtual FString GetTypeName() override { return GetRealtimeMeshBufferLayout<StreamType>().ToString(); }
		
			virtual void SetVector3(FRealtimeMeshStream* Stream, int32 Index, int32 ElementIdx, const FVector& NewValue) override
			{
				*Stream->GetDataAtVertex<StreamType>(Index, ElementIdx) = FVector3f(NewValue);
			}
		
			virtual FVector GetVector3(FRealtimeMeshStream* Stream, int32 Index, int32 ElementIdx) override
			{
				return FVector(*Stream->GetDataAtVertex<StreamType>(Index, ElementIdx));		
			}
		};

		template<typename StreamType>
		class TDataInterface<StreamType, std::enable_if_t<std::is_same_v<StreamType, FVector4f>>> : public IDataInterface
		{
			virtual FString GetTypeName() override { return GetRealtimeMeshBufferLayout<StreamType>().ToString(); }
		
			virtual void SetVector4(FRealtimeMeshStream* Stream, int32 Index, int32 ElementIdx, const FVector4d& NewValue) override
			{
				*Stream->GetDataAtVertex<StreamType>(Index, ElementIdx) = FVector4f(NewValue);
			}
		
			virtual FVector4d GetVector4(FRealtimeMeshStream* Stream, int32 Index, int32 ElementIdx) override
			{
				return FVector4d(*Stream->GetDataAtVertex<StreamType>(Index, ElementIdx));		
			}
		};


		template<typename StreamType>
		class TDataInterface<StreamType, std::enable_if_t<std::is_same_v<StreamType, FPackedNormal> || std::is_same_v<StreamType, FPackedRGBA16N>>> : public IDataInterface
		{
			virtual FString GetTypeName() override { return GetRealtimeMeshBufferLayout<StreamType>().ToString(); }
		
			virtual void SetVector3(FRealtimeMeshStream* Stream, int32 Index, int32 ElementIdx, const FVector& NewValue) override
			{
				*Stream->GetDataAtVertex<StreamType>(Index, ElementIdx) = StreamType(NewValue);
			}
		
			virtual void SetVector4(FRealtimeMeshStream* Stream, int32 Index, int32 ElementIdx, const FVector4d& NewValue) override
			{
				*Stream->GetDataAtVertex<StreamType>(Index, ElementIdx) = StreamType(NewValue);
			}
		
			virtual FVector GetVector3(FRealtimeMeshStream* Stream, int32 Index, int32 ElementIdx) override
			{
				return Stream->GetDataAtVertex<StreamType>(Index, ElementIdx)->ToFVector();		
			}
		
			virtual FVector4d GetVector4(FRealtimeMeshStream* Stream, int32 Index, int32 ElementIdx) override
			{
				return Stream->GetDataAtVertex<StreamType>(Index, ElementIdx)->ToFVector4();		
			}
		};

		template<typename StreamType>
		class TDataInterface<StreamType, std::enable_if_t<std::is_same_v<StreamType, FColor>>> : public IDataInterface
		{
			virtual FString GetTypeName() override { return GetRealtimeMeshBufferLayout<StreamType>().ToString(); }
		
			virtual void SetColor(FRealtimeMeshStream* Stream, int32 Index, int32 ElementIdx, const FLinearColor& NewValue) override
			{
				*Stream->GetDataAtVertex<StreamType>(Index, ElementIdx) = NewValue.ToFColor(true);
			}
		
			virtual FLinearColor GetColor(FRealtimeMeshStream* Stream, int32 Index, int32 ElementIdx) override
			{
				return FLinearColor(*Stream->GetDataAtVertex<StreamType>(Index, ElementIdx));		
			}
		};


		static IDataInterface* GetInterfaceForType(const FRealtimeMeshBufferLayout& BufferLayout)
		{
			if (BufferLayout.GetElementType() == GetRealtimeMeshDataElementType<uint16>())
			{
				return new TDataInterface<uint16>();
			}
			else if (BufferLayout.GetElementType() == GetRealtimeMeshDataElementType<int16>())
			{
				return new TDataInterface<int16>();
			}
			else if (BufferLayout.GetElementType() == GetRealtimeMeshDataElementType<uint32>())
			{
				return new TDataInterface<uint32>();
			}
			else if (BufferLayout.GetElementType() == GetRealtimeMeshDataElementType<int32>())
			{
				return new TDataInterface<int32>();
			}
			else if (BufferLayout.GetElementType() == GetRealtimeMeshDataElementType<float>())
			{
				return new TDataInterface<float>();
			}
			else if (BufferLayout.GetElementType() == GetRealtimeMeshDataElementType<FVector2f>())
			{
				return new TDataInterface<FVector2f>();
			}
			else if (BufferLayout.GetElementType() == GetRealtimeMeshDataElementType<FVector3f>())
			{
				return new TDataInterface<FVector3f>();
			}
			else if (BufferLayout.GetElementType() == GetRealtimeMeshDataElementType<FVector4f>())
			{
				return new TDataInterface<FVector4f>();
			}
			else if (BufferLayout.GetElementType() == GetRealtimeMeshDataElementType<FVector2DHalf>())
			{
				return new TDataInterface<FVector2DHalf>();
			}
			else if (BufferLayout.GetElementType() == GetRealtimeMeshDataElementType<FPackedNormal>())
			{
				return new TDataInterface<FPackedNormal>();
			}
			else if (BufferLayout.GetElementType() == GetRealtimeMeshDataElementType<FPackedRGBA16N>())
			{
				return new TDataInterface<FPackedRGBA16N>();
			}
			else if (BufferLayout.GetElementType() == GetRealtimeMeshDataElementType<FColor>())
			{
				return new TDataInterface<FColor>();
			}

			return nullptr;
		}



		struct FStreamState
		{		
			FRealtimeMeshStream* StreamPtr;
			TSharedPtr<IDataInterface> Interface;
			bool bIsValid;

			FStreamState(FRealtimeMeshStream* InStreamPtr, const TSharedPtr<IDataInterface>& InInterface, bool bInIsValid)
				: StreamPtr(InStreamPtr)
				, Interface(InInterface)
				, bIsValid(bInIsValid)
			{ }

			FStreamState()
				: StreamPtr(nullptr)
				, Interface(nullptr)
				, bIsValid(false)
			{ }

			bool IsValid() const
			{
				return StreamPtr != nullptr && Interface;
			}

			bool DoIfValid(const TFunctionRef<void(FRealtimeMeshStream*, IDataInterface*)>& Func)
			{
				if (IsValid())
				{
					Func(StreamPtr, Interface.Get());
					return true;
				}
				return false;
			}
		};

		
		static FStreamState GetStream(FRealtimeMeshStreamSet& StreamSet, const FRealtimeMeshStreamKey& StreamKey)
		{
			if (auto StreamPtr = StreamSet.Find(StreamKey))
			{
				if (auto Interface = GetInterfaceForType(StreamPtr->GetLayout()))
				{					
					return FStreamState(StreamPtr, MakeShareable(Interface), true);
				}
				
				return FStreamState(StreamPtr, nullptr, false);
			}
			
			return FStreamState(nullptr, nullptr, false);
		}
		
		static FStreamState GetOrCreateStream(FRealtimeMeshStreamSet& StreamSet, const FRealtimeMeshStreamKey& StreamKey, const FRealtimeMeshBufferLayout& Layout)
		{
			if (auto StreamPtr = StreamSet.Find(StreamKey))
			{
				if (auto Interface = GetInterfaceForType(Layout))
				{					
					if (StreamPtr->ConvertTo(Layout))
					{
						return FStreamState(StreamPtr, MakeShareable(Interface), true);
					}
					
					delete Interface;
					return FStreamState(nullptr, MakeShareable(Interface), false);
				}
				
				return FStreamState(StreamPtr, nullptr, false);
			}
			
			return FStreamState(nullptr, nullptr, false);
		}
		
	}
}







UENUM(BlueprintType)
enum class ERealtimeMeshSimpleStreamType : uint8
{
	Unknown,
	Int16,
	UInt16,
	Int32,
	UInt32,
	Float,
	Vector2,
	Vector3,
	HalfVector2,
	PackedNormal,
	PackedRGBA16N,
	Triangle16,
	Triangle32,
};



UENUM()
enum class ERealtimeMeshSimpleStreamConfig : uint8
{
	None,
	Normal,
	HighPrecision
};



class URealtimeMeshStreamSet;

USTRUCT(BlueprintType)
struct REALTIMEMESHCOMPONENT_API FRealtimeMeshStreamPtr
{
	GENERATED_BODY()
public:
	
	UPROPERTY()
	TObjectPtr<URealtimeMeshStreamSet> StreamSet;
	
	FRealtimeMeshStreamKey StreamKey;
	mutable RealtimeMesh::StreamBuilder::Private::FStreamState StreamState;
	mutable int32 ChangeId;

	FRealtimeMeshStreamPtr()
		: StreamSet(nullptr)
		, ChangeId(INDEX_NONE)
	{ }
	
	FRealtimeMeshStreamPtr(URealtimeMeshStreamSet* InStreamSet, const FRealtimeMeshStreamKey& InStreamKey, RealtimeMesh::StreamBuilder::Private::FStreamState InStreamState, int32 InChangeId)
		: StreamSet(InStreamSet)
		, StreamKey(InStreamKey)
		, StreamState(InStreamState)
		, ChangeId(InChangeId)
	{
		
	}

	void UpdateIfNecessary() const;
};

USTRUCT(BlueprintType)
struct REALTIMEMESHCOMPONENT_API FRealtimeMeshStreamRowPtr
{
	GENERATED_BODY()
public:
	
	UPROPERTY()
	TObjectPtr<URealtimeMeshStreamSet> StreamSet;
	FRealtimeMeshStreamKey StreamKey;	
	mutable RealtimeMesh::StreamBuilder::Private::FStreamState StreamState;
	mutable int32 ChangeId;
	int32 RowIndex;

	FRealtimeMeshStreamRowPtr()
		: StreamSet(nullptr)
		, ChangeId(INDEX_NONE)
		, RowIndex(INDEX_NONE)
	{ }
	
	FRealtimeMeshStreamRowPtr(const FRealtimeMeshStreamPtr& Stream, int32 InRowIndex)
		: StreamSet(Stream.StreamSet)
		, StreamKey(Stream.StreamKey)
		, StreamState(Stream.StreamState)
		, ChangeId(Stream.ChangeId)
		, RowIndex(InRowIndex)
	{
		
	}
	
	FRealtimeMeshStreamRowPtr(URealtimeMeshStreamSet* InStreamSet, FRealtimeMeshStreamKey InStreamKey, RealtimeMesh::StreamBuilder::Private::FStreamState InStreamState, int32 InChangeId, int32 InRowIndex)
		: StreamSet(InStreamSet)
		, StreamKey(InStreamKey)
		, StreamState(InStreamState)
		, ChangeId(InChangeId)
		, RowIndex(InRowIndex)
	{
		
	}
	
	void UpdateIfNecessary() const;
};


/*struct REALTIMEMESHCOMPONENT_API FRealtimeMeshLocalBuilderResources
{	
	mutable RealtimeMesh::StreamBuilder::Private::FStreamState Position;
	mutable RealtimeMesh::StreamBuilder::Private::FStreamState Tangents;
	mutable RealtimeMesh::StreamBuilder::Private::FStreamState TexCoords;
	mutable RealtimeMesh::StreamBuilder::Private::FStreamState Colors;

	mutable RealtimeMesh::StreamBuilder::Private::FStreamState Triangles;
	mutable RealtimeMesh::StreamBuilder::Private::FStreamState DepthOnlyTriangles;

	mutable RealtimeMesh::StreamBuilder::Private::FStreamState PolyGroups;
	mutable RealtimeMesh::StreamBuilder::Private::FStreamState DepthOnlyPolyGroups;

	mutable int32 ChangeId = INDEX_NONE;

	void UpdateIfNecessary(URealtimeMeshStreamSet* StreamSet);
};*/




/**
 * 
 */
UCLASS()
class REALTIMEMESHCOMPONENT_API URealtimeMeshStreamSet : public UObject
{
	GENERATED_BODY()
protected:
	RealtimeMesh::FRealtimeMeshStreamSet StreamSet;

	// This change id is incremented every time the streamset is changed, whether that's by adding a
	// stream, removing a stream, or emptying or moving all the streams
	// This is so anything referring to us knows to update/invalidate itself
	mutable int32 ChangeId;
public:
		
	RealtimeMesh::FRealtimeMeshStream& AddStream(const FRealtimeMeshStreamKey& StreamKey, const RealtimeMesh::FRealtimeMeshBufferLayout& BufferLayout)
	{
		ChangeId++;
		return StreamSet.AddStream(StreamKey, BufferLayout);
	}
	
	RealtimeMesh::FRealtimeMeshStream* GetStream(const FRealtimeMeshStreamKey& StreamKey)
	{
		return StreamSet.Find(StreamKey);
	}
	
	RealtimeMesh::FRealtimeMeshStream& GetOrAddStream(const FRealtimeMeshStreamKey& StreamKey, const RealtimeMesh::FRealtimeMeshBufferLayout& ConvertToLayout, bool bKeepData = true)
	{
		if (StreamSet.Find(StreamKey) == nullptr || StreamSet.Find(StreamKey)->GetLayout() != ConvertToLayout)
		{				
			auto& NewStream = StreamSet.FindOrAdd(StreamKey, ConvertToLayout, bKeepData);
			ChangeId++;
			return NewStream;
		}

		return StreamSet.FindChecked(StreamKey);
	}
	
	void RemoveStream(FRealtimeMeshStreamKey Tangents)
	{
		if (StreamSet.Remove(Tangents))
		{
			ChangeId++;
		}
	}
	
	RealtimeMesh::FRealtimeMeshStreamSet& GetStreamSet()
	{
		return StreamSet;
	}

	void IncrementChangeId() const { ChangeId++; }
	int32 GetChangeId() const { return ChangeId; }
};



USTRUCT(BlueprintType)
struct REALTIMEMESHCOMPONENT_API FRealtimeMeshLocalBuilder
{
	GENERATED_BODY()
public:
	
	UPROPERTY()
	TObjectPtr<URealtimeMeshStreamSet> StreamSet;

	mutable TOptional<RealtimeMesh::TRealtimeMeshBuilderLocal<void, void, void, 1, void>> Builder;

	mutable TOptional<RealtimeMesh::TRealtimeMeshStridedStreamBuilder<FVector2f, void>> UV1Builder;
	mutable TOptional<RealtimeMesh::TRealtimeMeshStridedStreamBuilder<FVector2f, void>> UV2Builder;
	mutable TOptional<RealtimeMesh::TRealtimeMeshStridedStreamBuilder<FVector2f, void>> UV3Builder;
	
	FRealtimeMeshLocalBuilder()
		: StreamSet(nullptr)
	{
		
	}
	
	FRealtimeMeshLocalBuilder(URealtimeMeshStreamSet* InStreamSet)
		: StreamSet(InStreamSet)
		, Builder(StreamSet->GetStreamSet())
	{
		
	}
	
};




UCLASS(meta=(ScriptName="RealtimeMeshStreamUtils"))
class REALTIMEMESHCOMPONENT_API URealtimeMeshStreamUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:

	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	static URealtimeMeshStreamSet* MakeRealtimeMeshStreamSet();


	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	static FRealtimeMeshStreamPtr GetStream(URealtimeMeshStreamSet* StreamSet, FRealtimeMeshStreamKey StreamKey);

	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	static FRealtimeMeshStreamPtr AddStream(URealtimeMeshStreamSet* StreamSet, FRealtimeMeshStreamKey StreamKey, ERealtimeMeshSimpleStreamType StreamType, int32 NumElements);

	
	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	static const FRealtimeMeshStreamPtr& GetNum(const FRealtimeMeshStreamPtr& Stream, int32& Num);

	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	static const FRealtimeMeshStreamPtr& IsIndexValid(const FRealtimeMeshStreamPtr& Stream, int32 Index, bool& bIsValid);
	
	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	static const FRealtimeMeshStreamPtr& IsEmpty(const FRealtimeMeshStreamPtr& Stream, bool& bIsEmpty);

	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	static const FRealtimeMeshStreamPtr& Reserve(const FRealtimeMeshStreamPtr& Stream, int32 Num);

	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	static const FRealtimeMeshStreamPtr& Shrink(const FRealtimeMeshStreamPtr& Stream);
	
	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	static const FRealtimeMeshStreamPtr& Empty(const FRealtimeMeshStreamPtr& Stream, int32 ExpectedSize = 0);

	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	static const FRealtimeMeshStreamPtr& SetNumUninitialized(const FRealtimeMeshStreamPtr& Stream, int32 NewNum);
	
	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	static const FRealtimeMeshStreamPtr& SetNumZeroed(const FRealtimeMeshStreamPtr& Stream, int32 NewNum);

	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	static FRealtimeMeshStreamRowPtr AddUninitialized(const FRealtimeMeshStreamPtr& Stream, int32 NumToAdd, int32& Index);
	
	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	static FRealtimeMeshStreamRowPtr AddZeroed(const FRealtimeMeshStreamPtr& Stream, int32 NumToAdd, int32& Index);
	

	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	static FRealtimeMeshStreamRowPtr AddInt(const FRealtimeMeshStreamPtr& Stream, int32 NewValue, int32& Index);
	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	static FRealtimeMeshStreamRowPtr AddLong(const FRealtimeMeshStreamPtr& Stream, int64 NewValue, int32& Index);
	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	static FRealtimeMeshStreamRowPtr AddFloat(const FRealtimeMeshStreamPtr& Stream, float NewValue, int32& Index);
	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	static FRealtimeMeshStreamRowPtr AddVector2(const FRealtimeMeshStreamPtr& Stream, FVector2D NewValue, int32& Index);
	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	static FRealtimeMeshStreamRowPtr AddVector3(const FRealtimeMeshStreamPtr& Stream, FVector NewValue, int32& Index);
	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	static FRealtimeMeshStreamRowPtr AddVector4(const FRealtimeMeshStreamPtr& Stream, FVector4 NewValue, int32& Index);

	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	static FRealtimeMeshStreamRowPtr SetInt(const FRealtimeMeshStreamPtr& Stream, int32 Index, int32 NewValue);
	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	static FRealtimeMeshStreamRowPtr SetLong(const FRealtimeMeshStreamPtr& Stream, int32 Index, int64 NewValue);
	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	static FRealtimeMeshStreamRowPtr SetFloat(const FRealtimeMeshStreamPtr& Stream, int32 Index, float NewValue);
	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	static FRealtimeMeshStreamRowPtr SetVector2(const FRealtimeMeshStreamPtr& Stream, int32 Index, FVector2D NewValue);
	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	static FRealtimeMeshStreamRowPtr SetVector3(const FRealtimeMeshStreamPtr& Stream, int32 Index, FVector NewValue);
	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	static FRealtimeMeshStreamRowPtr SetVector4(const FRealtimeMeshStreamPtr& Stream, int32 Index, FVector4 NewValue);


	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	static FRealtimeMeshStreamRowPtr GetInt(const FRealtimeMeshStreamPtr& Stream, int32 Index, int32 ElementIdx, int32& Value);
	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	static FRealtimeMeshStreamRowPtr GetLong(const FRealtimeMeshStreamPtr& Stream, int32 Index, int32 ElementIdx, int64& Value);
	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	static FRealtimeMeshStreamRowPtr GetFloat(const FRealtimeMeshStreamPtr& Stream, int32 Index, int32 ElementIdx, float& Value);
	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	static FRealtimeMeshStreamRowPtr GetVector2(const FRealtimeMeshStreamPtr& Stream, int32 Index, int32 ElementIdx, FVector2D& Value);
	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	static FRealtimeMeshStreamRowPtr GetVector3(const FRealtimeMeshStreamPtr& Stream, int32 Index, int32 ElementIdx, FVector& Value);
	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	static FRealtimeMeshStreamRowPtr GetVector4(const FRealtimeMeshStreamPtr& Stream, int32 Index, int32 ElementIdx, FVector4& Value);



	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	static const FRealtimeMeshStreamRowPtr& SetIntElement(const FRealtimeMeshStreamRowPtr& Stream, int32 Index, int32 ElementIdx, int32 NewValue);
	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	static const FRealtimeMeshStreamRowPtr& SetLongElement(const FRealtimeMeshStreamRowPtr& Stream, int32 Index, int32 ElementIdx, int64 NewValue);
	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	static const FRealtimeMeshStreamRowPtr& SetFloatElement(const FRealtimeMeshStreamRowPtr& Stream, int32 Index, int32 ElementIdx, float NewValue);
	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	static const FRealtimeMeshStreamRowPtr& SetVector2Element(const FRealtimeMeshStreamRowPtr& Stream, int32 Index, int32 ElementIdx, FVector2D NewValue);
	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	static const FRealtimeMeshStreamRowPtr& SetVector3Element(const FRealtimeMeshStreamRowPtr& Stream, int32 Index, int32 ElementIdx, FVector NewValue);
	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	static const FRealtimeMeshStreamRowPtr& SetVector4Element(const FRealtimeMeshStreamRowPtr& Stream, int32 Index, int32 ElementIdx, FVector4 NewValue);


	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	static const FRealtimeMeshStreamRowPtr& GetIntElement(const FRealtimeMeshStreamRowPtr& Stream, int32 Index, int32 ElementIdx, int32& Value);
	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	static const FRealtimeMeshStreamRowPtr& GetLongElement(const FRealtimeMeshStreamRowPtr& Stream, int32 Index, int32 ElementIdx, int64& Value);
	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	static const FRealtimeMeshStreamRowPtr& GetFloatElement(const FRealtimeMeshStreamRowPtr& Stream, int32 Index, int32 ElementIdx, float& Value);
	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	static const FRealtimeMeshStreamRowPtr& GetVector2Element(const FRealtimeMeshStreamRowPtr& Stream, int32 Index, int32 ElementIdx, FVector2D& Value);
	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	static const FRealtimeMeshStreamRowPtr& GetVector3Element(const FRealtimeMeshStreamRowPtr& Stream, int32 Index, int32 ElementIdx, FVector& Value);
	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	static const FRealtimeMeshStreamRowPtr& GetVector4Element(const FRealtimeMeshStreamRowPtr& Stream, int32 Index, int32 ElementIdx, FVector4& Value);




	
	
	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	static FRealtimeMeshLocalBuilder MakeLocalMeshBuilder(URealtimeMeshStreamSet* StreamSet,
		ERealtimeMeshSimpleStreamConfig WantedTangents = ERealtimeMeshSimpleStreamConfig::Normal,
		ERealtimeMeshSimpleStreamConfig WantedTexCoords = ERealtimeMeshSimpleStreamConfig::Normal,
		bool bWants32BitIndices = false,
		ERealtimeMeshSimpleStreamConfig WantedPolyGroupType = ERealtimeMeshSimpleStreamConfig::None,
		bool bWantsColors = true, int32 WantedTexCoordChannels = 1, bool bKeepExistingData = true);


	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	static const FRealtimeMeshLocalBuilder& EnableTangents(const FRealtimeMeshLocalBuilder& Builder, bool bUseHighPrecision = false);

	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	static const FRealtimeMeshLocalBuilder& DisableTangents(const FRealtimeMeshLocalBuilder& Builder);


	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	static const FRealtimeMeshLocalBuilder& EnableColors(const FRealtimeMeshLocalBuilder& Builder);

	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	static const FRealtimeMeshLocalBuilder& DisableColors(const FRealtimeMeshLocalBuilder& Builder);


	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	static const FRealtimeMeshLocalBuilder& EnableTexCoords(const FRealtimeMeshLocalBuilder& Builder, int32 NumChannels = 1, bool bUseHighPrecision = false);

	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	static const FRealtimeMeshLocalBuilder& DisableTexCoords(const FRealtimeMeshLocalBuilder& Builder);


	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	static const FRealtimeMeshLocalBuilder& EnableDepthOnlyTriangles(const FRealtimeMeshLocalBuilder& Builder, bool bUse32BitIndices = false);

	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	static const FRealtimeMeshLocalBuilder& DisableDepthOnlyTriangles(const FRealtimeMeshLocalBuilder& Builder);


	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	static const FRealtimeMeshLocalBuilder& EnablePolyGroups(const FRealtimeMeshLocalBuilder& Builder, bool bUse32BitIndices = false);

	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	static const FRealtimeMeshLocalBuilder& DisablePolyGroups(const FRealtimeMeshLocalBuilder& Builder);

	
	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	static const FRealtimeMeshLocalBuilder& AddTriangle(const FRealtimeMeshLocalBuilder& Builder, int32& TriangleIndex, int32 UV0, int32 UV1, int32 UV2, int32 PolyGroupIndex = 0);
	
	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	static const FRealtimeMeshLocalBuilder& SetTriangle(const FRealtimeMeshLocalBuilder& Builder, int32 Index, int32 UV0, int32 UV1, int32 UV2, int32 PolyGroupIndex = 0);
	
	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	static const FRealtimeMeshLocalBuilder& GetTriangle(const FRealtimeMeshLocalBuilder& Builder, int32 Index, int32& UV0, int32& UV1, int32& UV2, int32& PolyGroupIndex);

	
	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData", meta=(AutoCreateRefTerm=""))
	static const FRealtimeMeshLocalBuilder& AddVertex(const FRealtimeMeshLocalBuilder& Builder, int32& Index, FVector Position = FVector(0, 0, 0),
		FVector Normal = FVector(0, 0, 1), FVector Tangent = FVector(1, 0, 0), FLinearColor Color = FLinearColor::White,
		FVector2D UV0 = FVector2D(0, 0), FVector2D UV1 = FVector2D(0, 0), FVector2D UV2 = FVector2D(0, 0), FVector2D UV3 = FVector2D(0, 0));

	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	static const FRealtimeMeshLocalBuilder& EditVertex(const FRealtimeMeshLocalBuilder& Builder, int32 Index,
		FVector Position = FVector(0, 0, 0), bool bWritePosition = false,
		FVector Normal = FVector(0, 0, 1), bool bWriteNormal = false,
		FVector Tangent = FVector(1, 0, 0), bool bWriteTangent = false,
		FLinearColor Color = FLinearColor::White, bool bWriteColor = false,
		FVector2D UV0 = FVector2D(0, 0), bool bWriteUV0 = false,
		FVector2D UV1 = FVector2D(0, 0), bool bWriteUV1 = false,
		FVector2D UV2 = FVector2D(0, 0), bool bWriteUV2 = false,
		FVector2D UV3 = FVector2D(0, 0), bool bWriteUV3 = false);

	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData", meta=(AutoCreateRefTerm=""))
	static const FRealtimeMeshLocalBuilder& GetVertex(const FRealtimeMeshLocalBuilder& Builder, int32 Index, FVector& Position, FVector& Normal,
		FVector& Tangent, FLinearColor& Color, FVector2D& UV0, FVector2D& UV1, FVector2D& UV2, FVector2D& UV3);

	
};