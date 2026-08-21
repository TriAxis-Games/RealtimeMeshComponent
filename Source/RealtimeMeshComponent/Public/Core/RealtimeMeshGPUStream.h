// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "RealtimeMeshCoreFwd.h"
#include "RealtimeMeshStreamInterface.h"
#include "RealtimeMeshDataTypes.h"
#include "RealtimeMeshKeys.h"
#include "RHIFwd.h"
#include "RHIDefinitions.h"

namespace RealtimeMesh
{
	/**
	 * GPU-only stream: format and element count declared on the CPU side, the
	 * actual buffer lives on the GPU. The wrapped FBufferRHIRef may be empty at
	 * construction (e.g., the caller will populate it later via a compute job)
	 * and assigned via SetBuffer before the stream is shipped to the render
	 * proxy.
	 *
	 * Constraints, by design:
	 *   - Not resizable. ElementCount is fixed at construction.
	 *   - Cannot join an FRealtimeMeshStreamLinkage. Linkage::BindStream takes
	 *     FRealtimeMeshStream& (the CPU concrete) by signature, so this is a
	 *     compile-time exclusion.
	 *   - Not serializable. There are no CPU bytes to write to an archive.
	 *   - GPU->CPU readback is not yet supported.
	 *
	 * Render-path note: the default vertex factory binds the well-known stream
	 * keys (Position, Tangents, TexCoords, Color, and the index streams). A GPU
	 * stream registered under one of those keys will work end-to-end. A GPU
	 * stream registered under a custom user-defined key is held by the proxy
	 * but is not consumed by the default vertex factory — wiring custom keys
	 * to vertex factory inputs is a future pass.
	 */
	class REALTIMEMESHCOMPONENT_API FRealtimeMeshGPUStream final : public IRealtimeMeshStream
	{
	private:
		FRealtimeMeshStreamKey Key;
		FRealtimeMeshBufferLayout Layout;
		int32 ElementCount;
		EBufferUsageFlags Usage;
		FBufferRHIRef Buffer;
		// Row stride = ElementStride * NumElements. uint16 because large layouts
		// (e.g. FVector4d x 8 = 256 bytes) overflow a uint8 to 0.
		uint16 Stride;

	public:
		FRealtimeMeshGPUStream(
			const FRealtimeMeshStreamKey& InKey,
			const FRealtimeMeshBufferLayout& InLayout,
			int32 InElementCount,
			FBufferRHIRef InBuffer,
			EBufferUsageFlags InUsage)
			: Key(InKey)
			, Layout(InLayout)
			, ElementCount(InElementCount)
			, Usage(InUsage)
			, Buffer(MoveTemp(InBuffer))
		{
			const FRealtimeMeshBufferMemoryLayout MemoryLayout = FRealtimeMeshBufferLayoutUtilities::GetBufferLayoutMemoryLayout(Layout);
			Stride = MemoryLayout.GetStride();
		}

		virtual ~FRealtimeMeshGPUStream() override = default;

		// IRealtimeMeshStream
		virtual const FRealtimeMeshStreamKey& GetStreamKey() const override { return Key; }
		virtual const FRealtimeMeshBufferLayout& GetLayout() const override { return Layout; }
		virtual int32 GetStride() const override { return Stride; }
		virtual int32 Num() const override { return ElementCount; }
		virtual bool IsCPUBacked() const override { return false; }
		virtual FResourceArrayInterface* GetCPUResourceArray() override { return nullptr; }
		virtual const FResourceArrayInterface* GetCPUResourceArray() const override { return nullptr; }

		EBufferUsageFlags GetUsage() const { return Usage; }
		const FBufferRHIRef& GetBuffer() const { return Buffer; }
		void SetBuffer(FBufferRHIRef InBuffer) { Buffer = MoveTemp(InBuffer); }
	};
}
