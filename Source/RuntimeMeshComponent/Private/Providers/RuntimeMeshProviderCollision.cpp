// Copyright 2016-2019 Chris Conway (Koderz). All Rights Reserved.


#include "Providers/RuntimeMeshProviderCollision.h"

FRuntimeMeshProviderCollisionFromRenderableProxy::FRuntimeMeshProviderCollisionFromRenderableProxy(TWeakObjectPtr<URuntimeMeshProvider> InParent, const FRuntimeMeshProviderProxyPtr& InNextProvider)
	: FRuntimeMeshProviderProxyPassThrough(InParent, InNextProvider),
	LODForMeshCollision(0)
{

}

FRuntimeMeshProviderCollisionFromRenderableProxy::~FRuntimeMeshProviderCollisionFromRenderableProxy()
{

}

void FRuntimeMeshProviderCollisionFromRenderableProxy::UpdateProxyParameters(URuntimeMeshProvider* ParentProvider, bool bIsInitialSetup)
{
	URuntimeMeshProviderCollisionFromRenderable* CastParent = Cast<URuntimeMeshProviderCollisionFromRenderable>(ParentProvider);
	if (CastParent)
	{

		LODForMeshCollision = CastParent->LODForMeshCollision;
		SectionsForMeshCollision = CastParent->SectionsForMeshCollision;
		CollisionSettings = CastParent->CollisionSettings;
		CollisionMesh = CastParent->CollisionMesh;
		if (!bIsInitialSetup)
		{
			MarkCollisionDirty();
		}
	}
}

FRuntimeMeshCollisionSettings FRuntimeMeshProviderCollisionFromRenderableProxy::GetCollisionSettings()
{
	return CollisionSettings;
}

bool FRuntimeMeshProviderCollisionFromRenderableProxy::HasCollisionMesh()
{
	return SectionsForMeshCollision.Num()>0;
}

bool FRuntimeMeshProviderCollisionFromRenderableProxy::GetCollisionMesh(FRuntimeMeshCollisionData& CollisionData)
{
	if (CollisionMesh.Vertices.Num() > 0 && CollisionMesh.Triangles.Num() > 0) //If the given collision mesh is valid, use it
	{
		CollisionData = CollisionMesh;
		return true;
	}
	if (!NextProvider.IsValid())
	{
		return false;
	}
	auto Temp = NextProvider.Pin();
	for (int32 SectionIdx : SectionsForMeshCollision)
	{
		//Todo : This doesn't use material indices
		FRuntimeMeshRenderableMeshData SectionMesh;
		if (Temp->GetSectionMeshForLOD(LODForMeshCollision, SectionIdx, SectionMesh))
		{
			int32 FirstVertex = CollisionData.Vertices.Num();
			int32 NumVertex = SectionMesh.Positions.Num();
			int32 NumTexCoords = SectionMesh.TexCoords.Num();
			int32 NumChannels = SectionMesh.TexCoords.NumChannels();
			CollisionData.Vertices.SetNum(FirstVertex + NumVertex, false);
			CollisionData.MaterialIndices.SetNum(FirstVertex + NumVertex, false);
			CollisionData.TexCoords.SetNum(NumChannels, FirstVertex + NumVertex, false);
			for (int32 VertIdx = 0; VertIdx < NumVertex; VertIdx++)
			{
				CollisionData.Vertices.SetPosition(FirstVertex + VertIdx, SectionMesh.Positions.GetPosition(VertIdx));
				if (VertIdx >= NumTexCoords)
				{
					continue;
				}
				for (int32 ChannelIdx = 0; ChannelIdx < NumChannels; ChannelIdx++)
				{
					CollisionData.TexCoords.SetTexCoord(ChannelIdx, FirstVertex + VertIdx, SectionMesh.TexCoords.GetTexCoord(VertIdx, ChannelIdx));
				}
			}

			int32 FirstTris = CollisionData.Triangles.Num();
			int32 NumIndices = SectionMesh.Triangles.Num();
			for (int32 TrisIdx = 0; TrisIdx < NumIndices/3; TrisIdx++)
			{
				CollisionData.Triangles.SetTriangleIndices(TrisIdx + FirstTris, 
					SectionMesh.Triangles.GetVertexIndex(NumIndices * 3) + FirstVertex, 
					SectionMesh.Triangles.GetVertexIndex(NumIndices * 3 + 1) + FirstVertex, 
					SectionMesh.Triangles.GetVertexIndex(NumIndices * 3 + 2) + FirstVertex);
			}

			// Add the collision section
			CollisionData.CollisionSources.Emplace(FirstTris, CollisionData.Triangles.Num() - 1, GetParent(), SectionIdx, ERuntimeMeshCollisionFaceSourceType::Renderable);
		}
	}
	return true;
}

bool FRuntimeMeshProviderCollisionFromRenderableProxy::IsThreadSafe() const
{
	return true;
}



void URuntimeMeshProviderCollisionFromRenderable::SetCollisionSettings(const FRuntimeMeshCollisionSettings& NewCollisionSettings)
{
	CollisionSettings = NewCollisionSettings;
	MarkProxyParametersDirty();
}

void URuntimeMeshProviderCollisionFromRenderable::SetCollisionMesh(const FRuntimeMeshCollisionData& NewCollisionMesh)
{
	CollisionMesh = NewCollisionMesh;
	MarkProxyParametersDirty();
}

void URuntimeMeshProviderCollisionFromRenderable::SetRenderableLODForCollision(int32 LODIndex)
{
	LODForMeshCollision = LODIndex;
	MarkProxyParametersDirty();
}

void URuntimeMeshProviderCollisionFromRenderable::SetRenderableSectionAffectsCollision(int32 SectionId, bool bCollisionEnabled)
{
	if (bCollisionEnabled && !SectionsForMeshCollision.Contains(SectionId))
	{
		SectionsForMeshCollision.Add(SectionId);
		MarkProxyParametersDirty();
	}
	else if (!bCollisionEnabled && SectionsForMeshCollision.Contains(SectionId))
	{
		SectionsForMeshCollision.Remove(SectionId);
		MarkProxyParametersDirty();
	}
}