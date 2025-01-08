// Copyright (c) 2015-2025 TriAxis Games, L.L.C. All Rights Reserved.

#include "RealtimeMeshComponent.h"
#include "RealtimeMeshSimple.h"
#include "Core/RealtimeMeshInterface.h"
#include "Core/RealtimeMeshSimpleInterface.h"
#include "RenderProxy/RealtimeMeshProxyCommandBatch.h"
#include "Core/RealtimeMeshModularFeatures.h"
#include "Data/RealtimeMeshUpdateBuilder.h"

namespace RealtimeMesh
{

	/*class FRealtimeMeshSimpleImpl_v0 : public IRealtimeMeshSimple_v0
	{
		TSharedRef<FRealtimeMeshSimple> Mesh;
	public:
		FRealtimeMeshSimpleImpl_v0(const TSharedRef<FRealtimeMeshSimple>& InMesh)
			: Mesh(InMesh)
		{ }
		
		virtual TFuture<ERealtimeMeshProxyUpdateStatus> CreateSectionGroup(const FRealtimeMeshSectionGroupKey& Key, const RealtimeMesh::FRealtimeMeshStreamSet& Streams, const FRealtimeMeshSectionGroupConfig& InConfig, bool bShouldAutoCreateSectionsForPolyGroups) override
		{
			return Mesh->CreateSectionGroup(Key, Streams, InConfig, bShouldAutoCreateSectionsForPolyGroups);
		}
		
		virtual TFuture<ERealtimeMeshProxyUpdateStatus> CreateSectionGroup(const FRealtimeMeshSectionGroupKey& Key, RealtimeMesh::FRealtimeMeshStreamSet&& Streams, const FRealtimeMeshSectionGroupConfig& InConfig, bool bShouldAutoCreateSectionsForPolyGroups) override
		{
			return Mesh->CreateSectionGroup(Key, MoveTemp(Streams), InConfig, bShouldAutoCreateSectionsForPolyGroups);			
		}

		virtual TFuture<ERealtimeMeshProxyUpdateStatus> Reset(bool bShouldRecreate) override
		{
			FRealtimeMeshUpdateContext UpdateContext(Mesh);
			return Mesh->Reset(UpdateContext, bShouldRecreate);
		}
	};


	
	struct FRealtimeMeshSimpleInterfaceImpl_v0 : public IRealtimeMeshSimpleInterface_v0
	{
	private:
		mutable TMap<TSharedPtr<FRealtimeMeshSimple>, TWeakPtr<FRealtimeMeshSimpleImpl_v0>> MeshMap;

		void CleanupDanglingReferences() const
		{
			for (auto It = MeshMap.CreateIterator(); It; ++It)
			{
				if (!It.Value().IsValid())
				{
					It.RemoveCurrent();
				}
			}
		}
	public:
		virtual TSharedRef<IRealtimeMeshSimple_v0> InitializeMesh(UMeshComponent* MeshComponent) const override
		{
			URealtimeMeshComponent* RealtimeMeshComp = CastChecked<URealtimeMeshComponent>(MeshComponent);
			const URealtimeMeshSimple* MeshSimple = RealtimeMeshComp->InitializeRealtimeMesh<URealtimeMeshSimple>();
			const TSharedRef<FRealtimeMeshSimple> MeshData = MeshSimple->GetMeshAs<FRealtimeMeshSimple>();
			TSharedRef<FRealtimeMeshSimpleImpl_v0> MeshInterface = MakeShared<FRealtimeMeshSimpleImpl_v0>(MeshData);
			MeshMap.Add(MeshData.ToSharedPtr(), MeshInterface);		
			return MeshInterface;
		}

		virtual TSharedRef<IRealtimeMeshSimple_v0> GetMesh(UMeshComponent* MeshComponent) const override
		{
			const URealtimeMeshComponent* RealtimeMeshComp = CastChecked<URealtimeMeshComponent>(MeshComponent);
			const URealtimeMeshSimple* MeshSimple = RealtimeMeshComp->GetRealtimeMeshAs<URealtimeMeshSimple>();
			const TSharedRef<FRealtimeMeshSimple> MeshData = MeshSimple->GetMeshAs<FRealtimeMeshSimple>();

			if (const auto* Ptr = MeshMap.Find(MeshData))
			{
				if (const auto Pinned = Ptr->Pin())
				{
					return Pinned.ToSharedRef();
				}
			}

			TSharedRef<FRealtimeMeshSimpleImpl_v0> MeshInterface = MakeShared<FRealtimeMeshSimpleImpl_v0>(MeshData);
			MeshMap.Add(MeshData.ToSharedPtr(), MeshInterface);			
			return MeshInterface;
		}
	};

	// Register the interface
	TRealtimeMeshModularFeatureRegistration<FRealtimeMeshSimpleInterfaceImpl_v0> GRealtimeMeshSimpleInterfaceImpl_v0;*/
}
