// Copyright 2016-2019 Chris Conway (Koderz). All Rights Reserved.


#include "RealtimeMeshProviderStatic.h"


FRealtimeMeshProviderStaticProxy::FRealtimeMeshProviderStaticProxy(TWeakObjectPtr<URealtimeMeshProvider> InParent, const FVector& InBoxRadius, UMaterialInterface* InMaterial)
	: FRealtimeMeshProviderProxy(InParent)
{

}

FRealtimeMeshProviderStaticProxy::~FRealtimeMeshProviderStaticProxy()
{

}
