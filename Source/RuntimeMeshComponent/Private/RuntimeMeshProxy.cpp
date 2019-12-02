// Copyright 2016-2019 Chris Conway (Koderz). All Rights Reserved.

#include "RuntimeMeshProxy.h"
#include "RuntimeMeshComponentPlugin.h"
#include "RuntimeMesh.h"

FRuntimeMeshProxy::FRuntimeMeshProxy(ERHIFeatureLevel::Type InFeatureLevel)
	: FeatureLevel(InFeatureLevel)
{
	LODs.Empty();
	for (int32 Index = 0; Index < RuntimeMesh_MAXLODS; Index++)
	{
		LODs.Add(MakeShareable(new FRuntimeMeshLODProxy(InFeatureLevel), FRuntimeMeshRenderThreadDeleter<FRuntimeMeshLODProxy>()));
	}
}

FRuntimeMeshProxy::~FRuntimeMeshProxy()
{
	// The mesh proxy can only be safely destroyed from the rendering thread.
	// This is so that all the resources can be safely freed correctly.
	check(IsInRenderingThread());
}





void FRuntimeMeshProxy::ConfigureLOD_GameThread(int32 LODIndex, const FRuntimeMeshLODProperties& InProperties)
{
	ENQUEUE_RENDER_COMMAND(FRuntimeMeshProxy_ConfigureLOD)(
		[this, LODIndex, InProperties](FRHICommandListImmediate& RHICmdList)
		{
			ConfigureLOD_RenderThread(LODIndex, InProperties);
		}
	);
}

void FRuntimeMeshProxy::ClearLOD_GameThread(int32 LODIndex)
{
	ENQUEUE_RENDER_COMMAND(FRuntimeMeshProxy_ClearLOD)(
		[this, LODIndex](FRHICommandListImmediate& RHICmdList)
		{
			ClearLOD_RenderThread(LODIndex);
		}
	);
}

void FRuntimeMeshProxy::CreateSection_GameThread(int32 LODIndex, int32 SectionId, const FRuntimeMeshSectionProperties& InProperties)
{
	ENQUEUE_RENDER_COMMAND(FRuntimeMeshProxy_CreateSection)(
		[this, LODIndex, SectionId, InProperties](FRHICommandListImmediate& RHICmdList)
		{
			CreateSection_RenderThread(LODIndex, SectionId, InProperties);
		}
	);	
}

void FRuntimeMeshProxy::UpdateSectionProperties_GameThread(int32 LODIndex, int32 SectionId, const FRuntimeMeshSectionProperties& InProperties)
{
	ENQUEUE_RENDER_COMMAND(FRuntimeMeshProxy_UpdateSectionProperties)(
		[this, LODIndex, SectionId, InProperties](FRHICommandListImmediate& RHICmdList)
		{
			UpdateSectionProperties_RenderThread(LODIndex, SectionId, InProperties);
		}
	);
}

void FRuntimeMeshProxy::UpdateSection_GameThread(int32 LODIndex, int32 SectionId, const TSharedPtr<FRuntimeMeshRenderableMeshData>& MeshData)
{
	ENQUEUE_RENDER_COMMAND(FRuntimeMeshProxy_UpdateSection)(
		[this, LODIndex, SectionId, MeshData](FRHICommandListImmediate& RHICmdList)
		{
			UpdateSection_RenderThread(LODIndex, SectionId, MeshData);
		}
	);
}

void FRuntimeMeshProxy::ClearSection_GameThread(int32 LODIndex, int32 SectionId)
{
	ENQUEUE_RENDER_COMMAND(FRuntimeMeshProxy_ClerSection)(
		[this, LODIndex, SectionId](FRHICommandListImmediate& RHICmdList)
		{
			ClearSection_RenderThread(LODIndex, SectionId);
		}
	);
}

void FRuntimeMeshProxy::RemoveSection_GameThread(int32 LODIndex, int32 SectionId)
{
	ENQUEUE_RENDER_COMMAND(FRuntimeMeshProxy_RemoveSection)(
		[this, LODIndex, SectionId](FRHICommandListImmediate& RHICmdList)
		{
			RemoveSection_RenderThread(LODIndex, SectionId);
		}
	);
}





void FRuntimeMeshProxy::ConfigureLOD_RenderThread(int32 LODIndex, const FRuntimeMeshLODProperties& InProperties)
{
	check(IsInRenderingThread());

	LODs[LODIndex]->UpdateProperties_RenderThread(InProperties);
}

void FRuntimeMeshProxy::ClearLOD_RenderThread(int32 LODIndex)
{
	check(IsInRenderingThread());

	LODs[LODIndex]->Clear_RenderThread();
}

void FRuntimeMeshProxy::CreateSection_RenderThread(int32 LODIndex, int32 SectionId, const FRuntimeMeshSectionProperties& InProperties)
{
	check(IsInRenderingThread());

	LODs[LODIndex]->CreateSection_RenderThread(SectionId, InProperties);
}

void FRuntimeMeshProxy::UpdateSectionProperties_RenderThread(int32 LODIndex, int32 SectionId, const FRuntimeMeshSectionProperties& InProperties)
{
	check(IsInRenderingThread());

	LODs[LODIndex]->UpdateSectionProperties_RenderThread(SectionId, InProperties);
}

void FRuntimeMeshProxy::UpdateSection_RenderThread(int32 LODIndex, int32 SectionId, const TSharedPtr<FRuntimeMeshRenderableMeshData>& MeshData)
{
	check(IsInRenderingThread());

	LODs[LODIndex]->UpdateSectionMesh_RenderThread(SectionId, *MeshData);
}

void FRuntimeMeshProxy::ClearSection_RenderThread(int32 LODIndex, int32 SectionId)
{
	check(IsInRenderingThread());

	if (LODIndex == INDEX_NONE)
	{
		for (int32 Index = 0; Index < LODs.Num(); Index++)
		{
			LODs[Index]->ClearSectionMesh_RenderThread(SectionId);
		}
	}
	else
	{
		LODs[LODIndex]->ClearSectionMesh_RenderThread(SectionId);
	}

}

void FRuntimeMeshProxy::RemoveSection_RenderThread(int32 LODIndex, int32 SectionId)
{
	check(IsInRenderingThread());

	LODs[LODIndex]->RemoveSection_RenderThread(SectionId);
}

