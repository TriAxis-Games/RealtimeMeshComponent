// Copyright (c) 2015-2025 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "RealtimeMeshInterface.h"
#include "RealtimeMeshDataStream.h"
#include "RealtimeMeshSectionGroupConfig.h"

namespace RealtimeMesh
{
	class IRealtimeMeshSimple_v0 : public IRealtimeMesh_v0
	{
	public:
		static FName GetModularFeatureName()
		{
			static FName FeatureName = TEXT("IRealtimeMeshSimple_v0");
			return FeatureName;
		}

		virtual TFuture<ERealtimeMeshProxyUpdateStatus> CreateSectionGroup(const FRealtimeMeshSectionGroupKey& Key,
			const FRealtimeMeshStreamSet& Streams,
			const FRealtimeMeshSectionGroupConfig& InConfig = FRealtimeMeshSectionGroupConfig(),
			bool bShouldAutoCreateSectionsForPolyGroups = true) = 0;
		virtual TFuture<ERealtimeMeshProxyUpdateStatus> CreateSectionGroup(const FRealtimeMeshSectionGroupKey& Key,
			FRealtimeMeshStreamSet&& Streams,
			const FRealtimeMeshSectionGroupConfig& InConfig = FRealtimeMeshSectionGroupConfig(),
			bool bShouldAutoCreateSectionsForPolyGroups = true) = 0;

		virtual TFuture<ERealtimeMeshProxyUpdateStatus> Reset(bool bShouldRecreate) = 0;
	};

	class IRealtimeMeshSimpleInterface_v0 : public IModularFeature
	{
	public:
		virtual ~IRealtimeMeshSimpleInterface_v0() = default;

		static FName GetModularFeatureName()
		{
			static FName FeatureName = TEXT("IRealtimeMeshSimpleInterface_v0");
			return FeatureName;
		}


		virtual TSharedRef<IRealtimeMeshSimple_v0> InitializeMesh(UMeshComponent* MeshComponent) const = 0;
		virtual TSharedRef<IRealtimeMeshSimple_v0> GetMesh(UMeshComponent* MeshComponent) const = 0;
	};


#if !IS_REALTIME_MESH_LIBRARY
	using IRealtimeMeshSimple = IRealtimeMeshSimple_v0;
	using IRealtimeMeshSimpleInterface = IRealtimeMeshSimpleInterface_v0;
#endif
}
