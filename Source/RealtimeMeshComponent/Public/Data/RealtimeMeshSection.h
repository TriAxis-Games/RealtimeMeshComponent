// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "RealtimeMeshCore.h"
#include "RealtimeMeshConfig.h"
#include "Data/RealtimeMeshShared.h"

namespace RealtimeMesh
{
	struct FRealtimeMeshProxyCommandBatch;

	/**
	 * @brief This is the base class for all section types. It contains the shared resources, and the key used to identify the section.
	 * It also contains the bounds, and also the config and stream range for the section.
	 * This class is meant to be subclassed for custom functionality, providing only the common functionality here.
	 */
	class REALTIMEMESHCOMPONENT_API FRealtimeMeshSection : public TSharedFromThis<FRealtimeMeshSection>
	{
	protected:
		// Reference to the shared resources
		const FRealtimeMeshSharedResourcesRef SharedResources;

		// Key used to identify this section
		const FRealtimeMeshSectionKey Key;

	private:
		// Current config of this section
		FRealtimeMeshSectionConfig Config;

		// Current stream range of this section, used to render a portion of the parent SectionGroups buffers
		FRealtimeMeshStreamRange StreamRange;

		// Bounds for this section in local space
		FRealtimeMeshBounds Bounds;

	public:
		FRealtimeMeshSection(const FRealtimeMeshSharedResourcesRef& InSharedResources, const FRealtimeMeshSectionKey& InKey);
		virtual ~FRealtimeMeshSection() = default;

		/**
		 * @brief Gets the parent SectionGroup as the specified type
		 * @tparam SectionGroupType Type of the section group
		 * @return 
		 */
		template <typename SectionGroupType>
		TSharedPtr<SectionGroupType> GetSectionGroupAs() const
		{
			return StaticCastSharedPtr<SectionGroupType>(GetSectionGroup());
		}

		/**
		 * @brief 
		 * @return Gets the current parent SectionGroup
		 */
		FRealtimeMeshSectionGroupPtr GetSectionGroup() const;

		/**
		 * @brief 
		 * @return Gets the key used to identify this section
		 */
		const FRealtimeMeshSectionKey& GetKey() const { return Key; }

		/**
		 * @brief 
		 * @return Gets the Config for this section
		 */
		FRealtimeMeshSectionConfig GetConfig() const;

		/**
		 * @brief 
		 * @return Gets the StreamRange for this section, which controls the range of the parent buffers we render for this section
		 */
		FRealtimeMeshStreamRange GetStreamRange() const;

		/**
		 * @brief 
		 * @return Gets the bounds for this section in local space
		 */
		FBoxSphereBounds3f GetLocalBounds() const;

		/**
		 * @brief 
		 * @return Is this section currently visible
		 */
		bool IsVisible() const { return GetConfig().bIsVisible; }

		/**
		 * @brief 
		 * @return Is this section currently casting shadows
		 */
		bool IsCastingShadow() const { return GetConfig().bCastsShadow; }

		/**
		 * @brief Initializes this section, setting up the starting config/streamrange
		 * @param Commands Running commandqueue that we send RT commands too. This is used for command batching.
		 * @param InConfig Initial config for this section
		 * @param InRange Initial stream range to render for this section
		 */
		virtual void Initialize(FRealtimeMeshProxyCommandBatch& Commands, const FRealtimeMeshSectionConfig& InConfig, const FRealtimeMeshStreamRange& InRange);

		/**
		 * @brief Resets the section to a default state. For batching several commands together, use the Reset(FRealtimeMeshProxyCommandBatch&) variant.
		 * @return Future that will be set when the reset is complete, and will provide the status of the command completion.
		 */
		TFuture<ERealtimeMeshProxyUpdateStatus> Reset();

		/**
		 * @brief Resets the section to a default state
		 * @param Commands Running command queue that we send RT commands too. This is used for command batching.
		 */
		virtual void Reset(FRealtimeMeshProxyCommandBatch& Commands);

		/**
		 * @brief Sets a custom bounds for this section, overriding the calculated bounds and skipping calculation.
		 * @param InBounds The new bounds to use for this section in local space
		 */
		void SetOverrideBounds(const FBoxSphereBounds3f& InBounds);

		/**
		 * @brief 
		 * @return Does this section have a custom bounds set 
		 */
		bool HasOverrideBounds() const { return Bounds.HasUserSetBounds(); }

		/**
		 * @brief Clears any overriding bounds
		 */
		void ClearOverrideBounds();

		/**
		 * @brief Update the config for this section
		 * @param InConfig New section config
		 * @return Future that will be set when the update is complete, and will provide the status of the command completion.
		 */
		TFuture<ERealtimeMeshProxyUpdateStatus> UpdateConfig(const FRealtimeMeshSectionConfig& InConfig);

		/**
		 * @brief Update the config for this section
		 * @param Commands Running command queue that we send RT commands too. This is used for command batching.
		 * @param InConfig New section config
		 */
		virtual void UpdateConfig(FRealtimeMeshProxyCommandBatch& Commands, const FRealtimeMeshSectionConfig& InConfig);

		/**
		 * @brief Edits the config for this section using the specified function
		 * @param EditFunc Function to call to edit the config
		 * @return Future that will be set when the update is complete, and will provide the status of the command completion.
		 */
		TFuture<ERealtimeMeshProxyUpdateStatus> UpdateConfig(TFunction<void(FRealtimeMeshSectionConfig&)> EditFunc);

		/**
		 * @brief Edits the config for this section using the specified function
		 * @param Commands Running command queue that we send RT commands too. This is used for command batching.
		 * @param EditFunc Function to call to edit the config
		 */
		virtual void UpdateConfig(FRealtimeMeshProxyCommandBatch& Commands, TFunction<void(FRealtimeMeshSectionConfig&)> EditFunc);

		/**
		 * @brief Update the stream range for this section
		 * @param InRange New section stream range
		 * @return Future that will be set when the update is complete, and will provide the status of the command completion.
		 */
		TFuture<ERealtimeMeshProxyUpdateStatus> UpdateStreamRange(const FRealtimeMeshStreamRange& InRange);

		/**
		 * @brief Update the stream range for this section
		 * @param Commands Running command queue that we send RT commands too. This is used for command batching.
		 * @param InRange New section stream range
		 */
		virtual void UpdateStreamRange(FRealtimeMeshProxyCommandBatch& Commands, const FRealtimeMeshStreamRange& InRange);

		/**
		 * @brief Update the visibility for the section
		 * @param bIsVisible New visibility for section
		 * @return Future that will be set when the update is complete, and will provide the status of the command completion.
		 */
		TFuture<ERealtimeMeshProxyUpdateStatus> SetVisibility(bool bIsVisible);

		/**
		 * @brief Update the visibility for the section
		 * @param Commands Running command queue that we send RT commands too. This is used for command batching.
		 * @param bIsVisible New visibility for section
		 */
		virtual void SetVisibility(FRealtimeMeshProxyCommandBatch& Commands, bool bIsVisible);

		/**
		 * @brief Update the shadow casting state for the section
		 * @param bCastShadow New shadow casting state for section
		 * @return Future that will be set when the update is complete, and will provide the status of the command completion.
		 */
		TFuture<ERealtimeMeshProxyUpdateStatus> SetCastShadow(bool bCastShadow);

		/**
		 * @brief Update the shadow casting state for the section
		 * @param Commands Running command queue that we send RT commands too. This is used for command batching.
		 * @param bCastShadow New shadow casting state for section
		 */
		virtual void SetCastShadow(FRealtimeMeshProxyCommandBatch& Commands, bool bCastShadow);

		/**
		 * @brief Serializes this section to the running archive.
		 * @param Ar Archive to serialize too.
		 * @return 
		 */
		virtual bool Serialize(FArchive& Ar);

		/**
		 * @brief Initializes the proxy, adding all the necessary commands to initialize the state to the supplied CommandBatch
		 * @param Commands Running command queue that we send RT commands too. This is used for command batching.
		 */
		virtual void InitializeProxy(FRealtimeMeshProxyCommandBatch& Commands);
	protected:
		/**
		 * @brief Marks the bounds dirty, to request a recalculate, if the bounds are not overriden by a custom bound
		 */
		void MarkBoundsDirtyIfNotOverridden() const;

		/**
		 * @brief Calculates the bounds from the mesh data for this section
		 * @return The new calculated bounds.
		 */
		virtual FBoxSphereBounds3f CalculateBounds() const;

		/**
		 * @brief 
		 * @return Should we request a proxy recreate for the component when this section changes?
		 */
		virtual bool ShouldRecreateProxyOnChange() const { return Config.DrawType == ERealtimeMeshSectionDrawType::Static; }
	};


	/**
	 * @brief Custom KeyFuncs used to index the section set by the section key.
	 */
	struct FRealtimeMeshSectionRefKeyFuncs : BaseKeyFuncs<TSharedRef<FRealtimeMeshSection>, FRealtimeMeshSectionKey, false>
	{
		/**
		 * @return The key used to index the given element.
		 */
		static KeyInitType GetSetKey(ElementInitType Element)
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
