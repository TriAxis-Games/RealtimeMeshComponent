// Copyright 2016-2019 Chris Conway (Koderz). All Rights Reserved.

#include "RuntimeMesh.h"
#include "RuntimeMeshComponentPlugin.h"
#include "PhysicsEngine/BodySetup.h"
#include "PhysicsEngine/PhysicsSettings.h"
#include "IPhysXCookingModule.h"
#include "RuntimeMeshComponent.h"
#include "RuntimeMeshProxy.h"
#include "RuntimeMeshData.h"


DECLARE_DWORD_COUNTER_STAT(TEXT("RuntimeMeshDelayedActions - Updated Actors"), STAT_RuntimeMeshDelayedActions_UpdatedActors, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RuntimeMeshDelayedActions - Tick"), STAT_RuntimeMeshDelayedActions_Tick, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RuntimeMeshDelayedActions - Initialize"), STAT_RuntimeMesh_Initialize, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RuntimeMeshDelayedActions - Get Physics TriMesh"), STAT_RuntimeMesh_GetPhysicsTriMesh, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RuntimeMeshDelayedActions - Has Physics TriMesh"), STAT_RuntimeMesh_HasPhysicsTriMesh, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RuntimeMeshDelayedActions - Update Collision"), STAT_RuntimeMesh_UpdateCollision, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RuntimeMeshDelayedActions - Finish Collision Async Cook"), STAT_RuntimeMesh_FinishCollisionAsyncCook, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RuntimeMeshDelayedActions - Finalize Collision Cooked Data"), STAT_RuntimeMesh_FinalizeCollisionCookedData, STATGROUP_RuntimeMesh);


//////////////////////////////////////////////////////////////////////////
//	FRuntimeMeshCollisionCookTickObject

/*
*	This tick function is used to drive the collision cooker.
*	It is enabled for one frame when we need to update collision.
*	This keeps from cooking on each individual create/update section as the original PMC did
*/
struct FRuntimeMeshDelayedActionTickObject : FTickableGameObject
{
private:
	TQueue<TWeakObjectPtr<URuntimeMesh>, EQueueMode::Mpsc> UpdateList;

public:
	FRuntimeMeshDelayedActionTickObject() {}

	static FRuntimeMeshDelayedActionTickObject& GetInstance()
	{
		static TUniquePtr<FRuntimeMeshDelayedActionTickObject> UpdaterObject;
		if (!UpdaterObject.IsValid())
		{
			UpdaterObject = MakeUnique<FRuntimeMeshDelayedActionTickObject>();
		}
		return *UpdaterObject.Get();
	}
	void RegisterForUpdate(TWeakObjectPtr<URuntimeMesh> InMesh)
	{
		UpdateList.Enqueue(InMesh);
	}

	virtual void Tick(float DeltaTime)
	{
		SCOPE_CYCLE_COUNTER(STAT_RuntimeMeshDelayedActions_Tick);

		TWeakObjectPtr<URuntimeMesh> TempMesh;
		while (UpdateList.Dequeue(TempMesh))
		{
			URuntimeMesh* Mesh = TempMesh.Get();
			if (Mesh)
			{
				INC_DWORD_STAT_BY(STAT_RuntimeMeshDelayedActions_UpdatedActors, 1);

				if (Mesh->bNeedsInitialization)
				{
					if (Mesh->MeshProvider)
					{
						Mesh->InitializeInternal();
					}
					Mesh->UpdateAllComponentsBounds();
					Mesh->bNeedsInitialization = false;
				}

				if (Mesh->bCollisionIsDirty)
				{
					Mesh->UpdateCollision();
					Mesh->bCollisionIsDirty = false;
				}
			}
		}
	}
	virtual bool IsTickable() const { return !UpdateList.IsEmpty(); }
	virtual bool IsTickableInEditor() const { return true; }
	virtual TStatId GetStatId() const { return TStatId(); }
};



//////////////////////////////////////////////////////////////////////////
//	URuntimeMesh

URuntimeMesh::URuntimeMesh(const FObjectInitializer& ObjectInitializer)
	: UObject(ObjectInitializer)
	, bNeedsInitialization(false)
	, bCollisionIsDirty(false)
	, BodySetup(nullptr)
{
}

void URuntimeMesh::Initialize(URuntimeMeshProvider* Provider)
{
	MeshProvider = Provider;
	MarkChanged();
	InitializeInternal();
}

FRuntimeMeshProviderProxyPtr URuntimeMesh::GetCurrentProviderProxy()
{
	return Data.IsValid() ? Data->GetCurrentProviderProxy() : FRuntimeMeshProviderProxyPtr();
}

TArray<FRuntimeMeshMaterialSlot> URuntimeMesh::GetMaterialSlots() const
{
	return Data.IsValid() ? Data->GetMaterialSlots() : TArray<FRuntimeMeshMaterialSlot>();
}

int32 URuntimeMesh::GetNumMaterials()
{
	return Data.IsValid() ? Data->GetNumMaterials() : 0;
}

UMaterialInterface* URuntimeMesh::GetMaterial(int32 SlotIndex)
{
	return Data.IsValid() ? Data->GetMaterial(SlotIndex) : nullptr;
}

int32 URuntimeMesh::GetMaterialIndex(FName MaterialSlotName) const
{
	return Data.IsValid() ? Data->GetMaterialIndex(MaterialSlotName) : INDEX_NONE;
}

TArray<FName> URuntimeMesh::GetMaterialSlotNames() const
{
	return Data.IsValid() ? Data->GetMaterialSlotNames() : TArray<FName>();
}

bool URuntimeMesh::IsMaterialSlotNameValid(FName MaterialSlotName) const
{
	return Data.IsValid() ? Data->IsMaterialSlotNameValid(MaterialSlotName) : false;
}

FBoxSphereBounds URuntimeMesh::GetLocalBounds() const
{
	return Data.IsValid() ? Data->GetBounds() : FBoxSphereBounds();
}

TArray<FRuntimeMeshLOD, TInlineAllocator<RUNTIMEMESH_MAXLODS>> URuntimeMesh::GetCopyOfConfiguration() const
{

	if (Data.IsValid())
	{
		return Data->GetCopyOfConfiguration();
	}
	return TArray<FRuntimeMeshLOD, TInlineAllocator<RUNTIMEMESH_MAXLODS>>();
}

FRuntimeMeshCollisionHitInfo URuntimeMesh::GetHitSource(int32 FaceIndex) const
{
	const TArray<FRuntimeMeshCollisionSourceSectionInfo>& TempSections = CollisionSource;

	if (TempSections.Num() > 0)
	{
		for (const auto& Section : TempSections)
		{
			if (Section.StartIndex <= FaceIndex && Section.EndIndex >= FaceIndex)
			{
				FRuntimeMeshCollisionHitInfo HitInfo;
				HitInfo.SourceProvider = Section.SourceProvider;
				HitInfo.SourceType = Section.SourceType;
				HitInfo.SectionId = Section.SectionId;
				HitInfo.FaceIndex = FaceIndex - Section.StartIndex;
				return HitInfo;
			}
		}
	}

	FRuntimeMeshCollisionHitInfo HitInfo;
	HitInfo.SourceProvider = MeshProvider;
	HitInfo.SourceType = ERuntimeMeshCollisionFaceSourceType::Collision;
	HitInfo.SectionId = 0;
	HitInfo.FaceIndex = FaceIndex;
	return HitInfo;
}

void URuntimeMesh::MarkCollisionDirty()
{
	// Flag the collision as dirty
	bCollisionIsDirty = true;
	FRuntimeMeshDelayedActionTickObject::GetInstance().RegisterForUpdate(TWeakObjectPtr<URuntimeMesh>(this));
}

UBodySetup* URuntimeMesh::CreateNewBodySetup()
{
	UBodySetup* NewBodySetup = NewObject<UBodySetup>(this, NAME_None, (IsTemplate() ? RF_Public : RF_NoFlags));
	NewBodySetup->BodySetupGuid = FGuid::NewGuid();

	return NewBodySetup;
}

void URuntimeMesh::UpdateCollision(bool bForceCookNow)
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_UpdateCollision);

	check(IsInGameThread());

	if (Data.IsValid())
	{
		FRuntimeMeshCollisionSettings CollisionSettings = Data->BaseProvider->GetCollisionSettings();

		UWorld* World = GetWorld();
		const bool bShouldCookAsync = !bForceCookNow && World && World->IsGameWorld() && CollisionSettings.bUseAsyncCooking;

		const auto& SetupCollisionConfiguration = [&](UBodySetup* Setup)
		{
			Setup->BodySetupGuid = FGuid::NewGuid();

			Setup->bGenerateMirroredCollision = false;
			Setup->bDoubleSidedGeometry = true;
			Setup->CollisionTraceFlag = CollisionSettings.bUseComplexAsSimple ? CTF_UseComplexAsSimple : CTF_UseDefault;

			auto& ConvexElems = Setup->AggGeom.ConvexElems;
			ConvexElems.Empty();
			for (const FRuntimeMeshCollisionConvexMesh& Convex : CollisionSettings.ConvexElements)
			{
				FKConvexElem& NewConvexElem = *new(ConvexElems) FKConvexElem();
				NewConvexElem.VertexData = Convex.VertexBuffer;
				// TODO: Store this on the section so we don't have to compute it on each cook
				NewConvexElem.ElemBox = Convex.BoundingBox;
			}

			auto& BoxElems = Setup->AggGeom.BoxElems;
			BoxElems.Empty();
			for (const FRuntimeMeshCollisionBox& Box : CollisionSettings.Boxes)
			{
				FKBoxElem& NewBox = *new(BoxElems) FKBoxElem();
				NewBox.Center = Box.Center;
				NewBox.Rotation = Box.Rotation;
				NewBox.X = Box.Extents.X;
				NewBox.Y = Box.Extents.Y;
				NewBox.Z = Box.Extents.Z;
			}

			auto& SphereElems = Setup->AggGeom.SphereElems;
			SphereElems.Empty();
			for (const FRuntimeMeshCollisionSphere& Sphere : CollisionSettings.Spheres)
			{
				FKSphereElem& NewSphere = *new(SphereElems)FKSphereElem();
				NewSphere.Center = Sphere.Center;
				NewSphere.Radius = Sphere.Radius;
			}

			auto& SphylElems = Setup->AggGeom.SphylElems;
			SphylElems.Empty();
			for (const FRuntimeMeshCollisionCapsule& Capsule : CollisionSettings.Capsules)
			{
				FKSphylElem& NewSphyl = *new(SphylElems)FKSphylElem();
				NewSphyl.Center = Capsule.Center;
				NewSphyl.Rotation = Capsule.Rotation;
				NewSphyl.Radius = Capsule.Radius;
				NewSphyl.Length = Capsule.Length;
			}
		};


		if (bShouldCookAsync)
		{
			// Abort all previous ones still standing
			for (const auto& OldBody : AsyncBodySetupQueue)
			{
				OldBody.BodySetup->AbortPhysicsMeshAsyncCreation();
			}

			UBodySetup* NewBodySetup = CreateNewBodySetup();
			SetupCollisionConfiguration(NewBodySetup);

			// Create pending source info while the mesh updates
			PendingSourceInfo = MakeUnique<TArray<FRuntimeMeshCollisionSourceSectionInfo>>();

			NewBodySetup->CreatePhysicsMeshesAsync(
				FOnAsyncPhysicsCookFinished::CreateUObject(this, &URuntimeMesh::FinishPhysicsAsyncCook, NewBodySetup));
			
			// Copy source info and reset pending
			AsyncBodySetupQueue.Add(FRuntimeMeshAsyncBodySetupData(NewBodySetup, MoveTemp(*PendingSourceInfo.Get())));
			PendingSourceInfo.Reset();
		}
		else
		{
			AsyncBodySetupQueue.Empty();
			UBodySetup* NewBodySetup = CreateNewBodySetup();

			// Change body setup guid 
			NewBodySetup->BodySetupGuid = FGuid::NewGuid();

			SetupCollisionConfiguration(NewBodySetup);

			// Update meshes
			NewBodySetup->bHasCookedCollisionData = true;

			// Create pending source info while the mesh updates
			PendingSourceInfo = MakeUnique<TArray<FRuntimeMeshCollisionSourceSectionInfo>>();

			NewBodySetup->InvalidatePhysicsData();
			NewBodySetup->CreatePhysicsMeshes();

			// Copy source info and reset pending
			CollisionSource = MoveTemp(*PendingSourceInfo.Get());
			PendingSourceInfo.Reset();

			BodySetup = NewBodySetup;
			FinalizeNewCookedData();
		}
	}
}

void URuntimeMesh::FinishPhysicsAsyncCook(bool bSuccess, UBodySetup* FinishedBodySetup)
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_FinishCollisionAsyncCook);

	check(IsInGameThread());

	const auto& SearchPredicate = [&](const FRuntimeMeshAsyncBodySetupData& Entry)
	{ 
		return Entry.BodySetup == FinishedBodySetup; 
	};
	int32 FoundIdx = AsyncBodySetupQueue.IndexOfByPredicate(SearchPredicate);

	if (FoundIdx != INDEX_NONE)
	{
		if (bSuccess)
		{
			// The new body was found in the array meaning it's newer so use it
			BodySetup = FinishedBodySetup;
			CollisionSource = MoveTemp(AsyncBodySetupQueue[FoundIdx].CollisionSources);

			// Shift down all remaining body setups, removing any old setups
			for (int32 Index = FoundIdx + 1; Index < AsyncBodySetupQueue.Num(); Index++)
			{
				AsyncBodySetupQueue[Index - (FoundIdx + 1)] = MoveTemp(AsyncBodySetupQueue[Index]);
			}
			AsyncBodySetupQueue.SetNum(AsyncBodySetupQueue.Num() - (FoundIdx + 1));

			FinalizeNewCookedData();

		}
		else
		{
			AsyncBodySetupQueue.RemoveAt(FoundIdx);
		}
	}
}

void URuntimeMesh::FinalizeNewCookedData()
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_FinalizeCollisionCookedData);

	check(IsInGameThread());

	// Alert all linked components so they can update their physics state.
	DoForAllLinkedComponents([](URuntimeMeshComponent* Mesh)
		{
			Mesh->NewCollisionMeshReceived();
		});

	// Call user event to notify of collision updated.
	if (CollisionUpdated.IsBound())
	{
		CollisionUpdated.Broadcast();
	}
}




bool URuntimeMesh::GetPhysicsTriMeshData(struct FTriMeshCollisionData* CollisionData, bool InUseAllTriData)
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_GetPhysicsTriMesh);

	if (Data.IsValid())
	{
		FRuntimeMeshCollisionData CollisionMesh;

		if (Data->BaseProvider->GetCollisionMesh(CollisionMesh))
		{
			CollisionData->Vertices = CollisionMesh.Vertices.TakeContents();
			CollisionData->Indices = CollisionMesh.Triangles.TakeContents();
			CollisionData->UVs = CollisionMesh.TexCoords.TakeContents();
			CollisionData->MaterialIndices = CollisionMesh.MaterialIndices.TakeContents();

			CollisionData->bDeformableMesh = CollisionMesh.bDeformableMesh;
			CollisionData->bDisableActiveEdgePrecompute = CollisionMesh.bDisableActiveEdgePrecompute;
			CollisionData->bFastCook = CollisionMesh.bFastCook;
			CollisionData->bFlipNormals = CollisionMesh.bFlipNormals;

			*PendingSourceInfo = MoveTemp(CollisionMesh.CollisionSources);

			return true;
		}
	}

	return false;
}

bool URuntimeMesh::ContainsPhysicsTriMeshData(bool InUseAllTriData) const
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_HasPhysicsTriMesh);

	if (Data.IsValid())
	{
		return Data->BaseProvider->HasCollisionMesh();
	}

	return false;
}

void URuntimeMesh::InitializeInternal()
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_Initialize);

	auto ProviderProxy = MeshProvider->SetupProxy();

	// This is a very loaded assignment...
	// If the old provider loses all its references here then it will take with it
	// the render side proxy and all its resources, so this is a very 
	// deceptively heavy operation if a provider already existed.
	TWeakObjectPtr<URuntimeMesh> ThisPtr = TWeakObjectPtr<URuntimeMesh>(this);
	Data = MakeShared<FRuntimeMeshData, ESPMode::ThreadSafe>(ProviderProxy, ThisPtr);


	ERHIFeatureLevel::Type FeatureLevel;
	if (GetSceneFeatureLevel(FeatureLevel))
	{
		Data->GetOrCreateRenderProxy(FeatureLevel);
	}

	// Now we can initialize the data provider and actually get the component fully up and running
	Data->Initialize();
}

void URuntimeMesh::RegisterLinkedComponent(URuntimeMeshComponent* NewComponent)
{
	LinkedComponents.AddUnique(NewComponent);
}

void URuntimeMesh::UnRegisterLinkedComponent(URuntimeMeshComponent* ComponentToRemove)
{
	check(LinkedComponents.Contains(ComponentToRemove));

	LinkedComponents.RemoveSingleSwap(ComponentToRemove, true);
}


bool URuntimeMesh::GetSceneFeatureLevel(ERHIFeatureLevel::Type& OutFeatureLevel)
{
	for (auto& Comp : LinkedComponents)
	{
		if (Comp.IsValid())
		{
			URuntimeMeshComponent* Component = Comp.Get();
			auto* Scene = Component->GetScene();
			if (Scene)
			{
				OutFeatureLevel = Scene->GetFeatureLevel();
				return true;
			}
		}
	}
	return false;
}

void URuntimeMesh::EnsureReadyToRender(ERHIFeatureLevel::Type InFeatureLevel)
{
// 	if (Data.IsValid())
// 	{
// 		Data->EnsureReadyToRender(InFeatureLevel);
// 	}
}

FRuntimeMeshProxyPtr URuntimeMesh::GetRenderProxy(ERHIFeatureLevel::Type InFeatureLevel)
{
	return Data.IsValid() ? Data->GetOrCreateRenderProxy(InFeatureLevel) : nullptr;
}

void URuntimeMesh::UpdateAllComponentsBounds()
{
	DoForAllLinkedComponents([](URuntimeMeshComponent* Mesh)
	{
		Mesh->NewBoundsReceived();
	});
}

void URuntimeMesh::RecreateAllComponentProxies()
{
	DoForAllLinkedComponents([](URuntimeMeshComponent* Mesh)
	{
		Mesh->ForceProxyRecreate();
	});
}

void URuntimeMesh::MarkChanged()
{
#if WITH_EDITOR
	Modify(true);
	PostEditChange();
#endif
}

void URuntimeMesh::PostLoad()
{
	Super::PostLoad();

	bNeedsInitialization = true; 
	FRuntimeMeshDelayedActionTickObject::GetInstance().RegisterForUpdate(TWeakObjectPtr<URuntimeMesh>(this));
}


