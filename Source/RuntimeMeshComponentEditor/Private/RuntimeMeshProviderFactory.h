// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "RuntimeMeshProviderFactory.generated.h"

/**
 * 
 */
UCLASS(hidecategories = Object, collapsecategories)
class URuntimeMeshProviderFactory : public UFactory
{
	GENERATED_UCLASS_BODY()

	UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};
