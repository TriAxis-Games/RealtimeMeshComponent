// Copyright 2016-2020 TriAxis Games L.L.C. All Rights Reserved.

#include "Components/RuntimeMeshComponentStatic.h"
#include "Providers/RuntimeMeshProviderStatic.h"
#include "Providers/RuntimeMeshProviderModifiers.h"
#include "Modifiers/RuntimeMeshModifierNormals.h"
#include "Modifiers/RuntimeMeshModifierAdjacency.h"

URuntimeMeshComponentStatic::URuntimeMeshComponentStatic()
{
	// We can create our static provider here as a default subobject
	StaticProvider = CreateDefaultSubobject<URuntimeMeshProviderStatic>("StaticProvider");
}

void URuntimeMeshComponentStatic::OnRegister()
{
	if (RuntimeMesh)
	{
		StaticProvider = CastChecked<URuntimeMeshProviderStatic>(RuntimeMesh->GetProviderPtr());
		SetRuntimeMesh(RuntimeMesh);
	}
	else
	{
		Initialize(StaticProvider);
		RuntimeMesh = GetRuntimeMesh();
	}

	Super::OnRegister();
}

void URuntimeMeshComponentStatic::CreateSection_Blueprint(int32 LODIndex, int32 SectionId, const FRuntimeMeshSectionProperties & SectionProperties, const FRuntimeMeshRenderableMeshData & SectionData)
{
	StaticProvider->CreateSection_Blueprint(LODIndex, SectionId, SectionProperties, SectionData);
}

void URuntimeMeshComponentStatic::CreateSectionFromComponents(int32 LODIndex, int32 SectionIndex, int32 MaterialSlot, 
	const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals, const TArray<FVector2D>& UV0, const TArray<FVector2D>& UV1, const TArray<FVector2D>& UV2, 
	const TArray<FVector2D>& UV3, const TArray<FLinearColor>& VertexColors, const TArray<FRuntimeMeshTangent>& Tangents, ERuntimeMeshUpdateFrequency UpdateFrequency, bool bCreateCollision)
{
	StaticProvider->CreateSectionFromComponents(LODIndex, SectionIndex, MaterialSlot,
		Vertices, Triangles, Normals, UV0, UV1, UV2, UV3, VertexColors, Tangents, UpdateFrequency, bCreateCollision);
}

void URuntimeMeshComponentStatic::CreateSectionFromComponents(int32 LODIndex, int32 SectionIndex, int32 MaterialSlot, const TArray<FVector>& Vertices, const TArray<int32>& Triangles,
	const TArray<FVector>& Normals, const TArray<FVector2D>& UV0, const TArray<FVector2D>& UV1, const TArray<FVector2D>& UV2, const TArray<FVector2D>& UV3, const TArray<FColor>& VertexColors,
	const TArray<FRuntimeMeshTangent>& Tangents, ERuntimeMeshUpdateFrequency UpdateFrequency, bool bCreateCollision)
{
	StaticProvider->CreateSectionFromComponents(LODIndex, SectionIndex, MaterialSlot,
		Vertices, Triangles, Normals, UV0, UV1, UV2, UV3, VertexColors, Tangents, UpdateFrequency, bCreateCollision);
}

void URuntimeMeshComponentStatic::CreateSectionFromComponents(int32 LODIndex, int32 SectionIndex, int32 MaterialSlot, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, 
	const TArray<FVector>& Normals, const TArray<FVector2D>& UV0, const TArray<FLinearColor>& VertexColors, const TArray<FRuntimeMeshTangent>& Tangents,
	ERuntimeMeshUpdateFrequency UpdateFrequency, bool bCreateCollision)
{
	StaticProvider->CreateSectionFromComponents(LODIndex, SectionIndex, MaterialSlot,
		Vertices, Triangles, Normals, UV0, VertexColors, Tangents, UpdateFrequency, bCreateCollision);
}

void URuntimeMeshComponentStatic::CreateSectionFromComponents(int32 LODIndex, int32 SectionIndex, int32 MaterialSlot, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, 
	const TArray<FVector>& Normals, const TArray<FVector2D>& UV0, const TArray<FColor>& VertexColors, const TArray<FRuntimeMeshTangent>& Tangents,
	ERuntimeMeshUpdateFrequency UpdateFrequency, bool bCreateCollision)
{
	StaticProvider->CreateSectionFromComponents(LODIndex, SectionIndex, MaterialSlot,
		Vertices, Triangles, Normals, UV0, VertexColors, Tangents, UpdateFrequency, bCreateCollision);
}

void URuntimeMeshComponentStatic::UpdateSection_Blueprint(int32 LODIndex, int32 SectionId, const FRuntimeMeshRenderableMeshData & SectionData)
{
	StaticProvider->UpdateSection_Blueprint(LODIndex, SectionId, SectionData);
}

void URuntimeMeshComponentStatic::UpdateSectionFromComponents(int32 LODIndex, int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, 
	const TArray<FVector>& Normals, const TArray<FVector2D>& UV0, const TArray<FVector2D>& UV1, const TArray<FVector2D>& UV2, const TArray<FVector2D>& UV3, 
	const TArray<FLinearColor>& VertexColors, const TArray<FRuntimeMeshTangent>& Tangents)
{
	StaticProvider->UpdateSectionFromComponents(LODIndex, SectionIndex,
		Vertices, Triangles, Normals, UV0, UV1, UV2, UV3, VertexColors, Tangents);
}

void URuntimeMeshComponentStatic::UpdateSectionFromComponents(int32 LODIndex, int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, 
	const TArray<FVector>& Normals, const TArray<FVector2D>& UV0, const TArray<FVector2D>& UV1, const TArray<FVector2D>& UV2, const TArray<FVector2D>& UV3, 
	const TArray<FColor>& VertexColors, const TArray<FRuntimeMeshTangent>& Tangents)
{
	StaticProvider->UpdateSectionFromComponents(LODIndex, SectionIndex,
		Vertices, Triangles, Normals, UV0, UV1, UV2, UV3, VertexColors, Tangents);
}

void URuntimeMeshComponentStatic::UpdateSectionFromComponents(int32 LODIndex, int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals,
	const TArray<FVector2D>& UV0, const TArray<FLinearColor>& VertexColors, const TArray<FRuntimeMeshTangent>& Tangents)
{
	StaticProvider->UpdateSectionFromComponents(LODIndex, SectionIndex,
		Vertices, Triangles, Normals, UV0, VertexColors, Tangents);
}

void URuntimeMeshComponentStatic::UpdateSectionFromComponents(int32 LODIndex, int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals,
	const TArray<FVector2D>& UV0, const TArray<FColor>& VertexColors, const TArray<FRuntimeMeshTangent>& Tangents)
{
	StaticProvider->UpdateSectionFromComponents(LODIndex, SectionIndex,
		Vertices, Triangles, Normals, UV0, VertexColors, Tangents);
}

void URuntimeMeshComponentStatic::ClearSection(int32 LODIndex, int32 SectionId)
{
	StaticProvider->ClearSection(LODIndex, SectionId);
}

void URuntimeMeshComponentStatic::RemoveSection(int32 LODIndex, int32 SectionId)
{
	StaticProvider->RemoveSection(LODIndex, SectionId);
}

void URuntimeMeshComponentStatic::SetCollisionSettings(const FRuntimeMeshCollisionSettings & NewCollisionSettings)
{
	StaticProvider->SetCollisionSettings(NewCollisionSettings);
}

void URuntimeMeshComponentStatic::SetCollisionMesh(const FRuntimeMeshCollisionData & NewCollisionMesh)
{
	StaticProvider->SetCollisionMesh(NewCollisionMesh);
}

void URuntimeMeshComponentStatic::SetRenderableLODForCollision(int32 LODIndex)
{
	StaticProvider->SetRenderableLODForCollision(LODIndex);
}

void URuntimeMeshComponentStatic::SetRenderableSectionAffectsCollision(int32 SectionId, bool bCollisionEnabled)
{
	StaticProvider->SetRenderableSectionAffectsCollision(SectionId, bCollisionEnabled);
}

TArray<int32> URuntimeMeshComponentStatic::GetSectionIds(int32 LODIndex) const
{
	return StaticProvider->GetSectionIds(LODIndex);
}

int32 URuntimeMeshComponentStatic::GetLastSectionId(int32 LODIndex) const
{
	return StaticProvider->GetLastSectionId(LODIndex);
}

bool URuntimeMeshComponentStatic::DoesSectionHaveValidMeshData(int32 LODIndex, int32 SectionId) const
{
	return StaticProvider->DoesSectionHaveValidMeshData(LODIndex, SectionId);
}

void URuntimeMeshComponentStatic::RemoveAllSectionsForLOD(int32 LODIndex)
{
	return StaticProvider->RemoveAllSectionsForLOD(LODIndex);
}

FBoxSphereBounds URuntimeMeshComponentStatic::GetSectionBounds(int32 LODIndex, int32 SectionId) const
{
	return StaticProvider->GetSectionBounds(LODIndex, SectionId);
}

FRuntimeMeshSectionProperties URuntimeMeshComponentStatic::GetSectionProperties(int32 LODIndex, int32 SectionId) const
{
	return StaticProvider->GetSectionProperties(LODIndex, SectionId);
}

FRuntimeMeshRenderableMeshData URuntimeMeshComponentStatic::GetSectionRenderData(int32 LODIndex, int32 SectionId) const
{
	return StaticProvider->GetSectionRenderData(LODIndex, SectionId);
}

FRuntimeMeshRenderableMeshData URuntimeMeshComponentStatic::GetSectionRenderDataAndClear(int32 LODIndex, int32 SectionId)
{
	return StaticProvider->GetSectionRenderDataAndClear(LODIndex, SectionId);
}

int32 URuntimeMeshComponentStatic::GetLODForMeshCollision() const
{
	return StaticProvider->GetLODForMeshCollision();
}

TSet<int32> URuntimeMeshComponentStatic::GetSectionsForMeshCollision() const
{
	return StaticProvider->GetSectionsForMeshCollision();
}

FRuntimeMeshCollisionSettings URuntimeMeshComponentStatic::GetCollisionSettings() const
{
	return StaticProvider->GetCollisionSettingsStatic();
}

FRuntimeMeshCollisionData URuntimeMeshComponentStatic::GetCollisionMesh() const
{
	return StaticProvider->GetCollisionMeshStatic();
}





URuntimeMeshProviderStatic * URuntimeMeshComponentStatic::GetStaticProvider()
{
	return StaticProvider;
}

void URuntimeMeshComponentStatic::EnableNormalTangentGeneration()
{
	if (!NormalsModifier)
	{
		NormalsModifier = NewObject<URuntimeMeshModifierNormals>(this);
		StaticProvider->RegisterModifier(NormalsModifier);
	}
}

void URuntimeMeshComponentStatic::DisableNormalTangentGeneration()
{
	if (NormalsModifier)
	{
		StaticProvider->UnRegisterModifier(NormalsModifier);
		NormalsModifier = nullptr;
	}
}

bool URuntimeMeshComponentStatic::HasNormalTangentGenerationEnabled() const
{
	return NormalsModifier != nullptr;
}

void URuntimeMeshComponentStatic::EnabledTessellationTrianglesGeneration()
{
	if (!AdjacencyModifier)
	{
		AdjacencyModifier = NewObject<URuntimeMeshModifierAdjacency>(this);
		StaticProvider->RegisterModifier(AdjacencyModifier);
	}
}

void URuntimeMeshComponentStatic::DisableTessellationTrianglesGeneration()
{
	if (AdjacencyModifier)
	{
		StaticProvider->UnRegisterModifier(AdjacencyModifier);
		AdjacencyModifier = nullptr;
	}
}

bool URuntimeMeshComponentStatic::HasTessellationTriangleGenerationEnabled() const
{
	return AdjacencyModifier != nullptr;
}





