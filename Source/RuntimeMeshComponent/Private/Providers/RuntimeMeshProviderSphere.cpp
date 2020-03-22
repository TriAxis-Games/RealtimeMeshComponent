// Copyright 2016-2019 Chris Conway (Koderz). All Rights Reserved.


#include "RuntimeMeshProviderSphere.h"


FRuntimeMeshProviderSphereProxy::FRuntimeMeshProviderSphereProxy(TWeakObjectPtr<URuntimeMeshProvider> InParent)
	: FRuntimeMeshProviderProxy(InParent)
{

}

FRuntimeMeshProviderSphereProxy::~FRuntimeMeshProviderSphereProxy()
{

}

void FRuntimeMeshProviderSphereProxy::UpdateProxyParameters(URuntimeMeshProvider* ParentProvider, bool bIsInitialSetup)
{
	URuntimeMeshProviderSphere* SphereProvider = Cast<URuntimeMeshProviderSphere>(ParentProvider);
	if (SphereRadius != SphereProvider->SphereRadius)
	{
		MarkCollisionDirty();
	}
	SphereRadius = SphereProvider->SphereRadius;
	LatitudeSegmentsLOD0 = SphereProvider->LatitudeSegments;
	LongitudeSegmentsLOD0 = SphereProvider->LongitudeSegments;
	LODMultiplier = SphereProvider->LODMultiplier;
	Material = SphereProvider->Material;
	int32 MaxLODBefore = MaxLOD;
	MaxLOD = GetMaximumPossibleLOD();
	if (!bIsInitialSetup)
	{
		for (int32 LODIndex = 0; LODIndex <= MaxLOD; LODIndex++)
		{
			FRuntimeMeshLODProperties LODProperties;
			LODProperties.ScreenSize = CalculateScreenSize(LODIndex);
			ConfigureLOD(LODIndex, LODProperties);

			if (LODIndex >= MaxLODBefore)
			{
				FRuntimeMeshSectionProperties Properties;
				Properties.bCastsShadow = true;
				Properties.bIsVisible = true;
				Properties.MaterialSlot = 0;
				Properties.UpdateFrequency = ERuntimeMeshUpdateFrequency::Infrequent;
				CreateSection(LODIndex, 0, Properties);
			}
			else
			{
				MarkSectionDirty(LODIndex, 0);
			}
		}
		for (int32 LODIndex = MaxLOD; LODIndex < MaxLODBefore; LODIndex++)
		{
			RemoveSection(LODIndex, 0);
		}
	}
	
}

void FRuntimeMeshProviderSphereProxy::Initialize()
{
	for (int32 LODIndex = 0; LODIndex <= MaxLOD; LODIndex++)
	{
		FRuntimeMeshLODProperties LODProperties;
		LODProperties.ScreenSize = CalculateScreenSize(LODIndex);
		ConfigureLOD(LODIndex, LODProperties);
		
		FRuntimeMeshSectionProperties Properties;
		Properties.bCastsShadow = true;
		Properties.bIsVisible = true;
		Properties.MaterialSlot = 0;
		Properties.UpdateFrequency = ERuntimeMeshUpdateFrequency::Infrequent;
		CreateSection(LODIndex, 0, Properties);

	}
	SetupMaterialSlot(0, FName("Sphere Base"), Material.Get());
}

bool FRuntimeMeshProviderSphereProxy::GetSphereMesh(int32 LatitudeSegments, int32 LongitudeSegments, FRuntimeMeshRenderableMeshData & MeshData)
{
	TArray<FVector> LatitudeVerts;
	TArray<FVector> TangentVerts;
	int32 TrisOrder[6] = { 0, 1, LatitudeSegments + 1, 1, LatitudeSegments + 2, LatitudeSegments + 1 };
	LatitudeVerts.SetNumUninitialized(LatitudeSegments + 1);
	TangentVerts.SetNumUninitialized(LatitudeSegments + 1);
	for (int32 LatitudeIndex = 0; LatitudeIndex < LatitudeSegments + 1; LatitudeIndex++)
	{
		float angle = LatitudeIndex * 2.f * PI / LatitudeSegments;
		float x, y;
		FMath::SinCos(&y, &x, angle);
		LatitudeVerts[LatitudeIndex] = FVector(x, y, 0);
		FMath::SinCos(&y, &x, angle + PI / 2.f);
		TangentVerts[LatitudeIndex] = FVector(x, y, 0);
	}
	for (int32 LongitudeIndex = 0; LongitudeIndex < LongitudeSegments + 1; LongitudeIndex++)
	{
		float angle = LongitudeIndex * PI / LongitudeSegments;
		float z, r;
		FMath::SinCos(&r, &z, angle);
		for (int32 LatitudeIndex = 0; LatitudeIndex < LatitudeSegments + 1; LatitudeIndex++)
		{
			FVector Normal = LatitudeVerts[LatitudeIndex] * r + FVector(0, 0, z);
			FVector Position = Normal * SphereRadius;
			MeshData.Positions.Add(Position);
			MeshData.Tangents.Add(Normal, TangentVerts[LatitudeIndex]);
			MeshData.TexCoords.Add(FVector2D((float)LatitudeIndex / LatitudeSegments, (float)LongitudeIndex / LongitudeSegments));
			MeshData.Colors.Add(FColor::White);
		}
	}
	for (int32 LongitudeIndex = 0; LongitudeIndex < LongitudeSegments; LongitudeIndex++)
	{
		for (int32 LatitudeIndex = 0; LatitudeIndex < LatitudeSegments; LatitudeIndex++)
		{
			int32 TrisNumber = LatitudeIndex + LongitudeIndex * (LatitudeSegments + 1);
			for (int32 TrisIndex = 0; TrisIndex < 6; TrisIndex++)
			{
				MeshData.Triangles.Add(TrisOrder[TrisIndex] + TrisNumber);
			}
		}
	}
	return true;
}


bool FRuntimeMeshProviderSphereProxy::GetSectionMeshForLOD(int32 LODIndex, int32 SectionId, FRuntimeMeshRenderableMeshData& MeshData)
{
	// We should only ever be queried for section 0 and lod 0
	check(SectionId == 0 && LODIndex <= MaxLOD);

	int32 LatSegments = FMath::Max(FMath::RoundToInt(LatitudeSegmentsLOD0 * FMath::Pow(LODMultiplier, LODIndex)), 1);
	int32 LonSegments = FMath::Max(FMath::RoundToInt(LongitudeSegmentsLOD0 * FMath::Pow(LODMultiplier, LODIndex)), 1);

	return GetSphereMesh(LatSegments, LonSegments, MeshData);
}

FRuntimeMeshCollisionSettings FRuntimeMeshProviderSphereProxy::GetCollisionSettings()
{
	FRuntimeMeshCollisionSettings Settings;
	Settings.bUseAsyncCooking = false;
	Settings.bUseComplexAsSimple = false;

	Settings.Spheres.Emplace(SphereRadius);
	   	 
	return Settings;
}

bool FRuntimeMeshProviderSphereProxy::HasCollisionMesh()
{
	return false;
}

bool FRuntimeMeshProviderSphereProxy::GetCollisionMesh(FRuntimeMeshCollisionData& CollisionData)
{
	return false;
}

URuntimeMeshProviderSphere::URuntimeMeshProviderSphere()
{
	SphereRadius = 100.f;
	LatitudeSegments = 32;
	LongitudeSegments = 16;
	LODMultiplier = 0.5;
}