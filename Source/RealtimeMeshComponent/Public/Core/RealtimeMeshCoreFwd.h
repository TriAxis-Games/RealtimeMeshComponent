// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreFwd.h"
#include "Runtime/Launch/Resources/Version.h"
#include "StaticMeshResources.h"
#include "Logging/LogMacros.h"
#include "UObject/ObjectMacros.h"
#include "RealtimeMeshCoreFwd.generated.h"


// This plugin requires UE 5.5+ (enforced by the static_assert below); the supported engine range is
// 5.5-5.8. In-range, RMC_ENGINE_ABOVE_5_4 and RMC_ENGINE_ABOVE_5_5 are always true and
// RMC_ENGINE_BELOW_5_5 is always false, so they are not used to guard code (the macros are kept as
// public vocabulary). The 5.6/5.7/5.8 macros distinguish the supported engine versions.
#define RMC_ENGINE_ABOVE_5_4 (ENGINE_MAJOR_VERSION > 5 || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4))
#define RMC_ENGINE_BELOW_5_5 (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 5)
#define RMC_ENGINE_ABOVE_5_5 (ENGINE_MAJOR_VERSION > 5 || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5))
#define RMC_ENGINE_BELOW_5_6 (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 6)
#define RMC_ENGINE_ABOVE_5_6 (ENGINE_MAJOR_VERSION > 5 || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 6))
#define RMC_ENGINE_BELOW_5_7 (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 7)
#define RMC_ENGINE_ABOVE_5_7 (ENGINE_MAJOR_VERSION > 5 || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 7))
#define RMC_ENGINE_BELOW_5_8 (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 8)
#define RMC_ENGINE_ABOVE_5_8 (ENGINE_MAJOR_VERSION > 5 || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 8))
#define RMC_ENGINE_BELOW_5_9 (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 9)
#define RMC_ENGINE_ABOVE_5_9 (ENGINE_MAJOR_VERSION > 5 || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 9))

// This version of the RMC is only supported by engine version 5.5.0 and above
static_assert(RMC_ENGINE_ABOVE_5_5);

// --- Runtime Nanite build tiers (see RMC-Plan-Nanite-Stock-Engine-Fallback) ---
// RMC_NANITE_ENGINE_PROVIDER is defined by RealtimeMeshComponent.Build.cs from the engine-header probe
// (1 = the Nanite::FResourcesProvider fork present, 0 = stock). Defaulted here so a foreign build system
// that consumes these headers without the Build.cs definitions still compiles (safe = assume stock).
#ifndef RMC_NANITE_ENGINE_PROVIDER
	#define RMC_NANITE_ENGINE_PROVIDER 0
#endif
#ifndef RMC_NANITE_ENGINE_PROVIDER_VERSION
	#define RMC_NANITE_ENGINE_PROVIDER_VERSION 0
#endif

// Runtime-Nanite availability: either the provider fork (tier A), or a stock engine whose GPU page/cluster
// layout the RMC encoder has been validated against — the 5.5–5.8 window today (tier B; the encoder emits
// the pre-5.7 page/fixup/hierarchy layout on 5.5/5.6 and the 5.7/5.8 layout above). See the version fence
// in NaniteCluster.h before extending to a newer stock engine. When false (tier C) the Nanite module
// compiles to stubs and runtime Nanite is unavailable.
#define RMC_NANITE_AVAILABLE (RMC_NANITE_ENGINE_PROVIDER || (RMC_ENGINE_ABOVE_5_5 && !RMC_ENGINE_ABOVE_5_9))

#define REALTIME_MESH_MAX_TEX_COORDS MAX_STATIC_TEXCOORDS
#define REALTIME_MESH_MAX_LODS MAX_STATIC_MESH_LODS
#define REALTIME_MESH_MAX_LOD_INDEX (REALTIME_MESH_MAX_LODS - 1)

// Maximum number of elements in a vertex stream 
#define REALTIME_MESH_MAX_STREAM_ELEMENTS 8
#define REALTIME_MESH_NUM_INDICES_PER_PRIMITIVE 3

static_assert(REALTIME_MESH_MAX_STREAM_ELEMENTS >= REALTIME_MESH_MAX_TEX_COORDS, "REALTIME_MESH_MAX_STREAM_ELEMENTS must be large enough to contain REALTIME_MESH_MAX_TEX_COORDS");


REALTIMEMESHCOMPONENT_API DECLARE_LOG_CATEGORY_EXTERN(LogRealtimeMeshInterface, Warning, All);

namespace RealtimeMesh
{
	
	template <typename InElementType>
	using TFixedLODArray = TArray<InElementType, TFixedAllocator<REALTIME_MESH_MAX_LODS>>;
}



UENUM(BlueprintType)
enum class ERealtimeMeshProxyUpdateStatus : uint8
{
	NoProxy,
	NoUpdate,
	Updated,
};

enum class ERealtimeMeshBatchCreationFlags : uint8
{
	None = 0,
	ForceAllDynamic = 0x1,
	SkipStaticRayTracedSections = 0x2,
};
ENUM_CLASS_FLAGS(ERealtimeMeshBatchCreationFlags);

UENUM(BlueprintType)
enum class ERealtimeMeshOutcomePins : uint8
{
	Failure,
	Success
};