// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "RealtimeMeshCoreFwd.h"
#include "Misc/Crc.h"
#include "UObject/ObjectMacros.h"
#include "RealtimeMeshKeys.generated.h"

USTRUCT(BlueprintType)
struct FRealtimeMeshLODKey
{
	GENERATED_BODY()

private:
	UPROPERTY(VisibleAnywhere, Category="RealtimeMesh|Key")
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
		return GetTypeHash(LOD.LODIndex);
	}

	FString ToString() const { return TEXT("[LODKey:") + FString::FromInt(LODIndex) + TEXT("]"); }

	friend struct FRealtimeMeshBufferSetKey;
	friend struct FRealtimeMeshSectionKey;
	friend REALTIMEMESHCOMPONENT_API FArchive& operator<<(FArchive& Ar, FRealtimeMeshLODKey& Key);
};

/**
 * Identifies a buffer set inside a LOD. (The canonical C++ type retains the
 * older "SectionGroupKey" name to preserve the existing Blueprint API surface
 * — both UHT-generated Blueprint nodes and the UFUNCTION/UPROPERTY metadata
 * lock that name in. New C++ code is encouraged to use the more accurate
 * "BufferSet" terminology via the FRealtimeMeshBufferSetKey alias below.)
 *
 * Identity is the (LODIndex, SlotIndex) pair. The optional Name is friendly
 * metadata that callers can use for debugging, lookups, or migrating from the
 * older FName-only API; it does NOT participate in equality or hashing.
 *
 * In new code, callers supply SlotIndex explicitly (similar to UStaticMesh
 * material slot indices). For legacy callers that only have a FName, the
 * static Create(LODKey, FName) factory derives a stable SlotIndex via
 * FCrc::StrCrc32 so old code keeps working without modification. The mapping
 * is deterministic and stable across builds, so a name-based key and an
 * explicit-Index key matching that hash will compare equal — which is
 * intentional, so legacy and new code can interop on the same buffer set.
 */
USTRUCT(BlueprintType)
struct FRealtimeMeshBufferSetKey
{
	GENERATED_BODY()

private:
	UPROPERTY(VisibleAnywhere, Category="RealtimeMesh|Key")
	int8 LODIndex;

	UPROPERTY(VisibleAnywhere, Category="RealtimeMesh|Key")
	int32 SlotIndex;

	UPROPERTY(VisibleAnywhere, Category="RealtimeMesh|Key")
	FName SlotName;

	FRealtimeMeshBufferSetKey(int32 InLODIndex, int32 InSlotIndex, FName InSlotName)
		: LODIndex(InLODIndex)
		, SlotIndex(InSlotIndex)
		, SlotName(InSlotName)
	{ }

public:
	FRealtimeMeshBufferSetKey()
		: LODIndex(INDEX_NONE)
		, SlotIndex(INDEX_NONE)
		, SlotName(NAME_None)
	{ }

	FORCEINLINE bool IsValid() const { return LODIndex >= 0 && LODIndex < REALTIME_MESH_MAX_LODS && SlotIndex >= 0; }

	FORCEINLINE int32 Index() const { return SlotIndex; }
	FORCEINLINE const FName& Name() const { return SlotName; }

	// Legacy alias for code that used the previous "GroupName" accessor.
	FORCEINLINE const FName& GroupName() const { return SlotName; }

	FORCEINLINE FRealtimeMeshLODKey LOD() const { return FRealtimeMeshLODKey(LODIndex); }
	FORCEINLINE operator FRealtimeMeshLODKey() const { return FRealtimeMeshLODKey(LODIndex); }

	FORCEINLINE bool IsPartOf(const FRealtimeMeshLODKey& InLOD) const
	{
		return LOD() == InLOD;
	}

	// Identity by (LODIndex, SlotIndex). Name is metadata.
	FORCEINLINE bool operator==(const FRealtimeMeshBufferSetKey& Other) const
	{
		return LODIndex == Other.LODIndex && SlotIndex == Other.SlotIndex;
	}

	FORCEINLINE bool operator!=(const FRealtimeMeshBufferSetKey& Other) const
	{
		return !(*this == Other);
	}

	friend FORCEINLINE uint32 GetTypeHash(const FRealtimeMeshBufferSetKey& Key)
	{
		return HashCombine(GetTypeHash(Key.SlotIndex), GetTypeHash(Key.LODIndex));
	}

	FString ToString() const
	{
		return TEXT("[LODKey:") + FString::FromInt(LODIndex)
			+ TEXT(", BufferSetKey:") + FString::FromInt(SlotIndex)
			+ (SlotName != NAME_None ? (TEXT("(") + SlotName.ToString() + TEXT(")")) : FString())
			+ TEXT("]");
	}

	// Primary new-API factory: caller supplies the slot Index explicitly.
	static FRealtimeMeshBufferSetKey Create(const FRealtimeMeshLODKey& LODKey, int32 SlotIndex, FName SlotName = NAME_None)
	{
		return FRealtimeMeshBufferSetKey(LODKey, SlotIndex, SlotName);
	}

	// Legacy factory — derives a stable slot Index from the name's CRC32 so old
	// FName-based callers continue to work. The CRC is on the wide-string form
	// so the mapping is stable across runs/builds.
	static FRealtimeMeshBufferSetKey Create(const FRealtimeMeshLODKey& LODKey, FName GroupName)
	{
		const FString NameString = GroupName.ToString();
		const int32 Slot = static_cast<int32>(FCrc::StrCrc32(*NameString));
		return FRealtimeMeshBufferSetKey(LODKey, Slot, GroupName);
	}

	static FRealtimeMeshBufferSetKey CreateUnique(const FRealtimeMeshLODKey& LODKey)
	{
		const FName UniqueName = FName(*(TEXT("Group_") + FGuid::NewGuid().ToString()));
		return Create(LODKey, UniqueName);
	}

	friend struct FRealtimeMeshSectionKey;
	friend REALTIMEMESHCOMPONENT_API FArchive& operator<<(FArchive& Ar, FRealtimeMeshBufferSetKey& Key);
};

// Legacy name from the pre-BufferSet terminology, kept as a source-compatibility
// alias (Blueprint graphs are covered by a CoreRedirect). New code should use
// FRealtimeMeshBufferSetKey; this alias will be removed in a future release.
using FRealtimeMeshSectionGroupKey = FRealtimeMeshBufferSetKey;

/**
 * Identifies a section inside a LOD.
 *
 * Identity is (LODIndex, Index). Name is friendly metadata.
 *
 * Unlike the old design, a section's key does NOT carry its parent buffer set.
 * That association lives on the section object (FRealtimeMeshSectionProxy and
 * FRealtimeMeshSection) as a mutable BufferSetIndex, so a section can be
 * rebound to a different buffer set at runtime without changing identity.
 *
 * Legacy callers that used Create(SectionGroupKey, FName) still compile; the
 * resulting key carries the Index derived from the name (so equality works
 * across legacy and new code), and the calling layer is responsible for
 * setting the section's BufferSetIndex to the group key's Index when creating
 * the section on the LOD.
 */
USTRUCT(BlueprintType)
struct FRealtimeMeshSectionKey
{
	GENERATED_BODY()

private:
	UPROPERTY(VisibleAnywhere, Category="RealtimeMesh|Key")
	int8 LODIndex;

	UPROPERTY(VisibleAnywhere, Category="RealtimeMesh|Key")
	int32 SlotIndex;

	UPROPERTY(VisibleAnywhere, Category="RealtimeMesh|Key")
	FName SlotName;

	// Back-compat: the buffer-set slot the section was created against. The
	// CANONICAL runtime binding lives on the section object (proxy/data) as a
	// mutable BufferSetIndex and may differ from this if the section has been
	// rebound. This field exists so legacy callers that do
	// SectionKey.SectionGroup() to look up the section's group at creation
	// time keep working. INDEX_NONE for keys constructed without a group.
	UPROPERTY(VisibleAnywhere, Category="RealtimeMesh|Key")
	int32 BufferSetSlotIndex;

	FRealtimeMeshSectionKey(int32 InLODIndex, int32 InSlotIndex, FName InSlotName, int32 InBufferSetSlotIndex)
		: LODIndex(InLODIndex)
		, SlotIndex(InSlotIndex)
		, SlotName(InSlotName)
		, BufferSetSlotIndex(InBufferSetSlotIndex)
	{ }

public:
	FRealtimeMeshSectionKey()
		: LODIndex(INDEX_NONE)
		, SlotIndex(INDEX_NONE)
		, SlotName(NAME_None)
		, BufferSetSlotIndex(INDEX_NONE)
	{ }

	FORCEINLINE bool IsValid() const { return LODIndex >= 0 && LODIndex < REALTIME_MESH_MAX_LODS && SlotIndex >= 0; }

	FORCEINLINE int32 Index() const { return SlotIndex; }
	FORCEINLINE const FName& Name() const { return SlotName; }

	FORCEINLINE FRealtimeMeshLODKey LOD() const { return FRealtimeMeshLODKey(LODIndex); }
	FORCEINLINE operator FRealtimeMeshLODKey() const { return LOD(); }

	// Back-compat accessor — returns the buffer-set the section was created with.
	// The runtime-mutable binding on the section object may differ.
	FORCEINLINE FRealtimeMeshBufferSetKey SectionGroup() const
	{
		return FRealtimeMeshBufferSetKey::Create(LOD(), BufferSetSlotIndex);
	}

	// Back-compat implicit conversion to FRealtimeMeshBufferSetKey. Many legacy
	// callsites pass a section key where a buffer-set key is expected and rely on
	// the section's buffer-set association being inferred. New code should call
	// SectionGroup() explicitly to make the lookup intent clear.
	FORCEINLINE operator FRealtimeMeshBufferSetKey() const { return SectionGroup(); }

	FORCEINLINE bool IsPartOf(const FRealtimeMeshLODKey& InLOD) const
	{
		return LOD() == InLOD;
	}

	FORCEINLINE bool IsPartOf(const FRealtimeMeshBufferSetKey& InSectionGroup) const
	{
		return LODIndex == InSectionGroup.LOD().Index() && BufferSetSlotIndex == InSectionGroup.Index();
	}

	FORCEINLINE bool IsPolyGroupKey() const
	{
		// Stack-allocated name expansion avoids the heap FString that
		// FName::ToString() would allocate on every call.
		FNameBuilder Builder(SlotName);
		return Builder.ToView().StartsWith(TEXT("Section_PolyGroup"));
	}

	// Identity by (LODIndex, SlotIndex). Name and BufferSetSlotIndex are metadata.
	FORCEINLINE bool operator==(const FRealtimeMeshSectionKey& Other) const
	{
		return LODIndex == Other.LODIndex && SlotIndex == Other.SlotIndex;
	}

	FORCEINLINE bool operator!=(const FRealtimeMeshSectionKey& Other) const
	{
		return !(*this == Other);
	}

	friend FORCEINLINE uint32 GetTypeHash(const FRealtimeMeshSectionKey& Key)
	{
		return HashCombine(GetTypeHash(Key.SlotIndex), GetTypeHash(Key.LODIndex));
	}

	FString ToString() const
	{
		return TEXT("[LODKey:") + FString::FromInt(LODIndex)
			+ TEXT(", SectionKey:") + FString::FromInt(SlotIndex)
			+ (SlotName != NAME_None ? (TEXT("(") + SlotName.ToString() + TEXT(")")) : FString())
			+ TEXT("]");
	}

	// New-API factory: caller supplies the slot Index explicitly. BufferSetSlotIndex
	// is independent — section is not bound to any buffer set by this factory; bind
	// at create-on-LOD time via the BufferSetIndex parameter, or later via the
	// section object's SetBufferSetIndex method.
	static FRealtimeMeshSectionKey Create(const FRealtimeMeshLODKey& LODKey, int32 SlotIndex, FName SlotName = NAME_None, int32 BufferSetSlotIndex = INDEX_NONE)
	{
		return FRealtimeMeshSectionKey(LODKey, SlotIndex, SlotName, BufferSetSlotIndex);
	}

	// Legacy factories: take a buffer-set key and a section name. The buffer-set
	// slot index is recorded for back-compat (so SectionGroup() returns it). The
	// section's runtime BufferSetIndex on the section OBJECT is also wired to
	// this same index by FRealtimeMeshLODProxy::CreateSectionIfNotExists.
	static FRealtimeMeshSectionKey Create(const FRealtimeMeshBufferSetKey& BufferSetKey, FName SectionName)
	{
		const FString NameString = SectionName.ToString();
		const int32 Slot = static_cast<int32>(FCrc::StrCrc32(*NameString));
		return FRealtimeMeshSectionKey(BufferSetKey.LOD(), Slot, SectionName, BufferSetKey.Index());
	}

	static FRealtimeMeshSectionKey Create(const FRealtimeMeshBufferSetKey& BufferSetKey, int32 SectionID)
	{
		// Old "ID" overload — kept for back-compat. The name is synthesized like before.
		return Create(BufferSetKey, FName(*(TEXT("Section_") + FString::FromInt(SectionID))));
	}

	static FRealtimeMeshSectionKey CreateUnique(const FRealtimeMeshBufferSetKey& BufferSetKey)
	{
		return Create(BufferSetKey, FName(*(TEXT("Section_") + FGuid::NewGuid().ToString())));
	}

	static FRealtimeMeshSectionKey CreateForPolyGroup(const FRealtimeMeshBufferSetKey& BufferSetKey, int32 PolyGroup)
	{
		return Create(BufferSetKey, FName(*(TEXT("Section_PolyGroup_") + FString::FromInt(PolyGroup))));
	}

	friend REALTIMEMESHCOMPONENT_API FArchive& operator<<(FArchive& Ar, FRealtimeMeshSectionKey& Key);
};
