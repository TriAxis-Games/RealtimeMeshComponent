// Fill out your copyright notice in the Description page of Project Settings.


#include "RuntimeMeshProviderFactory.h"
#include  "RuntimeMeshProvider.h"

URuntimeMeshProviderFactory::URuntimeMeshProviderFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = URuntimeMeshProvider::StaticClass();
}

UObject* URuntimeMeshProviderFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<URuntimeMeshProvider>(InParent, InClass, InName, Flags);
}

