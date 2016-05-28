// Copyright 2016 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "RuntimeMeshComponentPluginPrivatePCH.h"
#include "RuntimeMeshCore.h"
#include "RuntimeMeshGenericVertex.h"
#include "RuntimeMeshVersion.h"


struct FRMCBatchUpdateCreateUpdateSection
{
	bool bIsCreate;
	FRuntimeMeshSectionUpdateDataInterface* SectionData;
};

struct FRMCBatchUpdatePropertyUpdateSection
{
	int32 TargetSection;
	bool bIsVisible;
	bool bCastsShadow;
};


struct FRMCBatchUpdateData
{
	TArray<FRMCBatchUpdateCreateUpdateSection> CreateUpdateSections;
	TArray<int32> DestroySections;
	TArray<FRMCBatchUpdatePropertyUpdateSection> PropertyUpdateSections;
};




/** Runtime mesh scene proxy */
class FRuntimeMeshSceneProxy : public FPrimitiveSceneProxy
{
private:
	TUniformBufferRef<FPrimitiveUniformShaderParameters> MeshUniformBuffer;

public:

	FRuntimeMeshSceneProxy(URuntimeMeshComponent* Component)
		: FPrimitiveSceneProxy(Component), MaterialRelevance(Component->GetMaterialRelevance(GetScene().GetFeatureLevel()))
	{
		// Get the proxy for all mesh sections

		const int32 NumSections = Component->MeshSections.Num();
		Sections.AddDefaulted(NumSections);

		for (int32 SectionIdx = 0; SectionIdx < NumSections; SectionIdx++)
		{
			RuntimeMeshSectionPtr& SourceSection = Component->MeshSections[SectionIdx];
			if (SourceSection.IsValid())
			{
				// Get the section creation data
				auto* SectionData = SourceSection->GetSectionCreationData(Component->GetMaterial(SectionIdx));
				

				auto Proxy = SectionData->GetNewProxy();

				if (!IsInRenderingThread())
				{
					// Enqueue update on RT
					ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
						FRuntimeMeshCreateSectionInternalCommand,
						FRuntimeMeshSectionProxyInterface*, Proxy, Proxy,
						FRuntimeMeshSectionUpdateDataInterface*, SectionData, SectionData,
						{
							Proxy->FinishCreate_RenderThread(SectionData);
						}
					);
				}
				else
				{
					Proxy->FinishCreate_RenderThread(SectionData);
				}

				// Save ref to new section
				Sections[SectionIdx] = Proxy;

			}
		}
	}

	virtual ~FRuntimeMeshSceneProxy()
	{
		for (FRuntimeMeshSectionProxyInterface* Section : Sections)
		{
			if (Section)
			{
				delete Section;
			}
		}
	}

	/** Called on render thread to create a new dynamic section. (Static sections are handled differently) */
	void CreateSection_RenderThread(FRuntimeMeshSectionUpdateDataInterface* SectionData)
	{
		SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_CreateSection_RenderThread);

		check(IsInRenderingThread());
		check(SectionData);

		int32 SectionIndex = SectionData->GetTargetSection();

		// Make sure the array is big enough
		Sections.SetNum(SectionIndex + 1, false);
		
		// If a section already exists... destroy it!
		if (FRuntimeMeshSectionProxyInterface* Section = Sections[SectionIndex])
		{			
			delete Section;
		}
		
		FRuntimeMeshSectionProxyInterface* Section = SectionData->GetNewProxy();

		Section->FinishCreate_RenderThread(SectionData);		

		// Save ref to new section
		Sections[SectionIndex] = Section;
		
		delete SectionData;
	}

	/** Called on render thread to assign new dynamic data */
  	void UpdateSection_RenderThread(FRuntimeMeshSectionUpdateDataInterface* SectionData)
  	{
		SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_UpdateSection_RenderThread);

  		check(IsInRenderingThread());
		check(SectionData);

		if (SectionData->GetTargetSection() < Sections.Num() && Sections[SectionData->GetTargetSection()] != nullptr)
		{
			Sections[SectionData->GetTargetSection()]->FinishUpdate_RenderThread(SectionData);
		}

		delete SectionData;
 	}

	void SetSectionVisibility_RenderThread(int32 SectionIndex, bool bIsVisible)
	{
		SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_SetSectionVisibility_RenderThread);

 		check(IsInRenderingThread());
 
 		if (SectionIndex < Sections.Num() && Sections[SectionIndex] != nullptr)
 		{
 			Sections[SectionIndex]->SetIsVisible(bIsVisible);
 		}
	}

	void SetSectionCastsShadow_RenderThread(int32 SectionIndex, bool bCastsShadow)
	{
		SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_SetSectionCastsShadow_RenderThread);

		check(IsInRenderingThread());

		if (SectionIndex < Sections.Num() && Sections[SectionIndex] != nullptr)
		{
			Sections[SectionIndex]->SetCastsShadow(bCastsShadow);
		}
	}

	void DestroySection_RenderThread(int32 SectionIndex)
	{
		check(IsInRenderingThread());

		if (SectionIndex < Sections.Num() && Sections[SectionIndex] != nullptr)
		{
			delete Sections[SectionIndex];
			Sections[SectionIndex] = nullptr;
		}
	}

	void ApplyBatchUpdate_RenderThread(FRMCBatchUpdateData* BatchUpdateData)
	{
		SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_ApplyBatchUpdate_RenderThread);
		check(IsInRenderingThread());
		check(BatchUpdateData);

		// Destroy flagged sections
		for (auto& SectionIndex : BatchUpdateData->DestroySections)
		{
			DestroySection_RenderThread(SectionIndex);
		}

		// Create new sections
		for (auto& SectionToCreate : BatchUpdateData->CreateUpdateSections)
		{
			if (SectionToCreate.bIsCreate)
			{
				CreateSection_RenderThread(SectionToCreate.SectionData);
			}
			else
			{
				UpdateSection_RenderThread(SectionToCreate.SectionData);
			}
		}

		// Apply section property updates
		for (auto& SectionToUpdate : BatchUpdateData->PropertyUpdateSections)
		{

			if (SectionToUpdate.TargetSection < Sections.Num() && Sections[SectionToUpdate.TargetSection] != nullptr)
			{
				auto& Section = Sections[SectionToUpdate.TargetSection];

				Section->SetIsVisible(SectionToUpdate.bIsVisible);
				Section->SetCastsShadow(SectionToUpdate.bCastsShadow);
			}
		}

		delete BatchUpdateData;

	}


	
	virtual void OnTransformChanged() override
	{
		SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_OnTransformChanged);

		// Create a uniform buffer with the transform for this mesh.
		MeshUniformBuffer = CreatePrimitiveUniformBufferImmediate(GetLocalToWorld(), GetBounds(), GetLocalBounds(), true, UseEditorDepthTest());
	}

	bool HasDynamicSections() const
	{
		for (FRuntimeMeshSectionProxyInterface* Section : Sections)
		{
			if (Section && Section->GetUpdateFrequency() != EUpdateFrequency::Infrequent)
			{
				return true;
			}
		}
		return false;
	}

	bool HasStaticSections() const 
	{
		for (FRuntimeMeshSectionProxyInterface* Section : Sections)
		{
			if (Section && Section->GetUpdateFrequency() == EUpdateFrequency::Infrequent)
			{
				return true;
			}
		}
		return false;
	}


	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const
	{
		FPrimitiveViewRelevance Result;
		Result.bDrawRelevance = IsShown(View);
		Result.bShadowRelevance = IsShadowCast(View);
		
		bool bForceDynamicPath = IsRichView(*View->Family) || View->Family->EngineShowFlags.Wireframe || IsSelected() || !IsStaticPathAvailable();
		Result.bStaticRelevance = !bForceDynamicPath && HasStaticSections();
		Result.bDynamicRelevance = bForceDynamicPath || HasDynamicSections();
		
		Result.bRenderInMainPass = ShouldRenderInMainPass();
		Result.bUsesLightingChannels = GetLightingChannelMask() != GetDefaultLightingChannelMask();
		Result.bRenderCustomDepth = ShouldRenderCustomDepth();
		MaterialRelevance.SetPrimitiveViewRelevance(Result);
		return Result;
	}


	void CreateMeshBatch(FMeshBatch& MeshBatch, FRuntimeMeshSectionProxyInterface* Section, FMaterialRenderProxy* MaterialProxy) const
	{
		MeshBatch.bWireframe = MaterialProxy == nullptr;
		MeshBatch.VertexFactory = &Section->GetVertexFactory();
		MeshBatch.MaterialRenderProxy = MaterialProxy;
		MeshBatch.ReverseCulling = IsLocalToWorldDeterminantNegative();
		MeshBatch.Type = PT_TriangleList;
		MeshBatch.DepthPriorityGroup = SDPG_World;
		MeshBatch.bCanApplyViewModeOverrides = false;
		MeshBatch.CastShadow = Section->CastsShadow();

		FMeshBatchElement& BatchElement = MeshBatch.Elements[0];
		BatchElement.PrimitiveUniformBuffer = MeshUniformBuffer;
		BatchElement.IndexBuffer = &Section->GetIndexBuffer();
		BatchElement.FirstIndex = 0;
		BatchElement.NumPrimitives = Section->GetIndexCount() / 3;
		BatchElement.MinVertexIndex = 0;
		BatchElement.MaxVertexIndex = Section->GetVertexCount() - 1;
	}

	virtual void DrawStaticElements(FStaticPrimitiveDrawInterface* PDI) override
	{
		SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_DrawStaticElements);

		for (FRuntimeMeshSectionProxyInterface* Section : Sections)
		{
			if (Section && Section->ShouldRender() && Section->GetUpdateFrequency() == EUpdateFrequency::Infrequent)
			{
				FMaterialRenderProxy* MaterialProxy = Section->GetMaterial()->GetRenderProxy(IsSelected());

				FMeshBatch Batch;
				CreateMeshBatch(Batch, Section, MaterialProxy);
				PDI->DrawMesh(Batch, FLT_MAX);
			}
		}
	}

	virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override
	{
		SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_GetDynamicMeshElements);

		// Set up wireframe material (if needed)
		const bool bWireframe = AllowDebugViewmodes() && ViewFamily.EngineShowFlags.Wireframe;

		FColoredMaterialRenderProxy* WireframeMaterialInstance = NULL;
		if (bWireframe)
		{
			WireframeMaterialInstance = new FColoredMaterialRenderProxy(
				GEngine->WireframeMaterial ? GEngine->WireframeMaterial->GetRenderProxy(IsSelected()) : NULL,
				FLinearColor(0, 0.5f, 1.f)
				);

			Collector.RegisterOneFrameMaterialProxy(WireframeMaterialInstance);
		}

		// Iterate over sections
		for (FRuntimeMeshSectionProxyInterface* Section : Sections)
		{
			if (Section && Section->ShouldRender())
			{
				FMaterialRenderProxy* MaterialProxy = bWireframe ? WireframeMaterialInstance : Section->GetMaterial()->GetRenderProxy(IsSelected());

				// Add the mesh batch to every view it's visible in
				for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
				{
					if (VisibilityMap & (1 << ViewIndex))
					{
						bool bForceDynamicPath = IsRichView(*Views[ViewIndex]->Family) || Views[ViewIndex]->Family->EngineShowFlags.Wireframe || IsSelected() || !IsStaticPathAvailable();

						if (bForceDynamicPath || Section->GetUpdateFrequency() != EUpdateFrequency::Infrequent)
						{
							FMeshBatch& MeshBatch = Collector.AllocateMesh();
							CreateMeshBatch(MeshBatch, Section, MaterialProxy);

							Collector.AddMesh(ViewIndex, MeshBatch);
						}
					}
				}
			}			
		}

		// Draw bounds
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
		{
			if (VisibilityMap & (1 << ViewIndex))
			{
				// Render bounds
				RenderBounds(Collector.GetPDI(ViewIndex), ViewFamily.EngineShowFlags, GetBounds(), IsSelected());
			}
		}
#endif
	}


	virtual bool CanBeOccluded() const override
	{
		return !MaterialRelevance.bDisableDepthTest;
	}

	virtual uint32 GetMemoryFootprint(void) const
	{
		return(sizeof(*this) + GetAllocatedSize());
	}

	uint32 GetAllocatedSize(void) const
	{
		return(FPrimitiveSceneProxy::GetAllocatedSize());
	}

private:
	/** Array of sections */
	TArray<FRuntimeMeshSectionProxyInterface*> Sections;

	FMaterialRelevance MaterialRelevance;
};




void FRuntimeMeshComponentPrePhysicsTickFunction::ExecuteTick( float DeltaTime, ELevelTick TickType, ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
{
	if (Target && !Target->IsPendingKillOrUnreachable())
	{
		FScopeCycleCounterUObject ActorScope(Target);
		Target->BakeCollision();
	}
}

FString FRuntimeMeshComponentPrePhysicsTickFunction::DiagnosticMessage()
{
	return Target->GetFullName() + TEXT("[PrePhysicsTick]");
}







URuntimeMeshComponent::URuntimeMeshComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer), bCollisionDirty(true), bUseComplexAsSimpleCollision(true), bShouldSerializeMeshData(true)
{
	PrePhysicsTick.TickGroup = TG_PrePhysics;
	PrePhysicsTick.bCanEverTick = true;
	PrePhysicsTick.bStartWithTickEnabled = true;


	BatchUpdateInfo.Reset();
}

TSharedPtr<FRuntimeMeshSectionInterface> URuntimeMeshComponent::CreateOrResetSectionInternalType(int32 SectionIndex, int32 NumUVChannels, bool WantsHalfPrecsionUVs)
{
	// Ensure sections array is long enough
	if (SectionIndex >= MeshSections.Num())
	{
		MeshSections.SetNum(SectionIndex + 1, false);
	}

	switch (NumUVChannels)
	{
	case 1:
		if (WantsHalfPrecsionUVs)
		{
			MeshSections[SectionIndex] = MakeShareable(new FRuntimeMeshSectionInternal<1, true>);
		}
		else
		{
			MeshSections[SectionIndex] = MakeShareable(new FRuntimeMeshSectionInternal<1, false>);
		}
		break;
	case 2:
		if (WantsHalfPrecsionUVs)
		{
			MeshSections[SectionIndex] = MakeShareable(new FRuntimeMeshSectionInternal<2, true>);
		}
		else
		{
			MeshSections[SectionIndex] = MakeShareable(new FRuntimeMeshSectionInternal<2, false>);
		}
		break;
	case 3:
		if (WantsHalfPrecsionUVs)
		{
			MeshSections[SectionIndex] = MakeShareable(new FRuntimeMeshSectionInternal<3, true>);
		}
		else
		{
			MeshSections[SectionIndex] = MakeShareable(new FRuntimeMeshSectionInternal<3, false>);
		}
		break;
	case 4:
		if (WantsHalfPrecsionUVs)
		{
			MeshSections[SectionIndex] = MakeShareable(new FRuntimeMeshSectionInternal<4, true>);
		}
		else
		{
			MeshSections[SectionIndex] = MakeShareable(new FRuntimeMeshSectionInternal<4, false>);
		}
		break;
	case 5:
		if (WantsHalfPrecsionUVs)
		{
			MeshSections[SectionIndex] = MakeShareable(new FRuntimeMeshSectionInternal<5, true>);
		}
		else
		{
			MeshSections[SectionIndex] = MakeShareable(new FRuntimeMeshSectionInternal<5, false>);
		}
		break;
	case 6:
		if (WantsHalfPrecsionUVs)
		{
			MeshSections[SectionIndex] = MakeShareable(new FRuntimeMeshSectionInternal<6, true>);
		}
		else
		{
			MeshSections[SectionIndex] = MakeShareable(new FRuntimeMeshSectionInternal<6, false>);
		}
		break;
	case 7:
		if (WantsHalfPrecsionUVs)
		{
			MeshSections[SectionIndex] = MakeShareable(new FRuntimeMeshSectionInternal<7, true>);
		}
		else
		{
			MeshSections[SectionIndex] = MakeShareable(new FRuntimeMeshSectionInternal<7, false>);
		}
		break;
	case 8:
		if (WantsHalfPrecsionUVs)
		{
			MeshSections[SectionIndex] = MakeShareable(new FRuntimeMeshSectionInternal<8, true>);
		}
		else
		{
			MeshSections[SectionIndex] = MakeShareable(new FRuntimeMeshSectionInternal<8, false>);
		}
		break;

	default:
		check(false && "Invalid number of UV channels for section.");
	}

	MeshSections[SectionIndex]->bIsInternalSectionType = true;
	return MeshSections[SectionIndex];
}

void URuntimeMeshComponent::FinishCreateSectionInternal(int32 SectionIndex, RuntimeMeshSectionPtr& Section, bool bNeedsBoundsUpdate)
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_FinishCreateSectionInternal);
	
	// Are we in a batch update?
	if (BatchUpdateInfo.bIsPending)
	{
		// add section to add or force recreate if this section is static
		if (Section->UpdateFrequency == EUpdateFrequency::Infrequent)
		{
			BatchUpdateInfo.bRequiresSceneProxyReCreate = true;
		}
		else
		{
			BatchUpdateInfo.AddSectionToAdd(SectionIndex);
		}

		// Flag collision update needed if necessary
		if (Section->CollisionEnabled)
		{
			BatchUpdateInfo.bRequiresCollisionUpdate = true;
		}

		if (bNeedsBoundsUpdate)
		{
			// Flag bounds update needed
			BatchUpdateInfo.bRequiresBoundsUpdate = true;
		}

		// Bail as the rest is only for non batched creates
		return;
	}


	// Enqueue the RT command if we already have a SceneProxy
	if (SceneProxy && Section->UpdateFrequency != EUpdateFrequency::Infrequent)
	{
		// Gather all needed update info
		auto* SectionData = Section->GetSectionCreationData(GetMaterial(SectionIndex));
		SectionData->SetTargetSection(SectionIndex);

		// Enqueue update on RT
		ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
			FRuntimeMeshSectionCreate,
			FRuntimeMeshSceneProxy*, RuntimeMeshSceneProxy, (FRuntimeMeshSceneProxy*)SceneProxy,
			FRuntimeMeshSectionUpdateDataInterface*, SectionData, SectionData,
			{
				RuntimeMeshSceneProxy->CreateSection_RenderThread(SectionData);
			}
		);
	}
	else
	{
		// Mark the render state dirty so it's recreated when necessary.
		MarkRenderStateDirty();
	}

	// Mark collision dirty so it's re-baked at the end of this frame
	if (Section->CollisionEnabled)
	{
		MarkCollisionDirty();
	}

	if (bNeedsBoundsUpdate)
	{
		// Update overall bounds
		UpdateLocalBounds();
	}
}

void URuntimeMeshComponent::FinishUpdateSectionInternal(int32 SectionIndex, RuntimeMeshSectionPtr& Section, bool bHadPositionUpdates, bool bHadIndexUpdates, bool bNeedsBoundsUpdate)
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_FinishUpdateSectionInternal);
	check(bHadPositionUpdates || bHadIndexUpdates);

	// Are we in a batch update?
	if (BatchUpdateInfo.bIsPending)
	{
		// add section to add or force recreate if this section is static
		if (Section->UpdateFrequency == EUpdateFrequency::Infrequent)
		{
			BatchUpdateInfo.bRequiresSceneProxyReCreate = true;
		}
		else
		{
			ERMCBatchSectionUpdateType UpdateType = ERMCBatchSectionUpdateType::None;
			if (bHadPositionUpdates)
			{
				UpdateType |= ERMCBatchSectionUpdateType::VerticesUpdate;
			}

			if (bHadIndexUpdates)
			{
				UpdateType |= ERMCBatchSectionUpdateType::IndicesUpdate;
			}

			BatchUpdateInfo.AddUpdateForSection(SectionIndex, UpdateType);
		}

		// Flag collision update needed if necessary
		if (Section->CollisionEnabled)
		{
			BatchUpdateInfo.bRequiresCollisionUpdate = true;
		}
		
		if (bNeedsBoundsUpdate)
		{
			// Flag bounds update needed
			BatchUpdateInfo.bRequiresBoundsUpdate = true;
		}

		// Bail as the rest is only for non batched updates
		return;
	}

	// Send the update to the render thread if the scene proxy exists
	if (SceneProxy && Section->UpdateFrequency != EUpdateFrequency::Infrequent)
	{
		auto* SectionData = Section->GetSectionUpdateData(bHadIndexUpdates);
		SectionData->SetTargetSection(SectionIndex);

		// Enqueue update on RT
		ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
			FRuntimeMeshSectionUpdate,
			FRuntimeMeshSceneProxy*, RuntimeMeshSceneProxy, (FRuntimeMeshSceneProxy*)SceneProxy,
			FRuntimeMeshSectionUpdateDataInterface*, SectionData, SectionData,
			{
				RuntimeMeshSceneProxy->UpdateSection_RenderThread(SectionData);
			}
		);
	}
	else
	{
		// Mark the renderstate dirty so it's recreated when necessary.
		MarkRenderStateDirty();
	}

	// Update bounds and flag RT to update as well if we changed any vertex positions
	if (bHadPositionUpdates || bHadIndexUpdates)
	{
		// Mark collision dirty so it's rebaked at the end of this frame
		if (Section->CollisionEnabled)
		{
			MarkCollisionDirty();
		}

		if (bNeedsBoundsUpdate)
		{
			// Update overall bounds
			UpdateLocalBounds();
		}
	}
}


void ConvertLinearColorToFColor(const TArray<FLinearColor>& LinearColors, TArray<FColor>& Colors)
{
	Colors.SetNumUninitialized(LinearColors.Num());
	for (int32 Index = 0; Index < LinearColors.Num(); Index++)
	{
		Colors[Index] = LinearColors[Index].ToFColor(true);
	}
}


void URuntimeMeshComponent::CreateMeshSection(int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals,
	const TArray<FVector2D>& UV0, const TArray<FColor>& Colors, const TArray<FRuntimeMeshTangent>& Tangents, bool bCreateCollision,	EUpdateFrequency UpdateFrequency)
{
	auto NewSection = CreateOrResetSectionInternalType(SectionIndex, 1, false);

	// Update the mesh data in the section
	NewSection->UpdateVertexBufferInternal(Vertices, Normals, Tangents, UV0, TArray<FVector2D>(), Colors);
	NewSection->UpdateIndexBuffer(Triangles);

	// Track collision status and update collision information if necessary
	NewSection->CollisionEnabled = bCreateCollision;
	NewSection->UpdateFrequency = UpdateFrequency;

	FinishCreateSectionInternal(SectionIndex, NewSection, true);
}

void URuntimeMeshComponent::CreateMeshSection(int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals,
	const TArray<FVector2D>& UV0, const TArray<FVector2D>& UV1, const TArray<FColor>& Colors, const TArray<FRuntimeMeshTangent>& Tangents,
	bool bCreateCollision, EUpdateFrequency UpdateFrequency)
{
	auto NewSection = CreateOrResetSectionInternalType(SectionIndex, 2, false);

	// Update the mesh data in the section
	NewSection->UpdateVertexBufferInternal(Vertices, Normals, Tangents, UV0, UV1, Colors);
	NewSection->UpdateIndexBuffer(Triangles);

	// Track collision status and update collision information if necessary
	NewSection->CollisionEnabled = bCreateCollision;
	NewSection->UpdateFrequency = UpdateFrequency;

	FinishCreateSectionInternal(SectionIndex, NewSection, true);
}



void URuntimeMeshComponent::UpdateMeshSection(int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<FVector>& Normals, const TArray<FVector2D>& UV0,
	const TArray<FColor>& Colors, const TArray<FRuntimeMeshTangent>& Tangents)
{
	check(SectionIndex < MeshSections.Num() && MeshSections[SectionIndex].IsValid())
	RuntimeMeshSectionPtr& Section = MeshSections[SectionIndex];

	check(Section->bIsInternalSectionType);

	// Tell the section to update the vertex buffer
	TArray<FVector2D> BlankUVs;
	Section->UpdateVertexBufferInternal(Vertices, Normals, Tangents, UV0, BlankUVs, Colors);
	
	FinishUpdateSectionInternal(SectionIndex, Section, Vertices.Num() > 0, false, true);
}

void URuntimeMeshComponent::UpdateMeshSection(int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<FVector>& Normals, const TArray<FVector2D>& UV0,
	const TArray<FVector2D>& UV1, const TArray<FColor>& Colors, const TArray<FRuntimeMeshTangent>& Tangents)
{
	check(SectionIndex < MeshSections.Num() && MeshSections[SectionIndex].IsValid())
	RuntimeMeshSectionPtr& Section = MeshSections[SectionIndex];

	check(Section->bIsInternalSectionType);

	// Tell the section to update the vertex buffer
	Section->UpdateVertexBufferInternal(Vertices, Normals, Tangents, UV0, UV1, Colors);
	
	FinishUpdateSectionInternal(SectionIndex, Section, Vertices.Num() > 0, false, true);
}

void URuntimeMeshComponent::UpdateMeshSection(int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals,
	const TArray<FVector2D>& UV0, const TArray<FColor>& Colors, const TArray<FRuntimeMeshTangent>& Tangents)
{
	check(SectionIndex < MeshSections.Num() && MeshSections[SectionIndex].IsValid())
	RuntimeMeshSectionPtr& Section = MeshSections[SectionIndex];

	check(Section->bIsInternalSectionType);

	// Tell the section to update the vertex buffer
	TArray<FVector2D> BlankUVs;
	Section->UpdateVertexBufferInternal(Vertices, Normals, Tangents, UV0, BlankUVs, Colors);

	if (Triangles.Num() > 0)
	{
		Section->UpdateIndexBuffer(Triangles);
	}

	FinishUpdateSectionInternal(SectionIndex, Section, Vertices.Num() > 0, Triangles.Num() > 0, true);
}

void URuntimeMeshComponent::UpdateMeshSection(int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals,
	const TArray<FVector2D>& UV0, const TArray<FVector2D>& UV1, const TArray<FColor>& Colors, const TArray<FRuntimeMeshTangent>& Tangents)
{
	check(SectionIndex < MeshSections.Num() && MeshSections[SectionIndex].IsValid())
	RuntimeMeshSectionPtr& Section = MeshSections[SectionIndex];

	check(Section->bIsInternalSectionType);
	
	// Tell the section to update the vertex buffer
	Section->UpdateVertexBufferInternal(Vertices, Normals, Tangents, UV0, UV1, Colors);

	if (Triangles.Num() > 0)
	{
		Section->UpdateIndexBuffer(Triangles);
	}

	FinishUpdateSectionInternal(SectionIndex, Section, Vertices.Num() > 0, Triangles.Num() > 0, true);
}


void URuntimeMeshComponent::CreateMeshSection_Blueprint(int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals, const TArray<FRuntimeMeshTangent>& Tangents,
	const TArray<FVector2D>& UV0, const TArray<FVector2D>& UV1, const TArray<FLinearColor>& VertexColors, bool bCreateCollision, EUpdateFrequency UpdateFrequency)
{	
	TArray<FColor> Colors;
	ConvertLinearColorToFColor(VertexColors, Colors);

	CreateMeshSection(SectionIndex, Vertices, Triangles, Normals, UV0, UV1, Colors, Tangents, bCreateCollision, UpdateFrequency);
}

void URuntimeMeshComponent::UpdateMeshSection_Blueprint(int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals, const TArray<FRuntimeMeshTangent>& Tangents,
	const TArray<FVector2D>& UV0, const TArray<FVector2D>& UV1, const TArray<FLinearColor>& VertexColors)
{
	TArray<FColor> Colors;
	ConvertLinearColorToFColor(VertexColors, Colors);

	UpdateMeshSection(SectionIndex, Vertices, Triangles, Normals, UV0, UV1, Colors, Tangents);
}




void URuntimeMeshComponent::ClearMeshSection(int32 SectionIndex)
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_ClearMeshSection);

 	if (SectionIndex < MeshSections.Num() && MeshSections[SectionIndex].IsValid())
 	{
		// Did this section have collision
		bool HadCollision = MeshSections[SectionIndex]->CollisionEnabled;
		bool bWasStaticSection = MeshSections[SectionIndex]->UpdateFrequency == EUpdateFrequency::Infrequent;

		// Clear the section
		MeshSections[SectionIndex].Reset();
		
		// Are we in a batch update?
		if (BatchUpdateInfo.bIsPending)
		{
			// Mark the section to remove
			if (bWasStaticSection)
			{
				BatchUpdateInfo.bRequiresSceneProxyReCreate = true;
			}
			else
			{
				BatchUpdateInfo.AddSectionToRemove(SectionIndex);
			}

			// Flag bounds updates
			BatchUpdateInfo.bRequiresBoundsUpdate = true;

			// Flag collision if necessary
			if (HadCollision)
			{
				BatchUpdateInfo.bRequiresCollisionUpdate = true;
			}

			// Bail as the rest is only for non batched
			return;
		}


		if (SceneProxy && !bWasStaticSection)
		{			
			// Enqueue update on RT
			ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
				FRuntimeMeshSectionUpdate,
				FRuntimeMeshSceneProxy*, RuntimeMeshSceneProxy, (FRuntimeMeshSceneProxy*)SceneProxy,
				int32, SectionIndex, SectionIndex,
				{
					RuntimeMeshSceneProxy->DestroySection_RenderThread(SectionIndex);
				}
			);

		}
		else
		{
			MarkRenderStateDirty();
		}
		
		UpdateLocalBounds();

		// Update our collision info only if this section had any influence on it
		if (HadCollision)
		{
			MarkCollisionDirty();
		}
 	}
}

void URuntimeMeshComponent::ClearAllMeshSections()
{
 	MeshSections.Empty();
	
	// Are we in a batch update?
	if (BatchUpdateInfo.bIsPending)
	{
		// Recreate the scene proxy
		BatchUpdateInfo.bRequiresSceneProxyReCreate = true;

		// Flag bounds updates
		BatchUpdateInfo.bRequiresBoundsUpdate = true;

		BatchUpdateInfo.bRequiresCollisionUpdate = true;
		
		// Bail as the rest is only for non batched
		return;
	}
	
 	UpdateLocalBounds();
 	MarkCollisionDirty();
 	MarkRenderStateDirty();
}

bool URuntimeMeshComponent::GetSectionBoundingBox(int32 SectionIndex, FBox& OutBoundingBox)
{
	if (SectionIndex < MeshSections.Num() && MeshSections[SectionIndex].IsValid())
	{
		OutBoundingBox = MeshSections[SectionIndex]->LocalBoundingBox;
		return true;
	}
	return false;
}

void URuntimeMeshComponent::SetMeshSectionVisible(int32 SectionIndex, bool bNewVisibility)
{
 	if (SectionIndex < MeshSections.Num() && MeshSections[SectionIndex].IsValid())
 	{
 		// Set game thread state
 		MeshSections[SectionIndex]->bIsVisible = bNewVisibility;

		// Are we in a batch update?
		if (BatchUpdateInfo.bIsPending)
		{
			// Flag for property update
			BatchUpdateInfo.AddUpdateForSection(SectionIndex, ERMCBatchSectionUpdateType::VisibilityOrShadowsUpdate);

			// Bail as the rest is only for non batched
			return;
		}
 
		if (SceneProxy)
		{
			// Enqueue command to modify render thread info
			ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER(
				FRuntimeMeshSectionVisibilityUpdate,
				FRuntimeMeshSceneProxy*, RuntimeMeshSceneProxy, (FRuntimeMeshSceneProxy*)SceneProxy,
				int32, SectionIndex, SectionIndex,
				bool, bNewVisibility, bNewVisibility,
				{
					RuntimeMeshSceneProxy->SetSectionVisibility_RenderThread(SectionIndex, bNewVisibility);
				}
			);
		}
		else
		{
			MarkRenderStateDirty();
		}
 	}
}

bool URuntimeMeshComponent::IsMeshSectionVisible(int32 SectionIndex) const
{
	return SectionIndex < MeshSections.Num() && MeshSections[SectionIndex].IsValid() && MeshSections[SectionIndex]->bIsVisible;
}

void URuntimeMeshComponent::SetMeshSectionCastsShadow(int32 SectionIndex, bool bNewCastsShadow)
{
	if (SectionIndex < MeshSections.Num() && MeshSections[SectionIndex].IsValid())
	{
		// Set game thread state
		MeshSections[SectionIndex]->bCastsShadow = bNewCastsShadow;

		// Are we in a batch update?
		if (BatchUpdateInfo.bIsPending)
		{
			// Flag for property update
			BatchUpdateInfo.AddUpdateForSection(SectionIndex, ERMCBatchSectionUpdateType::VisibilityOrShadowsUpdate);

			// Bail as the rest is only for non batched
			return;
		}

		if (SceneProxy && MeshSections[SectionIndex]->UpdateFrequency != EUpdateFrequency::Infrequent)
		{
			// Enqueue command to modify render thread info
			ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER(
				FRuntimeMeshSectionCastsShadowUpdate,
				FRuntimeMeshSceneProxy*, RuntimeMeshSceneProxy, (FRuntimeMeshSceneProxy*)SceneProxy,
				int32, SectionIndex, SectionIndex,
				bool, bNewCastsShadow, bNewCastsShadow,
				{
					RuntimeMeshSceneProxy->SetSectionCastsShadow_RenderThread(SectionIndex, bNewCastsShadow);
				}
			);
		}
		else
		{
			MarkRenderStateDirty();
		}
	}
}

bool URuntimeMeshComponent::IsMeshSectionCastingShadows(int32 SectionIndex) const
{
	return SectionIndex < MeshSections.Num() && MeshSections[SectionIndex].IsValid() && MeshSections[SectionIndex]->bCastsShadow;
}

void URuntimeMeshComponent::SetMeshSectionCollisionEnabled(int32 SectionIndex, bool bNewCollisionEnabled)
{
	if (SectionIndex < MeshSections.Num() && MeshSections[SectionIndex].IsValid())
	{
		auto& Section = MeshSections[SectionIndex];
		if (Section->CollisionEnabled != bNewCollisionEnabled)
		{
			Section->CollisionEnabled = bNewCollisionEnabled;
			
			// Are we in a batch update?
			if (BatchUpdateInfo.bIsPending)
			{
				BatchUpdateInfo.bRequiresCollisionUpdate;
			}
			else
			{
				MarkCollisionDirty();
			}
		}
	}
}

bool URuntimeMeshComponent::IsMeshSectionCollisionEnabled(int32 SectionIndex)
{
	return SectionIndex < MeshSections.Num() && MeshSections[SectionIndex].IsValid() && MeshSections[SectionIndex]->CollisionEnabled;
}



int32 URuntimeMeshComponent::GetNumSections() const
{
	return MeshSections.Num();
}

bool URuntimeMeshComponent::DoesSectionExist(int32 SectionIndex) const
{
	return SectionIndex < MeshSections.Num() && MeshSections[SectionIndex].IsValid();
}


void URuntimeMeshComponent::SetMeshCollisionSection(int32 CollisionSectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles)
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_SetMeshCollisionSection);

	if (MeshCollisionSections.Num() <= CollisionSectionIndex)
	{
		MeshCollisionSections.SetNum(CollisionSectionIndex + 1, false);
	}

	auto& Section = MeshCollisionSections[CollisionSectionIndex];
	Section.VertexBuffer = Vertices;
	Section.IndexBuffer = Triangles;

	// Are we in a batch update?
	if (BatchUpdateInfo.bIsPending)
	{
		BatchUpdateInfo.bRequiresCollisionUpdate;
	}
	else
	{
		MarkCollisionDirty();
	}
}

void URuntimeMeshComponent::ClearMeshCollisionSection(int32 CollisionSectionIndex)
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_ClearMeshCollisionSection);

	if (MeshCollisionSections.Num() <= CollisionSectionIndex)
		return;

	MeshCollisionSections[CollisionSectionIndex].Reset();

	// Are we in a batch update?
	if (BatchUpdateInfo.bIsPending)
	{
		BatchUpdateInfo.bRequiresCollisionUpdate;
	}
	else
	{
		MarkCollisionDirty();
	}
}

void URuntimeMeshComponent::ClearAllMeshCollisionSections()
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_ClearAllMeshCollisionSections);

	MeshCollisionSections.Empty();

	// Are we in a batch update?
	if (BatchUpdateInfo.bIsPending)
	{
		BatchUpdateInfo.bRequiresCollisionUpdate;
	}
	else
	{
		MarkCollisionDirty();
	}
}


void URuntimeMeshComponent::AddCollisionConvexMesh(TArray<FVector> ConvexVerts)
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_AddCollisionConvexMesh);

	if (ConvexVerts.Num() >= 4)
	{
		FRuntimeConvexCollisionSection ConvexSection;
		ConvexSection.VertexBuffer = ConvexVerts;
		ConvexSection.BoundingBox = FBox(ConvexVerts);
		ConvexCollisionSections.Add(ConvexSection);
		

		// Are we in a batch update?
		if (BatchUpdateInfo.bIsPending)
		{
			BatchUpdateInfo.bRequiresCollisionUpdate;
		}
		else
		{
			MarkCollisionDirty();
		}
	}
}

void URuntimeMeshComponent::ClearCollisionConvexMeshes()
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_ClearCollisionConvexMeshes);

	// Empty simple collision info
	ConvexCollisionSections.Empty();


	// Are we in a batch update?
	if (BatchUpdateInfo.bIsPending)
	{
		BatchUpdateInfo.bRequiresCollisionUpdate;
	}
	else
	{
		MarkCollisionDirty();
	}
}

void URuntimeMeshComponent::SetCollisionConvexMeshes(const TArray< TArray<FVector> >& ConvexMeshes)
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_SetCollisionConvexMeshes);

	ConvexCollisionSections.Empty(ConvexMeshes.Num());

	// Create element for each convex mesh
	for (int32 ConvexIndex = 0; ConvexIndex < ConvexMeshes.Num(); ConvexIndex++)
	{
		FRuntimeConvexCollisionSection ConvexSection;
		ConvexSection.VertexBuffer = ConvexMeshes[ConvexIndex];
		ConvexSection.BoundingBox = FBox(ConvexSection.VertexBuffer);
		ConvexCollisionSections.Add(ConvexSection);
	}


	// Are we in a batch update?
	if (BatchUpdateInfo.bIsPending)
	{
		BatchUpdateInfo.bRequiresCollisionUpdate;
	}
	else
	{
		MarkCollisionDirty();
	}
}


void URuntimeMeshComponent::UpdateLocalBounds(bool bMarkRenderTransform)
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_UpdateLocalBounds);
	
	FBox LocalBox(0);

	for (const RuntimeMeshSectionPtr& Section : MeshSections)
	{
		if (Section.IsValid() && Section->bIsVisible)
		{
			LocalBox += Section->LocalBoundingBox;
		}
	}

	LocalBounds = LocalBox.IsValid ? FBoxSphereBounds(LocalBox) : FBoxSphereBounds(FVector(0, 0, 0), FVector(0, 0, 0), 0); // fallback to reset box sphere bounds

	// Update global bounds
	UpdateBounds();

	if (bMarkRenderTransform)
	{
		// Need to send to render thread
		MarkRenderTransformDirty();
	}
}

FPrimitiveSceneProxy* URuntimeMeshComponent::CreateSceneProxy()
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_CreateSceneProxy);

	return new FRuntimeMeshSceneProxy(this);
}

int32 URuntimeMeshComponent::GetNumMaterials() const
{
	return MeshSections.Num();
}

FBoxSphereBounds URuntimeMeshComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	return LocalBounds.TransformBy(LocalToWorld);
}





void URuntimeMeshComponent::EndBatchUpdates()
{
	// Bail if we have no pending updates
	if (!BatchUpdateInfo.bIsPending)
		return;

	// Handle all pending rendering updates..
	if (BatchUpdateInfo.bRequiresSceneProxyReCreate)
	{
		MarkRenderStateDirty();
	}
	else
	{
		FRMCBatchUpdateData* BatchUpdateData = new FRMCBatchUpdateData;

		for (int32 Index = 0; Index < BatchUpdateInfo.SectionUpdates.Num(); Index++)
		{
			// Fetch Update info
			ERMCBatchSectionUpdateType UpdateType = BatchUpdateInfo.SectionUpdates[Index];

			// Bail if this section had no updates
			if (UpdateType == ERMCBatchSectionUpdateType::None)
			{
				continue;
			}

			// Check that we don't have both create and destroy flagged
			check(!(((UpdateType & ERMCBatchSectionUpdateType::Create) != ERMCBatchSectionUpdateType::None) && ((UpdateType & ERMCBatchSectionUpdateType::Destroy) != ERMCBatchSectionUpdateType::None)));


			// Handle Create
			if ((UpdateType & ERMCBatchSectionUpdateType::Create) != ERMCBatchSectionUpdateType::None)
			{
				// Validate section exists
				check(MeshSections.Num() >= Index && MeshSections[Index].IsValid());

				auto CreateSection = new(BatchUpdateData->CreateUpdateSections) FRMCBatchUpdateCreateUpdateSection;

				CreateSection->bIsCreate = true;
				CreateSection->SectionData = MeshSections[Index]->GetSectionCreationData(GetMaterial(Index));
				CreateSection->SectionData->SetTargetSection(Index);
			}
			// Handle destroy
			else if ((UpdateType & ERMCBatchSectionUpdateType::Destroy) != ERMCBatchSectionUpdateType::None)
			{
				// Validate section exists
				check(MeshSections.Num() >= Index && MeshSections[Index].IsValid());

				BatchUpdateData->DestroySections.Add(Index);

			}
			// Handle other update types
			else
			{

				if ((UpdateType & ERMCBatchSectionUpdateType::VerticesUpdate) != ERMCBatchSectionUpdateType::None)
				{
					// Validate section exists
					check(MeshSections.Num() >= Index && MeshSections[Index].IsValid());

					auto CreateSection = new(BatchUpdateData->CreateUpdateSections) FRMCBatchUpdateCreateUpdateSection;

					CreateSection->bIsCreate = false;
					CreateSection->SectionData = MeshSections[Index]->GetSectionUpdateData((UpdateType & ERMCBatchSectionUpdateType::IndicesUpdate) != ERMCBatchSectionUpdateType::None);
				}

				if ((UpdateType & ERMCBatchSectionUpdateType::VisibilityOrShadowsUpdate) != ERMCBatchSectionUpdateType::None)
				{
					// Validate section exists
					check(MeshSections.Num() >= Index && MeshSections[Index].IsValid());

					auto SectionProperties = new(BatchUpdateData->PropertyUpdateSections) FRMCBatchUpdatePropertyUpdateSection;

					auto& Section = MeshSections[Index];

					SectionProperties->TargetSection = Index;
					SectionProperties->bIsVisible = Section->bIsVisible;
					SectionProperties->bCastsShadow = Section->bCastsShadow;
				}
			}
		}



		// Enqueue update on RT
		ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
			FRuntimeMeshBatchUpdateCommand,
			FRuntimeMeshSceneProxy*, RuntimeMeshSceneProxy, (FRuntimeMeshSceneProxy*)SceneProxy,
			FRMCBatchUpdateData*, BatchUpdateData, BatchUpdateData,
			{
				RuntimeMeshSceneProxy->ApplyBatchUpdate_RenderThread(BatchUpdateData);
			}
		);


	}

	// Update collision if necessary
	if (BatchUpdateInfo.bRequiresCollisionUpdate)
	{
		MarkCollisionDirty();
	}

	// Update local bounds if necessary
	if (BatchUpdateInfo.bRequiresBoundsUpdate)
	{
		UpdateLocalBounds(!BatchUpdateInfo.bRequiresSceneProxyReCreate);
	}

	// Clear batch info
	BatchUpdateInfo.Reset();
}









bool URuntimeMeshComponent::GetPhysicsTriMeshData(struct FTriMeshCollisionData* CollisionData, bool InUseAllTriData)
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_GetPhysicsTriMeshData);
 	int32 VertexBase = 0; // Base vertex index for current section
 
	bool HadCollision = false;

	// For each section..
	for (int32 SectionIdx = 0; SectionIdx < MeshSections.Num(); SectionIdx++)
	{ 
		const RuntimeMeshSectionPtr& Section = MeshSections[SectionIdx];

		if (Section.IsValid() && Section->CollisionEnabled)
		{
			// Copy vertex data
			Section->GetAllVertexPositions(CollisionData->Vertices);

			// Copy indices
			const int32 NumTriangles = Section->IndexBuffer.Num() / 3;
			for (int32 TriIdx = 0; TriIdx < NumTriangles; TriIdx++)
			{
				// Add the triangle
				FTriIndices& Triangle = *new (CollisionData->Indices) FTriIndices;
				Triangle.v0 = Section->IndexBuffer[(TriIdx * 3) + 0] + VertexBase;
				Triangle.v1 = Section->IndexBuffer[(TriIdx * 3) + 1] + VertexBase;
				Triangle.v2 = Section->IndexBuffer[(TriIdx * 3) + 2] + VertexBase;

				// Add material info
				CollisionData->MaterialIndices.Add(SectionIdx);
			}

			// Update the vertex base index
			VertexBase = CollisionData->Vertices.Num();
			HadCollision = true;
		}
	}

	for (int32 SectionIdx = 0; SectionIdx < MeshCollisionSections.Num(); SectionIdx++)
	{
		auto& Section = MeshCollisionSections[SectionIdx];
		if (Section.VertexBuffer.Num() > 0 && Section.IndexBuffer.Num() > 0)
		{
			CollisionData->Vertices.Append(Section.VertexBuffer);

			const int32 NumTriangles = Section.IndexBuffer.Num() / 3;
			for (int32 TriIdx = 0; TriIdx < NumTriangles; TriIdx++)
			{
				// Add the triangle
				FTriIndices& Triangle = *new (CollisionData->Indices) FTriIndices;
				Triangle.v0 = Section.IndexBuffer[(TriIdx * 3) + 0] + VertexBase;
				Triangle.v1 = Section.IndexBuffer[(TriIdx * 3) + 1] + VertexBase;
				Triangle.v2 = Section.IndexBuffer[(TriIdx * 3) + 2] + VertexBase;

				// Add material info
				CollisionData->MaterialIndices.Add(SectionIdx);
			}


			VertexBase = CollisionData->Vertices.Num();
			HadCollision = true;
		}
	}
 
 	CollisionData->bFlipNormals = true;
 
 	return HadCollision;
 }

 bool URuntimeMeshComponent::ContainsPhysicsTriMeshData(bool InUseAllTriData) const
 {
 	for (const RuntimeMeshSectionPtr& Section : MeshSections)
 	{
 		if (Section.IsValid() && Section->IndexBuffer.Num() >= 3 && Section->CollisionEnabled)
 		{
 			return true;
 		}
 	}

	for (const auto& Section : MeshCollisionSections)
	{
		if (Section.VertexBuffer.Num() > 0 && Section.IndexBuffer.Num() > 0)
		{
			return true;
		}
	}
 
 	return false;
 }

void URuntimeMeshComponent::EnsureBodySetupCreated()
{
	if (BodySetup == nullptr)
	{
		BodySetup = NewObject<UBodySetup>(this);
		BodySetup->BodySetupGuid = FGuid::NewGuid();

		BodySetup->bGenerateMirroredCollision = false;
		BodySetup->bDoubleSidedGeometry = true;
	}
}

void URuntimeMeshComponent::UpdateCollision()
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_UpdateCollision);

	bool NeedsNewPhysicsState = false;

	// Destroy physics state if it exists
	if (bPhysicsStateCreated)
	{
		DestroyPhysicsState();
		NeedsNewPhysicsState = true;
	}

	// Ensure we have a BodySetup
	EnsureBodySetupCreated();

	// Fill in simple collision convex elements
	BodySetup->AggGeom.ConvexElems.SetNum(ConvexCollisionSections.Num());
	for (int32 Index = 0; Index < ConvexCollisionSections.Num(); Index++)
	{
		FKConvexElem& NewConvexElem = BodySetup->AggGeom.ConvexElems[Index];

		NewConvexElem.VertexData = ConvexCollisionSections[Index].VertexBuffer;
		NewConvexElem.ElemBox = FBox(NewConvexElem.VertexData);
	} 

	// Set trace flag
	BodySetup->CollisionTraceFlag = bUseComplexAsSimpleCollision ? CTF_UseComplexAsSimple : CTF_UseDefault;

	// New GUID as collision has changed
	BodySetup->BodySetupGuid = FGuid::NewGuid();


#if WITH_RUNTIME_PHYSICS_COOKING || WITH_EDITOR
	// Clear current mesh data
	BodySetup->InvalidatePhysicsData();
	// Create new mesh data
	BodySetup->CreatePhysicsMeshes();
#endif // WITH_RUNTIME_PHYSICS_COOKING || WITH_EDITOR

	// Recreate physics state if necessary
	if (NeedsNewPhysicsState)
	{
		CreatePhysicsState();
	}
}

UBodySetup* URuntimeMeshComponent::GetBodySetup()
{
	EnsureBodySetupCreated();
	return BodySetup;
}

void URuntimeMeshComponent::MarkCollisionDirty()
{
	if (!bCollisionDirty)
	{
		bCollisionDirty = true;
		PrePhysicsTick.SetTickFunctionEnable(true);
	}
}


void URuntimeMeshComponent::BakeCollision()
{
	// Bake the collision
	UpdateCollision();

	bCollisionDirty = false;
	PrePhysicsTick.SetTickFunctionEnable(false);
}

void URuntimeMeshComponent::RegisterComponentTickFunctions(bool bRegister)
{
	Super::RegisterComponentTickFunctions(bRegister);

	if (bRegister)
	{
		if (SetupActorComponentTickFunction(&PrePhysicsTick))
		{
			PrePhysicsTick.Target = this;
			PrePhysicsTick.SetTickFunctionEnable(bCollisionDirty);
		}
	}
	else
	{
		if (PrePhysicsTick.IsTickFunctionRegistered())
		{
			PrePhysicsTick.UnRegisterTickFunction();
		}
	}
}


void URuntimeMeshComponent::Serialize(FArchive& Ar)
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_Serialize);
	
	Super::Serialize(Ar);

	Ar.UsingCustomVersion(FRuntimeMeshVersion::GUID);

	if (Ar.CustomVer(FRuntimeMeshVersion::GUID) >= FRuntimeMeshVersion::Initial)
	{
		int32 SectionsCount = bShouldSerializeMeshData ? MeshSections.Num() : 0;
		Ar << SectionsCount;
		if (Ar.IsLoading())
		{
			MeshSections.SetNum(SectionsCount);
		}

		for (int32 Index = 0; Index < SectionsCount; Index++)
		{
			bool IsSectionValid = MeshSections[Index].IsValid();

			// WE can only load/save internal types (we don't know how to serialize arbitrary vertex types.
			if (Ar.IsSaving() && (IsSectionValid && !MeshSections[Index]->bIsInternalSectionType))
			{
				IsSectionValid = false;
			}

			Ar << IsSectionValid;

			if (IsSectionValid)
			{
				if (Ar.CustomVer(FRuntimeMeshVersion::GUID) >= FRuntimeMeshVersion::TemplatedVertexFix)
				{
					int32 NumUVChannels;
					bool WantsHalfPrecisionUVs;

					if (Ar.IsSaving())
					{
						MeshSections[Index]->GetInternalVertexComponents(NumUVChannels, WantsHalfPrecisionUVs);
					}

					Ar << NumUVChannels;
					Ar << WantsHalfPrecisionUVs;

					if (Ar.IsLoading())
					{
						CreateOrResetSectionInternalType(Index, NumUVChannels, WantsHalfPrecisionUVs);
					}

				}
				else
				{
					bool bWantsNormal;
					bool bWantsTangent;
					bool bWantsColor;
					int32 TextureChannels;

					Ar << bWantsNormal;
					Ar << bWantsTangent;
					Ar << bWantsColor;
					Ar << TextureChannels;

					if (Ar.IsLoading())
					{
						CreateOrResetSectionInternalType(Index, TextureChannels, false);
					}
				}

				FRuntimeMeshSectionInterface& SectionPtr = *MeshSections[Index].Get();
				Ar << SectionPtr;

			}
		}
	}

	if (Ar.CustomVer(FRuntimeMeshVersion::GUID) >= FRuntimeMeshVersion::SerializationOptional)
	{		
		
		if (bShouldSerializeMeshData || Ar.IsLoading())
		{
			// Serialize the real data if we want it, also use this path for loading to get anything that was in the last save

			// Serialize the collision data
			Ar << MeshCollisionSections;
			Ar << ConvexCollisionSections;
		}
		else
		{
			// serialize empty arrays if we don't want serialization
			TArray<FRuntimeMeshCollisionSection> NullCollisionSections;
			Ar << NullCollisionSections;
			TArray<FRuntimeConvexCollisionSection> NullConvexBodies;
			Ar << NullConvexBodies;
		}
	}
}	

void URuntimeMeshComponent::PostLoad()
{
	Super::PostLoad();

	// Rebuild collision and local bounds.
	MarkCollisionDirty();
	UpdateLocalBounds();
}