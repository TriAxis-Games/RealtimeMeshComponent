// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#include "RenderProxy/RealtimeMeshGPUBuffer.h"
#include "Data/RealtimeMeshUpdateBuilder.h"

namespace RealtimeMesh
{
#if RMC_ENGINE_ABOVE_5_6
	// Shared 5.6+ construction of the FRHIBufferCreateDesc for this stream's buffer (vertex,
	// index, or null when empty). Both CreateBufferAsyncIfPossible and FinalizeInitialization
	// build byte-identical descs from the same Stream/UsageFlags; only the PROXY-F8 async gate
	// (GRHISupportsMultithreadedResources) and the command list they submit the CreateBuffer to
	// differ, and both of those stay at the call sites.
	FRHIBufferCreateDesc FRealtimeMeshSectionGroupStreamUpdateData::BuildBufferCreateDesc()
	{
		FRHIBufferCreateDesc BufferDesc;

		if (Stream.Num() > 0 && Stream.GetStride() > 0)
		{
			if (GetStreamKey().IsVertexStream())
			{
				BufferDesc = FRHIBufferCreateDesc::CreateVertex(TEXT("RealtimeMeshBuffer-Temp"))
					.SetSize(Stream.GetResourceDataSize())
					.SetStride(Stream.GetStride())
					.SetUsage(UsageFlags | BUF_VertexBuffer | BUF_ShaderResource)
					.SetInitialState(ERHIAccess::VertexOrIndexBuffer | ERHIAccess::SRVMask)
					.SetInitActionResourceArray(&Stream);
			}
			else
			{
				check(GetStreamKey().IsIndexStream());
				BufferDesc = FRHIBufferCreateDesc::CreateIndex(TEXT("RealtimeMeshBuffer-Temp"))
					.SetSize(Stream.GetResourceDataSize())
					.SetStride(Stream.GetElementStride())
					.SetUsage(UsageFlags | BUF_IndexBuffer | BUF_ShaderResource)
					.SetInitialState(ERHIAccess::VertexOrIndexBuffer | ERHIAccess::SRVMask)
					.SetInitActionResourceArray(&Stream);
			}
		}
		else
		{
			BufferDesc = FRHIBufferCreateDesc::CreateNull(TEXT("RealtimeMeshBuffer-Temp"));
		}

		return BufferDesc;
	}
#endif

	void FRealtimeMeshSectionGroupStreamUpdateData::CreateBufferAsyncIfPossible(FRealtimeMeshUpdateContext& UpdateContext)
	{
		// Async (deferred) buffer creation: record the CreateBuffer + CPU upload onto the
		// update context's non-immediate FRHICommandList so the multi-MB memcpy happens off
		// the render thread. This is only legal when the RHI supports recording resource
		// commands (create/lock/unlock) on non-immediate command lists — gated by
		// GRHISupportsMultithreadedResources (D3D12 / Vulkan set it true). On RHIs without
		// it, Buffer stays null and FinalizeInitialization() creates the buffer synchronously
		// on the render thread's command list, so this is a no-op there.
		// NOTE: GRHISupportsAsyncTextureCreation (the previous gate) is a *texture* capability
		// and is the wrong flag for buffers.
		if (GRHISupportsMultithreadedResources)
		{
			auto& RHICmdList = UpdateContext.GetRHICmdList();

#if RMC_ENGINE_ABOVE_5_6
			Buffer = RHICmdList.CreateBuffer(BuildBufferCreateDesc());

#else

			FRHIResourceCreateInfo CreateInfo(TEXT("RealtimeMeshBuffer-Temp"), &Stream);
			CreateInfo.bWithoutNativeResource = Stream.Num() == 0 || Stream.GetStride() == 0;

			if (GetStreamKey().IsVertexStream())
			{
				Buffer = RHICmdList.CreateBuffer(Stream.GetResourceDataSize(), UsageFlags | BUF_VertexBuffer | BUF_ShaderResource,
					Stream.GetStride(), ERHIAccess::SRVMask, CreateInfo);
			}
			else
			{
				check(GetStreamKey().IsIndexStream());
				Buffer = RHICmdList.CreateBuffer(Stream.GetResourceDataSize(), UsageFlags | BUF_IndexBuffer | BUF_ShaderResource,
					Stream.GetElementStride(), ERHIAccess::SRVMask, CreateInfo);
			}
			
#endif			
		}
	}

	void FRealtimeMeshSectionGroupStreamUpdateData::FinalizeInitialization(FRHICommandListBase& RHICmdList)
	{
		if (!Buffer.IsValid())
		{
			check(Stream.GetResourceDataSize());

#if RMC_ENGINE_ABOVE_5_6
			Buffer = RHICmdList.CreateBuffer(BuildBufferCreateDesc());

#else

			FRHIResourceCreateInfo CreateInfo(TEXT("RealtimeMeshBuffer-Temp"), &Stream);
			CreateInfo.bWithoutNativeResource = Stream.Num() == 0 || Stream.GetStride() == 0;

			if (GetStreamKey().IsVertexStream())
			{
				Buffer = RHICmdList.CreateVertexBuffer(Stream.GetResourceDataSize(), UsageFlags | BUF_VertexBuffer | BUF_ShaderResource, CreateInfo);
			}
			else
			{
				check(GetStreamKey().IsIndexStream());
				Buffer =  RHICmdList.CreateIndexBuffer(Stream.GetElementStride(), Stream.GetResourceDataSize(), UsageFlags | BUF_IndexBuffer | BUF_ShaderResource, CreateInfo);
			}
#endif
		}
	}
}
