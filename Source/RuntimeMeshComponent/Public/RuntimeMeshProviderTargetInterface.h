// Copyright 2016-2020 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "RuntimeMeshRenderable.h"
#include "RuntimeMeshCollision.h"
#include "RuntimeMeshGCSharedPointer.h"
#include "RuntimeMeshProviderTargetInterface.generated.h"

class URuntimeMesh;

UINTERFACE(BlueprintType, Blueprintable)
class URuntimeMeshProviderTargetInterface : public UInterface
{
	GENERATED_BODY()
};

class RUNTIMEMESHCOMPONENT_API IRuntimeMeshProviderTargetInterface
{
	GENERATED_BODY()
public:
	virtual FRuntimeMeshWeakRef GetMeshReference() = 0;
	virtual void ShutdownInternal() = 0;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void Shutdown();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void ConfigureLODs(const TArray<FRuntimeMeshLODProperties>& InLODs);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void SetLODScreenSize(int32 LODIndex, float ScreenSize);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void MarkLODDirty(int32 LODIndex);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void MarkAllLODsDirty();


	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void CreateSection(int32 LODIndex, int32 SectionId, const FRuntimeMeshSectionProperties& SectionProperties);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void SetSectionVisibility(int32 LODIndex, int32 SectionId, bool bIsVisible);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void SetSectionCastsShadow(int32 LODIndex, int32 SectionId, bool bCastsShadow);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void MarkSectionDirty(int32 LODIndex, int32 SectionId);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void ClearSection(int32 LODIndex, int32 SectionId);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void RemoveSection(int32 LODIndex, int32 SectionId);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void MarkCollisionDirty();


	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void SetupMaterialSlot(int32 MaterialSlot, FName SlotName, UMaterialInterface* InMaterial);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	int32 GetMaterialIndex(FName MaterialSlotName);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	bool IsMaterialSlotNameValid(FName MaterialSlotName) const;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	FRuntimeMeshMaterialSlot GetMaterialSlot(int32 SlotIndex);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	int32 GetNumMaterials();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	TArray<FName> GetMaterialSlotNames();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	TArray<FRuntimeMeshMaterialSlot> GetMaterialSlots();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	UMaterialInterface* GetMaterial(int32 SlotIndex);
};
