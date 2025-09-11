// Copyright (c) 2015-2025 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreFwd.h"
#include "UObject/NameTypes.h"
#include "Math/Range.h"
#include "RealtimeMeshInterfaceFwd.h"

enum class ERealtimeMeshStreamType : uint8
{
	Unknown,
	Vertex,
	Index,
};


struct FRealtimeMeshStreamKey
{
private:
	ERealtimeMeshStreamType StreamType;	
	FName StreamName;

public:
	FRealtimeMeshStreamKey()
		: StreamType(ERealtimeMeshStreamType::Unknown)
		, StreamName(NAME_None)
	{ }

	FRealtimeMeshStreamKey(ERealtimeMeshStreamType InStreamType, FName InStreamName)
		: StreamType(InStreamType)
		, StreamName(InStreamName)
	{ }

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

struct FRealtimeMeshStreamRange
{
	FInt32Range Vertices;
	FInt32Range Indices;
	
	FRealtimeMeshStreamRange() { }

	FRealtimeMeshStreamRange(const FInt32Range& VerticesRange, const FInt32Range& IndicesRange)
		: Vertices(VerticesRange), Indices(IndicesRange) { }

	FRealtimeMeshStreamRange(uint32 VerticesLower, uint32 VerticesUpper, uint32 IndicesLower, uint32 IndicesUpper)
		: Vertices(VerticesLower, VerticesUpper), Indices(IndicesLower, IndicesUpper) { }

	FRealtimeMeshStreamRange(const FInt32RangeBound& VerticesLower, const FInt32RangeBound& VerticesUpper,
	                         const FInt32RangeBound& IndicesLower, const FInt32RangeBound& IndicesUpper)
		: Vertices(VerticesLower, VerticesUpper), Indices(IndicesLower, IndicesUpper) { }
	
	static FRealtimeMeshStreamRange Empty() { return FRealtimeMeshStreamRange(FInt32Range::Empty(), FInt32Range::Empty()); }

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

	bool Overlaps(const FRealtimeMeshStreamRange& Other) const
	{
		return Vertices.Overlaps(Other.Vertices) && Indices.Overlaps(Other.Indices);
	}

	bool Contains(const FRealtimeMeshStreamRange& Other) const
	{
		return Vertices.Contains(Other.Vertices) && Indices.Contains(Other.Indices);
	}

	FRealtimeMeshStreamRange Intersection(const FRealtimeMeshStreamRange& Other) const
	{
		return FRealtimeMeshStreamRange(
			FInt32Range::Intersection(Vertices, Other.Vertices),
			FInt32Range::Intersection(Indices, Other.Indices));
	}

	FRealtimeMeshStreamRange Hull(const FRealtimeMeshStreamRange& Other) const
	{
		return FRealtimeMeshStreamRange(FInt32Range::Hull(Vertices, Other.Vertices), FInt32Range::Hull(Indices, Other.Indices));
	}

	friend FArchive& operator<<(FArchive& Ar, FRealtimeMeshStreamRange& Range)
	{		
		Ar << Range.Vertices << Range.Indices;
		return Ar;
	}
};