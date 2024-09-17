// Copyright (c) 2015-2024 TriAxis Games, L.L.C. All Rights Reserved.

#include "RealtimeMeshComponent.h"
#include "Core/RealtimeMeshCollision.h"
#include "Core/RealtimeMeshModularFeatures.h"


namespace RealtimeMesh
{
	struct FRealtimeMeshCollisionToolsImpl_v0 : public IRealtimeMeshCollisionTools_v0
	{
		virtual bool FindCollisionUVRealtimeMesh(const struct FHitResult& Hit, int32 UVChannel, FVector2D& UV) const override
		{
			return URealtimeMeshCollisionTools::FindCollisionUVRealtimeMesh(Hit, UVChannel, UV);
		}
		virtual void CookConvexHull(FRealtimeMeshCollisionConvex& ConvexHull) const override
		{
			URealtimeMeshCollisionTools::CookConvexHull(ConvexHull);
		}
		virtual void CookComplexMesh(FRealtimeMeshCollisionMesh& CollisionMesh) const override
		{
			URealtimeMeshCollisionTools::CookComplexMesh(CollisionMesh);
		}
	};
	
	// Register the interface
	TRealtimeMeshModularFeatureRegistration<FRealtimeMeshCollisionToolsImpl_v0> GRealtimeMeshCollisionToolsImpl_v0;
}
