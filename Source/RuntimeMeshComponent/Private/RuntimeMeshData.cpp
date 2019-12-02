// Copyright 2016-2019 Chris Conway (Koderz). All Rights Reserved.


#include "RuntimeMeshData.h"
#include "RuntimeMeshProxy.h"
#include "RuntimeMesh.h"

FRuntimeMeshData::FRuntimeMeshData(const FRuntimeMeshProviderProxyRef& InBaseProvider, TWeakObjectPtr<URuntimeMesh> InParentMeshObject)
	: FRuntimeMeshProviderProxy(nullptr), ParentMeshObject(InParentMeshObject), BaseProvider(InBaseProvider)
{
	LODs.SetNum(RuntimeMesh_MAXLODS);
}

FRuntimeMeshData::~FRuntimeMeshData()
{

}

int32 FRuntimeMeshData::GetNumMaterials()
{
	return MaterialSlots.Num();
}

void FRuntimeMeshData::GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials)
{
	for (const auto& Mat : MaterialSlots)
	{
		OutMaterials.Add(Mat.Material.Get());
	}
}

UMaterialInterface* FRuntimeMeshData::GetMaterial(int32 SlotIndex)
{
	if (!MaterialSlots.IsValidIndex(SlotIndex))
	{
		return nullptr;
	}

	TWeakObjectPtr<UMaterialInterface> Material = MaterialSlots[SlotIndex].Material;
	return Material.Get();
}

void FRuntimeMeshData::Initialize()
{
	UE_LOG(LogRuntimeMesh, Warning, TEXT("RMD: initializing data.. %d"), FPlatformTLS::GetCurrentThreadId());

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

	UE_LOG(LogRuntimeMesh, Warning, TEXT("RMD: Creating section.. %d"), FPlatformTLS::GetCurrentThreadId());
	{
		FScopeLock Lock(&SyncRoot);

		LODs[LODIndex].Sections.FindOrAdd(SectionId) = SectionProperties;
	}

	if (RenderProxy.IsValid())
	{
		UE_LOG(LogRuntimeMesh, Warning, TEXT("RMD: requesting recreate proxy for new section.. %d"), FPlatformTLS::GetCurrentThreadId());
		RenderProxy->CreateSection_GameThread(LODIndex, SectionId, SectionProperties);
		HandleProxySectionUpdate(LODIndex, SectionId, true);
	}
}

void FRuntimeMeshData::SetupMaterialSlot(int32 MaterialSlot, FName SlotName, UMaterialInterface* InMaterial)
{
	if (!MaterialSlots.IsValidIndex(MaterialSlot))
	{
		MaterialSlots.SetNum(MaterialSlot + 1);
	}
	MaterialSlots[MaterialSlot] = FRuntimeMeshMaterialSlot(SlotName, InMaterial);
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
	if (RenderProxy.IsValid())
	{
		RenderProxy->UpdateSectionProperties_GameThread(LODIndex, SectionId, LODs[LODIndex].Sections[SectionId]);
	}
}

void FRuntimeMeshData::HandleProxySectionUpdate(int32 LODIndex, int32 SectionId, bool bForceRecreateProxies, bool bSkipRecreateProxies)
{
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

			UE_LOG(LogRuntimeMesh, Warning, TEXT("RMD: updating sectiond data.. %d"), FPlatformTLS::GetCurrentThreadId());
			if (bResult)
			{

				UE_LOG(LogRuntimeMesh, Warning, TEXT("RMD: got mesh.. %d"), FPlatformTLS::GetCurrentThreadId());
				RenderProxy->UpdateSection_GameThread(LODIdx, SectionIdx, MeshData);
			}
			else
			{
				UE_LOG(LogRuntimeMesh, Warning, TEXT("RMD: cleared mesh.. %d"), FPlatformTLS::GetCurrentThreadId());
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
			for (int32 Index = 0; Index < RuntimeMesh_MAXLODS; Index++)
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
