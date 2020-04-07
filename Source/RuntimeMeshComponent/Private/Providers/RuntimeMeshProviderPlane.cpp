// Copyright 2016-2019 Chris Conway (Koderz). All Rights Reserved.


#include "Providers/RuntimeMeshProviderPlane.h"


FRuntimeMeshProviderPlaneProxy::FRuntimeMeshProviderPlaneProxy(TWeakObjectPtr<URuntimeMeshProvider> InParent)
	: FRuntimeMeshProviderProxy(InParent)
{

}

FRuntimeMeshProviderPlaneProxy::~FRuntimeMeshProviderPlaneProxy()
{

}

void FRuntimeMeshProviderPlaneProxy::UpdateProxyParameters(URuntimeMeshProvider* ParentProvider, bool bIsInitialSetup)
{
	URuntimeMeshProviderPlane* PlaneProvider = Cast<URuntimeMeshProviderPlane>(ParentProvider);
	MarkCollisionDirty();
	if (LocationA != PlaneProvider->LocationA || LocationB != PlaneProvider->LocationB || LocationC != PlaneProvider->LocationC)
	{
		LocationA = PlaneProvider->LocationA;
		LocationB = PlaneProvider->LocationB;
		LocationC = PlaneProvider->LocationC;
		MarkSectionDirty(INDEX_NONE, 0); //Mark all LODs dirty
	}
	bool bHasParameterChanged = false;
	TArray<int32> VertsABBefore = VertsAB, VertsACBefore = VertsAC;
	if (VertsAB != PlaneProvider->VertsAB)
	{
		bHasParameterChanged = true;
		VertsAB = PlaneProvider->VertsAB;
	}
	if (VertsAC != PlaneProvider->VertsAC)
	{
		bHasParameterChanged = true;
		VertsAC = PlaneProvider->VertsAC;
	}
	if (ScreenSize != PlaneProvider->ScreenSize)
	{
		bHasParameterChanged = true;
		ScreenSize = PlaneProvider->ScreenSize;
	}
	Material = PlaneProvider->Material;
	int32 MaxLODBefore = MaxLOD;
	MaxLOD = GetMaximumPossibleLOD();


	if (!bIsInitialSetup && bHasParameterChanged)
	{
		Initialize();
	}
	
}

void FRuntimeMeshProviderPlaneProxy::Initialize()
{
	SetupMaterialSlot(0, FName("Plane Base"), Material.Get());


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


bool FRuntimeMeshProviderPlaneProxy::GetSectionMeshForLOD(int32 LODIndex, int32 SectionId, FRuntimeMeshRenderableMeshData& MeshData)
{
	// We should only ever be queried for section 0
	check(SectionId == 0 && LODIndex <= MaxLOD);
	//UE_LOG(LogTemp, Log, TEXT("Asking for LOD index %i, VertsAB len = %i, VertsAC len = %i"), LODIndex, VertsAB.Num(), VertsAC.Num());
	//return true;
	int32 NumVertsAB = VertsAB[LODIndex], NumVertsAC = VertsAC[LODIndex];
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
			FVector2D TexCoord = FVector2D((float)ABIndex / (float)(NumVertsAB-1), (float)ACIndex / (float)(NumVertsAC-1));
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

FRuntimeMeshCollisionSettings FRuntimeMeshProviderPlaneProxy::GetCollisionSettings()
{
	FRuntimeMeshCollisionSettings Settings;
	Settings.bUseAsyncCooking = false;
	Settings.bUseComplexAsSimple = true;
	   	 
	return Settings;
}

bool FRuntimeMeshProviderPlaneProxy::HasCollisionMesh()
{
	return false;
}

bool FRuntimeMeshProviderPlaneProxy::GetCollisionMesh(FRuntimeMeshCollisionData& CollisionData)
{
	return false;
}

URuntimeMeshProviderPlane::URuntimeMeshProviderPlane()
{
	LocationA = FVector(100, 100, 0);
	LocationB = FVector(-100, 100, 0);
	LocationC = FVector(100, -100, 0);
	VertsAB = TArray<int32>({100,10,1});
	VertsAC = TArray<int32>({100,10,1});
	ScreenSize = TArray<float>({ 0.1,0.01 });
}