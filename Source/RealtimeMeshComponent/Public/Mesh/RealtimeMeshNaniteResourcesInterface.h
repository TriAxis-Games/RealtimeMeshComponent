// Copyright (c) 2015-2025 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "RealtimeMeshCore.h"
#include "Rendering/NaniteResources.h"
#include "RealtimeMeshComponentModule.h"

namespace RealtimeMesh
{
	struct FRealtimeMeshNaniteResources;

	struct FRealtimeMeshNaniteResourcesDeleter
	{
		void operator()(FRealtimeMeshNaniteResources* Resources) const
		{
			Destroy(Resources);
		}

		static void Destroy(FRealtimeMeshNaniteResources* Resources);
	};

	using FRealtimeMeshNaniteResourcesPtr = TUniquePtr<FRealtimeMeshNaniteResources, FRealtimeMeshNaniteResourcesDeleter>;


	
	struct FRealtimeMeshNaniteResources : protected ::Nanite::FResources
	{
		friend struct FRealtimeMeshNaniteResourcesDeleter;
	private:
		FBoxSphereBounds3f Bounds;
		bool bIsInitialized;


		FRealtimeMeshNaniteResources(::Nanite::FResources&& InResources, const FBoxSphereBounds3f& InBounds)
			: ::Nanite::FResources(MoveTemp(InResources))
			, Bounds(InBounds)
			, bIsInitialized(false)
		{
			ClearRuntimeState();
		}
		FRealtimeMeshNaniteResources(const FRealtimeMeshNaniteResources& Other)
			: ::Nanite::FResources(Other)
			, Bounds(Other.Bounds)
			, bIsInitialized(false)
		{
			ClearRuntimeState();
		}
		FRealtimeMeshNaniteResources(FRealtimeMeshNaniteResources&& Other)
			: Bounds(Other.Bounds)
			, bIsInitialized(false)
		{
			check(!Other.bIsInitialized);
			::Nanite::FResources::operator=(MoveTemp(Other));
			ClearRuntimeState();
		}

		
	public:
		FRealtimeMeshNaniteResources()
			: bIsInitialized(false)
		{
			ClearRuntimeState();
		}

		FRealtimeMeshNaniteResources& operator=(const FRealtimeMeshNaniteResources&) = delete;
		FRealtimeMeshNaniteResources& operator=(FRealtimeMeshNaniteResources&&) = delete;

		static FRealtimeMeshNaniteResourcesPtr Create(::Nanite::FResources&& InResources, const FBoxSphereBounds3f& InBounds)
		{
			return FRealtimeMeshNaniteResourcesPtr(new FRealtimeMeshNaniteResources(MoveTemp(InResources), InBounds));
		}

		static FRealtimeMeshNaniteResourcesPtr CreateFromCopy(const ::Nanite::FResources& InResources, const FBoxSphereBounds3f& InBounds)
		{
			::Nanite::FResources ResourcesCopy = InResources;
			return FRealtimeMeshNaniteResourcesPtr(new FRealtimeMeshNaniteResources(MoveTemp(ResourcesCopy), InBounds));
		}

		FRealtimeMeshNaniteResourcesPtr Clone() const
		{			
			return FRealtimeMeshNaniteResourcesPtr(new FRealtimeMeshNaniteResources(*this));
		}

		FRealtimeMeshNaniteResourcesPtr Consume()
		{
			return FRealtimeMeshNaniteResourcesPtr(new FRealtimeMeshNaniteResources(MoveTemp(*this)));
		}
		
		bool HasValidData() const { return RootData.Num() > 0; }
		
		const FBoxSphereBounds3f& GetBounds() const { return Bounds; }
		
		void InitResources(const UObject* OwningMesh)
		{
			check(IsValid(OwningMesh));
			if (!bIsInitialized)
			{
				// Validate essential resource data before initialization
				if (!HasValidData())
				{
					UE_LOG(LogRealtimeMesh, Warning, TEXT("Attempting to initialize Nanite resources with invalid data for mesh: %s"),
						OwningMesh ? *OwningMesh->GetName() : TEXT("NULL"));
					return;
				}

				// Validate hierarchy nodes are present (required for Nanite streaming)
				if (HierarchyNodes.IsEmpty())
				{
					UE_LOG(LogRealtimeMesh, Warning, TEXT("Missing hierarchy nodes for Nanite mesh: %s"),
						OwningMesh ? *OwningMesh->GetName() : TEXT("NULL"));
					return;
				}

				if (NumClusters == 0)
				{
					UE_LOG(LogRealtimeMesh, Warning, TEXT("Nanite mesh has 0 clusters for mesh: %s - this may cause render issues"),
						OwningMesh ? *OwningMesh->GetName() : TEXT("NULL"));
				}

				::Nanite::FResources::InitResources(OwningMesh);
				bIsInitialized = true;
			}
		}

		void ReleaseResources()
		{
			if (bIsInitialized)
			{
				::Nanite::FResources::ReleaseResources();
				bIsInitialized = false;
			}
		}

		::Nanite::FResources* GetNanitePtr() { return static_cast<::Nanite::FResources*>(this); }

		void ClearRuntimeState()
		{
			if (!ensure(!bIsInitialized))
			{
				return;
			}

			// Blank all the runtime state on this copy
			static const ::Nanite::FResources NullResources;
			RuntimeResourceID = NullResources.RuntimeResourceID;
			HierarchyOffset = NullResources.HierarchyOffset;
			RootPageIndex = NullResources.RootPageIndex;
#if !RMC_ENGINE_ABOVE_5_7
			ImposterIndex = NullResources.ImposterIndex;
#endif
			NumHierarchyNodes = NullResources.NumHierarchyNodes;
			NumResidentClusters = NullResources.NumResidentClusters;
			PersistentHash = NullResources.PersistentHash;
#if RMC_ENGINE_ABOVE_5_6
			// Fields added in UE 5.6
			AssemblyTransformOffset = NullResources.AssemblyTransformOffset;
			NumHierarchyDwords = NullResources.NumHierarchyDwords;
#endif
#if WITH_EDITOR
			ResourceName = NullResources.ResourceName;
			DDCKeyHash = NullResources.DDCKeyHash;
			DDCRawHash = NullResources.DDCRawHash;
#endif
		}

	};



	inline void FRealtimeMeshNaniteResourcesDeleter::Destroy(FRealtimeMeshNaniteResources* Resources)
	{
		if (Resources)
		{
			// Release resources on the current thread (will queue to render thread internally)
			Resources->ReleaseResources();

			// Queue the actual delete for the render thread
			ENQUEUE_RENDER_COMMAND(DestroyRealtimeMeshNaniteResources)(
				[Resources](FRHICommandListImmediate&)
				{
					delete Resources;
				}
			);
		}
	}
}
