// Copyright 2016-2020 TriAxis Games L.L.C. All Rights Reserved.


#include "Providers/RuntimeMeshProviderMemoryCache.h"


void URuntimeMeshProviderMemoryCache::ClearCache()
{
	{
		FScopeLock Lock(&MeshSyncRoot);
		CachedMeshData.Empty();
	}
	{
		FScopeLock Lock(&CollisionSyncRoot);
		CachedCollisionSettings.Reset();
		CachedHasCollisionMesh.Reset();
		CachedCollisionData.Reset();
	}
}



bool URuntimeMeshProviderMemoryCache::GetSectionMeshForLOD(int32 LODIndex, int32 SectionId, FRuntimeMeshRenderableMeshData& MeshData)
{
	FScopeLock Lock(&MeshSyncRoot);

	const auto* FoundLOD = CachedMeshData.Find(LODIndex);
	if (FoundLOD)
	{
		const auto* FoundSection = FoundLOD->Find(SectionId);
		if (FoundSection)
		{
			if (FoundSection->IsSet())
			{
				MeshData = FoundSection->GetValue();
			}

			return FoundSection->IsSet();
		}
	}

	if (GetChildProvider())
	{
		bool bHasData = GetChildProvider()->GetSectionMeshForLOD(LODIndex, SectionId, MeshData);
		TOptional<FRuntimeMeshRenderableMeshData>& NewCache = CachedMeshData.FindOrAdd(LODIndex).FindOrAdd(SectionId);
		if (bHasData)
		{
			NewCache = MeshData;
		}
		return bHasData;
	}

	return false;
}

bool URuntimeMeshProviderMemoryCache::GetAllSectionsMeshForLOD(int32 LODIndex, TMap<int32, FRuntimeMeshSectionData>& MeshDatas)
{
	FScopeLock Lock(&MeshSyncRoot);

	const auto* FoundLOD = CachedMeshData.Find(LODIndex);
	if (FoundLOD)
	{
		MeshDatas.Empty();

		// Add all cached sections
		for (const auto& CachedSection : *FoundLOD)
		{
			FRuntimeMeshSectionData& SectionData = MeshDatas.FindOrAdd(CachedSection.Key);

			// Copy the section properties
			SectionData.Properties = CacheSectionConfig.FindOrAdd(LODIndex).FindChecked(CachedSection.Key);
			
			if (CachedSection.Value.IsSet())
			{
				SectionData.MeshData = CachedSection.Value.GetValue();
			}
		}

		return FoundLOD->Num() > 0;
	}

	if (GetChildProvider())
	{
		bool bHasData = GetChildProvider()->GetAllSectionsMeshForLOD(LODIndex, MeshDatas);

		TMap<int32, TOptional<FRuntimeMeshRenderableMeshData>>& NewCache = CachedMeshData.FindOrAdd(LODIndex);
		TMap<int32, FRuntimeMeshSectionProperties>& PropertiesCache = CacheSectionConfig.FindOrAdd(LODIndex);

		NewCache.Empty();
		PropertiesCache.Empty();

		if (bHasData)
		{
			for (const auto& NewSection : MeshDatas)
			{
				PropertiesCache.FindOrAdd(NewSection.Key) = NewSection.Value.Properties;
				NewCache.FindOrAdd(NewSection.Key) = NewSection.Value.MeshData;
			}
		}

		return bHasData;
	}

	return false;
}

FRuntimeMeshCollisionSettings URuntimeMeshProviderMemoryCache::GetCollisionSettings()
{
	FScopeLock Lock(&CollisionSyncRoot);

	if (CachedCollisionSettings.IsSet())
	{
		return CachedCollisionSettings.Get(FRuntimeMeshCollisionSettings());
	}

	if (GetChildProvider())
	{
		FRuntimeMeshCollisionSettings NewSettings = GetChildProvider()->GetCollisionSettings();
		CachedCollisionSettings = NewSettings;
		return NewSettings;
	}
	return FRuntimeMeshCollisionSettings();
}

bool URuntimeMeshProviderMemoryCache::HasCollisionMesh()
{
	FScopeLock Lock(&CollisionSyncRoot);

	if (CachedHasCollisionMesh.IsSet())
	{
		return CachedHasCollisionMesh.Get(false);
	}

	if (GetChildProvider())
	{
		bool bHasData = GetChildProvider()->HasCollisionMesh();
		CachedHasCollisionMesh = bHasData;
		return bHasData;
	}
	return false;
}

bool URuntimeMeshProviderMemoryCache::GetCollisionMesh(FRuntimeMeshCollisionData& CollisionData)
{
	FScopeLock Lock(&CollisionSyncRoot);

	if (CachedCollisionData.IsSet())
	{
		CollisionData = CachedCollisionData->Value;
		return CachedCollisionData->Key;
	}

	if (GetChildProvider())
	{
		bool bHasData = GetChildProvider()->GetCollisionMesh(CollisionData);
		CachedCollisionData = TPair<bool, FRuntimeMeshCollisionData>(bHasData, CollisionData);
		return bHasData;
	}
	return false;
}

bool URuntimeMeshProviderMemoryCache::IsThreadSafe()
{
	// We ourselves are threadsafe, so our thread safety depends on the child provider
	return GetChildProvider()->IsThreadSafe();
}




void URuntimeMeshProviderMemoryCache::ConfigureLODs(const TArray<FRuntimeMeshLODProperties>& InLODs)
{
	{
		FScopeLock Lock(&MeshSyncRoot);
		CacheSectionConfig.Empty();
	}
	ClearCacheLOD(INDEX_NONE);
	URuntimeMeshProviderPassthrough::ConfigureLODs(InLODs);
}

void URuntimeMeshProviderMemoryCache::MarkLODDirty(int32 LODIndex)
{
	ClearCacheLOD(LODIndex);
	URuntimeMeshProviderPassthrough::MarkLODDirty(LODIndex);
}

void URuntimeMeshProviderMemoryCache::MarkAllLODsDirty()
{
	ClearCacheLOD(INDEX_NONE);
	URuntimeMeshProviderPassthrough::MarkAllLODsDirty();
}

void URuntimeMeshProviderMemoryCache::CreateSection(int32 LODIndex, int32 SectionId, const FRuntimeMeshSectionProperties& SectionProperties)
{
	{
		FScopeLock Lock(&MeshSyncRoot);
		CacheSectionConfig.FindOrAdd(LODIndex).FindOrAdd(SectionId) = SectionProperties;
	}
	ClearCacheSection(LODIndex, SectionId);
	URuntimeMeshProviderPassthrough::CreateSection(LODIndex, SectionId, SectionProperties);
}

void URuntimeMeshProviderMemoryCache::MarkSectionDirty(int32 LODIndex, int32 SectionId)
{
	ClearCacheSection(LODIndex, SectionId);
	URuntimeMeshProviderPassthrough::MarkSectionDirty(LODIndex, SectionId);
}

void URuntimeMeshProviderMemoryCache::ClearSection(int32 LODIndex, int32 SectionId)
{
	ClearCacheSection(LODIndex, SectionId);
	URuntimeMeshProviderPassthrough::ClearSection(LODIndex, SectionId);
}

void URuntimeMeshProviderMemoryCache::RemoveSection(int32 LODIndex, int32 SectionId)
{
	{
		FScopeLock Lock(&MeshSyncRoot);
		CacheSectionConfig.FindOrAdd(LODIndex).Remove(SectionId);
	}
	ClearCacheSection(LODIndex, SectionId);
	URuntimeMeshProviderPassthrough::RemoveSection(LODIndex, SectionId);
}

void URuntimeMeshProviderMemoryCache::MarkCollisionDirty()
{
	FScopeLock Lock(&CollisionSyncRoot);

	CachedCollisionSettings.Reset();
	CachedHasCollisionMesh.Reset();
	CachedCollisionData.Reset();
	URuntimeMeshProviderPassthrough::MarkCollisionDirty();
}




void URuntimeMeshProviderMemoryCache::BeginDestroy()
{
	ClearCache();
	Super::BeginDestroy();
}




void URuntimeMeshProviderMemoryCache::ClearCacheLOD(int32 LODIndex)
{
	FScopeLock Lock(&MeshSyncRoot);
	if (LODIndex == INDEX_NONE)
	{
		CachedMeshData.Empty();
	}
	else
	{
		CachedMeshData.Remove(LODIndex);
	}
}

void URuntimeMeshProviderMemoryCache::ClearCacheSection(int32 LODIndex, int32 SectionId)
{
	FScopeLock Lock(&MeshSyncRoot);
	TMap<int32, TOptional<FRuntimeMeshRenderableMeshData>>* FoundLOD = CachedMeshData.Find(LODIndex);

	if (FoundLOD)
	{
		FoundLOD->Remove(SectionId);
	}
}