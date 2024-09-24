// Copyright (c) 2015-2024 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/RealtimeMeshBuilder.h"
#include "Core/RealtimeMeshDataStream.h"
#include "Core/RealtimeMeshDataTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RealtimeMeshBlueprintMeshBuilder.generated.h"

struct FRealtimeMeshStreamKey;
class URealtimeMeshStreamSet;
class URealtimeMeshLocalBuilder;

/*
 * The data type of a stream for the blueprint interface
 */
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


USTRUCT(BlueprintType)
struct REALTIMEMESHCOMPONENT_API FRealtimeMeshBasicVertex
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="RealtimeMesh|MeshData")
	FVector Position = FVector::Zero();
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="RealtimeMesh|MeshData")
	FVector Normal = FVector::UnitZ();
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="RealtimeMesh|MeshData")
	FVector Tangent = FVector::UnitX();
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="RealtimeMesh|MeshData")
	FVector Binormal = FVector::UnitY();
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="RealtimeMesh|MeshData")
	FLinearColor Color = FLinearColor::Transparent;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="RealtimeMesh|MeshData")
	FVector2D UV0 = FVector2D::Zero();
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="RealtimeMesh|MeshData")
	FVector2D UV1 = FVector2D::Zero();
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="RealtimeMesh|MeshData")
	FVector2D UV2 = FVector2D::Zero();
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="RealtimeMesh|MeshData")
	FVector2D UV3 = FVector2D::Zero();
};


UENUM(BlueprintType)
enum class ERealtimeMeshSimpleStreamConfig : uint8
{
	None,
	Normal,
	HighPrecision
};

USTRUCT(BlueprintType)
struct REALTIMEMESHCOMPONENT_API FRealtimeMeshStreamRowPtr
{
	GENERATED_BODY()
public:
	
	UPROPERTY()
	TObjectPtr<URealtimeMeshStream> Stream;

	UPROPERTY()
	int32 RowIndex;

	FRealtimeMeshStreamRowPtr(URealtimeMeshStream* InStream = nullptr, int32 InRowIndex = INDEX_NONE)
		: Stream(InStream)
		, RowIndex(InRowIndex)
	{ }

	bool IsValid() const;
};

UCLASS(BlueprintType)
class REALTIMEMESHCOMPONENT_API URealtimeMeshStream : public UObject
{
	GENERATED_BODY()
protected:
	friend struct FRealtimeMeshStreamRowPtr;
	friend class URealtimeMeshStreamUtils;
	
	TSharedPtr<RealtimeMesh::FRealtimeMeshStream> Stream;

	TArray<RealtimeMesh::TRealtimeMeshStridedStreamBuilder<int32, void>> IntAccessors;
	TArray<RealtimeMesh::TRealtimeMeshStridedStreamBuilder<float, void>> FloatAccessors;
	TArray<RealtimeMesh::TRealtimeMeshStridedStreamBuilder<FVector2D, void>> Vector2Accessors;
	TArray<RealtimeMesh::TRealtimeMeshStridedStreamBuilder<FVector, void>> Vector3Accessors;
	TArray<RealtimeMesh::TRealtimeMeshStridedStreamBuilder<FVector4, void>> Vector4Accessors;

	void ClearAccessors();
	void SetupIntAccessors();
	void SetupFloatAccessors();
	void SetupVector2Accessors();
	void SetupVector3Accessors();
	void SetupVector4Accessors();
	
public:

	RealtimeMesh::FRealtimeMeshStream& GetStream() { return *Stream; }
	const RealtimeMesh::FRealtimeMeshStream& GetStream() const { return *Stream; }
	RealtimeMesh::FRealtimeMeshStream Consume();

	void Initialize(const FRealtimeMeshStreamKey& StreamKey, ERealtimeMeshSimpleStreamType StreamType, int32 NumElements);
	bool HasValidData() const { return Stream.IsValid(); }
	void Reset()
	{
		Stream.Reset();
	}

	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	int32 GetNum(URealtimeMeshStream*& Builder);

	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	bool IsIndexValid(URealtimeMeshStream*& Builder, int32 Index);
	
	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	bool IsEmpty(URealtimeMeshStream*& Builder);

	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	void Reserve(URealtimeMeshStream*& Builder, int32 ExpectedSize);

	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	void Shrink(URealtimeMeshStream*& Builder);
	
	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	void Empty(URealtimeMeshStream*& Builder, int32 ExpectedSize = 0);

	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	void SetNumUninitialized(URealtimeMeshStream*& Builder, int32 NewNum);
	
	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	void SetNumZeroed(URealtimeMeshStream*& Builder, int32 NewNum);

	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	int32 AddUninitialized(URealtimeMeshStream*& Builder, FRealtimeMeshStreamRowPtr& Row, int32 NumToAdd);
	
	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	int32 AddZeroed(URealtimeMeshStream*& Builder, FRealtimeMeshStreamRowPtr& Row, int32 NumToAdd);
	
	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	FRealtimeMeshStreamRowPtr EditRow(URealtimeMeshStream*& Builder, int32 Index);

	
	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	int32 AddInt(URealtimeMeshStream*& Builder, FRealtimeMeshStreamRowPtr& Row, int32 NewValue);
	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	int32 AddFloat(URealtimeMeshStream*& Builder, FRealtimeMeshStreamRowPtr& Row, float NewValue);
	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	int32 AddVector2(URealtimeMeshStream*& Builder, FRealtimeMeshStreamRowPtr& Row, FVector2D NewValue);
	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	int32 AddVector3(URealtimeMeshStream*& Builder, FRealtimeMeshStreamRowPtr& Row, FVector NewValue);
	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	int32 AddVector4(URealtimeMeshStream*& Builder, FRealtimeMeshStreamRowPtr& Row, FVector4 NewValue);

	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	void SetInt(URealtimeMeshStream*& Builder, FRealtimeMeshStreamRowPtr& Row, int32 Index, int32 NewValue);
	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	void SetFloat(URealtimeMeshStream*& Builder, FRealtimeMeshStreamRowPtr& Row, int32 Index, float NewValue);
	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	void SetVector2(URealtimeMeshStream*& Builder, FRealtimeMeshStreamRowPtr& Row, int32 Index, FVector2D NewValue);
	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	void SetVector3(URealtimeMeshStream*& Builder, FRealtimeMeshStreamRowPtr& Row, int32 Index, FVector NewValue);
	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	void SetVector4(URealtimeMeshStream*& Builder, FRealtimeMeshStreamRowPtr& Row, int32 Index, FVector4 NewValue);


	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	int32 GetInt(URealtimeMeshStream*& Builder, FRealtimeMeshStreamRowPtr& Row, int32 Index);
	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	float GetFloat(URealtimeMeshStream*& Builder, FRealtimeMeshStreamRowPtr& Row, int32 Index);
	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	FVector2D GetVector2(URealtimeMeshStream*& Builder, FRealtimeMeshStreamRowPtr& Row, int32 Index);
	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	FVector GetVector3(URealtimeMeshStream*& Builder, FRealtimeMeshStreamRowPtr& Row, int32 Index);
	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	FVector4 GetVector4(URealtimeMeshStream*& Builder, FRealtimeMeshStreamRowPtr& Row, int32 Index);
};

// ReSharper disable UnrealHeaderToolError
UCLASS(BlueprintType)
class REALTIMEMESHCOMPONENT_API URealtimeMeshStreamSet : public UObject
{
	GENERATED_BODY()
protected:
	TSharedPtr<RealtimeMesh::FRealtimeMeshStreamSet> Streams;
	virtual void EnsureInitialized();
public:
	URealtimeMeshStreamSet() = default;

	RealtimeMesh::FRealtimeMeshStreamSet& GetStreamSet() { EnsureInitialized(); return *Streams; }
	const RealtimeMesh::FRealtimeMeshStreamSet& GetStreamSet() const { const_cast<URealtimeMeshStreamSet*>(this)->EnsureInitialized(); return *Streams; }
	RealtimeMesh::FRealtimeMeshStreamSet Consume();
	
	UFUNCTION(BlueprintCallable, Category="Realtime Mesh")
	virtual void AddStream(URealtimeMeshStream* Stream);

	UFUNCTION(BlueprintCallable, Category="Realtime Mesh")
	virtual void RemoveStream(const FRealtimeMeshStreamKey& StreamKey);

	UFUNCTION(BlueprintCallable, Category="Realtime Mesh")
	virtual void Reset()
	{
		Streams.Reset();
		EnsureInitialized();
	}
	
	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	URealtimeMeshLocalBuilder* MakeLocalMeshBuilder(ERealtimeMeshSimpleStreamConfig WantedTangents = ERealtimeMeshSimpleStreamConfig::Normal,
		ERealtimeMeshSimpleStreamConfig WantedTexCoords = ERealtimeMeshSimpleStreamConfig::Normal,
		bool bWants32BitIndices = false,
		ERealtimeMeshSimpleStreamConfig WantedPolyGroupType = ERealtimeMeshSimpleStreamConfig::None,
		bool bWantsColors = true, int32 WantedTexCoordChannels = 1, bool bKeepExistingData = true);

};

// ReSharper restore UnrealHeaderToolError

UCLASS(BlueprintType)
class REALTIMEMESHCOMPONENT_API URealtimeMeshLocalBuilder : public URealtimeMeshStreamSet
{
	GENERATED_BODY()
protected:
	TUniquePtr<RealtimeMesh::TRealtimeMeshBuilderLocal<void, void, void, 1, void>> MeshBuilder;
	TUniquePtr<RealtimeMesh::TRealtimeMeshStreamBuilder<FVector2D, void>> UV1Builder;
	TUniquePtr<RealtimeMesh::TRealtimeMeshStreamBuilder<FVector2D, void>> UV2Builder;
	TUniquePtr<RealtimeMesh::TRealtimeMeshStreamBuilder<FVector2D, void>> UV3Builder;
	virtual void EnsureInitialized() override;
public:
	URealtimeMeshLocalBuilder() = default;

	void Initialize(TUniquePtr<RealtimeMesh::FRealtimeMeshStreamSet>&& InStreams,
		TUniquePtr<RealtimeMesh::TRealtimeMeshBuilderLocal<void, void, void, 1, void>>&& InBuilder,
		TUniquePtr<RealtimeMesh::TRealtimeMeshStreamBuilder<FVector2D, void>>&& InUV1Builder,
		TUniquePtr<RealtimeMesh::TRealtimeMeshStreamBuilder<FVector2D, void>>&& InUV2Builder,
		TUniquePtr<RealtimeMesh::TRealtimeMeshStreamBuilder<FVector2D, void>>&& InUV3Builder)
	{
		Streams = MakeShared<RealtimeMesh::FRealtimeMeshStreamSet>(MoveTemp(*InStreams));
		MeshBuilder = MoveTemp(InBuilder);
		UV1Builder = MoveTemp(InUV1Builder);
		UV2Builder = MoveTemp(InUV2Builder);
		UV3Builder = MoveTemp(InUV3Builder);
	}
	
	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	URealtimeMeshLocalBuilder* Initialize(ERealtimeMeshSimpleStreamConfig WantedTangents = ERealtimeMeshSimpleStreamConfig::Normal,
		ERealtimeMeshSimpleStreamConfig WantedTexCoords = ERealtimeMeshSimpleStreamConfig::Normal,
		bool bWants32BitIndices = false,
		ERealtimeMeshSimpleStreamConfig WantedPolyGroupType = ERealtimeMeshSimpleStreamConfig::None,
		bool bWantsColors = true, int32 WantedTexCoordChannels = 1, bool bKeepExistingData = true);
	

	virtual void AddStream(URealtimeMeshStream* Stream) override;
	virtual void RemoveStream(const FRealtimeMeshStreamKey& StreamKey) override;
	virtual void Reset() override;


	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	URealtimeMeshLocalBuilder* EnableTangents(bool bUseHighPrecision = false);

	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	URealtimeMeshLocalBuilder* DisableTangents();


	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	URealtimeMeshLocalBuilder* EnableColors();

	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	URealtimeMeshLocalBuilder* DisableColors();


	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	URealtimeMeshLocalBuilder* EnableTexCoords(int32 NumChannels = 1, bool bUseHighPrecision = false);

	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	URealtimeMeshLocalBuilder* DisableTexCoords();


	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	URealtimeMeshLocalBuilder* EnableDepthOnlyTriangles(bool bUse32BitIndices = false);

	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	URealtimeMeshLocalBuilder* DisableDepthOnlyTriangles();


	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	URealtimeMeshLocalBuilder* EnablePolyGroups(bool bUse32BitIndices = false);

	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	URealtimeMeshLocalBuilder* DisablePolyGroups();

	
	
	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	int32 AddTriangle(URealtimeMeshLocalBuilder*& Builder, int32 UV0, int32 UV1, int32 UV2, int32 PolyGroupIndex = 0);
	
	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	void SetTriangle(URealtimeMeshLocalBuilder*& Builder, int32 Index, int32 UV0, int32 UV1, int32 UV2, int32 PolyGroupIndex = 0);
	
	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	void GetTriangle(URealtimeMeshLocalBuilder*& Builder, int32 Index, int32& UV0, int32& UV1, int32& UV2, int32& PolyGroupIndex);

	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	int32 AddVertex(URealtimeMeshLocalBuilder*& Builder, const FRealtimeMeshBasicVertex& Vertex);
	
	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	void EditVertex(URealtimeMeshLocalBuilder*& Builder, int32 Index,
		FVector Position = FVector(0, 0, 0), bool bWritePosition = false,
		FVector Normal = FVector(0, 0, 1), bool bWriteNormal = false,
		FVector Tangent = FVector(1, 0, 0), bool bWriteTangent = false,
		FLinearColor Color = FLinearColor::White, bool bWriteColor = false,
		FVector2D UV0 = FVector2D(0, 0), bool bWriteUV0 = false,
		FVector2D UV1 = FVector2D(0, 0), bool bWriteUV1 = false,
		FVector2D UV2 = FVector2D(0, 0), bool bWriteUV2 = false,
		FVector2D UV3 = FVector2D(0, 0), bool bWriteUV3 = false);

	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	void GetVertex(URealtimeMeshLocalBuilder*& Builder, int32 Index, FVector& Position, FVector& Normal,
		FVector& Tangent, FLinearColor& Color, FVector2D& UV0, FVector2D& UV1, FVector2D& UV2, FVector2D& UV3);

	friend class URealtimeMeshStreamSet;
};




// ReSharper disable UnrealHeaderToolError

/*
 *	An object pool for reusing Realtime Mesh Streams, StreamSets, and MeshBuilders
 */
UCLASS(BlueprintType, Transient, MinimalAPI)
class URealtimeMeshStreamPool : public UObject
{
	GENERATED_BODY()
public:
	/** @return an available URealtimeMeshStream from the pool (possibly allocating a new stream) */
	UFUNCTION(BlueprintCallable, Category="Realtime Mesh")
	REALTIMEMESHCOMPONENT_API URealtimeMeshStream* RequestStream(const FRealtimeMeshStreamKey& StreamKey, ERealtimeMeshSimpleStreamType StreamType, int32 NumElements);

	/** Release a URealtimeMeshStream returned by RequestStream() back to the pool */
	UFUNCTION(BlueprintCallable, Category = "Realtime Mesh")
	REALTIMEMESHCOMPONENT_API void ReturnStream(URealtimeMeshStream* Stream);

	/** @return an available URealtimeMeshStreamSet from the pool (possibly allocating a new stream) */
	UFUNCTION(BlueprintCallable, Category="Realtime Mesh")
	REALTIMEMESHCOMPONENT_API URealtimeMeshStreamSet* RequestStreamSet();

	/** Release a URealtimeMeshStreamSet returned by RequestStreamSet() back to the pool */
	UFUNCTION(BlueprintCallable, Category = "Realtime Mesh")
	REALTIMEMESHCOMPONENT_API void ReturnStreamSet(URealtimeMeshStreamSet* StreamSet);

	/** @return an available RealtimeMeshLocalBuilder from the pool (possibly allocating a new stream) */
	UFUNCTION(BlueprintCallable, Category="Realtime Mesh")
	REALTIMEMESHCOMPONENT_API URealtimeMeshLocalBuilder* RequestMeshBuilder();

	/** Release a URealtimeMeshLocalBuilder returned by RequestMeshBuilder() back to the pool */
	UFUNCTION(BlueprintCallable, Category = "Realtime Mesh")
	REALTIMEMESHCOMPONENT_API void ReturnMeshBuilder(URealtimeMeshLocalBuilder* Builder);

	
	/** Release all Streams/StreamSets/Builders back to the pool */
	UFUNCTION(BlueprintCallable, Category = "Realtime Mesh")
	REALTIMEMESHCOMPONENT_API void ReturnAllStreams();

	/** Release all Streams/StreamSets/Builders back to the pool and allow them to be garbage collected */
	UFUNCTION(BlueprintCallable, Category = "Realtime Mesh")
	REALTIMEMESHCOMPONENT_API void FreeAllStreams();


protected:
	/** Streams in the pool that are available */
	UPROPERTY()
	TArray<TObjectPtr<URealtimeMeshStream>> CachedStreams;

	/** All streams the pool has allocated */
	UPROPERTY()
	TArray<TObjectPtr<URealtimeMeshStream>> AllCreatedStreams;
	
	/** StreamSets in the pool that are available */
	UPROPERTY()
	TArray<TObjectPtr<URealtimeMeshStreamSet>> CachedStreamSets;

	/** All stream sets the pool has allocated */
	UPROPERTY()
	TArray<TObjectPtr<URealtimeMeshStreamSet>> AllCreatedStreamSets;
	
	/** StreamSets in the pool that are available */
	UPROPERTY()
	TArray<TObjectPtr<URealtimeMeshLocalBuilder>> CachedBuilders;

	/** All stream sets the pool has allocated */
	UPROPERTY()
	TArray<TObjectPtr<URealtimeMeshLocalBuilder>> AllCreatedBuilders;
};

// ReSharper restore UnrealHeaderToolError



USTRUCT(BlueprintType)
struct REALTIMEMESHCOMPONENT_API FRealtimeMeshStreamSetFromComponents
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="RealtimeMesh|MeshData")
	TArray<int32> Triangles;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="RealtimeMesh|MeshData")
	TArray<int32> PolyGroups;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="RealtimeMesh|MeshData")
	TArray<FVector> Positions;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="RealtimeMesh|MeshData")
	TArray<FVector> Normals;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="RealtimeMesh|MeshData")
	TArray<FVector> Tangents;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="RealtimeMesh|MeshData")
	TArray<FVector> Binormals;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="RealtimeMesh|MeshData")
	TArray<FLinearColor> Colors;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="RealtimeMesh|MeshData")
	TArray<FVector2D> UV0;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="RealtimeMesh|MeshData")
	TArray<FVector2D> UV1;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="RealtimeMesh|MeshData")
	TArray<FVector2D> UV2;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="RealtimeMesh|MeshData")
	TArray<FVector2D> UV3;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="RealtimeMesh|MeshData")
	bool bUse32BitIndices = false;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="RealtimeMesh|MeshData")
	bool bUseHighPrecisionTangents = false;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="RealtimeMesh|MeshData")
	bool bUseHighPrecisionTexCoords = false;
};




UCLASS(meta=(ScriptName="RealtimeMeshStreamUtils"))
class REALTIMEMESHCOMPONENT_API URealtimeMeshStreamUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:

	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	static URealtimeMeshStreamSet* CopyStreamSetFromComponents(URealtimeMeshStreamSet* Streams, const FRealtimeMeshStreamSetFromComponents& Components);

	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	static const FRealtimeMeshStreamRowPtr& SetIntElement(const FRealtimeMeshStreamRowPtr& Row, int32 Index, int32 ElementIdx, int32 NewValue);
	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	static const FRealtimeMeshStreamRowPtr& SetFloatElement(const FRealtimeMeshStreamRowPtr& Row, int32 Index, int32 ElementIdx, float NewValue);
	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	static const FRealtimeMeshStreamRowPtr& SetVector2Element(const FRealtimeMeshStreamRowPtr& Row, int32 Index, int32 ElementIdx, FVector2D NewValue);
	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	static const FRealtimeMeshStreamRowPtr& SetVector3Element(const FRealtimeMeshStreamRowPtr& Row, int32 Index, int32 ElementIdx, FVector NewValue);
	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	static const FRealtimeMeshStreamRowPtr& SetVector4Element(const FRealtimeMeshStreamRowPtr& Row, int32 Index, int32 ElementIdx, FVector4 NewValue);


	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	static int32 GetIntElement(const FRealtimeMeshStreamRowPtr& Row, int32 Index, int32 ElementIdx, FRealtimeMeshStreamRowPtr& OutRow);
	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	static float GetFloatElement(const FRealtimeMeshStreamRowPtr& Row, int32 Index, int32 ElementIdx, FRealtimeMeshStreamRowPtr& OutRow);
	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	static FVector2D GetVector2Element(const FRealtimeMeshStreamRowPtr& Row, int32 Index, int32 ElementIdx, FRealtimeMeshStreamRowPtr& OutRow);
	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	static FVector GetVector3Element(const FRealtimeMeshStreamRowPtr& Row, int32 Index, int32 ElementIdx, FRealtimeMeshStreamRowPtr& OutRow);
	UFUNCTION(BlueprintCallable, Category="RealtimeMesh|MeshData")
	static FVector4 GetVector4Element(const FRealtimeMeshStreamRowPtr& Row, int32 Index, int32 ElementIdx, FRealtimeMeshStreamRowPtr& OutRow);
};