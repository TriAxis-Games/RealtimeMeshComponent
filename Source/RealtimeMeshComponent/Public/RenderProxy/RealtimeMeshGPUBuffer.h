// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "Core/RealtimeMeshDataTypes.h"
#include "Containers/ResourceArray.h"
#include "Core/RealtimeMeshDataStream.h"
#include "Core/RealtimeMeshGPUStream.h"
#include "DataDrivenShaderPlatformInfo.h"

#if RMC_ENGINE_ABOVE_5_6
struct FRHIBufferCreateDesc;
#endif

namespace RealtimeMesh
{
	struct FRealtimeMeshUpdateContext;
	class FRealtimeMeshGPUBuffer;
	class FRealtimeMeshVertexBuffer;
	class FRealtimeMeshIndexBuffer;

	struct REALTIMEMESHCOMPONENT_API FRealtimeMeshSectionGroupStreamUpdateData
	{
	private:
		FRealtimeMeshStream Stream;
		EBufferUsageFlags UsageFlags;
		FBufferRHIRef Buffer;

#if RMC_ENGINE_ABOVE_5_6
		// Shared 5.6+ FRHIBufferCreateDesc builder used by both CreateBufferAsyncIfPossible and
		// FinalizeInitialization (see the .cpp). The PROXY-F8 async gate stays at the call sites.
		FRHIBufferCreateDesc BuildBufferCreateDesc();
#endif

	public:
		FRealtimeMeshSectionGroupStreamUpdateData(FRealtimeMeshStream&& InStream, EBufferUsageFlags InUsageFlags)
			: Stream(MoveTemp(InStream))
			, UsageFlags(InUsageFlags)
		{
		}

		const FResourceArrayInterface* GetResource() const { return &Stream; }
		FRealtimeMeshBufferLayout GetBufferLayout() const { return Stream.GetLayout(); }
		FRealtimeMeshStreamKey GetStreamKey() const { return Stream.GetStreamKey(); }
		int32 GetNumElements() const { return Stream.Num(); }
		EBufferUsageFlags GetUsageFlags() const { return UsageFlags; }
		FBufferRHIRef& GetBuffer() { return Buffer; }

		void CreateBufferAsyncIfPossible(FRealtimeMeshUpdateContext& UpdateContext);

		void FinalizeInitialization(FRHICommandListBase& RHICmdList);
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
		virtual void InitializeResources(FRHICommandListBase& RHICmdList, const FRealtimeMeshSectionGroupStreamUpdateDataRef& UpdateData) = 0;

		// Compute-writable buffers (created with BUF_UnorderedAccess) expose a UAV so a compute
		// pass can write the geometry directly. Returns nullptr for normal (SRV-only) buffers.
		virtual FRHIUnorderedAccessView* GetUAV() const { return nullptr; }
		FORCEINLINE bool IsComputeWritable() const { return EnumHasAnyFlags(UsageFlags, EBufferUsageFlags::UnorderedAccess); }

		FORCEINLINE EBufferUsageFlags GetUsageFlags() const { return UsageFlags; }

		// The underlying RHI buffer (vertex or index), e.g. to import into an RDG graph. Null until initialized.
		virtual FBufferRHIRef GetRHIBufferRef() const { return nullptr; }

		// Adopt a caller-provided GPU-resident buffer (no CPU upload). Default impl
		// rejects — only the concrete vertex / index buffer types support this.
		virtual void InitializeResourcesFromGPUStream(FRHICommandListBase& RHICmdList, const FRealtimeMeshGPUStream& Stream)
		{
			checkf(false, TEXT("GPU buffer subclass %s does not support direct FRealtimeMeshGPUStream registration"), *GetBufferName());
		}

		virtual void ReleaseUnderlyingResource() = 0;
		virtual bool IsResourceInitialized() const = 0;

		FORCEINLINE const FRealtimeMeshBufferLayout& GetBufferLayout() const { return BufferLayout; }
		FORCEINLINE EPixelFormat GetElementFormat() const { return ElementDetails.GetPixelFormat(); }
		FORCEINLINE int32 GetElementStride() const { return GPixelFormats[GetElementFormat()].BlockBytes; }
		FORCEINLINE uint32 GetStride() const { return MemoryLayout.GetStride(); }
		FORCEINLINE int32 Num() const { return BufferNum; }

		FORCEINLINE int32 NumElements() const { return BufferLayout.GetNumElements(); }


		static constexpr int32 RHIUpdateBatchSize = 16;

		// ===== In-place fast update =====
		// A copy-target-capable buffer (created with BUF_KeepCPUAccessible, which grants
		// TRANSFER_DST) can have a sub-range overwritten via a RLM_WriteOnly lock that the
		// RHI services with a staging buffer + GPU copy — preserving the untouched bytes and
		// keeping the same FRHIBuffer handle and SRV (so no realloc and no vertex-factory
		// reinit). Index and non-copy-target buffers don't support this; the fast-update
		// path falls back to a full reallocating update for them.
		bool SupportsInPlaceUpdate() const { return EnumHasAnyFlags(UsageFlags, EBufferUsageFlags::KeepCPUAccessible); }

		// Reports whether overwriting [ElementOffset, ElementOffset + NumElements) is
		// currently possible (dynamic usage, initialized resource, matching layout, range
		// in bounds). Default false.
		virtual bool CanUpdateInPlace(const FRealtimeMeshBufferLayout& InLayout, int32 ElementOffset, int32 NumElements) const { return false; }

		// Overwrites [ElementOffset, ElementOffset + NumElements) from SrcData, which
		// points at element 0 of the source stream (the implementation offsets into it).
		// Returns true on success; default false so the caller falls back to a
		// reallocating update.
		virtual bool UpdateInPlace(FRHICommandListBase& RHICmdList, const void* SrcData, int32 ElementOffset, int32 NumElements) { return false; }

	protected:
		// Creates a typed UAV (matching the element format) for a compute-writable buffer
		// (UsageFlags carries UnorderedAccess). No-op below 5.6 or when BufferRHI is null,
		// leaving OutUAV untouched. Shared by the vertex and index buffer subclasses.
		void CreateComputeUAV(FRHICommandListBase& RHICmdList, const FBufferRHIRef& BufferRHI, FUnorderedAccessViewRHIRef& OutUAV)
		{
			if (!BufferRHI || !EnumHasAnyFlags(UsageFlags, EBufferUsageFlags::UnorderedAccess))
			{
				return;
			}
#if RMC_ENGINE_ABOVE_5_6
			OutUAV = RHICmdList.CreateUnorderedAccessView(BufferRHI,
				FRHIViewDesc::CreateBufferUAV().SetType(FRHIViewDesc::EBufferType::Typed).SetFormat(GetElementFormat()));
#endif
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

		virtual void InitializeResources(FRHICommandListBase& RHICmdList, const FRealtimeMeshSectionGroupStreamUpdateDataRef& UpdateData) override
		{
			InitResource(RHICmdList);
			InitializeResourcesCommon(RHICmdList, UpdateData->GetBufferLayout(), UpdateData->GetNumElements(), UpdateData->GetUsageFlags(),
				UpdateData->GetStreamKey(), UpdateData->GetBuffer());
		}

		virtual void InitializeResourcesFromGPUStream(FRHICommandListBase& RHICmdList, const FRealtimeMeshGPUStream& Stream) override
		{
			InitResource(RHICmdList);
			InitializeResourcesCommon(RHICmdList, Stream.GetLayout(), Stream.Num(), Stream.GetUsage(),
				Stream.GetStreamKey(), Stream.GetBuffer());
		}

		virtual FRHIUnorderedAccessView* GetUAV() const override { return UnorderedAccessViewRHI; }
		virtual FBufferRHIRef GetRHIBufferRef() const override { return VertexBufferRHI; }

		virtual void ReleaseUnderlyingResource() override { ReleaseResource(); }

		virtual bool IsResourceInitialized() const override { return IsInitialized(); }

		virtual bool CanUpdateInPlace(const FRealtimeMeshBufferLayout& InLayout, int32 ElementOffset, int32 NumElements) const override
		{
			if (!EnumHasAnyFlags(UsageFlags, EBufferUsageFlags::KeepCPUAccessible) || !IsInitialized() || !VertexBufferRHI.IsValid())
			{
				return false;
			}
			if (BufferLayout != InLayout)
			{
				return false;
			}
			const uint32 Stride = GetStride();
			// int64 on both sides: BufferNum is uint32, so a signed/unsigned compare here is both an
			// MSVC C4018 and a real hazard if ElementOffset + NumElements overflows int32.
			if (Stride == 0 || ElementOffset < 0 || NumElements <= 0 ||
				static_cast<int64>(ElementOffset) + NumElements > static_cast<int64>(BufferNum))
			{
				return false;
			}
			return (static_cast<uint64>(ElementOffset) + NumElements) * Stride <= VertexBufferRHI->GetSize();
		}

		virtual bool UpdateInPlace(FRHICommandListBase& RHICmdList, const void* SrcData, int32 ElementOffset, int32 NumElements) override
		{
			if (!SrcData || !CanUpdateInPlace(BufferLayout, ElementOffset, NumElements))
			{
				return false;
			}
			const uint32 Stride = GetStride();
			const uint32 ByteOffset = static_cast<uint32>(ElementOffset) * Stride;
			const uint32 ByteSize = static_cast<uint32>(NumElements) * Stride;
			if (void* Dst = RHICmdList.LockBuffer(VertexBufferRHI, ByteOffset, ByteSize, RLM_WriteOnly))
			{
				FMemory::Memcpy(Dst, static_cast<const uint8*>(SrcData) + ByteOffset, ByteSize);
				RHICmdList.UnlockBuffer(VertexBufferRHI);
				return true;
			}
			return false;
		}

		/** Gets the format of the vertex */
		FORCEINLINE EVertexElementType GetVertexType() const { return ElementDetails.GetVertexType(); }

	private:
		// Shared body for InitializeResources / InitializeResourcesFromGPUStream; the two paths
		// differ only in their data source (section-group update data vs GPU stream).
		void InitializeResourcesCommon(FRHICommandListBase& RHICmdList, const FRealtimeMeshBufferLayout& InLayout, int32 InNum,
			EBufferUsageFlags InUsage, const FRealtimeMeshStreamKey& InKey, const FBufferRHIRef& InBuffer)
		{
			check(BufferLayout == InLayout);
			BufferNum = InNum;
			UsageFlags = InUsage;

#if WITH_EDITOR
			BufferName = InKey.GetName().ToString();
#endif

			check(BufferLayout.IsValid());
			check(GetStride() > 0);

			VertexBufferRHI = InBuffer;

			if (VertexBufferRHI && RHISupportsManualVertexFetch(GMaxRHIShaderPlatform))
			{
				ShaderResourceViewRHI = RHICmdList.CreateShaderResourceView(FShaderResourceViewInitializer(VertexBufferRHI, GetElementFormat()));
			}

			CreateComputeUAV(RHICmdList, VertexBufferRHI, UnorderedAccessViewRHI);
		}

	public:
		virtual void InitRHI(FRHICommandListBase& RHICmdList) override
		{
		}

		virtual void ReleaseRHI() override
		{
			FVertexBufferWithSRV::ReleaseRHI();
			BufferLayout = FRealtimeMeshBufferLayout::Invalid;
			BufferNum = 0;
			UsageFlags = BUF_None;
		}
	};

	class REALTIMEMESHCOMPONENT_API FRealtimeMeshIndexBuffer : public FRealtimeMeshGPUBuffer, public FIndexBuffer
	{
	private:
		// FIndexBuffer has no UAV member of its own; compute-writable index buffers keep theirs here.
		FUnorderedAccessViewRHIRef UnorderedAccessViewRHI;

	public:
		FRealtimeMeshIndexBuffer(const FRealtimeMeshBufferLayout& InBufferLayout) : FRealtimeMeshGPUBuffer(TEXT("RealtimeMesh-IndexBuffer"), InBufferLayout)
		{
		}

		virtual FString GetFriendlyName() const override { return GetBufferName(); }

		virtual ERealtimeMeshStreamType GetStreamType() const override { return ERealtimeMeshStreamType::Index; }

		virtual void InitializeResources(FRHICommandListBase& RHICmdList, const FRealtimeMeshSectionGroupStreamUpdateDataRef& UpdateData) override
		{
			InitResource(RHICmdList);
			InitializeResourcesCommon(RHICmdList, UpdateData->GetBufferLayout(), UpdateData->GetNumElements(), UpdateData->GetUsageFlags(),
				UpdateData->GetStreamKey(), UpdateData->GetBuffer());
		}

		virtual void InitializeResourcesFromGPUStream(FRHICommandListBase& RHICmdList, const FRealtimeMeshGPUStream& Stream) override
		{
			InitResource(RHICmdList);
			InitializeResourcesCommon(RHICmdList, Stream.GetLayout(), Stream.Num(), Stream.GetUsage(),
				Stream.GetStreamKey(), Stream.GetBuffer());
		}

		virtual FRHIUnorderedAccessView* GetUAV() const override { return UnorderedAccessViewRHI; }
		virtual FBufferRHIRef GetRHIBufferRef() const override { return IndexBufferRHI; }

		virtual void ReleaseUnderlyingResource() override { ReleaseResource(); }

		virtual bool IsResourceInitialized() const override { return IsInitialized(); }

		virtual void InitRHI(FRHICommandListBase& RHICmdList) override
		{
		}

		virtual void ReleaseRHI() override
		{
			UnorderedAccessViewRHI.SafeRelease();
			FIndexBuffer::ReleaseRHI();
			BufferLayout = FRealtimeMeshBufferLayout::Invalid;
			BufferNum = 0;
			UsageFlags = BUF_None;
		}

	private:
		// Shared body for InitializeResources / InitializeResourcesFromGPUStream; the two paths
		// differ only in their data source (section-group update data vs GPU stream).
		void InitializeResourcesCommon(FRHICommandListBase& RHICmdList, const FRealtimeMeshBufferLayout& InLayout, int32 InNum,
			EBufferUsageFlags InUsage, const FRealtimeMeshStreamKey& InKey, const FBufferRHIRef& InBuffer)
		{
			check(BufferLayout == InLayout);
			BufferNum = InNum;
			UsageFlags = InUsage;

#if WITH_EDITOR
			BufferName = InKey.GetName().ToString();
#endif

			check(BufferLayout.IsValid());
			check(GetStride() > 0);

			// Adjust size by number of elements to handle structs containing 3 indices.
			BufferNum *= BufferLayout.GetNumElements();
			IndexBufferRHI = InBuffer;

			CreateComputeUAV(RHICmdList, IndexBufferRHI, UnorderedAccessViewRHI);
		}
	};
}
