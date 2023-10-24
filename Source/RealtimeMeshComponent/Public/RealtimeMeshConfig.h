// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "RealtimeMeshCore.h"
#include "RealtimeMeshConfig.generated.h"

#define LOCTEXT_NAMESPACE "RealtimeMesh"


USTRUCT(BlueprintType, meta=(HasNativeMake="RealtimeMeshComponent.RealtimeMeshBlueprintFunctionLibrary.MakeStreamRange"))
struct REALTIMEMESHCOMPONENT_API FRealtimeMeshStreamRange
{
	GENERATED_BODY()

public:
	static const FRealtimeMeshStreamRange Empty;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RealtimeMesh|Streams")
	FInt32Range Vertices;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RealtimeMesh|Streams")
	FInt32Range Indices;

	FRealtimeMeshStreamRange() = default;

	FRealtimeMeshStreamRange(const FInt32Range& VerticesRange, const FInt32Range& IndicesRange)
		: Vertices(VerticesRange), Indices(IndicesRange)
	{
	}

	FRealtimeMeshStreamRange(uint32 VerticesLower, uint32 VerticesUpper, uint32 IndicesLower, uint32 IndicesUpper)
		: Vertices(VerticesLower, VerticesUpper), Indices(IndicesLower, IndicesUpper)
	{
	}

	FRealtimeMeshStreamRange(const FInt32RangeBound& VerticesLower, const FInt32RangeBound& VerticesUpper,
	                         const FInt32RangeBound& IndicesLower, const FInt32RangeBound& IndicesUpper)
		: Vertices(VerticesLower, VerticesUpper), Indices(IndicesLower, IndicesUpper)
	{
	}

	bool Overlaps(const FRealtimeMeshStreamRange& Other) const
	{
		return Vertices.Overlaps(Other.Vertices) && Indices.Overlaps(Other.Indices);
	}

	FRealtimeMeshStreamRange Intersection(const FRealtimeMeshStreamRange& Other) const
	{
		return FRealtimeMeshStreamRange(
			FInt32Range::Intersection(Vertices, Other.Vertices),
			FInt32Range::Intersection(Indices, Other.Indices));
	}

	bool Contains(const FRealtimeMeshStreamRange& Other) const
	{
		return Vertices.Contains(Other.Vertices) && Indices.Contains(Other.Indices);
	}

	int32 NumPrimitives(int32 VertexCountPerPrimitive) const
	{
		if (!Indices.HasLowerBound() || !Indices.HasUpperBound() || Indices.IsDegenerate())
		{
			return 0;
		}
		return (GetMaxIndex() - GetMinIndex() + 1) / VertexCountPerPrimitive;
	}

	int32 NumVertices() const
	{
		if (!Vertices.HasLowerBound() || !Vertices.HasUpperBound() || Vertices.IsDegenerate())
		{
			return 0;
		}
		return GetMaxVertex() - GetMinVertex() + 1;
	}

	int32 GetMinVertex() const { return Vertices.GetLowerBound().IsExclusive() ? Vertices.GetLowerBoundValue() + 1 : Vertices.GetLowerBoundValue(); }
	int32 GetMaxVertex() const { return Vertices.GetUpperBound().IsExclusive() ? Vertices.GetUpperBoundValue() - 1 : Vertices.GetUpperBoundValue(); }

	int32 GetMinIndex() const { return Indices.GetLowerBound().IsExclusive() ? Indices.GetLowerBoundValue() + 1 : Indices.GetLowerBoundValue(); }
	int32 GetMaxIndex() const { return Indices.GetUpperBound().IsExclusive() ? Indices.GetUpperBoundValue() - 1 : Indices.GetUpperBoundValue(); }


	bool operator==(const FRealtimeMeshStreamRange& Other) const
	{
		return Vertices == Other.Vertices && Indices == Other.Indices;
	}

	bool operator!=(const FRealtimeMeshStreamRange& Other) const
	{
		return Vertices != Other.Vertices || Indices != Other.Indices;
	}

	FRealtimeMeshStreamRange Hull(const FRealtimeMeshStreamRange& Other) const
	{
		return FRealtimeMeshStreamRange(FInt32Range::Hull(Vertices, Other.Vertices), FInt32Range::Hull(Indices, Other.Indices));
	}

	friend FArchive& operator<<(FArchive& Ar, FRealtimeMeshStreamRange& Range);
};

USTRUCT(BlueprintType)
struct REALTIMEMESHCOMPONENT_API FRealtimeMeshMaterialSlot
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FName SlotName;

	UPROPERTY()
	UMaterialInterface* Material;

	FRealtimeMeshMaterialSlot() : SlotName(NAME_None), Material(nullptr)
	{
	}

	FRealtimeMeshMaterialSlot(const FName& InSlotName, UMaterialInterface* InMaterial)
		: SlotName(InSlotName), Material(InMaterial)
	{
	}
};

/* The rendering path to use for this section.
 * Static has lower overhead but requires a proxy recreation on change for all components
 * Dynamic has slightly higher overhead but allows for more efficient section updates
 */
UENUM(BlueprintType)
enum class ERealtimeMeshSectionDrawType : uint8
{
	Static,
	Dynamic,
};

USTRUCT(BlueprintType)
struct REALTIMEMESHCOMPONENT_API FRealtimeMeshSectionConfig
{
	GENERATED_BODY()

public:
	FRealtimeMeshSectionConfig(ERealtimeMeshSectionDrawType InDrawType = ERealtimeMeshSectionDrawType::Static, int32 InMaterialSlot = 0)
		: MaterialSlot(InMaterialSlot)
		  , DrawType(InDrawType)
		  , bIsVisible(true)
		  , bCastsShadow(true)
		  , bIsMainPassRenderable(true)
		  , bForceOpaque(false)
	{
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RealtimeMesh|Section|Config")
	int32 MaterialSlot;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RealtimeMesh|Section|Config")
	ERealtimeMeshSectionDrawType DrawType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RealtimeMesh|Section|Config")
	bool bIsVisible;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RealtimeMesh|Section|Config")
	bool bCastsShadow;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RealtimeMesh|Section|Config", AdvancedDisplay)
	bool bIsMainPassRenderable;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RealtimeMesh|Section|Config", AdvancedDisplay)
	bool bForceOpaque;

	friend FArchive& operator<<(FArchive& Ar, FRealtimeMeshSectionConfig& Config);
};

USTRUCT(BlueprintType)
struct REALTIMEMESHCOMPONENT_API FRealtimeMeshLODConfig
{
	GENERATED_BODY()

public:
	FRealtimeMeshLODConfig(float InScreenSize = 0.0f)
		: bIsVisible(true)
		  , ScreenSize(InScreenSize)
	{
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RealtimeMesh|LOD|Config")
	bool bIsVisible;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RealtimeMesh|LOD|Config")
	float ScreenSize;

	friend FArchive& operator<<(FArchive& Ar, FRealtimeMeshLODConfig& Config);
};

USTRUCT(BlueprintType)
struct REALTIMEMESHCOMPONENT_API FRealtimeMeshConfig
{
	GENERATED_BODY()

public:
	FRealtimeMeshConfig()
		: ForcedLOD(INDEX_NONE)
	{
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RealtimeMesh|LOD|Config")
	int32 ForcedLOD;

	friend FArchive& operator<<(FArchive& Ar, FRealtimeMeshConfig& Config);
};

USTRUCT(BlueprintType)
struct REALTIMEMESHCOMPONENT_API FRealtimeMeshLODKey
{
	GENERATED_BODY()

private:
	int8 LODIndex;

public:
	FRealtimeMeshLODKey() : LODIndex(INDEX_NONE)
	{
	}

	FRealtimeMeshLODKey(int32 InLODIndex) : LODIndex(InLODIndex)
	{
	}

	operator int32() const { return LODIndex; }
	int32 Index() const { return LODIndex; }

	bool operator==(const FRealtimeMeshLODKey& Other) const
	{
		return LODIndex == Other.LODIndex;
	}

	bool operator!=(const FRealtimeMeshLODKey& Other) const
	{
		return LODIndex != Other.LODIndex;
	}

	friend FORCEINLINE uint32 GetTypeHash(const FRealtimeMeshLODKey& LOD)
	{
		return GetTypeHash(LOD.LODIndex);;
	}

	friend FORCEINLINE FArchive& operator<<(FArchive& Ar, FRealtimeMeshLODKey& Key);

	FString ToString() const { return TEXT("[LODKey:") + FString::FromInt(LODIndex) + TEXT("]"); }

	friend struct FRealtimeMeshSectionGroupKey;
	friend struct FRealtimeMeshSectionKey;
};

USTRUCT(BlueprintType)
struct REALTIMEMESHCOMPONENT_API FRealtimeMeshSectionGroupKey
{
	GENERATED_BODY()

private:
	FName GroupName;
	int8 LODIndex;

	FRealtimeMeshSectionGroupKey(uint32 InLODIndex, FName InGroupName)
		: GroupName(InGroupName)
		  , LODIndex(InLODIndex)
	{
	}

public:
	FRealtimeMeshSectionGroupKey() : GroupName(NAME_None), LODIndex(INDEX_NONE)
	{
	}

	bool IsValid() const { return LODIndex >= 0 && LODIndex < 8 && GroupName != NAME_None; }

	const FName& Name() const { return GroupName; }

	FRealtimeMeshLODKey LOD() const { return FRealtimeMeshLODKey(LODIndex); }

	operator FRealtimeMeshLODKey() const { return FRealtimeMeshLODKey(LODIndex); }

	bool IsPartOf(const FRealtimeMeshLODKey& InLOD) const
	{
		return LOD() == InLOD;
	}

	bool operator==(const FRealtimeMeshSectionGroupKey& Other) const
	{
		return LODIndex == Other.LODIndex && GroupName == Other.GroupName;
	}

	bool operator!=(const FRealtimeMeshSectionGroupKey& Other) const
	{
		return LODIndex != Other.LODIndex || GroupName != Other.GroupName;
	}

	friend FORCEINLINE uint32 GetTypeHash(const FRealtimeMeshSectionGroupKey& Group)
	{
		return HashCombine(GetTypeHash(Group.GroupName), GetTypeHash(Group.LODIndex));
	}

	friend FORCEINLINE FArchive& operator<<(FArchive& Ar, FRealtimeMeshSectionGroupKey& Key);

	FString ToString() const { return TEXT("[LODKey:") + FString::FromInt(LODIndex) + TEXT(", SectionGroupKey:") + GroupName.ToString() + TEXT("]"); }

	static FRealtimeMeshSectionGroupKey Create(const FRealtimeMeshLODKey& LODKey, FName GroupName);
	static FRealtimeMeshSectionGroupKey Create(const FRealtimeMeshLODKey& LODKey, int32 GroupID);
	static FRealtimeMeshSectionGroupKey CreateUnique(const FRealtimeMeshLODKey& LODKey);

	friend struct FRealtimeMeshSectionKey;
};

USTRUCT(BlueprintType)
struct REALTIMEMESHCOMPONENT_API FRealtimeMeshSectionKey
{
	GENERATED_BODY()

private:
	FName GroupName;
	FName SectionName;
	int8 LODIndex;

	FRealtimeMeshSectionKey(uint32 InLODIndex, FName InGroupName, FName InSectionName)
		: GroupName(InGroupName)
		  , SectionName(InSectionName)
		  , LODIndex(InLODIndex)
	{
	}

public:
	FRealtimeMeshSectionKey() : GroupName(NAME_None), SectionName(NAME_None), LODIndex(INDEX_NONE)
	{
	}

	bool IsValid() const { return LODIndex >= 0 && LODIndex < 8 && GroupName != NAME_None && SectionName != NAME_None; }

	const FName& Name() const { return SectionName; }

	FRealtimeMeshLODKey LOD() const { return FRealtimeMeshLODKey(LODIndex); }
	FRealtimeMeshSectionGroupKey SectionGroup() const { return FRealtimeMeshSectionGroupKey(LODIndex, GroupName); }

	operator FRealtimeMeshLODKey() const { return LOD(); }
	operator FRealtimeMeshSectionGroupKey() const { return SectionGroup(); }

	bool IsPartOf(const FRealtimeMeshLODKey& InLOD) const
	{
		return LOD() == InLOD;
	}

	bool IsPartOf(const FRealtimeMeshSectionGroupKey& InSectionGroup) const
	{
		return SectionGroup() == InSectionGroup;
	}

	bool IsPolyGroupKey() const;
	
	bool operator==(const FRealtimeMeshSectionKey& Other) const
	{
		return LODIndex == Other.LODIndex && GroupName == Other.GroupName && SectionName == Other.SectionName;
	}

	bool operator!=(const FRealtimeMeshSectionKey& Other) const
	{
		return LODIndex != Other.LODIndex || GroupName != Other.GroupName || SectionName != Other.SectionName;
	}

	friend FORCEINLINE uint32 GetTypeHash(const FRealtimeMeshSectionKey& Section)
	{
		return HashCombine(GetTypeHash(Section.SectionName), HashCombine(GetTypeHash(Section.GroupName), GetTypeHash(Section.LODIndex)));
	}

	friend FORCEINLINE FArchive& operator<<(FArchive& Ar, FRealtimeMeshSectionKey& Key);

	FString ToString() const
	{
		return TEXT("[LODKey:") + FString::FromInt(LODIndex) + TEXT(", SectionGroupKey:") + GroupName.ToString() + TEXT(", SectionKey:") + SectionName.ToString() + TEXT("]");
	}

	static FRealtimeMeshSectionKey Create(const FRealtimeMeshSectionGroupKey& SectionGroupKey, FName SectionName);
	static FRealtimeMeshSectionKey Create(const FRealtimeMeshSectionGroupKey& SectionGroupKey, int32 SectionID);
	static FRealtimeMeshSectionKey CreateUnique(const FRealtimeMeshSectionGroupKey& SectionGroupKey);
	static FRealtimeMeshSectionKey CreateForPolyGroup(const FRealtimeMeshSectionGroupKey& SectionGroupKey, int32 PolyGroup);
};

UENUM()
enum class ERealtimeMeshStreamType : uint8
{
	Unknown,
	Vertex,
	Index,
};


USTRUCT(BlueprintType,  meta=(HasNativeMake="RealtimeMeshComponent.RealtimeMeshBlueprintFunctionLibrary.MakeStreamKey"))
struct FRealtimeMeshStreamKey
{
	GENERATED_BODY()
private:
	UPROPERTY(VisibleAnywhere, Category="RealtimeMesh|Key")
	ERealtimeMeshStreamType StreamType;
	
	UPROPERTY(VisibleAnywhere, Category="RealtimeMesh|Key")
	FName StreamName;

public:
	FRealtimeMeshStreamKey() : StreamType(ERealtimeMeshStreamType::Unknown), StreamName(NAME_None)
	{
	}

	FRealtimeMeshStreamKey(ERealtimeMeshStreamType InStreamType, FName InStreamName)
		: StreamType(InStreamType), StreamName(InStreamName)
	{
	}

	FName GetName() const { return StreamName; }

	ERealtimeMeshStreamType GetStreamType() const { return StreamType; }
	bool IsVertexStream() const { return StreamType == ERealtimeMeshStreamType::Vertex; }
	bool IsIndexStream() const { return StreamType == ERealtimeMeshStreamType::Index; }

	FORCEINLINE bool operator==(const FRealtimeMeshStreamKey& Other) const { return StreamType == Other.StreamType && StreamName == Other.StreamName; }
	FORCEINLINE bool operator!=(const FRealtimeMeshStreamKey& Other) const { return StreamType != Other.StreamType || StreamName != Other.StreamName; }

	friend inline uint32 GetTypeHash(const FRealtimeMeshStreamKey& StreamKey)
	{
		return GetTypeHashHelper(StreamKey.StreamType) + 23 * GetTypeHashHelper(StreamKey.StreamName);
	}

	FString ToString() const
	{
		FString TypeString;

		switch (StreamType)
		{
		case ERealtimeMeshStreamType::Unknown:
			TypeString += "Unknown";
			break;
		case ERealtimeMeshStreamType::Vertex:
			TypeString += "Vertex";
			break;
		case ERealtimeMeshStreamType::Index:
			TypeString += "Index";
			break;
		}

		return TEXT("[") + StreamName.ToString() + TEXT(", Type:") + TypeString + TEXT("]");
	}

	friend FArchive& operator<<(FArchive& Ar, FRealtimeMeshStreamKey& Key);
};



#undef LOCTEXT_NAMESPACE
