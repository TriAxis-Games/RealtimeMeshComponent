// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "RealtimeMeshCore.h"
#include "RealtimeMeshProxyShared.h"
#include "Core/RealtimeMeshKeys.h"
#include "Core/RealtimeMeshSectionConfig.h"
#include "Core/RealtimeMeshStreamRange.h"

namespace RealtimeMesh
{
	class REALTIMEMESHCOMPONENT_API FRealtimeMeshSectionProxy : public TSharedFromThis<FRealtimeMeshSectionProxy>
	{
	private:
		const FRealtimeMeshContextRef Context;
		const FRealtimeMeshSectionKey Key;
		FRealtimeMeshSectionConfig Config;
		FRealtimeMeshStreamRange StreamRange;
		FRealtimeMeshDrawMask DrawMask;

		// Index of the buffer set this section reads from, within the same LOD.
		// Mutable — a section can be reassigned to a different buffer set by writing
		// this field. Set via SetBufferSetIndex (or the convenience SetBufferSet that
		// takes a key). Default is INDEX_NONE; LOD::CreateSectionIfNotExists will
		// initialize it from the legacy SG key when the section is created
		// via the old API so existing callers continue to work.
		int32 BufferSetIndex;

		bool bRangeChanged;

		// Optional GPU-driven indirect draw. When IndirectArgsBuffer is set, the section renders
		// with NumPrimitives==0 so the engine sources the index count from this buffer (a compute
		// pass decides the count). The buffer holds an FRHIDrawIndexedIndirectParameters at the
		// given byte offset. The section's StreamRange must cover the allocated buffer capacity so
		// the GPU-decided draw stays in bounds. Null = normal CPU-counted draw.
		FBufferRHIRef IndirectArgsBuffer;
		uint32 IndirectArgsOffset;

	public:
		FRealtimeMeshSectionProxy(const FRealtimeMeshContextRef& InContext, const FRealtimeMeshSectionKey InKey);
		virtual ~FRealtimeMeshSectionProxy();

		/**
		 * Produce an independent clone used to populate a published snapshot.
		 * The section is a pure value type (config + range + draw mask), so a shallow
		 * copy via the copy constructor suffices.
		 */
		TSharedRef<FRealtimeMeshSectionProxy> Clone() const;

		const FRealtimeMeshSectionKey& GetKey() const { return Key; }
		const FRealtimeMeshSectionConfig& GetConfig() const { return Config; }
		const FRealtimeMeshStreamRange& GetStreamRange() const { return StreamRange; }
		int32 GetMaterialSlot() const { return Config.MaterialSlot; }
		FRealtimeMeshDrawMask GetDrawMask() const { return DrawMask; }

		bool IsRangeDirty() const { return bRangeChanged; }

		// Buffer-set reference accessors. The buffer set this section uses can be
		// changed at any time — callers do not need to recreate the section to
		// rebind it.
		int32 GetBufferSetIndex() const { return BufferSetIndex; }
		FRealtimeMeshBufferSetKey GetBufferSetKey() const
		{
			return FRealtimeMeshBufferSetKey::Create(Key.LOD(), BufferSetIndex);
		}
		void SetBufferSetIndex(int32 InBufferSetIndex) { BufferSetIndex = InBufferSetIndex; }
		void SetBufferSet(const FRealtimeMeshBufferSetKey& InBufferSet) { BufferSetIndex = InBufferSet.Index(); }

		void UpdateConfig(const FRealtimeMeshSectionConfig& NewConfig);
		void UpdateStreamRange(const FRealtimeMeshStreamRange& NewStreamRange);

		// GPU-driven indirect draw. InArgsBuffer must be created with BUF_DrawIndirect and hold an
		// FRHIDrawIndexedIndirectParameters at InOffset. Pass nullptr (or call ClearIndirectArgs)
		// to return to a normal CPU-counted draw.
		void SetIndirectArgs(const FBufferRHIRef& InArgsBuffer, uint32 InOffset = 0) { IndirectArgsBuffer = InArgsBuffer; IndirectArgsOffset = InOffset; }
		void ClearIndirectArgs() { IndirectArgsBuffer = nullptr; IndirectArgsOffset = 0; }
		bool HasIndirectArgs() const { return IndirectArgsBuffer.IsValid(); }

		bool InitializeMeshBatch(FMeshBatch& MeshBatch, FRHIUniformBuffer* PrimitiveUniformBuffer) const;


		void UpdateCachedState(FRealtimeMeshBufferSetProxy& ParentGroup);
		void Reset();
	};
}
