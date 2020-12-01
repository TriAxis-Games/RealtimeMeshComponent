// Copyright 2016-2020 TriAxis Games L.L.C. All Rights Reserved.


#include "Providers/RuntimeMeshProviderCollision.h"


URuntimeMeshProviderCollision::URuntimeMeshProviderCollision()
	: LODForMeshCollision(0)
{

}

void URuntimeMeshProviderCollision::SetCollisionSettings(const FRuntimeMeshCollisionSettings& NewCollisionSettings)
{
	{
		FScopeLock Lock(&SyncRoot);
		CollisionSettings = NewCollisionSettings;
	}
	MarkCollisionDirty();
}

void URuntimeMeshProviderCollision::SetCollisionMesh(const FRuntimeMeshCollisionData& NewCollisionMesh)
{
	{
		FScopeLock Lock(&SyncRoot);
		CollisionMesh = NewCollisionMesh;
	}
	MarkCollisionDirty();
}

void URuntimeMeshProviderCollision::SetRenderableLODForCollision(int32 LODIndex)
{
	bool bMarkCollisionDirty = false;
	{
		FScopeLock Lock(&SyncRoot);
		if (LODForMeshCollision != LODIndex)
		{
			LODForMeshCollision = LODIndex;
			RenderableCollisionData.Empty();

			bMarkCollisionDirty = true;
		}
	}

	if (bMarkCollisionDirty)
	{
		MarkCollisionDirty();
	}
}

void URuntimeMeshProviderCollision::SetRenderableSectionAffectsCollision(int32 SectionId, bool bCollisionEnabled)
{
	bool bShouldMarkCollisionDirty = false;
	{
		FScopeLock Lock(&SyncRoot);
		if (bCollisionEnabled && !SectionsAffectingCollision.Contains(SectionId))
		{
			SectionsAffectingCollision.Add(SectionId);
			bShouldMarkCollisionDirty = true;
		}
		else if (!bCollisionEnabled && SectionsAffectingCollision.Contains(SectionId))
		{
			SectionsAffectingCollision.Remove(SectionId);
			RenderableCollisionData.Remove(SectionId);
			bShouldMarkCollisionDirty = true;
		}
	}

	if (bShouldMarkCollisionDirty)
	{
		MarkCollisionDirty();
	}
}




bool URuntimeMeshProviderCollision::GetSectionMeshForLOD(int32 LODIndex, int32 SectionId, FRuntimeMeshRenderableMeshData& MeshData)
{
	bool bResult = Super::GetSectionMeshForLOD(LODIndex, SectionId, MeshData);

	FScopeLock Lock(&SyncRoot);
	if (bResult && LODIndex == LODForMeshCollision && SectionsAffectingCollision.Contains(SectionId))
	{
		FRuntimeMeshRenderableCollisionData& SectionCacheData = RenderableCollisionData.FindOrAdd(SectionId);
		SectionCacheData = FRuntimeMeshRenderableCollisionData(MeshData);
		MarkCollisionDirty();
	}

	return bResult;
}

bool URuntimeMeshProviderCollision::GetAllSectionsMeshForLOD(int32 LODIndex, TMap<int32, FRuntimeMeshSectionData>& MeshDatas)
{
	bool bResult = Super::GetAllSectionsMeshForLOD(LODIndex, MeshDatas);

	FScopeLock Lock(&SyncRoot);
	if (bResult && LODIndex == LODForMeshCollision)
	{
		RenderableCollisionData.Empty();
		for (const auto& Entry : MeshDatas)
		{
			if (SectionsAffectingCollision.Contains(Entry.Key))
			{
				FRuntimeMeshRenderableCollisionData& SectionCacheData = RenderableCollisionData.FindOrAdd(Entry.Key);
				SectionCacheData = FRuntimeMeshRenderableCollisionData(Entry.Value.MeshData);
			}
		}

		MarkCollisionDirty();
	}

	return bResult;
}

FRuntimeMeshCollisionSettings URuntimeMeshProviderCollision::GetCollisionSettings()
{
	FScopeLock Lock(&SyncRoot);
	return CollisionSettings;
}

bool URuntimeMeshProviderCollision::HasCollisionMesh()
{
	FScopeLock Lock(&SyncRoot);
	return (CollisionMesh.Vertices.Num() > 0 && CollisionMesh.Triangles.Num() > 0) ||
		RenderableCollisionData.Num() > 0;
}

bool URuntimeMeshProviderCollision::GetCollisionMesh(FRuntimeMeshCollisionData& CollisionData)
{
	FScopeLock Lock(&SyncRoot);

	//If the given collision mesh is valid, use it
	if (CollisionMesh.Vertices.Num() > 0 && CollisionMesh.Triangles.Num() > 0)
	{
		CollisionData = CollisionMesh;
		return true;
	}


	for (const auto& CachedSectionEntry : RenderableCollisionData)
	{
		int32 SectionId = CachedSectionEntry.Key;
		const FRuntimeMeshRenderableCollisionData& CachedSection = CachedSectionEntry.Value;

		int32 FirstVertex = CollisionData.Vertices.Num();


		// Copy the vertices
		int32 NumVertices = CachedSection.Vertices.Num();
		CollisionData.Vertices.SetNum(FirstVertex + NumVertices);
		for (int32 Index = 0; Index < NumVertices; Index++)
		{
			CollisionData.Vertices.SetPosition(FirstVertex + Index, CachedSection.Vertices.GetPosition(Index));
		}

		// Copy tex coords
		int32 MaxTexCoordChannels = FMath::Max(CollisionData.TexCoords.NumChannels(), CachedSection.TexCoords.NumChannels());
		CollisionData.TexCoords.SetNum(MaxTexCoordChannels, FirstVertex + NumVertices);
		for (int32 Index = 0; Index < NumVertices; Index++)
		{
			for (int32 ChannelId = 0; ChannelId < MaxTexCoordChannels; ChannelId++)
			{
				if (ChannelId < CachedSection.TexCoords.NumChannels() && CachedSection.TexCoords.NumTexCoords(ChannelId) > Index)
				{
					FVector2D TexCoord = CachedSection.TexCoords.GetTexCoord(ChannelId, Index);
					CollisionData.TexCoords.SetTexCoord(ChannelId, Index, TexCoord);
				}
				else
				{
					CollisionData.TexCoords.SetTexCoord(ChannelId, Index, FVector2D::ZeroVector);
				}
			}
		}


		// Copy triangles and fill in material indices
		int32 StartTriangle = CollisionData.Triangles.Num();
		int32 NumTriangles = CachedSection.Triangles.Num();
		CollisionData.Triangles.SetNum(StartTriangle + NumTriangles);
		CollisionData.MaterialIndices.SetNum(StartTriangle + NumTriangles);
		for (int32 Index = 0; Index < NumTriangles; Index++)
		{
			int32 IdA, IdB, IdC;
			CachedSection.Triangles.GetTriangleIndices(Index, IdA, IdB, IdC);

			CollisionData.Triangles.SetTriangleIndices(StartTriangle + Index, IdA + FirstVertex, IdB + FirstVertex, IdC + FirstVertex);
			CollisionData.MaterialIndices.SetMaterialIndex(StartTriangle + Index, SectionId);
		}


		CollisionData.CollisionSources.Emplace(StartTriangle, StartTriangle + NumTriangles - 1, this, SectionId, ERuntimeMeshCollisionFaceSourceType::Renderable);
	}
	return true;
}

bool URuntimeMeshProviderCollision::IsThreadSafe()
{
	return Super::IsThreadSafe();
}




void URuntimeMeshProviderCollision::ConfigureLODs(const TArray<FRuntimeMeshLODProperties>& InLODs)
{
	ClearCachedData();

	Super::ConfigureLODs(InLODs);
}

void URuntimeMeshProviderCollision::CreateSection(int32 LODIndex, int32 SectionId, const FRuntimeMeshSectionProperties& SectionProperties)
{
	ClearSectionData(LODIndex, SectionId);

	Super::CreateSection(LODIndex, SectionId, SectionProperties);
}

void URuntimeMeshProviderCollision::ClearSection(int32 LODIndex, int32 SectionId)
{
	ClearSectionData(LODIndex, SectionId);

	Super::ClearSection(LODIndex, SectionId);
}

void URuntimeMeshProviderCollision::RemoveSection(int32 LODIndex, int32 SectionId)
{
	ClearSectionData(LODIndex, SectionId);

	URuntimeMeshProvider::RemoveSection(LODIndex, SectionId);
}

void URuntimeMeshProviderCollision::ClearCachedData()
{
	FScopeLock Lock(&SyncRoot);
	RenderableCollisionData.Empty();
}

void URuntimeMeshProviderCollision::ClearSectionData(int32 LODIndex, int32 SectionId)
{
	FScopeLock Lock(&SyncRoot);
	if (LODIndex == LODForMeshCollision)
	{
		RenderableCollisionData.Remove(SectionId);
	}
}

