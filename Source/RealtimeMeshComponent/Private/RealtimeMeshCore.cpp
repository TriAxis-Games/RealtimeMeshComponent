// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#include "RealtimeMeshCore.h"

#include "Data/RealtimeMeshSectionGroup.h"
#include "Data/RealtimeMeshLOD.h"
#include "Data/RealtimeMeshSection.h"
#include "Data/RealtimeMeshData.h"
#include "RenderProxy/RealtimeMeshLODProxy.h"
#include "RenderProxy/RealtimeMeshProxy.h"
#include "RenderProxy/RealtimeMeshSectionGroupProxy.h"
#include "RenderProxy/RealtimeMeshSectionProxy.h"
#include "RenderProxy/RealtimeMeshVertexFactory.h"

namespace RealtimeMesh
{
	FArchive& operator<<(FArchive& Ar, FRealtimeMeshStreamKey& Key)
	{
		Ar << Key.StreamName;
		Ar << Key.StreamType;
		return Ar;
	}
	
	FRealtimeMeshVertexFactoryRef FRealtimeMeshClassFactory::CreateVertexFactory(ERHIFeatureLevel::Type InFeatureLevel) const
	{
		return MakeShareable(new FRealtimeMeshLocalVertexFactory(InFeatureLevel), FRealtimeMeshRenderThreadDeleter<FRealtimeMeshLocalVertexFactory>());
	}

	FRealtimeMeshSectionProxyRef FRealtimeMeshClassFactory::CreateSectionProxy(const FRealtimeMeshProxyRef& InProxy, FRealtimeMeshSectionKey InKey,
		const FRealtimeMeshSectionProxyInitializationParametersRef& InitParams) const
	{
		return MakeShareable(new FRealtimeMeshSectionProxy(this->AsShared(), InProxy, InKey, InitParams),
			FRealtimeMeshRenderThreadDeleter<FRealtimeMeshSectionProxy>());
	}

	TSharedRef<FRealtimeMeshSectionGroupProxy> FRealtimeMeshClassFactory::CreateSectionGroupProxy(const FRealtimeMeshProxyRef& InProxy,
		FRealtimeMeshSectionGroupKey InKey, const FRealtimeMeshSectionGroupProxyInitializationParametersRef& InitParams) const
	{
		return MakeShareable(new FRealtimeMeshSectionGroupProxy(this->AsShared(), InProxy, InKey, InitParams),
			FRealtimeMeshRenderThreadDeleter<FRealtimeMeshSectionGroupProxy>());
	}

	TSharedRef<FRealtimeMeshLODProxy> FRealtimeMeshClassFactory::CreateLODProxy(const FRealtimeMeshProxyRef& InProxy, FRealtimeMeshLODKey InKey,
		const FRealtimeMeshLODProxyInitializationParametersRef& InitParams) const
	{
		return MakeShareable(new FRealtimeMeshLODProxy(this->AsShared(), InProxy, InKey, InitParams),
			FRealtimeMeshRenderThreadDeleter<FRealtimeMeshLODProxy>());
	}

	TSharedRef<FRealtimeMeshProxy> FRealtimeMeshClassFactory::CreateRealtimeMeshProxy(const TSharedRef<FRealtimeMesh>& InMesh) const
	{
		return MakeShareable(new FRealtimeMeshProxy(this->AsShared(), InMesh),
			FRealtimeMeshRenderThreadDeleter<FRealtimeMeshProxy>());
	}
	
	FRealtimeMeshSectionDataRef FRealtimeMeshClassFactory::CreateSection(const FRealtimeMeshRef& InMesh, FRealtimeMeshSectionKey InKey,
		const FRealtimeMeshSectionConfig& InConfig, const FRealtimeMeshStreamRange& InStreamRange) const
	{
		return MakeShared<FRealtimeMeshSectionData>(this->AsShared(), InMesh, InKey, InConfig, InStreamRange);
	}

	TSharedRef<FRealtimeMeshSectionGroup> FRealtimeMeshClassFactory::CreateSectionGroup(const FRealtimeMeshRef& InMesh, FRealtimeMeshSectionGroupKey InKey) const
	{
		return MakeShared<FRealtimeMeshSectionGroup>(this->AsShared(), InMesh, InKey);
	}
	
	FRealtimeMeshLODDataRef FRealtimeMeshClassFactory::CreateLOD(const FRealtimeMeshRef& InMesh, FRealtimeMeshLODKey InKey, const FRealtimeMeshLODConfig& InConfig) const
	{
		return MakeShared<FRealtimeMeshLODData>(this->AsShared(), InMesh, InKey, InConfig);
	}

	FRealtimeMeshRef FRealtimeMeshClassFactory::CreateRealtimeMesh() const
	{
		check(false && "Cannot create abstract FRealtimeMesh"); return MakeShareable(static_cast<FRealtimeMesh*>(nullptr));
	}
}
