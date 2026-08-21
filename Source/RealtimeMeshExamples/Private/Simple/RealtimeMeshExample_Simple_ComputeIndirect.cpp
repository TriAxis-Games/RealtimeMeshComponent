// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#include "Simple/RealtimeMeshExample_Simple_ComputeIndirect.h"
#include "RealtimeMeshSimple.h"
#include "RealtimeMesh.h"
#include "Mesh/RealtimeMeshBasicShapeTools.h"
#include "RenderProxy/RealtimeMeshProxyCommandBatch.h"
#include "RenderProxy/RealtimeMeshSectionProxy.h"
#include "Containers/ResourceArray.h"
#include "RHICommandList.h"
#include "RHIResources.h"

using namespace RealtimeMesh;

namespace
{
	// Deterministic key shared by OnConstruction (build) and BeginPlay (limit).
	FRealtimeMeshBufferSetKey GetComputeIndirectBoxGroupKey()
	{
		return FRealtimeMeshBufferSetKey::Create(FRealtimeMeshLODKey(0), FName("ComputeIndirectBox"));
	}
}

ARealtimeMeshExample_Simple_ComputeIndirect::ARealtimeMeshExample_Simple_ComputeIndirect()
{
	Description = NSLOCTEXT("RealtimeMeshExamples", "Simple_ComputeIndirect",
		"Compute-mesh hooks (Phase 1): draw only part of a compute-writable box via a GPU indirect-args "
		"buffer, and compare against the same partial draw expressed as a CPU stream range. Both must match.");
}

void ARealtimeMeshExample_Simple_ComputeIndirect::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	URealtimeMeshSimple* RealtimeMesh = GetRealtimeMeshComponent()->InitializeRealtimeMesh<URealtimeMeshSimple>();
	RealtimeMesh->SetupMaterialSlot(0, "DefaultMaterial");

	FRealtimeMeshStreamSet StreamSet;
	URealtimeMeshBasicShapeTools::AppendBoxMesh(StreamSet, BoxRadius, FTransform3f::Identity, 0, FColor::White);

	const FRealtimeMeshBufferSetKey BoxGroupKey = GetComputeIndirectBoxGroupKey();

	// Compute-writable so the geometry buffers are allocated with UAV support (Phase 0). The indirect
	// draw itself (Phase 1) doesn't require the UAV, but this exercises the compute-writable path end
	// to end and matches how later phases create the section.
	const FRealtimeMeshBufferSetConfig Config(ERealtimeMeshSectionDrawType::Static, /*bComputeWritable*/ true);
	RealtimeMesh->CreateBufferSet(BoxGroupKey, StreamSet, Config);
	RealtimeMesh->UpdateSectionConfig(FRealtimeMeshSectionKey::CreateForPolyGroup(BoxGroupKey, 0), FRealtimeMeshSectionConfig(0));

	VerifyMeshBuilt();
}

void ARealtimeMeshExample_Simple_ComputeIndirect::BeginPlay()
{
	Super::BeginPlay();
	ApplyDrawLimit();
}

void ARealtimeMeshExample_Simple_ComputeIndirect::ApplyDrawLimit()
{
	URealtimeMeshSimple* RealtimeMesh = Cast<URealtimeMeshSimple>(GetRealtimeMeshComponent()->GetRealtimeMesh());
	if (!RealtimeMesh)
	{
		return;
	}

	const FRealtimeMeshBufferSetKey GroupKey = GetComputeIndirectBoxGroupKey();
	const FRealtimeMeshSectionKey SectionKey = FRealtimeMeshSectionKey::CreateForPolyGroup(GroupKey, 0);

	const uint32 NumIndices = static_cast<uint32>(FMath::Clamp(VisibleTriangles, 0, 12)) * 3;
	const bool bIndirect = bUseIndirect;

	FRealtimeMeshProxyUpdateBuilder Builder;
	Builder.AddSectionTask(SectionKey,
		[NumIndices, bIndirect](FRHICommandListBase& RHICmdList, FRealtimeMeshSectionProxy& Section)
		{
			if (bIndirect)
			{
				// FRHIDrawIndexedIndirectParameters layout: IndexCountPerInstance, InstanceCount,
				// StartIndexLocation, BaseVertexLocation, StartInstanceLocation.
				TResourceArray<uint32> Init;
				Init.Add(NumIndices);
				Init.Add(1);
				Init.Add(0);
				Init.Add(0);
				Init.Add(0);

				// Canonical indirect-args usage (mirrors FRDGBufferDesc::CreateIndirectDesc); a pure
				// DrawIndirect buffer has no defined Vulkan alignment category and asserts.
				constexpr EBufferUsageFlags IndirectArgsUsage =
						EBufferUsageFlags::Static | EBufferUsageFlags::DrawIndirect | EBufferUsageFlags::UnorderedAccess
						| EBufferUsageFlags::ShaderResource | EBufferUsageFlags::VertexBuffer;
#if RMC_ENGINE_ABOVE_5_6 // FRHIBufferCreateDesc is the 5.6+ buffer-creation API; 5.5 uses FRHIResourceCreateInfo + CreateBuffer(Size,Usage,Stride,State,Info)
				FRHIBufferCreateDesc Desc = FRHIBufferCreateDesc::Create(
						TEXT("RMC_ExampleIndirectArgs"), Init.GetResourceDataSize(), sizeof(uint32),
						IndirectArgsUsage)
					.SetInitialState(ERHIAccess::IndirectArgs)
					.SetInitActionResourceArray(&Init);

				FBufferRHIRef Args = RHICmdList.CreateBuffer(Desc);
#else
				FRHIResourceCreateInfo CreateInfo(TEXT("RMC_ExampleIndirectArgs"), &Init);
				FBufferRHIRef Args = RHICmdList.CreateBuffer(Init.GetResourceDataSize(), IndirectArgsUsage, sizeof(uint32), ERHIAccess::IndirectArgs, CreateInfo);
#endif
				Section.SetIndirectArgs(Args, 0);
			}
			else
			{
				// Express the same limit as a normal CPU-counted draw by shrinking the index range.
				const FRealtimeMeshStreamRange Full = Section.GetStreamRange();
				Section.ClearIndirectArgs();
				Section.UpdateStreamRange(FRealtimeMeshStreamRange(Full.Vertices, FInt32Range(0, static_cast<int32>(NumIndices))));
			}
		}, /*bRequiresProxyRecreate*/ false);

	Builder.Commit(RealtimeMesh->GetMesh()).Next([bIndirect, NumIndices](ERealtimeMeshProxyUpdateStatus Status)
	{
		UE_LOG(LogTemp, Log, TEXT("ComputeIndirect example: applied %s limit of %u indices (status %d)."),
			bIndirect ? TEXT("indirect") : TEXT("CPU-range"), NumIndices, static_cast<int32>(Status));
	});
}
