// Copyright (c) 2015-2024 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "RealtimeMeshConfig.h"
#include "RealtimeMeshGPUBuffer.h"
#include "RealtimeMeshProxyShared.h"
#include "RealtimeMeshVertexFactory.h"
#include "RealtimeMeshSectionProxy.h"

namespace RealtimeMesh
{
	using FRealtimeMeshSectionMask = TBitArray<TInlineAllocator<1>>;

	class FRealtimeMeshActiveSectionIterator
	{
	private:
		const FRealtimeMeshSectionGroupProxy& Proxy;
		TConstSetBitIterator<TInlineAllocator<1>> Iterator;

	public:
		FRealtimeMeshActiveSectionIterator(const FRealtimeMeshSectionGroupProxy& InProxy, const FRealtimeMeshSectionMask& InMask)
			: Proxy(InProxy), Iterator(TConstSetBitIterator(InMask)) { }
		
		/** Forwards iteration operator. */
		FORCEINLINE FRealtimeMeshActiveSectionIterator& operator++()
		{
			++Iterator;
			return *this;
		}

		FORCEINLINE bool operator==(const FRealtimeMeshActiveSectionIterator& Other) const
		{
			return Iterator == Other.Iterator;
		}

		FORCEINLINE bool operator!=(const FRealtimeMeshActiveSectionIterator& Other) const
		{ 
			return Iterator != Other.Iterator;
		}

		/** conversion to "bool" returning true if the iterator is valid. */
		FORCEINLINE explicit operator bool() const
		{
			return (bool)Iterator;
		}
		/** inverse of the "bool" operator */
		FORCEINLINE bool operator !() const 
		{
			return !(bool)*this;
		}

		FRealtimeMeshSectionProxy* operator*() const;
		FRealtimeMeshSectionProxy& operator->() const;
		
		/** Index accessor. */
		FORCEINLINE int32 GetIndex() const
		{
			return Iterator.GetIndex();
		}
	};
	
	class REALTIMEMESHCOMPONENT_API FRealtimeMeshSectionGroupProxy : public TSharedFromThis<FRealtimeMeshSectionGroupProxy>
	{
	private:
		const FRealtimeMeshSharedResourcesRef SharedResources;
		const FRealtimeMeshSectionGroupKey Key;
		TSharedPtr<FRealtimeMeshVertexFactory> VertexFactory;
		TArray<FRealtimeMeshSectionProxyRef> Sections;
		TMap<FRealtimeMeshSectionKey, uint32> SectionMap;
		FRealtimeMeshSectionMask ActiveSectionMask;		
		FRealtimeMeshSectionMask ActiveStaticSectionMask;
		FRealtimeMeshSectionMask ActiveDynamicSectionMask;
		FRealtimeMeshStreamProxyMap Streams;
#if RHI_RAYTRACING
		FRayTracingGeometry RayTracingGeometry;
#endif

		FRealtimeMeshDrawMask DrawMask;
		uint32 bIsStateDirty : 1;

	public:
		FRealtimeMeshSectionGroupProxy(const FRealtimeMeshSharedResourcesRef& InSharedResources, const FRealtimeMeshSectionGroupKey& InKey);
		virtual ~FRealtimeMeshSectionGroupProxy();

		const FRealtimeMeshSectionGroupKey& GetKey() const { return Key; }
		TSharedPtr<FRealtimeMeshVertexFactory> GetVertexFactory() const { return VertexFactory; }
		FRealtimeMeshDrawMask GetDrawMask() const { return DrawMask; }
		FRealtimeMeshActiveSectionIterator GetActiveSectionMaskIter() const { return FRealtimeMeshActiveSectionIterator(*this, ActiveSectionMask); }
		FRealtimeMeshActiveSectionIterator GetActiveStaticSectionMaskIter() const { return FRealtimeMeshActiveSectionIterator(*this, ActiveStaticSectionMask); }
		FRealtimeMeshActiveSectionIterator GetActiveDynamicSectionMaskIter() const { return FRealtimeMeshActiveSectionIterator(*this, ActiveDynamicSectionMask); }

		FRealtimeMeshSectionProxyPtr GetSection(const FRealtimeMeshSectionKey& SectionKey) const;
		TSharedPtr<FRealtimeMeshGPUBuffer> GetStream(const FRealtimeMeshStreamKey& StreamKey) const;

#if RHI_RAYTRACING
		const FRayTracingGeometry* GetRayTracingGeometry() const { return &RayTracingGeometry; }
#endif
		
		FRayTracingGeometry* GetRayTracingGeometry();
		
		virtual void CreateSectionIfNotExists(const FRealtimeMeshSectionKey& SectionKey);
		virtual void RemoveSection(const FRealtimeMeshSectionKey& SectionKey);

		virtual void CreateOrUpdateStream(const FRealtimeMeshSectionGroupStreamUpdateDataRef& InStream);
		virtual void RemoveStream(const FRealtimeMeshStreamKey& StreamKey);

		virtual bool InitializeMeshBatch(FMeshBatch& MeshBatch, FRealtimeMeshResourceReferenceList& Resources, bool bIsLocalToWorldDeterminantNegative, bool bWantsDepthOnly) const;

		virtual bool UpdateCachedState(bool bShouldForceUpdate);
		virtual void Reset();

	protected:
		virtual void UpdateRayTracingInfo();

		void MarkStateDirty();
		void RebuildSectionMap();

		friend class FRealtimeMeshActiveSectionIterator;
	};	

	
	FORCEINLINE FRealtimeMeshSectionProxy* FRealtimeMeshActiveSectionIterator::operator*() const
	{
		check(Proxy.Sections.IsValidIndex(Iterator.GetIndex()));
		return &Proxy.Sections[Iterator.GetIndex()].Get();
	}

	FORCEINLINE FRealtimeMeshSectionProxy& FRealtimeMeshActiveSectionIterator::operator->() const
	{
		check(Proxy.Sections.IsValidIndex(Iterator.GetIndex()));
		return Proxy.Sections[Iterator.GetIndex()].Get();
	}
}
