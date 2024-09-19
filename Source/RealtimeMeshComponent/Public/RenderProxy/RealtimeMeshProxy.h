// Copyright (c) 2015-2024 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "RealtimeMeshComponentProxy.h"
#include "RealtimeMeshCore.h"
#include "RealtimeMeshProxyCommandBatch.h"
#include "RealtimeMeshProxyShared.h"
#include "HAL/ThreadSafeBool.h"
#include "Containers/Queue.h"
#include "Mesh/RealtimeMeshCardRepresentation.h"
#include "Mesh/RealtimeMeshDistanceField.h"
#include "PhysicsEngine/AggregateGeom.h"


struct FRealtimeMeshDistanceField;
enum class ERealtimeMeshSectionDrawType : uint8;

namespace RealtimeMesh
{
	struct IRealtimeMeshNaniteResources;

	static_assert(REALTIME_MESH_MAX_LODS <= FBitSet::BitsPerWord, "REALTIME_MESH_MAX_LODS must be less than or equal to FBitSet::BitsPerWord");
	using FRealtimeMeshLODMask = TBitArray<TFixedAllocator<1>>;


	class FRealtimeMeshActiveLODIterator
	{
	private:
		const FRealtimeMeshProxy& Proxy;
		TConstSetBitIterator<TFixedAllocator<1>> Iterator;

	public:
		FRealtimeMeshActiveLODIterator(const FRealtimeMeshProxy& InProxy, const FRealtimeMeshLODMask& InMask)
			: Proxy(InProxy), Iterator(TConstSetBitIterator(InMask)) { }
		
		/** Forwards iteration operator. */
		FORCEINLINE FRealtimeMeshActiveLODIterator& operator++()
		{
			++Iterator;
			return *this;
		}

		FORCEINLINE bool operator==(const FRealtimeMeshActiveLODIterator& Other) const
		{
			return Iterator == Other.Iterator;
		}

		FORCEINLINE bool operator!=(const FRealtimeMeshActiveLODIterator& Other) const
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

		FRealtimeMeshLODProxy* operator*() const;
		FRealtimeMeshLODProxy& operator->() const;
		
		/** Index accessor. */
		FORCEINLINE int32 GetIndex() const
		{
			return Iterator.GetIndex();
		}
	};


	
	
	class REALTIMEMESHCOMPONENT_API FRealtimeMeshProxy : public TSharedFromThis<FRealtimeMeshProxy>
	{		
	protected:		
		const FRealtimeMeshSharedResourcesRef SharedResources;
		TFixedLODArray<FRealtimeMeshLODProxyPtr> LODs;
		FRealtimeMeshDrawMask DrawMask;
		FRealtimeMeshLODMask ActiveLODMask;
		FRealtimeMeshLODMask ScreenPercentageNextLODMask;
		FRealtimeMeshLODMask ActiveStaticLODMask;
		FRealtimeMeshLODMask ActiveDynamicLODMask;

		TUniquePtr<FDistanceFieldVolumeData> DistanceField;
		TUniquePtr<FCardRepresentationData> CardRepresentation;

		TSharedPtr<IRealtimeMeshNaniteResources> NaniteResources;

		

		struct FCommandBatch
		{
			TArray<FRealtimeMeshProxyUpdateBuilder::TaskFunctionType> Tasks;
			TSharedPtr<FRealtimeMeshCommandBatchIntermediateFuture> ThreadState;
		};
		TMpscQueue<FCommandBatch> CommandQueue;
		FCriticalSection CommandQueueLock;

#if UE_ENABLE_DEBUG_DRAWING
		// If debug drawing is enabled, we store collision data here so that collision shapes can be rendered when requested by showflags

		bool bOwnerIsNull = true;
		/** Whether the collision data has been set up for rendering */
		bool bHasCollisionData = false;

		/** Collision trace flags */
		ECollisionTraceFlag		CollisionTraceFlag;
		/** Collision Response of this component */
		FCollisionResponseContainer CollisionResponse;
		/** Cached AggGeom holding the collision shapes to render */
		FKAggregateGeom CachedAggGeom;

#endif
	public:
		FRealtimeMeshProxy(const FRealtimeMeshSharedResourcesRef& InSharedResources);
		virtual ~FRealtimeMeshProxy();

		const FRealtimeMeshSharedResourcesRef& GetSharedResources() const { return SharedResources; }

		virtual ERHIFeatureLevel::Type GetRHIFeatureLevel() const;
		FRealtimeMeshDrawMask GetDrawMask() const { return DrawMask; }
		int32 GetFirstLODIndex() const { return ActiveLODMask.Find(true); }
		int32 GetLastLODIndex() const { return ActiveLODMask.FindLast(true); }
		FRealtimeMeshActiveLODIterator GetActiveLODMaskIter() const { return FRealtimeMeshActiveLODIterator(*this, ActiveLODMask); }
		FRealtimeMeshActiveLODIterator GetActiveStaticLODMaskIter() const { return FRealtimeMeshActiveLODIterator(*this, ActiveStaticLODMask); }
		FRealtimeMeshActiveLODIterator GetActiveDynamicLODMaskIter() const { return FRealtimeMeshActiveLODIterator(*this, ActiveDynamicLODMask); }
		
		TRange<float> GetScreenSizeRangeForLOD(const FRealtimeMeshLODKey& LODKey) const;

		virtual void SetDistanceField(FRealtimeMeshDistanceField&& InDistanceField);
		bool HasDistanceFieldData() const;
		const FDistanceFieldVolumeData* GetDistanceFieldData() const { return DistanceField.Get(); }
		
		virtual void SetNaniteResources(const TSharedPtr<IRealtimeMeshNaniteResources>& InNaniteResources);
		bool HasNaniteResources() const;
		template<typename ResourcesType>
		TSharedPtr<ResourcesType> GetNaniteResources() const { return StaticCastSharedPtr<ResourcesType>(NaniteResources); }
		
		virtual void SetCardRepresentation(FRealtimeMeshCardRepresentation&& InCardRepresentation);
		bool HasCardRepresentation() const { return CardRepresentation.IsValid(); }
		const FCardRepresentationData* GetCardRepresentation() const { return CardRepresentation.IsValid()? CardRepresentation.Get() : nullptr; }
		void ClearCardRepresentation() { CardRepresentation.Reset(); }

		int8 GetNumLODs() const { return LODs.Num(); }
		FRealtimeMeshLODProxyPtr GetLOD(FRealtimeMeshLODKey LODKey) const;

		virtual void AddLODIfNotExists(const FRealtimeMeshLODKey& LODKey);
		virtual void RemoveLOD(const FRealtimeMeshLODKey& LODKey);

#if UE_ENABLE_DEBUG_DRAWING
		virtual void SetCollisionRenderData(const FKAggregateGeom& InAggGeom, ECollisionTraceFlag InCollisionTraceFlag, const FCollisionResponseContainer& InCollisionResponse);
#endif

		void EnqueueCommandBatch(TArray<FRealtimeMeshProxyUpdateBuilder::TaskFunctionType>&& InTasks, const TSharedPtr<FRealtimeMeshCommandBatchIntermediateFuture>& ThreadState);
		void ProcessCommands(FRHICommandListBase& RHICmdList);
		
		virtual void UpdatedCachedState(FRHICommandListBase& RHICmdList);
		virtual void Reset();

	protected:

		friend class FRealtimeMeshActiveLODIterator;
	};



	FORCEINLINE FRealtimeMeshLODProxy* FRealtimeMeshActiveLODIterator::operator*() const
	{
		check(Proxy.LODs.IsValidIndex(Iterator.GetIndex()) && Proxy.LODs[Iterator.GetIndex()].IsValid());
		return Proxy.LODs[Iterator.GetIndex()].Get();
	}

	FORCEINLINE FRealtimeMeshLODProxy& FRealtimeMeshActiveLODIterator::operator->() const
	{
		check(Proxy.LODs.IsValidIndex(Iterator.GetIndex()) && Proxy.LODs[Iterator.GetIndex()].IsValid());
		return *Proxy.LODs[Iterator.GetIndex()].Get();
	}
	
}
