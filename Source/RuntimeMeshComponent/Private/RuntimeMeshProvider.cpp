// Copyright 2016-2020 Chris Conway (Koderz). All Rights Reserved.


#include "RuntimeMeshProvider.h"

void URuntimeMeshProvider::ShutdownInternal()
{
	URuntimeMeshProviderTargetInterface* Target = GetTarget();
	if (Target)
	{
		return Target->ShutdownInternal();
	}
}

void URuntimeMeshProvider::BindTargetProvider_Implementation(URuntimeMeshProviderTargetInterface* InTarget)
{
	FScopeLock Lock(&TargetSyncRoot);
	InternalTarget = InTarget;
	ReferenceAnchor.BindToAssociated(InTarget->GetMeshReference());
}


void URuntimeMeshProvider::Unlink_Implementation()
{
	FScopeLock Lock(&TargetSyncRoot);
	InternalTarget = nullptr;
}


URuntimeMeshProvider::URuntimeMeshProvider()
	: ReferenceAnchor(this)
{

}

FRuntimeMeshProviderWeakRef URuntimeMeshProvider::GetReference()
{
	return ReferenceAnchor.GetReference();
}

FRuntimeMeshWeakRef URuntimeMeshProvider::GetMeshReference()
{
	auto Target = GetTarget();
	if (Target)
	{
		return Target->GetMeshReference();
	}
	return FRuntimeMeshObjectSharedRef<URuntimeMesh>();
}

void URuntimeMeshProvider::Initialize_Implementation()
{

}

FBoxSphereBounds URuntimeMeshProvider::GetBounds_Implementation()
{
	return FBoxSphereBounds(FSphere(FVector::ZeroVector, 1.0f));
}

bool URuntimeMeshProvider::GetSectionMeshForLOD_Implementation(int32 LODIndex, int32 SectionId, FRuntimeMeshRenderableMeshData& MeshData)
{
	return false;
}

bool URuntimeMeshProvider::GetAllSectionsMeshForLOD_Implementation(int32 LODIndex, TMap<int32, FRuntimeMeshSectionData>& MeshDatas)
{
	return false;
}

FRuntimeMeshCollisionSettings URuntimeMeshProvider::GetCollisionSettings_Implementation()
{
	return FRuntimeMeshCollisionSettings();
}

bool URuntimeMeshProvider::HasCollisionMesh_Implementation()
{
	return false;
}

bool URuntimeMeshProvider::GetCollisionMesh_Implementation(FRuntimeMeshCollisionData& CollisionData)
{
	return false;
}

void URuntimeMeshProvider::CollisionUpdateCompleted_Implementation()
{

}

bool URuntimeMeshProvider::IsThreadSafe_Implementation()
{
	return false;
}



void URuntimeMeshProvider::ConfigureLODs_Implementation(const TArray<FRuntimeMeshLODProperties>& InLODs)
{
	auto Target = GetTarget();
	if (Target)
	{
		Target->ConfigureLODs(InLODs);
	}
}

void URuntimeMeshProvider::SetLODScreenSize_Implementation(int32 LODIndex, float ScreenSize)
{
	auto Target = GetTarget();
	if (Target)
	{
		Target->SetLODScreenSize(LODIndex, ScreenSize);
	}
}

void URuntimeMeshProvider::MarkLODDirty_Implementation(int32 LODIndex)
{
	auto Target = GetTarget();
	if (Target)
	{
		Target->MarkLODDirty(LODIndex);
	}
}

void URuntimeMeshProvider::MarkAllLODsDirty_Implementation()
{
	auto Target = GetTarget();
	if (Target)
	{
		Target->MarkAllLODsDirty();
	}
}

void URuntimeMeshProvider::CreateSection_Implementation(int32 LODIndex, int32 SectionId, const FRuntimeMeshSectionProperties& SectionProperties)
{
	auto Target = GetTarget();
	if (Target)
	{
		Target->CreateSection(LODIndex, SectionId, SectionProperties);
	}
}

void URuntimeMeshProvider::SetSectionVisibility_Implementation(int32 LODIndex, int32 SectionId, bool bIsVisible)
{
	auto Target = GetTarget();
	if (Target)
	{
		Target->SetSectionVisibility(LODIndex, SectionId, bIsVisible);
	}
}

void URuntimeMeshProvider::SetSectionCastsShadow_Implementation(int32 LODIndex, int32 SectionId, bool bCastsShadow)
{
	auto Target = GetTarget();
	if (Target)
	{
		Target->SetSectionCastsShadow(LODIndex, SectionId, bCastsShadow);
	}
}

void URuntimeMeshProvider::MarkSectionDirty_Implementation(int32 LODIndex, int32 SectionId)
{
	auto Target = GetTarget();
	if (Target)
	{
		Target->MarkSectionDirty(LODIndex, SectionId);
	}
}

void URuntimeMeshProvider::ClearSection_Implementation(int32 LODIndex, int32 SectionId)
{
	auto Target = GetTarget();
	if (Target)
	{
		Target->ClearSection(LODIndex, SectionId);
	}
}

void URuntimeMeshProvider::RemoveSection_Implementation(int32 LODIndex, int32 SectionId)
{
	auto Target = GetTarget();
	if (Target)
	{
		Target->RemoveSection(LODIndex, SectionId);
	}
}

void URuntimeMeshProvider::MarkCollisionDirty_Implementation()
{
	auto Target = GetTarget();
	if (Target)
	{
		Target->MarkCollisionDirty();
	}
}

void URuntimeMeshProvider::SetupMaterialSlot_Implementation(int32 MaterialSlot, FName SlotName, UMaterialInterface* InMaterial)
{
	auto Target = GetTarget();
	if (Target)
	{
		Target->SetupMaterialSlot(MaterialSlot, SlotName, InMaterial);
	}
}

int32 URuntimeMeshProvider::GetMaterialIndex_Implementation(FName MaterialSlotName)
{
	auto Target = GetTarget();
	if (Target)
	{
		return Target->GetMaterialIndex(MaterialSlotName);
	}
	return INDEX_NONE;
}

bool URuntimeMeshProvider::IsMaterialSlotNameValid_Implementation(FName MaterialSlotName) const
{
	auto Target = GetTarget();
	if (Target)
	{
		return Target->IsMaterialSlotNameValid(MaterialSlotName);
	}
	return false;
}

FRuntimeMeshMaterialSlot URuntimeMeshProvider::GetMaterialSlot_Implementation(int32 SlotIndex)
{
	auto Target = GetTarget();
	if (Target)
	{
		return Target->GetMaterialSlot(SlotIndex);
	}
	return FRuntimeMeshMaterialSlot();
}

int32 URuntimeMeshProvider::GetNumMaterials_Implementation()
{
	auto Target = GetTarget();
	if (Target)
	{
		return Target->GetNumMaterials();
	}
	return 0;
}

TArray<FName> URuntimeMeshProvider::GetMaterialSlotNames_Implementation()
{
	auto Target = GetTarget();
	if (Target)
	{
		return Target->GetMaterialSlotNames();
	}
	return TArray<FName>();
}

TArray<FRuntimeMeshMaterialSlot> URuntimeMeshProvider::GetMaterialSlots_Implementation()
{
	auto Target = GetTarget();
	if (Target)
	{
		return Target->GetMaterialSlots();
	}
	return TArray<FRuntimeMeshMaterialSlot>();
}

UMaterialInterface* URuntimeMeshProvider::GetMaterial_Implementation(int32 SlotIndex)
{
	auto Target = GetTarget();
	if (Target)
	{
		return Target->GetMaterial(SlotIndex);
	}
	return nullptr;
}


void URuntimeMeshProvider::BeginDestroy()
{
	Super::BeginDestroy();
	ShutdownInternal();
}

bool URuntimeMeshProvider::IsReadyForFinishDestroy()
{
	return (Super::IsReadyForFinishDestroy() && ReferenceAnchor.IsFree());// || (GIsEditor && GIsReconstructingBlueprintInstances);
}






void URuntimeMeshProviderPassthrough::BindTargetProvider_Implementation(URuntimeMeshProviderTargetInterface* InTarget)
{
	URuntimeMeshProvider::BindTargetProvider_Implementation(InTarget);

	if (ChildProvider)
	{
		ChildProvider->BindTargetProvider(this);
	}
}

void URuntimeMeshProviderPassthrough::Unlink_Implementation()
{
	URuntimeMeshProvider::Unlink_Implementation();
	if (ChildProvider)
	{
		ChildProvider->Unlink();
	}
}

URuntimeMeshProviderPassthrough::URuntimeMeshProviderPassthrough()
	: ChildProvider(nullptr)
{
}

URuntimeMeshProvider* URuntimeMeshProviderPassthrough::GetChildProvider() const
{
	return ChildProvider;
}

void URuntimeMeshProviderPassthrough::SetChildProvider(URuntimeMeshProvider* InProvider)
{
	if (!HasBeenBound())
	{
		ChildProvider = InProvider;
	}
}

void URuntimeMeshProviderPassthrough::Initialize_Implementation()
{
	URuntimeMeshProvider* ChildProviderTemp = GetChildProvider();
	if (ChildProviderTemp)
	{
		ChildProviderTemp->Initialize_Implementation();
	}
}

FBoxSphereBounds URuntimeMeshProviderPassthrough::GetBounds_Implementation()
{
	URuntimeMeshProvider* ChildProviderTemp = GetChildProvider();
	if (ChildProviderTemp)
	{
		return ChildProviderTemp->GetBounds_Implementation();
	}
	return FBoxSphereBounds(FSphere(FVector::ZeroVector, 1.0f));
}

bool URuntimeMeshProviderPassthrough::GetSectionMeshForLOD_Implementation(int32 LODIndex, int32 SectionId, FRuntimeMeshRenderableMeshData& MeshData)
{
	URuntimeMeshProvider* ChildProviderTemp = GetChildProvider();
	if (ChildProviderTemp)
	{
		return ChildProviderTemp->GetSectionMeshForLOD_Implementation(LODIndex, SectionId, MeshData);
	}
	return false;
}

bool URuntimeMeshProviderPassthrough::GetAllSectionsMeshForLOD_Implementation(int32 LODIndex, TMap<int32, FRuntimeMeshSectionData>& MeshDatas)
{
	URuntimeMeshProvider* ChildProviderTemp = GetChildProvider();
	if (ChildProviderTemp)
	{
		return ChildProviderTemp->GetAllSectionsMeshForLOD_Implementation(LODIndex, MeshDatas);
	}
	return false;
}

FRuntimeMeshCollisionSettings URuntimeMeshProviderPassthrough::GetCollisionSettings_Implementation()
{
	URuntimeMeshProvider* ChildProviderTemp = GetChildProvider();
	if (ChildProviderTemp)
	{
		return ChildProviderTemp->GetCollisionSettings_Implementation();
	}
	return FRuntimeMeshCollisionSettings();
}

bool URuntimeMeshProviderPassthrough::HasCollisionMesh_Implementation()
{
	URuntimeMeshProvider* ChildProviderTemp = GetChildProvider();
	if (ChildProviderTemp)
	{
		return ChildProviderTemp->HasCollisionMesh_Implementation();
	}
	return false;
}

bool URuntimeMeshProviderPassthrough::GetCollisionMesh_Implementation(FRuntimeMeshCollisionData& CollisionData)
{
	URuntimeMeshProvider* ChildProviderTemp = GetChildProvider();
	if (ChildProviderTemp)
	{
		return ChildProviderTemp->GetCollisionMesh_Implementation(CollisionData);
	}
	return false;
}

void URuntimeMeshProviderPassthrough::CollisionUpdateCompleted_Implementation()
{
	URuntimeMeshProvider* ChildProviderTemp = GetChildProvider();
	if (ChildProviderTemp)
	{
		ChildProviderTemp->CollisionUpdateCompleted_Implementation();
	}
}

bool URuntimeMeshProviderPassthrough::IsThreadSafe_Implementation()
{
	URuntimeMeshProvider* ChildProviderTemp = GetChildProvider();
	if (ChildProviderTemp)
	{
		return ChildProviderTemp->IsThreadSafe_Implementation();
	}
	return false;
}
