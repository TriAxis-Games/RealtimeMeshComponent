// Copyright 2016-2020 Chris Conway (Koderz). All Rights Reserved.


#include "RuntimeMeshProviderTargetInterface.h"

void URuntimeMeshProviderTargetInterface::Shutdown_Implementation()
{

}

void URuntimeMeshProviderTargetInterface::ConfigureLODs_Implementation(const TArray<FRuntimeMeshLODProperties>& InLODs)
{

}

void URuntimeMeshProviderTargetInterface::SetLODScreenSize_Implementation(int32 LODIndex, float ScreenSize)
{

}

void URuntimeMeshProviderTargetInterface::MarkLODDirty_Implementation(int32 LODIndex)
{

}

void URuntimeMeshProviderTargetInterface::MarkAllLODsDirty_Implementation()
{

}

void URuntimeMeshProviderTargetInterface::CreateSection_Implementation(int32 LODIndex, int32 SectionId, const FRuntimeMeshSectionProperties& SectionProperties)
{

}

void URuntimeMeshProviderTargetInterface::SetSectionVisibility_Implementation(int32 LODIndex, int32 SectionId, bool bIsVisible)
{

}

void URuntimeMeshProviderTargetInterface::SetSectionCastsShadow_Implementation(int32 LODIndex, int32 SectionId, bool bCastsShadow)
{

}

void URuntimeMeshProviderTargetInterface::MarkSectionDirty_Implementation(int32 LODIndex, int32 SectionId)
{

}

void URuntimeMeshProviderTargetInterface::ClearSection_Implementation(int32 LODIndex, int32 SectionId)
{

}

void URuntimeMeshProviderTargetInterface::RemoveSection_Implementation(int32 LODIndex, int32 SectionId)
{

}

void URuntimeMeshProviderTargetInterface::MarkCollisionDirty_Implementation()
{

}

void URuntimeMeshProviderTargetInterface::SetupMaterialSlot_Implementation(int32 MaterialSlot, FName SlotName, UMaterialInterface* InMaterial)
{

}

int32 URuntimeMeshProviderTargetInterface::GetMaterialIndex_Implementation(FName MaterialSlotName)
{
	return INDEX_NONE;
}

bool URuntimeMeshProviderTargetInterface::IsMaterialSlotNameValid_Implementation(FName MaterialSlotName) const
{
	return false;
}

FRuntimeMeshMaterialSlot URuntimeMeshProviderTargetInterface::GetMaterialSlot_Implementation(int32 SlotIndex)
{
	return FRuntimeMeshMaterialSlot();
}

int32 URuntimeMeshProviderTargetInterface::GetNumMaterials_Implementation()
{
	return 0;
}

TArray<FName> URuntimeMeshProviderTargetInterface::GetMaterialSlotNames_Implementation()
{
	return TArray<FName>();
}

TArray<FRuntimeMeshMaterialSlot> URuntimeMeshProviderTargetInterface::GetMaterialSlots_Implementation()
{
	return TArray<FRuntimeMeshMaterialSlot>();
}

UMaterialInterface* URuntimeMeshProviderTargetInterface::GetMaterial_Implementation(int32 SlotIndex)
{
	return nullptr;
}
