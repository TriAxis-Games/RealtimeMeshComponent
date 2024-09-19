// Copyright (c) 2015-2024 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "RealtimeMeshCore.h"
#include "RealtimeMeshProxyShared.h"
#include "Core/RealtimeMeshKeys.h"
#include "Core/RealtimeMeshSectionConfig.h"
#include "Core/RealtimeMeshStreamRange.h"

namespace RealtimeMesh
{
	class REALTIMEMESHCOMPONENT_API FRealtimeMeshSectionProxy : public TSharedFromThis<FRealtimeMeshSectionProxy>
	{
	private:
		const FRealtimeMeshSharedResourcesRef SharedResources;
		const FRealtimeMeshSectionKey Key;
		FRealtimeMeshSectionConfig Config;
		FRealtimeMeshStreamRange StreamRange;
		FRealtimeMeshDrawMask DrawMask;

	public:
		FRealtimeMeshSectionProxy(const FRealtimeMeshSharedResourcesRef& InSharedResources, const FRealtimeMeshSectionKey InKey);
		virtual ~FRealtimeMeshSectionProxy();

		const FRealtimeMeshSectionKey& GetKey() const { return Key; }
		const FRealtimeMeshSectionConfig& GetConfig() const { return Config; }
		const FRealtimeMeshStreamRange& GetStreamRange() const { return StreamRange; }
		int32 GetMaterialSlot() const { return Config.MaterialSlot; }
		FRealtimeMeshDrawMask GetDrawMask() const { return DrawMask; }

		virtual void UpdateConfig(const FRealtimeMeshSectionConfig& NewConfig);
		virtual void UpdateStreamRange(const FRealtimeMeshStreamRange& NewStreamRange);
		
		virtual bool InitializeMeshBatch(FMeshBatch& MeshBatch, FRHIUniformBuffer* PrimitiveUniformBuffer) const;
		
		
		virtual void UpdateCachedState(FRealtimeMeshSectionGroupProxy& ParentGroup);
		virtual void Reset();
	};
}
