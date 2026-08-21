// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RealtimeMesh.h"
#include "Data/RealtimeMeshLOD.h"
#include "Data/RealtimeMeshSection.h"
#include "Data/RealtimeMeshBufferSet.h"
#include "Interface_CollisionDataProviderCore.h"
#include "Core/RealtimeMeshBuilder.h"
#include "Core/RealtimeMeshDataStream.h"
#include "Mesh/RealtimeMeshDistanceField.h"
#include "Mesh/RealtimeMeshCardRepresentation.h"
#include "RealtimeMeshManaged.generated.h"


class URealtimeMeshManaged;


namespace RealtimeMesh
{
	/**
	 * Section base for "managed" meshes (Simple, Procedural). Carries the per-section
	 * complex-collision-enabled flag and the dirty-on-change collision plumbing
	 * that both leaves need; leaves still override FinalizeUpdate to compute bounds
	 * from their own vertex storage.
	 */
	class REALTIMEMESHCOMPONENT_API FRealtimeMeshSectionManaged : public FRealtimeMeshSection
	{
	protected:
		bool bShouldCreateMeshCollision;

	public:
		FRealtimeMeshSectionManaged(const FRealtimeMeshContextRef& InContext, const FRealtimeMeshSectionKey& InKey);
		virtual ~FRealtimeMeshSectionManaged() override = default;

		bool HasCollision(const FRealtimeMeshLockContext& LockContext) const { return bShouldCreateMeshCollision; }
		void SetShouldCreateCollision(FRealtimeMeshUpdateContext& UpdateContext, bool bNewShouldCreateMeshCollision);

		virtual bool Serialize(FArchive& Ar) override;
		virtual void Reset(FRealtimeMeshUpdateContext& UpdateContext) override;

	protected:
		virtual void FinalizeUpdate(FRealtimeMeshUpdateContext& UpdateContext) override;
		void MarkCollisionDirty(FRealtimeMeshUpdateContext& UpdateContext) const;
	};


	/**
	 * BufferSet base for managed meshes. Owns a CPU-side FRealtimeMeshStreamSet
	 * and all the generic plumbing around it: GPU upload on InitializeProxy,
	 * Reset, Serialize, stream lookup, stream CRUD, complex-collision walk over
	 * each section's contribution. Leaves add their own flavor on top:
	 *   - FRealtimeMeshBufferSetSimple adds the PolyGroup auto-section hooks.
	 *   - Procedural has no additions and uses this base directly.
	 */
	class REALTIMEMESHCOMPONENT_API FRealtimeMeshBufferSetManaged : public FRealtimeMeshBufferSet
	{
	protected:
		FRealtimeMeshStreamSet Streams;

	public:
		FRealtimeMeshBufferSetManaged(const FRealtimeMeshContextRef& InContext, const FRealtimeMeshBufferSetKey& InKey)
			: FRealtimeMeshBufferSet(InContext, InKey)
		{
		}

		FRealtimeMeshStreamRange GetValidStreamRange(const FRealtimeMeshLockContext& LockContext) const;
		virtual const FRealtimeMeshStream* GetStream(const FRealtimeMeshLockContext& LockContext, FRealtimeMeshStreamKey StreamKey) const;

		/**
		 * Deep-copies the authoritative CPU stream set (optionally only DesiredStreams) into
		 * OutStreams while the caller holds the mesh lock. This is the safe capture pattern for
		 * off-thread consumers (serialization, net sync): copy under the guard, process the copy
		 * outside it.
		 */
		void CopyStreams(const FRealtimeMeshLockContext& LockContext, FRealtimeMeshStreamSet& OutStreams,
						 const TSet<FRealtimeMeshStreamKey>& DesiredStreams = TSet<FRealtimeMeshStreamKey>()) const
		{
			OutStreams.CopyFrom(Streams, false, DesiredStreams);
		}

		virtual void CreateOrUpdateStream(FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshStream&& Stream) override;
		virtual void RemoveStream(FRealtimeMeshUpdateContext& UpdateContext, const FRealtimeMeshStreamKey& StreamKey) override;

		/**
		 * Managed-tier counterpart of FRealtimeMeshBufferSet::FastUpdateStream: first refreshes the
		 * authoritative CPU copy in Streams (which GetStream, collision extraction, serialization and
		 * proxy re-initialization all read), then queues the in-place GPU update. Callers that have
		 * already mutated Streams in place (like Simple's EditMeshData) should call the base directly.
		 * Hides (does not override) the non-virtual base method.
		 */
		void FastUpdateStream(FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshStream&& Stream, const FInt32Range& ElementRange);

		using FRealtimeMeshBufferSet::SetAllStreams;

		virtual void InitializeProxy(FRealtimeMeshUpdateContext& UpdateContext) override;
		virtual void Reset(FRealtimeMeshUpdateContext& UpdateContext) override;
		virtual bool Serialize(FArchive& Ar) override;

		virtual bool GenerateComplexCollision(const FRealtimeMeshLockContext& LockContext, FRealtimeMeshCollisionMesh& CollisionMesh) const;
	};


	/**
	 * LOD base for managed meshes. Implements the per-LOD complex-collision walk
	 * that gathers each section group's contribution via the BufferSetManaged
	 * virtual.
	 */
	class REALTIMEMESHCOMPONENT_API FRealtimeMeshLODManaged : public FRealtimeMeshLOD
	{
	public:
		FRealtimeMeshLODManaged(const FRealtimeMeshContextRef& InContext, const FRealtimeMeshLODKey& InKey)
			: FRealtimeMeshLOD(InContext, InKey)
		{
		}

		virtual bool GenerateComplexCollision(const FRealtimeMeshLockContext& LockContext, FRealtimeMeshComplexGeometry& ComplexGeometry) const;
	};


	DECLARE_MULTICAST_DELEGATE(FRealtimeMeshManagedCollisionDataChangedEvent);

	struct FRealtimeMeshManagedCollisionGroupDirtySet
	{
	private:
		TSet<FRealtimeMeshBufferSetKey> DirtyCollisionSectionGroups;
	public:
		void Flag(const FRealtimeMeshBufferSetKey& SectionGroup)
		{
			DirtyCollisionSectionGroups.Add(SectionGroup);
		}

		bool IsDirty(const FRealtimeMeshBufferSetKey& SectionGroup) const
		{
			return DirtyCollisionSectionGroups.Contains(SectionGroup);
		}

		bool HasAnyDirty() const
		{
			return !DirtyCollisionSectionGroups.IsEmpty();
		}
	};

	struct FRealtimeMeshManagedUpdateState : FRealtimeMeshUpdateState
	{
		FRealtimeMeshManagedCollisionGroupDirtySet CollisionGroupDirtySet;
	};


	/**
	 * Data-layer base for managed meshes. Owns collision, Nanite, distance-field,
	 * and Lumen card storage plus the collision-dirty plumbing — everything that's
	 * shared by Simple's StreamSet-flavored API and Procedural's PMC-parity API.
	 * Leaves provide the section/buffer-set storage representation.
	 */
	class REALTIMEMESHCOMPONENT_API FRealtimeMeshManaged : public FRealtimeMesh
	{
	protected:
		FRealtimeMeshCollisionConfiguration CollisionConfig;
		FRealtimeMeshSimpleGeometry SimpleGeometry;
		FRealtimeMeshComplexGeometry ComplexGeometry;

		mutable TSharedPtr<TPromise<ERealtimeMeshCollisionUpdateResult>> PendingCollisionPromise;

		// Nanite resource-sharing: the single live, InitResources'd instance for this mesh. Held by
		// TSharedPtr so every published proxy version pins the same GPU registration by refcount bump
		// (no per-recreate deep clone; streamed-in detail pages survive proxy recreates). The custom
		// deleter (ReleaseResources + RT-deferred delete) fires when the last reference drops.
		TSharedPtr<FRealtimeMeshNaniteResources> NaniteResources;
		// API-L8: DistanceField / CardRepresentation are held as ref-counted immutable
		// snapshots (copy-on-write). The setters allocate a fresh snapshot and swap it
		// in; the game thread never mutates a live snapshot, so a proxy task that
		// captured an older snapshot stays valid until the proxy is recreated. Proxy
		// recreates therefore just bump the refcount instead of deep-copying the payload.
		TSharedRef<const FRealtimeMeshDistanceField> DistanceField;
		TSharedPtr<const FRealtimeMeshCardRepresentation> CardRepresentation;

	public:
		FRealtimeMeshManaged(const FRealtimeMeshContextRef& InContext)
			: FRealtimeMesh(InContext)
			, NaniteResources()
			, DistanceField(MakeShared<const FRealtimeMeshDistanceField>())
		{
		}

		// Defined out-of-line (RealtimeMeshManaged.cpp) so the last-ref drop can route
		// PendingCollisionPromise fulfilment onto the game thread (API-L7).
		virtual ~FRealtimeMeshManaged() override;

		FRealtimeMeshCollisionConfiguration GetCollisionConfig() const;
		TFuture<ERealtimeMeshCollisionUpdateResult> SetCollisionConfig(const FRealtimeMeshCollisionConfiguration& InCollisionConfig);
		FRealtimeMeshSimpleGeometry GetSimpleGeometry() const;
		TFuture<ERealtimeMeshCollisionUpdateResult> SetSimpleGeometry(const FRealtimeMeshSimpleGeometry& InSimpleGeometry);

		bool HasCustomComplexMeshGeometry() const { return ComplexGeometry.NumMeshes() > 0; }
		TFuture<ERealtimeMeshCollisionUpdateResult> ClearCustomComplexMeshGeometry();
		TFuture<ERealtimeMeshCollisionUpdateResult> SetCustomComplexMeshGeometry(FRealtimeMeshComplexGeometry&& InComplexMeshGeometry);
		TFuture<ERealtimeMeshCollisionUpdateResult> SetCustomComplexMeshGeometry(const FRealtimeMeshComplexGeometry& InComplexMeshGeometry);
		void ProcessCustomComplexMeshGeometry(TFunctionRef<void(const FRealtimeMeshComplexGeometry&)> ProcessFunc) const;
		TFuture<ERealtimeMeshCollisionUpdateResult> EditCustomComplexMeshGeometry(TFunctionRef<void(FRealtimeMeshComplexGeometry&)> EditFunc);

		bool HasNaniteResources(const FRealtimeMeshLockContext& LockContext) const;
		const FRealtimeMeshNaniteResources* GetNaniteResources(const FRealtimeMeshLockContext& LockContext) const { return NaniteResources.Get(); }
		void SetNaniteResources(FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshNaniteResourcesPtr&& InNaniteResources);
		virtual void ClearNaniteResources(FRealtimeMeshUpdateContext& UpdateContext) override;

		// API-L8: read-lock-held callback accessor replacing the old GetDistanceField()
		// that leaked a reference past its read guard (TOCTOU). Mirrors the sibling
		// ProcessCustomComplexMeshGeometry pattern (lock taken internally).
		// ProcessFunc MUST NOT retain the reference beyond the callback (it is only valid
		// for the duration of the call, under the held read lock), and MUST NOT call any
		// write/edit API on this mesh from inside the callback: a read->write lock upgrade
		// while the read lock is held is a hard check() in the lock guard.
		void ProcessDistanceField(TFunctionRef<void(const FRealtimeMeshDistanceField&)> ProcessFunc) const;
		virtual void SetDistanceField(FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshDistanceField&& InDistanceField) override;
		virtual void ClearDistanceField(FRealtimeMeshUpdateContext& UpdateContext) override;
		const FRealtimeMeshCardRepresentation* GetCardRepresentation(const FRealtimeMeshLockContext& LockContext) const;

		virtual void SetCardRepresentation(FRealtimeMeshUpdateContext& UpdateContext, FRealtimeMeshCardRepresentation&& InCardRepresentation) override;
		virtual void ClearCardRepresentation(FRealtimeMeshUpdateContext& UpdateContext) override;

		virtual bool GenerateComplexCollision(const FRealtimeMeshLockContext& LockContext, FRealtimeMeshComplexGeometry& OutComplexGeometry) const;

		virtual void InitializeProxy(FRealtimeMeshUpdateContext& UpdateContext) const override;

		virtual void ResetInternal(FRealtimeMeshUpdateContext& UpdateContext, bool bRemoveRenderProxy) override;

		virtual void FinalizeUpdate(FRealtimeMeshUpdateContext& UpdateContext) override;

		virtual bool Serialize(FArchive& Ar, URealtimeMesh* Owner) override;

		// Type-tier factory overrides. The Managed tier uses its own Section /
		// BufferSet / LOD subclasses and its own UpdateState.
		virtual FRealtimeMeshSectionRef     CreateSection(const FRealtimeMeshSectionKey& InKey) const override;
		virtual FRealtimeMeshSectionGroupRef CreateSectionGroup(const FRealtimeMeshBufferSetKey& InKey) const override;
		virtual FRealtimeMeshLODRef         CreateLOD(const FRealtimeMeshLODKey& InKey) const override;
		virtual FRealtimeMeshUpdateStateRef CreateUpdateState() const override;

	protected:
		void MarkCollisionDirtyNoCallback() const;
		TFuture<ERealtimeMeshCollisionUpdateResult> MarkCollisionDirty() const;

		virtual void ProcessEndOfFrameUpdates() override;

		friend class ::URealtimeMeshManaged;
	};
}


DECLARE_DYNAMIC_DELEGATE_OneParam(FRealtimeMeshManagedCompletionCallback, ERealtimeMeshProxyUpdateStatus, ProxyUpdateResult);
DECLARE_DYNAMIC_DELEGATE_OneParam(FRealtimeMeshManagedCollisionCompletionCallback, ERealtimeMeshCollisionUpdateResult, CollisionResult);


/**
 * UObject-layer base for managed RMC subclasses (Simple, Procedural). Lifts the
 * "we own the data lifecycle for you" Blueprint surface: collision,
 * Nanite/DistanceField/CardRepresentation, section-state wrappers, section/group
 * removal. Concrete leaves add their own geometry-population API on top.
 */
UCLASS(Abstract)
class REALTIMEMESHCOMPONENT_API URealtimeMeshManaged : public URealtimeMesh
{
	GENERATED_UCLASS_BODY()

protected:
	/* Should the underlying FRealtimeMesh data be written into this object's
	 * asset bytes when serialized? Only Managed-tier meshes (Simple, Procedural)
	 * have asset-stored data to gate; non-Managed leaves (Constructed/Factory)
	 * generate their data at runtime and don't need this. */
	uint32 bShouldSerializeMeshData : 1;

public:
	TSharedRef<RealtimeMesh::FRealtimeMeshManaged> GetManagedMeshData() const { return StaticCastSharedRef<RealtimeMesh::FRealtimeMeshManaged>(GetMesh()); }

	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh")
	bool ShouldSerializeMeshData() const { return bShouldSerializeMeshData; }

	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh")
	void SetShouldSerializeMeshData(bool bNewShouldSerializeMeshData) { bShouldSerializeMeshData = bNewShouldSerializeMeshData; }

	virtual void Serialize(FArchive& Ar) override;

	TFuture<ERealtimeMeshProxyUpdateStatus> CreateSection(const FRealtimeMeshSectionKey& SectionKey, const FRealtimeMeshSectionConfig& Config,
															const FRealtimeMeshStreamRange& StreamRange, bool bShouldCreateCollision = false);

	TFuture<ERealtimeMeshProxyUpdateStatus> UpdateSectionConfig(const FRealtimeMeshSectionKey& SectionKey, const FRealtimeMeshSectionConfig& Config, bool bShouldCreateCollision = false);
	TFuture<ERealtimeMeshProxyUpdateStatus> UpdateSectionRange(const FRealtimeMeshSectionKey& SectionKey, const FRealtimeMeshStreamRange& StreamRange);


	TArray<FRealtimeMeshLODKey> GetLODs() const;

	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh")
	TArray<FRealtimeMeshBufferSetKey> GetBufferSets(const FRealtimeMeshLODKey& LODKey) const;

	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh", meta = (DeprecatedFunction, DeprecationMessage = "Use GetBufferSets"))
	TArray<FRealtimeMeshBufferSetKey> GetSectionGroups(const FRealtimeMeshLODKey& LODKey) const;


	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh", DisplayName="CreateSection", meta = (AutoCreateRefTerm = "Config, StreamRange, OnComplete"))
	void CreateSection(const FRealtimeMeshSectionKey& SectionKey, const FRealtimeMeshSectionConfig& Config,
								 const FRealtimeMeshStreamRange& StreamRange, bool bShouldCreateCollision, const FRealtimeMeshManagedCompletionCallback& OnComplete);

	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh", DisplayName="UpdateSectionConfig", meta = (AutoCreateRefTerm = "SectionKey, bShouldCreateCollision, OnComplete"))
	void UpdateSectionConfig(const FRealtimeMeshSectionKey& SectionKey, const FRealtimeMeshSectionConfig& Config, bool bShouldCreateCollision,
									   const FRealtimeMeshManagedCompletionCallback& OnComplete);

	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh")
	TArray<FRealtimeMeshSectionKey> GetSectionsInBufferSet(const FRealtimeMeshBufferSetKey& BufferSetKey);

	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh", meta = (DeprecatedFunction, DeprecationMessage = "Use GetSectionsInBufferSet"))
	TArray<FRealtimeMeshSectionKey> GetSectionsInGroup(const FRealtimeMeshBufferSetKey& SectionGroupKey);


	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh", DisplayName="RemoveSection", meta = (AutoCreateRefTerm = "SectionKey, OnComplete"))
	void RemoveSection(const FRealtimeMeshSectionKey& SectionKey, const FRealtimeMeshManagedCompletionCallback& OnComplete);

	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh", DisplayName="RemoveBufferSet", meta = (AutoCreateRefTerm = "BufferSetKey, OnComplete"))
	void RemoveBufferSet(const FRealtimeMeshBufferSetKey& BufferSetKey, const FRealtimeMeshManagedCompletionCallback& OnComplete);

	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh", DisplayName="RemoveSectionGroup", meta = (AutoCreateRefTerm = "SectionGroupKey, OnComplete", DeprecatedFunction, DeprecationMessage = "Use RemoveBufferSet"))
	void RemoveSectionGroup(const FRealtimeMeshBufferSetKey& SectionGroupKey, const FRealtimeMeshManagedCompletionCallback& OnComplete);


	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh", meta = (AutoCreateRefTerm = "SectionKey"))
	FRealtimeMeshSectionConfig GetSectionConfig(const FRealtimeMeshSectionKey& SectionKey) const;

	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh", meta = (AutoCreateRefTerm = "SectionKey"))
	bool IsSectionVisible(const FRealtimeMeshSectionKey& SectionKey) const;

	TFuture<ERealtimeMeshProxyUpdateStatus> SetSectionVisibility(const FRealtimeMeshSectionKey& SectionKey, bool bIsVisible);

	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh", DisplayName="SetSectionVisibility", meta = (AutoCreateRefTerm = "SectionKey, OnComplete"))
	void SetSectionVisibility(const FRealtimeMeshSectionKey& SectionKey, bool bIsVisible, const FRealtimeMeshManagedCompletionCallback& OnComplete);

	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh", meta = (AutoCreateRefTerm = "SectionKey"))
	bool IsSectionCastingShadow(const FRealtimeMeshSectionKey& SectionKey) const;

	TFuture<ERealtimeMeshProxyUpdateStatus> SetSectionCastShadow(const FRealtimeMeshSectionKey& SectionKey, bool bCastShadow);

	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh", DisplayName="SetSectionCastShadow", meta = (AutoCreateRefTerm = "SectionKey, OnComplete"))
	void SetSectionCastShadow(const FRealtimeMeshSectionKey& SectionKey, bool bCastShadow, const FRealtimeMeshManagedCompletionCallback& OnComplete);

	TFuture<ERealtimeMeshProxyUpdateStatus> RemoveSection(const FRealtimeMeshSectionKey& SectionKey);

	TFuture<ERealtimeMeshProxyUpdateStatus> RemoveBufferSet(const FRealtimeMeshBufferSetKey& BufferSetKey);

	UE_DEPRECATED(5.7, "Renamed to RemoveBufferSet to match the BufferSet terminology")
	TFuture<ERealtimeMeshProxyUpdateStatus> RemoveSectionGroup(const FRealtimeMeshBufferSetKey& SectionGroupKey) { return RemoveBufferSet(SectionGroupKey); }


	// API-L8: callback accessor invoked while the mesh read lock is held. Replaces the
	// former GetDistanceField() that returned a reference whose read guard was already
	// released (TOCTOU). Not BlueprintCallable, so it can safely be a callback.
	// ProcessFunc MUST NOT retain the reference beyond the callback (it is only valid for
	// the duration of the call, under the held read lock), and MUST NOT call any write/edit
	// API on this mesh from inside the callback: a read->write lock upgrade while the read
	// lock is held is a hard check() in the lock guard.
	void ProcessDistanceField(TFunctionRef<void(const FRealtimeMeshDistanceField&)> ProcessFunc) const;

	TFuture<ERealtimeMeshProxyUpdateStatus> SetDistanceField(FRealtimeMeshDistanceField&& InDistanceField);

	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh", meta = (AutoCreateRefTerm = "OnComplete"))
	void SetDistanceField(const FRealtimeMeshDistanceField& DistanceField, const FRealtimeMeshManagedCompletionCallback& OnComplete);

	TFuture<ERealtimeMeshProxyUpdateStatus> ClearDistanceField();

	const FRealtimeMeshCardRepresentation* GetCardRepresentation(const RealtimeMesh::FRealtimeMeshLockContext& LockContext) const;

	TFuture<ERealtimeMeshProxyUpdateStatus> SetCardRepresentation(FRealtimeMeshCardRepresentation&& InCardRepresentation);

	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh", meta = (AutoCreateRefTerm = "OnComplete"))
	void SetCardRepresentation(const FRealtimeMeshCardRepresentation& CardRepresentation, const FRealtimeMeshManagedCompletionCallback& OnComplete);

	TFuture<ERealtimeMeshProxyUpdateStatus> ClearCardRepresentation();

	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh")
	bool HasCustomComplexMeshGeometry() const;

	TFuture<ERealtimeMeshCollisionUpdateResult> ClearCustomComplexMeshGeometry();
	TFuture<ERealtimeMeshCollisionUpdateResult> SetCustomComplexMeshGeometry(FRealtimeMeshComplexGeometry&& InComplexMeshGeometry);
	TFuture<ERealtimeMeshCollisionUpdateResult> SetCustomComplexMeshGeometry(const FRealtimeMeshComplexGeometry& InComplexMeshGeometry);
	void ProcessCustomComplexMeshGeometry(TFunctionRef<void(const FRealtimeMeshComplexGeometry&)> ProcessFunc) const;
	TFuture<ERealtimeMeshCollisionUpdateResult> EditCustomComplexMeshGeometry(TFunctionRef<void(FRealtimeMeshComplexGeometry&)> EditFunc);

	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh")
	FRealtimeMeshCollisionConfiguration GetCollisionConfig() const;

	TFuture<ERealtimeMeshCollisionUpdateResult> SetCollisionConfig(const FRealtimeMeshCollisionConfiguration& InCollisionConfig);

	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh", DisplayName="SetCollisionConfig", meta = (AutoCreateRefTerm = "OnComplete"))
	void SetCollisionConfig(const FRealtimeMeshCollisionConfiguration& InCollisionConfig, const FRealtimeMeshManagedCollisionCompletionCallback& OnComplete);

	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh")
	FRealtimeMeshSimpleGeometry GetSimpleGeometry() const;

	TFuture<ERealtimeMeshCollisionUpdateResult> SetSimpleGeometry(const FRealtimeMeshSimpleGeometry& InSimpleGeometry);

	UFUNCTION(BlueprintCallable, Category = "Components|RealtimeMesh", DisplayName="SetSimpleGeometry", meta = (AutoCreateRefTerm = "OnComplete"))
	void SetSimpleGeometry(const FRealtimeMeshSimpleGeometry& InSimpleGeometry, const FRealtimeMeshManagedCollisionCompletionCallback& OnComplete);

	virtual void PostDuplicate(bool bDuplicateForPIE) override;
	virtual void PostLoad() override;
};
