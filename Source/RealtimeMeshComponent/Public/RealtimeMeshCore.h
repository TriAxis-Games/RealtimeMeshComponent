// Copyright (c) 2015-2025 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Runtime/Launch/Resources/Version.h"
#include "StaticMeshResources.h"
#include "Core/RealtimeMeshInterfaceFwd.h"

DECLARE_STATS_GROUP(TEXT("RealtimeMesh"), STATGROUP_RealtimeMesh, STATCAT_Advanced);

namespace RealtimeMesh
{
	// Custom version for runtime mesh serialization
	namespace FRealtimeMeshVersion
	{
		enum Type
		{
			Initial = 0,
			StreamsNowHoldEntireKey = 1,
			DataRestructure = 2,
			CollisionUpdateFlowRestructure = 3,
			StreamKeySizeChanged = 4,
			RemovedNamedStreamElements = 5,
			SimpleMeshStoresCollisionConfig = 6,
			ImprovingDataTypes = 7,
			SimpleMeshStoresCustomComplexCollision = 8,
			DistanceFieldAndCardRepresentationSupport = 9,
			SupportOptionalDataSerialization = 10,
			CollisionOverhaul = 11,
			DrawTypeMovedToSectionGroup = 12,
			ActorSupportsOptionalConstructionDefer = 13,

			// -----<new versions can be added above this line>-------------------------------------------------
			VersionPlusOne,
			LatestVersion = VersionPlusOne - 1
		};

		// The GUID for this custom version
		const static FGuid GUID = FGuid(0xAF3DD80C, 0x4C114B25, 0x9C7A9515, 0x5062D6E9);
	}



	/** Deleter function for TSharedPtrs that only allows the object to be destructed on the render thread. */
	template <typename Type>
	struct FRealtimeMeshRenderThreadDeleter
	{
		void operator()(Type* Object) const
		{
			// This is a custom deleter to make sure the runtime mesh proxy is only ever deleted on the rendering thread.
			if (IsInRenderingThread())
			{
				delete Object;
			}
			else
			{
				ENQUEUE_RENDER_COMMAND(FRealtimeMeshProxyDeleterCommand)(
					[Object](FRHICommandListImmediate& RHICmdList)
					{
						delete static_cast<Type*>(Object);
					}
				);
			}
		}
	};

#define CREATE_RMC_PTR_TYPES(TypeName) \
	using TypeName##Ref = TSharedRef<TypeName, ESPMode::ThreadSafe>; \
	using TypeName##Ptr = TSharedPtr<TypeName, ESPMode::ThreadSafe>; \
	using TypeName##WeakPtr = TWeakPtr<TypeName, ESPMode::ThreadSafe>; \
	using TypeName##ConstRef = TSharedRef<const TypeName, ESPMode::ThreadSafe>; \
	using TypeName##ConstPtr = TSharedPtr<const TypeName, ESPMode::ThreadSafe>; \
	using TypeName##ConstWeakPtr = TWeakPtr<const TypeName, ESPMode::ThreadSafe>;

	struct FRealtimeMeshGPUUpdateBuilder;

	struct FRealtimeMeshStream;

	struct FRealtimeMeshUpdateState;
	CREATE_RMC_PTR_TYPES(FRealtimeMeshUpdateState);
	
	class FRealtimeMeshVertexFactory;
	CREATE_RMC_PTR_TYPES(FRealtimeMeshVertexFactory);

	class FRealtimeMeshSectionGroupProxy;
	CREATE_RMC_PTR_TYPES(FRealtimeMeshSectionGroupProxy);

	class FRealtimeMeshSectionProxy;
	CREATE_RMC_PTR_TYPES(FRealtimeMeshSectionProxy);

	class FRealtimeMeshLODProxy;
	CREATE_RMC_PTR_TYPES(FRealtimeMeshLODProxy);

	class FRealtimeMeshProxy;
	CREATE_RMC_PTR_TYPES(FRealtimeMeshProxy);

	class FRealtimeMeshSharedResources;
	CREATE_RMC_PTR_TYPES(FRealtimeMeshSharedResources);

	class FRealtimeMeshSectionGroup;
	CREATE_RMC_PTR_TYPES(FRealtimeMeshSectionGroup);

	class FRealtimeMeshSection;
	CREATE_RMC_PTR_TYPES(FRealtimeMeshSection);

	class FRealtimeMeshLOD;
	CREATE_RMC_PTR_TYPES(FRealtimeMeshLOD);

	class FRealtimeMesh;
	CREATE_RMC_PTR_TYPES(FRealtimeMesh);

#undef CREATE_RMC_PTR_TYPES

	template <typename InElementType>
	using TFixedLODArray = TArray<InElementType, TFixedAllocator<REALTIME_MESH_MAX_LODS>>;

	class FRealtimeMeshGPUBuffer;
}


class URealtimeMesh;
class URealtimeMeshComponent;
