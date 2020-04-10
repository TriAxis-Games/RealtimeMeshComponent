// Copyright 2016-2020 Chris Conway (Koderz). All Rights Reserved.

#include "Providers/RuntimeMeshProviderStatic.h"
#include "RuntimeMeshComponentPlugin.h"


URuntimeMeshProviderStatic::URuntimeMeshProviderStatic()
	: StoreEditorGeneratedDataForGame(true)
	, LODForMeshCollision(0)
	, CombinedBounds(ForceInit)
{
	UE_LOG(RuntimeMeshLog, Verbose, TEXT("StaticProvider(%d): Created"), FPlatformTLS::GetCurrentThreadId());

}

void URuntimeMeshProviderStatic::CreateSectionFromComponents(int32 LODIndex, int32 SectionIndex, int32 MaterialSlot, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals, 
	const TArray<FVector2D>& UV0, const TArray<FVector2D>& UV1, const TArray<FVector2D>& UV2, const TArray<FVector2D>& UV3, const TArray<FLinearColor>& VertexColors, 
	const TArray<FRuntimeMeshTangent>& Tangents, ERuntimeMeshUpdateFrequency UpdateFrequency, bool bCreateCollision)
{
	FRuntimeMeshSectionProperties Properties;
	Properties.MaterialSlot = MaterialSlot;
	Properties.UpdateFrequency = UpdateFrequency;
	Properties.bWants32BitIndices = Vertices.Num() > MAX_uint16;
	Properties.bUseHighPrecisionTexCoords = true;
	Properties.NumTexCoords =
		UV3.Num() > 0 ? 4 :
		UV2.Num() > 0 ? 3 :
		UV1.Num() > 0 ? 2 : 1;


	FRuntimeMeshRenderableMeshData SectionData(Properties);
	SectionData.Positions.Append(Vertices);
	SectionData.Tangents.Append(Normals, Tangents);
	if (SectionData.Tangents.Num() < SectionData.Positions.Num())
	{
		int32 Count = SectionData.Tangents.Num();
		SectionData.Tangents.SetNum(SectionData.Positions.Num());
		for (int32 Index = Count; Index < SectionData.Tangents.Num(); Index++)
		{
			SectionData.Tangents.SetTangents(Index, FVector(1, 0, 0), FVector(0, 1, 0), FVector(0, 0, 1));
		}
	}
	SectionData.Colors.Append(VertexColors);
	if (SectionData.Colors.Num() < SectionData.Positions.Num())
	{
		SectionData.Colors.SetNum(SectionData.Positions.Num());
	}

	int32 StartIndexTexCoords = SectionData.TexCoords.Num();
	SectionData.TexCoords.FillIn(StartIndexTexCoords, UV0, 0);
	SectionData.TexCoords.FillIn(StartIndexTexCoords, UV1, 1);
	SectionData.TexCoords.FillIn(StartIndexTexCoords, UV2, 2);
	SectionData.TexCoords.FillIn(StartIndexTexCoords, UV3, 3);

	if (SectionData.TexCoords.Num() < SectionData.Positions.Num())
	{
		SectionData.TexCoords.SetNum(SectionData.Positions.Num());
	}

	SectionData.Triangles.Append(Triangles);

	CreateSection(LODIndex, SectionIndex, Properties, SectionData);

	UpdateSectionAffectsCollision(LODIndex, SectionIndex, bCreateCollision);
}

void URuntimeMeshProviderStatic::CreateSectionFromComponents(int32 LODIndex, int32 SectionIndex, int32 MaterialSlot, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals, 
	const TArray<FVector2D>& UV0, const TArray<FVector2D>& UV1, const TArray<FVector2D>& UV2, const TArray<FVector2D>& UV3, const TArray<FColor>& VertexColors, 
	const TArray<FRuntimeMeshTangent>& Tangents, ERuntimeMeshUpdateFrequency UpdateFrequency, bool bCreateCollision)
{
	FRuntimeMeshSectionProperties Properties;
	Properties.MaterialSlot = MaterialSlot;
	Properties.UpdateFrequency = UpdateFrequency;
	Properties.bWants32BitIndices = Vertices.Num() > MAX_uint16;
	Properties.bUseHighPrecisionTexCoords = true;
	Properties.NumTexCoords =
		UV3.Num() > 0 ? 4 :
		UV2.Num() > 0 ? 3 :
		UV1.Num() > 0 ? 2 : 1;

	FRuntimeMeshRenderableMeshData SectionData(Properties);
	SectionData.Positions.Append(Vertices);
	SectionData.Tangents.Append(Normals, Tangents);
	if (SectionData.Tangents.Num() < SectionData.Positions.Num())
	{
		int32 Count = SectionData.Tangents.Num();
		SectionData.Tangents.SetNum(SectionData.Positions.Num());
		for (int32 Index = Count; Index < SectionData.Tangents.Num(); Index++)
		{
			SectionData.Tangents.SetTangents(Index, FVector(1, 0, 0), FVector(0, 1, 0), FVector(0, 0, 1));
		}
	}
	SectionData.Colors.Append(VertexColors);
	if (SectionData.Colors.Num() < SectionData.Positions.Num())
	{
		SectionData.Colors.SetNum(SectionData.Positions.Num());
	}

	int32 StartIndexTexCoords = SectionData.TexCoords.Num();
	SectionData.TexCoords.FillIn(StartIndexTexCoords, UV0, 0);
	SectionData.TexCoords.FillIn(StartIndexTexCoords, UV1, 1);
	SectionData.TexCoords.FillIn(StartIndexTexCoords, UV2, 2);
	SectionData.TexCoords.FillIn(StartIndexTexCoords, UV3, 3);

	if (SectionData.TexCoords.Num() < SectionData.Positions.Num())
	{
		SectionData.TexCoords.SetNum(SectionData.Positions.Num());
	}

	SectionData.Triangles.Append(Triangles);

	CreateSection(LODIndex, SectionIndex, Properties, SectionData);

	UpdateSectionAffectsCollision(LODIndex, SectionIndex, bCreateCollision);
}	

void  URuntimeMeshProviderStatic::CreateSectionFromComponents(int32 LODIndex, int32 SectionIndex, int32 MaterialSlot, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals,
	const TArray<FVector2D>& UV0, const TArray<FLinearColor>& VertexColors, const TArray<FRuntimeMeshTangent>& Tangents, ERuntimeMeshUpdateFrequency UpdateFrequency, bool bCreateCollision)
{
	FRuntimeMeshSectionProperties Properties;
	Properties.MaterialSlot = MaterialSlot;
	Properties.UpdateFrequency = UpdateFrequency;
	Properties.bWants32BitIndices = Vertices.Num() > MAX_uint16;
	Properties.bUseHighPrecisionTexCoords = true;
	Properties.NumTexCoords = 1;

	FRuntimeMeshRenderableMeshData SectionData(Properties);
	SectionData.Positions.Append(Vertices);
	SectionData.Tangents.Append(Normals, Tangents);
	if (SectionData.Tangents.Num() < SectionData.Positions.Num())
	{
		int32 Count = SectionData.Tangents.Num();
		SectionData.Tangents.SetNum(SectionData.Positions.Num());
		for (int32 Index = Count; Index < SectionData.Tangents.Num(); Index++)
		{
			SectionData.Tangents.SetTangents(Index, FVector(1, 0, 0), FVector(0, 1, 0), FVector(0, 0, 1));
		}
	}
	SectionData.Colors.Append(VertexColors);
	if (SectionData.Colors.Num() < SectionData.Positions.Num())
	{
		SectionData.Colors.SetNum(SectionData.Positions.Num());
	}

	int32 StartIndexTexCoords = SectionData.TexCoords.Num();
	SectionData.TexCoords.FillIn(StartIndexTexCoords, UV0, 0);

	if (SectionData.TexCoords.Num() < SectionData.Positions.Num())
	{
		SectionData.TexCoords.SetNum(SectionData.Positions.Num());
	}
	SectionData.Triangles.Append(Triangles);

	CreateSection(LODIndex, SectionIndex, Properties, SectionData);

	UpdateSectionAffectsCollision(LODIndex, SectionIndex, bCreateCollision);
}

void URuntimeMeshProviderStatic::CreateSectionFromComponents(int32 LODIndex, int32 SectionIndex, int32 MaterialSlot, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals, 
	const TArray<FVector2D>& UV0, const TArray<FColor>& VertexColors, const TArray<FRuntimeMeshTangent>& Tangents, ERuntimeMeshUpdateFrequency UpdateFrequency, bool bCreateCollision)
{
	FRuntimeMeshSectionProperties Properties;
	Properties.MaterialSlot = MaterialSlot;
	Properties.UpdateFrequency = UpdateFrequency;
	Properties.bWants32BitIndices = Vertices.Num() > MAX_uint16;
	Properties.bUseHighPrecisionTexCoords = true;
	Properties.NumTexCoords = 1;

	FRuntimeMeshRenderableMeshData SectionData(Properties);
	SectionData.Positions.Append(Vertices);
	SectionData.Tangents.Append(Normals, Tangents);
	if (SectionData.Tangents.Num() < SectionData.Positions.Num())
	{
		int32 Count = SectionData.Tangents.Num();
		SectionData.Tangents.SetNum(SectionData.Positions.Num());
		for (int32 Index = Count; Index < SectionData.Tangents.Num(); Index++)
		{
			SectionData.Tangents.SetTangents(Index, FVector(1, 0, 0), FVector(0, 1, 0), FVector(0, 0, 1));
		}
	}
	SectionData.Colors.Append(VertexColors);
	if (SectionData.Colors.Num() < SectionData.Positions.Num())
	{
		SectionData.Colors.SetNum(SectionData.Positions.Num());
	}

	int32 StartIndexTexCoords = SectionData.TexCoords.Num();
	SectionData.TexCoords.FillIn(StartIndexTexCoords, UV0, 0);

	if (SectionData.TexCoords.Num() < SectionData.Positions.Num())
	{
		SectionData.TexCoords.SetNum(SectionData.Positions.Num());
	}
	SectionData.Triangles.Append(Triangles);

	CreateSection(LODIndex, SectionIndex, Properties, SectionData);

	UpdateSectionAffectsCollision(LODIndex, SectionIndex, bCreateCollision);
}

void URuntimeMeshProviderStatic::UpdateSectionFromComponents(int32 LODIndex, int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals, 
	const TArray<FVector2D>& UV0, const TArray<FVector2D>& UV1, const TArray<FVector2D>& UV2, const TArray<FVector2D>& UV3, const TArray<FLinearColor>& VertexColors, const TArray<FRuntimeMeshTangent>& Tangents)
{
	int32 NumTexChannels =
		UV3.Num() > 0 ? 4 :
		UV2.Num() > 0 ? 3 :
		UV1.Num() > 0 ? 2 : 1;

	FRuntimeMeshRenderableMeshData SectionData(false, true, NumTexChannels, Vertices.Num() > MAX_uint16);
	SectionData.Positions.Append(Vertices);
	SectionData.Tangents.Append(Normals, Tangents);
	if (SectionData.Tangents.Num() < SectionData.Positions.Num())
	{
		int32 Count = SectionData.Tangents.Num();
		SectionData.Tangents.SetNum(SectionData.Positions.Num());
		for (int32 Index = Count; Index < SectionData.Tangents.Num(); Index++)
		{
			SectionData.Tangents.SetTangents(Index, FVector(1, 0, 0), FVector(0, 1, 0), FVector(0, 0, 1));
		}
	}
	SectionData.Colors.Append(VertexColors);
	if (SectionData.Colors.Num() < SectionData.Positions.Num())
	{
		SectionData.Colors.SetNum(SectionData.Positions.Num());
	}

	int32 StartIndexTexCoords = SectionData.TexCoords.Num();
	SectionData.TexCoords.FillIn(StartIndexTexCoords, UV0, 0);
	SectionData.TexCoords.FillIn(StartIndexTexCoords, UV1, 1);
	SectionData.TexCoords.FillIn(StartIndexTexCoords, UV2, 2);
	SectionData.TexCoords.FillIn(StartIndexTexCoords, UV3, 3);

	if (SectionData.TexCoords.Num() < SectionData.Positions.Num())
	{
		SectionData.TexCoords.SetNum(SectionData.Positions.Num());
	}
	SectionData.Triangles.Append(Triangles);

	UpdateSection(LODIndex, SectionIndex, SectionData);
}

void URuntimeMeshProviderStatic::UpdateSectionFromComponents(int32 LODIndex, int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals, 
	const TArray<FVector2D>& UV0, const TArray<FVector2D>& UV1, const TArray<FVector2D>& UV2, const TArray<FVector2D>& UV3, const TArray<FColor>& VertexColors, const TArray<FRuntimeMeshTangent>& Tangents)
{
	int32 NumTexChannels =
		UV3.Num() > 0 ? 4 :
		UV2.Num() > 0 ? 3 :
		UV1.Num() > 0 ? 2 : 1;

	FRuntimeMeshRenderableMeshData SectionData(false, true, NumTexChannels, Vertices.Num() > MAX_uint16);
	SectionData.Positions.Append(Vertices);
	SectionData.Tangents.Append(Normals, Tangents);
	if (SectionData.Tangents.Num() < SectionData.Positions.Num())
	{
		int32 Count = SectionData.Tangents.Num();
		SectionData.Tangents.SetNum(SectionData.Positions.Num());
		for (int32 Index = Count; Index < SectionData.Tangents.Num(); Index++)
		{
			SectionData.Tangents.SetTangents(Index, FVector(1, 0, 0), FVector(0, 1, 0), FVector(0, 0, 1));
		}
	}
	SectionData.Colors.Append(VertexColors);
	if (SectionData.Colors.Num() < SectionData.Positions.Num())
	{
		SectionData.Colors.SetNum(SectionData.Positions.Num());
	}

	int32 StartIndexTexCoords = SectionData.TexCoords.Num();
	SectionData.TexCoords.FillIn(StartIndexTexCoords, UV0, 0);
	SectionData.TexCoords.FillIn(StartIndexTexCoords, UV1, 1);
	SectionData.TexCoords.FillIn(StartIndexTexCoords, UV2, 2);
	SectionData.TexCoords.FillIn(StartIndexTexCoords, UV3, 3);

	if (SectionData.TexCoords.Num() < SectionData.Positions.Num())
	{
		SectionData.TexCoords.SetNum(SectionData.Positions.Num());
	}
	SectionData.Triangles.Append(Triangles);

	UpdateSection(LODIndex, SectionIndex, SectionData);
}

void URuntimeMeshProviderStatic::UpdateSectionFromComponents(int32 LODIndex, int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals,
	const TArray<FVector2D>& UV0, const TArray<FLinearColor>& VertexColors, const TArray<FRuntimeMeshTangent>& Tangents)
{
	int32 NumTexChannels = 1;

	FRuntimeMeshRenderableMeshData SectionData(false, true, NumTexChannels, Vertices.Num() > MAX_uint16);
	SectionData.Positions.Append(Vertices);
	SectionData.Tangents.Append(Normals, Tangents);
	if (SectionData.Tangents.Num() < SectionData.Positions.Num())
	{
		int32 Count = SectionData.Tangents.Num();
		SectionData.Tangents.SetNum(SectionData.Positions.Num());
		for (int32 Index = Count; Index < SectionData.Tangents.Num(); Index++)
		{
			SectionData.Tangents.SetTangents(Index, FVector(1, 0, 0), FVector(0, 1, 0), FVector(0, 0, 1));
		}
	}
	SectionData.Colors.Append(VertexColors);
	if (SectionData.Colors.Num() < SectionData.Positions.Num())
	{
		SectionData.Colors.SetNum(SectionData.Positions.Num());
	}

	int32 StartIndexTexCoords = SectionData.TexCoords.Num();
	SectionData.TexCoords.FillIn(StartIndexTexCoords, UV0, 0);

	if (SectionData.TexCoords.Num() < SectionData.Positions.Num())
	{
		SectionData.TexCoords.SetNum(SectionData.Positions.Num());
	}
	SectionData.Triangles.Append(Triangles);

	UpdateSection(LODIndex, SectionIndex, SectionData);
}

void URuntimeMeshProviderStatic::UpdateSectionFromComponents(int32 LODIndex, int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals,
	const TArray<FVector2D>& UV0, const TArray<FColor>& VertexColors, const TArray<FRuntimeMeshTangent>& Tangents)
{
	int32 NumTexChannels = 1;

	FRuntimeMeshRenderableMeshData SectionData(false, true, NumTexChannels, Vertices.Num() > MAX_uint16);
	SectionData.Positions.Append(Vertices);
	SectionData.Tangents.Append(Normals, Tangents);
	if (SectionData.Tangents.Num() < SectionData.Positions.Num())
	{
		int32 Count = SectionData.Tangents.Num();
		SectionData.Tangents.SetNum(SectionData.Positions.Num());
		for (int32 Index = Count; Index < SectionData.Tangents.Num(); Index++)
		{
			SectionData.Tangents.SetTangents(Index, FVector(1, 0, 0), FVector(0, 1, 0), FVector(0, 0, 1));
		}
	}
	SectionData.Colors.Append(VertexColors);
	if (SectionData.Colors.Num() < SectionData.Positions.Num())
	{
		SectionData.Colors.SetNum(SectionData.Positions.Num());
	}

	int32 StartIndexTexCoords = SectionData.TexCoords.Num();
	SectionData.TexCoords.FillIn(StartIndexTexCoords, UV0, 0);

	if (SectionData.TexCoords.Num() < SectionData.Positions.Num())
	{
		SectionData.TexCoords.SetNum(SectionData.Positions.Num());
	}
	SectionData.Triangles.Append(Triangles);

	UpdateSection(LODIndex, SectionIndex, SectionData);
}



void URuntimeMeshProviderStatic::Initialize_Implementation()
{
	UE_LOG(RuntimeMeshLog, Verbose, TEXT("StaticProvider(%d): Initialize called"), FPlatformTLS::GetCurrentThreadId());

	// Setup loaded materials
	for (int32 Index = 0; Index < LoadedMaterialSlots.Num(); Index++)
	{
		FName SlotName = LoadedMaterialSlots[Index].SlotName;
		UMaterialInterface* Material = LoadedMaterialSlots[Index].Material.Get();

		SetupMaterialSlot(Index, SlotName, Material);
	}
	LoadedMaterialSlots.Empty();

	// Setup existing LODs
 	if (LODConfigurations.Num() > 0)
	{
		//TMap<int32, TMap<int32, FSectionDataMapEntry>> SectionDataMapTemp = MoveTemp(SectionDataMap);

		URuntimeMeshProvider::ConfigureLODs_Implementation(LODConfigurations);

		// Setup existing sections
		for (const auto& LOD : SectionDataMap)
		{
			for (const auto& Section : LOD.Value)
			{
				// Create the section
				URuntimeMeshProvider::CreateSection_Implementation(LOD.Key, Section.Key, Section.Value.Get<0>());
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
		URuntimeMeshProvider::ConfigureLODs(
			{
				FRuntimeMeshLODProperties(),
			});
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
	UpdateSectionAffectsCollision(LODForMeshCollision, SectionId, bCollisionEnabled);
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
			if (Section)
			{
				if (Section->Get<1>().HasValidMeshData())
				{
					return true;
				}
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

void URuntimeMeshProviderStatic::UpdateSectionAffectsCollision(int32 LODIndex, int32 SectionId, bool bAffectsCollision)
{
	if (LODIndex == LODForMeshCollision)
	{
		if (bAffectsCollision && !SectionsForMeshCollision.Contains(SectionId))
		{
			SectionsForMeshCollision.Add(SectionId);
			MarkCollisionDirty();
		}
		else if (!bAffectsCollision && SectionsForMeshCollision.Contains(SectionId))
		{
			SectionsForMeshCollision.Remove(SectionId);
			MarkCollisionDirty();
		}
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
		}
	}
}