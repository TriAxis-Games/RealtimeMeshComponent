// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "Data/RealtimeMeshDataTypes.h"
#include "Containers/ResourceArray.h"
#include "..\Data\RealtimeMeshDataStream.h"
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 2
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
		FRealtimeMeshDataStream Stream;
		EBufferUsageFlags UsageFlags;
		FBufferRHIRef Buffer;

	public:
		FRealtimeMeshSectionGroupStreamUpdateData(FRealtimeMeshDataStream&& InStream)
			: Stream(MoveTemp(InStream))
			  , UsageFlags(EBufferUsageFlags::None)
		{
		}

		FRealtimeMeshSectionGroupStreamUpdateData(const FRealtimeMeshDataStream& InStream)
			: Stream(InStream)
			  , UsageFlags(EBufferUsageFlags::None)
		{
		}

		const FResourceArrayInterface* GetResource() const { return &Stream; }
		FRealtimeMeshBufferLayoutDefinition GetBufferLayout() const { return Stream.GetLayoutDefinition(); }
		FRealtimeMeshStreamKey GetStreamKey() const { return Stream.GetStreamKey(); }
		int32 GetNumElements() const { return Stream.Num(); }
		EBufferUsageFlags GetUsageFlags() const { return UsageFlags; }
		FBufferRHIRef& GetBuffer() { return Buffer; }

		void ConfigureBuffer(EBufferUsageFlags InUsageFlags, bool bShouldAttemptAsyncCreation = true)
		{
			UsageFlags = InUsageFlags;
			if (GRHISupportsAsyncTextureCreation && bShouldAttemptAsyncCreation && !Buffer.IsValid())
			{
				FRHIResourceCreateInfo CreateInfo(TEXT("RealtimeMeshBuffer-Temp"), &Stream);
				CreateInfo.bWithoutNativeResource = Stream.Num() == 0 || Stream.GetStride() == 0;

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
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
			}
		}

		void InitializeIfRequired()
		{
			if (!Buffer.IsValid())
			{
				FRHIResourceCreateInfo CreateInfo(TEXT("RealtimeMeshBuffer-Temp"), &Stream);
				CreateInfo.bWithoutNativeResource = Stream.Num() == 0 || Stream.GetStride() == 0;

				if (GetStreamKey().IsVertexStream())
				{
					Buffer = RHICreateVertexBuffer(Stream.GetResourceDataSize(), UsageFlags | BUF_VertexBuffer | BUF_ShaderResource, CreateInfo);
				}
				else
				{
					check(GetStreamKey().IsIndexStream());
					Buffer = RHICreateIndexBuffer(Stream.GetElementStride(), Stream.GetResourceDataSize(), UsageFlags | BUF_IndexBuffer, CreateInfo);
				}
			}
		}
	};

	using FRealtimeMeshSectionGroupStreamUpdateDataRef = TSharedRef<FRealtimeMeshSectionGroupStreamUpdateData>;

	class REALTIMEMESHCOMPONENT_API FRealtimeMeshGPUBuffer
	{
	protected:
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
		virtual bool IsResourceInitialized() const = 0;

		FORCEINLINE const FRealtimeMeshBufferLayoutDefinition& GetBufferLayout() const { return BufferLayout; }
		FORCEINLINE EPixelFormat GetElementFormat() const { return BufferLayout.GetElementTypeDefinition().GetPixelFormat(); }
		FORCEINLINE int32 GetElementStride() const { return GPixelFormats[GetElementFormat()].BlockBytes; }
		FORCEINLINE uint32 GetStride() const { return BufferLayout.GetStride(); }
		FORCEINLINE int32 Num() const { return BufferNum; }

		FORCEINLINE int32 NumElements() const { return BufferLayout.GetBufferLayout().GetNumElements(); }
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

		virtual bool IsResourceInitialized() const override { return IsInitialized(); }

		/** Gets the format of the vertex */
		FORCEINLINE EVertexElementType GetVertexType() const { return GetBufferLayout().GetElementTypeDefinition().GetVertexType(); }

		virtual void InitRHI() override
		{
			FRHIResourceCreateInfo CreateInfo(TEXT("RealtimeMeshBuffer-Vertex-Init"));
			CreateInfo.bWithoutNativeResource = true;
			VertexBufferRHI = RHICreateVertexBuffer(0, BUF_VertexBuffer | BUF_Static, CreateInfo);
			if (VertexBufferRHI && RHISupportsManualVertexFetch(GMaxRHIShaderPlatform))
			{
				ShaderResourceViewRHI = RHICreateShaderResourceView(FShaderResourceViewInitializer(nullptr, PF_R32G32B32F));
			}
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
					if (UpdateData->GetBuffer().IsValid())
					{
						Batcher.QueueUpdateRequest(ShaderResourceViewRHI, VertexBufferRHI, GetElementStride(), GetElementFormat());
					}
					else
					{
						Batcher.QueueUpdateRequest(ShaderResourceViewRHI, nullptr, 0, 0);
					}
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

		virtual bool IsResourceInitialized() const override { return IsInitialized(); }

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

			// Adjust size by number of elements to handle structs containing 3 indices.
			BufferNum *= BufferLayout.GetBufferLayout().GetNumElements();

			Batcher.QueueUpdateRequest(IndexBufferRHI, UpdateData->GetBuffer());
		}
	};
}
