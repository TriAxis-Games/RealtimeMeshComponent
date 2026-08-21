// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RealtimeMeshManaged.h"
#include "Data/RealtimeMeshLOD.h"
#include "Data/RealtimeMeshSection.h"
#include "Data/RealtimeMeshBufferSet.h"
#include "Interface_CollisionDataProviderCore.h"
#include "Core/RealtimeMeshBuilder.h"
#include "Core/RealtimeMeshDataStream.h"
#include "Mesh/RealtimeMeshDistanceField.h"
#include "Mesh/RealtimeMeshCardRepresentation.h"
#include "RealtimeMeshSimple.generated.h"


class URealtimeMeshStreamSet;
class URealtimeMeshSimple;


namespace RealtimeMesh
{
	DECLARE_DELEGATE_RetVal_OneParam(FRealtimeMeshSectionConfig, FRealtimeMeshPolyGroupConfigHandler, int32);

	/**
	 * BufferSet leaf for the Simple leaf. Adds PolyGroup-driven section auto-
	 * creation on top of the Managed-tier StreamSet machinery. Also exposes
	 * the StreamSet read/edit surface (ProcessMeshData / EditMeshData) that
	 * Simple's builder-flavored API plumbs through.
	 */
	class REALTIMEMESHCOMPONENT_API FRealtimeMeshBufferSetSimple : public FRealtimeMeshBufferSetManaged
	{
		FRealtimeMeshPolyGroupConfigHandler ConfigHandler;
		uint8 bAutoCreateSectionsForPolygonGroups : 1;

		// Per-instance suppression flag for nested PolyGroup auto-section updates.
		// SetAllStreams flips this on around the bulk write so each individual
		// CreateOrUpdateStream skips its per-stream PolyGroup re-evaluation;
		// SetAllStreams then does one bulk re-evaluation at the end. Scoped via
		// TGuardValue at the call site so nesting works correctly.
		bool bDeferPolyGroupUpdates = false;

		// Section keys derived from the fixed group Key depend only on the polygroup index, so cache them
		// per index. This avoids re-synthesizing the FString + FName (name-table lock) + CRC on every
		// polygroup update. The key derivation scheme is unchanged, so serialized/replicated key identity
		// is preserved. Access is serialized by the mesh update lock, matching the rest of the update path.
		mutable TMap<int32, FRealtimeMeshSectionKey> PolyGroupSectionKeyCache;

		const FRealtimeMeshSectionKey& GetPolyGroupSectionKey(int32 PolyGroupIndex) const;

	public:
		FRealtimeMeshBufferSetSimple(const FRealtimeMeshContextRef& InContext, const FRealtimeMeshBufferSetKey& InKey)
			: FRealtimeMeshBufferSetManaged(InContext, InKey)
			, bAutoCreateSectionsForPolygonGroups(true)
		{
		}

		void SetShouldAutoCreateSectionsForPolyGroups(FRealtimeMeshUpdateContext& UpdateContext, bool bNewValue) { bAutoCreateSectionsForPolygonGroups = bNewValue; }
		bool ShouldAutoCreateSectionsForPolygonGroups(const FRealtimeMeshLockContext& LockContext) const { return bAutoCreateSectionsForPolygonGroups; }

		void SetPolyGroupSectionHandler(FRealtimeMeshUpdateContext& UpdateContext, const FRealtimeMeshPolyGroupConfigHandler& NewHandler);
		void ClearPolyGroupSectionHandler(FRealtimeMeshUpdateContext& UpdateContext);

		void ProcessMeshData(const FRealtimeMeshLockContext& LockContext, TFunctionRef<void(const FRealtimeMeshStreamSet&)> ProcessFunc) const;
		void EditMeshData(FRealtimeMeshUpdateContext& UpdateContext, TFunctionRef<TSet<FRealtimeMeshStreamKey>(FRealtimeMeshStreamSet&)> EditFunc);

		// Ranged variant of EditMeshData: EditFunc returns each changed stream mapped to the
		// element range it changed (half-open [lower, upper)), so the fast path uploads only
		// that sub-region of the GPU buffer. Eligible vertex streams on a Dynamic buffer set
		// become in-place ranged GPU updates; anything else falls back to a full update.
		void EditMeshDataRanged(FRealtimeMeshUpdateContext& UpdateContext, TFunctionRef<TMap<FRealtimeMeshStreamKey, FInt32Range>(FRealtimeMeshStreamSet&)> EditFunc);

		virtual void CreateOrUpdateStream(FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshStream&& Stream) override;

		using FRealtimeMeshBufferSetManaged::SetAllStreams;
		virtual void SetAllStreams(FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshStreamSet&& InStreams) override;

	protected:

		virtual void UpdatePolyGroupSections(FRealtimeMeshUpdateContext& UpdateContext, bool bUpdateDepthOnly);
		virtual FRealtimeMeshSectionConfig DefaultPolyGroupSectionHandler(int32 PolyGroupIndex) const;

		bool ShouldCreateSingularSection() const;
	};

	class REALTIMEMESHCOMPONENT_API FRealtimeMeshSimple : public FRealtimeMeshManaged
	{
	public:
		FRealtimeMeshSimple(const FRealtimeMeshContextRef& InContext)
			: FRealtimeMeshManaged(InContext)
		{
		}

		TFuture<ERealtimeMeshProxyUpdateStatus> CreateSectionGroup(const FRealtimeMeshBufferSetKey& SectionGroupKey, const FRealtimeMeshBufferSetConfig& InConfig = FRealtimeMeshBufferSetConfig(), bool bShouldAutoCreateSectionsForPolyGroups = true);
		TFuture<ERealtimeMeshProxyUpdateStatus> CreateSectionGroup(const FRealtimeMeshBufferSetKey& SectionGroupKey, FRealtimeMeshStreamSet&& MeshData, const FRealtimeMeshBufferSetConfig& InConfig = FRealtimeMeshBufferSetConfig(), bool bShouldAutoCreateSectionsForPolyGroups = true);
		TFuture<ERealtimeMeshProxyUpdateStatus> CreateSectionGroup(const FRealtimeMeshBufferSetKey& SectionGroupKey, const FRealtimeMeshStreamSet& MeshData, const FRealtimeMeshBufferSetConfig& InConfig = FRealtimeMeshBufferSetConfig(), bool bShouldAutoCreateSectionsForPolyGroups = true);
		TFuture<ERealtimeMeshProxyUpdateStatus> UpdateSectionGroup(const FRealtimeMeshBufferSetKey& SectionGroupKey, FRealtimeMeshStreamSet&& MeshData);
		TFuture<ERealtimeMeshProxyUpdateStatus> UpdateSectionGroup(const FRealtimeMeshBufferSetKey& SectionGroupKey, const FRealtimeMeshStreamSet& MeshData);

		// Override Managed's CreateSectionGroup factory to return the
		// Simple-flavored buffer set (StreamSet + PolyGroup auto-section hooks).
		virtual FRealtimeMeshSectionGroupRef CreateSectionGroup(const FRealtimeMeshBufferSetKey& InKey) const override
		{
			return MakeShared<FRealtimeMeshBufferSetSimple>(Context, InKey);
		}

		friend class ::URealtimeMeshSimple;
	};
}


// Back-compat aliases for code that referenced the pre-Managed delegate names.
// Blueprint asset graphs that hard-code these names may need to be recompiled
// against the Managed-tier delegate types.
using FRealtimeMeshSimpleCompletionCallback = FRealtimeMeshManagedCompletionCallback;
using FRealtimeMeshSimpleCollisionCompletionCallback = FRealtimeMeshManagedCollisionCompletionCallback;


UCLASS(Blueprintable)
class REALTIMEMESHCOMPONENT_API URealtimeMeshSimple : public URealtimeMeshManaged
{
	GENERATED_UCLASS_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh")
	static URealtimeMeshSimple* InitializeRealtimeMeshSimple(URealtimeMeshComponent* Owner);

	TSharedRef<RealtimeMesh::FRealtimeMeshSimple> GetMeshData() const { return StaticCastSharedRef<RealtimeMesh::FRealtimeMeshSimple>(GetMesh()); }

	TFuture<ERealtimeMeshProxyUpdateStatus> CreateBufferSet(const FRealtimeMeshBufferSetKey& BufferSetKey, const FRealtimeMeshBufferSetConfig& InConfig = FRealtimeMeshBufferSetConfig(), bool bShouldAutoCreateSectionsForPolyGroups = true);

	TFuture<ERealtimeMeshProxyUpdateStatus> CreateBufferSet(const FRealtimeMeshBufferSetKey& BufferSetKey, RealtimeMesh::FRealtimeMeshStreamSet&& MeshData, const FRealtimeMeshBufferSetConfig& InConfig = FRealtimeMeshBufferSetConfig(), bool bShouldAutoCreateSectionsForPolyGroups = true);
	TFuture<ERealtimeMeshProxyUpdateStatus> CreateBufferSet(const FRealtimeMeshBufferSetKey& BufferSetKey, const RealtimeMesh::FRealtimeMeshStreamSet& MeshData, const FRealtimeMeshBufferSetConfig& InConfig = FRealtimeMeshBufferSetConfig(), bool bShouldAutoCreateSectionsForPolyGroups = true);

	TFuture<ERealtimeMeshProxyUpdateStatus> UpdateBufferSet(const FRealtimeMeshBufferSetKey& BufferSetKey, RealtimeMesh::FRealtimeMeshStreamSet&& MeshData);
	TFuture<ERealtimeMeshProxyUpdateStatus> UpdateBufferSet(const FRealtimeMeshBufferSetKey& BufferSetKey, const RealtimeMesh::FRealtimeMeshStreamSet& MeshData);

	// --- Deprecated SectionGroup-terminology aliases (use the *BufferSet forms) ---
	UE_DEPRECATED(5.7, "Renamed to CreateBufferSet to match the BufferSet terminology")
	TFuture<ERealtimeMeshProxyUpdateStatus> CreateSectionGroup(const FRealtimeMeshBufferSetKey& SectionGroupKey, const FRealtimeMeshBufferSetConfig& InConfig = FRealtimeMeshBufferSetConfig(), bool bShouldAutoCreateSectionsForPolyGroups = true) { return CreateBufferSet(SectionGroupKey, InConfig, bShouldAutoCreateSectionsForPolyGroups); }
	UE_DEPRECATED(5.7, "Renamed to CreateBufferSet to match the BufferSet terminology")
	TFuture<ERealtimeMeshProxyUpdateStatus> CreateSectionGroup(const FRealtimeMeshBufferSetKey& SectionGroupKey, RealtimeMesh::FRealtimeMeshStreamSet&& MeshData, const FRealtimeMeshBufferSetConfig& InConfig = FRealtimeMeshBufferSetConfig(), bool bShouldAutoCreateSectionsForPolyGroups = true) { return CreateBufferSet(SectionGroupKey, MoveTemp(MeshData), InConfig, bShouldAutoCreateSectionsForPolyGroups); }
	UE_DEPRECATED(5.7, "Renamed to CreateBufferSet to match the BufferSet terminology")
	TFuture<ERealtimeMeshProxyUpdateStatus> CreateSectionGroup(const FRealtimeMeshBufferSetKey& SectionGroupKey, const RealtimeMesh::FRealtimeMeshStreamSet& MeshData, const FRealtimeMeshBufferSetConfig& InConfig = FRealtimeMeshBufferSetConfig(), bool bShouldAutoCreateSectionsForPolyGroups = true) { return CreateBufferSet(SectionGroupKey, MeshData, InConfig, bShouldAutoCreateSectionsForPolyGroups); }
	UE_DEPRECATED(5.7, "Renamed to UpdateBufferSet to match the BufferSet terminology")
	TFuture<ERealtimeMeshProxyUpdateStatus> UpdateSectionGroup(const FRealtimeMeshBufferSetKey& SectionGroupKey, RealtimeMesh::FRealtimeMeshStreamSet&& MeshData) { return UpdateBufferSet(SectionGroupKey, MoveTemp(MeshData)); }
	UE_DEPRECATED(5.7, "Renamed to UpdateBufferSet to match the BufferSet terminology")
	TFuture<ERealtimeMeshProxyUpdateStatus> UpdateSectionGroup(const FRealtimeMeshBufferSetKey& SectionGroupKey, const RealtimeMesh::FRealtimeMeshStreamSet& MeshData) { return UpdateBufferSet(SectionGroupKey, MeshData); }

	TSharedPtr<RealtimeMesh::FRealtimeMeshBufferSetSimple> GetBufferSet(const FRealtimeMeshBufferSetKey& BufferSetKey) const;

	UE_DEPRECATED(5.7, "Renamed to GetBufferSet to match the BufferSet terminology")
	TSharedPtr<RealtimeMesh::FRealtimeMeshBufferSetSimple> GetSectionGroup(const FRealtimeMeshBufferSetKey& SectionGroupKey) const { return GetBufferSet(SectionGroupKey); }

	// Lock-safe alternative to GetSectionGroup(): invokes ProcessFunc with the section group's buffer set
	// while the accessor lock is still held, so the reference never escapes the lock scope. If the requested
	// section group does not exist, ProcessFunc is simply not called. (Not a UFUNCTION — takes a TFunctionRef.)
	void ProcessBufferSet(const FRealtimeMeshBufferSetKey& BufferSetKey, TFunctionRef<void(const RealtimeMesh::FRealtimeMeshBufferSetSimple&)> ProcessFunc) const;

	UE_DEPRECATED(5.7, "Renamed to ProcessBufferSet to match the BufferSet terminology")
	void ProcessSectionGroup(const FRealtimeMeshBufferSetKey& SectionGroupKey, TFunctionRef<void(const RealtimeMesh::FRealtimeMeshBufferSetSimple&)> ProcessFunc) const { ProcessBufferSet(SectionGroupKey, ProcessFunc); }

	void ProcessMesh(const FRealtimeMeshBufferSetKey& SectionGroupKey, const TFunctionRef<void(const RealtimeMesh::FRealtimeMeshStreamSet&)>& ProcessFunc) const;
	TFuture<ERealtimeMeshProxyUpdateStatus> EditMeshInPlace(const FRealtimeMeshBufferSetKey& SectionGroupKey, const TFunctionRef<TSet<FRealtimeMeshStreamKey>(RealtimeMesh::FRealtimeMeshStreamSet&)>& EditFunc);

	// Ranged in-place edit: EditFunc returns each changed stream mapped to the element
	// range it touched (half-open [lower, upper)). For a Dynamic section group this uploads
	// only that sub-region of each eligible vertex stream's GPU buffer (no realloc, no new
	// proxy version). Streams/ranges that aren't eligible fall back to a full update.
	TFuture<ERealtimeMeshProxyUpdateStatus> EditMeshInPlaceRanged(const FRealtimeMeshBufferSetKey& SectionGroupKey, const TFunctionRef<TMap<FRealtimeMeshStreamKey, FInt32Range>(RealtimeMesh::FRealtimeMeshStreamSet&)>& EditFunc);


	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh", DisplayName="CreateBufferSet", meta=(AutoCreateRefTerm="OnComplete"))
	void CreateBufferSet(const FRealtimeMeshBufferSetKey& BufferSetKey, URealtimeMeshStreamSet* MeshData, const FRealtimeMeshManagedCompletionCallback& OnComplete);

	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh", DisplayName="CreateBufferSetUnique", meta=(AutoCreateRefTerm="OnComplete"))
	FRealtimeMeshBufferSetKey CreateBufferSetUnique(const FRealtimeMeshLODKey& LODKey, URealtimeMeshStreamSet* MeshData, const FRealtimeMeshManagedCompletionCallback& OnComplete);

	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh", DisplayName="UpdateBufferSet", meta=(AutoCreateRefTerm="OnComplete"))
	void UpdateBufferSet(const FRealtimeMeshBufferSetKey& BufferSetKey, URealtimeMeshStreamSet* MeshData, const FRealtimeMeshManagedCompletionCallback& OnComplete);

	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh", DisplayName="CreateSectionGroup", meta=(AutoCreateRefTerm="OnComplete", DeprecatedFunction, DeprecationMessage="Use CreateBufferSet"))
	void CreateSectionGroup(const FRealtimeMeshBufferSetKey& SectionGroupKey, URealtimeMeshStreamSet* MeshData, const FRealtimeMeshManagedCompletionCallback& OnComplete);

	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh", DisplayName="CreateSectionGroupUnique", meta=(AutoCreateRefTerm="OnComplete", DeprecatedFunction, DeprecationMessage="Use CreateBufferSetUnique"))
	FRealtimeMeshBufferSetKey CreateSectionGroupUnique(const FRealtimeMeshLODKey& LODKey, URealtimeMeshStreamSet* MeshData, const FRealtimeMeshManagedCompletionCallback& OnComplete);

	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh", DisplayName="UpdateSectionGroup", meta=(AutoCreateRefTerm="OnComplete", DeprecatedFunction, DeprecationMessage="Use UpdateBufferSet"))
	void UpdateSectionGroup(const FRealtimeMeshBufferSetKey& SectionGroupKey, URealtimeMeshStreamSet* MeshData, const FRealtimeMeshManagedCompletionCallback& OnComplete);

	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh")
	void SetShouldAutoCreateSectionsForPolyGroups(const FRealtimeMeshBufferSetKey& SectionGroupKey, bool bNewValue);

	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh")
	bool ShouldAutoCreateSectionsForPolygonGroups(const FRealtimeMeshBufferSetKey& SectionGroupKey) const;
};
