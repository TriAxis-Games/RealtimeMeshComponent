// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.


#include "RealtimeMeshStaticMeshConverter.h"

#include "MaterialDomain.h"
#include "Materials/Material.h"
#include "RealtimeMeshComponentModule.h"
#include "RealtimeMeshSimple.h"
#include "StaticMeshLODResourcesAdapter.h"
#include "Engine/StaticMesh.h"
#include "Mesh/RealtimeMeshBlueprintMeshBuilder.h"
#include "Core/RealtimeMeshBuilder.h"
#include "RenderProxy/RealtimeMeshNaniteProxyInterface.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#include "PhysicsEngine/BodySetup.h"
#include "Core/RealtimeMeshDataTypes.h"

#if WITH_EDITOR
#include "StaticMeshOperations.h"
#endif

using namespace UE::Geometry;
using namespace RealtimeMesh;

namespace
{
#if WITH_EDITOR
	// Appends one StreamSet's geometry into MeshDescription, resolving each triangle's poly-group
	// value to a target polygon group via ResolvePolygonGroup. Uses conversion-aware readers so
	// packed normals / half-precision UVs / 16-bit indices are all handled correctly. Accumulates
	// the emitted triangle count and flags whether the StreamSet lacked normals (so the caller can
	// have the build recompute them). Shared by the StreamSet and RealtimeMesh export paths.
	void AppendStreamSetToMeshDescription(
		FMeshDescription& MeshDescription,
		const FRealtimeMeshStreamSet& StreamSet,
		const FStreamSetStaticMeshConversionOptions& Options,
		TFunctionRef<FPolygonGroupID(int32 PolyGroupValue)> ResolvePolygonGroup,
		int32& InOutTriangleCount,
		bool& bInOutMissingNormals)
	{
		const FRealtimeMeshStream* PositionStream = StreamSet.Find(FRealtimeMeshStreams::Position);
		const FRealtimeMeshStream* TriangleStream = StreamSet.Find(FRealtimeMeshStreams::Triangles);
		if (!PositionStream || !TriangleStream || PositionStream->Num() == 0 || TriangleStream->Num() == 0)
		{
			return;
		}

		const FRealtimeMeshStream* TangentStream = StreamSet.Find(FRealtimeMeshStreams::Tangents);
		const FRealtimeMeshStream* UVStream = StreamSet.Find(FRealtimeMeshStreams::TexCoords);
		const FRealtimeMeshStream* ColorStream = StreamSet.Find(FRealtimeMeshStreams::Color);
		const FRealtimeMeshStream* PolyGroupStream = StreamSet.Find(FRealtimeMeshStreams::PolyGroups);

		const int32 NumVertices = PositionStream->Num();
		const int32 NumTriangles = TriangleStream->Num();

		const bool bUseTangents = TangentStream && Options.bWantTangents;
		const bool bUseUVs = UVStream && Options.bWantUVs;
		const bool bUseColors = ColorStream && Options.bWantVertexColors;
		const bool bUsePolyGroups = PolyGroupStream && Options.bWantPolyGroups;
		if (!bUseTangents)
		{
			bInOutMissingNormals = true;
		}

		FStaticMeshAttributes Attributes(MeshDescription);
		TVertexAttributesRef<FVector3f> VertexPositions = Attributes.GetVertexPositions();
		TVertexInstanceAttributesRef<FVector3f> InstanceNormals = Attributes.GetVertexInstanceNormals();
		TVertexInstanceAttributesRef<FVector2f> InstanceUVs = Attributes.GetVertexInstanceUVs();
		TVertexInstanceAttributesRef<FVector4f> InstanceColors = Attributes.GetVertexInstanceColors();

		const TRealtimeMeshStreamBuilder<const FVector3f, void> PositionData(*PositionStream);
		const TRealtimeMeshStreamBuilder<const TIndex3<uint32>, void> TriangleData(*TriangleStream);

		// Hoist the optional attribute stream readers out of the per-corner/per-triangle loops so
		// they are constructed once, not per vertex-instance / triangle.
		TOptional<TRealtimeMeshStreamBuilder<const FRealtimeMeshTangentsHighPrecision, void>> TangentData;
		if (bUseTangents) { TangentData.Emplace(*TangentStream); }
		TOptional<TRealtimeMeshStridedStreamBuilder<const FVector2f, void>> UVData;
		if (bUseUVs) { UVData.Emplace(*UVStream, 0); }
		TOptional<TRealtimeMeshStreamBuilder<const FColor, void>> ColorData;
		if (bUseColors) { ColorData.Emplace(*ColorStream); }
		TOptional<TRealtimeMeshStreamBuilder<const int32, void>> PolyGroupData;
		if (bUsePolyGroups) { PolyGroupData.Emplace(*PolyGroupStream); }

		// Reserve up front now that the vertex/triangle counts are known.
		MeshDescription.ReserveNewVertices(NumVertices);
		MeshDescription.ReserveNewVertexInstances(NumTriangles * 3);
		MeshDescription.ReserveNewEdges(NumTriangles * 3);
		MeshDescription.ReserveNewPolygons(NumTriangles);

		// Each StreamSet contributes its own vertices (groups are independent buffer sets).
		TArray<FVertexID> VertexIDs;
		VertexIDs.SetNumUninitialized(NumVertices);
		for (int32 VertIdx = 0; VertIdx < NumVertices; VertIdx++)
		{
			const FVertexID VertexID = MeshDescription.CreateVertex();
			VertexPositions[VertexID] = PositionData.GetValue(VertIdx);
			VertexIDs[VertIdx] = VertexID;
		}

		for (int32 TriIdx = 0; TriIdx < NumTriangles; TriIdx++)
		{
			const TIndex3<uint32> Tri = TriangleData.GetValue(TriIdx);
			if (Tri.V0 >= (uint32)NumVertices || Tri.V1 >= (uint32)NumVertices || Tri.V2 >= (uint32)NumVertices)
			{
				continue;
			}

			const uint32 Corners[3] = { Tri.V0, Tri.V1, Tri.V2 };
			FVertexInstanceID InstanceIDs[3];
			for (int32 Corner = 0; Corner < 3; Corner++)
			{
				const uint32 VertIdx = Corners[Corner];
				const FVertexInstanceID InstanceID = MeshDescription.CreateVertexInstance(VertexIDs[VertIdx]);

				if (bUseTangents)
				{
					InstanceNormals[InstanceID] = TangentData->GetValue(VertIdx).GetNormal();
				}
				else
				{
					InstanceNormals[InstanceID] = FVector3f::UpVector;
				}

				if (bUseUVs)
				{
					InstanceUVs[InstanceID] = UVData->GetValue(VertIdx);
				}
				else
				{
					InstanceUVs[InstanceID] = FVector2f::ZeroVector;
				}

				if (bUseColors)
				{
					InstanceColors[InstanceID] = FVector4f(FLinearColor(ColorData->GetValue(VertIdx)));
				}
				else
				{
					InstanceColors[InstanceID] = FVector4f::One();
				}

				InstanceIDs[Corner] = InstanceID;
			}

			int32 PolyGroupValue = 0;
			if (bUsePolyGroups)
			{
				PolyGroupValue = PolyGroupData->GetValue(TriIdx);
			}

			MeshDescription.CreatePolygon(ResolvePolygonGroup(PolyGroupValue), TArrayView<const FVertexInstanceID>(InstanceIDs, 3));
			InOutTriangleCount++;
		}
	}
#endif // WITH_EDITOR
}

bool URealtimeMeshStaticMeshConverter::CopyStreamSetToStaticMesh(const FRealtimeMeshStreamSet& InStreamSet, UStaticMesh* OutStaticMesh,
	const FStreamSetStaticMeshConversionOptions& Options)
{
#if !WITH_EDITOR
	UE_LOG(LogRealtimeMesh, Warning, TEXT("CopyStreamSetToStaticMesh: static mesh building is only available in editor"));
	return false;
#else
	if (!OutStaticMesh)
	{
		UE_LOG(LogRealtimeMesh, Warning, TEXT("CopyStreamSetToStaticMesh: OutStaticMesh is null"));
		return false;
	}

	const int32 LODIndex = FMath::Max(0, Options.LODIndex);
	if (OutStaticMesh->GetNumSourceModels() <= LODIndex)
	{
		OutStaticMesh->SetNumSourceModels(LODIndex + 1);
	}

	FMeshDescription* MeshDescription = OutStaticMesh->CreateMeshDescription(LODIndex);
	if (!MeshDescription)
	{
		UE_LOG(LogRealtimeMesh, Warning, TEXT("CopyStreamSetToStaticMesh: failed to create mesh description for LOD %d"), LODIndex);
		return false;
	}
	FStaticMeshAttributes Attributes(*MeshDescription);
	Attributes.Register();
	TPolygonGroupAttributesRef<FName> PolyGroupSlotNames = Attributes.GetPolygonGroupMaterialSlotNames();

	// One polygon group per distinct poly-group value, named "Material_<n>" so the build matches it
	// to a static material slot (each poly group becomes its own section/material).
	TMap<int32, FPolygonGroupID> PolyGroupValueToID;
	auto ResolvePolygonGroup = [&](int32 PolyGroupValue) -> FPolygonGroupID
	{
		if (const FPolygonGroupID* Existing = PolyGroupValueToID.Find(PolyGroupValue))
		{
			return *Existing;
		}
		const FPolygonGroupID NewID = MeshDescription->CreatePolygonGroup();
		PolyGroupSlotNames[NewID] = *FString::Printf(TEXT("Material_%d"), PolyGroupValue);
		PolyGroupValueToID.Add(PolyGroupValue, NewID);
		return NewID;
	};

	int32 TriangleCount = 0;
	bool bMissingNormals = false;
	AppendStreamSetToMeshDescription(*MeshDescription, InStreamSet, Options, ResolvePolygonGroup, TriangleCount, bMissingNormals);

	if (TriangleCount == 0)
	{
		UE_LOG(LogRealtimeMesh, Warning, TEXT("CopyStreamSetToStaticMesh: no triangles produced (missing position/triangle streams?)"));
		return false;
	}

	// Give the static mesh a material slot per polygon group (matching names) if it has none yet.
	if (OutStaticMesh->GetStaticMaterials().Num() == 0)
	{
		TArray<int32> PolyGroupValues;
		PolyGroupValueToID.GetKeys(PolyGroupValues);
		PolyGroupValues.Sort();
		for (const int32 PolyGroupValue : PolyGroupValues)
		{
			FStaticMaterial& StaticMaterial = OutStaticMesh->GetStaticMaterials().AddDefaulted_GetRef();
			StaticMaterial.MaterialInterface = UMaterial::GetDefaultMaterial(MD_Surface);
			StaticMaterial.MaterialSlotName = *FString::Printf(TEXT("Material_%d"), PolyGroupValue);
			StaticMaterial.UVChannelData = FMeshUVChannelInfo(1.0f);
		}
	}

	// Keep authored normals unless they were absent; let the build derive a tangent basis.
	FStaticMeshSourceModel& SourceModel = OutStaticMesh->GetSourceModel(LODIndex);
	SourceModel.BuildSettings.bRecomputeNormals = bMissingNormals;
	SourceModel.BuildSettings.bRecomputeTangents = true;
	SourceModel.BuildSettings.bRemoveDegenerates = false;
	SourceModel.BuildSettings.bUseHighPrecisionTangentBasis = false;
	SourceModel.BuildSettings.bUseFullPrecisionUVs = false;

	OutStaticMesh->CommitMeshDescription(LODIndex);
	OutStaticMesh->Build();
	OutStaticMesh->PostEditChange();
	OutStaticMesh->MarkPackageDirty();

	return true;
#endif // WITH_EDITOR
}

bool URealtimeMeshStaticMeshConverter::CopyStreamSetFromStaticMesh(const UStaticMesh* InStaticMesh, FRealtimeMeshStreamSet& OutStreamSet,
	const FStreamSetStaticMeshConversionOptions& Options)
{
	OutStreamSet = FRealtimeMeshStreamSet();
	if (InStaticMesh == nullptr)
	{
		UE_LOG(LogRealtimeMesh, Warning, TEXT("RealtimeMeshWarning: CopyFromStaticMesh failed: InStaticMesh is null"));
		return false;
	}

#if WITH_EDITOR
	// Prefer the editor source MeshDescription when it's available — it's higher fidelity than the
	// cooked render data (which has been through build-time vertex splitting / optimization).
	const int32 SourceLODIndex = FMath::Clamp(Options.LODIndex, 0, FMath::Max(0, InStaticMesh->GetNumSourceModels() - 1));
	if (InStaticMesh->IsMeshDescriptionValid(SourceLODIndex))
	{
		return CopyStreamSetFromStaticMesh_SourceData(InStaticMesh, OutStreamSet, Options);
	}
#endif

	return CopyStreamSetFromStaticMesh_RenderData(InStaticMesh, OutStreamSet, Options);
}


URealtimeMeshStreamSet* URealtimeMeshStaticMeshConverter::CopyStreamSetFromStaticMesh(UStaticMesh* FromStaticMeshAsset, URealtimeMeshStreamSet* ToStreamSet,
	FStreamSetStaticMeshConversionOptions Options, ERealtimeMeshOutcomePins& Outcome)
{
	if (FromStaticMeshAsset == nullptr)
	{
		UE_LOG(LogRealtimeMesh, Warning, TEXT("RealtimeMeshWarning: CopyFromStaticMesh failed: FromStaticMeshAsset is null"));
		Outcome = ERealtimeMeshOutcomePins::Failure;
		return ToStreamSet;
	}

	if (ToStreamSet == nullptr)
	{
		UE_LOG(LogRealtimeMesh, Warning, TEXT("RealtimeMeshWarning: CopyFromStaticMesh failed: ToStreamSet is null"));
		Outcome = ERealtimeMeshOutcomePins::Failure;
		return ToStreamSet;
	}

	const bool bSuccess = CopyStreamSetFromStaticMesh(FromStaticMeshAsset, ToStreamSet->GetStreamSet(), Options);

	Outcome = bSuccess? ERealtimeMeshOutcomePins::Success : ERealtimeMeshOutcomePins::Failure;
	return ToStreamSet;
}

UStaticMesh* URealtimeMeshStaticMeshConverter::CopyStreamSetToStaticMesh(URealtimeMeshStreamSet* FromStreamSet, UStaticMesh* ToStaticMeshAsset,
	FStreamSetStaticMeshConversionOptions Options, ERealtimeMeshOutcomePins& Outcome)
{
	if (FromStreamSet == nullptr)
	{
		UE_LOG(LogRealtimeMesh, Warning, TEXT("RealtimeMeshWarning: CopyToStaticMesh failed: FromStreamSet is null"));
		Outcome = ERealtimeMeshOutcomePins::Failure;
		return ToStaticMeshAsset;
	}

	if (ToStaticMeshAsset == nullptr)
	{
		UE_LOG(LogRealtimeMesh, Warning, TEXT("RealtimeMeshWarning: CopyToStaticMesh failed: ToStaticMeshAsset is null"));
		Outcome = ERealtimeMeshOutcomePins::Failure;
		return ToStaticMeshAsset;
	}
	
	const bool bSuccess = CopyStreamSetToStaticMesh(FromStreamSet->GetStreamSet(), ToStaticMeshAsset, Options);

	Outcome = bSuccess? ERealtimeMeshOutcomePins::Success : ERealtimeMeshOutcomePins::Failure;
	return ToStaticMeshAsset;
}

URealtimeMeshSimple* URealtimeMeshStaticMeshConverter::CopyRealtimeMeshFromStaticMesh(UStaticMesh* FromStaticMeshAsset, URealtimeMeshSimple* ToRealtimeMesh,
	FRealtimeMeshStaticMeshConversionOptions Options, ERealtimeMeshOutcomePins& Outcome)
{
	if (FromStaticMeshAsset == nullptr)
	{
		UE_LOG(LogRealtimeMesh, Warning, TEXT("RealtimeMeshWarning: CopyFromStaticMesh failed: FromStaticMeshAsset is null"));
		Outcome = ERealtimeMeshOutcomePins::Failure;
		return ToRealtimeMesh;
	}

	if (ToRealtimeMesh == nullptr)
	{
		UE_LOG(LogRealtimeMesh, Warning, TEXT("RealtimeMeshWarning: CopyFromStaticMesh failed: ToRealtimeMesh is null"));
		Outcome = ERealtimeMeshOutcomePins::Failure;
		return ToRealtimeMesh;
	}

	// Grab the materials
	if (Options.bWantsMaterials)
	{
		const auto& Materials = FromStaticMeshAsset->GetStaticMaterials();
		for (int32 MatID = 0; MatID < Materials.Num(); MatID++)
		{
			ToRealtimeMesh->SetupMaterialSlot(MatID, Materials[MatID].MaterialSlotName, Materials[MatID].MaterialInterface);
		}		
	}

	// Grab all the LODs
	const int32 MinLOD = FMath::Clamp(Options.MinLODIndex, 0, FromStaticMeshAsset->GetNumLODs() - 1);
	const int32 MaxLOD = FMath::Clamp(Options.MaxLODIndex, 0, FromStaticMeshAsset->GetNumLODs() - 1);
	
	for (int32 LODIndex = MinLOD; LODIndex <= MaxLOD; LODIndex++)
	{
		FStreamSetStaticMeshConversionOptions SectionOptions;
		//SectionOptions.LODType = Options.LODType;
		SectionOptions.LODIndex = LODIndex;
		SectionOptions.bWantTangents = Options.bWantTangents;
		SectionOptions.bWantUVs = Options.bWantUVs;
		SectionOptions.bWantVertexColors = Options.bWantVertexColors;
		SectionOptions.bWantPolyGroups = Options.bWantPolyGroups;
		
		FRealtimeMeshStreamSet Streams;

		if (!CopyStreamSetFromStaticMesh(FromStaticMeshAsset, Streams, SectionOptions))
		{
			UE_LOG(LogRealtimeMesh, Warning, TEXT("RealtimeMeshWarning: CopyFromStaticMesh failed: Failed to copy LOD %d"), LODIndex);
			ToRealtimeMesh->Reset();
			return nullptr;
		}

		const FRealtimeMeshLODKey LODKey = LODIndex > MinLOD? ToRealtimeMesh->AddLOD(FRealtimeMeshLODConfig()) : FRealtimeMeshLODKey(0);
		const FRealtimeMeshBufferSetKey SectionGroupKey = FRealtimeMeshBufferSetKey::Create(LODKey, "Default");

		if (FromStaticMeshAsset->GetRenderData())
		{
			ToRealtimeMesh->UpdateLODConfig(LODKey, FRealtimeMeshLODConfig(FromStaticMeshAsset->GetRenderData()->ScreenSize[LODIndex].GetValue()));
		}
		ToRealtimeMesh->CreateBufferSet(SectionGroupKey, MoveTemp(Streams));
	}

	// Copy distance field if requested. DistanceFieldData is null when the asset was built
	// without 'Generate Mesh Distance Fields' (project or per-asset setting).
	if (Options.bWantsDistanceField && FromStaticMeshAsset->GetRenderData() && FromStaticMeshAsset->GetRenderData()->LODResources.IsValidIndex(0)
		&& FromStaticMeshAsset->GetRenderData()->LODResources[0].DistanceFieldData)
	{
		// Grab a copy of the data
		FRealtimeMeshDistanceField DistancField(*FromStaticMeshAsset->GetRenderData()->LODResources[0].DistanceFieldData);
		ToRealtimeMesh->SetDistanceField(MoveTemp(DistancField));
	}

	// Copy card representation if requested
	if (Options.bWantsLumenCards && FromStaticMeshAsset->GetRenderData() && FromStaticMeshAsset->GetRenderData()->LODResources.IsValidIndex(0))
	{
		// Grab a copy of the data
		if (FromStaticMeshAsset->GetRenderData()->LODResources[0].CardRepresentationData)
		{
			FRealtimeMeshCardRepresentation CardRepresentation(*FromStaticMeshAsset->GetRenderData()->LODResources[0].CardRepresentationData);
			ToRealtimeMesh->SetCardRepresentation(MoveTemp(CardRepresentation));			
		}
	}

	Outcome = ERealtimeMeshOutcomePins::Success;
	return ToRealtimeMesh;
}

UStaticMesh* URealtimeMeshStaticMeshConverter::CopyRealtimeMeshToStaticMesh(URealtimeMeshSimple* FromRealtimeMesh, UStaticMesh* ToStaticMeshAsset,
	FRealtimeMeshStaticMeshConversionOptions Options, ERealtimeMeshOutcomePins& Outcome)
{
#if !WITH_EDITOR
	UE_LOG(LogRealtimeMesh, Warning, TEXT("CopyRealtimeMeshToStaticMesh: Static mesh building is only available in editor"));
	Outcome = ERealtimeMeshOutcomePins::Failure;
	return ToStaticMeshAsset;
#else
	if (FromRealtimeMesh == nullptr)
	{
		UE_LOG(LogRealtimeMesh, Warning, TEXT("CopyRealtimeMeshToStaticMesh: FromRealtimeMesh is null"));
		Outcome = ERealtimeMeshOutcomePins::Failure;
		return ToStaticMeshAsset;
	}

	if (ToStaticMeshAsset == nullptr)
	{
		UE_LOG(LogRealtimeMesh, Warning, TEXT("CopyRealtimeMeshToStaticMesh: ToStaticMeshAsset is null"));
		Outcome = ERealtimeMeshOutcomePins::Failure;
		return ToStaticMeshAsset;
	}

	// --- Materials: one static material slot per RealtimeMesh material slot ---
	ToStaticMeshAsset->GetStaticMaterials().Empty();
	if (Options.bWantsMaterials)
	{
		const int32 NumMaterials = FromRealtimeMesh->GetNumMaterials();
		for (int32 MatIdx = 0; MatIdx < NumMaterials; MatIdx++)
		{
			FStaticMaterial& StaticMaterial = ToStaticMeshAsset->GetStaticMaterials().AddDefaulted_GetRef();
			StaticMaterial.MaterialInterface = FromRealtimeMesh->GetMaterial(MatIdx);
			// Section->material matching is by slot NAME, so a slot must have a non-None name.
			FName SlotName = FromRealtimeMesh->GetMaterialSlotName(MatIdx);
			if (SlotName.IsNone())
			{
				SlotName = *FString::Printf(TEXT("Material_%d"), MatIdx);
			}
			StaticMaterial.MaterialSlotName = SlotName;
			StaticMaterial.UVChannelData = FMeshUVChannelInfo(1.0f);
		}
	}
	// Always have at least one slot so polygons have somewhere to live.
	if (ToStaticMeshAsset->GetStaticMaterials().Num() == 0)
	{
		FStaticMaterial& StaticMaterial = ToStaticMeshAsset->GetStaticMaterials().AddDefaulted_GetRef();
		StaticMaterial.MaterialInterface = UMaterial::GetDefaultMaterial(MD_Surface);
		StaticMaterial.MaterialSlotName = FName("Material_0");
		StaticMaterial.UVChannelData = FMeshUVChannelInfo(1.0f);
	}
	const int32 NumSlots = ToStaticMeshAsset->GetStaticMaterials().Num();

	// LOD 0 section groups.
	const TArray<FRealtimeMeshLODKey> LODs = FromRealtimeMesh->GetLODs();
	if (LODs.Num() == 0)
	{
		UE_LOG(LogRealtimeMesh, Warning, TEXT("CopyRealtimeMeshToStaticMesh: No LODs found"));
		Outcome = ERealtimeMeshOutcomePins::Failure;
		return ToStaticMeshAsset;
	}
	const TArray<FRealtimeMeshBufferSetKey> SectionGroups = FromRealtimeMesh->GetBufferSets(LODs[0]);
	if (SectionGroups.Num() == 0)
	{
		UE_LOG(LogRealtimeMesh, Warning, TEXT("CopyRealtimeMeshToStaticMesh: No section groups in LOD 0"));
		Outcome = ERealtimeMeshOutcomePins::Failure;
		return ToStaticMeshAsset;
	}

	// --- Build a single LOD0 MeshDescription spanning every section group ---
	ToStaticMeshAsset->SetNumSourceModels(1);

	FMeshDescription* MeshDescription = ToStaticMeshAsset->CreateMeshDescription(0);
	FStaticMeshAttributes Attributes(*MeshDescription);
	Attributes.Register();
	TPolygonGroupAttributesRef<FName> PolyGroupSlotNames = Attributes.GetPolygonGroupMaterialSlotNames();

	// One polygon group per material slot, tagged with the slot name so the build matches it to
	// the corresponding StaticMaterial (i.e. each material becomes its own static-mesh section).
	TArray<FPolygonGroupID> SlotToPolyGroup;
	SlotToPolyGroup.Reserve(NumSlots);
	for (int32 Slot = 0; Slot < NumSlots; Slot++)
	{
		const FPolygonGroupID PolyGroupID = MeshDescription->CreatePolygonGroup();
		PolyGroupSlotNames[PolyGroupID] = ToStaticMeshAsset->GetStaticMaterials()[Slot].MaterialSlotName;
		SlotToPolyGroup.Add(PolyGroupID);
	}

	int32 TotalTriangles = 0;
	bool bAnyGroupMissingNormals = false;

	for (const FRealtimeMeshBufferSetKey& GroupKey : SectionGroups)
	{
		// Map this group's poly-group indices -> material slots (from each section's config).
		// Section keys are named "Section_PolyGroup_<N>"; a singular-section group uses N=0.
		TMap<int32, int32> PolyGroupToSlot;
		for (const FRealtimeMeshSectionKey& SectionKey : FromRealtimeMesh->GetSectionsInBufferSet(GroupKey))
		{
			int32 PolyGroupIndex = 0;
			FString Trailing;
			if (SectionKey.Name().ToString().Split(TEXT("Section_PolyGroup_"), nullptr, &Trailing))
			{
				PolyGroupIndex = FCString::Atoi(*Trailing);
			}
			PolyGroupToSlot.Add(PolyGroupIndex, FromRealtimeMesh->GetSectionConfig(SectionKey).MaterialSlot);
		}

		FStreamSetStaticMeshConversionOptions StreamOptions;
		StreamOptions.LODIndex = 0;
		StreamOptions.bWantTangents = Options.bWantTangents;
		StreamOptions.bWantUVs = Options.bWantUVs;
		StreamOptions.bWantVertexColors = Options.bWantVertexColors;
		StreamOptions.bWantPolyGroups = Options.bWantPolyGroups;

		FromRealtimeMesh->ProcessMesh(GroupKey, [&](const FRealtimeMeshStreamSet& StreamSet)
		{
			AppendStreamSetToMeshDescription(*MeshDescription, StreamSet, StreamOptions,
				[&](int32 PolyGroupValue) -> FPolygonGroupID
				{
					const int32 Slot = FMath::Clamp(PolyGroupToSlot.FindRef(PolyGroupValue), 0, NumSlots - 1);
					return SlotToPolyGroup[Slot];
				},
				TotalTriangles, bAnyGroupMissingNormals);
		});
	}

	if (TotalTriangles == 0)
	{
		UE_LOG(LogRealtimeMesh, Warning, TEXT("CopyRealtimeMeshToStaticMesh: No triangles were produced"));
		Outcome = ERealtimeMeshOutcomePins::Failure;
		return ToStaticMeshAsset;
	}

	// Keep our authored normals (unless a group lacked them); let the build derive a tangent basis.
	FStaticMeshSourceModel& SourceModel = ToStaticMeshAsset->GetSourceModel(0);
	SourceModel.BuildSettings.bRecomputeNormals = bAnyGroupMissingNormals;
	SourceModel.BuildSettings.bRecomputeTangents = true;
	SourceModel.BuildSettings.bRemoveDegenerates = false;
	SourceModel.BuildSettings.bUseHighPrecisionTangentBasis = false;
	SourceModel.BuildSettings.bUseFullPrecisionUVs = false;

	ToStaticMeshAsset->CommitMeshDescription(0);

	// Copy collision if available.
	if (UBodySetup* BodySetup = FromRealtimeMesh->GetBodySetup())
	{
		if (!ToStaticMeshAsset->GetBodySetup())
		{
			ToStaticMeshAsset->CreateBodySetup();
		}
		if (UBodySetup* StaticMeshBodySetup = ToStaticMeshAsset->GetBodySetup())
		{
			StaticMeshBodySetup->CopyBodyPropertiesFrom(BodySetup);
			StaticMeshBodySetup->AggGeom = BodySetup->AggGeom;
		}
	}

	ToStaticMeshAsset->Build();
	ToStaticMeshAsset->PostEditChange();
	ToStaticMeshAsset->MarkPackageDirty();

	UE_LOG(LogRealtimeMesh, Log, TEXT("CopyRealtimeMeshToStaticMesh: built static mesh with %d triangles across %d section group(s), %d material slot(s)"),
		TotalTriangles, SectionGroups.Num(), NumSlots);

	Outcome = ERealtimeMeshOutcomePins::Success;
	return ToStaticMeshAsset;
#endif // WITH_EDITOR
}




bool URealtimeMeshStaticMeshConverter::CopyStreamSetFromStaticMesh_RenderData(const UStaticMesh* InStaticMesh, FRealtimeMeshStreamSet& OutStreamSet,
                                                                            const FStreamSetStaticMeshConversionOptions& Options)
{
	OutStreamSet = FRealtimeMeshStreamSet();

	// NOTE: outside the editor this relies on StaticMesh CPU access; on a dedicated server
	// render data may be stripped, so this path is not validated for that configuration (the
	// bAllowCPUAccess check below is the only guard).
#if !WITH_EDITOR
	if (InStaticMesh->bAllowCPUAccess == false)
	{
		UE_LOG(LogRealtimeMesh, Warning, TEXT("RealtimeMeshWarning: CopyFromStaticMesh failed: StaticMesh bAllowCPUAccess must be set to true to read mesh data at Runtime"));
		return false;
	}
#endif

	const int32 UseLODIndex = FMath::Clamp(Options.LODIndex, 0, InStaticMesh->GetNumLODs() - 1);

	const FStaticMeshLODResources* LODResources = nullptr;
	if (const FStaticMeshRenderData* RenderData = InStaticMesh->GetRenderData())
	{
		LODResources = &RenderData->LODResources[UseLODIndex];
	}
	if (LODResources == nullptr)
	{
		UE_LOG(LogRealtimeMesh, Warning, TEXT("RealtimeMeshWarning: CopyFromStaticMesh failed: Request LOD is not available"));
		return false;
	}
	
#if WITH_EDITOR
	// respect BuildScale build setting
	const FMeshBuildSettings& LODBuildSettings = InStaticMesh->GetSourceModel(UseLODIndex).BuildSettings;
	const FVector3d BuildScale = LODBuildSettings.BuildScale3D;
#else
	const FVector3d BuildScale = FVector3d::One();	
#endif

	TRealtimeMeshStreamBuilder<FVector3f> PositionData(OutStreamSet.AddStream(FRealtimeMeshStreams::Position, GetRealtimeMeshBufferLayout<FVector3f>()));
	TRealtimeMeshStreamBuilder<TIndex3<uint32>> TriangleData(OutStreamSet.AddStream(FRealtimeMeshStreams::Triangles,
		GetRealtimeMeshBufferLayout<TIndex3<uint32>>()));

	FStaticMeshLODResourcesMeshAdapter Adapter(LODResources);
	Adapter.SetBuildScale(BuildScale, false);

	// Copy vertices. LODMesh is dense so this should be 1-1
	const int32 VertexCount = Adapter.VertexCount();
	PositionData.SetNumUninitialized(VertexCount);
	for (int32 VertID = 0; VertID < VertexCount; VertID++)
	{
		const FVector3f Position = static_cast<FVector3f>(Adapter.GetVertex(VertID));
		PositionData.Set(VertID, Position);
	}

	// Copy triangles. LODMesh is dense so this should be 1-1 unless there is a duplicate tri or non-manifold edge (currently aborting in that case)
	const int32 TriangleCount = Adapter.TriangleCount();
	TriangleData.SetNumUninitialized(TriangleCount);
	for (int32 TriID = 0; TriID < TriangleCount; TriID++)
	{
		const FIndex3i Tri = Adapter.GetTriangle(TriID);
		TriangleData.Set(TriID, TIndex3<uint32>(Tri.A, Tri.B, Tri.C));
	}
	
	// transfer sections to PolyGroups
	if (Options.bWantPolyGroups)
	{
		TRealtimeMeshStreamBuilder<uint16> PolyGroupData(OutStreamSet.AddStream(FRealtimeMeshStreams::PolyGroups,
		GetRealtimeMeshBufferLayout<uint16>()));
		PolyGroupData.SetNumUninitialized(TriangleCount);

		for (int32 SectionIdx = 0; SectionIdx < LODResources->Sections.Num(); SectionIdx++)
		{
			const FStaticMeshSection& Section = LODResources->Sections[SectionIdx];
			for (uint32 TriIdx = 0; TriIdx < Section.NumTriangles; TriIdx++)
			{
				const uint32 TriangleID = Section.FirstIndex / 3 + TriIdx;
				PolyGroupData.Set(TriangleID, SectionIdx);
			}
		}
	}

	// copy tangents
	if (Adapter.HasNormals() && Options.bWantTangents)
	{
		TRealtimeMeshStreamBuilder<TRealtimeMeshTangents<FVector4f>, TRealtimeMeshTangents<FPackedNormal>> TangentData(OutStreamSet.AddStream(FRealtimeMeshStreams::Tangents,
		GetRealtimeMeshBufferLayout<TRealtimeMeshTangents<FPackedNormal>>()));
		TangentData.SetNumUninitialized(Adapter.VertexCount());
		
		for (int32 VertID = 0; VertID < VertexCount; VertID++)
		{
			const FVector3f N = Adapter.GetNormal(VertID);
			const FVector3f T = Adapter.GetTangentX(VertID);
			const FVector3f B = Adapter.GetTangentY(VertID);			
			TangentData.Set(VertID, TRealtimeMeshTangents<FVector4f>(N, B, T));
		}
	}

	// copy UV layers
	if (Adapter.HasUVs() && Options.bWantUVs && Adapter.NumUVLayers() > 0)
	{
		const int32 NumUVLayers = Adapter.NumUVLayers();

		FRealtimeMeshStream& TexCoordStream = OutStreamSet.AddStream(FRealtimeMeshStreams::TexCoords, GetRealtimeMeshBufferLayout<FVector2f>(NumUVLayers));
		TexCoordStream.SetNumUninitialized(Adapter.VertexCount());
		
		for (int32 UVLayerIndex = 0; UVLayerIndex < NumUVLayers; UVLayerIndex++)
		{
			TRealtimeMeshStridedStreamBuilder<FVector2f> TexCoordData(TexCoordStream, UVLayerIndex);
			for (int32 VertID = 0; VertID < VertexCount; VertID++)
			{
				const FVector2f UV = Adapter.GetUV(VertID, UVLayerIndex);
				TexCoordData.Set(VertID, UV);
			}
		}
	}

	// copy colors
	if ( Adapter.HasColors() && Options.bWantVertexColors )
	{
		TRealtimeMeshStreamBuilder<FColor> ColorData(OutStreamSet.AddStream(FRealtimeMeshStreams::Color,
		GetRealtimeMeshBufferLayout<FColor>()));
		ColorData.SetNumUninitialized(Adapter.VertexCount());
		for (int32 VertID = 0; VertID < VertexCount; VertID++)
		{
			const FColor Color = Adapter.GetColor(VertID);
			ColorData.Set(VertID, Color);
		}
	}

	return true;
}

bool URealtimeMeshStaticMeshConverter::CopyStreamSetFromStaticMesh_SourceData(const UStaticMesh* InStaticMesh, FRealtimeMeshStreamSet& OutStreamSet,
                                                                             const FStreamSetStaticMeshConversionOptions& Options)
{
	OutStreamSet = FRealtimeMeshStreamSet();

#if !WITH_EDITOR
	UE_LOG(LogRealtimeMesh, Warning, TEXT("CopyStreamSetFromStaticMesh_SourceData: source mesh data is only available in editor"));
	return false;
#else
	const int32 UseLODIndex = FMath::Clamp(Options.LODIndex, 0, FMath::Max(0, InStaticMesh->GetNumSourceModels() - 1));

	const FMeshDescription* SourceMesh = InStaticMesh->GetMeshDescription(UseLODIndex);
	if (SourceMesh == nullptr)
	{
		UE_LOG(LogRealtimeMesh, Warning, TEXT("CopyStreamSetFromStaticMesh_SourceData: LOD %d has no source MeshDescription"), UseLODIndex);
		return false;
	}

	const FStaticMeshSourceModel& SourceModel = InStaticMesh->GetSourceModel(UseLODIndex);
	const FMeshBuildSettings& BuildSettings = SourceModel.BuildSettings;

	// Bake normals/tangents per the LOD build settings so the result matches what the static mesh
	// build would produce, recomputing into a local copy only when those settings actually demand it.
	const bool bNeedsRecompute = BuildSettings.bRecomputeNormals || (BuildSettings.bRecomputeTangents && Options.bWantTangents);
	FMeshDescription LocalSourceMeshCopy;
	if (bNeedsRecompute)
	{
		LocalSourceMeshCopy = *SourceMesh;

		FStaticMeshAttributes LocalAttributes(LocalSourceMeshCopy);
		if (!LocalAttributes.GetTriangleNormals().IsValid() || !LocalAttributes.GetTriangleTangents().IsValid())
		{
			FStaticMeshOperations::ComputeTriangleTangentsAndNormals(LocalSourceMeshCopy);
		}

		EComputeNTBsFlags ComputeFlags = EComputeNTBsFlags::BlendOverlappingNormals;
		ComputeFlags |= BuildSettings.bRecomputeNormals ? EComputeNTBsFlags::Normals : EComputeNTBsFlags::None;
		if (Options.bWantTangents)
		{
			ComputeFlags |= BuildSettings.bRecomputeTangents ? EComputeNTBsFlags::Tangents : EComputeNTBsFlags::None;
			ComputeFlags |= BuildSettings.bUseMikkTSpace ? EComputeNTBsFlags::UseMikkTSpace : EComputeNTBsFlags::None;
		}
		ComputeFlags |= BuildSettings.bComputeWeightedNormals ? EComputeNTBsFlags::WeightedNTBs : EComputeNTBsFlags::None;

		FStaticMeshOperations::ComputeTangentsAndNormals(LocalSourceMeshCopy, ComputeFlags);
		SourceMesh = &LocalSourceMeshCopy;
	}

	const FStaticMeshConstAttributes Attributes(*SourceMesh);
	const TVertexAttributesConstRef<FVector3f> VertexPositions = Attributes.GetVertexPositions();
	const TVertexInstanceAttributesConstRef<FVector3f> InstanceNormals = Attributes.GetVertexInstanceNormals();
	const TVertexInstanceAttributesConstRef<FVector3f> InstanceTangents = Attributes.GetVertexInstanceTangents();
	const TVertexInstanceAttributesConstRef<float> InstanceBinormalSigns = Attributes.GetVertexInstanceBinormalSigns();
	const TVertexInstanceAttributesConstRef<FVector4f> InstanceColors = Attributes.GetVertexInstanceColors();
	const TVertexInstanceAttributesConstRef<FVector2f> InstanceUVs = Attributes.GetVertexInstanceUVs();

	const bool bHasTangents = Options.bWantTangents && InstanceNormals.IsValid() && InstanceTangents.IsValid() && InstanceBinormalSigns.IsValid();
	const bool bHasColors = Options.bWantVertexColors && InstanceColors.IsValid();
	const int32 NumUVLayers = (Options.bWantUVs && InstanceUVs.IsValid()) ? InstanceUVs.GetNumChannels() : 0;

	const FVector3f BuildScale = FVector3f(BuildSettings.BuildScale3D);

	// A StreamSet is per-vertex with indexed triangles, while a MeshDescription stores attributes
	// per vertex-INSTANCE (a corner). Treat each vertex instance as one StreamSet vertex (instances
	// are already shared/split exactly where attributes differ), and remap triangle corners to them.
	TArray<FVertexInstanceID> Instances;
	Instances.Reserve(SourceMesh->VertexInstances().Num());
	TMap<FVertexInstanceID, int32> InstanceToIndex;
	InstanceToIndex.Reserve(SourceMesh->VertexInstances().Num());
	for (const FVertexInstanceID InstanceID : SourceMesh->VertexInstances().GetElementIDs())
	{
		InstanceToIndex.Add(InstanceID, Instances.Num());
		Instances.Add(InstanceID);
	}

	const int32 NumVerts = Instances.Num();
	const int32 NumTris = SourceMesh->Triangles().Num();
	if (NumVerts == 0 || NumTris == 0)
	{
		UE_LOG(LogRealtimeMesh, Warning, TEXT("CopyStreamSetFromStaticMesh_SourceData: LOD %d source mesh is empty"), UseLODIndex);
		return false;
	}

	// Dense polygon-group index (matches material slot order on the static mesh).
	TMap<FPolygonGroupID, int32> PolyGroupToIndex;
	for (const FPolygonGroupID PolygonGroupID : SourceMesh->PolygonGroups().GetElementIDs())
	{
		PolyGroupToIndex.Add(PolygonGroupID, PolyGroupToIndex.Num());
	}

	// --- Per-vertex streams ---
	TRealtimeMeshStreamBuilder<FVector3f> PositionData(OutStreamSet.AddStream(FRealtimeMeshStreams::Position, GetRealtimeMeshBufferLayout<FVector3f>()));
	PositionData.SetNumUninitialized(NumVerts);
	for (int32 VertID = 0; VertID < NumVerts; VertID++)
	{
		const FVertexID Vertex = SourceMesh->GetVertexInstanceVertex(Instances[VertID]);
		PositionData.Set(VertID, VertexPositions[Vertex] * BuildScale);
	}

	if (bHasTangents)
	{
		TRealtimeMeshStreamBuilder<TRealtimeMeshTangents<FVector4f>, TRealtimeMeshTangents<FPackedNormal>> TangentData(
			OutStreamSet.AddStream(FRealtimeMeshStreams::Tangents, GetRealtimeMeshBufferLayout<TRealtimeMeshTangents<FPackedNormal>>()));
		TangentData.SetNumUninitialized(NumVerts);
		for (int32 VertID = 0; VertID < NumVerts; VertID++)
		{
			const FVertexInstanceID InstanceID = Instances[VertID];
			const FVector3f N = InstanceNormals[InstanceID];
			const FVector3f T = InstanceTangents[InstanceID];
			const FVector3f B = InstanceBinormalSigns[InstanceID] * FVector3f::CrossProduct(N, T);
			TangentData.Set(VertID, TRealtimeMeshTangents<FVector4f>(N, B, T));
		}
	}

	if (NumUVLayers > 0)
	{
		FRealtimeMeshStream& TexCoordStream = OutStreamSet.AddStream(FRealtimeMeshStreams::TexCoords, GetRealtimeMeshBufferLayout<FVector2f>(NumUVLayers));
		TexCoordStream.SetNumUninitialized(NumVerts);
		for (int32 UVLayer = 0; UVLayer < NumUVLayers; UVLayer++)
		{
			TRealtimeMeshStridedStreamBuilder<FVector2f> TexCoordData(TexCoordStream, UVLayer);
			for (int32 VertID = 0; VertID < NumVerts; VertID++)
			{
				TexCoordData.Set(VertID, InstanceUVs.Get(Instances[VertID], UVLayer));
			}
		}
	}

	if (bHasColors)
	{
		TRealtimeMeshStreamBuilder<FColor> ColorData(OutStreamSet.AddStream(FRealtimeMeshStreams::Color, GetRealtimeMeshBufferLayout<FColor>()));
		ColorData.SetNumUninitialized(NumVerts);
		for (int32 VertID = 0; VertID < NumVerts; VertID++)
		{
			const FVector4f C = InstanceColors[Instances[VertID]];
			// MeshDescription instance colors are linear; the export side decodes the source
			// FColor as sRGB (FLinearColor(FColor)), so re-encode with sRGB here to round-trip
			// vertex colors symmetrically (ToFColor(true)).
			ColorData.Set(VertID, FLinearColor(C.X, C.Y, C.Z, C.W).ToFColor(true));
		}
	}

	// --- Triangles (+ poly groups) ---
	TRealtimeMeshStreamBuilder<TIndex3<uint32>> TriangleData(OutStreamSet.AddStream(FRealtimeMeshStreams::Triangles, GetRealtimeMeshBufferLayout<TIndex3<uint32>>()));
	TriangleData.SetNumUninitialized(NumTris);

	TUniquePtr<TRealtimeMeshStreamBuilder<uint16>> PolyGroupData;
	if (Options.bWantPolyGroups)
	{
		PolyGroupData = MakeUnique<TRealtimeMeshStreamBuilder<uint16>>(OutStreamSet.AddStream(FRealtimeMeshStreams::PolyGroups, GetRealtimeMeshBufferLayout<uint16>()));
		PolyGroupData->SetNumUninitialized(NumTris);
	}

	int32 TriIndex = 0;
	for (const FTriangleID TriID : SourceMesh->Triangles().GetElementIDs())
	{
		const TArrayView<const FVertexInstanceID> Corners = SourceMesh->GetTriangleVertexInstances(TriID);
		TriangleData.Set(TriIndex, TIndex3<uint32>(InstanceToIndex[Corners[0]], InstanceToIndex[Corners[1]], InstanceToIndex[Corners[2]]));
		if (PolyGroupData)
		{
			PolyGroupData->Set(TriIndex, static_cast<uint16>(PolyGroupToIndex[SourceMesh->GetTrianglePolygonGroup(TriID)]));
		}
		TriIndex++;
	}

	return true;
#endif // WITH_EDITOR
}

