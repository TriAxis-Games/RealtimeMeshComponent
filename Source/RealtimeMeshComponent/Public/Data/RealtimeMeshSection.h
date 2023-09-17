// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "RealtimeMeshCore.h"
#include "RealtimeMeshConfig.h"
#include "RealtimeMeshShared.h"

namespace RealtimeMesh
{
	struct FRealtimeMeshProxyCommandBatch;
	struct FRealtimeMeshSectionUpdateContext;

	class REALTIMEMESHCOMPONENT_API FRealtimeMeshSection : public TSharedFromThis<FRealtimeMeshSection>
	{
	protected:
		const FRealtimeMeshSharedResourcesRef SharedResources;
		const FRealtimeMeshSectionKey Key;

	private:
		FRealtimeMeshSectionConfig Config;
		FRealtimeMeshStreamRange StreamRange;
		FRealtimeMeshBounds Bounds;

	public:
		FRealtimeMeshSection(const FRealtimeMeshSharedResourcesRef& InSharedResources, const FRealtimeMeshSectionKey& InKey);
		virtual ~FRealtimeMeshSection() = default;

		template <typename SectionGroupType>
		TSharedPtr<SectionGroupType> GetSectionGroupAs() const
		{
			return StaticCastSharedPtr<SectionGroupType>(GetSectionGroup());
		}

		FRealtimeMeshSectionGroupPtr GetSectionGroup() const;

		FRealtimeMeshSectionKey GetKey() const { return Key; }
		FRealtimeMeshSectionConfig GetConfig() const;
		FRealtimeMeshStreamRange GetStreamRange() const;
		FBoxSphereBounds3f GetLocalBounds() const;
		bool IsVisible() const { return GetConfig().bIsVisible; }
		bool IsCastingShadow() const { return GetConfig().bCastsShadow; }

		virtual void Initialize(FRealtimeMeshProxyCommandBatch& Commands, const FRealtimeMeshSectionConfig& InConfig, const FRealtimeMeshStreamRange& InRange);
		TFuture<ERealtimeMeshProxyUpdateStatus> Reset();
		virtual void Reset(FRealtimeMeshProxyCommandBatch& Commands);

		void SetOverrideBounds(const FBoxSphereBounds3f& InBounds);
		bool HasOverrideBounds() const { return Bounds.HasUserSetBounds(); }
		void ClearOverrideBounds();

		TFuture<ERealtimeMeshProxyUpdateStatus> UpdateConfig(const FRealtimeMeshSectionConfig& InConfig);
		virtual void UpdateConfig(FRealtimeMeshProxyCommandBatch& Commands, const FRealtimeMeshSectionConfig& InConfig);
		TFuture<ERealtimeMeshProxyUpdateStatus> UpdateConfig(TFunction<void(FRealtimeMeshSectionConfig&)> EditFunc);
		virtual void UpdateConfig(FRealtimeMeshProxyCommandBatch& Commands, TFunction<void(FRealtimeMeshSectionConfig&)> EditFunc);
		TFuture<ERealtimeMeshProxyUpdateStatus> UpdateStreamRange(const FRealtimeMeshStreamRange& InRange);
		virtual void UpdateStreamRange(FRealtimeMeshProxyCommandBatch& Commands, const FRealtimeMeshStreamRange& InRange);

		TFuture<ERealtimeMeshProxyUpdateStatus> SetVisibility(bool bIsVisible);
		virtual void SetVisibility(FRealtimeMeshProxyCommandBatch& Commands, bool bIsVisible);
		TFuture<ERealtimeMeshProxyUpdateStatus> SetCastShadow(bool bCastShadow);
		virtual void SetCastShadow(FRealtimeMeshProxyCommandBatch& Commands, bool bCastShadow);


		virtual bool Serialize(FArchive& Ar);

		virtual void InitializeProxy(FRealtimeMeshProxyCommandBatch& Commands);
		/*virtual void ApplyStateUpdate(FRealtimeMeshProxyCommandBatch& Commands, FRealtimeMeshSectionUpdateContext& Update);*/
	protected:
		void MarkBoundsDirtyIfNotOverridden() const;

		virtual FBoxSphereBounds3f CalculateBounds() const;

		virtual bool ShouldRecreateProxyOnChange() const { return Config.DrawType == ERealtimeMeshSectionDrawType::Static; }
	};

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
