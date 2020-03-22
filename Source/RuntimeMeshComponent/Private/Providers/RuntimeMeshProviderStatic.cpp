// Copyright 2016-2019 Chris Conway (Koderz). All Rights Reserved.

#include "RuntimeMeshProviderStatic.h"
#include "RuntimeMeshComponentPlugin.h"


URuntimeMeshProviderStatic::URuntimeMeshProviderStatic()
	: LODForMeshCollision(0)
	, CombinedBounds(ForceInit)
{
	UE_LOG(RuntimeMeshLog, Verbose, TEXT("StaticProvider(%d): Created"), FPlatformTLS::GetCurrentThreadId());

}

void URuntimeMeshProviderStatic::Initialize_Implementation()
{
	UE_LOG(RuntimeMeshLog, Verbose, TEXT("StaticProvider(%d): Initialize called"), FPlatformTLS::GetCurrentThreadId());

	// Setup existing LODs
	if (LODConfigurations.Num() > 0)
	{
		URuntimeMeshProvider::ConfigureLODs(LODConfigurations);
	}
	else
	{
		// Default LOD 0
		URuntimeMeshProvider::ConfigureLODs(
			{
				FRuntimeMeshLODProperties(),
			});
	}

	// Setup existing sections
	for (const auto& LOD : SectionDataMap)
	{
		for (const auto& Section : LOD.Value)
		{
			// Create the section
			URuntimeMeshProvider::CreateSection(LOD.Key, Section.Key, Section.Value.Get<0>());
			if (Section.Value.Get<1>().HasValidMeshData())
			{
				URuntimeMeshProvider::MarkSectionDirty(LOD.Key, Section.Key);
			}
		}
	}
}








void URuntimeMeshProviderStatic::ClearSection(int32 LODIndex, int32 SectionId)
{
	TMap<int32, FSectionDataMapEntry>* LODSections = SectionDataMap.Find(LODIndex);
	if (LODSections)
	{
		FSectionDataMapEntry* Section = LODSections->Find(SectionId);
		if (Section)
		{
			Section->Get<1>().Reset();
			UpdateBounds();
			MarkSectionDirty(LODIndex, SectionId);
		}
	}
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

void URuntimeMeshProviderStatic::ConfigureLODs_Implementation(const TArray<FRuntimeMeshLODProperties>& LODSettings)
{
	check(LODSettings.Num() > 0 && LODSettings.Num() <= RUNTIMEMESH_MAXLODS);

	LODConfigurations = LODSettings;
	SectionDataMap.Empty();

	LODForMeshCollision = FMath::Min(RUNTIMEMESH_MAXLODS - 1, LODForMeshCollision);

	URuntimeMeshProvider::ConfigureLODs_Implementation(LODSettings);
}


void URuntimeMeshProviderStatic::CreateSection_Implementation(int32 LODIndex, int32 SectionId, const FRuntimeMeshSectionProperties& SectionProperties)
{
	SectionDataMap.FindOrAdd(LODIndex).FindOrAdd(SectionId) = MakeTuple(SectionProperties, FRuntimeMeshRenderableMeshData(), FBoxSphereBounds(FVector::ZeroVector, FVector::ZeroVector, 0));
	URuntimeMeshProvider::CreateSection_Implementation(LODIndex, SectionId, SectionProperties);
}



void URuntimeMeshProviderStatic::RemoveSection_Implementation(int32 LODIndex, int32 SectionId)
{
	TMap<int32, FSectionDataMapEntry>* LODSections = SectionDataMap.Find(LODIndex);
	if (LODSections)
	{
		if (LODSections->Remove(SectionId))
		{
			// Remove map for LOD if empty
			if (LODSections->Num() == 0)
			{
				SectionDataMap.Remove(LODIndex);
			}
			UpdateBounds();
		}
	}
	URuntimeMeshProvider::RemoveSection_Implementation(LODIndex, SectionId);
}

void URuntimeMeshProviderStatic::SetSectionCastsShadow_Implementation(int32 LODIndex, int32 SectionId, bool bCastsShadow)
{
	TMap<int32, FSectionDataMapEntry>* LODSections = SectionDataMap.Find(LODIndex);
	if (LODSections)
	{
		FSectionDataMapEntry* Section = LODSections->Find(SectionId);
		if (Section)
		{
			Section->Get<0>().bCastsShadow = bCastsShadow;
			URuntimeMeshProvider::SetSectionCastsShadow_Implementation(LODIndex, SectionId, bCastsShadow);
		}
	}
}

void URuntimeMeshProviderStatic::SetSectionVisibility_Implementation(int32 LODIndex, int32 SectionId, bool bIsVisible)
{
	TMap<int32, FSectionDataMapEntry>* LODSections = SectionDataMap.Find(LODIndex);
	if (LODSections)
	{
		FSectionDataMapEntry* Section = LODSections->Find(SectionId);
		if (Section)
		{
			Section->Get<0>().bIsVisible = bIsVisible;
			URuntimeMeshProvider::SetSectionVisibility_Implementation(LODIndex, SectionId, bIsVisible);
		}
	}
}





bool URuntimeMeshProviderStatic::GetSectionMeshForLOD_Implementation(int32 LODIndex, int32 SectionId, FRuntimeMeshRenderableMeshData& MeshData)
{
	TMap<int32, FSectionDataMapEntry>* LODSections = SectionDataMap.Find(LODIndex);
	if (LODSections)
	{
		FSectionDataMapEntry* Section = LODSections->Find(SectionId);
		if (Section)
		{
			MeshData = Section->Get<1>();
			return true;
		}
	}
	return false;
}

FRuntimeMeshCollisionSettings URuntimeMeshProviderStatic::GetCollisionSettings_Implementation()
{
	return CollisionSettings;
}

bool URuntimeMeshProviderStatic::HasCollisionMesh_Implementation()
{
	if (CollisionMesh.IsSet())
	{
		return true;
	}

	TMap<int32, FSectionDataMapEntry>* LODSections = SectionDataMap.Find(LODForMeshCollision);
	if (LODSections)
	{
		for (int32 SectionId : SectionsForMeshCollision)
		{
			FSectionDataMapEntry* Section = LODSections->Find(SectionId);
			if (Section && Section->Get<1>().HasValidMeshData())
			{
				return true;
			}
		}
	}

	return false;
}

bool URuntimeMeshProviderStatic::GetCollisionMesh_Implementation(FRuntimeMeshCollisionData& CollisionData)
{
	bool bHadMeshData = false;
	if (CollisionMesh.IsSet())
	{
		CollisionData = CollisionMesh.GetValue();
		bHadMeshData = true;
	}

	TMap<int32, FSectionDataMapEntry>* LODSections = SectionDataMap.Find(LODForMeshCollision);
	if (LODSections)
	{
		for (int32 SectionId : SectionsForMeshCollision)
		{
			FSectionDataMapEntry* Section = LODSections->Find(SectionId);
			if (Section)
			{
				FRuntimeMeshRenderableMeshData& SectionData = Section->Get<1>();
				if (SectionData.HasValidMeshData())
				{
					// Append the mesh data
					int32 StartIndex = CollisionData.Vertices.Num();
					int32 StartTriangle = CollisionData.Triangles.Num();

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


					// Add the collision section
					CollisionData.CollisionSources.Emplace(StartTriangle, CollisionData.Triangles.Num() - 1, this, SectionId, ERuntimeMeshCollisionFaceSourceType::Renderable);

					// TODO: Append the UV's First you must fill the UV's if there's existing collision data

					bHadMeshData = true;
				}
			}
		}
	}

	return bHadMeshData;
}

void URuntimeMeshProviderStatic::UpdateBounds()
{
	FBoxSphereBounds NewBounds(FVector::ZeroVector, FVector::ZeroVector, 0);
	bool bFirst = true;

	for (const auto& LOD : SectionDataMap)
	{
		for (const auto& Section : LOD.Value)
		{
			if (bFirst)
			{
				NewBounds = Section.Value.Get<2>();
				bFirst = false;
			}
			else
			{
				NewBounds = NewBounds + Section.Value.Get<2>();
			}
		}
	}
	CombinedBounds = NewBounds;
}

FBoxSphereBounds URuntimeMeshProviderStatic::GetBoundsFromMeshData(const FRuntimeMeshRenderableMeshData& MeshData)
{
	return FBoxSphereBounds(MeshData.Positions.GetBounds());
}



void URuntimeMeshProviderStatic::UpdateSectionInternal(int32 LODIndex, int32 SectionId, FRuntimeMeshRenderableMeshData&& SectionData, FBoxSphereBounds KnownBounds)
{
	// This is just to alert the user of invalid mesh data
	SectionData.HasValidMeshData(true);

	TMap<int32, FSectionDataMapEntry>* LODSections = SectionDataMap.Find(LODIndex);
	if (LODSections)
	{
		FSectionDataMapEntry* Section = LODSections->Find(SectionId);
		if (Section)
		{
			(*Section) = MakeTuple(Section->Get<0>(), SectionData, KnownBounds);

			UpdateBounds();
			MarkSectionDirty(LODIndex, SectionId);
		}
	}
}

void URuntimeMeshProviderStatic::Serialize(FArchive& Ar)
{
	UE_LOG(RuntimeMeshLog, Verbose, TEXT("StaticProvider(%d): Serialize called"), FPlatformTLS::GetCurrentThreadId());
	Ar.UsingCustomVersion(FRuntimeMeshVersion::GUID);

	Super::Serialize(Ar);


	if (Ar.CustomVer(FRuntimeMeshVersion::GUID) >= FRuntimeMeshVersion::StaticProviderSupportsSerialization)
	{
		// Serialize mesh data
		Ar << LODConfigurations;
		Ar << SectionDataMap;
		//Ar << NumMaterialSlots;

		// Serialize collision data
		Ar << LODForMeshCollision;
		Ar << SectionsForMeshCollision;

		Ar << CollisionSettings;
		Ar << CollisionMesh;

		UpdateBounds();
	}
}