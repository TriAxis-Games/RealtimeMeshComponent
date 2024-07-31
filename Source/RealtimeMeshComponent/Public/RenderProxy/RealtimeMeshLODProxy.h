// Copyright (c) 2015-2024 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "RealtimeMeshConfig.h"
#include "RealtimeMeshCore.h"
#include "RealtimeMeshProxyShared.h"
#include "RealtimeMeshSectionGroupProxy.h"

namespace RealtimeMesh
{
	using FRealtimeMeshSectionGroupMask = TBitArray<TInlineAllocator<1>>;
	
	class FRealtimeMeshActiveSectionGroupIterator
	{
	private:
		const FRealtimeMeshLODProxy& Proxy;
		TConstSetBitIterator<TInlineAllocator<1>> Iterator;

	public:
		FRealtimeMeshActiveSectionGroupIterator(const FRealtimeMeshLODProxy& InProxy, const FRealtimeMeshSectionGroupMask& InMask)
			: Proxy(InProxy), Iterator(TConstSetBitIterator(InMask)) { }
		
		/** Forwards iteration operator. */
		FORCEINLINE FRealtimeMeshActiveSectionGroupIterator& operator++()
		{
			++Iterator;
			return *this;
		}

		FORCEINLINE bool operator==(const FRealtimeMeshActiveSectionGroupIterator& Other) const
		{
			return Iterator == Other.Iterator;
		}

		FORCEINLINE bool operator!=(const FRealtimeMeshActiveSectionGroupIterator& Other) const
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

		FRealtimeMeshSectionGroupProxy* operator*() const;
		FRealtimeMeshSectionGroupProxy& operator->() const;
		
		/** Index accessor. */
		FORCEINLINE int32 GetIndex() const
		{
			return Iterator.GetIndex();
		}
	};
	
	
	class REALTIMEMESHCOMPONENT_API FRealtimeMeshLODProxy : public TSharedFromThis<FRealtimeMeshLODProxy>
	{
	private:
		const FRealtimeMeshSharedResourcesRef SharedResources;
		const FRealtimeMeshLODKey Key;
		TArray<FRealtimeMeshSectionGroupProxyRef> SectionGroups;
		TMap<FRealtimeMeshSectionGroupKey, uint32> SectionGroupMap;
		FRealtimeMeshSectionGroupMask ActiveSectionGroupMask;		
		FRealtimeMeshSectionGroupMask ActiveStaticSectionGroupMask;
		FRealtimeMeshSectionGroupMask ActiveDynamicSectionGroupMask;
		TOptional<FRealtimeMeshSectionGroupKey> OverrideStaticRayTracingGroup;

		FRealtimeMeshLODConfig Config;
		FRealtimeMeshDrawMask DrawMask;
#if RHI_RAYTRACING
		FRealtimeMeshSectionGroupProxyPtr StaticRaytracingSectionGroup;
#endif
		uint32 bIsStateDirty : 1;
		

	public:
		FRealtimeMeshLODProxy(const FRealtimeMeshSharedResourcesRef& InSharedResources, const FRealtimeMeshLODKey& InKey);
		virtual ~FRealtimeMeshLODProxy();

		const FRealtimeMeshLODKey& GetKey() const { return Key; }
		const FRealtimeMeshLODConfig& GetConfig() const { return Config; }
		FRealtimeMeshDrawMask GetDrawMask() const { return DrawMask; }
		FRealtimeMeshActiveSectionGroupIterator GetActiveSectionGroupMaskIter() const { return FRealtimeMeshActiveSectionGroupIterator(*this, ActiveSectionGroupMask); }
		FRealtimeMeshActiveSectionGroupIterator GetActiveStaticSectionGroupMaskIter() const { return FRealtimeMeshActiveSectionGroupIterator(*this, ActiveStaticSectionGroupMask); }
		FRealtimeMeshActiveSectionGroupIterator GetActiveDynamicSectionGroupMaskIter() const { return FRealtimeMeshActiveSectionGroupIterator(*this, ActiveDynamicSectionGroupMask); }
		float GetScreenSize() const { return Config.ScreenSize; }

		FRealtimeMeshSectionGroupProxyPtr GetStaticRayTracedSectionGroup() const { return StaticRaytracingSectionGroup; }

		FRealtimeMeshSectionGroupProxyPtr GetSectionGroup(const FRealtimeMeshSectionGroupKey& SectionGroupKey) const;

		virtual void UpdateConfig(const FRealtimeMeshLODConfig& NewConfig);

		virtual void CreateSectionGroupIfNotExists(const FRealtimeMeshSectionGroupKey& SectionGroupKey);
		virtual void RemoveSectionGroup(const FRealtimeMeshSectionGroupKey& SectionGroupKey);

#if RHI_RAYTRACING
		virtual FRayTracingGeometry* GetStaticRayTracingGeometry() const;
#endif
		
		virtual bool UpdateCachedState(bool bShouldForceUpdate);
		virtual void Reset();

	protected:
		void MarkStateDirty();
		void RebuildSectionGroupMap();

		friend class FRealtimeMeshActiveSectionGroupIterator;
	};

	
	FORCEINLINE FRealtimeMeshSectionGroupProxy* FRealtimeMeshActiveSectionGroupIterator::operator*() const
	{
		check(Proxy.SectionGroups.IsValidIndex(Iterator.GetIndex()));
		return &Proxy.SectionGroups[Iterator.GetIndex()].Get();
	}

	FORCEINLINE FRealtimeMeshSectionGroupProxy& FRealtimeMeshActiveSectionGroupIterator::operator->() const
	{
		check(Proxy.SectionGroups.IsValidIndex(Iterator.GetIndex()));
		return Proxy.SectionGroups[Iterator.GetIndex()].Get();
	}
}
