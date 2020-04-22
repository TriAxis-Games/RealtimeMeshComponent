// Copyright 2016-2020 Chris Conway (Koderz). All Rights Reserved.

#include "Components/RuntimeMeshComponentStatic.h"
#include "Providers/RuntimeMeshProviderStatic.h"
#include "Providers/RuntimeMeshProviderNormals.h"

URuntimeMeshComponentStatic::URuntimeMeshComponentStatic(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{

}

void URuntimeMeshComponentStatic::OnRegister()
{
	Super::OnRegister();

	if (RuntimeMesh)
	{
		SetRuntimeMesh(RuntimeMesh);
	}
	else
	{
		URuntimeMeshProvider* RootProvider;

		StaticProvider = NewObject<URuntimeMeshProviderStatic>(this, TEXT("RuntimeMeshProvider-Static"));
		StaticProvider->SetRenderableLODForCollision(LODForMeshCollision);
		for (auto& elem : SectionsForMeshCollision)
		{
			StaticProvider->SetRenderableSectionAffectsCollision(elem, true);
		}
		StaticProvider->SetCollisionSettings(CollisionSettings);
		StaticProvider->SetCollisionMesh(CollisionMesh);
		RootProvider = StaticProvider;

		if (bWantsNormals || bWantsTangents)
		{
			NormalsProvider = NewObject<URuntimeMeshProviderNormals>(this, TEXT("RuntimeMeshProvider-Normals"));
			NormalsProvider->SourceProvider = RootProvider;
			NormalsProvider->ComputeNormals = bWantsNormals;
			NormalsProvider->ComputeTangents = bWantsNormals || bWantsTangents;
			RootProvider = NormalsProvider;
		}

		Initialize(RootProvider);
	}
	
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

URuntimeMeshProviderStatic * URuntimeMeshComponentStatic::GetStaticProvider()
{
	if (GetNormalsProvider())
	{
		StaticProvider = (URuntimeMeshProviderStatic*)NormalsProvider->SourceProvider;
	}
	else
	{
		StaticProvider = (URuntimeMeshProviderStatic*)GetProvider();
	}
	return StaticProvider;
}

URuntimeMeshProviderNormals* URuntimeMeshComponentStatic::GetNormalsProvider()
{
	NormalsProvider = (URuntimeMeshProviderNormals*)GetProvider();;
	return NormalsProvider;
}

void URuntimeMeshComponentStatic::GetOrCreateNormalsProvider()
{
	if (NormalsProvider)
	{
		return;
	}
	else
	{
		NormalsProvider = NewObject<URuntimeMeshProviderNormals>(this, TEXT("RuntimeMeshProvider-Normals"));
		NormalsProvider->SourceProvider = StaticProvider;
		NormalsProvider->ComputeNormals = false;
		NormalsProvider->ComputeTangents = false;
		Initialize(NormalsProvider);
	}
}

void URuntimeMeshComponentStatic::EnsureNormalsProviderIsWanted()
{
	if (!NormalsProvider->ComputeNormals && !NormalsProvider->ComputeTangents)
	{
		Initialize(StaticProvider);
		NormalsProvider = nullptr;
	}
}

void URuntimeMeshComponentStatic::SetbWantsNormals(bool InbWantsNormals, bool bDeleteNormalsProviderIfUseless)
{
	if (InbWantsNormals)
	{
		GetOrCreateNormalsProvider();
		NormalsProvider->ComputeNormals = true;
		NormalsProvider->MarkProxyParametersDirty();
	}
	else
	{
		if (NormalsProvider)
		{
			NormalsProvider->ComputeNormals = false;
			NormalsProvider->MarkProxyParametersDirty();
			if (bDeleteNormalsProviderIfUseless)
			{
				EnsureNormalsProviderIsWanted();
			}
		}
	}
}

void URuntimeMeshComponentStatic::SetbWantsTangents(bool InbWantsTangents, bool bDeleteNormalsProviderIfUseless)
{
	if (InbWantsTangents)
	{
		GetOrCreateNormalsProvider();
		NormalsProvider->ComputeTangents = true;
		NormalsProvider->MarkProxyParametersDirty();
	}
	else
	{
		if (NormalsProvider)
		{
			NormalsProvider->ComputeTangents = false;
			NormalsProvider->MarkProxyParametersDirty();
			if (bDeleteNormalsProviderIfUseless)
			{
				EnsureNormalsProviderIsWanted();
			}
		}
	}
}

bool URuntimeMeshComponentStatic::GetbWantsNormals()
{
	if (GetNormalsProvider())
	{
		return NormalsProvider->ComputeNormals;
	}
	return false;
}

bool URuntimeMeshComponentStatic::GetbWantsTangents()
{
	if (GetNormalsProvider())
	{
		return NormalsProvider->ComputeTangents;
	}
	return false;
}

int32 URuntimeMeshComponentStatic::GetLODForMeshCollision()
{
	if (GetStaticProvider())
	{
		return StaticProvider->GetRenderableLODForCollision();
	}
	return int32();
}

TSet<int32> URuntimeMeshComponentStatic::GetSectionsForMeshCollision()
{
	if (GetStaticProvider())
	{
		return StaticProvider->GetRenderableSectionAffectingCollision();
	}
	return TSet<int32>();
}

FRuntimeMeshCollisionSettings URuntimeMeshComponentStatic::GetCollisionSettings()
{
	if (GetStaticProvider())
	{
		return StaticProvider->GetCollisionSettingsStored();
	}
	return FRuntimeMeshCollisionSettings();
}

FRuntimeMeshCollisionData URuntimeMeshComponentStatic::GetCollisionMesh()
{
	if (GetStaticProvider())
	{
		return StaticProvider->GetCollisionMeshWithoutVisible();
	}
	return FRuntimeMeshCollisionData();
}
