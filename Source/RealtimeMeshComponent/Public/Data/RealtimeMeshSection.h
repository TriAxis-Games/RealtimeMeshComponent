// Copyright (c) 2015-2024 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "RealtimeMeshCore.h"
#include "Data/RealtimeMeshShared.h"
#include "RenderProxy/RealtimeMeshProxy.h"


namespace RealtimeMesh
{
	struct FRealtimeMeshLockContext;
	struct FRealtimeMeshUpdateContext;
	
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
		 * @brief Gets the current parent SectionGroup
		 * @param LockContext Context object for access to the RMC data
		 * @return Parent SectionGroup to this section
		 */
		FRealtimeMeshSectionGroupPtr GetSectionGroup(const FRealtimeMeshLockContext& LockContext) const;
		
		/**
		 * @brief Gets the parent SectionGroup as the specified type
		 * @tparam SectionGroupType Type of the section group
		 * @param LockContext Context object for access to the RMC data
		 * @return 
		 */
		template <typename SectionGroupType>
		TSharedPtr<SectionGroupType> GetSectionGroupAs(const FRealtimeMeshLockContext& LockContext) const
		{
			return StaticCastSharedPtr<SectionGroupType>(GetSectionGroup(LockContext));
		}

		/**
		 * @brief Gets the key used to identify this section
		 * @param LockContext Context object for access to the RMC data
		 */
		const FRealtimeMeshSectionKey& GetKey(const FRealtimeMeshLockContext& LockContext) const { return Key; }

		/**
		 * @brief Gets the Config for this section
		 * @param LockContext Context object for access to the RMC data
		 */
		FRealtimeMeshSectionConfig GetConfig(const FRealtimeMeshLockContext& LockContext) const;

		/**
		 * @brief Gets the StreamRange for this section, which controls the range of the parent buffers we render for this section
		 * @param LockContext Context object for access to the RMC data
		 */
		FRealtimeMeshStreamRange GetStreamRange(const FRealtimeMeshLockContext& LockContext) const;

		/**
		 * @brief Gets the bounds for this section in local space
		 * @param LockContext Context object for access to the RMC data
		 */
		TOptional<FBoxSphereBounds3f> GetLocalBounds(const FRealtimeMeshLockContext& LockContext) const;

		/**
		 * @brief Is this section currently visible
		 * @param LockContext Context object for access to the RMC data
		 */
		bool IsVisible(const FRealtimeMeshLockContext& LockContext) const { return GetConfig(LockContext).bIsVisible; }

		/**
		 * @brief Is this section currently casting shadows
		 * @param LockContext Context object for access to the RMC data
		 */
		bool IsCastingShadow(const FRealtimeMeshLockContext& LockContext) const { return GetConfig(LockContext).bCastsShadow; }

		/**
		 * @brief Initializes this section, setting up the starting config/streamrange
		 * @param UpdateContext Update context used for this operation
		 * @param InConfig Initial config for this section
		 * @param InRange Initial stream range to render for this section
		 */
		virtual void Initialize(FRealtimeMeshUpdateContext& UpdateContext, const FRealtimeMeshSectionConfig& InConfig, const FRealtimeMeshStreamRange& InRange);

		/**
		 * @brief Resets the section to a default state
		 * @param UpdateContext Update context used for this operation
		 */
		virtual void Reset(FRealtimeMeshUpdateContext& UpdateContext);

		/**
		 * @brief Sets a custom bounds for this section, overriding the calculated bounds and skipping calculation.
		 * @param UpdateContext Update context used for this operation
		 * @param InBounds The new bounds to use for this section in local space
		 */
		void SetOverrideBounds(FRealtimeMeshUpdateContext& UpdateContext, const FBoxSphereBounds3f& InBounds);

		/**
		 * @brief Does this section have custom bounds
		 * @param LockContext Context object for access to the RMC data
		 * @return Does this section have a custom bounds set 
		 */
		bool HasOverrideBounds(const FRealtimeMeshLockContext& LockContext) const { return Bounds.HasUserSetBounds(); }

		/**
		 * @brief Clears any overriding bounds
		 */
		void ClearOverrideBounds(FRealtimeMeshUpdateContext& UpdateContext);

		/**
		 * @brief Update the config for this section
		 * @param UpdateContext Update context used for this operation
		 * @param InConfig New section config
		 */
		virtual void UpdateConfig(FRealtimeMeshUpdateContext& UpdateContext, const FRealtimeMeshSectionConfig& InConfig);

		/**
		 * @brief Edits the config for this section using the specified function
		 * @param UpdateContext Update context used for this operation
		 * @param EditFunc Function to call to edit the config
		 */
		virtual void UpdateConfig(FRealtimeMeshUpdateContext& UpdateContext, TFunction<void(FRealtimeMeshSectionConfig&)> EditFunc);

		/**
		 * @brief Update the stream range for this section
		 * @param UpdateContext Update context used for this operation
		 * @param InRange New section stream range
		 */
		virtual void UpdateStreamRange(FRealtimeMeshUpdateContext& UpdateContext, const FRealtimeMeshStreamRange& InRange);

		/**
		 * @brief Update the visibility for the section
		 * @param UpdateContext Update context used for this operation
		 * @param bIsVisible New visibility for section
		 */
		virtual void SetVisibility(FRealtimeMeshUpdateContext& UpdateContext, bool bIsVisible);

		/**
		 * @brief Update the shadow casting state for the section
		 * @param UpdateContext Update context used for this operation
		 * @param bCastShadow New shadow casting state for section
		 */
		virtual void SetCastShadow(FRealtimeMeshUpdateContext& UpdateContext, bool bCastShadow);

		/**
		 * @brief Serializes this section to the running archive.
		 * @param Ar Archive to serialize too.
		 * @return 
		 */
		virtual bool Serialize(FArchive& Ar);

		/**
		 * @brief Initializes the proxy, adding all the necessary commands to initialize the state to the supplied CommandBatch
		 * @param UpdateContext Update context used for this operation
		 */
		virtual void InitializeProxy(FRealtimeMeshUpdateContext& UpdateContext);


		virtual void FinalizeUpdate(FRealtimeMeshUpdateContext& UpdateContext);

		virtual bool ShouldRecreateProxyOnChange(const FRealtimeMeshLockContext& LockContext) const;
	protected:
		const FRealtimeMeshSectionKey& GetKey_AssumesLocked() const { return Key; }
		friend struct FRealtimeMeshSectionRefKeyFuncs;
		friend class FRealtimeMeshSectionGroup;
		
		void MarkBoundsDirtyIfNotOverridden(FRealtimeMeshUpdateContext& UpdateContext);
		void UpdateCalculatedBounds(FRealtimeMeshUpdateContext& UpdateContext, TOptional<FBoxSphereBounds3f>& CalculatedBounds);
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
			return Element->GetKey_AssumesLocked();
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
