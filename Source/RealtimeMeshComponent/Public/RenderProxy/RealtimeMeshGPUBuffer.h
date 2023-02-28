// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "Data/RealtimeMeshDataTypes.h"
#include "Containers/ResourceArray.h"

namespace RealtimeMesh
{
	class FRealtimeMeshGPUBuffer;
	class FRealtimeMeshVertexBuffer;
	class FRealtimeMeshIndexBuffer;

	struct REALTIMEMESHCOMPONENT_API FRealtimeMeshSectionGroupStreamUpdateData
	{
	private:
		TUniquePtr<FResourceArrayInterface> Resource;
		FRealtimeMeshBufferLayoutDefinition BufferLayout;
		FRealtimeMeshStreamKey StreamKey;
		EBufferUsageFlags UsageFlags;
		FBufferRHIRef Buffer;
	public:
		FRealtimeMeshSectionGroupStreamUpdateData(TUniquePtr<FResourceArrayInterface>&& InResource, const FRealtimeMeshBufferLayoutDefinition& InBufferLayout,
		    const FRealtimeMeshStreamKey& InStreamKey)
			: Resource(MoveTemp(InResource))
			, BufferLayout(InBufferLayout)
			, StreamKey(InStreamKey)
			, UsageFlags(EBufferUsageFlags::None)
		{
		}

		FResourceArrayInterface* GetResource() const { return Resource.Get(); }
		FRealtimeMeshBufferLayoutDefinition GetBufferLayout() const { return BufferLayout; }
		FRealtimeMeshStreamKey GetStreamKey() const { return StreamKey; }
		int32 GetNumElements() const { return Resource->GetResourceDataSize() / BufferLayout.GetStride(); }
		EBufferUsageFlags GetUsageFlags() const { return UsageFlags; }
		FBufferRHIRef& GetBuffer() { return Buffer; }

		void ConfigureBuffer(EBufferUsageFlags InUsageFlags, bool bShouldAttemptAsyncCreation = true)
		{
			UsageFlags = InUsageFlags;
			if (GRHISupportsAsyncTextureCreation && bShouldAttemptAsyncCreation && !Buffer.IsValid())
			{
				FRHIResourceCreateInfo CreateInfo(TEXT("RealtimeMeshBuffer-Temp"), Resource.Get());
				CreateInfo.bWithoutNativeResource = Resource == nullptr || BufferLayout.GetStride() == 0 || GetNumElements() == 0;

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
				FRHIAsyncCommandList CommandList;

				if (StreamKey.IsVertexStream())
				{
					Buffer = CommandList->CreateBuffer(Resource->GetResourceDataSize(), UsageFlags | BUF_VertexBuffer | BUF_ShaderResource,
						BufferLayout.GetStride(), ERHIAccess::SRVMask, CreateInfo);
				}
				else
				{
					check(StreamKey.IsIndexStream());
					Buffer = CommandList->CreateBuffer(Resource->GetResourceDataSize(), UsageFlags | BUF_IndexBuffer | BUF_ShaderResource,
						BufferLayout.GetStride(), ERHIAccess::SRVMask, CreateInfo);
				}
#else
				if (StreamKey.IsVertexStream())
				{
					Buffer = RHIAsyncCreateVertexBuffer(Resource->GetResourceDataSize(), UsageFlags | BUF_VertexBuffer | BUF_ShaderResource,
						ERHIAccess::SRVMask, CreateInfo);
				}
				else
				{
					check(StreamKey.IsIndexStream());
					Buffer = RHIAsyncCreateIndexBuffer(BufferLayout.GetStride(), Resource->GetResourceDataSize(), UsageFlags | BUF_IndexBuffer | BUF_ShaderResource,
						ERHIAccess::SRVMask, CreateInfo);
				}
#endif
			}
		}

		void InitializeIfRequired()
		{
			if (!Buffer.IsValid())
			{
				FRHIResourceCreateInfo CreateInfo(TEXT("RealtimeMeshBuffer-Temp"), Resource.Get());
				CreateInfo.bWithoutNativeResource = Resource == nullptr || BufferLayout.GetStride() == 0 || GetNumElements() == 0;

				if (StreamKey.IsVertexStream())
				{
					Buffer = RHICreateVertexBuffer(Resource->GetResourceDataSize(), UsageFlags | BUF_VertexBuffer | BUF_ShaderResource, CreateInfo);
				}
				else
				{
					check(StreamKey.IsIndexStream());
					Buffer = RHICreateIndexBuffer(BufferLayout.GetStride(), Resource->GetResourceDataSize(), UsageFlags | BUF_IndexBuffer | BUF_ShaderResource, CreateInfo);
				}
			}
		}
	};

	using FRealtimeMeshSectionGroupStreamUpdateDataRef = TSharedRef<FRealtimeMeshSectionGroupStreamUpdateData>;

	class REALTIMEMESHCOMPONENT_API FRealtimeMeshGPUBuffer
	{
	private:
		FRealtimeMeshBufferLayoutDefinition BufferLayout;
		uint32 BufferNum;
		EBufferUsageFlags UsageFlags;

#if WITH_EDITOR
		FString BufferName;
#endif

	public:
		FRealtimeMeshGPUBuffer(const TCHAR* InBufferName)
			: BufferLayout(FRealtimeMeshBufferLayoutDefinition::Invalid)
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

		FORCEINLINE const FRealtimeMeshBufferLayoutDefinition& GetBufferLayout() const { return BufferLayout; }
		FORCEINLINE EPixelFormat GetElementFormat() const { return BufferLayout.GetElementTypeDefinition().GetPixelFormat(); }
		FORCEINLINE int32 GetElementStride() const { return GPixelFormats[GetElementFormat()].BlockBytes; }
		FORCEINLINE uint32 GetStride() const { return BufferLayout.GetStride(); }
		FORCEINLINE int32 Num() const { return BufferNum; }

		FORCEINLINE bool TryGetElementOffset(FName SubComponentName, uint16& OutSubComponentOffset) const
		{
			if (const uint8* Entry = BufferLayout.FindElementOffset(SubComponentName))
			{
				OutSubComponentOffset = *Entry;
				return true;
			}
			return false;
		}


		static constexpr int32 RHIUpdateBatchSize = 16;

		virtual void ApplyBufferUpdate(TRHIResourceUpdateBatcher<RHIUpdateBatchSize>& Batcher,
			const FRealtimeMeshSectionGroupStreamUpdateDataRef& UpdateData)
		{
			BufferLayout = UpdateData->GetBufferLayout();
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
		FRealtimeMeshVertexBuffer() : FRealtimeMeshGPUBuffer(TEXT("RealtimeMesh-VertexBuffer"))
		{
		}

		virtual FString GetFriendlyName() const override { return GetBufferName(); }

		virtual ERealtimeMeshStreamType GetStreamType() const override { return ERealtimeMeshStreamType::Vertex; }

		virtual void InitializeResources() override
		{
			InitResource();
		}

		virtual void ReleaseUnderlyingResource() override { ReleaseResource(); }

		/** Gets the format of the vertex */
		FORCEINLINE EVertexElementType GetVertexType() const { return GetBufferLayout().GetElementTypeDefinition().GetVertexType(); }

		virtual void InitRHI() override
		{
			FRHIResourceCreateInfo CreateInfo(TEXT("RealtimeMeshBuffer-Vertex-Init"));
			CreateInfo.bWithoutNativeResource = true;
			VertexBufferRHI = RHICreateVertexBuffer(0, BUF_VertexBuffer | BUF_Static, CreateInfo);
			ShaderResourceViewRHI = RHISupportsManualVertexFetch(GMaxRHIShaderPlatform)
				? RHICreateShaderResourceView(FShaderResourceViewInitializer(nullptr, PF_R32_FLOAT))
				: nullptr;
		}
		
		virtual void ApplyBufferUpdate(TRHIResourceUpdateBatcher<RHIUpdateBatchSize>& Batcher, 
		                               const FRealtimeMeshSectionGroupStreamUpdateDataRef& UpdateData) override
		{
			check(IsInitialized());
			FRealtimeMeshGPUBuffer::ApplyBufferUpdate(Batcher, UpdateData);
			{
				Batcher.QueueUpdateRequest(VertexBufferRHI, UpdateData->GetBuffer());
				if (ShaderResourceViewRHI)
				{
					Batcher.QueueUpdateRequest(ShaderResourceViewRHI, VertexBufferRHI, GetElementStride(), GetElementFormat());
				}
			}
		}
	};

	class REALTIMEMESHCOMPONENT_API FRealtimeMeshIndexBuffer : public FRealtimeMeshGPUBuffer, public FIndexBuffer
	{
	public:
		FRealtimeMeshIndexBuffer() : FRealtimeMeshGPUBuffer(TEXT("RealtimeMesh-IndexBuffer"))
		{
		}

		virtual FString GetFriendlyName() const override { return GetBufferName(); }

		virtual ERealtimeMeshStreamType GetStreamType() const override { return ERealtimeMeshStreamType::Index; }

		virtual void InitializeResources() override
		{
			InitResource();
		}

		virtual void ReleaseUnderlyingResource() override { ReleaseResource(); }

		/** Gets the format of the index buffer */
		FORCEINLINE bool IsUsing32BitIndices() const { return FRealtimeMeshBufferLayoutUtilities::Is32BitIndex(GetBufferLayout().GetElementTypeDefinition()); }

		virtual void InitRHI() override
		{
			FRHIResourceCreateInfo CreateInfo(TEXT("RealtimeMeshBuffer-Vertex-Init"));
			CreateInfo.bWithoutNativeResource = true;
			IndexBufferRHI = RHICreateIndexBuffer(sizeof(uint16), 0, BUF_VertexBuffer | BUF_Static, CreateInfo);
		}
		
		virtual void ApplyBufferUpdate(TRHIResourceUpdateBatcher<RHIUpdateBatchSize>& Batcher,
			const FRealtimeMeshSectionGroupStreamUpdateDataRef& UpdateData) override
		{
			check(IsInitialized());
			FRealtimeMeshGPUBuffer::ApplyBufferUpdate(Batcher, UpdateData);
			Batcher.QueueUpdateRequest(IndexBufferRHI, UpdateData->GetBuffer());
		}
	};
	
}
