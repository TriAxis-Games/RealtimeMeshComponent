// Copyright 2016-2019 Chris Conway (Koderz). All Rights Reserved.


#include "RuntimeMeshProviderStatic.h"


URuntimeMeshProviderStatic::URuntimeMeshProviderStatic()
	: LODForMeshCollision(0)
	, CombinedBounds(ForceInit)
{

}

void URuntimeMeshProviderStatic::ClearSection(int32 LODIndex, int32 SectionId)
{
	StoredMeshData.Remove(GenerateKeyFromLODAndSection(LODIndex, SectionId));
	UpdateBounds();
	MarkSectionDirty(LODIndex, SectionId);
}

void URuntimeMeshProviderStatic::SetCollisionSettings(const FRuntimeMeshCollisionSettings& NewCollisionSettings)
{
	CollisionSettings = NewCollisionSettings;
	MarkCollisionDirty();
}

void URuntimeMeshProviderStatic::SetCollisionMesh(const FRuntimeMeshCollisionData& NewCollisionMesh)
{
	CollisionMesh = NewCollisionMesh;
	MarkCollisionDirty();
}

void URuntimeMeshProviderStatic::SetRenderableLODForCollision(int32 LODIndex)
{
	LODForMeshCollision = LODIndex;
	MarkCollisionDirty();
}

void URuntimeMeshProviderStatic::SetRenderableSectionAffectsCollision(int32 SectionId, bool bCollisionEnabled)
{
	if (bCollisionEnabled && !SectionsForMeshCollision.Contains(SectionId))
	{
		SectionsForMeshCollision.Add(SectionId);
		MarkCollisionDirty();
	}
	else if (!bCollisionEnabled && SectionsForMeshCollision.Contains(SectionId))
	{
		SectionsForMeshCollision.Remove(SectionId);
		MarkCollisionDirty();
	}
}

bool URuntimeMeshProviderStatic::GetSectionMeshForLOD(int32 LODIndex, int32 SectionId, FRuntimeMeshRenderableMeshData& MeshData)
{
	TTuple<FRuntimeMeshRenderableMeshData, FBoxSphereBounds>* CachedData = StoredMeshData.Find(GenerateKeyFromLODAndSection(LODIndex, SectionId));
	if (CachedData)
	{
		MeshData = CachedData->Get<0>();
		return true;
	}
	return false;
}

FRuntimeMeshCollisionSettings URuntimeMeshProviderStatic::GetCollisionSettings()
{
	return CollisionSettings;
}

bool URuntimeMeshProviderStatic::HasCollisionMesh()
{
	if (CollisionMesh.IsSet())
	{
		return true;
	}

	for (int32 Section : SectionsForMeshCollision)
	{
		TTuple<FRuntimeMeshRenderableMeshData, FBoxSphereBounds>* CachedData = 
			StoredMeshData.Find(GenerateKeyFromLODAndSection(LODForMeshCollision, Section));
		if (CachedData)
		{
			if (CachedData->Get<0>().HasValidMeshData())
			{
				return true;
			}
		}
	}

	return false;
}

bool URuntimeMeshProviderStatic::GetCollisionMesh(FRuntimeMeshCollisionData& CollisionData)
{
	bool bHadMeshData = false;
	if (CollisionMesh.IsSet())
	{
		CollisionData = CollisionMesh.GetValue();
		bHadMeshData = true;
	}

	for (int32 Section : SectionsForMeshCollision)
	{
		TTuple<FRuntimeMeshRenderableMeshData, FBoxSphereBounds>* CachedData =
			StoredMeshData.Find(GenerateKeyFromLODAndSection(LODForMeshCollision, Section));
		if (CachedData)
		{
			FRuntimeMeshRenderableMeshData& SectionData = CachedData->Get<0>();
			if (SectionData.HasValidMeshData())
			{
				// Append the mesh data
				int32 StartIndex = CollisionData.Vertices.Num();

				// Copy vertices
				for (int32 Index = 0; Index < SectionData.Positions.Num(); Index++)
				{
					CollisionData.Vertices.Add(SectionData.Positions.GetPosition(Index));
				}

				// Copy indices offsetting for vertex move
				for (int32 Index = 0; Index < SectionData.Triangles.Num(); Index += 3)
				{
					CollisionData.Triangles.Add(
						SectionData.Triangles.GetVertexIndex(Index + 0) + StartIndex,
						SectionData.Triangles.GetVertexIndex(Index + 1) + StartIndex,
						SectionData.Triangles.GetVertexIndex(Index + 2) + StartIndex);
				}


				// TODO: Append the UV's First you must fill the UV's if there's existing collision data

				bHadMeshData = true;
			}
		}
	}

	return bHadMeshData;
}

void URuntimeMeshProviderStatic::UpdateBounds()
{
	FBoxSphereBounds NewBounds(FVector::ZeroVector, FVector::ZeroVector, 0);
	bool bFirst = true;
	for (const auto& Section : StoredMeshData)
	{
		if (bFirst)
		{
			NewBounds = Section.Value.Get<1>();
			bFirst = false;
		}
		else
		{
			NewBounds = NewBounds + Section.Value.Get<1>();
		}
	}
	CombinedBounds = NewBounds;
}

FBoxSphereBounds URuntimeMeshProviderStatic::GetBoundsFromMeshData(const FRuntimeMeshRenderableMeshData& MeshData)
{
	return FBoxSphereBounds(MeshData.Positions.GetBounds());
}

