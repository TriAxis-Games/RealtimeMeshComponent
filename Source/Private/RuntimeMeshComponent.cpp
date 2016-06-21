// Copyright 2016 Chris Conway (Koderz). All Rights Reserved.

#include "RuntimeMeshComponentPluginPrivatePCH.h"
#include "RuntimeMeshComponent.h"
#include "RuntimeMeshCore.h"
#include "RuntimeMeshGenericVertex.h"
#include "RuntimeMeshVersion.h"


/** Runtime mesh scene proxy */
class FRuntimeMeshSceneProxy : public FPrimitiveSceneProxy
{
private:
	TUniformBufferRef<FPrimitiveUniformShaderParameters> MeshUniformBuffer;

	// Temporarily holds all section creation data until this proxy is passsed to the RT.
	// After this data is applied this array is cleared.
	TArray<FRuntimeMeshSectionCreateDataInterface*> SectionCreationData;

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
				UMaterialInterface* Material = Component->GetMaterial(SectionIdx);
				if (Material == nullptr)
				{
					Material = UMaterial::GetDefaultMaterial(MD_Surface);
				}


				// Get the section creation data
				auto* SectionData = SourceSection->GetSectionCreationData(Material);
				SectionData->SetTargetSection(SectionIdx);
				SectionCreationData.Add(SectionData);

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

	void CreateRenderThreadResources() override
	{
		FPrimitiveSceneProxy::CreateRenderThreadResources();

		for (auto Section : SectionCreationData)
		{
			CreateSection_RenderThread(Section);
		}
		// The individual items are deleted by CreateSection_RenderThread so just clear the array.
		SectionCreationData.Empty();
	}

	/** Called on render thread to create a new dynamic section. (Static sections are handled differently) */
	void CreateSection_RenderThread(FRuntimeMeshSectionCreateDataInterface* SectionData)
	{
		SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_CreateSection_RenderThread);

		check(IsInRenderingThread());
		check(SectionData);

		int32 SectionIndex = SectionData->GetTargetSection();

		// Make sure the array is big enough
		if (SectionIndex >= Sections.Num())
		{
			Sections.SetNum(SectionIndex + 1, false);
		}
		
		// If a section already exists... destroy it!
		if (FRuntimeMeshSectionProxyInterface* Section = Sections[SectionIndex])
		{			
			delete Section;
		}
		
		// Get the proxy and finish the creation here on the render thread.
		FRuntimeMeshSectionProxyInterface* Section = SectionData->NewProxy;
		Section->FinishCreate_RenderThread(SectionData);		

		// Save ref to new section
		Sections[SectionIndex] = Section;
		
		delete SectionData;
	}

	/** Called on render thread to assign new dynamic data */
  	void UpdateSection_RenderThread(FRuntimeMeshRenderThreadCommandInterface* SectionData)
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

	void UpdateSectionPositionOnly_RenderThread(FRuntimeMeshRenderThreadCommandInterface* SectionData)
	{
		SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_UpdateSectionPositionOnly_RenderThread);

		check(IsInRenderingThread());
		check(SectionData);

		if (SectionData->GetTargetSection() < Sections.Num() && Sections[SectionData->GetTargetSection()] != nullptr)
		{
			Sections[SectionData->GetTargetSection()]->FinishPositionUpdate_RenderThread(SectionData);
		}

		delete SectionData;
	}

	void UpdateSectionProperties_RenderThread(FRuntimeMeshRenderThreadCommandInterface* SectionData)
	{
		SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_UpdateSectionProperties_RenderThread);

		check(IsInRenderingThread());
		check(SectionData);

		int32 SectionIndex = SectionData->GetTargetSection();

		if (SectionIndex < Sections.Num() && Sections[SectionIndex] != nullptr)
		{
			Sections[SectionIndex]->FinishPropertyUpdate_RenderThread(SectionData);
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

	void ApplyBatchUpdate_RenderThread(FRuntimeMeshBatchUpdateData* BatchUpdateData)
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
		for (auto& SectionToCreate : BatchUpdateData->CreateSections)
		{
			CreateSection_RenderThread(static_cast<FRuntimeMeshSectionCreateDataInterface*>(SectionToCreate));
		}

		// Update sections
		for (auto& SectionToUpdate : BatchUpdateData->UpdateSections)
		{
			UpdateSection_RenderThread(SectionToUpdate);
		}		

		// Apply section property updates
		for (auto& SectionToUpdate : BatchUpdateData->PropertyUpdateSections)
		{
			UpdateSectionProperties_RenderThread(SectionToUpdate);
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
			if (Section && !Section->WantsToRenderInStaticPath())
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
			if (Section && Section->WantsToRenderInStaticPath())
			{
				return true;
			}
		}
		return false;
	}

#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 11
	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override
#else
	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) override
#endif
	{
		FPrimitiveViewRelevance Result;
		Result.bDrawRelevance = IsShown(View);
		Result.bShadowRelevance = IsShadowCast(View);
		
		bool bForceDynamicPath = IsRichView(*View->Family) || View->Family->EngineShowFlags.Wireframe || IsSelected() || !IsStaticPathAvailable();
		Result.bStaticRelevance = !bForceDynamicPath && HasStaticSections();
		Result.bDynamicRelevance = bForceDynamicPath || HasDynamicSections();
		
		Result.bRenderInMainPass = ShouldRenderInMainPass();
#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 11
		Result.bUsesLightingChannels = GetLightingChannelMask() != GetDefaultLightingChannelMask();
#endif
		Result.bRenderCustomDepth = ShouldRenderCustomDepth();
		MaterialRelevance.SetPrimitiveViewRelevance(Result);
		return Result;
	}

	void CreateMeshBatch(FMeshBatch& MeshBatch, FRuntimeMeshSectionProxyInterface* Section, FMaterialRenderProxy* WireframeMaterial) const
	{
		Section->CreateMeshBatch(MeshBatch, WireframeMaterial, IsSelected());

		MeshBatch.ReverseCulling = IsLocalToWorldDeterminantNegative();
		MeshBatch.bCanApplyViewModeOverrides = false;
		
		FMeshBatchElement& BatchElement = MeshBatch.Elements[0];
		BatchElement.PrimitiveUniformBuffer = MeshUniformBuffer;
	}
	
	virtual void DrawStaticElements(FStaticPrimitiveDrawInterface* PDI) override
	{
		SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_DrawStaticElements);

		for (FRuntimeMeshSectionProxyInterface* Section : Sections)
		{
			if (Section && Section->ShouldRender() && Section->WantsToRenderInStaticPath())
			{
				FMeshBatch MeshBatch;
				CreateMeshBatch(MeshBatch, Section, nullptr);
				PDI->DrawMesh(MeshBatch, FLT_MAX);
			}
		}
	}

	virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override
	{
		SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_GetDynamicMeshElements);

		// Set up wireframe material (if needed)
		const bool bWireframe = AllowDebugViewmodes() && ViewFamily.EngineShowFlags.Wireframe;

		FColoredMaterialRenderProxy* WireframeMaterialInstance = nullptr;
		if (bWireframe)
		{
			WireframeMaterialInstance = new FColoredMaterialRenderProxy(
				GEngine->WireframeMaterial ? GEngine->WireframeMaterial->GetRenderProxy(IsSelected()) : nullptr,
				FLinearColor(0, 0.5f, 1.f)
				);

			Collector.RegisterOneFrameMaterialProxy(WireframeMaterialInstance);
		}

		// Iterate over sections
		for (FRuntimeMeshSectionProxyInterface* Section : Sections)
		{
			if (Section && Section->ShouldRender())
			{
				// Add the mesh batch to every view it's visible in
				for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
				{
					if (VisibilityMap & (1 << ViewIndex))
					{
						bool bForceDynamicPath = IsRichView(*Views[ViewIndex]->Family) || Views[ViewIndex]->Family->EngineShowFlags.Wireframe || IsSelected() || !IsStaticPathAvailable();

						if (bForceDynamicPath || !Section->WantsToRenderInStaticPath())
						{
							FMeshBatch& MeshBatch = Collector.AllocateMesh();
							CreateMeshBatch(MeshBatch, Section, WireframeMaterialInstance);

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
	/* Ensure target still exists */

#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 11
	bool bIsValid = Target && !Target->IsPendingKillOrUnreachable();
#else
	bool bIsValid = Target && !Target->HasAnyFlags(RF_PendingKill | RF_Unreachable);
#endif

	if (bIsValid)
	{
		FScopeCycleCounterUObject ActorScope(Target);
		Target->BakeCollision();
	}
}

FString FRuntimeMeshComponentPrePhysicsTickFunction::DiagnosticMessage()
{
	return Target->GetFullName() + TEXT("[PrePhysicsTick]");
}



/* Helper for converting an array of FLinearColor to an array of FColors*/
void ConvertLinearColorToFColor(const TArray<FLinearColor>& LinearColors, TArray<FColor>& Colors)
{
	Colors.SetNumUninitialized(LinearColors.Num());
	for (int32 Index = 0; Index < LinearColors.Num(); Index++)
	{
		Colors[Index] = LinearColors[Index].ToFColor(true);
	}
}




URuntimeMeshComponent::URuntimeMeshComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer), bUseComplexAsSimpleCollision(true), bShouldSerializeMeshData(true), bCollisionDirty(true)
{
	// Setup the collision update ticker
	PrePhysicsTick.TickGroup = TG_PrePhysics;
	PrePhysicsTick.bCanEverTick = true;
	PrePhysicsTick.bStartWithTickEnabled = true;

	// Reset the batch state
	BatchState.ResetBatch();
}

TSharedPtr<FRuntimeMeshSectionInterface> URuntimeMeshComponent::CreateOrResetSectionInternalType(int32 SectionIndex, int32 NumUVChannels, bool WantsHalfPrecsionUVs)
{
	switch (NumUVChannels)
	{
	case 1:
		if (WantsHalfPrecsionUVs)
		{
			return CreateOrResetSection<FRuntimeMeshSectionInternal<1, true>>(SectionIndex, false, true);
		}
		else
		{
			return CreateOrResetSection<FRuntimeMeshSectionInternal<1, false>>(SectionIndex, false, true);
		}
		break;
	case 2:
		if (WantsHalfPrecsionUVs)
		{
			return CreateOrResetSection<FRuntimeMeshSectionInternal<2, true>>(SectionIndex, false, true);
		}
		else
		{
			return CreateOrResetSection<FRuntimeMeshSectionInternal<2, false>>(SectionIndex, false, true);
		}
		break;
	case 3:
		if (WantsHalfPrecsionUVs)
		{
			return CreateOrResetSection<FRuntimeMeshSectionInternal<3, true>>(SectionIndex, false, true);
		}
		else
		{
			return CreateOrResetSection<FRuntimeMeshSectionInternal<3, false>>(SectionIndex, false, true);
		}
		break;
	case 4:
		if (WantsHalfPrecsionUVs)
		{
			return CreateOrResetSection<FRuntimeMeshSectionInternal<4, true>>(SectionIndex, false, true);
		}
		else
		{
			return CreateOrResetSection<FRuntimeMeshSectionInternal<4, false>>(SectionIndex, false, true);
		}
		break;
	case 5:
		if (WantsHalfPrecsionUVs)
		{
			return CreateOrResetSection<FRuntimeMeshSectionInternal<5, true>>(SectionIndex, false, true);
		}
		else
		{
			return CreateOrResetSection<FRuntimeMeshSectionInternal<5, false>>(SectionIndex, false, true);
		}
		break;
	case 6:
		if (WantsHalfPrecsionUVs)
		{
			return CreateOrResetSection<FRuntimeMeshSectionInternal<6, true>>(SectionIndex, false, true);
		}
		else
		{
			return CreateOrResetSection<FRuntimeMeshSectionInternal<6, false>>(SectionIndex, false, true);
		}
		break;
	case 7:
		if (WantsHalfPrecsionUVs)
		{
			return CreateOrResetSection<FRuntimeMeshSectionInternal<7, true>>(SectionIndex, false, true);
		}
		else
		{
			return CreateOrResetSection<FRuntimeMeshSectionInternal<7, false>>(SectionIndex, false, true);
		}
		break;
	case 8:
		if (WantsHalfPrecsionUVs)
		{
			return CreateOrResetSection<FRuntimeMeshSectionInternal<8, true>>(SectionIndex, false, true);
		}
		else
		{
			return CreateOrResetSection<FRuntimeMeshSectionInternal<8, false>>(SectionIndex, false, true);
		}
		break;

	default:
		check(false && "Invalid number of UV channels for section.");
		return nullptr;
	}
}


void URuntimeMeshComponent::CreateSectionInternal(int32 SectionIndex)
{
	RuntimeMeshSectionPtr Section = MeshSections[SectionIndex];
	check(Section.IsValid());

	// Use the batch update if one is running
	if (BatchState.IsBatchPending())
	{
		// Mark section created
		BatchState.MarkSectionCreated(SectionIndex, Section->UpdateFrequency == EUpdateFrequency::Infrequent);

		// Flag collision if this section affects it
		if (Section->CollisionEnabled)
		{
			BatchState.MarkCollisionDirty();
		}
		
		// Flag bounds update
		BatchState.MarkBoundsDirty();

		// bail since we don't update directly in this case.
		return;
	}

	// Enqueue the RT command if we already have a SceneProxy
	if (SceneProxy && Section->UpdateFrequency != EUpdateFrequency::Infrequent)
	{
		// Gather all needed update info
		auto* SectionData = Section->GetSectionCreationData(GetSectionMaterial(SectionIndex));
		SectionData->SetTargetSection(SectionIndex);

		// Enqueue update on RT
		ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
			FRuntimeMeshSectionCreate,
			FRuntimeMeshSceneProxy*, RuntimeMeshSceneProxy, (FRuntimeMeshSceneProxy*)SceneProxy,
			FRuntimeMeshSectionCreateDataInterface*, SectionData, SectionData,
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

	// Update overall bounds
	UpdateLocalBounds();

}

void URuntimeMeshComponent::UpdateSectionInternal(int32 SectionIndex, bool bHadVertexPositionsUpdate, bool bHadVertexUpdates, bool bHadIndexUpdates, bool bNeedsBoundsUpdate)
{
	// Ensure that something was updated
	check(bHadVertexPositionsUpdate || bHadVertexUpdates || bNeedsBoundsUpdate);

	check(SectionIndex < MeshSections.Num() && MeshSections[SectionIndex].IsValid());	
	RuntimeMeshSectionPtr Section = MeshSections[SectionIndex];

	/* Make sure this is only flagged if the section is dual buffer */
	bHadVertexPositionsUpdate = Section->IsDualBufferSection() && bHadVertexPositionsUpdate;
	bool bNeedsCollisionUpdate = Section->CollisionEnabled && (bHadVertexPositionsUpdate || (!Section->IsDualBufferSection() && bHadVertexUpdates));
	
	// Use the batch update if one is running
	if (BatchState.IsBatchPending())
	{
		// Mark update for section or promote to proxy recreate if static section
		if (Section->UpdateFrequency == EUpdateFrequency::Infrequent)
		{
			BatchState.MarkRenderStateDirty();
		}
		else
		{
			ERuntimeMeshSectionBatchUpdateType UpdateType = ERuntimeMeshSectionBatchUpdateType::None;
			UpdateType |= bHadVertexPositionsUpdate ? ERuntimeMeshSectionBatchUpdateType::PositionsUpdate : ERuntimeMeshSectionBatchUpdateType::None;
			UpdateType |= bHadVertexUpdates ? ERuntimeMeshSectionBatchUpdateType::VerticesUpdate : ERuntimeMeshSectionBatchUpdateType::None;
			UpdateType |= bHadIndexUpdates ? ERuntimeMeshSectionBatchUpdateType::IndicesUpdate : ERuntimeMeshSectionBatchUpdateType::None;

			BatchState.MarkUpdateForSection(SectionIndex, UpdateType);
		}

		// Flag collision if this section affects it
		if (bNeedsCollisionUpdate)
		{
			BatchState.MarkCollisionDirty();
		}

		// Flag bounds update if needed.
		if (bNeedsBoundsUpdate)
		{
			BatchState.MarkBoundsDirty();
		}

		// bail since we don't update directly in this case.
		return;
	}


	// Send the update to the render thread if the scene proxy exists
	if (SceneProxy && Section->UpdateFrequency != EUpdateFrequency::Infrequent)
	{
		auto* SectionData = Section->GetSectionUpdateData(bHadVertexPositionsUpdate, bHadVertexUpdates, bHadIndexUpdates);
		SectionData->SetTargetSection(SectionIndex);

		// Enqueue update on RT
		ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
			FRuntimeMeshSectionUpdate,
			FRuntimeMeshSceneProxy*, RuntimeMeshSceneProxy, (FRuntimeMeshSceneProxy*)SceneProxy,
			FRuntimeMeshRenderThreadCommandInterface*, SectionData, SectionData,
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

	// Mark collision dirty so it's re-baked at the end of this frame
	if (bNeedsCollisionUpdate)
	{
		MarkCollisionDirty();
	}

	// Update overall bounds if needed
	if (bNeedsBoundsUpdate)
	{
		UpdateLocalBounds();
	}
}

void URuntimeMeshComponent::UpdateSectionVertexPositionsInternal(int32 SectionIndex, bool bNeedsBoundsUpdate)
{
	check(SectionIndex < MeshSections.Num() && MeshSections[SectionIndex].IsValid());
	RuntimeMeshSectionPtr Section = MeshSections[SectionIndex];

	if (SceneProxy)
	{
		auto SectionData = Section->GetSectionPositionUpdateData();
		SectionData->SetTargetSection(SectionIndex);

		// Enqueue command to modify render thread info
		ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
			FRuntimeMeshSectionPositionUpdate,
			FRuntimeMeshSceneProxy*, RuntimeMeshSceneProxy, (FRuntimeMeshSceneProxy*)SceneProxy,
			FRuntimeMeshRenderThreadCommandInterface*, SectionData, SectionData,
			{
				RuntimeMeshSceneProxy->UpdateSectionPositionOnly_RenderThread(SectionData);
			}
		);
	}
	else
	{
		MarkRenderStateDirty();
	}

	if (bNeedsBoundsUpdate)
	{
		UpdateLocalBounds();
	}
}

void URuntimeMeshComponent::UpdateSectionPropertiesInternal(int32 SectionIndex, bool bUpdateRequiresProxyRecreateIfStatic)
{
	check(SectionIndex < MeshSections.Num() && MeshSections[SectionIndex].IsValid());
	RuntimeMeshSectionPtr Section = MeshSections[SectionIndex];

	bool bRequiresRecreate = bUpdateRequiresProxyRecreateIfStatic && Section->UpdateFrequency == EUpdateFrequency::Infrequent;

	// Use the batch update if one is running
	if (BatchState.IsBatchPending())
	{
		if (bRequiresRecreate)
		{
			BatchState.MarkRenderStateDirty();
		}
		else
		{
			BatchState.MarkUpdateForSection(SectionIndex, ERuntimeMeshSectionBatchUpdateType::PropertyUpdate);
		}

		// bail since we don't update directly in this case.
		return;
	}



	if (SceneProxy && !bRequiresRecreate)
	{
		auto SectionData = new FRuntimeMeshSectionPropertyUpdateData();
		SectionData->SetTargetSection(SectionIndex);
		SectionData->bIsVisible = Section->bIsVisible;
		SectionData->bCastsShadow = Section->bCastsShadow;


		// Enqueue command to modify render thread info
		ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
			FRuntimeMeshSectionPropertyUpdate,
			FRuntimeMeshSceneProxy*, RuntimeMeshSceneProxy, (FRuntimeMeshSceneProxy*)SceneProxy,
			FRuntimeMeshRenderThreadCommandInterface*, SectionData, SectionData,
			{
				RuntimeMeshSceneProxy->UpdateSectionProperties_RenderThread(SectionData);
			}
		);
	}
	else
	{
		MarkRenderStateDirty();
	}
}


void URuntimeMeshComponent::UpdateMeshSectionPositionsImmediate(int32 SectionIndex, TArray<FVector>& VertexPositions, ESectionUpdateFlags UpdateFlags)
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_UpdateMeshSectionPositionsImmediate);

	// Validate all update parameters
	RMC_VALIDATE_UPDATEPARAMETERS_DUALBUFFER(SectionIndex);

	// Get section
	RuntimeMeshSectionPtr& Section = MeshSections[SectionIndex];

	// Check dual buffer section status
	if (VertexPositions.Num() != Section->PositionVertexBuffer.Num())
	{
		Log(TEXT("UpdateMeshSection() - Positions cannot change length unless the vertexdata is updated as well."), true);
		return;
	}

	bool bShouldUseMove = (UpdateFlags & ESectionUpdateFlags::MoveArrays) != ESectionUpdateFlags::None;
	bool bNeedsBoundsUpdate = false;

	// Update vertex positions if supplied
	bool bUpdatedVertexPositions = false;
	if (VertexPositions.Num() > 0)
	{
		bNeedsBoundsUpdate = Section->UpdateVertexPositionBuffer(VertexPositions, nullptr, bShouldUseMove);
		bUpdatedVertexPositions = true;
	}
	else
	{
		Log(TEXT("UpdatemeshSection() - Vertex positions empty. They will not be updated."));
	}

	// Finalize section update if we have anything to apply
	if (bUpdatedVertexPositions)
	{
		UpdateSectionVertexPositionsInternal(SectionIndex, bNeedsBoundsUpdate);
	}
}

void URuntimeMeshComponent::UpdateMeshSectionPositionsImmediate(int32 SectionIndex, TArray<FVector>& VertexPositions, const FBox& BoundingBox, ESectionUpdateFlags UpdateFlags)
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_UpdateMeshSectionPositionsImmediate_WithBoundinBox);

	// Validate all update parameters
	RMC_VALIDATE_UPDATEPARAMETERS_DUALBUFFER(SectionIndex);

	// Get section
	RuntimeMeshSectionPtr& Section = MeshSections[SectionIndex];

	// Check dual buffer section status
	if (VertexPositions.Num() != Section->PositionVertexBuffer.Num())
	{
		Log(TEXT("UpdateMeshSection() - Positions cannot change length unless the vertexdata is updated as well."), true);
		return;
	}

	bool bShouldUseMove = (UpdateFlags & ESectionUpdateFlags::MoveArrays) != ESectionUpdateFlags::None;
	bool bNeedsBoundsUpdate = false;

	// Update vertex positions if supplied
	bool bUpdatedVertexPositions = false;
	if (VertexPositions.Num() > 0)
	{
		bNeedsBoundsUpdate = Section->UpdateVertexPositionBuffer(VertexPositions, &BoundingBox, bShouldUseMove);
		bUpdatedVertexPositions = true;
	}
	else
	{
		Log(TEXT("UpdatemeshSection() - Vertex positions empty. They will not be updated."));
	}

	// Finalize section update if we have anything to apply
	if (bUpdatedVertexPositions)
	{
		UpdateSectionVertexPositionsInternal(SectionIndex, bNeedsBoundsUpdate);
	}
}


TArray<FVector>* URuntimeMeshComponent::BeginMeshSectionPositionUpdate(int32 SectionIndex)
{
	// Validate all update parameters
	RMC_VALIDATE_UPDATEPARAMETERS_DUALBUFFER(SectionIndex);

	// Get section
	RuntimeMeshSectionPtr& Section = MeshSections[SectionIndex];
	
	return &Section->PositionVertexBuffer;
}

void URuntimeMeshComponent::EndMeshSectionPositionUpdate(int32 SectionIndex)
{
	// Validate all update parameters
	RMC_VALIDATE_UPDATEPARAMETERS_DUALBUFFER(SectionIndex);
	
	// TODO: Validate that the position buffer is still the same length

	UpdateSectionVertexPositionsInternal(SectionIndex, true);
}

void URuntimeMeshComponent::EndMeshSectionPositionUpdate(int32 SectionIndex, const FBox& BoundingBox)
{
	// Validate all update parameters
	RMC_VALIDATE_UPDATEPARAMETERS_DUALBUFFER(SectionIndex);

	RuntimeMeshSectionPtr& Section = MeshSections[SectionIndex];
	
	bool bNeedsBoundingBoxUpdate = !(Section->LocalBoundingBox == BoundingBox);
	if (bNeedsBoundingBoxUpdate)
	{
		Section->LocalBoundingBox = BoundingBox;
	}
	
	// TODO: Validate that the position buffer is still the same length

	UpdateSectionVertexPositionsInternal(SectionIndex, bNeedsBoundingBoxUpdate);
}








void URuntimeMeshComponent::CreateMeshSection(int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals,
	const TArray<FVector2D>& UV0, const TArray<FColor>& Colors, const TArray<FRuntimeMeshTangent>& Tangents, bool bCreateCollision,	EUpdateFrequency UpdateFrequency)
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_CreateMeshSection);

	// Validate all creation parameters
	RMC_VALIDATE_CREATIONPARAMETERS(SectionIndex, Vertices, Triangles);

	// Create the section
	auto NewSection = CreateOrResetSectionInternalType(SectionIndex, 1, false);

	// Update the mesh data in the section
	NewSection->UpdateVertexBufferInternal(Vertices, Normals, Tangents, UV0, TArray<FVector2D>(), Colors);

	TArray<int32>& TrianglesRef = const_cast<TArray<int32>&>(Triangles);
	NewSection->UpdateIndexBuffer(TrianglesRef, false);

	// Track collision status and update collision information if necessary
	NewSection->CollisionEnabled = bCreateCollision;
	NewSection->UpdateFrequency = UpdateFrequency;

	// Finalize section.
	CreateSectionInternal(SectionIndex);
}

void URuntimeMeshComponent::CreateMeshSection(int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals,
	const TArray<FVector2D>& UV0, const TArray<FVector2D>& UV1, const TArray<FColor>& Colors, const TArray<FRuntimeMeshTangent>& Tangents,
	bool bCreateCollision, EUpdateFrequency UpdateFrequency)
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_CreateMeshSection_DualUV);

	// Validate all creation parameters
	RMC_VALIDATE_CREATIONPARAMETERS(SectionIndex, Vertices, Triangles);

	// Create the section
	auto NewSection = CreateOrResetSectionInternalType(SectionIndex, 2, false);

	// Update the mesh data in the section
	NewSection->UpdateVertexBufferInternal(Vertices, Normals, Tangents, UV0, UV1, Colors);

	TArray<int32>& TrianglesRef = const_cast<TArray<int32>&>(Triangles);
	NewSection->UpdateIndexBuffer(TrianglesRef, false);

	// Track collision status and update collision information if necessary
	NewSection->CollisionEnabled = bCreateCollision;
	NewSection->UpdateFrequency = UpdateFrequency;

	// Finalize section.
	CreateSectionInternal(SectionIndex);
}



void URuntimeMeshComponent::UpdateMeshSection(int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<FVector>& Normals, const TArray<FVector2D>& UV0,
	const TArray<FColor>& Colors, const TArray<FRuntimeMeshTangent>& Tangents)
{
	UpdateMeshSection(SectionIndex, Vertices, TArray<int32>(), Normals, UV0, Colors, Tangents);
}

void URuntimeMeshComponent::UpdateMeshSection(int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<FVector>& Normals, const TArray<FVector2D>& UV0,
	const TArray<FVector2D>& UV1, const TArray<FColor>& Colors, const TArray<FRuntimeMeshTangent>& Tangents)
{
	UpdateMeshSection(SectionIndex, Vertices, TArray<int32>(), Normals, UV0, UV1, Colors, Tangents);
}

void URuntimeMeshComponent::UpdateMeshSection(int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals,
	const TArray<FVector2D>& UV0, const TArray<FColor>& Colors, const TArray<FRuntimeMeshTangent>& Tangents)
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_UpdateMeshSection);

	// Validate all update parameters
	RMC_VALIDATE_UPDATEPARAMETERS_INTERNALSECTION(SectionIndex);

	// Validate section type
	MeshSections[SectionIndex]->GetVertexType()->EnsureEquals<FRuntimeMeshVertex<1>>();

	// Get section
	RuntimeMeshSectionPtr& Section = MeshSections[SectionIndex];
	
	// Tell the section to update the vertex buffer
	bool bHadVertexUpdates = Section->UpdateVertexBufferInternal(Vertices, Normals, Tangents, UV0, TArray<FVector2D>(), Colors);

	bool bHadTriangleUpdates = Triangles.Num() > 0;
	if (bHadTriangleUpdates)
	{
		TArray<int32>& TrianglesRef = const_cast<TArray<int32>&>(Triangles);

		Section->UpdateIndexBuffer(TrianglesRef, false);
	}

	UpdateSectionInternal(SectionIndex, false, bHadVertexUpdates, bHadTriangleUpdates, true);
}

void URuntimeMeshComponent::UpdateMeshSection(int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals,
	const TArray<FVector2D>& UV0, const TArray<FVector2D>& UV1, const TArray<FColor>& Colors, const TArray<FRuntimeMeshTangent>& Tangents)
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_UpdateMeshSection_DualUV);

	// Validate all update parameters
	RMC_VALIDATE_UPDATEPARAMETERS_INTERNALSECTION(SectionIndex);

	// Validate section type
	MeshSections[SectionIndex]->GetVertexType()->EnsureEquals<FRuntimeMeshVertex<2>>();

	// Get section
	RuntimeMeshSectionPtr& Section = MeshSections[SectionIndex];
	
	// Tell the section to update the vertex buffer
	bool bHadVertexUpdates = Section->UpdateVertexBufferInternal(Vertices, Normals, Tangents, UV0, UV1, Colors);

	bool bHadTriangleUpdates = Triangles.Num() > 0;
	if (bHadTriangleUpdates)
	{
		TArray<int32>& TrianglesRef = const_cast<TArray<int32>&>(Triangles);

		Section->UpdateIndexBuffer(TrianglesRef, false);
	}

	UpdateSectionInternal(SectionIndex, false, bHadVertexUpdates, bHadTriangleUpdates, true);
}


void URuntimeMeshComponent::CreateMeshSection_Blueprint(int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals, const TArray<FRuntimeMeshTangent>& Tangents,
	const TArray<FVector2D>& UV0, const TArray<FVector2D>& UV1, const TArray<FLinearColor>& VertexColors, bool bCreateCollision, EUpdateFrequency UpdateFrequency)
{	
	// Convert vertex colors to FColor
	TArray<FColor> Colors;
	ConvertLinearColorToFColor(VertexColors, Colors);

	// Create section
	CreateMeshSection(SectionIndex, Vertices, Triangles, Normals, UV0, UV1, Colors, Tangents, bCreateCollision, UpdateFrequency);
}

void URuntimeMeshComponent::UpdateMeshSection_Blueprint(int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals, const TArray<FRuntimeMeshTangent>& Tangents,
	const TArray<FVector2D>& UV0, const TArray<FVector2D>& UV1, const TArray<FLinearColor>& VertexColors)
{
	// Convert vertex colors to FColor
	TArray<FColor> Colors;
	ConvertLinearColorToFColor(VertexColors, Colors);

	// Update section
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
		
		// Use the batch update if one is running
		if (BatchState.IsBatchPending())
		{
			// Mark section created
			BatchState.MarkSectionDestroyed(SectionIndex, bWasStaticSection);

			// Flag collision if this section affects it
			if (HadCollision)
			{
				BatchState.MarkCollisionDirty();
			}

			// Flag bounds update
			BatchState.MarkBoundsDirty();
			
			// bail since we don't update directly in this case.
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

		// Update our collision info only if this section had any influence on it
		if (HadCollision)
		{
			MarkCollisionDirty();
		}
		
		UpdateLocalBounds();

 	}
}

void URuntimeMeshComponent::ClearAllMeshSections()
{
 	MeshSections.Empty();

	// Use the batch update if one is running
	if (BatchState.IsBatchPending())
	{
		// Mark render state dirty
		BatchState.MarkRenderStateDirty();

		// Flag collision
		BatchState.MarkCollisionDirty();

		// Flag bounds update
		BatchState.MarkBoundsDirty();

		// bail since we don't update directly in this case.
		return;
	}
	
 	MarkRenderStateDirty();
	MarkCollisionDirty();
	UpdateLocalBounds();
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

		// Finish the update
		UpdateSectionPropertiesInternal(SectionIndex, false);
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

		// Finish the update
		UpdateSectionPropertiesInternal(SectionIndex, true);
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
			
			// Use the batch update if one is running
			if (BatchState.IsBatchPending())
			{
				// Mark render state dirty
				BatchState.MarkCollisionDirty();
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
	int32 SectionCount = 0;
	for (int32 Index = 0; Index < MeshSections.Num(); Index++)
	{
		if (MeshSections[Index].IsValid())
		{
			SectionCount++;
		}
	}

	return SectionCount;
}

bool URuntimeMeshComponent::DoesSectionExist(int32 SectionIndex) const
{
	return SectionIndex < MeshSections.Num() && MeshSections[SectionIndex].IsValid();
}

int32 URuntimeMeshComponent::FirstAvailableMeshSectionIndex(int32 SectionIndex) const
{
	for (int32 Index = 0; Index < MeshSections.Num(); Index++)
	{
		if (!MeshSections[Index].IsValid())
		{
			return Index;
		}
	}
	return MeshSections.Num();
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

	// Use the batch update if one is running
	if (BatchState.IsBatchPending())
	{
		// Mark render state dirty
		BatchState.MarkCollisionDirty();
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

	// Use the batch update if one is running
	if (BatchState.IsBatchPending())
	{
		// Mark render state dirty
		BatchState.MarkCollisionDirty();
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

	// Use the batch update if one is running
	if (BatchState.IsBatchPending())
	{
		// Mark render state dirty
		BatchState.MarkCollisionDirty();
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
		

		// Use the batch update if one is running
		if (BatchState.IsBatchPending())
		{
			// Mark render state dirty
			BatchState.MarkCollisionDirty();
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


	// Use the batch update if one is running
	if (BatchState.IsBatchPending())
	{
		// Mark render state dirty
		BatchState.MarkCollisionDirty();
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


	// Use the batch update if one is running
	if (BatchState.IsBatchPending())
	{
		// Mark render state dirty
		BatchState.MarkCollisionDirty();
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
	if (!BatchState.IsBatchPending())
		return;

	// Handle all pending rendering updates..
	if (BatchState.RequiresSceneProxyRecreate())
	{
		MarkRenderStateDirty();
	}
	else
	{
		auto* BatchUpdateData = new FRuntimeMeshBatchUpdateData;

		for (int32 Index = 0; Index <= BatchState.GetMaxSection(); Index++)
		{
			// Skip this section if it has no updates.
			if (!BatchState.HasAnyFlagSet(Index))
			{
				continue;
			}

			// Check that we don't have both create and destroy flagged
			check(!(BatchState.HasFlagSet(Index, ERuntimeMeshSectionBatchUpdateType::Create) && BatchState.HasFlagSet(Index, ERuntimeMeshSectionBatchUpdateType::Destroy)));

			// Handle section created
			if (BatchState.HasFlagSet(Index, ERuntimeMeshSectionBatchUpdateType::Create))
			{
				// Validate section exists
				check(MeshSections.Num() >= Index && MeshSections[Index].IsValid());
				
				UMaterialInterface* Material = GetMaterial(Index);
				if (Material == nullptr)
				{
					Material = UMaterial::GetDefaultMaterial(MD_Surface);
				}

				// Get the section create data and add it to the list
				auto SectionCreateData = MeshSections[Index]->GetSectionCreationData(Material);
				SectionCreateData->SetTargetSection(Index);

				BatchUpdateData->CreateSections.Add(SectionCreateData);
			}
			// Handle destroy
			else if (BatchState.HasFlagSet(Index, ERuntimeMeshSectionBatchUpdateType::Destroy))
			{
				BatchUpdateData->DestroySections.Add(Index);
			}
			// Handle vertex/index updates
			else if (BatchState.HasFlagSet(Index, ERuntimeMeshSectionBatchUpdateType::VerticesUpdate) || BatchState.HasFlagSet(Index, ERuntimeMeshSectionBatchUpdateType::IndicesUpdate))
			{
				// Validate section exists
				check(MeshSections.Num() >= Index && MeshSections[Index].IsValid());

				// Get the section update data and add it to the list.
				bool bHadPositionUpdates = BatchState.HasFlagSet(Index, ERuntimeMeshSectionBatchUpdateType::PositionsUpdate);
				bool bHadVertexUpdates = BatchState.HasFlagSet(Index, ERuntimeMeshSectionBatchUpdateType::VerticesUpdate);
				bool bHadIndexUpdates = BatchState.HasFlagSet(Index, ERuntimeMeshSectionBatchUpdateType::IndicesUpdate);
				auto SectionUpdateData = MeshSections[Index]->GetSectionUpdateData(bHadPositionUpdates, bHadVertexUpdates, bHadIndexUpdates);
				SectionUpdateData->SetTargetSection(Index);

				BatchUpdateData->UpdateSections.Add(SectionUpdateData);
			}
			// Handle property updates
			else if (BatchState.HasFlagSet(Index, ERuntimeMeshSectionBatchUpdateType::PropertyUpdate))
			{
				// Validate section exists
				check(MeshSections.Num() >= Index && MeshSections[Index].IsValid());

				auto SectionProperties = new(BatchUpdateData->PropertyUpdateSections) FRuntimeMeshSectionPropertyUpdateData;

				auto& Section = MeshSections[Index];

				SectionProperties->SetTargetSection(Index);
				SectionProperties->bIsVisible = Section->bIsVisible;
				SectionProperties->bCastsShadow = Section->bCastsShadow;
			}
			else
			{
				// Unknown update type.
				checkNoEntry();
			}
		}



		// Enqueue update on RT
		ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
			FRuntimeMeshBatchUpdateCommand,
			FRuntimeMeshSceneProxy*, RuntimeMeshSceneProxy, (FRuntimeMeshSceneProxy*)SceneProxy,
			FRuntimeMeshBatchUpdateData*, BatchUpdateData, BatchUpdateData,
			{
				RuntimeMeshSceneProxy->ApplyBatchUpdate_RenderThread(BatchUpdateData);
			}
		);


	}

	// Update collision if necessary
	if (BatchState.RequiresCollisionUpdate())
	{
		MarkCollisionDirty();
	}

	// Update local bounds if necessary
	if (BatchState.RequiresBoundsUpdate())
	{
		UpdateLocalBounds(!BatchState.RequiresSceneProxyRecreate());
	}

	// Clear batch info
	BatchState.ResetBatch();
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
		if (Ar.IsLoading() && MeshSections.Num() < SectionsCount)
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