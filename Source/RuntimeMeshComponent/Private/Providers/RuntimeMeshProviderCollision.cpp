// Copyright 2016-2019 Chris Conway (Koderz). All Rights Reserved.


#include "RuntimeMeshProviderCollision.h"
//
//FRuntimeMeshProviderCollisionFromRenderable::FRuntimeMeshProviderCollisionFromRenderable(TWeakObjectPtr<URuntimeMeshProvider> InParent, const FRuntimeMeshProviderProxyPtr& InNextProvider)
//	: LODForMeshCollision(0)
//{
//
//}
//
//FRuntimeMeshProviderCollisionFromRenderable::~FRuntimeMeshProviderCollisionFromRenderable()
//{
//
//}
//
//void FRuntimeMeshProviderCollisionFromRenderable::UpdateProxyParameters(URuntimeMeshProvider* ParentProvider, bool bIsInitialSetup)
//{
//	LODForMeshCollision = ParentProvider->LODForMeshCollision;
//	SectionsForMeshCollision = ParentProvider->SectionsForMeshCollision;
//	CollisionSettings = ParentProvider->CollisionSettings;
//	if (!bIsInitialSetup)
//	{
//		MarkCollisionDirty();
//	}
//}
//
//FRuntimeMeshCollisionSettings FRuntimeMeshProviderCollisionFromRenderable::GetCollisionSettings()
//{
//	return CollisionSettings;
//}
//
//bool FRuntimeMeshProviderCollisionFromRenderable::HasCollisionMesh()
//{
//	return false;
//}
//
//bool FRuntimeMeshProviderCollisionFromRenderable::GetCollisionMesh(FRuntimeMeshCollisionData& CollisionData)
//{
//	return false;
//}
//
//bool FRuntimeMeshProviderCollisionFromRenderable::IsThreadSafe() const
//{
//	return true;
//}
//
//
//
//void URuntimeMeshProviderCollisionFromRenderable::SetCollisionSettings(const FRuntimeMeshCollisionSettings& NewCollisionSettings)
//{
//	CollisionSettings = NewCollisionSettings;
//	MarkProxyParametersDirty();
//}
//
//void URuntimeMeshProviderCollisionFromRenderable::SetCollisionMesh(const FRuntimeMeshCollisionData& NewCollisionMesh)
//{
//	CollisionMesh = NewCollisionMesh;
//	MarkProxyParametersDirty();
//}
//
//void URuntimeMeshProviderCollisionFromRenderable::SetRenderableLODForCollision(int32 LODIndex)
//{
//	LODForMeshCollision = LODIndex;
//	MarkProxyParametersDirty();
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