// Copyright 2016-2019 Chris Conway (Koderz). All Rights Reserved.


#include "RealtimeMeshData.h"
#include "RealtimeMeshProxy.h"
#include "RealtimeMesh.h"

FRealtimeMeshData::FRealtimeMeshData(const IRealtimeMeshProviderProxyRef& InBaseProvider, TWeakObjectPtr<URealtimeMesh> InParentMeshObject)
	: ParentMeshObject(InParentMeshObject), BaseProvider(InBaseProvider)
{
	LODs.SetNum(REALTIMEMESH_MAXLODS);
}

FRealtimeMeshData::~FRealtimeMeshData()
{

}

int32 FRealtimeMeshData::GetNumMaterials()
{
	return MaterialSlots.Num();
}

void FRealtimeMeshData::GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials)
{
	for (const auto& Mat : MaterialSlots)
	{
		OutMaterials.Add(Mat.Material.Get());
	}
}

UMaterialInterface* FRealtimeMeshData::GetMaterial(int32 SlotIndex)
{
	if (!MaterialSlots.IsValidIndex(SlotIndex))
	{
		return nullptr;
	}

	TWeakObjectPtr<UMaterialInterface> Material = MaterialSlots[SlotIndex].Material;
	return Material.Get();
}

void FRealtimeMeshData::Initialize()
{

	UE_LOG(LogRealtimeMesh, Warning, TEXT("RMD: initializing data.. %d"), FPlatformTLS::GetCurrentThreadId());

	// Make sure the provider chain is bound
	BaseProvider->BindPreviousProvider(this->AsShared());

	BaseProvider->Initialize();

	MarkCollisionDirty();
}

void FRealtimeMeshData::ConfigureLOD(uint8 LODIndex, const FRealtimeMeshLODProperties& LODProperties)
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

void FRealtimeMeshData::CreateSection(uint8 LODIndex, int32 SectionId, const FRealtimeMeshSectionProperties& SectionProperties)
{

	UE_LOG(LogRealtimeMesh, Warning, TEXT("RMD: Creating section.. %d"), FPlatformTLS::GetCurrentThreadId());
	{
		FScopeLock Lock(&SyncRoot);

		LODs[LODIndex].Sections.FindOrAdd(SectionId) = SectionProperties;
	}

	if (RenderProxy.IsValid())
	{
		UE_LOG(LogRealtimeMesh, Warning, TEXT("RMD: requesting recreate proxy for new section.. %d"), FPlatformTLS::GetCurrentThreadId());
		RenderProxy->CreateSection_GameThread(LODIndex, SectionId, SectionProperties);
		HandleProxySectionUpdate(LODIndex, SectionId, true);
	}
}

void FRealtimeMeshData::SetupMaterialSlot(int32 MaterialSlot, FName SlotName, UMaterialInterface* InMaterial)
{
	if (!MaterialSlots.IsValidIndex(MaterialSlot))
	{
		MaterialSlots.SetNum(MaterialSlot + 1);
	}
	MaterialSlots[MaterialSlot] = FRealtimeMeshMaterialSlot(SlotName, InMaterial);
}

void FRealtimeMeshData::MarkSectionDirty(uint8 LODIndex, int32 SectionId)
{
	if (RenderProxy.IsValid())
	{
		// TODO: Alter this to not just clear it but instead fire off a full recreate
		// this keeps around the current rendering mesh until the new mesh is ready
		RenderProxy->ClearSection_GameThread(LODIndex, SectionId);

		HandleProxySectionUpdate(LODIndex, SectionId);
	}	
}

void FRealtimeMeshData::SetSectionVisibility(uint8 LODIndex, int32 SectionId, bool bIsVisible)
{
	FScopeLock Lock(&SyncRoot);

	FRealtimeMeshSectionProperties* Section = LODs[LODIndex].Sections.Find(SectionId);
	if (Section)
	{
		Section->bIsVisible = bIsVisible;
		HandleProxySectionPropertiesUpdate(LODIndex, SectionId);
	}
}

void FRealtimeMeshData::SetSectionCastsShadow(uint8 LODIndex, int32 SectionId, bool bCastsShadow)
{
	FScopeLock Lock(&SyncRoot);

	FRealtimeMeshSectionProperties* Section = LODs[LODIndex].Sections.Find(SectionId);
	if (Section)
	{
		Section->bCastsShadow = bCastsShadow;
		HandleProxySectionPropertiesUpdate(LODIndex, SectionId);
	}
}

void FRealtimeMeshData::RemoveSection(uint8 LODIndex, int32 SectionId)
{
	FScopeLock Lock(&SyncRoot);

	LODs[LODIndex].Sections.Remove(SectionId);

	if (RenderProxy.IsValid())
	{
		RenderProxy->RemoveSection_GameThread(LODIndex, SectionId);
		RecreateAllComponentProxies();
	}
}

void FRealtimeMeshData::MarkCollisionDirty()
{
	DoOnGameThread(FRealtimeMeshGameThreadTaskDelegate::CreateLambda(
		[](URealtimeMesh* Mesh)
		{
			Mesh->MarkCollisionDirty();
		}
	));
}

void FRealtimeMeshData::HandleProxySectionPropertiesUpdate(int32 LODIndex, int32 SectionId)
{
	if (RenderProxy.IsValid())
	{
		RenderProxy->UpdateSectionProperties_GameThread(LODIndex, SectionId, LODs[LODIndex].Sections[SectionId]);
	}
}

void FRealtimeMeshData::HandleProxySectionUpdate(int32 LODIndex, int32 SectionId, bool bForceRecreateProxies, bool bSkipRecreateProxies)
{
	if (RenderProxy.IsValid())
	{
		// TODO: Here we'd want to interject threading to allow the proxy to be 
		// called on alternate threads and have the result passed to the render proxy directly

		bool bRequiresProxyRecreate = bForceRecreateProxies;

		const auto UpdateSingleSection = [&](int32 LODIdx, int32 SectionIdx, FRealtimeMeshSectionProperties& Properties) {
			TSharedPtr<FRealtimeMeshRenderableMeshData> MeshData = MakeShared<FRealtimeMeshRenderableMeshData>(
				Properties.bUseHighPrecisionTangents,
				Properties.bUseHighPrecisionTexCoords,
				Properties.NumTexCoords,
				Properties.bWants32BitIndices);
			bool bResult = BaseProvider->GetSectionMeshForLOD(LODIdx, SectionIdx, *MeshData);

			UE_LOG(LogRealtimeMesh, Warning, TEXT("RMD: updating sectiond data.. %d"), FPlatformTLS::GetCurrentThreadId());
			if (bResult)
			{

				UE_LOG(LogRealtimeMesh, Warning, TEXT("RMD: got mesh.. %d"), FPlatformTLS::GetCurrentThreadId());
				RenderProxy->UpdateSection_GameThread(LODIdx, SectionIdx, MeshData);
			}
			else
			{
				UE_LOG(LogRealtimeMesh, Warning, TEXT("RMD: cleared mesh.. %d"), FPlatformTLS::GetCurrentThreadId());
				RenderProxy->ClearSection_GameThread(LODIdx, SectionIdx);
			}

			bRequiresProxyRecreate |= Properties.UpdateFrequency == ERealtimeMeshUpdateFrequency::Infrequent;
		};

		const auto UpdateSingleLOD = [&](int32 LODIdx) {
			if (SectionId != INDEX_NONE)
			{
				FRealtimeMeshSectionProperties Properties = LODs[LODIdx].Sections.FindChecked(SectionId);
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
			for (int32 Index = 0; Index < REALTIMEMESH_MAXLODS; Index++)
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

void FRealtimeMeshData::RecreateAllComponentProxies()
{
	DoOnGameThread(FRealtimeMeshGameThreadTaskDelegate::CreateLambda(
		[](URealtimeMesh* Mesh)
		{
			Mesh->RecreateAllComponentProxies();
		}
	));
}

FRealtimeMeshProxyPtr FRealtimeMeshData::GetOrCreateRenderProxy(ERHIFeatureLevel::Type InFeatureLevel)
{
	if (RenderProxy.IsValid())
	{
		check(InFeatureLevel == RenderProxy->GetFeatureLevel());
		return RenderProxy;
	}

	{
		FScopeLock Lock(&SyncRoot);

		RenderProxy = MakeShareable(new FRealtimeMeshProxy(InFeatureLevel), FRealtimeMeshRenderThreadDeleter<FRealtimeMeshProxy>());

		for (int32 LODIndex = 0; LODIndex < LODs.Num(); LODIndex++)
		{
			const FRealtimeMeshLOD& LOD = LODs[LODIndex];
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
