// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#include "RealtimeMeshCore.h"

#include "RealtimeMesh.h"
#include "Data/RealtimeMeshBufferSet.h"
#include "Data/RealtimeMeshLOD.h"
#include "Data/RealtimeMeshSection.h"
#include "Data/RealtimeMeshData.h"
#include "Core/RealtimeMeshKeys.h"
#include "Data/RealtimeMeshUpdateBuilder.h"
#include "RenderProxy/RealtimeMeshLODProxy.h"
#include "RenderProxy/RealtimeMeshProxy.h"
#include "RenderProxy/RealtimeMeshBufferSetProxy.h"
#include "RenderProxy/RealtimeMeshSectionProxy.h"
#include "RenderProxy/RealtimeMeshVertexFactory.h"


enum class ERealtimeMeshStreamType_OLD
{
	Unknown,
	Vertex,
	Index,
};
static_assert(sizeof(ERealtimeMeshStreamType_OLD) == 4);

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
	void FRealtimeMeshContext::SetOwnerMesh(URealtimeMesh* InOwningMesh, const FRealtimeMeshRef& InOwner)
	{
		OwningMesh = InOwningMesh;
		Owner = InOwner;
	}

	ERHIFeatureLevel::Type FRealtimeMeshContext::GetFeatureLevel() const
	{
		if (const auto ProxyPinned = Proxy.Pin()) { return ProxyPinned->GetRHIFeatureLevel(); }
		return GMaxRHIFeatureLevel;
	}

	FRealtimeMeshSectionRef FRealtimeMeshContext::CreateSection(const FRealtimeMeshSectionKey& InKey) const
	{
		const auto Mesh = Owner.Pin();
		check(Mesh.IsValid());
		return Mesh->CreateSection(InKey);
	}

	FRealtimeMeshSectionGroupRef FRealtimeMeshContext::CreateSectionGroup(const FRealtimeMeshBufferSetKey& InKey) const
	{
		const auto Mesh = Owner.Pin();
		check(Mesh.IsValid());
		return Mesh->CreateSectionGroup(InKey);
	}

	FRealtimeMeshLODRef FRealtimeMeshContext::CreateLOD(const FRealtimeMeshLODKey& InKey) const
	{
		const auto Mesh = Owner.Pin();
		check(Mesh.IsValid());
		return Mesh->CreateLOD(InKey);
	}

	FRealtimeMeshUpdateStateRef FRealtimeMeshContext::CreateUpdateState() const
	{
		const auto Mesh = Owner.Pin();
		check(Mesh.IsValid());
		return Mesh->CreateUpdateState();
	}

	// RT-side factories construct directly. The leaves don't currently customize
	// RT proxy types (only data-side); if that ever changes, add a virtual on
	// FRealtimeMeshProxy and route through the proxy weak-ptr.
	FRealtimeMeshSectionProxyRef FRealtimeMeshContext::CreateSectionProxy(const FRealtimeMeshSectionKey& InKey) const
	{
		return MakeShareable(new FRealtimeMeshSectionProxy(ConstCastSharedRef<FRealtimeMeshContext>(this->AsShared()), InKey),
		                     FRealtimeMeshRenderThreadDeleter<FRealtimeMeshSectionProxy>());
	}

	FRealtimeMeshSectionGroupProxyRef FRealtimeMeshContext::CreateSectionGroupProxy(const FRealtimeMeshBufferSetKey& InKey) const
	{
		return MakeShareable(new FRealtimeMeshBufferSetProxy(ConstCastSharedRef<FRealtimeMeshContext>(this->AsShared()), InKey),
		                     FRealtimeMeshRenderThreadDeleter<FRealtimeMeshBufferSetProxy>());
	}

	FRealtimeMeshLODProxyRef FRealtimeMeshContext::CreateLODProxy(const FRealtimeMeshLODKey& InKey) const
	{
		return MakeShareable(new FRealtimeMeshLODProxy(ConstCastSharedRef<FRealtimeMeshContext>(this->AsShared()), InKey),
		                     FRealtimeMeshRenderThreadDeleter<FRealtimeMeshLODProxy>());
	}

	FRealtimeMeshVertexFactoryRef FRealtimeMeshContext::CreateVertexFactory() const
	{
		return MakeShareable(new FRealtimeMeshLocalVertexFactory(GetFeatureLevel()),
		                     FRealtimeMeshRenderResourceDeleter<FRealtimeMeshLocalVertexFactory>());
	}
}

