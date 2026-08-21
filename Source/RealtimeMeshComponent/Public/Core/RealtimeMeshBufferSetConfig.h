// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreFwd.h"
#include "UObject/ObjectMacros.h"
#include "RealtimeMeshBufferSetConfig.generated.h"

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
struct FRealtimeMeshBufferSetConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RealtimeMesh|SectionGroup|Config", AdvancedDisplay)
	ERealtimeMeshSectionDrawType DrawType;

	/* When true, this section group's GPU buffers are allocated with unordered-access (UAV)
	 * support so a compute pass can write directly into them. Implies dynamic-like behavior
	 * (no proxy recreation on change), since the geometry is mutated on the GPU rather than
	 * re-uploaded from the CPU. Independent of DrawType. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RealtimeMesh|SectionGroup|Config", AdvancedDisplay)
	bool bComputeWritable;

	FRealtimeMeshBufferSetConfig(ERealtimeMeshSectionDrawType InDrawType = ERealtimeMeshSectionDrawType::Static, bool bInComputeWritable = false)
		: DrawType(InDrawType)
		, bComputeWritable(bInComputeWritable)
	{ }

	bool operator==(const FRealtimeMeshBufferSetConfig& Other) const
	{
		return DrawType == Other.DrawType && bComputeWritable == Other.bComputeWritable;
	}

	bool operator!=(const FRealtimeMeshBufferSetConfig& Other) const
	{
		return !(*this == Other);
	}

	friend REALTIMEMESHCOMPONENT_API FArchive& operator<<(FArchive& Ar, FRealtimeMeshBufferSetConfig& Config);
};

// Legacy name from the pre-BufferSet terminology, kept as a source-compatibility
// alias (Blueprint graphs are covered by a CoreRedirect). New code should use
// FRealtimeMeshBufferSetConfig; this alias will be removed in a future release.
using FRealtimeMeshSectionGroupConfig = FRealtimeMeshBufferSetConfig;