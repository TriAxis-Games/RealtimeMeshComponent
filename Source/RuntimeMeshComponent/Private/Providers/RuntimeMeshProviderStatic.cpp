// Copyright 2016-2019 Chris Conway (Koderz). All Rights Reserved.


#include "RuntimeMeshProviderStatic.h"


FRuntimeMeshProviderStaticProxy::FRuntimeMeshProviderStaticProxy(TWeakObjectPtr<URuntimeMeshProvider> InParent, const FVector& InBoxRadius, UMaterialInterface* InMaterial)
	: FRuntimeMeshProviderProxy(InParent)
{

}

FRuntimeMeshProviderStaticProxy::~FRuntimeMeshProviderStaticProxy()
{

}
