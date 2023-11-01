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


enum class ERealtimeMeshStreamType_OLD
{
	Unknown,
	Vertex,
	Index,
};

FArchive& operator<<(FArchive& Ar, FRealtimeMeshStreamKey& Key)
{
	Ar << Key.StreamName;

	if (Ar.CustomVer(RealtimeMesh::FRealtimeMeshVersion::GUID) < RealtimeMesh::FRealtimeMeshVersion::StreamKeySizeChanged)
	{
		check(Ar.IsLoading());
		ERealtimeMeshStreamType_OLD OldKey;
		Ar << OldKey;
		Key.StreamType = static_cast<ERealtimeMeshStreamType>(OldKey);
	}
	else
	{
		Ar << Key.StreamType;		
	}
	
	return Ar;
}

namespace RealtimeMesh
{
	ERHIFeatureLevel::Type FRealtimeMeshSharedResources::GetFeatureLevel() const
	{
		if (const auto ProxyPinned = Proxy.Pin()) { return ProxyPinned->GetRHIFeatureLevel(); }
		return GMaxRHIFeatureLevel;
	}

	FRealtimeMeshVertexFactoryRef FRealtimeMeshSharedResources::CreateVertexFactory() const
	{
		return MakeShareable(new FRealtimeMeshLocalVertexFactory(GetFeatureLevel()), FRealtimeMeshRenderThreadDeleter<FRealtimeMeshLocalVertexFactory>());
	}

	FRealtimeMeshSectionProxyRef FRealtimeMeshSharedResources::CreateSectionProxy(const FRealtimeMeshSectionKey& InKey) const
	{
		return MakeShareable(new FRealtimeMeshSectionProxy(ConstCastSharedRef<FRealtimeMeshSharedResources>(this->AsShared()), InKey),
		                     FRealtimeMeshRenderThreadDeleter<FRealtimeMeshSectionProxy>());
	}

	TSharedRef<FRealtimeMeshSectionGroupProxy> FRealtimeMeshSharedResources::CreateSectionGroupProxy(const FRealtimeMeshSectionGroupKey& InKey) const
	{
		return MakeShareable(new FRealtimeMeshSectionGroupProxy(ConstCastSharedRef<FRealtimeMeshSharedResources>(this->AsShared()), InKey),
		                     FRealtimeMeshRenderThreadDeleter<FRealtimeMeshSectionGroupProxy>());
	}

	TSharedRef<FRealtimeMeshLODProxy> FRealtimeMeshSharedResources::CreateLODProxy(const FRealtimeMeshLODKey& InKey) const
	{
		return MakeShareable(new FRealtimeMeshLODProxy(ConstCastSharedRef<FRealtimeMeshSharedResources>(this->AsShared()), InKey),
		                     FRealtimeMeshRenderThreadDeleter<FRealtimeMeshLODProxy>());
	}

	TSharedRef<FRealtimeMeshProxy> FRealtimeMeshSharedResources::CreateRealtimeMeshProxy() const
	{
		return MakeShareable(new FRealtimeMeshProxy(ConstCastSharedRef<FRealtimeMeshSharedResources>(this->AsShared())),
		                     FRealtimeMeshRenderThreadDeleter<FRealtimeMeshProxy>());
	}

	FRealtimeMeshSectionRef FRealtimeMeshSharedResources::CreateSection(const FRealtimeMeshSectionKey& InKey) const
	{
		return MakeShared<FRealtimeMeshSection>(ConstCastSharedRef<FRealtimeMeshSharedResources>(this->AsShared()), InKey);
	}

	TSharedRef<FRealtimeMeshSectionGroup> FRealtimeMeshSharedResources::CreateSectionGroup(const FRealtimeMeshSectionGroupKey& InKey) const
	{
		return MakeShared<FRealtimeMeshSectionGroup>(ConstCastSharedRef<FRealtimeMeshSharedResources>(this->AsShared()), InKey);
	}

	FRealtimeMeshLODDataRef FRealtimeMeshSharedResources::CreateLOD(const FRealtimeMeshLODKey& InKey) const
	{
		return MakeShared<FRealtimeMeshLODData>(ConstCastSharedRef<FRealtimeMeshSharedResources>(this->AsShared()), InKey);
	}

	FRealtimeMeshRef FRealtimeMeshSharedResources::CreateRealtimeMesh() const
	{
		check(false && "Cannot create abstract FRealtimeMesh");
		return MakeShareable(static_cast<FRealtimeMesh*>(nullptr));
	}

	FRealtimeMeshSharedResourcesRef FRealtimeMeshSharedResources::CreateSharedResources() const
	{
		return MakeShared<FRealtimeMeshSharedResources>();
	}
}
