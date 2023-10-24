// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "RealtimeMeshCore.h"
#include "RealtimeMeshConfig.h"
#include "Mesh/RealtimeMeshDataStream.h"
#include "RealtimeMeshSection.h"
#include "Data/RealtimeMeshShared.h"

namespace RealtimeMesh
{
	class REALTIMEMESHCOMPONENT_API FRealtimeMeshSectionGroup : public TSharedFromThis<FRealtimeMeshSectionGroup>
	{
	protected:
		const FRealtimeMeshSharedResourcesRef SharedResources;
		const FRealtimeMeshSectionGroupKey Key;

		TSet<FRealtimeMeshStreamKey> Streams;
		TSet<FRealtimeMeshSectionRef, FRealtimeMeshSectionRefKeyFuncs> Sections;
		FRealtimeMeshBounds Bounds;

	public:
		FRealtimeMeshSectionGroup(const FRealtimeMeshSharedResourcesRef& InSharedResources, const FRealtimeMeshSectionGroupKey& InKey);
		virtual ~FRealtimeMeshSectionGroup();

		const FRealtimeMeshSectionGroupKey& GetKey() const { return Key; }
		FRealtimeMeshStreamRange GetInUseRange() const;
		FBoxSphereBounds3f GetLocalBounds() const;
		bool HasSections() const;
		int32 NumSections() const;
		bool HasStreams() const;

		TSet<FRealtimeMeshStreamKey> GetStreams() const { return Streams; }
		TSet<FRealtimeMeshStreamKey> GetStreamKeys() const;
		TSet<FRealtimeMeshSectionKey> GetSectionKeys() const
		{
			TSet<FRealtimeMeshSectionKey> SectionKeys;
			for (const auto& Section : Sections)
			{
				SectionKeys.Add(Section->GetKey());
			}
			return SectionKeys;
		}

		template <typename SectionType>
		TSharedPtr<SectionType> GetSectionAs(const FRealtimeMeshSectionKey& SectionKey) const
		{
			return StaticCastSharedPtr<SectionType>(GetSection(SectionKey));
		}

		FRealtimeMeshSectionPtr GetSection(const FRealtimeMeshSectionKey& SectionKey) const;


		virtual void Initialize(FRealtimeMeshProxyCommandBatch& Commands);
		TFuture<ERealtimeMeshProxyUpdateStatus> Reset();
		virtual void Reset(FRealtimeMeshProxyCommandBatch& Commands);

		virtual void SetOverrideBounds(const FBoxSphereBounds3f& InBounds);
		virtual void ClearOverrideBounds();

		TFuture<ERealtimeMeshProxyUpdateStatus> CreateOrUpdateStream(FRealtimeMeshStream&& Stream);
		virtual void CreateOrUpdateStream(FRealtimeMeshProxyCommandBatch& Commands, FRealtimeMeshStream&& Stream);
		TFuture<ERealtimeMeshProxyUpdateStatus> RemoveStream(const FRealtimeMeshStreamKey& StreamKey);
		virtual void RemoveStream(FRealtimeMeshProxyCommandBatch& Commands, const FRealtimeMeshStreamKey& StreamKey);

		TFuture<ERealtimeMeshProxyUpdateStatus> SetAllStreams(const FRealtimeMeshStreamSet& InStreams);
		void SetAllStreams(FRealtimeMeshProxyCommandBatch& Commands, const FRealtimeMeshStreamSet& InStreams);
		TFuture<ERealtimeMeshProxyUpdateStatus> SetAllStreams(FRealtimeMeshStreamSet&& InStreams);
		virtual void SetAllStreams(FRealtimeMeshProxyCommandBatch& Commands, FRealtimeMeshStreamSet&& InStreams);

		TFuture<ERealtimeMeshProxyUpdateStatus> CreateOrUpdateSection(const FRealtimeMeshSectionKey& SectionKey, const FRealtimeMeshSectionConfig& InConfig,
		                                                              const FRealtimeMeshStreamRange& InStreamRange);
		virtual void CreateOrUpdateSection(FRealtimeMeshProxyCommandBatch& Commands, const FRealtimeMeshSectionKey& SectionKey, const FRealtimeMeshSectionConfig& InConfig,
		                                   const FRealtimeMeshStreamRange& InStreamRange);
		TFuture<ERealtimeMeshProxyUpdateStatus> RemoveSection(const FRealtimeMeshSectionKey& SectionKey);
		virtual void RemoveSection(FRealtimeMeshProxyCommandBatch& Commands, const FRealtimeMeshSectionKey& SectionKey);

		virtual bool Serialize(FArchive& Ar);

		virtual void InitializeProxy(FRealtimeMeshProxyCommandBatch& Commands);


	protected:
		void InvalidateBounds() const;
		virtual FBoxSphereBounds3f CalculateBounds() const;
		virtual void HandleSectionChanged(const FRealtimeMeshSectionKey& RealtimeMeshSectionKey, ERealtimeMeshChangeType RealtimeMeshChange);
		virtual void HandleSectionBoundsChanged(const FRealtimeMeshSectionKey& RealtimeMeshSectionKey);
		virtual bool ShouldRecreateProxyOnStreamChange() const;
	};

	struct FRealtimeMeshSectionGroupRefKeyFuncs : BaseKeyFuncs<TSharedRef<FRealtimeMeshSectionGroup>, FRealtimeMeshSectionGroupKey, false>
	{
		/**
		 * @return The key used to index the given element.
		 */
		static KeyInitType GetSetKey(const TSharedRef<FRealtimeMeshSectionGroup>& Element)
		{
			return Element->GetKey();
		}

		/**
		 * @return True if the keys match.
		 */
		static bool Matches(KeyInitType A, KeyInitType B)
		{
			return A == B;
		}

		/** Calculates a hash index for a key. */
		static uint32 GetKeyHash(KeyInitType Key)
		{
			return GetTypeHash(Key);
		}
	};
}
