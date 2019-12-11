// Copyright 2016-2019 Chris Conway (Koderz). All Rights Reserved.


#include "RuntimeMeshProviderStaticMesh.h"
#include "RuntimeMeshStaticMeshConverter.h"

URuntimeMeshProviderStaticMesh::URuntimeMeshProviderStaticMesh()
	: StaticMesh(nullptr)
	, MaxLOD(RuntimeMesh_MAXLODS)
	, ComplexCollisionLOD(0)
{

}

void URuntimeMeshProviderStaticMesh::Initialize_Implementation()
{
	// Setup the lods and sections

	// Check valid static mesh
	if (StaticMesh == nullptr || StaticMesh->IsPendingKill())
	{
		return;
	}

	// Check mesh data is accessible
	if (!((GIsEditor || StaticMesh->bAllowCPUAccess) && StaticMesh->RenderData != nullptr))
	{
		return;
	}

	// Copy materials
	const auto& MaterialSlots = StaticMesh->StaticMaterials;
	for (int32 SlotIndex = 0; SlotIndex < MaterialSlots.Num(); SlotIndex++)
	{
		SetupMaterialSlot(SlotIndex, MaterialSlots[SlotIndex].MaterialSlotName, MaterialSlots[SlotIndex].MaterialInterface);
	}

	// Copy LODs
	const auto& LODResources = StaticMesh->RenderData->LODResources;
	for (int32 LODIndex = 0; LODIndex < LODResources.Num() && LODIndex <= MaxLOD; LODIndex++)
	{
		const auto& LOD = LODResources[LODIndex];
		
		// Copy Sections
		for (int32 SectionId = 0; SectionId < LOD.Sections.Num(); SectionId++)
		{
			const auto& Section = LOD.Sections[SectionId];

			FRuntimeMeshSectionProperties SectionProperties;
			

			CreateSection(LODIndex, SectionId, SectionProperties);


		}
	}
}

FBoxSphereBounds URuntimeMeshProviderStaticMesh::GetBounds_Implementation()
{
	if (StaticMesh)
	{
		return StaticMesh->GetBounds();
	}
	return FBoxSphereBounds(ForceInit);
}

bool URuntimeMeshProviderStaticMesh::GetSectionMeshForLOD_Implementation(int32 LODIndex, int32 SectionId, FRuntimeMeshRenderableMeshData& MeshData)
{ 
	return URuntimeMeshStaticMeshConverter::CopyStaticMeshSectionToRenderableMeshData(StaticMesh, LODIndex, SectionId, MeshData);
}

FRuntimeMeshCollisionSettings URuntimeMeshProviderStaticMesh::GetCollisionSettings_Implementation()
{
	FRuntimeMeshCollisionSettings NewSettings;
	NewSettings.bUseAsyncCooking = true;
	NewSettings.bUseComplexAsSimple = false;
	NewSettings.CookingMode = ERuntimeMeshCollisionCookingMode::CookingPerformance;
	
	URuntimeMeshStaticMeshConverter::CopyStaticMeshCollisionToCollisionSettings(StaticMesh, NewSettings);

	return NewSettings;
}

bool URuntimeMeshProviderStaticMesh::HasCollisionMesh_Implementation()
{
	return ComplexCollisionLOD >= 0;
}

bool URuntimeMeshProviderStaticMesh::GetCollisionMesh_Implementation(FRuntimeMeshCollisionData& CollisionData)
{
	return URuntimeMeshStaticMeshConverter::CopyStaticMeshLODToCollisionData(StaticMesh, ComplexCollisionLOD, CollisionData);
}
