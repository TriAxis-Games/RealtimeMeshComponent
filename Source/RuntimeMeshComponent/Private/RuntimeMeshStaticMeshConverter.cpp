// Copyright 2016-2020 TriAxis Games L.L.C. All Rights Reserved.


#include "RuntimeMeshStaticMeshConverter.h"
#include "RuntimeMeshComponentPlugin.h"
#include "EngineGlobals.h"
#include "Engine/StaticMesh.h"
#include "PhysicsEngine/BodySetup.h"
#include "RuntimeMeshRenderable.h"
#include "RuntimeMeshComponent.h"
#include "Providers/RuntimeMeshProviderStatic.h"


#define RMC_LOG_VERBOSE(MeshId, Format, ...) \
	UE_LOG(RuntimeMeshLog, Verbose, TEXT("[SMC->RMC Mesh:%d Thread:%d]: " Format), MeshId, FPlatformTLS::GetCurrentThreadId(), ##__VA_ARGS__);


int32 URuntimeMeshStaticMeshConverter::CopyVertexOrGetIndex(const FStaticMeshLODResources& LOD, const FStaticMeshSection& Section, TMap<int32, int32>& MeshToSectionVertexMap, int32 VertexIndex, FRuntimeMeshRenderableMeshData& NewMeshData)
{
	int32* FoundIndex = MeshToSectionVertexMap.Find(VertexIndex);
	if (FoundIndex != nullptr)
	{
		return *FoundIndex;
	}
	else
	{
		// Copy Position
		int32 NewVertexIndex = NewMeshData.Positions.Add(LOD.VertexBuffers.PositionVertexBuffer.VertexPosition(VertexIndex));

		// Copy Tangents
		NewMeshData.Tangents.Add(
			LOD.VertexBuffers.StaticMeshVertexBuffer.VertexTangentX(VertexIndex),
			LOD.VertexBuffers.StaticMeshVertexBuffer.VertexTangentY(VertexIndex),
			LOD.VertexBuffers.StaticMeshVertexBuffer.VertexTangentZ(VertexIndex));

		int32 NumTexCoords = LOD.VertexBuffers.StaticMeshVertexBuffer.GetNumTexCoords();

		// Copy UV's
		if (NumTexCoords > 0)
		{
			for (int32 TexIndex = 0; TexIndex < NumTexCoords; TexIndex++)
			{
				NewMeshData.TexCoords.Add(LOD.VertexBuffers.StaticMeshVertexBuffer.GetVertexUV(VertexIndex, TexIndex), TexIndex);
			}
		}
		else
		{
			NewMeshData.TexCoords.Add(FVector2D::ZeroVector);
		}		

		// Copy Color

#if ENGINE_MAJOR_VERSION >= 4 && ENGINE_MINOR_VERSION >= 23
		if (LOD.bHasColorVertexData)
#else
		if (LOD.VertexBuffers.ColorVertexBuffer.GetNumVertices() > 0)
#endif
		{
			NewMeshData.Colors.Add(LOD.VertexBuffers.ColorVertexBuffer.VertexColor(VertexIndex));
		}
		else
		{
			NewMeshData.Colors.Add(FColor::White);
		}

		MeshToSectionVertexMap.Add(VertexIndex, NewVertexIndex);
		return NewVertexIndex;
	}
}

int32 URuntimeMeshStaticMeshConverter::CopyVertexOrGetIndex(const FStaticMeshLODResources& LOD, const FStaticMeshSection& Section, TMap<int32, int32>& MeshToSectionVertexMap, int32 VertexIndex, int32 NumUVChannels, FRuntimeMeshCollisionData& NewMeshData)
{
	int32* FoundIndex = MeshToSectionVertexMap.Find(VertexIndex);
	if (FoundIndex != nullptr)
	{
		return *FoundIndex;
	}
	else
	{
		// Copy Position
		int32 NewVertexIndex = NewMeshData.Vertices.Add(LOD.VertexBuffers.PositionVertexBuffer.VertexPosition(VertexIndex));
		
		// Copy UV's
		for (int32 UVIndex = 0; UVIndex < NumUVChannels; UVIndex++)
		{
			NewMeshData.TexCoords.Add(UVIndex, LOD.VertexBuffers.StaticMeshVertexBuffer.GetVertexUV(VertexIndex, UVIndex));
		}
		
		MeshToSectionVertexMap.Add(VertexIndex, NewVertexIndex);
		return NewVertexIndex;
	}
}

bool URuntimeMeshStaticMeshConverter::CopyStaticMeshSectionToRenderableMeshData(UStaticMesh* StaticMesh, int32 LODIndex, int32 SectionId, FRuntimeMeshRenderableMeshData& OutMeshData)
{
	// Check valid static mesh
	if (StaticMesh == nullptr || StaticMesh->IsPendingKill())
	{
		return false;
	}

	// Check mesh data is accessible
	if (!((GIsEditor || StaticMesh->bAllowCPUAccess) && StaticMesh->RenderData != nullptr))
	{
		return false;
	}

	// Check valid LOD
	if (!StaticMesh->RenderData->LODResources.IsValidIndex(LODIndex))
	{
		return false;
	}

	const FStaticMeshLODResources& LOD = StaticMesh->RenderData->LODResources[LODIndex];

	// Check valid section
	if (!LOD.Sections.IsValidIndex(SectionId))
	{
		return false;
	}

	OutMeshData = FRuntimeMeshRenderableMeshData(
		LOD.VertexBuffers.StaticMeshVertexBuffer.GetUseHighPrecisionTangentBasis(),
		LOD.VertexBuffers.StaticMeshVertexBuffer.GetUseFullPrecisionUVs(),
		LOD.VertexBuffers.StaticMeshVertexBuffer.GetNumTexCoords(),
		LOD.IndexBuffer.Is32Bit());
	
	// Map from vert buffer for whole mesh to vert buffer for section of interest
	TMap<int32, int32> MeshToSectionVertMap;

	const FStaticMeshSection& Section = LOD.Sections[SectionId];
	const uint32 OnePastLastIndex = Section.FirstIndex + Section.NumTriangles * 3;
	FIndexArrayView Indices = LOD.IndexBuffer.GetArrayView();
	
	// Iterate over section index buffer, copying verts as needed
	for (uint32 i = Section.FirstIndex; i < OnePastLastIndex; i++)
	{
		uint32 MeshVertIndex = Indices[i];

		// See if we have this vert already in our section vert buffer, and copy vert in if not 
		int32 SectionVertIndex = CopyVertexOrGetIndex(LOD, Section, MeshToSectionVertMap, MeshVertIndex, OutMeshData);

		// Add to index buffer
		OutMeshData.Triangles.Add(SectionVertIndex);
	}

	// Lets copy the adjacency information too for tessellation 
	// At this point all vertices should be copied so it should work to just copy/convert the indices.

#if ENGINE_MAJOR_VERSION >= 4 && ENGINE_MINOR_VERSION >= 23
	const auto& LODAdjacencyIndexBuffer = LOD.AdditionalIndexBuffers->AdjacencyIndexBuffer;
#else
	const auto& LODAdjacencyIndexBuffer = LOD.AdjacencyIndexBuffer;
#endif

	if (LOD.bHasAdjacencyInfo && LODAdjacencyIndexBuffer.GetNumIndices() > 0)
	{
		FIndexArrayView AdjacencyIndices = LODAdjacencyIndexBuffer.GetArrayView();

		// We multiply these by 4 as the adjacency data is 12 indices per triangle instead of the normal 3
		uint32 StartIndex = Section.FirstIndex * 4;
		uint32 NumIndices = Section.NumTriangles * 3 * 4;

		for (uint32 Index = 0; Index < NumIndices; Index++)
		{
			OutMeshData.AdjacencyTriangles.Add(MeshToSectionVertMap[AdjacencyIndices[StartIndex + Index]]);
		}
	}

	return true;
}

bool URuntimeMeshStaticMeshConverter::CopyStaticMeshCollisionToCollisionSettings(UStaticMesh* StaticMesh, FRuntimeMeshCollisionSettings& OutCollisionSettings)
{
	// Check valid static mesh
	if (StaticMesh == nullptr || StaticMesh->IsPendingKill())
	{
		return false;
	}

	// Check mesh data is accessible
	if (!((GIsEditor || StaticMesh->bAllowCPUAccess) && StaticMesh->RenderData != nullptr))
	{
		return false;
	}

	// Do we have a body setup to copy?
	if (StaticMesh->BodySetup == nullptr)
	{
		return false;
	}

	bool bHadSimple = false;

	// Copy convex elements
	const auto& SourceConvexElems = StaticMesh->BodySetup->AggGeom.ConvexElems;
	for (int32 ConvexIndex = 0; ConvexIndex < SourceConvexElems.Num(); ConvexIndex++)
	{
		bHadSimple = true;
		OutCollisionSettings.ConvexElements.Emplace(
			SourceConvexElems[ConvexIndex].VertexData, 
			SourceConvexElems[ConvexIndex].ElemBox);
	}

	// Copy boxes
	const auto& SourceBoxes = StaticMesh->BodySetup->AggGeom.BoxElems;
	for (int32 BoxIndex = 0; BoxIndex < SourceBoxes.Num(); BoxIndex++)
	{
		bHadSimple = true;
		OutCollisionSettings.Boxes.Emplace(
			SourceBoxes[BoxIndex].Center,
			SourceBoxes[BoxIndex].Rotation,
			SourceBoxes[BoxIndex].X,
			SourceBoxes[BoxIndex].Y,
			SourceBoxes[BoxIndex].Z);
	}

	// Copy spheres
	const auto& SourceSpheres = StaticMesh->BodySetup->AggGeom.SphereElems;
	for (int32 SphereIndex = 0; SphereIndex < SourceSpheres.Num(); SphereIndex++)
	{
		bHadSimple = true;
		OutCollisionSettings.Spheres.Emplace(
			SourceSpheres[SphereIndex].Center, 
			SourceSpheres[SphereIndex].Radius);
	}

	// Copy capsules
	const auto& SourceCapsules = StaticMesh->BodySetup->AggGeom.SphylElems;
	for (int32 CapsuleIndex = 0; CapsuleIndex < SourceCapsules.Num(); CapsuleIndex++)
	{
		bHadSimple = true;
		OutCollisionSettings.Capsules.Emplace(
			SourceCapsules[CapsuleIndex].Center,
			SourceCapsules[CapsuleIndex].Rotation,
			SourceCapsules[CapsuleIndex].Radius,
			SourceCapsules[CapsuleIndex].Length);
	}

	if (bHadSimple)
	{
		OutCollisionSettings.bUseComplexAsSimple = false;
	}

	return true;
}

bool URuntimeMeshStaticMeshConverter::CopyStaticMeshLODToCollisionData(UStaticMesh* StaticMesh, int32 LODIndex, FRuntimeMeshCollisionData& OutCollisionData)
{
	// Check valid static mesh
	if (StaticMesh == nullptr || StaticMesh->IsPendingKill())
	{
		return false;
	}

	// Check mesh data is accessible
	if (!((GIsEditor || StaticMesh->bAllowCPUAccess) && StaticMesh->RenderData != nullptr))
	{
		return false;
	}

	// Check valid LOD
	if (!StaticMesh->RenderData->LODResources.IsValidIndex(LODIndex))
	{
		return false;
	}

	const FStaticMeshLODResources& LOD = StaticMesh->RenderData->LODResources[LODIndex];

	uint32 NumUVChannels = LOD.VertexBuffers.StaticMeshVertexBuffer.GetNumTexCoords();
	
	OutCollisionData.Vertices.Empty();
	OutCollisionData.TexCoords.Empty();
	OutCollisionData.MaterialIndices.Empty();
	OutCollisionData.Triangles.Empty();
	OutCollisionData.TexCoords.SetNumChannels(NumUVChannels);

	// Map from vert buffer for whole mesh to vert buffer for section of interest
	TMap<int32, int32> MeshToSectionVertMap;

	int32 TempIndices[3] = { 0, 0, 0 };

	int32 SectionId = 0;
	for (const FStaticMeshSection& Section : LOD.Sections)
	{
		const uint32 OnePastLastIndex = Section.FirstIndex + Section.NumTriangles;
		FIndexArrayView Indices = LOD.IndexBuffer.GetArrayView();
		
		int32 StartTriangle = OutCollisionData.Triangles.Num();

		// Iterate over section index buffer, copying verts as needed
		for (uint32 TriIndex = Section.FirstIndex; TriIndex < OnePastLastIndex; TriIndex++)
		{
			for (int32 Index = 0; Index < 3; Index++)
			{
				uint32 MeshVertIndex = Indices[TriIndex * 3 + Index];

				// See if we have this vert already in our section vert buffer, and copy vert in if not 
				int32 SectionVertIndex = CopyVertexOrGetIndex(LOD, Section, MeshToSectionVertMap, MeshVertIndex, NumUVChannels, OutCollisionData);

				TempIndices[Index] = MeshVertIndex;
			}

			// Add to index buffer
			OutCollisionData.Triangles.Add(TempIndices[0], TempIndices[1], TempIndices[2]);
			OutCollisionData.MaterialIndices.Add(Section.MaterialIndex);
		}

		// Add the collision section
		OutCollisionData.CollisionSources.Emplace(StartTriangle, OutCollisionData.Triangles.Num() - 1, nullptr, SectionId, ERuntimeMeshCollisionFaceSourceType::Renderable);

		SectionId++;
	}

	return true;
}


bool URuntimeMeshStaticMeshConverter::CopyStaticMeshToRuntimeMesh(UStaticMesh* StaticMesh, URuntimeMeshComponent* RuntimeMeshComponent, int32 CollisionLODIndex, int32 MaxLODToCopy)
{
	URuntimeMeshProviderStatic* StaticProvider = Cast<URuntimeMeshProviderStatic>(RuntimeMeshComponent->GetProvider());
	RMC_LOG_VERBOSE(RuntimeMeshComponent->GetRuntimeMeshId(), "StaticMesh to RuntimeMesh conversion started")

	// Not able to convert to RMC without a static provider
	if (StaticProvider == nullptr)
	{
		RMC_LOG_VERBOSE(RuntimeMeshComponent->GetRuntimeMeshId(), "Unable to convert StaticMesh to RuntimeMesh. No StaticProvider present.");
		return false;
	}


	// Check valid static mesh
	if (StaticMesh == nullptr || StaticMesh->IsPendingKill())
	{
		RMC_LOG_VERBOSE(RuntimeMeshComponent->GetRuntimeMeshId(), "Unable to convert StaticMesh to RuntimeMesh. Invalid source StaticMesh.");
		StaticProvider->ConfigureLODs({ FRuntimeMeshLODProperties() });
		StaticProvider->SetCollisionMesh(FRuntimeMeshCollisionData());
		StaticProvider->SetCollisionSettings(FRuntimeMeshCollisionSettings());
		return false;
	}

	// Check mesh data is accessible
	if (!((GIsEditor || StaticMesh->bAllowCPUAccess) && StaticMesh->RenderData != nullptr))
	{
		RMC_LOG_VERBOSE(RuntimeMeshComponent->GetRuntimeMeshId(), "Unable to convert StaticMesh to RuntimeMesh. Invalid source StaticMesh.");
		StaticProvider->ConfigureLODs({ FRuntimeMeshLODProperties() });
		StaticProvider->SetCollisionMesh(FRuntimeMeshCollisionData());
		StaticProvider->SetCollisionSettings(FRuntimeMeshCollisionSettings());
		return false;
	}

	// Copy materials
	const TArray<FStaticMaterial>& MaterialSlots = StaticMesh->StaticMaterials;
	for (int32 SlotIndex = 0; SlotIndex < MaterialSlots.Num(); SlotIndex++)
	{
		StaticProvider->SetupMaterialSlot(SlotIndex, MaterialSlots[SlotIndex].MaterialSlotName, MaterialSlots[SlotIndex].MaterialInterface);
	}

	const auto& LODResources = StaticMesh->RenderData->LODResources;

	// Setup LODs
	TArray<FRuntimeMeshLODProperties> LODs;
	for (int32 LODIndex = 0; LODIndex < LODResources.Num() && LODIndex <= MaxLODToCopy; LODIndex++)
	{
		FRuntimeMeshLODProperties LODProperties;
		LODProperties.ScreenSize = StaticMesh->RenderData->ScreenSize[LODIndex].Default;

		LODs.Add(LODProperties);
	}
	StaticProvider->ConfigureLODs(LODs);


	// Create all sections for all LODs
	for (int32 LODIndex = 0; LODIndex < LODResources.Num() && LODIndex <= MaxLODToCopy; LODIndex++)
	{
		const auto& LOD = LODResources[LODIndex];

		for (int32 SectionId = 0; SectionId < LOD.Sections.Num(); SectionId++)
		{
			const auto& Section = LOD.Sections[SectionId];

			FRuntimeMeshSectionProperties SectionProperties;

			FRuntimeMeshRenderableMeshData MeshData;
			CopyStaticMeshSectionToRenderableMeshData(StaticMesh, LODIndex, SectionId, MeshData);

			StaticProvider->CreateSection(LODIndex, SectionId, SectionProperties, MeshData);
		}
	}

	FRuntimeMeshCollisionSettings CollisionSettings;
	if (CopyStaticMeshCollisionToCollisionSettings(StaticMesh, CollisionSettings))
	{
		StaticProvider->SetCollisionSettings(CollisionSettings);
	}

	FRuntimeMeshCollisionData CollisionData;
	if (CollisionLODIndex != INDEX_NONE && CopyStaticMeshLODToCollisionData(StaticMesh, CollisionLODIndex, CollisionData))
	{
		StaticProvider->SetCollisionMesh(CollisionData);
	}


	return true;
}

bool URuntimeMeshStaticMeshConverter::CopyStaticMeshComponentToRuntimeMesh(UStaticMeshComponent* StaticMeshComponent, URuntimeMeshComponent* RuntimeMeshComponent, int32 CollisionLODIndex, int32 MaxLODToCopy)
{
	if (StaticMeshComponent)
	{
		return CopyStaticMeshToRuntimeMesh(StaticMeshComponent->GetStaticMesh(), RuntimeMeshComponent, CollisionLODIndex, MaxLODToCopy);
	}
	return false;
}


#undef RMC_LOG_VERBOSE