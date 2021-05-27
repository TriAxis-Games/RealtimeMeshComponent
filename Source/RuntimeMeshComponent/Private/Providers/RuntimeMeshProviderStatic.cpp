// Copyright 2016-2020 TriAxis Games L.L.C. All Rights Reserved.

#include "Providers/RuntimeMeshProviderStatic.h"
#include "RuntimeMeshComponentPlugin.h"
#include "RuntimeMeshModifier.h"


#define RMC_LOG_VERBOSE(Format, ...) \
	UE_LOG(RuntimeMeshLog, Verbose, TEXT("[RMPS:%d Thread:%d]: " Format), GetUniqueID(), FPlatformTLS::GetCurrentThreadId(), ##__VA_ARGS__);

URuntimeMeshProviderStatic::URuntimeMeshProviderStatic()
	: StoreEditorGeneratedDataForGame(true)
	, LODForMeshCollision(0)
	, CombinedBounds(ForceInit)
{
	RMC_LOG_VERBOSE("Created");

}

void URuntimeMeshProviderStatic::RegisterModifier(URuntimeMeshModifier* Modifier)
{
	FWriteScopeLock Lock(ModifierRWLock);
	CurrentMeshModifiers.Add(Modifier);
}

void URuntimeMeshProviderStatic::UnRegisterModifier(URuntimeMeshModifier* Modifier)
{
	FWriteScopeLock Lock(ModifierRWLock);
	CurrentMeshModifiers.Remove(Modifier);
}

void URuntimeMeshProviderStatic::CreateSectionFromComponents(int32 LODIndex, int32 SectionIndex, int32 MaterialSlot, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals, 
	const TArray<FVector2D>& UV0, const TArray<FVector2D>& UV1, const TArray<FVector2D>& UV2, const TArray<FVector2D>& UV3, const TArray<FLinearColor>& VertexColors, 
	const TArray<FRuntimeMeshTangent>& Tangents, ERuntimeMeshUpdateFrequency UpdateFrequency, bool bCreateCollision)
{
	FRuntimeMeshSectionProperties Properties;
	Properties.MaterialSlot = MaterialSlot;
	Properties.UpdateFrequency = UpdateFrequency;

	FRuntimeMeshRenderableMeshData SectionData = FillMeshData(Properties, Vertices, Normals, Tangents, VertexColors, UV0, UV1, UV2, UV3, Triangles);

	CreateSection(LODIndex, SectionIndex, Properties, SectionData);

	UpdateSectionAffectsCollision(LODIndex, SectionIndex, bCreateCollision, true);
}

void URuntimeMeshProviderStatic::CreateSectionFromComponents(int32 LODIndex, int32 SectionIndex, int32 MaterialSlot, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals, 
	const TArray<FVector2D>& UV0, const TArray<FVector2D>& UV1, const TArray<FVector2D>& UV2, const TArray<FVector2D>& UV3, const TArray<FColor>& VertexColors, 
	const TArray<FRuntimeMeshTangent>& Tangents, ERuntimeMeshUpdateFrequency UpdateFrequency, bool bCreateCollision)
{
	FRuntimeMeshSectionProperties Properties;
	Properties.MaterialSlot = MaterialSlot;
	Properties.UpdateFrequency = UpdateFrequency;

	FRuntimeMeshRenderableMeshData SectionData = FillMeshData(Properties, Vertices, Normals, Tangents, VertexColors, UV0, UV1, UV2, UV3, Triangles);

	CreateSection(LODIndex, SectionIndex, Properties, SectionData);

	UpdateSectionAffectsCollision(LODIndex, SectionIndex, bCreateCollision, true);
}	

void  URuntimeMeshProviderStatic::CreateSectionFromComponents(int32 LODIndex, int32 SectionIndex, int32 MaterialSlot, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals,
	const TArray<FVector2D>& UV0, const TArray<FLinearColor>& VertexColors, const TArray<FRuntimeMeshTangent>& Tangents, ERuntimeMeshUpdateFrequency UpdateFrequency, bool bCreateCollision)
{
	FRuntimeMeshSectionProperties Properties;
	Properties.MaterialSlot = MaterialSlot;
	Properties.UpdateFrequency = UpdateFrequency;

	FRuntimeMeshRenderableMeshData SectionData = FillMeshData(Properties, Vertices, Normals, Tangents, VertexColors, UV0, EmptyUVs, EmptyUVs, EmptyUVs, Triangles);

	CreateSection(LODIndex, SectionIndex, Properties, SectionData);

	UpdateSectionAffectsCollision(LODIndex, SectionIndex, bCreateCollision, true);
}

void URuntimeMeshProviderStatic::CreateSectionFromComponents(int32 LODIndex, int32 SectionIndex, int32 MaterialSlot, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals, 
	const TArray<FVector2D>& UV0, const TArray<FColor>& VertexColors, const TArray<FRuntimeMeshTangent>& Tangents, ERuntimeMeshUpdateFrequency UpdateFrequency, bool bCreateCollision)
{
	FRuntimeMeshSectionProperties Properties;
	Properties.MaterialSlot = MaterialSlot;
	Properties.UpdateFrequency = UpdateFrequency;

	FRuntimeMeshRenderableMeshData SectionData = FillMeshData(Properties, Vertices, Normals, Tangents, VertexColors, UV0, EmptyUVs, EmptyUVs, EmptyUVs, Triangles);

	CreateSection(LODIndex, SectionIndex, Properties, SectionData);

	UpdateSectionAffectsCollision(LODIndex, SectionIndex, bCreateCollision, true);
}

void URuntimeMeshProviderStatic::UpdateSectionFromComponents(int32 LODIndex, int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals, 
	const TArray<FVector2D>& UV0, const TArray<FVector2D>& UV1, const TArray<FVector2D>& UV2, const TArray<FVector2D>& UV3, const TArray<FLinearColor>& VertexColors, const TArray<FRuntimeMeshTangent>& Tangents)
{
	FRuntimeMeshSectionProperties Properties;
	FRuntimeMeshRenderableMeshData SectionData = FillMeshData(Properties, Vertices, Normals, Tangents, VertexColors, UV0, UV1, UV2, UV3, Triangles);

	UpdateSection(LODIndex, SectionIndex, SectionData);
}

void URuntimeMeshProviderStatic::UpdateSectionFromComponents(int32 LODIndex, int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals, 
	const TArray<FVector2D>& UV0, const TArray<FVector2D>& UV1, const TArray<FVector2D>& UV2, const TArray<FVector2D>& UV3, const TArray<FColor>& VertexColors, const TArray<FRuntimeMeshTangent>& Tangents)
{
	FRuntimeMeshSectionProperties Properties;
	FRuntimeMeshRenderableMeshData SectionData = FillMeshData(Properties, Vertices, Normals, Tangents, VertexColors, UV0, UV1, UV2, UV3, Triangles);

	UpdateSection(LODIndex, SectionIndex, SectionData);
}

void URuntimeMeshProviderStatic::UpdateSectionFromComponents(int32 LODIndex, int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals,
	const TArray<FVector2D>& UV0, const TArray<FLinearColor>& VertexColors, const TArray<FRuntimeMeshTangent>& Tangents)
{
	FRuntimeMeshSectionProperties Properties;
	FRuntimeMeshRenderableMeshData SectionData = FillMeshData(Properties, Vertices, Normals, Tangents, VertexColors, UV0, EmptyUVs, EmptyUVs, EmptyUVs, Triangles);

	UpdateSection(LODIndex, SectionIndex, SectionData);
}

void URuntimeMeshProviderStatic::UpdateSectionFromComponents(int32 LODIndex, int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals,
	const TArray<FVector2D>& UV0, const TArray<FColor>& VertexColors, const TArray<FRuntimeMeshTangent>& Tangents)
{
	int32 NumTexChannels = 1;

	FRuntimeMeshSectionProperties Properties;
	FRuntimeMeshRenderableMeshData SectionData = FillMeshData(Properties, Vertices, Normals, Tangents, VertexColors, UV0, EmptyUVs, EmptyUVs, EmptyUVs, Triangles);

	UpdateSection(LODIndex, SectionIndex, SectionData);
}





FRuntimeMeshCollisionSettings URuntimeMeshProviderStatic::GetCollisionSettingsStatic() const
{
	FScopeLock Lock(&CollisionSyncRoot);
	return CollisionSettings;
}

void URuntimeMeshProviderStatic::SetCollisionSettings(const FRuntimeMeshCollisionSettings& NewCollisionSettings)
{
	{
		FScopeLock Lock(&CollisionSyncRoot);
		CollisionSettings = NewCollisionSettings;
	}
	MarkCollisionDirty();
}

FRuntimeMeshCollisionData URuntimeMeshProviderStatic::GetCollisionMeshStatic() const
{
	FScopeLock Lock(&CollisionSyncRoot);
	if (CollisionMesh.IsSet())
	{
		return CollisionMesh.GetValue();
	}
	return FRuntimeMeshCollisionData();
}

void URuntimeMeshProviderStatic::SetCollisionMesh(const FRuntimeMeshCollisionData& NewCollisionMesh)
{
	{
		FScopeLock Lock(&CollisionSyncRoot);
		CollisionMesh = NewCollisionMesh;
	}
	MarkCollisionDirty();
}

int32 URuntimeMeshProviderStatic::GetLODForMeshCollision() const
{
	FScopeLock Lock(&CollisionSyncRoot);
	return LODForMeshCollision;
}

void URuntimeMeshProviderStatic::SetRenderableLODForCollision(int32 LODIndex)
{
	{
		FScopeLock Lock(&CollisionSyncRoot);
		LODForMeshCollision = LODIndex;
	}
	MarkCollisionDirty();
}

TSet<int32> URuntimeMeshProviderStatic::GetSectionsForMeshCollision() const
{
	FScopeLock Lock(&CollisionSyncRoot);
	return SectionsForMeshCollision;
}

void URuntimeMeshProviderStatic::SetRenderableSectionAffectsCollision(int32 SectionId, bool bCollisionEnabled, bool bForceUpdate)
{
	UpdateSectionAffectsCollision(LODForMeshCollision, SectionId, bCollisionEnabled, bForceUpdate);
}

TArray<int32> URuntimeMeshProviderStatic::GetSectionIds(int32 LODIndex) const
{
	FScopeLock Lock(&MeshSyncRoot);
	TArray<int32> SectionIds;

	const auto* FoundLOD = SectionDataMap.Find(LODIndex);
	if (FoundLOD)
	{
		FoundLOD->GetKeys(SectionIds);
	}

	return SectionIds;
}

int32 URuntimeMeshProviderStatic::GetLastSectionId(int32 LODIndex) const
{
	TArray<int32> SectionIds;
	{
		FScopeLock Lock(&MeshSyncRoot);

		const auto* FoundLOD = SectionDataMap.Find(LODIndex);
		if (FoundLOD)
		{
			FoundLOD->GetKeys(SectionIds);
		}
	}

	int32 MaxIndex = INDEX_NONE;

	for (int32 Index = 0; Index < SectionIds.Num(); Index++)
	{
		MaxIndex = FMath::Max(MaxIndex, SectionIds[Index]);
	}

	return MaxIndex;
}

bool URuntimeMeshProviderStatic::DoesSectionHaveValidMeshData(int32 LODIndex, int32 SectionId) const
{
	FScopeLock Lock(&MeshSyncRoot);

	check(SectionDataMap.Contains(LODIndex));
	check(SectionDataMap[LODIndex].Contains(SectionId));

	return SectionDataMap.FindChecked(LODIndex).FindChecked(SectionId).Get<1>().HasValidMeshData();
}

void URuntimeMeshProviderStatic::RemoveAllSectionsForLOD(int32 LODIndex)
{
	{
		FScopeLock Lock(&MeshSyncRoot);
		SectionDataMap.Remove(LODIndex);
	}
	MarkLODDirty(LODIndex);
}

FBoxSphereBounds URuntimeMeshProviderStatic::GetSectionBounds(int32 LODIndex, int32 SectionId) const
{
	FScopeLock Lock(&MeshSyncRoot);

	check(SectionDataMap.Contains(LODIndex));
	check(SectionDataMap[LODIndex].Contains(SectionId));

	return SectionDataMap.FindChecked(LODIndex).FindChecked(SectionId).Get<2>();
}

FRuntimeMeshSectionProperties URuntimeMeshProviderStatic::GetSectionProperties(int32 LODIndex, int32 SectionId) const
{
	FScopeLock Lock(&MeshSyncRoot);

	check(SectionDataMap.Contains(LODIndex));
	check(SectionDataMap[LODIndex].Contains(SectionId));

	return SectionDataMap.FindChecked(LODIndex).FindChecked(SectionId).Get<0>();
}

FRuntimeMeshRenderableMeshData URuntimeMeshProviderStatic::GetSectionRenderData(int32 LODIndex, int32 SectionId) const
{
	FScopeLock Lock(&MeshSyncRoot);

	check(SectionDataMap.Contains(LODIndex));
	check(SectionDataMap[LODIndex].Contains(SectionId));

	return SectionDataMap.FindChecked(LODIndex).FindChecked(SectionId).Get<1>();
}

FRuntimeMeshRenderableMeshData URuntimeMeshProviderStatic::GetSectionRenderDataAndClear(int32 LODIndex, int32 SectionId)
{
	FRuntimeMeshRenderableMeshData MeshData;
	{
		FScopeLock Lock(&MeshSyncRoot);

		check(SectionDataMap.Contains(LODIndex));
		check(SectionDataMap[LODIndex].Contains(SectionId));

		MeshData = MoveTemp(SectionDataMap.FindChecked(LODIndex).FindChecked(SectionId).Get<1>());
	}
	ClearSection(LODIndex, SectionId);

	return MeshData;
}

void URuntimeMeshProviderStatic::SetShouldSerializeMeshData(bool bIsSerialized)
{
	StoreEditorGeneratedDataForGame = bIsSerialized;
}



void URuntimeMeshProviderStatic::Initialize()
{
	RMC_LOG_VERBOSE("Initializing...");

	// Setup loaded materials
	for (int32 Index = 0; Index < LoadedMaterialSlots.Num(); Index++)
	{
		FName SlotName = LoadedMaterialSlots[Index].SlotName;
		UMaterialInterface* Material = LoadedMaterialSlots[Index].Material;

		SetupMaterialSlot(Index, SlotName, Material);
	}
	LoadedMaterialSlots.Empty();

	// Setup existing LODs
 	if (LODConfigurations.Num() > 0)
	{
		//TMap<int32, TMap<int32, FSectionDataMapEntry>> SectionDataMapTemp = MoveTemp(SectionDataMap);

		URuntimeMeshProvider::ConfigureLODs(LODConfigurations);

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

		UpdateBounds();
		MarkCollisionDirty();
	}
	else
	{
		// Default LOD 0
		ConfigureLODs(
			{
				FRuntimeMeshLODProperties(),
			});
	}
}

bool URuntimeMeshProviderStatic::GetSectionMeshForLOD(int32 LODIndex, int32 SectionId, FRuntimeMeshRenderableMeshData& MeshData)
{
	FScopeLock Lock(&MeshSyncRoot);
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

FRuntimeMeshCollisionSettings URuntimeMeshProviderStatic::GetCollisionSettings()
{
	FScopeLock Lock(&CollisionSyncRoot);
	return CollisionSettings;
}

bool URuntimeMeshProviderStatic::HasCollisionMesh()
{
	int32 LODForMeshCollisionTemp = INDEX_NONE;
	TSet<int32> SectionsForMeshCollisionTemp;

	{
		FScopeLock Lock(&CollisionSyncRoot);
		if (CollisionMesh.IsSet())
		{
			return true;
		}
		LODForMeshCollisionTemp = LODForMeshCollision;
		SectionsForMeshCollisionTemp = SectionsForMeshCollision;
	}

	{
		FScopeLock Lock(&MeshSyncRoot);
		TMap<int32, FSectionDataMapEntry>* LODSections = SectionDataMap.Find(LODForMeshCollisionTemp);
		if (LODSections)
		{
			for (int32 SectionId : SectionsForMeshCollisionTemp)
			{
				FSectionDataMapEntry* Section = LODSections->Find(SectionId);
				if (Section)
				{
					if (Section->Get<1>().HasValidMeshData())
					{
						return true;
					}
				}
			}
		}
	}

	return false;
}

bool URuntimeMeshProviderStatic::GetCollisionMesh(FRuntimeMeshCollisionData& CollisionData)
{
	int32 LODForMeshCollisionTemp = INDEX_NONE;
	TSet<int32> SectionsForMeshCollisionTemp;
	bool bHadMeshData = false;

	{
		FScopeLock Lock(&CollisionSyncRoot);
		if (CollisionMesh.IsSet())
		{
			CollisionData = CollisionMesh.GetValue();
			bHadMeshData = true;
		}
		LODForMeshCollisionTemp = LODForMeshCollision;
		SectionsForMeshCollisionTemp = SectionsForMeshCollision;
	}

	{
		FScopeLock Lock(&MeshSyncRoot);
		TMap<int32, FSectionDataMapEntry>* LODSections = SectionDataMap.Find(LODForMeshCollisionTemp);
		if (LODSections)
		{
			for (int32 SectionId : SectionsForMeshCollisionTemp)
			{
				FSectionDataMapEntry* Section = LODSections->Find(SectionId);
				if (Section)
				{
					FRuntimeMeshRenderableMeshData& SectionData = Section->Get<1>();
					if (SectionData.HasValidMeshData())
					{
						int32 FirstVertex = CollisionData.Vertices.Num();
						int32 NumVertex = SectionData.Positions.Num();
						int32 NumTexCoords = SectionData.TexCoords.Num();
						int32 NumChannels = SectionData.TexCoords.NumChannels();
						CollisionData.Vertices.SetNum(FirstVertex + NumVertex, false);
						CollisionData.TexCoords.SetNum(NumChannels, FirstVertex + NumVertex, false);
						for (int32 VertIdx = 0; VertIdx < NumVertex; VertIdx++)
						{
							CollisionData.Vertices.SetPosition(FirstVertex + VertIdx, SectionData.Positions.GetPosition(VertIdx));
							if (VertIdx >= NumTexCoords)
							{
								continue;
							}
							for (int32 ChannelIdx = 0; ChannelIdx < NumChannels; ChannelIdx++)
							{
								CollisionData.TexCoords.SetTexCoord(ChannelIdx, FirstVertex + VertIdx, SectionData.TexCoords.GetTexCoord(VertIdx, ChannelIdx));
							}
						}

						int32 FirstTris = CollisionData.Triangles.Num();
						int32 NumTriangles = SectionData.Triangles.NumTriangles();
						CollisionData.Triangles.SetNum(FirstTris + NumTriangles, false);
						CollisionData.MaterialIndices.SetNum(FirstTris + NumTriangles, false);
						for (int32 TrisIdx = 0; TrisIdx < NumTriangles; TrisIdx++)
						{
							int32 Index0 = SectionData.Triangles.GetVertexIndex(TrisIdx * 3) + FirstVertex;
							int32 Index1 = SectionData.Triangles.GetVertexIndex(TrisIdx * 3 + 1) + FirstVertex;
							int32 Index2 = SectionData.Triangles.GetVertexIndex(TrisIdx * 3 + 2) + FirstVertex;


							CollisionData.Triangles.SetTriangleIndices(TrisIdx + FirstTris, Index0, Index1, Index2);
							CollisionData.MaterialIndices.SetMaterialIndex(TrisIdx + FirstTris, Section->Get<0>().MaterialSlot);
						}


						CollisionData.CollisionSources.Emplace(FirstTris, CollisionData.Triangles.Num() - 1, this, SectionId, ERuntimeMeshCollisionFaceSourceType::Renderable);
						bHadMeshData = true;
					}
				}
			}
		}
	}

	return bHadMeshData;
}


void URuntimeMeshProviderStatic::ConfigureLODs(const TArray<FRuntimeMeshLODProperties>& LODSettings)
{
	check(LODSettings.Num() > 0 && LODSettings.Num() <= RUNTIMEMESH_MAXLODS);

	{
		FScopeLock Lock(&MeshSyncRoot);
		LODConfigurations = LODSettings;
		SectionDataMap.Empty();
	}

	{
		FScopeLock Lock(&CollisionSyncRoot);
		LODForMeshCollision = FMath::Min(LODSettings.Num(), LODForMeshCollision);
	}

	Super::ConfigureLODs(LODSettings);
}

void URuntimeMeshProviderStatic::SetLODScreenSize(int32 LODIndex, float ScreenSize)
{
	check(LODConfigurations.IsValidIndex(LODIndex));

	{
		FScopeLock Lock(&MeshSyncRoot);
		LODConfigurations[LODIndex].ScreenSize = ScreenSize;
	}

	Super::SetLODScreenSize(LODIndex, ScreenSize);
}

void URuntimeMeshProviderStatic::CreateSection(int32 LODIndex, int32 SectionId, const FRuntimeMeshSectionProperties& SectionProperties)
{
	{
		FScopeLock Lock(&MeshSyncRoot);
		SectionDataMap.FindOrAdd(LODIndex).FindOrAdd(SectionId) = MakeTuple(SectionProperties, FRuntimeMeshRenderableMeshData(), FBoxSphereBounds(FVector::ZeroVector, FVector::ZeroVector, 0));
	}
	URuntimeMeshProvider::CreateSection(LODIndex, SectionId, SectionProperties);
}

void URuntimeMeshProviderStatic::SetSectionVisibility(int32 LODIndex, int32 SectionId, bool bIsVisible)
{
	{
		FScopeLock Lock(&MeshSyncRoot);
		TMap<int32, FSectionDataMapEntry>* LODSections = SectionDataMap.Find(LODIndex);
		if (LODSections)
		{
			FSectionDataMapEntry* Section = LODSections->Find(SectionId);
			if (Section)
			{
				Section->Get<0>().bIsVisible = bIsVisible;
			}
		}
	}
	URuntimeMeshProvider::SetSectionVisibility(LODIndex, SectionId, bIsVisible);
}

void URuntimeMeshProviderStatic::SetSectionCastsShadow(int32 LODIndex, int32 SectionId, bool bCastsShadow)
{
	{
		FScopeLock Lock(&MeshSyncRoot);
		TMap<int32, FSectionDataMapEntry>* LODSections = SectionDataMap.Find(LODIndex);
		if (LODSections)
		{
			FSectionDataMapEntry* Section = LODSections->Find(SectionId);
			if (Section)
			{
				Section->Get<0>().bCastsShadow = bCastsShadow;
			}
		}
	}
	URuntimeMeshProvider::SetSectionCastsShadow(LODIndex, SectionId, bCastsShadow);
}

void URuntimeMeshProviderStatic::ClearSection(int32 LODIndex, int32 SectionId)
{
	{
		FScopeLock Lock(&MeshSyncRoot);
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
		if (LODIndex == LODForMeshCollision && SectionsForMeshCollision.Contains(SectionId)) //collision needs to be cleared
		{
			UpdateSectionAffectsCollision(LODIndex, SectionId, true, true); //section should still be set as needed for collision but collision mesh needs to be updated
		}
	}
	URuntimeMeshProvider::ClearSection(LODIndex, SectionId);
}

void URuntimeMeshProviderStatic::RemoveSection(int32 LODIndex, int32 SectionId)
{
	{
		FScopeLock Lock(&MeshSyncRoot);
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
				MarkSectionDirty(LODIndex, SectionId);
			}
		}
		if (LODIndex == LODForMeshCollision && SectionsForMeshCollision.Contains(SectionId)) //collision needs to be removed
		{
			UpdateSectionAffectsCollision(LODIndex, SectionId, true, true); //section should still be set as needed for collision but collision mesh needs to be updated
		}
	}
	URuntimeMeshProvider::RemoveSection(LODIndex, SectionId);
}


void URuntimeMeshProviderStatic::Serialize(FArchive& Ar)
{
	RMC_LOG_VERBOSE("Serializing...");
	Ar.UsingCustomVersion(FRuntimeMeshVersion::GUID);

	Super::Serialize(Ar);

	if (Ar.CustomVer(FRuntimeMeshVersion::GUID) >= FRuntimeMeshVersion::StaticProviderSupportsSerializationV2)
	{
		Ar << StoreEditorGeneratedDataForGame;
	}
	else
	{
		StoreEditorGeneratedDataForGame = true;
	}

	if (Ar.CustomVer(FRuntimeMeshVersion::GUID) >= FRuntimeMeshVersion::StaticProviderSupportsSerialization)
	{
		if (StoreEditorGeneratedDataForGame)
		{
			// Serialize mesh data
			Ar << LODConfigurations;
			Ar << SectionDataMap;

			// Serialize collision data
			Ar << LODForMeshCollision;
			Ar << SectionsForMeshCollision;

			Ar << CollisionSettings;
			Ar << CollisionMesh;

			if (Ar.CustomVer(FRuntimeMeshVersion::GUID) >= FRuntimeMeshVersion::StaticProviderSupportsSerializationV2)
			{
				TArray<FRuntimeMeshMaterialSlot> MaterialSlots = GetMaterialSlots();
				Ar << MaterialSlots;

				if (Ar.IsLoading())
				{
					LoadedMaterialSlots = MaterialSlots;
				}
			}

			if (Ar.CustomVer(FRuntimeMeshVersion::GUID) >= FRuntimeMeshVersion::AddedDistanceField)
			{
				if (Ar.IsSaving())
				{
					bool bHasDistanceField = false;
					Ar << bHasDistanceField;
					if (bHasDistanceField)
					{
						FRuntimeMeshDistanceFieldData NullDistanceField;
						Ar << NullDistanceField;
					}
				}
				else if (Ar.IsLoading())
				{
					bool bHasDistanceField;
					Ar << bHasDistanceField;
					if (bHasDistanceField)
					{
						FRuntimeMeshDistanceFieldData NullDistanceField;
						Ar << NullDistanceField;
					}
				}
			}
		}
	}
}




const TArray<FVector2D> URuntimeMeshProviderStatic::EmptyUVs;

void URuntimeMeshProviderStatic::UpdateSectionAffectsCollision(int32 LODIndex, int32 SectionId, bool bAffectsCollision, bool bForceUpdate)
{
	bool bMarkCollisionDirty = false;
	{
		FScopeLock Lock(&CollisionSyncRoot);
		if (LODIndex == LODForMeshCollision)
		{
			if (bAffectsCollision && !SectionsForMeshCollision.Contains(SectionId))
			{
				SectionsForMeshCollision.Add(SectionId);
				bMarkCollisionDirty = true;
			}
			else if (!bAffectsCollision && SectionsForMeshCollision.Contains(SectionId))
			{
				SectionsForMeshCollision.Remove(SectionId);
				bMarkCollisionDirty = true;
			}
		}
	}

	if (bForceUpdate || bMarkCollisionDirty)
	{
		MarkCollisionDirty();
	}
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


	{
		FReadScopeLock Lock(ModifierRWLock);

		for (URuntimeMeshModifier* Modifier : CurrentMeshModifiers)
		{
			check(Modifier->IsValidLowLevel());

			Modifier->ApplyToMesh(SectionData);
		}
	}


	{
		FScopeLock Lock(&MeshSyncRoot);
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

	{
		FScopeLock Lock(&CollisionSyncRoot);
		if (LODForMeshCollision == LODIndex && SectionsForMeshCollision.Contains(SectionId))
		{
			MarkCollisionDirty();
		}
	}
}

void URuntimeMeshProviderStatic::BeginDestroy()
{
	{
		FWriteScopeLock Lock(ModifierRWLock);
		CurrentMeshModifiers.Empty();
	}
	Super::BeginDestroy();
}

#undef RMC_LOG_VERBOSE