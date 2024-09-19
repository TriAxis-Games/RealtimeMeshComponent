// Copyright (c) 2015-2024 TriAxis Games, L.L.C. All Rights Reserved.

#include "RenderProxy/RealtimeMeshGPUBuffer.h"
#include "Data/RealtimeMeshUpdateBuilder.h"

namespace RealtimeMesh
{
	inline void FRealtimeMeshSectionGroupStreamUpdateData::CreateBufferAsyncIfPossible(FRealtimeMeshUpdateContext& UpdateContext)
	{
		if (GRHISupportsAsyncTextureCreation)
		{
			FRHIAsyncCommandList& RHICmdList = UpdateContext.GetRHICmdList();

			FRHIResourceCreateInfo CreateInfo(TEXT("RealtimeMeshBuffer-Temp"), &Stream);
			CreateInfo.bWithoutNativeResource = Stream.Num() == 0 || Stream.GetStride() == 0;
				
			if (GetStreamKey().IsVertexStream())
			{
				Buffer = RHICmdList->CreateBuffer(Stream.GetResourceDataSize(), UsageFlags | BUF_VertexBuffer | BUF_ShaderResource,
												  Stream.GetStride(), ERHIAccess::SRVMask, CreateInfo);
			}
			else
			{
				check(GetStreamKey().IsIndexStream());
				Buffer = RHICmdList->CreateBuffer(Stream.GetResourceDataSize(), UsageFlags | BUF_IndexBuffer | BUF_ShaderResource,
												  Stream.GetElementStride(), ERHIAccess::SRVMask, CreateInfo);
			}				
		}

			
		/*if (GRHISupportsAsyncTextureCreation && bShouldAttemptAsyncCreation && !Buffer.IsValid())
			{
				FRHIResourceCreateInfo CreateInfo(TEXT("RealtimeMeshBuffer-Temp"), &Stream);
				CreateInfo.bWithoutNativeResource = Stream.Num() == 0 || Stream.GetStride() == 0;

#if RMC_ENGINE_ABOVE_5_3
				FRHIAsyncCommandList CommandList;

				if (GetStreamKey().IsVertexStream())
				{
					Buffer = CommandList->CreateBuffer(Stream.GetResourceDataSize(), UsageFlags | BUF_VertexBuffer | BUF_ShaderResource,
													   Stream.GetStride(), ERHIAccess::SRVMask, CreateInfo);
				}
				else
				{
					check(GetStreamKey().IsIndexStream());
					Buffer = CommandList->CreateBuffer(Stream.GetResourceDataSize(), UsageFlags | BUF_IndexBuffer | BUF_ShaderResource,
													   Stream.GetElementStride(), ERHIAccess::SRVMask, CreateInfo);
				}
#else
				if (GetStreamKey().IsVertexStream())
				{
					Buffer = RHIAsyncCreateVertexBuffer(Stream.GetResourceDataSize(), UsageFlags | BUF_VertexBuffer | BUF_ShaderResource,
						ERHIAccess::SRVMask, CreateInfo);
				}
				else
				{
					check(GetStreamKey().IsIndexStream());
					Buffer = RHIAsyncCreateIndexBuffer(Stream.GetElementStride(), Stream.GetResourceDataSize(), UsageFlags | BUF_IndexBuffer | BUF_ShaderResource,
						ERHIAccess::SRVMask, CreateInfo);
				}
#endif
			}*/
	}

	void FRealtimeMeshSectionGroupStreamUpdateData::FinalizeInitialization(FRHICommandListBase& RHICmdList)
	{
		if (!Buffer.IsValid())
		{
			check(Stream.GetResourceDataSize());
				
			FRHIResourceCreateInfo CreateInfo(TEXT("RealtimeMeshBuffer-Temp"), &Stream);
			CreateInfo.bWithoutNativeResource = Stream.Num() == 0 || Stream.GetStride() == 0;

#if RMC_ENGINE_ABOVE_5_3
			if (GetStreamKey().IsVertexStream())
			{
				Buffer = RHICmdList.CreateVertexBuffer(Stream.GetResourceDataSize(), UsageFlags | BUF_VertexBuffer | BUF_ShaderResource, CreateInfo);
			}
			else
			{
				check(GetStreamKey().IsIndexStream());
				Buffer =  RHICmdList.CreateIndexBuffer(Stream.GetElementStride(), Stream.GetResourceDataSize(), UsageFlags | BUF_IndexBuffer | BUF_ShaderResource, CreateInfo);
			}
#else
				if (GetStreamKey().IsVertexStream())
				{
					Buffer = RHICreateVertexBuffer(Stream.GetResourceDataSize(), UsageFlags | BUF_VertexBuffer | BUF_ShaderResource, CreateInfo);
				}
				else
				{
					check(GetStreamKey().IsIndexStream());
					Buffer = RHICreateIndexBuffer(Stream.GetElementStride(), Stream.GetResourceDataSize(), UsageFlags | BUF_IndexBuffer | BUF_ShaderResource, CreateInfo);
				}
#endif
		}
	}
}
