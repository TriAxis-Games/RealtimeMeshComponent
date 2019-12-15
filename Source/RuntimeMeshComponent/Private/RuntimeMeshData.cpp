// Copyright 2016-2019 Chris Conway (Koderz). All Rights Reserved.

#include "RuntimeMeshData.h"
#include "RuntimeMeshProxy.h"
#include "RuntimeMesh.h"

DECLARE_CYCLE_STAT(TEXT("RuntimeMeshData - Initialize"), STAT_RuntimeMeshData_Initialize, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RuntimeMeshData - Update Section Properties"), STAT_RuntimeMeshData_UpdateSectionProperties, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RuntimeMeshData - Update Section"), STAT_RuntimeMeshData_UpdateSection, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RuntimeMeshData - Recreate All Proxies"), STAT_RuntimeMeshData_RecreateProxies, STATGROUP_RuntimeMesh);


FRuntimeMeshData::FRuntimeMeshData(const FRuntimeMeshProviderProxyRef& InBaseProvider, TWeakObjectPtr<URuntimeMesh> InParentMeshObject)
	: FRuntimeMeshProviderProxy(nullptr), ParentMeshObject(InParentMeshObject), BaseProvider(InBaseProvider)
{
	LODs.SetNum(RUNTIMEMESH_MAXLODS);
	for (int32 Index = 1; Index < RUNTIMEMESH_MAXLODS; Index++)
	{
		// All but the first LOD gets defaulted to 0 screen size to not show ever.
		LODs[Index].Properties.ScreenSize = 0.0f;
	}
}

FRuntimeMeshData::~FRuntimeMeshData()
{

}

int32 FRuntimeMeshData::GetNumMaterials() const
{
	return MaterialSlots.Num();
}

UMaterialInterface* FRuntimeMeshData::GetMaterial(int32 SlotIndex) const
{
	if (!MaterialSlots.IsValidIndex(SlotIndex))
	{
		return nullptr;
	}

	TWeakObjectPtr<UMaterialInterface> Material = MaterialSlots[SlotIndex].Material;
	return Material.Get();
}

int32 FRuntimeMeshData::GetMaterialIndex(FName MaterialSlotName) const
{
	const int32* SlotIndex = SlotNameLookup.Find(MaterialSlotName);
	return SlotIndex ? *SlotIndex : INDEX_NONE;
}

TArray<FName> FRuntimeMeshData::GetMaterialSlotNames() const
{
	TArray<FName> OutNames;
	SlotNameLookup.GetKeys(OutNames);
	return OutNames;
}

bool FRuntimeMeshData::IsMaterialSlotNameValid(FName MaterialSlotName) const
{
	return SlotNameLookup.Contains(MaterialSlotName);
}

void FRuntimeMeshData::Initialize()
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMeshData_Initialize);

	// Make sure the provider chain is bound
	BaseProvider->BindPreviousProvider(this->AsShared());

	BaseProvider->Initialize();

	MarkSectionDirty(INDEX_NONE, INDEX_NONE);
	MarkCollisionDirty();
}

void FRuntimeMeshData::ConfigureLOD(int32 LODIndex, const FRuntimeMeshLODProperties& LODProperties)
{
	{
		FScopeLock Lock(&SyncRoot);
		LODs[LODIndex].Properties = LODProperties;
	}

	if (RenderProxy.IsValid())
	{
		RenderProxy->ConfigureLOD_GameThread(LODIndex, LODProperties);
		RecreateAllComponentProxies();
	}
}

void FRuntimeMeshData::CreateSection(int32 LODIndex, int32 SectionId, const FRuntimeMeshSectionProperties& SectionProperties)
{
	{
		FScopeLock Lock(&SyncRoot);

		LODs[LODIndex].Sections.FindOrAdd(SectionId) = SectionProperties;
	}

	if (RenderProxy.IsValid())
	{
		RenderProxy->CreateSection_GameThread(LODIndex, SectionId, SectionProperties);
		HandleProxySectionUpdate(LODIndex, SectionId, true);
	}
}

bool FRuntimeMeshData::SetupMaterialSlot(int32 MaterialSlot, FName SlotName, UMaterialInterface* InMaterial)
{
	// Does this slot already exist?
	if (SlotNameLookup.Contains(SlotName))
	{
		// If the indices match then just go with it
		if (SlotNameLookup[SlotName] == MaterialSlot)
		{
			MaterialSlots[SlotNameLookup[SlotName]].Material = InMaterial;
			return true;
		}
		return false;
	}
	
	if (!MaterialSlots.IsValidIndex(MaterialSlot))
	{
		MaterialSlots.SetNum(MaterialSlot + 1);
	}
	MaterialSlots[MaterialSlot] = FRuntimeMeshMaterialSlot(SlotName, InMaterial);
	SlotNameLookup.Add(SlotName, MaterialSlots.Num() - 1);

	return true;
}

void FRuntimeMeshData::MarkSectionDirty(int32 LODIndex, int32 SectionId)
{
	if (RenderProxy.IsValid())
	{
		// TODO: Alter this to not just clear it but instead fire off a full recreate
		// this keeps around the current rendering mesh until the new mesh is ready
		RenderProxy->ClearSection_GameThread(LODIndex, SectionId);

		HandleProxySectionUpdate(LODIndex, SectionId);
	}	
}

void FRuntimeMeshData::SetSectionVisibility(int32 LODIndex, int32 SectionId, bool bIsVisible)
{
	FScopeLock Lock(&SyncRoot);

	FRuntimeMeshSectionProperties* Section = LODs[LODIndex].Sections.Find(SectionId);
	if (Section)
	{
		Section->bIsVisible = bIsVisible;
		HandleProxySectionPropertiesUpdate(LODIndex, SectionId);
	}
}

void FRuntimeMeshData::SetSectionCastsShadow(int32 LODIndex, int32 SectionId, bool bCastsShadow)
{
	FScopeLock Lock(&SyncRoot);

	FRuntimeMeshSectionProperties* Section = LODs[LODIndex].Sections.Find(SectionId);
	if (Section)
	{
		Section->bCastsShadow = bCastsShadow;
		HandleProxySectionPropertiesUpdate(LODIndex, SectionId);
	}
}

void FRuntimeMeshData::RemoveSection(int32 LODIndex, int32 SectionId)
{
	FScopeLock Lock(&SyncRoot);

	LODs[LODIndex].Sections.Remove(SectionId);

	if (RenderProxy.IsValid())
	{
		RenderProxy->RemoveSection_GameThread(LODIndex, SectionId);
		RecreateAllComponentProxies();
	}
}

void FRuntimeMeshData::MarkCollisionDirty()
{
	DoOnGameThread(FRuntimeMeshGameThreadTaskDelegate::CreateLambda(
		[](URuntimeMesh* Mesh)
		{
			Mesh->MarkCollisionDirty();
		}
	));
}

void FRuntimeMeshData::HandleProxySectionPropertiesUpdate(int32 LODIndex, int32 SectionId)
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMeshData_UpdateSectionProperties);

	if (RenderProxy.IsValid())
	{
		RenderProxy->UpdateSectionProperties_GameThread(LODIndex, SectionId, LODs[LODIndex].Sections[SectionId]);
	}
}

void FRuntimeMeshData::HandleProxySectionUpdate(int32 LODIndex, int32 SectionId, bool bForceRecreateProxies, bool bSkipRecreateProxies)
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMeshData_UpdateSection);

	if (RenderProxy.IsValid())
	{
		// TODO: Here we'd want to interject threading to allow the proxy to be 
		// called on alternate threads and have the result passed to the render proxy directly

		bool bRequiresProxyRecreate = bForceRecreateProxies;

		const auto UpdateSingleSection = [&](int32 LODIdx, int32 SectionIdx, FRuntimeMeshSectionProperties& Properties) {
			TSharedPtr<FRuntimeMeshRenderableMeshData> MeshData = MakeShared<FRuntimeMeshRenderableMeshData>(
				Properties.bUseHighPrecisionTangents,
				Properties.bUseHighPrecisionTexCoords,
				Properties.NumTexCoords,
				Properties.bWants32BitIndices);
			bool bResult = BaseProvider->GetSectionMeshForLOD(LODIdx, SectionIdx, *MeshData);

			if (bResult)
			{
				RenderProxy->UpdateSection_GameThread(LODIdx, SectionIdx, MeshData);
			}
			else
			{
				RenderProxy->ClearSection_GameThread(LODIdx, SectionIdx);
			}

			bRequiresProxyRecreate |= Properties.UpdateFrequency == ERuntimeMeshUpdateFrequency::Infrequent;
		};

		const auto UpdateSingleLOD = [&](int32 LODIdx) {
			if (SectionId != INDEX_NONE)
			{
				FRuntimeMeshSectionProperties Properties = LODs[LODIdx].Sections.FindChecked(SectionId);
				UpdateSingleSection(LODIdx, SectionId, Properties);
			}
			else
			{
				for (auto& Entry : LODs[LODIdx].Sections)
				{
					UpdateSingleSection(LODIdx, Entry.Key, Entry.Value);
				}
			}
		};

		if (LODIndex != INDEX_NONE)
		{
			UpdateSingleLOD(LODIndex);
		}
		else
		{
			for (int32 Index = 0; Index < RUNTIMEMESH_MAXLODS; Index++)
			{
				UpdateSingleLOD(Index);
			}
		}

		


		if (!bSkipRecreateProxies && bRequiresProxyRecreate && RenderProxy.IsValid())
		{
			RecreateAllComponentProxies();
		}
	}
}

void FRuntimeMeshData::RecreateAllComponentProxies()
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMeshData_RecreateProxies);

	DoOnGameThread(FRuntimeMeshGameThreadTaskDelegate::CreateLambda(
		[](URuntimeMesh* Mesh)
		{
			Mesh->RecreateAllComponentProxies();
		}
	));
}

FRuntimeMeshProxyPtr FRuntimeMeshData::GetOrCreateRenderProxy(ERHIFeatureLevel::Type InFeatureLevel)
{
	if (RenderProxy.IsValid())
	{
		check(InFeatureLevel == RenderProxy->GetFeatureLevel());
		return RenderProxy;
	}

	{
		FScopeLock Lock(&SyncRoot);

		RenderProxy = MakeShareable(new FRuntimeMeshProxy(InFeatureLevel), FRuntimeMeshRenderThreadDeleter<FRuntimeMeshProxy>());

		for (int32 LODIndex = 0; LODIndex < LODs.Num(); LODIndex++)
		{
			const FRuntimeMeshLOD& LOD = LODs[LODIndex];
			RenderProxy->ConfigureLOD_GameThread(LODIndex, LOD.Properties);

			for (const auto& Section : LOD.Sections)
			{
				RenderProxy->CreateSection_GameThread(LODIndex, Section.Key, Section.Value);
			}
		}

		HandleProxySectionUpdate(INDEX_NONE, INDEX_NONE, false, true);
	}
	return RenderProxy;
}
