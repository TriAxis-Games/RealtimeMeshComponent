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
				// Debug logging for Nanite resource initialization
				UE_LOG(LogRealtimeMesh, Verbose, TEXT("Initializing Nanite resources for mesh: %s (Thread: %s)"), 
					OwningMesh ? *OwningMesh->GetName() : TEXT("NULL"),
					IsInRenderingThread() ? TEXT("RenderThread") : IsInGameThread() ? TEXT("GameThread") : TEXT("Other"));
				
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
				
				// Enhanced validation with thread safety check
				if (IsInGameThread())
				{
					UE_LOG(LogRealtimeMesh, Warning, TEXT("POTENTIAL ISSUE: Nanite resources being initialized on game thread for mesh: %s. Should be on render thread."), 
						OwningMesh ? *OwningMesh->GetName() : TEXT("NULL"));
				}
				
				UE_LOG(LogRealtimeMesh, Verbose, TEXT("Nanite mesh validation: RootData=%d bytes, HierarchyNodes=%d, StreamablePages=%lld"), 
					RootData.Num(), HierarchyNodes.Num(), StreamablePages.GetBulkDataSize());
				
				// Additional validation for resource integrity
				if (NumClusters == 0)
				{
					UE_LOG(LogRealtimeMesh, Warning, TEXT("Nanite mesh has 0 clusters for mesh: %s - this may cause render issues"), 
						OwningMesh ? *OwningMesh->GetName() : TEXT("NULL"));
				}
				
				::Nanite::FResources::InitResources(OwningMesh);
				bIsInitialized = true;
				
				UE_LOG(LogRealtimeMesh, Verbose, TEXT("Successfully initialized Nanite resources for mesh: %s"), 
					OwningMesh ? *OwningMesh->GetName() : TEXT("NULL"));
			}
			else
			{
				UE_LOG(LogRealtimeMesh, VeryVerbose, TEXT("Nanite resources already initialized for mesh: %s"), 
					OwningMesh ? *OwningMesh->GetName() : TEXT("NULL"));
			}
		}

		void ReleaseResources()
		{
			if (bIsInitialized)
			{
				UE_LOG(LogRealtimeMesh, Verbose, TEXT("Releasing Nanite resources (Thread: %s)"), 
					IsInRenderingThread() ? TEXT("RenderThread") : IsInGameThread() ? TEXT("GameThread") : TEXT("Other"));
				
				::Nanite::FResources::ReleaseResources();
				bIsInitialized = false;
				UE_LOG(LogRealtimeMesh, Verbose, TEXT("Successfully released Nanite resources"));
			}
			else
			{
				UE_LOG(LogRealtimeMesh, VeryVerbose, TEXT("ReleaseResources called on uninitialized Nanite resources"));
			}
		}

		::Nanite::FResources* GetNanitePtr() { return static_cast<::Nanite::FResources*>(this); }

		void ClearRuntimeState()
		{
			if (!ensure(!bIsInitialized))
			{
				UE_LOG(LogRealtimeMesh, Warning, TEXT("Attempting to clear runtime state on initialized Nanite resources"));
				return;
			}
			
			UE_LOG(LogRealtimeMesh, VeryVerbose, TEXT("Clearing Nanite runtime state"));
			
			// Blank all the runtime state on this copy
			static const ::Nanite::FResources NullResources;
			RuntimeResourceID = NullResources.RuntimeResourceID;
			HierarchyOffset = NullResources.HierarchyOffset;
			RootPageIndex = NullResources.RootPageIndex;
			ImposterIndex = NullResources.ImposterIndex;
			NumHierarchyNodes = NullResources.NumHierarchyNodes;
			NumResidentClusters = NullResources.NumResidentClusters;
			PersistentHash = NullResources.PersistentHash;			
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
			UE_LOG(LogRealtimeMesh, VeryVerbose, TEXT("Destroying Nanite resources (Thread: %s)"), 
				IsInRenderingThread() ? TEXT("RenderThread") : IsInGameThread() ? TEXT("GameThread") : TEXT("Other"));
				
			// Validate resource state before destruction
			if (!Resources->HasValidData())
			{
				UE_LOG(LogRealtimeMesh, VeryVerbose, TEXT("Destroying Nanite resources with no valid data"));
			}
				
			// Safety check - ensure we're not double-releasing
			bool bWasInitialized = Resources->bIsInitialized;
				
			// Let the render thread release the resources safely
			Resources->ReleaseResources();

			// Then queue the class delete for after the render thread releases the resource
			ENQUEUE_RENDER_COMMAND(DestroyRealtimeMeshNaniteResources)(
				[Resources, bWasInitialized](FRHICommandListImmediate&)
				{
					UE_LOG(LogRealtimeMesh, VeryVerbose, TEXT("Deleting Nanite resources on render thread (was initialized: %s)"), 
						bWasInitialized ? TEXT("true") : TEXT("false"));
					delete Resources;
				}
			);
		}
		else
		{
			UE_LOG(LogRealtimeMesh, Warning, TEXT("Destroy called with null Nanite resources pointer"));
		}
	}
}
