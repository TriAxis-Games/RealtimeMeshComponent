// Copyright 2016-2019 Chris Conway (Koderz). All Rights Reserved.

#include "RealtimeMeshProxy.h"
#include "RealtimeMeshComponentPlugin.h"
#include "RealtimeMesh.h"

FRealtimeMeshProxy::FRealtimeMeshProxy(ERHIFeatureLevel::Type InFeatureLevel)
	: FeatureLevel(InFeatureLevel)
{
	LODs.Empty();
	for (int32 Index = 0; Index < REALTIMEMESH_MAXLODS; Index++)
	{
		LODs.Add(MakeShareable(new FRealtimeMeshLODProxy(InFeatureLevel), FRealtimeMeshRenderThreadDeleter<FRealtimeMeshLODProxy>()));
	}
}

FRealtimeMeshProxy::~FRealtimeMeshProxy()
{
	// The mesh proxy can only be safely destroyed from the rendering thread.
	// This is so that all the resources can be safely freed correctly.
	check(IsInRenderingThread());
}





void FRealtimeMeshProxy::ConfigureLOD_GameThread(uint8 LODIndex, const FRealtimeMeshLODProperties& InProperties)
{
	ENQUEUE_RENDER_COMMAND(FRealtimeMeshProxy_ConfigureLOD)(
		[this, LODIndex, InProperties](FRHICommandListImmediate& RHICmdList)
		{
			ConfigureLOD_RenderThread(LODIndex, InProperties);
		}
	);
}

void FRealtimeMeshProxy::ClearLOD_GameThread(uint8 LODIndex)
{
	ENQUEUE_RENDER_COMMAND(FRealtimeMeshProxy_ClearLOD)(
		[this, LODIndex](FRHICommandListImmediate& RHICmdList)
		{
			ClearLOD_RenderThread(LODIndex);
		}
	);
}

void FRealtimeMeshProxy::CreateSection_GameThread(uint8 LODIndex, int32 SectionId, const FRealtimeMeshSectionProperties& InProperties)
{
	ENQUEUE_RENDER_COMMAND(FRealtimeMeshProxy_CreateSection)(
		[this, LODIndex, SectionId, InProperties](FRHICommandListImmediate& RHICmdList)
		{
			CreateSection_RenderThread(LODIndex, SectionId, InProperties);
		}
	);	
}

void FRealtimeMeshProxy::UpdateSectionProperties_GameThread(uint8 LODIndex, int32 SectionId, const FRealtimeMeshSectionProperties& InProperties)
{
	ENQUEUE_RENDER_COMMAND(FRealtimeMeshProxy_UpdateSectionProperties)(
		[this, LODIndex, SectionId, InProperties](FRHICommandListImmediate& RHICmdList)
		{
			UpdateSectionProperties_RenderThread(LODIndex, SectionId, InProperties);
		}
	);
}

void FRealtimeMeshProxy::UpdateSection_GameThread(uint8 LODIndex, int32 SectionId, const TSharedPtr<FRealtimeMeshRenderableMeshData>& MeshData)
{
	ENQUEUE_RENDER_COMMAND(FRealtimeMeshProxy_UpdateSection)(
		[this, LODIndex, SectionId, MeshData](FRHICommandListImmediate& RHICmdList)
		{
			UpdateSection_RenderThread(LODIndex, SectionId, MeshData);
		}
	);
}

void FRealtimeMeshProxy::ClearSection_GameThread(uint8 LODIndex, int32 SectionId)
{
	ENQUEUE_RENDER_COMMAND(FRealtimeMeshProxy_ClerSection)(
		[this, LODIndex, SectionId](FRHICommandListImmediate& RHICmdList)
		{
			ClearSection_RenderThread(LODIndex, SectionId);
		}
	);
}

void FRealtimeMeshProxy::RemoveSection_GameThread(uint8 LODIndex, int32 SectionId)
{
	ENQUEUE_RENDER_COMMAND(FRealtimeMeshProxy_RemoveSection)(
		[this, LODIndex, SectionId](FRHICommandListImmediate& RHICmdList)
		{
			RemoveSection_RenderThread(LODIndex, SectionId);
		}
	);
}





void FRealtimeMeshProxy::ConfigureLOD_RenderThread(uint8 LODIndex, const FRealtimeMeshLODProperties& InProperties)
{
	check(IsInRenderingThread());

	LODs[LODIndex]->UpdateProperties_RenderThread(InProperties);
}

void FRealtimeMeshProxy::ClearLOD_RenderThread(uint8 LODIndex)
{
	check(IsInRenderingThread());

	LODs[LODIndex]->Clear_RenderThread();
}

void FRealtimeMeshProxy::CreateSection_RenderThread(uint8 LODIndex, int32 SectionId, const FRealtimeMeshSectionProperties& InProperties)
{
	check(IsInRenderingThread());

	LODs[LODIndex]->CreateSection_RenderThread(SectionId, InProperties);
}

void FRealtimeMeshProxy::UpdateSectionProperties_RenderThread(uint8 LODIndex, int32 SectionId, const FRealtimeMeshSectionProperties& InProperties)
{
	check(IsInRenderingThread());

	LODs[LODIndex]->UpdateSectionProperties_RenderThread(SectionId, InProperties);
}

void FRealtimeMeshProxy::UpdateSection_RenderThread(uint8 LODIndex, int32 SectionId, const TSharedPtr<FRealtimeMeshRenderableMeshData>& MeshData)
{
	check(IsInRenderingThread());

	LODs[LODIndex]->UpdateSectionMesh_RenderThread(SectionId, *MeshData);
}

void FRealtimeMeshProxy::ClearSection_RenderThread(uint8 LODIndex, int32 SectionId)
{
	check(IsInRenderingThread());

	LODs[LODIndex]->ClearSectionMesh_RenderThread(SectionId);
}

void FRealtimeMeshProxy::RemoveSection_RenderThread(uint8 LODIndex, int32 SectionId)
{
	check(IsInRenderingThread());

	LODs[LODIndex]->RemoveSection_RenderThread(SectionId);
}

