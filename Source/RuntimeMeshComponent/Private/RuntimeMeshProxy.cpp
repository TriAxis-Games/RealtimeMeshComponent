// Copyright 2016-2019 Chris Conway (Koderz). All Rights Reserved.

#include "RuntimeMeshProxy.h"
#include "RuntimeMeshComponentPlugin.h"
#include "RuntimeMesh.h"


DECLARE_CYCLE_STAT(TEXT("RuntimeMeshProxy - Configure LOD - RenderThread"), STAT_RuntimeMeshProxy_ConfigureLOD_RT, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RuntimeMeshProxy - Clear LOD - RenderThread"), STAT_RuntimeMeshProxy_ClearLOD_RT, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RuntimeMeshProxy - Create Section - RenderThread"), STAT_RuntimeMeshProxy_CreateSection_RT, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RuntimeMeshProxy - Update Section Properties - RenderThread"), STAT_RuntimeMeshProxy_UpdateSectionProperties_RT, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RuntimeMeshProxy - Update Section - RenderThread"), STAT_RuntimeMeshProxy_UpdateSection_RT, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RuntimeMeshProxy - Clear Section - RenderThread"), STAT_RuntimeMeshProxy_ClearSection_RT, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RuntimeMeshProxy - Remove Section - RenderThread"), STAT_RuntimeMeshProxy_RemoveSection_RT, STATGROUP_RuntimeMesh);

FRuntimeMeshProxy::FRuntimeMeshProxy(ERHIFeatureLevel::Type InFeatureLevel)
	: FeatureLevel(InFeatureLevel)
{
	LODs.Empty();
	for (int32 Index = 0; Index < RUNTIMEMESH_MAXLODS; Index++)
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

float FRuntimeMeshProxy::GetScreenSize(int32 LODIndex)
{
	if (LODIndex >= RUNTIMEMESH_MAXLODS)
	{
		return 0;
	}

	return LODs[LODIndex]->GetMaxScreenSize();
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
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMeshProxy_ConfigureLOD_RT);

	check(IsInRenderingThread());

	LODs[LODIndex]->UpdateProperties_RenderThread(InProperties);
}

void FRuntimeMeshProxy::ClearLOD_RenderThread(int32 LODIndex)
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMeshProxy_ClearLOD_RT);

	check(IsInRenderingThread());

	LODs[LODIndex]->Clear_RenderThread();
}

void FRuntimeMeshProxy::CreateSection_RenderThread(int32 LODIndex, int32 SectionId, const FRuntimeMeshSectionProperties& InProperties)
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMeshProxy_CreateSection_RT);

	check(IsInRenderingThread());

	LODs[LODIndex]->CreateSection_RenderThread(SectionId, InProperties);
}

void FRuntimeMeshProxy::UpdateSectionProperties_RenderThread(int32 LODIndex, int32 SectionId, const FRuntimeMeshSectionProperties& InProperties)
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMeshProxy_UpdateSectionProperties_RT);

	check(IsInRenderingThread());

	LODs[LODIndex]->UpdateSectionProperties_RenderThread(SectionId, InProperties);
}

void FRuntimeMeshProxy::UpdateSection_RenderThread(int32 LODIndex, int32 SectionId, const TSharedPtr<FRuntimeMeshRenderableMeshData>& MeshData)
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMeshProxy_UpdateSection_RT);

	check(IsInRenderingThread());

	LODs[LODIndex]->UpdateSectionMesh_RenderThread(SectionId, *MeshData);
}

void FRuntimeMeshProxy::ClearSection_RenderThread(int32 LODIndex, int32 SectionId)
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMeshProxy_ClearSection_RT);

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
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMeshProxy_RemoveSection_RT);

	check(IsInRenderingThread());

	LODs[LODIndex]->RemoveSection_RenderThread(SectionId);
}

