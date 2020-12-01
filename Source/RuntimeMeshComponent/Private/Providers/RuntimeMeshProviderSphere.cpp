// Copyright 2016-2020 TriAxis Games L.L.C. All Rights Reserved.


#include "Providers/RuntimeMeshProviderSphere.h"
#include "RuntimeMeshComponentPlugin.h"


URuntimeMeshProviderSphere::URuntimeMeshProviderSphere()
	: MaxLOD(0)
	, SphereRadius(100.0f)
	, MaxLatitudeSegments(32)
	, MinLatitudeSegments(8)
	, MaxLongitudeSegments(16)
	, MinLongitudeSegments(5)
	, LODMultiplier(0.75)
	, SphereMaterial(nullptr)
{
	MaxLOD = GetMaxNumberOfLODs() - 1;
}

float URuntimeMeshProviderSphere::GetSphereRadius() const
{
	FScopeLock Lock(&PropertySyncRoot);
	return SphereRadius;
}

void URuntimeMeshProviderSphere::SetSphereRadius(float InSphereRadius)
{
	FScopeLock Lock(&PropertySyncRoot);
	SphereRadius = InSphereRadius; 
	UpdateMeshParameters(true);
}

int32 URuntimeMeshProviderSphere::GetMaxLatitudeSegments() const
{
	FScopeLock Lock(&PropertySyncRoot);
	return MaxLatitudeSegments;
}

void URuntimeMeshProviderSphere::SetMaxLatitudeSegments(int32 InMaxLatitudeSegments)
{
	FScopeLock Lock(&PropertySyncRoot);
	MaxLatitudeSegments = InMaxLatitudeSegments; 
	UpdateMeshParameters(false);
}

int32 URuntimeMeshProviderSphere::GetMinLatitudeSegments() const
{
	FScopeLock Lock(&PropertySyncRoot);
	return MinLatitudeSegments;
}

void URuntimeMeshProviderSphere::SetMinLatitudeSegments(int32 InMinLatitudeSegments)
{
	FScopeLock Lock(&PropertySyncRoot);
	MinLatitudeSegments = InMinLatitudeSegments; 
	UpdateMeshParameters(false);
}

int32 URuntimeMeshProviderSphere::GetMaxLongitudeSegments() const
{
	FScopeLock Lock(&PropertySyncRoot);
	return MaxLongitudeSegments;
}

void URuntimeMeshProviderSphere::SetMaxLongitudeSegments(int32 InMaxLongitudeSegments)
{
	FScopeLock Lock(&PropertySyncRoot);
	MaxLongitudeSegments = InMaxLongitudeSegments; 
	UpdateMeshParameters(false);
}

int32 URuntimeMeshProviderSphere::GetMinLongitudeSegments() const
{
	FScopeLock Lock(&PropertySyncRoot);
	return MinLongitudeSegments;
}

void URuntimeMeshProviderSphere::SetMinLongitudeSegments(int32 InMinLongitudeSegments)
{
	FScopeLock Lock(&PropertySyncRoot);
	MinLongitudeSegments = InMinLongitudeSegments; 
	UpdateMeshParameters(false);
}

float URuntimeMeshProviderSphere::GetLODMultiplier() const
{
	FScopeLock Lock(&PropertySyncRoot);
	return LODMultiplier;
}

void URuntimeMeshProviderSphere::SetLODMultiplier(float InLODMultiplier)
{
	FScopeLock Lock(&PropertySyncRoot);
	if (InLODMultiplier >= 1)
	{
		//If the LODMultiplier is greater than one, there'd be infinite LODs as it would diverge. (It's a geometric series)
		UE_LOG(RuntimeMeshLog, Error, TEXT("RMC Sphere Provider(%d): LODMultiplier was set greater than or equal to 1 ! Aborting ! Value : %f"), FPlatformTLS::GetCurrentThreadId(), InLODMultiplier);
		return;
	}
	LODMultiplier = InLODMultiplier; 
	UpdateMeshParameters(false);
}

UMaterialInterface* URuntimeMeshProviderSphere::GetSphereMaterial() const
{
	FScopeLock Lock(&PropertySyncRoot);
	return SphereMaterial;
}

void URuntimeMeshProviderSphere::SetSphereMaterial(UMaterialInterface* InSphereMaterial)
{
	FScopeLock Lock(&PropertySyncRoot);
	SphereMaterial = InSphereMaterial;
	this->SetupMaterialSlot(0, FName("Sphere Base"), SphereMaterial);
}

void URuntimeMeshProviderSphere::Initialize()
{
	SetupMaterialSlot(0, FName("Sphere Base"), SphereMaterial);

	// Setup LODs
	TArray<FRuntimeMeshLODProperties> LODs;
	for (int32 LODIndex = 0; LODIndex <= MaxLOD; LODIndex++)
	{
		FRuntimeMeshLODProperties LODProperties;
		LODProperties.ScreenSize = CalculateScreenSize(LODIndex);
		LODs.Add(LODProperties);
	}
	ConfigureLODs(LODs);

	// Setup sections
	for (int32 LODIndex = 0; LODIndex <= MaxLOD; LODIndex++)
	{
		FRuntimeMeshSectionProperties Properties;
		Properties.bCastsShadow = true;
		Properties.bIsVisible = true;
		Properties.MaterialSlot = 0;
		Properties.UpdateFrequency = ERuntimeMeshUpdateFrequency::Infrequent;
		int32 LatSegments, LonSegments;
		GetSegmentsForLOD(LODIndex, LatSegments, LonSegments);
		Properties.bWants32BitIndices = (LatSegments + 1)*(LonSegments + 1) >= 1 << 16; //1<<16 is the 16 bit integer limit (2^16), so if we have more verts than that then we switch
		CreateSection(LODIndex, 0, Properties);
	}
}

bool URuntimeMeshProviderSphere::GetSectionMeshForLOD(int32 LODIndex, int32 SectionId, FRuntimeMeshRenderableMeshData& MeshData)
{
	UE_LOG(RuntimeMeshLog, Verbose, TEXT("RMC Sphere Provider(%d): Getting LOD:%d Section:%d"), FPlatformTLS::GetCurrentThreadId(), LODIndex, SectionId);

	// We should only ever be queried for section 0
	check(SectionId == 0 && LODIndex <= MaxLOD);

	float TempRadius;
	int32 TempMinLat, TempMaxLat;
	int32 TempMinLong, TempMaxLong;
	float TempLODMultiplier;

	GetShapeParams(TempRadius, TempMinLat, TempMaxLat, TempMinLong, TempMaxLong, TempLODMultiplier);

	int32 LatSegments, LonSegments;
	GetSegmentsForLOD(LODIndex, TempLODMultiplier, TempMaxLat, TempMinLat, TempMaxLong, TempMinLong, LatSegments, LonSegments);

	return GetSphereMesh(TempRadius, LatSegments, LonSegments, MeshData);
}

FRuntimeMeshCollisionSettings URuntimeMeshProviderSphere::GetCollisionSettings()
{
	FRuntimeMeshCollisionSettings Settings;
	Settings.bUseAsyncCooking = false;
	Settings.bUseComplexAsSimple = false;

	Settings.Spheres.Emplace(GetSphereRadius());

	return Settings;
}

FBoxSphereBounds URuntimeMeshProviderSphere::GetBounds()
{
	return FBoxSphereBounds(FSphere(FVector::ZeroVector, SphereRadius));
}

bool URuntimeMeshProviderSphere::IsThreadSafe()
{
	return true;
}



void URuntimeMeshProviderSphere::GetShapeParams(float& OutRadius, int32& OutMinLatitudeSegments, int32& OutMaxLatitudeSegments, int32& OutMinLongitudeSegments, int32& OutMaxLongitudeSegments, float& OutLODMultiplier)
{
	FScopeLock Lock(&PropertySyncRoot);
	OutRadius = SphereRadius;
	OutMinLatitudeSegments = MinLatitudeSegments;
	OutMaxLatitudeSegments = MaxLatitudeSegments;
	OutMinLongitudeSegments = MinLongitudeSegments;
	OutMaxLongitudeSegments = MaxLongitudeSegments;
	OutLODMultiplier = LODMultiplier;
}

int32 URuntimeMeshProviderSphere::GetMaxNumberOfLODs()
{
	FScopeLock Lock(&PropertySyncRoot);
	int32 MaxLODs = 1;
	float CurrentLatitudeSegments = MaxLatitudeSegments;
	float CurrentLongitudeSegments = MaxLongitudeSegments;

	while (MaxLODs < RUNTIMEMESH_MAXLODS)
	{
		CurrentLatitudeSegments *= LODMultiplier;
		CurrentLongitudeSegments *= LODMultiplier;

		// Have we gone far enough?
		if (CurrentLatitudeSegments <= MinLatitudeSegments && CurrentLongitudeSegments <= MinLongitudeSegments)
		{
			MaxLODs++;
			break;
		}

		MaxLODs++;
	}

	return MaxLODs;
}

float URuntimeMeshProviderSphere::CalculateScreenSize(int32 LODIndex)
{
	FScopeLock Lock(&PropertySyncRoot);
	float ScreenSize = FMath::Pow(LODMultiplier, LODIndex);

	return ScreenSize;
}

bool URuntimeMeshProviderSphere::GetSphereMesh(int32 SphereRadius, int32 LatitudeSegments, int32 LongitudeSegments, FRuntimeMeshRenderableMeshData& MeshData)
{
	TArray<FVector> LatitudeVerts;
	TArray<FVector> TangentVerts;
	int32 TrisOrder[6] = { 0, 1, LatitudeSegments + 1, 1, LatitudeSegments + 2, LatitudeSegments + 1 };
	//Baked trigonometric data to avoid computing it too much (sin and cos are expensive !)
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
	//Making the verts
	for (int32 LongitudeIndex = 0; LongitudeIndex < LongitudeSegments + 1; LongitudeIndex++) //This is one more vert than geometrically needed but this avoid having to make wrap-around code
	{
		float angle = LongitudeIndex * PI / LongitudeSegments;
		float z, r;
		FMath::SinCos(&r, &z, angle);
		for (int32 LatitudeIndex = 0; LatitudeIndex < LatitudeSegments + 1; LatitudeIndex++) //In total, we only waste (2*LatitudeSegments + LongitudeSegments - 2) vertices but save LatitudeSegments*LongitudeSegments operations
		{
			FVector Normal = LatitudeVerts[LatitudeIndex] * r + FVector(0, 0, z);
			FVector Position = Normal * SphereRadius;
			MeshData.Positions.Add(Position);
			MeshData.Tangents.Add(Normal, TangentVerts[LatitudeIndex]);
			MeshData.TexCoords.Add(FVector2D((float)LatitudeIndex / LatitudeSegments, (float)LongitudeIndex / LongitudeSegments));
			MeshData.Colors.Add(FColor::White);
		}
	}
	//Creating the tris
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

void URuntimeMeshProviderSphere::UpdateMeshParameters(bool bAffectsCollision)
{
	MaxLOD = GetMaxNumberOfLODs() - 1;

	MarkAllLODsDirty();
	if (bAffectsCollision)
	{
		MarkCollisionDirty();
	}
}
