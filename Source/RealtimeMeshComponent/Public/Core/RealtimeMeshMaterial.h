// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreFwd.h"
#include "UObject/ObjectMacros.h"
#include "RealtimeMeshMaterial.generated.h"

/* Material slot, including an optional name, and the material reference
 * that a section can then index to set the material on a mesh section
 */
USTRUCT(BlueprintType)
struct FRealtimeMeshMaterialSlot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RealtimeMesh|Material")
	FName SlotName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RealtimeMesh|Material")
	TObjectPtr<UMaterialInterface> Material;

	FRealtimeMeshMaterialSlot() : SlotName(NAME_None), Material(nullptr) { }

	FRealtimeMeshMaterialSlot(const FName& InSlotName, UMaterialInterface* InMaterial)
		: SlotName(InSlotName), Material(InMaterial) { }
};