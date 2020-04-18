//// Copyright 2016-2020 Chris Conway (Koderz). All Rights Reserved.
//
//
//#include "Providers/RuntimeMeshProviderCollision.h"
//
////
////FRuntimeMeshProviderCollisionFromRenderableProxy::~FRuntimeMeshProviderCollisionFromRenderableProxy()
////{
////
////}
////
////void FRuntimeMeshProviderCollisionFromRenderableProxy::UpdateProxyParameters(URuntimeMeshProvider* ParentProvider, bool bIsInitialSetup)
////{
////	URuntimeMeshProviderCollisionFromRenderable* CastParent = Cast<URuntimeMeshProviderCollisionFromRenderable>(ParentProvider);
////	if (CastParent)
////	{
////
////		LODForMeshCollision = CastParent->LODForMeshCollision;
////		SectionsForMeshCollision = CastParent->SectionsForMeshCollision;
////		CollisionSettings = CastParent->CollisionSettings;
////		CollisionMesh = CastParent->CollisionMesh;
////		if (!bIsInitialSetup)
////		{
////			MarkCollisionDirty();
////		}
////	}
////}
////
////FRuntimeMeshCollisionSettings FRuntimeMeshProviderCollisionFromRenderableProxy::GetCollisionSettings()
////{
////	return CollisionSettings;
////}
////
////bool FRuntimeMeshProviderCollisionFromRenderableProxy::HasCollisionMesh()
////{
////	return SectionsForMeshCollision.Num()>0;
////}
////
////bool FRuntimeMeshProviderCollisionFromRenderableProxy::GetCollisionMesh(FRuntimeMeshCollisionData& CollisionData)
////{
////	if (CollisionMesh.Vertices.Num() > 0 && CollisionMesh.Triangles.Num() > 0) //If the given collision mesh is valid, use it
////	{
////		CollisionData = CollisionMesh;
////		return true;
////	}
////	if (!NextProvider.IsValid())
////	{
////		return false;
////	}
////	for (int32 SectionIdx : SectionsForMeshCollision)
////	{
////		//Todo : This doesn't use material indices
////		FRuntimeMeshRenderableMeshData SectionMesh;
////		if (NextProvider->GetSectionMeshForLOD(LODForMeshCollision, SectionIdx, SectionMesh))
////		{
////			int32 FirstVertex = CollisionData.Vertices.Num();
////			int32 NumVertex = SectionMesh.Positions.Num();
////			int32 NumTexCoords = SectionMesh.TexCoords.Num();
////			int32 NumChannels = SectionMesh.TexCoords.NumChannels();
////			CollisionData.Vertices.SetNum(FirstVertex + NumVertex, false);
////			CollisionData.TexCoords.SetNum(NumChannels, FirstVertex + NumVertex, false);
////			for (int32 VertIdx = 0; VertIdx < NumVertex; VertIdx++)
////			{
////				CollisionData.Vertices.SetPosition(FirstVertex + VertIdx, SectionMesh.Positions.GetPosition(VertIdx));
////				if (VertIdx >= NumTexCoords)
////				{
////					continue;
////				}
////				for (int32 ChannelIdx = 0; ChannelIdx < NumChannels; ChannelIdx++)
////				{
////					CollisionData.TexCoords.SetTexCoord(ChannelIdx, FirstVertex + VertIdx, SectionMesh.TexCoords.GetTexCoord(VertIdx, ChannelIdx));
////				}
////			}
////
////			int32 FirstTris = CollisionData.Triangles.Num();
////			int32 NumTriangles = SectionMesh.Triangles.NumTriangles();
////			CollisionData.Triangles.SetNum(FirstTris + NumTriangles, false);
////			CollisionData.MaterialIndices.SetNum(FirstTris + NumTriangles, false);
////			for (int32 TrisIdx = 0; TrisIdx < NumTriangles; TrisIdx++)
////			{
////				int32 Index0 = SectionMesh.Triangles.GetVertexIndex(TrisIdx * 3) + FirstVertex;
////				int32 Index1 = SectionMesh.Triangles.GetVertexIndex(TrisIdx * 3 + 1) + FirstVertex;
////				int32 Index2 = SectionMesh.Triangles.GetVertexIndex(TrisIdx * 3 + 2) + FirstVertex;
////
////
////				CollisionData.Triangles.SetTriangleIndices(TrisIdx + FirstTris, Index0, Index1, Index2);
////				CollisionData.MaterialIndices.SetMaterialIndex(TrisIdx + FirstTris, 0 /* TODO: Get section material index */);
////			}
////
////
////			CollisionData.CollisionSources.Emplace(FirstTris, CollisionData.Triangles.Num() - 1, GetParent(), SectionIdx, ERuntimeMeshCollisionFaceSourceType::Renderable);
////		}
////	}
////	return true;
////}
////
////bool FRuntimeMeshProviderCollisionFromRenderableProxy::IsThreadSafe() const
////{
////	return true;
////}
////
////
////
//void URuntimeMeshProviderCollisionFromRenderable::SetCollisionSettings(const FRuntimeMeshCollisionSettings& NewCollisionSettings)
//{
//	SyncRoot.Lock();
//	CollisionSettings = NewCollisionSettings;
//	SyncRoot.Unlock();
//	MarkCollisionDirty();
//}
//
//void URuntimeMeshProviderCollisionFromRenderable::SetCollisionMesh(const FRuntimeMeshCollisionData& NewCollisionMesh)
//{
//	SyncRoot.Lock();
//	CollisionMesh = NewCollisionMesh;
//	SyncRoot.Unlock();
//	MarkCollisionDirty();
//}
//
//void URuntimeMeshProviderCollisionFromRenderable::SetRenderableLODForCollision(int32 LODIndex)
//{
//	SyncRoot.Lock();
//	LODForMeshCollision = LODIndex;
//	SyncRoot.Unlock();
//	MarkCollisionDirty();
//}
//
//void URuntimeMeshProviderCollisionFromRenderable::SetRenderableSectionAffectsCollision(int32 SectionId, bool bCollisionEnabled)
//{
//	if (bCollisionEnabled && !SectionsForMeshCollision.Contains(SectionId))
//	{
//		SectionsForMeshCollision.Add(SectionId);
//		MarkProxyParametersDirty();
//	}
//	else if (!bCollisionEnabled && SectionsForMeshCollision.Contains(SectionId))
//	{
//		SectionsForMeshCollision.Remove(SectionId);
//		MarkProxyParametersDirty();
//	}
//}
//
//URuntimeMeshProviderCollisionFromRenderable::URuntimeMeshProviderCollisionFromRenderable()
//	: LODForMeshCollision(0)
//{
//
//}
//
//bool URuntimeMeshProviderCollisionFromRenderable::GetSectionMeshForLOD_Implementation(int32 LODIndex, int32 SectionId, FRuntimeMeshRenderableMeshData& MeshData)
//{
//	bool bResult = URuntimeMeshProvider::GetSectionMeshForLOD_Implementation(LODIndex, SectionId, MeshData);
//
//	if (bResult)
//	{
//		TArray<FVector>* FoundSection = RenderableCollisionData.Find(SectionId);
//		if (FoundSection)
//		{
//			(*FoundSection) = MoveTemp(MeshData.Positions.GetCopy());
//		}
//	}
//
//	return bResult;
//}
//
//bool URuntimeMeshProviderCollisionFromRenderable::GetAllSectionsMeshForLOD_Implementation(int32 LODIndex, TMap<int32, FRuntimeMeshSectionData>& MeshDatas)
//{
//	bool bResult = URuntimeMeshProvider::GetAllSectionsMeshForLOD_Implementation(LODIndex, MeshDatas);
//
//	if (bResult)
//	{
//		for (const auto& Entry : MeshDatas)
//		{
//			TArray<FVector>* FoundSection = RenderableCollisionData.Find(Entry.Key);
//			if (FoundSection)
//			{
//				(*FoundSection) = MoveTemp(Entry.Value.MeshData.Positions.GetCopy());
//			}
//		}
//
//	}
//
//	return bResult;
//}
//
//void URuntimeMeshProviderCollisionFromRenderable::ClearSection_Implementation(int32 LODIndex, int32 SectionId)
//{
//	if (LODIndex == LODForMeshCollision)
//	{
//		TArray<FVector>* FoundSection = RenderableCollisionData.Find(SectionId);
//		if (FoundSection)
//		{
//			FoundSection->Empty();
//		}
//	}
//	URuntimeMeshProvider::ClearSection_Implementation(LODIndex, SectionId);
//}
//
//void URuntimeMeshProviderCollisionFromRenderable::RemoveSection_Implementation(int32 LODIndex, int32 SectionId)
//{
//	if (LODIndex == LODForMeshCollision)
//	{
//		TArray<FVector>* FoundSection = RenderableCollisionData.Find(SectionId);
//		if (FoundSection)
//		{
//			FoundSection->Empty();
//		}
//	}
//	URuntimeMeshProvider::RemoveSection_Implementation(LODIndex, SectionId);
//}
//
