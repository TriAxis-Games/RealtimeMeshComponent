// Copyright 2016-2020 Chris Conway (Koderz). All Rights Reserved.


#include "Providers/RuntimeMeshProviderMemoryCache.h"

FRuntimeMeshProviderMemoryCacheProxy::FRuntimeMeshProviderMemoryCacheProxy(TWeakObjectPtr<URuntimeMeshProvider> InParent, const FRuntimeMeshProviderProxyPtr& InNextProvider)
	: FRuntimeMeshProviderProxyPassThrough(InParent, InNextProvider)
{
}

FRuntimeMeshProviderMemoryCacheProxy::~FRuntimeMeshProviderMemoryCacheProxy()
{
}

void FRuntimeMeshProviderMemoryCacheProxy::ClearCache()
{
	CachedMeshData.Empty();
	CachedCollisionSettings.Reset();
	CachedHasCollisionMesh.Reset();
	CachedCollisionData.Reset();
}

void FRuntimeMeshProviderMemoryCacheProxy::CreateSection(int32 LODIndex, int32 SectionId, const FRuntimeMeshSectionProperties& SectionProperties)
{
	CachedMeshData.Remove(GenerateKeyFromLODAndSection(LODIndex, SectionId));
	FRuntimeMeshProviderProxyPassThrough::CreateSection(LODIndex, SectionId, SectionProperties);
}

void FRuntimeMeshProviderMemoryCacheProxy::MarkSectionDirty(int32 LODIndex, int32 SectionId)
{
	CachedMeshData.Remove(GenerateKeyFromLODAndSection(LODIndex, SectionId));
	FRuntimeMeshProviderProxyPassThrough::MarkSectionDirty(LODIndex, SectionId);
}

void FRuntimeMeshProviderMemoryCacheProxy::RemoveSection(int32 LODIndex, int32 SectionId)
{
	CachedMeshData.Remove(GenerateKeyFromLODAndSection(LODIndex, SectionId));
	FRuntimeMeshProviderProxyPassThrough::RemoveSection(LODIndex, SectionId);
}

void FRuntimeMeshProviderMemoryCacheProxy::MarkCollisionDirty()
{
	CachedCollisionSettings.Reset();
	CachedHasCollisionMesh.Reset();
	CachedCollisionData.Reset();
	FRuntimeMeshProviderProxyPassThrough::MarkCollisionDirty();
}

bool FRuntimeMeshProviderMemoryCacheProxy::GetSectionMeshForLOD(int32 LODIndex, int32 SectionId, FRuntimeMeshRenderableMeshData& MeshData)
{
	TOptional<FRuntimeMeshRenderableMeshData>* CachedData = CachedMeshData.Find(GenerateKeyFromLODAndSection(LODIndex, SectionId));
	if (CachedData)
	{
		if (CachedData->IsSet())
		{
			MeshData = CachedData->GetValue();
		}
		return CachedData->IsSet();
	}

	if (NextProvider.IsValid())
	{
		bool bHasData = NextProvider->GetSectionMeshForLOD(LODIndex, SectionId, MeshData);
		TOptional<FRuntimeMeshRenderableMeshData>& NewCache = CachedMeshData.FindOrAdd(GenerateKeyFromLODAndSection(LODIndex, SectionId));
		if (bHasData)
		{
			NewCache = MeshData;
		}
		return bHasData;
	}
	return false;
}

FRuntimeMeshCollisionSettings FRuntimeMeshProviderMemoryCacheProxy::GetCollisionSettings()
{
	if (CachedCollisionSettings.IsSet())
	{
		return CachedCollisionSettings.Get(FRuntimeMeshCollisionSettings());
	}

	if (NextProvider.IsValid())
	{
		FRuntimeMeshCollisionSettings NewSettings = NextProvider->GetCollisionSettings();
		CachedCollisionSettings = NewSettings;
		return NewSettings;
	}
	return FRuntimeMeshCollisionSettings();
}

bool FRuntimeMeshProviderMemoryCacheProxy::HasCollisionMesh()
{
	if (CachedHasCollisionMesh.IsSet())
	{
		return CachedHasCollisionMesh.Get(false);
	}

	if (NextProvider.IsValid())
	{
		bool bHasData = NextProvider->HasCollisionMesh();
		CachedHasCollisionMesh = bHasData;
		return bHasData;
	}
	return false;
}

bool FRuntimeMeshProviderMemoryCacheProxy::GetCollisionMesh(FRuntimeMeshCollisionData& CollisionData)
{
	if (CachedCollisionData.IsSet())
	{		
		CollisionData = CachedCollisionData->Value;
		return CachedCollisionData->Key;
	}

	if (NextProvider.IsValid())
	{
		bool bHasData = NextProvider->GetCollisionMesh(CollisionData);
		CachedCollisionData = TPair<bool, FRuntimeMeshCollisionData>(bHasData, CollisionData);
		return bHasData;
	}
	return false;
}

bool FRuntimeMeshProviderMemoryCacheProxy::IsThreadSafe() const
{
	// Since all we're doing is acting as an intermediary cache with no ties to anything
	// it's perfectly safe for this to be multithreaded. The RMC guarantees to never 
	// call the same section on multiple threads at the same time
	return true;
}