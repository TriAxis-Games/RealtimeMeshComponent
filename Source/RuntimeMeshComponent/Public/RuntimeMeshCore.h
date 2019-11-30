// Copyright 2016-2019 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "Engine/Engine.h"
#include "Components/MeshComponent.h"
#include "Runtime/Launch/Resources/Version.h"
#include "Stats/Stats.h"
#include "Logging//LogMacros.h"
#include "HAL/CriticalSection.h"
#include "RuntimeMeshCore.generated.h"

DECLARE_STATS_GROUP(TEXT("RuntimeMesh"), STATGROUP_RuntimeMesh, STATCAT_Advanced);
DECLARE_LOG_CATEGORY_EXTERN(LogRuntimeMesh, Log, Warning);

#define RuntimeMesh_MAXTEXCOORDS MAX_STATIC_TEXCOORDS
#define RuntimeMesh_MAXLODS MAX_STATIC_MESH_LODS

#define RuntimeMesh_ENABLE_DEBUG_RENDERING (!(UE_BUILD_SHIPPING || UE_BUILD_TEST) || WITH_EDITOR)

// Custom version for runtime mesh serialization
namespace FRuntimeMeshVersion
{
	enum Type
	{
		Initial = 0,

		// -----<new versions can be added above this line>-------------------------------------------------
		VersionPlusOne,
		LatestVersion = VersionPlusOne - 1
	};

	// The GUID for this custom version
	const static FGuid GUID = FGuid(0xEE48714B, 0x8A2C4652, 0x98BE40E6, 0xCB7EF0E6);
};


class FRuntimeMeshVertexFactory;


/* Mobility status for a RuntimeMeshComponent */
UENUM(BlueprintType)
enum class ERuntimeMeshMobility : uint8
{
	Movable,
	Stationary,
	Static
};

/* Update frequency for a section. Used to optimize for update or render speed*/
UENUM(BlueprintType)
enum class ERuntimeMeshUpdateFrequency : uint8
{
	/* Tries to skip recreating the scene proxy if possible. */
	Average UMETA(DisplayName = "Average"),
	/* Tries to skip recreating the scene proxy if possible and optimizes the buffers for frequent updates. */
	Frequent UMETA(DisplayName = "Frequent"),
	/* If the component is static it will try to use the static rendering path (this will force a recreate of the scene proxy) */
	Infrequent UMETA(DisplayName = "Infrequent")
};

/*
*	Configuration flag for the collision cooking to prioritize cooking speed or collision performance.
*/
UENUM(BlueprintType)
enum class ERuntimeMeshCollisionCookingMode : uint8
{
	/*
	*	Favors runtime collision performance of cooking speed.
	*	This means that cooking a new mesh will be slower, but collision will be faster.
	*/
	CollisionPerformance UMETA(DisplayName = "Collision Performance"),

	/*
	*	Favors cooking speed over collision performance.
	*	This means that cooking a new mesh will be faster, but collision will be slower.
	*/
	CookingPerformance UMETA(DisplayName = "Cooking Performance"),
};
