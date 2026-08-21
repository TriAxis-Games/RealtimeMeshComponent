// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "RealtimeMeshCoreFwd.h"
#include "RealtimeMeshDataTypes.h"
#include "RealtimeMeshKeys.h"

class FResourceArrayInterface;

namespace RealtimeMesh
{
	/**
	 * Abstract base for stream-like resources. Two concrete kinds exist today:
	 *   - FRealtimeMeshStream     — CPU-backed byte buffer with a runtime layout
	 *                                descriptor. Resizable. Serializable. Can join
	 *                                an FRealtimeMeshStreamLinkage.
	 *   - FRealtimeMeshGPUStream  — GPU-only. Format and element count are declared
	 *                                on the CPU side but there are no CPU bytes;
	 *                                the buffer lives on the GPU. Not resizable, not
	 *                                serializable, cannot join a linkage.
	 *
	 * Render-path note: the default vertex factory binds the named streams
	 * (Position, Tangents, TexCoords, Color) and the index streams. Custom user
	 * stream keys can be added at the data layer and uploaded to GPU, but the
	 * default vertex factory will not consume them. Wiring custom keys into
	 * vertex factory inputs is a separate (future) pass.
	 */
	class REALTIMEMESHCOMPONENT_API IRealtimeMeshStream
	{
	public:
		virtual ~IRealtimeMeshStream() = default;

		virtual const FRealtimeMeshStreamKey& GetStreamKey() const = 0;
		virtual const FRealtimeMeshBufferLayout& GetLayout() const = 0;
		virtual int32 GetStride() const = 0;
		virtual int32 Num() const = 0;

		// True for CPU-backed streams (FRealtimeMeshStream). False for GPU-only
		// streams (FRealtimeMeshGPUStream). Callers that need to access CPU bytes,
		// resize, or serialize must check this and downcast.
		virtual bool IsCPUBacked() const = 0;

		// CPU streams return themselves (as FResourceArrayInterface*) so the upload
		// path can hand the bytes to the RHI. GPU streams return nullptr — they
		// have no CPU resource to upload.
		virtual FResourceArrayInterface* GetCPUResourceArray() = 0;
		virtual const FResourceArrayInterface* GetCPUResourceArray() const = 0;
	};
}
