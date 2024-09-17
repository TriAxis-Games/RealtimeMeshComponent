// Copyright (c) 2015-2024 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "RealtimeMeshCore.h"
#include "Core/RealtimeMeshDataStream.h"
#include "RealtimeMeshSection.h"
#include "Data/RealtimeMeshShared.h"
#include "Core/RealtimeMeshKeys.h"
#include "Core/RealtimeMeshSectionGroupConfig.h"

namespace RealtimeMesh
{
	class REALTIMEMESHCOMPONENT_API FRealtimeMeshSectionGroup : public TSharedFromThis<FRealtimeMeshSectionGroup>
	{
	protected:
		const FRealtimeMeshSharedResourcesRef SharedResources;
		const FRealtimeMeshSectionGroupKey Key;

		TSet<FRealtimeMeshStreamKey> Streams;
		TSet<FRealtimeMeshSectionRef, FRealtimeMeshSectionRefKeyFuncs> Sections;
		FRealtimeMeshSectionGroupConfig Config;
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

		TSet<FRealtimeMeshStreamKey> GetStreamKeys() const;
		TSet<FRealtimeMeshSectionKey> GetSectionKeys() const;

		template <typename SectionType>
		TSharedPtr<SectionType> GetSectionAs(const FRealtimeMeshSectionKey& SectionKey) const
		{
			return StaticCastSharedPtr<SectionType>(GetSection(SectionKey));
		}

		FRealtimeMeshSectionPtr GetSection(const FRealtimeMeshSectionKey& SectionKey) const;


		virtual void Initialize(FRealtimeMeshProxyUpdateBuilder& ProxyBuilder, const FRealtimeMeshSectionGroupConfig& InConfig);
		TFuture<ERealtimeMeshProxyUpdateStatus> Reset();
		virtual void Reset(FRealtimeMeshProxyUpdateBuilder& ProxyBuilder);

		virtual void SetOverrideBounds(const FBoxSphereBounds3f& InBounds);
		virtual void ClearOverrideBounds();

		
		/**
		 * @brief Update the config for this section group
		 * @param InConfig New section group config
		 * @return Future that will be set when the update is complete, and will provide the status of the command completion.
		 */
		TFuture<ERealtimeMeshProxyUpdateStatus> UpdateConfig(const FRealtimeMeshSectionGroupConfig& InConfig);

		/**
		 * @brief Update the config for this section group
		 * @param ProxyBuilder Running command queue that we send RT commands too. This is used for command batching.
		 * @param InConfig New section group config
		 */
		virtual void UpdateConfig(FRealtimeMeshProxyUpdateBuilder& ProxyBuilder, const FRealtimeMeshSectionGroupConfig& InConfig);

		/**
		 * @brief Edits the config for this section group using the specified function
		 * @param EditFunc Function to call to edit the config
		 * @return Future that will be set when the update is complete, and will provide the status of the command completion.
		 */
		TFuture<ERealtimeMeshProxyUpdateStatus> UpdateConfig(TFunction<void(FRealtimeMeshSectionGroupConfig&)> EditFunc);

		/**
		 * @brief Edits the config for this section group using the specified function
		 * @param ProxyBuilder Running command queue that we send RT commands too. This is used for command batching.
		 * @param EditFunc Function to call to edit the config
		 */
		virtual void UpdateConfig(FRealtimeMeshProxyUpdateBuilder& ProxyBuilder, TFunction<void(FRealtimeMeshSectionGroupConfig&)> EditFunc);

		TFuture<ERealtimeMeshProxyUpdateStatus> CreateOrUpdateStream(FRealtimeMeshStream&& Stream);
		virtual void CreateOrUpdateStream(FRealtimeMeshProxyUpdateBuilder& ProxyBuilder, FRealtimeMeshStream&& Stream);
		TFuture<ERealtimeMeshProxyUpdateStatus> RemoveStream(const FRealtimeMeshStreamKey& StreamKey);
		virtual void RemoveStream(FRealtimeMeshProxyUpdateBuilder& ProxyBuilder, const FRealtimeMeshStreamKey& StreamKey);

		TFuture<ERealtimeMeshProxyUpdateStatus> SetAllStreams(const FRealtimeMeshStreamSet& InStreams);
		void SetAllStreams(FRealtimeMeshProxyUpdateBuilder& ProxyBuilder, const FRealtimeMeshStreamSet& InStreams);
		TFuture<ERealtimeMeshProxyUpdateStatus> SetAllStreams(FRealtimeMeshStreamSet&& InStreams);
		virtual void SetAllStreams(FRealtimeMeshProxyUpdateBuilder& ProxyBuilder, FRealtimeMeshStreamSet&& InStreams);

		TFuture<ERealtimeMeshProxyUpdateStatus> CreateOrUpdateSection(const FRealtimeMeshSectionKey& SectionKey, const FRealtimeMeshSectionConfig& InConfig,
		                                                              const FRealtimeMeshStreamRange& InStreamRange);
		virtual void CreateOrUpdateSection(FRealtimeMeshProxyUpdateBuilder& ProxyBuilder, const FRealtimeMeshSectionKey& SectionKey, const FRealtimeMeshSectionConfig& InConfig,
										   const FRealtimeMeshStreamRange& InStreamRange);
		TFuture<ERealtimeMeshProxyUpdateStatus> RemoveSection(const FRealtimeMeshSectionKey& SectionKey);
		virtual void RemoveSection(FRealtimeMeshProxyUpdateBuilder& ProxyBuilder, const FRealtimeMeshSectionKey& SectionKey);

		virtual bool Serialize(FArchive& Ar);

		virtual void InitializeProxy(FRealtimeMeshProxyUpdateBuilder& ProxyBuilder);

		/**
		 * @brief 
		 * @return Should we request a proxy recreate for the component when this section group changes?
		 */
		virtual bool ShouldRecreateProxyOnChange() const { return Config.DrawType == ERealtimeMeshSectionDrawType::Static; }
	protected:
		void InvalidateBounds() const;
		virtual FBoxSphereBounds3f CalculateBounds() const;
		virtual void HandleSectionChanged(const FRealtimeMeshSectionKey& RealtimeMeshSectionKey, ERealtimeMeshChangeType RealtimeMeshChange);
		virtual void HandleSectionBoundsChanged(const FRealtimeMeshSectionKey& RealtimeMeshSectionKey);

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
