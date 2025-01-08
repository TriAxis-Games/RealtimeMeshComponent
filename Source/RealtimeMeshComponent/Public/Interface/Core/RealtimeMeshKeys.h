// Copyright (c) 2015-2025 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "RealtimeMeshInterfaceFwd.h"

struct FRealtimeMeshLODKey
{
private:
	int8 LODIndex;

public:
	FRealtimeMeshLODKey() : LODIndex(INDEX_NONE) { }
	FRealtimeMeshLODKey(int32 InLODIndex) : LODIndex(InLODIndex) { }

	operator int32() const { return LODIndex; }
	int32 Index() const { return LODIndex; }

	FORCEINLINE bool operator==(const FRealtimeMeshLODKey& Other) const
	{
		return LODIndex == Other.LODIndex;
	}

	FORCEINLINE bool operator!=(const FRealtimeMeshLODKey& Other) const
	{
		return LODIndex != Other.LODIndex;
	}

	friend FORCEINLINE uint32 GetTypeHash(const FRealtimeMeshLODKey& LOD)
	{
		return GetTypeHash(LOD.LODIndex);;
	}
	
	FString ToString() const { return TEXT("[LODKey:") + FString::FromInt(LODIndex) + TEXT("]"); }

	friend struct FRealtimeMeshSectionGroupKey;
	friend struct FRealtimeMeshSectionKey;
	friend FArchive& operator<<(FArchive& Ar, FRealtimeMeshLODKey& Key);
};

struct FRealtimeMeshSectionGroupKey
{
private:
	FName GroupName;
	int8 LODIndex;

	FRealtimeMeshSectionGroupKey(uint32 InLODIndex, FName InGroupName)
		: GroupName(InGroupName)
		, LODIndex(InLODIndex)
	{ }

public:
	FRealtimeMeshSectionGroupKey()
		: GroupName(NAME_None)
		, LODIndex(INDEX_NONE)
	{ }

	FORCEINLINE bool IsValid() const { return LODIndex >= 0 && LODIndex < 8 && GroupName != NAME_None; }

	FORCEINLINE const FName& Name() const { return GroupName; }

	FORCEINLINE FRealtimeMeshLODKey LOD() const { return FRealtimeMeshLODKey(LODIndex); }

	FORCEINLINE operator FRealtimeMeshLODKey() const { return FRealtimeMeshLODKey(LODIndex); }

	FORCEINLINE bool IsPartOf(const FRealtimeMeshLODKey& InLOD) const
	{
		return LOD() == InLOD;
	}

	FORCEINLINE bool operator==(const FRealtimeMeshSectionGroupKey& Other) const
	{
		return LODIndex == Other.LODIndex && GroupName == Other.GroupName;
	}

	FORCEINLINE bool operator!=(const FRealtimeMeshSectionGroupKey& Other) const
	{
		return LODIndex != Other.LODIndex || GroupName != Other.GroupName;
	}

	friend FORCEINLINE uint32 GetTypeHash(const FRealtimeMeshSectionGroupKey& Group)
	{
		return HashCombine(GetTypeHash(Group.GroupName), GetTypeHash(Group.LODIndex));
	}


	FString ToString() const { return TEXT("[LODKey:") + FString::FromInt(LODIndex) + TEXT(", SectionGroupKey:") + GroupName.ToString() + TEXT("]"); }

	static FRealtimeMeshSectionGroupKey Create(const FRealtimeMeshLODKey& LODKey, FName GroupName)
	{		
		return FRealtimeMeshSectionGroupKey(LODKey, GroupName);
	}
	static FRealtimeMeshSectionGroupKey Create(const FRealtimeMeshLODKey& LODKey, int32 GroupID)
	{		
		return FRealtimeMeshSectionGroupKey(LODKey, FName("Group_", GroupID));
	}
	static FRealtimeMeshSectionGroupKey CreateUnique(const FRealtimeMeshLODKey& LODKey)
	{		
		return FRealtimeMeshSectionGroupKey(LODKey, FName("Group_" + FGuid::NewGuid().ToString()));
	}

	friend struct FRealtimeMeshSectionKey;
	friend FArchive& operator<<(FArchive& Ar, FRealtimeMeshSectionGroupKey& Key);
};

struct FRealtimeMeshSectionKey
{
private:
	FName GroupName;
	FName SectionName;
	int8 LODIndex;

	FRealtimeMeshSectionKey(uint32 InLODIndex, FName InGroupName, FName InSectionName)
		: GroupName(InGroupName)
		, SectionName(InSectionName)
		, LODIndex(InLODIndex)
	{ }

public:
	FRealtimeMeshSectionKey()
		: GroupName(NAME_None)
		, SectionName(NAME_None)
		, LODIndex(INDEX_NONE)
	{ }

	FORCEINLINE bool IsValid() const { return LODIndex >= 0 && LODIndex < 8 && GroupName != NAME_None && SectionName != NAME_None; }

	FORCEINLINE const FName& Name() const { return SectionName; }

	FORCEINLINE FRealtimeMeshLODKey LOD() const { return FRealtimeMeshLODKey(LODIndex); }
	FORCEINLINE FRealtimeMeshSectionGroupKey SectionGroup() const { return FRealtimeMeshSectionGroupKey(LODIndex, GroupName); }

	FORCEINLINE operator FRealtimeMeshLODKey() const { return LOD(); }
	FORCEINLINE operator FRealtimeMeshSectionGroupKey() const { return SectionGroup(); }

	FORCEINLINE bool IsPartOf(const FRealtimeMeshLODKey& InLOD) const
	{
		return LOD() == InLOD;
	}

	FORCEINLINE bool IsPartOf(const FRealtimeMeshSectionGroupKey& InSectionGroup) const
	{
		return SectionGroup() == InSectionGroup;
	}

	FORCEINLINE bool IsPolyGroupKey() const
	{
		return GroupName.ToString().StartsWith("Section_PolyGroup");		
	}
	
	FORCEINLINE bool operator==(const FRealtimeMeshSectionKey& Other) const
	{
		return LODIndex == Other.LODIndex && GroupName == Other.GroupName && SectionName == Other.SectionName;
	}

	FORCEINLINE bool operator!=(const FRealtimeMeshSectionKey& Other) const
	{
		return LODIndex != Other.LODIndex || GroupName != Other.GroupName || SectionName != Other.SectionName;
	}

	friend FORCEINLINE uint32 GetTypeHash(const FRealtimeMeshSectionKey& Section)
	{
		return HashCombine(GetTypeHash(Section.SectionName), HashCombine(GetTypeHash(Section.GroupName), GetTypeHash(Section.LODIndex)));
	}

	FString ToString() const
	{
		return TEXT("[LODKey:") + FString::FromInt(LODIndex) + TEXT(", SectionGroupKey:") + GroupName.ToString() + TEXT(", SectionKey:") + SectionName.ToString() + TEXT("]");
	}

	static FRealtimeMeshSectionKey Create(const FRealtimeMeshSectionGroupKey& SectionGroupKey, FName SectionName)
	{
		return FRealtimeMeshSectionKey(SectionGroupKey.LOD(), SectionGroupKey.GroupName, SectionName);		
	}
	
	static FRealtimeMeshSectionKey Create(const FRealtimeMeshSectionGroupKey& SectionGroupKey, int32 SectionID)
	{
		return FRealtimeMeshSectionKey(SectionGroupKey.LOD(), SectionGroupKey.GroupName, FName("Section_", SectionID));		
	}
	
	static FRealtimeMeshSectionKey CreateUnique(const FRealtimeMeshSectionGroupKey& SectionGroupKey)
	{
		return FRealtimeMeshSectionKey(SectionGroupKey.LOD(), SectionGroupKey.GroupName, FName("Section_" + FGuid::NewGuid().ToString()));		
	}
	
	static FRealtimeMeshSectionKey CreateForPolyGroup(const FRealtimeMeshSectionGroupKey& SectionGroupKey, int32 PolyGroup)
	{
		return FRealtimeMeshSectionKey(SectionGroupKey.LOD(), SectionGroupKey.GroupName, FName("Section_PolyGroup", PolyGroup));		
	}

	friend FArchive& operator<<(FArchive& Ar, FRealtimeMeshSectionKey& Key);
};