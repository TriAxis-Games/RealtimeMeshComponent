// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "Mesh/RealtimeMeshDataTypes.h"
#include "Containers/ResourceArray.h"
#include "Mesh/RealtimeMeshDataStream.h"
#if RMC_ENGINE_ABOVE_5_2
#include "RHIResourceUpdates.h"
#include "DataDrivenShaderPlatformInfo.h"
#endif

namespace RealtimeMesh
{
	class FRealtimeMeshGPUBuffer;
	class FRealtimeMeshVertexBuffer;
	class FRealtimeMeshIndexBuffer;

	struct REALTIMEMESHCOMPONENT_API FRealtimeMeshSectionGroupStreamUpdateData
	{
	private:
		FRealtimeMeshStream Stream;
		EBufferUsageFlags UsageFlags;
		FBufferRHIRef Buffer;

	public:
		FRealtimeMeshSectionGroupStreamUpdateData(FRealtimeMeshStream&& InStream)
			: Stream(MoveTemp(InStream))
			  , UsageFlags(EBufferUsageFlags::None)
		{
		}

		FRealtimeMeshSectionGroupStreamUpdateData(const FRealtimeMeshStream& InStream)
			: Stream(InStream)
			  , UsageFlags(EBufferUsageFlags::None)
		{
		}

		const FResourceArrayInterface* GetResource() const { return &Stream; }
		FRealtimeMeshBufferLayout GetBufferLayout() const { return Stream.GetLayout(); }
		FRealtimeMeshStreamKey GetStreamKey() const { return Stream.GetStreamKey(); }
		int32 GetNumElements() const { return Stream.Num(); }
		EBufferUsageFlags GetUsageFlags() const { return UsageFlags; }
		FBufferRHIRef& GetBuffer() { return Buffer; }

		void ConfigureBuffer(EBufferUsageFlags InUsageFlags, bool bShouldAttemptAsyncCreation = true)
		{
			UsageFlags = InUsageFlags;
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

		void InitializeIfRequired()
		{
			if (!Buffer.IsValid())
			{
				check(Stream.GetResourceDataSize());
				
				FRHIResourceCreateInfo CreateInfo(TEXT("RealtimeMeshBuffer-Temp"), &Stream);
				CreateInfo.bWithoutNativeResource = Stream.Num() == 0 || Stream.GetStride() == 0;

#if RMC_ENGINE_ABOVE_5_3
				FRHIAsyncCommandList CommandList;
				if (GetStreamKey().IsVertexStream())
				{
					Buffer = CommandList->CreateVertexBuffer(Stream.GetResourceDataSize(), UsageFlags | BUF_VertexBuffer | BUF_ShaderResource, CreateInfo);
				}
				else
				{
					check(GetStreamKey().IsIndexStream());
					Buffer =  CommandList->CreateIndexBuffer(Stream.GetElementStride(), Stream.GetResourceDataSize(), UsageFlags | BUF_IndexBuffer | BUF_ShaderResource, CreateInfo);
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
	};

	using FRealtimeMeshSectionGroupStreamUpdateDataRef = TSharedRef<FRealtimeMeshSectionGroupStreamUpdateData>;

	class REALTIMEMESHCOMPONENT_API FRealtimeMeshGPUBuffer
	{
	protected:
		FRealtimeMeshBufferLayout BufferLayout;
		FRealtimeMeshElementTypeDetails ElementDetails;
		FRealtimeMeshBufferMemoryLayout MemoryLayout;
		uint32 BufferNum;
		EBufferUsageFlags UsageFlags;

#if WITH_EDITOR
		FString BufferName;
#endif

	public:
		FRealtimeMeshGPUBuffer(const TCHAR* InBufferName, const FRealtimeMeshBufferLayout& InBufferLayout)
			: BufferLayout(InBufferLayout)
			, ElementDetails(FRealtimeMeshBufferLayoutUtilities::GetElementTypeDetails(InBufferLayout.GetElementType()))
			, MemoryLayout(FRealtimeMeshBufferLayoutUtilities::GetBufferLayoutMemoryLayout(InBufferLayout))
			, BufferNum(0)
			, UsageFlags(BUF_Static | BUF_ShaderResource)
#if WITH_EDITOR
			, BufferName(InBufferName)
#endif
		{
		}

		virtual ~FRealtimeMeshGPUBuffer() = default;

		FORCEINLINE FString GetBufferName() const
		{
#if WITH_EDITOR
			return BufferName;
#else
			return FString();
#endif
		}

		virtual ERealtimeMeshStreamType GetStreamType() const = 0;
		virtual void InitializeResources() = 0;
		virtual void ReleaseUnderlyingResource() = 0;
		virtual bool IsResourceInitialized() const = 0;

		FORCEINLINE const FRealtimeMeshBufferLayout& GetBufferLayout() const { return BufferLayout; }
		FORCEINLINE EPixelFormat GetElementFormat() const { return ElementDetails.GetPixelFormat(); }
		FORCEINLINE int32 GetElementStride() const { return GPixelFormats[GetElementFormat()].BlockBytes; }
		FORCEINLINE uint32 GetStride() const { return MemoryLayout.GetStride(); }
		FORCEINLINE int32 Num() const { return BufferNum; }

		FORCEINLINE int32 NumElements() const { return BufferLayout.GetNumElements(); }


		static constexpr int32 RHIUpdateBatchSize = 16;

		virtual void ApplyBufferUpdate(TRHIResourceUpdateBatcher<RHIUpdateBatchSize>& Batcher, const FRealtimeMeshSectionGroupStreamUpdateDataRef& UpdateData)
		{
			check(BufferLayout == UpdateData->GetBufferLayout());
			BufferNum = UpdateData->GetNumElements();
			UsageFlags = UpdateData->GetUsageFlags();

#if WITH_EDITOR
			BufferName = UpdateData->GetStreamKey().GetName().ToString();
#endif

			check(BufferLayout.IsValid());
			check(GetStride() > 0);
		}
	};

	class REALTIMEMESHCOMPONENT_API FRealtimeMeshVertexBuffer : public FRealtimeMeshGPUBuffer, public FVertexBufferWithSRV
	{
	public:
		FRealtimeMeshVertexBuffer(const FRealtimeMeshBufferLayout& InBufferLayout) : FRealtimeMeshGPUBuffer(TEXT("RealtimeMesh-VertexBuffer"), InBufferLayout)
		{
		}

		virtual FString GetFriendlyName() const override { return GetBufferName(); }

		virtual ERealtimeMeshStreamType GetStreamType() const override { return ERealtimeMeshStreamType::Vertex; }

		virtual void InitializeResources() override
		{
#if RMC_ENGINE_ABOVE_5_3
			InitResource(FRHICommandListImmediate::Get());
#else
			InitResource();
#endif
		}

		virtual void ReleaseUnderlyingResource() override { ReleaseResource(); }

		virtual bool IsResourceInitialized() const override { return IsInitialized(); }

		/** Gets the format of the vertex */
		FORCEINLINE EVertexElementType GetVertexType() const { return ElementDetails.GetVertexType(); }

#if RMC_ENGINE_ABOVE_5_3
		virtual void InitRHI(FRHICommandListBase& RHICmdList) override
		{
			FRHIResourceCreateInfo CreateInfo(TEXT("RealtimeMeshBuffer-Vertex-Init"));
			CreateInfo.bWithoutNativeResource = true;
			VertexBufferRHI = RHICmdList.CreateVertexBuffer(0, BUF_VertexBuffer | BUF_Static, CreateInfo);
			if (VertexBufferRHI && RHISupportsManualVertexFetch(GMaxRHIShaderPlatform))
			{
				ShaderResourceViewRHI = RHICmdList.CreateShaderResourceView(FShaderResourceViewInitializer(VertexBufferRHI, GetElementFormat()));
			}
		}
#else
		virtual void InitRHI() override
		{
			FRHIResourceCreateInfo CreateInfo(TEXT("RealtimeMeshBuffer-Vertex-Init"));
			CreateInfo.bWithoutNativeResource = true;
			VertexBufferRHI = RHICreateVertexBuffer(0, BUF_VertexBuffer | BUF_Static, CreateInfo);
			if (VertexBufferRHI && RHISupportsManualVertexFetch(GMaxRHIShaderPlatform))
			{
				ShaderResourceViewRHI = RHICreateShaderResourceView(FShaderResourceViewInitializer(nullptr, GetElementFormat()));
			}
		}
#endif

		virtual void ApplyBufferUpdate(TRHIResourceUpdateBatcher<RHIUpdateBatchSize>& Batcher,
		                               const FRealtimeMeshSectionGroupStreamUpdateDataRef& UpdateData) override
		{
			check(IsInitialized());

			FRealtimeMeshGPUBuffer::ApplyBufferUpdate(Batcher, UpdateData);
			{
				VertexBufferRHI = UpdateData->GetBuffer();
				if (ShaderResourceViewRHI)
				{
#if RMC_ENGINE_ABOVE_5_3
					ShaderResourceViewRHI = FRHICommandListImmediate::Get().CreateShaderResourceView(FShaderResourceViewInitializer(UpdateData->GetNumElements() > 0? VertexBufferRHI : nullptr, GetElementFormat()));
#else
					ShaderResourceViewRHI = RHICreateShaderResourceView(FShaderResourceViewInitializer(UpdateData->GetNumElements() > 0? VertexBufferRHI : nullptr, GetElementFormat()));
#endif
				}
				
				/*Batcher.QueueUpdateRequest(VertexBufferRHI, UpdateData->GetNumElements() > 0? UpdateData->GetBuffer() : nullptr);

#if RMC_ENGINE_BELOW_5_3
				if (ShaderResourceViewRHI)
				{
					if (UpdateData->GetBuffer().IsValid())
					{
						Batcher.QueueUpdateRequest(ShaderResourceViewRHI, VertexBufferRHI, GetElementStride(), GetElementFormat());
					}
					else
					{
						Batcher.QueueUpdateRequest(ShaderResourceViewRHI, nullptr, 0, 0);
					}
				}
#endif*/
			}
		}
	};

	class REALTIMEMESHCOMPONENT_API FRealtimeMeshIndexBuffer : public FRealtimeMeshGPUBuffer, public FIndexBuffer
	{
	public:
		FRealtimeMeshIndexBuffer(const FRealtimeMeshBufferLayout& InBufferLayout) : FRealtimeMeshGPUBuffer(TEXT("RealtimeMesh-IndexBuffer"), InBufferLayout)
		{
		}

		virtual FString GetFriendlyName() const override { return GetBufferName(); }

		virtual ERealtimeMeshStreamType GetStreamType() const override { return ERealtimeMeshStreamType::Index; }

		virtual void InitializeResources() override
		{
#if RMC_ENGINE_ABOVE_5_3
			InitResource(FRHICommandListImmediate::Get());
#else
			InitResource();
#endif
		}

		virtual void ReleaseUnderlyingResource() override { ReleaseResource(); }

		virtual bool IsResourceInitialized() const override { return IsInitialized(); }
		
#if RMC_ENGINE_ABOVE_5_3
		virtual void InitRHI(FRHICommandListBase& RHICmdList) override
		{
			FRHIResourceCreateInfo CreateInfo(TEXT("RealtimeMeshBuffer-Vertex-Init"));
			CreateInfo.bWithoutNativeResource = true;
			IndexBufferRHI = RHICmdList.CreateIndexBuffer(sizeof(uint16), 0, BUF_VertexBuffer | BUF_Static, CreateInfo);
		}
#else
		virtual void InitRHI() override
		{
			FRHIResourceCreateInfo CreateInfo(TEXT("RealtimeMeshBuffer-Vertex-Init"));
			CreateInfo.bWithoutNativeResource = true;
			IndexBufferRHI = RHICreateIndexBuffer(sizeof(uint16), 0, BUF_IndexBuffer | BUF_Static, CreateInfo);
		}
#endif

		virtual void ApplyBufferUpdate(TRHIResourceUpdateBatcher<RHIUpdateBatchSize>& Batcher,
		                               const FRealtimeMeshSectionGroupStreamUpdateDataRef& UpdateData) override
		{
			check(IsInitialized());
			FRealtimeMeshGPUBuffer::ApplyBufferUpdate(Batcher, UpdateData);

			// Adjust size by number of elements to handle structs containing 3 indices.
			BufferNum *= BufferLayout.GetNumElements();

			IndexBufferRHI = UpdateData->GetBuffer();
			//Batcher.QueueUpdateRequest(IndexBufferRHI, UpdateData->GetNumElements() > 0? UpdateData->GetBuffer() : nullptr);
		}
	};
}
