// Copyright 2016-2020 TriAxis Games L.L.C. All Rights Reserved.


#include "Providers/RuntimeMeshProviderPlane.h"


URuntimeMeshProviderPlane::URuntimeMeshProviderPlane()
	: LocationA(100, 100, 0)
	, LocationB(-100, 100, 0)
	, LocationC(100, -100, 0)
	, VertsAB({100,10,2})
	, VertsAC({100,10,2})
	, ScreenSize({ 0.1,0.01 })
{
}

void URuntimeMeshProviderPlane::Initialize()
{
	MaxLOD = GetMaximumPossibleLOD();
	
	SetupMaterialSlot(0, FName("Plane Base"), Material);


	TArray<FRuntimeMeshLODProperties> NewLODs;
	for (int32 LODIndex = 0; LODIndex <= MaxLOD; LODIndex++)
	{
		FRuntimeMeshLODProperties LODProperties;
		LODProperties.ScreenSize = LODIndex >= ScreenSize.Num() ? 0.0 : ScreenSize[LODIndex];
		NewLODs.Add(LODProperties);
	}
	ConfigureLODs(NewLODs);


	for (int32 LODIndex = 0; LODIndex <= MaxLOD; LODIndex++)
	{
		FRuntimeMeshSectionProperties Properties;
		Properties.bCastsShadow = true;
		Properties.bIsVisible = true;
		Properties.MaterialSlot = 0;
		Properties.UpdateFrequency = ERuntimeMeshUpdateFrequency::Infrequent;
		Properties.bWants32BitIndices = VertsAB[LODIndex] * VertsAC[LODIndex] > 1 << 16; //Use 32 bit indices if more than 2^16 verts are needed

		CreateSection(LODIndex, 0, Properties);
	}
}

bool URuntimeMeshProviderPlane::GetSectionMeshForLOD(int32 LODIndex, int32 SectionId, FRuntimeMeshRenderableMeshData& MeshData)
{
	FScopeLock Lock(&PropertySyncRoot);

	// We should only ever be queried for section 0
	check(SectionId == 0 && LODIndex <= MaxLOD);

	int32 NumVertsAB = VertsAB[LODIndex];
	int32 NumVertsAC = VertsAC[LODIndex];
	FVector ABDirection = (LocationB - LocationA) / (NumVertsAB - 1);
	FVector ACDirection = (LocationC - LocationA) / (NumVertsAB - 1);
	FVector Normal = (ABDirection ^ ACDirection).GetUnsafeNormal();
	FVector Tangent = ABDirection.GetUnsafeNormal();
	FColor Color = FColor::White;
	for (int32 ACIndex = 0; ACIndex < NumVertsAC; ACIndex++)
	{
		for (int32 ABIndex = 0; ABIndex < NumVertsAB; ABIndex++)
		{
			FVector Location = LocationA + ABDirection * ABIndex + ACDirection * ACIndex;
			FVector2D TexCoord = FVector2D((float)ABIndex / (float)(NumVertsAB - 1), (float)ACIndex / (float)(NumVertsAC - 1));
			//UE_LOG(LogTemp, Log, TEXT("TexCoord for vertex %i:%i : %s"), ABIndex, ACIndex, *TexCoord.ToString());
			MeshData.Positions.Add(Location);
			MeshData.Tangents.Add(Normal, Tangent);
			MeshData.Colors.Add(Color);
			MeshData.TexCoords.Add(TexCoord);
			if (ABIndex != NumVertsAB - 1 && ACIndex != NumVertsAC - 1)
			{
				int32 AIndex = ABIndex + ACIndex * NumVertsAB;
				int32 BIndex = AIndex + 1;
				int32 CIndex = AIndex + NumVertsAB;
				int32 DIndex = CIndex + 1;
				MeshData.Triangles.AddTriangle(AIndex, CIndex, BIndex);
				MeshData.Triangles.AddTriangle(BIndex, CIndex, DIndex);
			}
		}
	}

	return true;
}

FRuntimeMeshCollisionSettings URuntimeMeshProviderPlane::GetCollisionSettings()
{
	FRuntimeMeshCollisionSettings Settings;
	Settings.bUseAsyncCooking = false;
	Settings.bUseComplexAsSimple = true;

	return Settings;
}

FBoxSphereBounds URuntimeMeshProviderPlane::GetBounds()
{
	FScopeLock Lock(&PropertySyncRoot);
	FVector LocationD = LocationB - LocationA + LocationC; // C + BA
	FVector points[4] = { LocationA, LocationB, LocationC, LocationD };
	FBox BoundingBox = FBox(points, 4);
	return FBoxSphereBounds(BoundingBox);
}

bool URuntimeMeshProviderPlane::IsThreadSafe()
{
	return true;
}

int32 URuntimeMeshProviderPlane::GetMaximumPossibleLOD()
{
	return FMath::Min3(VertsAB.Num() - 1, VertsAC.Num() - 1, ScreenSize.Num());
}
