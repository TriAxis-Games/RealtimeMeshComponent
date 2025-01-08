// Copyright (c) 2015-2025 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreFwd.h"

/* Material slot, including an optional name, and the material reference
 * that a section can then index to set the material on a mesh section
 */
struct FRealtimeMeshMaterialSlot
{
	FName SlotName;
	TObjectPtr<UMaterialInterface> Material;

	FRealtimeMeshMaterialSlot() : SlotName(NAME_None), Material(nullptr) { }

	FRealtimeMeshMaterialSlot(const FName& InSlotName, UMaterialInterface* InMaterial)
		: SlotName(InSlotName), Material(InMaterial) { }
};