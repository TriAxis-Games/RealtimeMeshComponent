// Copyright 2016-2020 Chris Conway (Koderz). All Rights Reserved.


#include "RuntimeMeshProvider.h"

void URuntimeMeshProvider::ShutdownInternal()
{
	auto Target = GetTarget();
	if (Target)
	{
		return Target->ShutdownInternal();
	}
}

void URuntimeMeshProvider::BindTargetProvider_Implementation(const TScriptInterface<IRuntimeMeshProviderTargetInterface>& InTarget)
{
	FScopeLock Lock(&TargetSyncRoot);
	check(!bHasBeenBound);
	bHasBeenBound = true;
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
	, bHasBeenBound(false)
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
		IRuntimeMeshProviderTargetInterface::Execute_ConfigureLODs(Target.GetObject(), InLODs);
	}
}

void URuntimeMeshProvider::SetLODScreenSize_Implementation(int32 LODIndex, float ScreenSize)
{
	auto Target = GetTarget();
	if (Target)
	{
		IRuntimeMeshProviderTargetInterface::Execute_SetLODScreenSize(Target.GetObject(), LODIndex, ScreenSize);
	}
}

void URuntimeMeshProvider::MarkLODDirty_Implementation(int32 LODIndex)
{
	auto Target = GetTarget();
	if (Target)
	{
		IRuntimeMeshProviderTargetInterface::Execute_MarkLODDirty(Target.GetObject(), LODIndex);
	}
}

void URuntimeMeshProvider::MarkAllLODsDirty_Implementation()
{
	auto Target = GetTarget();
	if (Target)
	{
		IRuntimeMeshProviderTargetInterface::Execute_MarkAllLODsDirty(Target.GetObject());
	}
}

void URuntimeMeshProvider::CreateSection_Implementation(int32 LODIndex, int32 SectionId, const FRuntimeMeshSectionProperties& SectionProperties)
{
	auto Target = GetTarget();
	if (Target)
	{
		IRuntimeMeshProviderTargetInterface::Execute_CreateSection(Target.GetObject(), LODIndex, SectionId, SectionProperties);
	}
}

void URuntimeMeshProvider::SetSectionVisibility_Implementation(int32 LODIndex, int32 SectionId, bool bIsVisible)
{
	auto Target = GetTarget();
	if (Target)
	{
		IRuntimeMeshProviderTargetInterface::Execute_SetSectionVisibility(Target.GetObject(), LODIndex, SectionId, bIsVisible);
	}
}

void URuntimeMeshProvider::SetSectionCastsShadow_Implementation(int32 LODIndex, int32 SectionId, bool bCastsShadow)
{
	auto Target = GetTarget();
	if (Target)
	{
		IRuntimeMeshProviderTargetInterface::Execute_SetSectionCastsShadow(Target.GetObject(), LODIndex, SectionId, bCastsShadow);
	}
}

void URuntimeMeshProvider::MarkSectionDirty_Implementation(int32 LODIndex, int32 SectionId)
{
	auto Target = GetTarget();
	if (Target)
	{
		IRuntimeMeshProviderTargetInterface::Execute_MarkSectionDirty(Target.GetObject(), LODIndex, SectionId);
	}
}

void URuntimeMeshProvider::ClearSection_Implementation(int32 LODIndex, int32 SectionId)
{
	auto Target = GetTarget();
	if (Target)
	{
		IRuntimeMeshProviderTargetInterface::Execute_ClearSection(Target.GetObject(), LODIndex, SectionId);
	}
}

void URuntimeMeshProvider::RemoveSection_Implementation(int32 LODIndex, int32 SectionId)
{
	auto Target = GetTarget();
	if (Target)
	{
		IRuntimeMeshProviderTargetInterface::Execute_RemoveSection(Target.GetObject(), LODIndex, SectionId);
	}
}

void URuntimeMeshProvider::MarkCollisionDirty_Implementation()
{
	auto Target = GetTarget();
	if (Target)
	{
		IRuntimeMeshProviderTargetInterface::Execute_MarkCollisionDirty(Target.GetObject());
	}
}

void URuntimeMeshProvider::SetupMaterialSlot_Implementation(int32 MaterialSlot, FName SlotName, UMaterialInterface* InMaterial)
{
	auto Target = GetTarget();
	if (Target)
	{
		IRuntimeMeshProviderTargetInterface::Execute_SetupMaterialSlot(Target.GetObject(), MaterialSlot, SlotName, InMaterial);
	}
}

int32 URuntimeMeshProvider::GetMaterialIndex_Implementation(FName MaterialSlotName)
{
	auto Target = GetTarget();
	if (Target)
	{
		return IRuntimeMeshProviderTargetInterface::Execute_GetMaterialIndex(Target.GetObject(), MaterialSlotName);
	}
	return INDEX_NONE;
}

bool URuntimeMeshProvider::IsMaterialSlotNameValid_Implementation(FName MaterialSlotName) const
{
	auto Target = GetTarget();
	if (Target)
	{
		return IRuntimeMeshProviderTargetInterface::Execute_IsMaterialSlotNameValid(Target.GetObject(), MaterialSlotName);
	}
	return false;
}

FRuntimeMeshMaterialSlot URuntimeMeshProvider::GetMaterialSlot_Implementation(int32 SlotIndex)
{
	auto Target = GetTarget();
	if (Target)
	{
		return IRuntimeMeshProviderTargetInterface::Execute_GetMaterialSlot(Target.GetObject(), SlotIndex);
	}
	return FRuntimeMeshMaterialSlot();
}

int32 URuntimeMeshProvider::GetNumMaterials_Implementation()
{
	auto Target = GetTarget();
	if (Target)
	{
		return IRuntimeMeshProviderTargetInterface::Execute_GetNumMaterials(Target.GetObject());
	}
	return 0;
}

TArray<FName> URuntimeMeshProvider::GetMaterialSlotNames_Implementation()
{
	auto Target = GetTarget();
	if (Target)
	{
		return IRuntimeMeshProviderTargetInterface::Execute_GetMaterialSlotNames(Target.GetObject());
	}
	return TArray<FName>();
}

TArray<FRuntimeMeshMaterialSlot> URuntimeMeshProvider::GetMaterialSlots_Implementation()
{
	auto Target = GetTarget();
	if (Target)
	{
		return IRuntimeMeshProviderTargetInterface::Execute_GetMaterialSlots(Target.GetObject());
	}
	return TArray<FRuntimeMeshMaterialSlot>();
}

UMaterialInterface* URuntimeMeshProvider::GetMaterial_Implementation(int32 SlotIndex)
{
	auto Target = GetTarget();
	if (Target)
	{
		return IRuntimeMeshProviderTargetInterface::Execute_GetMaterial(Target.GetObject(), SlotIndex);
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
	return Super::IsReadyForFinishDestroy() && ReferenceAnchor.IsFree();
}






