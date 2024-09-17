// Copyright (c) 2015-2024 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreFwd.h"
#include "Runtime/Launch/Resources/Version.h"
#include "StaticMeshResources.h"
#include "Logging/LogMacros.h"


#ifndef IS_REALTIME_MESH_LIBRARY
#define IS_REALTIME_MESH_LIBRARY 0
#endif

#if IS_REALTIME_MESH_LIBRARY
#define REALTIMEMESHCOMPONENT_INTERFACE_API REALTIMEMESHCOMPONENT_API
#else
#define REALTIMEMESHCOMPONENT_INTERFACE_API
#endif

#define RMC_ENGINE_ABOVE_5_0 (ENGINE_MAJOR_VERSION >= 5)
#define RMC_ENGINE_BELOW_5_1 (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 1)
#define RMC_ENGINE_ABOVE_5_1 (ENGINE_MAJOR_VERSION > 5 || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1))
#define RMC_ENGINE_BELOW_5_2 (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 2)
#define RMC_ENGINE_ABOVE_5_2 (ENGINE_MAJOR_VERSION > 5 || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 2))
#define RMC_ENGINE_BELOW_5_3 (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 3)
#define RMC_ENGINE_ABOVE_5_3 (ENGINE_MAJOR_VERSION > 5 || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3))
#define RMC_ENGINE_BELOW_5_4 (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 4)
#define RMC_ENGINE_ABOVE_5_4 (ENGINE_MAJOR_VERSION > 5 || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4))
#define RMC_ENGINE_BELOW_5_5 (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 5)
#define RMC_ENGINE_ABOVE_5_5 (ENGINE_MAJOR_VERSION > 5 || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5))

// This version of the RMC is only supported by engine version 5.0.0 and above
static_assert(RMC_ENGINE_ABOVE_5_0);

#define REALTIME_MESH_MAX_TEX_COORDS MAX_STATIC_TEXCOORDS
#define REALTIME_MESH_MAX_LODS MAX_STATIC_MESH_LODS
#define REALTIME_MESH_MAX_LOD_INDEX (REALTIME_MESH_MAX_LODS - 1)

// Maximum number of elements in a vertex stream 
#define REALTIME_MESH_MAX_STREAM_ELEMENTS 8
#define REALTIME_MESH_NUM_INDICES_PER_PRIMITIVE 3

static_assert(REALTIME_MESH_MAX_STREAM_ELEMENTS >= REALTIME_MESH_MAX_TEX_COORDS, "REALTIME_MESH_MAX_STREAM_ELEMENTS must be large enough to contain REALTIME_MESH_MAX_TEX_COORDS");

#if RMC_ENGINE_ABOVE_5_1
#define RMC_NODISCARD_CTOR UE_NODISCARD_CTOR
#else
#define RMC_NODISCARD_CTOR
#endif


#if RMC_ENGINE_BELOW_5_2
template <typename T> FORCEINLINE uint32 GetTypeHashHelper(const T& V) { return GetTypeHash(V); }
#endif


REALTIMEMESHCOMPONENT_INTERFACE_API DECLARE_LOG_CATEGORY_EXTERN(LogRealtimeMeshInterface, Warning, All);

namespace RealtimeMesh
{
	
	template <typename InElementType>
	using TFixedLODArray = TArray<InElementType, TFixedAllocator<REALTIME_MESH_MAX_LODS>>;
}



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

enum class ERealtimeMeshOutcomePins : uint8
{
	Failure,
	Success
};