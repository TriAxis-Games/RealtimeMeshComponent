// Copyright 2016-2020 TriAxis Games L.L.C. All Rights Reserved.


#include "RuntimeMeshProvider.h"
#include "RuntimeMesh.h"


void URuntimeMeshProvider::Shutdown()
{
	FRuntimeMeshWeakRef MeshRef;
	{
		FReadScopeLock Lock(TargetLock);
		if (bIsBound)
		{
			check(Target);
			MeshRef = Target->GetMeshReference();
		}
	}

	FRuntimeMeshSharedRef PinnedRef = MeshRef.Pin();
	if (PinnedRef)
	{
		PinnedRef->Reset();
	}
}

void URuntimeMeshProvider::BindTargetProvider(URuntimeMeshProviderTargetInterface* InTarget)
{
	FWriteScopeLock Lock(TargetLock);
	Target = InTarget;
	bIsBound = true;
}

void URuntimeMeshProvider::UnlinkProviders()
{
	FWriteScopeLock Lock(TargetLock);
	Target = nullptr;
	bIsBound = false;
}

URuntimeMeshProvider::URuntimeMeshProvider()
	: GCAnchor(this)
	, bIsBound(false)
{

}

FRuntimeMeshProviderWeakRef URuntimeMeshProvider::GetReference()
{
	return GCAnchor.GetReference();
}

FRuntimeMeshWeakRef URuntimeMeshProvider::GetMeshReference()
{
	FReadScopeLock Lock(TargetLock);
	return Target ? Target->GetMeshReference() : FRuntimeMeshWeakRef();
}




void URuntimeMeshProvider::ConfigureLODs(const TArray<FRuntimeMeshLODProperties>& InLODs)
{
	FReadScopeLock Lock(TargetLock);
	if (Target)
	{
		Target->ConfigureLODs(InLODs);
	}
}

void URuntimeMeshProvider::SetLODScreenSize(int32 LODIndex, float ScreenSize)
{
	FReadScopeLock Lock(TargetLock);
	if (Target)
	{
		Target->SetLODScreenSize(LODIndex, ScreenSize);
	}
}

void URuntimeMeshProvider::MarkLODDirty(int32 LODIndex)
{
	FReadScopeLock Lock(TargetLock);
	if (Target)
	{
		Target->MarkLODDirty(LODIndex);
	}
}

void URuntimeMeshProvider::MarkAllLODsDirty()
{
	FReadScopeLock Lock(TargetLock);
	if (Target)
	{
		Target->MarkAllLODsDirty();
	}
}

void URuntimeMeshProvider::CreateSection(int32 LODIndex, int32 SectionId, const FRuntimeMeshSectionProperties& SectionProperties)
{
	FReadScopeLock Lock(TargetLock);
	if (Target)
	{
		Target->CreateSection(LODIndex, SectionId, SectionProperties);
	}
}

void URuntimeMeshProvider::SetSectionVisibility(int32 LODIndex, int32 SectionId, bool bIsVisible)
{
	FReadScopeLock Lock(TargetLock);
	if (Target)
	{
		Target->SetSectionVisibility(LODIndex, SectionId, bIsVisible);
	}
}

void URuntimeMeshProvider::SetSectionCastsShadow(int32 LODIndex, int32 SectionId, bool bCastsShadow)
{
	FReadScopeLock Lock(TargetLock);
	if (Target)
	{
		Target->SetSectionCastsShadow(LODIndex, SectionId, bCastsShadow);
	}
}

void URuntimeMeshProvider::MarkSectionDirty(int32 LODIndex, int32 SectionId)
{
	FReadScopeLock Lock(TargetLock);
	if (Target)
	{
		Target->MarkSectionDirty(LODIndex, SectionId);
	}
}

void URuntimeMeshProvider::ClearSection(int32 LODIndex, int32 SectionId)
{
	FReadScopeLock Lock(TargetLock);
	if (Target)
	{
		Target->ClearSection(LODIndex, SectionId);
	}
}

void URuntimeMeshProvider::RemoveSection(int32 LODIndex, int32 SectionId)
{
	FReadScopeLock Lock(TargetLock);
	if (Target)
	{
		Target->RemoveSection(LODIndex, SectionId);
	}
}

void URuntimeMeshProvider::MarkCollisionDirty()
{
	FReadScopeLock Lock(TargetLock);
	if (Target)
	{
		Target->MarkCollisionDirty();
	}
}


void URuntimeMeshProvider::SetupMaterialSlot(int32 MaterialSlot, FName SlotName, UMaterialInterface* InMaterial)
{
	FReadScopeLock Lock(TargetLock);
	if (Target)
	{
		Target->SetupMaterialSlot(MaterialSlot, SlotName, InMaterial);
	}
}

int32 URuntimeMeshProvider::GetMaterialIndex(FName MaterialSlotName)
{
	FReadScopeLock Lock(TargetLock);
	if (Target)
	{
		return Target->GetMaterialIndex(MaterialSlotName);
	}
	return INDEX_NONE;
}

bool URuntimeMeshProvider::IsMaterialSlotNameValid(FName MaterialSlotName) const
{
	FReadScopeLock Lock(TargetLock);
	if (Target)
	{
		return Target->IsMaterialSlotNameValid(MaterialSlotName);
	}
	return false;
}

FRuntimeMeshMaterialSlot URuntimeMeshProvider::GetMaterialSlot(int32 SlotIndex)
{
	FReadScopeLock Lock(TargetLock);
	if (Target)
	{
		return Target->GetMaterialSlot(SlotIndex);
	}
	return FRuntimeMeshMaterialSlot();
}

int32 URuntimeMeshProvider::GetNumMaterials()
{
	FReadScopeLock Lock(TargetLock);
	if (Target)
	{
		return Target->GetNumMaterials();
	}
	return 0;
}

TArray<FName> URuntimeMeshProvider::GetMaterialSlotNames()
{
	FReadScopeLock Lock(TargetLock);
	if (Target)
	{
		return Target->GetMaterialSlotNames();
	}
	return TArray<FName>();
}

TArray<FRuntimeMeshMaterialSlot> URuntimeMeshProvider::GetMaterialSlots()
{
	FReadScopeLock Lock(TargetLock);
	if (Target)
	{
		return Target->GetMaterialSlots();
	}
	return TArray<FRuntimeMeshMaterialSlot>();
}

UMaterialInterface* URuntimeMeshProvider::GetMaterial(int32 SlotIndex)
{
	FReadScopeLock Lock(TargetLock);
	if (Target)
	{
		return Target->GetMaterial(SlotIndex);
	}
	return nullptr;
}


void URuntimeMeshProvider::BeginDestroy()
{
	Shutdown();
	Super::BeginDestroy();
}

bool URuntimeMeshProvider::IsReadyForFinishDestroy()
{
	return (Super::IsReadyForFinishDestroy() && GCAnchor.IsFree());// || (GIsEditor && GIsReconstructingBlueprintInstances);
}






void URuntimeMeshProviderPassthrough::BindTargetProvider(URuntimeMeshProviderTargetInterface* InTarget)
{
	URuntimeMeshProvider::BindTargetProvider(InTarget);

	FReadScopeLock Lock(ChildLock);
	if (ChildProvider)
	{
		ChildProvider->BindTargetProvider(this);
	}
}

void URuntimeMeshProviderPassthrough::UnlinkProviders()
{
	FReadScopeLock Lock(ChildLock);
	if (ChildProvider)
	{
		ChildProvider->UnlinkProviders();
	}

	URuntimeMeshProvider::UnlinkProviders();
}

URuntimeMeshProviderPassthrough::URuntimeMeshProviderPassthrough()
	: ChildProvider(nullptr)
{
}

void URuntimeMeshProviderPassthrough::SetChildProvider(URuntimeMeshProvider* InProvider)
{
	FWriteScopeLock Lock(ChildLock);
	ChildProvider = InProvider;
}

void URuntimeMeshProviderPassthrough::Initialize()
{
	FReadScopeLock Lock(ChildLock);
	if (ChildProvider)
	{
		ChildProvider->Initialize();
	}
}

FBoxSphereBounds URuntimeMeshProviderPassthrough::GetBounds()
{
	FReadScopeLock Lock(ChildLock);
	if (ChildProvider)
	{
		return ChildProvider->GetBounds();
	}
	return FBoxSphereBounds(FSphere(FVector::ZeroVector, 1.0f));
}

bool URuntimeMeshProviderPassthrough::GetSectionMeshForLOD(int32 LODIndex, int32 SectionId, FRuntimeMeshRenderableMeshData& MeshData)
{
	FReadScopeLock Lock(ChildLock);
	if (ChildProvider)
	{
		return ChildProvider->GetSectionMeshForLOD(LODIndex, SectionId, MeshData);
	}
	return false;
}

bool URuntimeMeshProviderPassthrough::GetAllSectionsMeshForLOD(int32 LODIndex, TMap<int32, FRuntimeMeshSectionData>& MeshDatas)
{
	FReadScopeLock Lock(ChildLock);
	if (ChildProvider)
	{
		return ChildProvider->GetAllSectionsMeshForLOD(LODIndex, MeshDatas);
	}
	return false;
}

FRuntimeMeshCollisionSettings URuntimeMeshProviderPassthrough::GetCollisionSettings()
{
	FReadScopeLock Lock(ChildLock);
	if (ChildProvider)
	{
		return ChildProvider->GetCollisionSettings();
	}
	return FRuntimeMeshCollisionSettings();
}

bool URuntimeMeshProviderPassthrough::HasCollisionMesh()
{
	FReadScopeLock Lock(ChildLock);
	if (ChildProvider)
	{
		return ChildProvider->HasCollisionMesh();
	}
	return false;
}

bool URuntimeMeshProviderPassthrough::GetCollisionMesh(FRuntimeMeshCollisionData& CollisionData)
{
	FReadScopeLock Lock(ChildLock);
	if (ChildProvider)
	{
		return ChildProvider->GetCollisionMesh(CollisionData);
	}
	return false;
}

void URuntimeMeshProviderPassthrough::CollisionUpdateCompleted()
{
	FReadScopeLock Lock(ChildLock);
	if (ChildProvider)
	{
		ChildProvider->CollisionUpdateCompleted();
	}
}


bool URuntimeMeshProviderPassthrough::IsThreadSafe()
{
	FReadScopeLock Lock(ChildLock);
	if (ChildProvider)
	{
		return ChildProvider->IsThreadSafe();
	}
	return false;
}
