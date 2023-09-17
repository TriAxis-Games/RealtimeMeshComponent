// Copyright TriAxis Games, L.L.C. All Rights Reserved.

// ReSharper disable CppMemberFunctionMayBeConst
#pragma once

#include "RealtimeMeshCollision.h"
#include "RealtimeMeshConfig.h"
#include "RealtimeMeshDataStream.h"


namespace RealtimeMesh
{
	enum class ERealtimeMeshUpdateType
	{
		Update,
		Replace
	};


	/*
	struct FRealtimeMeshSectionUpdateContext
	{
	private:
		TOptional<FRealtimeMeshSectionConfig> Config;
		TOptional<FRealtimeMeshStreamRange> StreamRange;
		TOptional<FBoxSphereBounds3f> LocalBounds;

	public:
		FRealtimeMeshSectionUpdateContext() { }

		bool HasUpdates() const { return HasConfigUpdate() || HasStreamRangeUpdate() || HasLocalBoundsUpdate(); }
		bool HasConfigUpdate() const { return Config.IsSet(); }
		bool HasStreamRangeUpdate() const { return StreamRange.IsSet(); }
		bool HasLocalBoundsUpdate() const { return LocalBounds.IsSet(); }

		const FRealtimeMeshSectionConfig& GetConfigUpdate() const { check(HasConfigUpdate()); return Config.GetValue(); }
		FRealtimeMeshSectionConfig& GetConfigUpdate() { check(HasConfigUpdate()); return Config.GetValue(); }
		const FRealtimeMeshStreamRange& GetStreamRangeUpdate() const { check(HasStreamRangeUpdate()); return StreamRange.GetValue(); }
		FRealtimeMeshStreamRange& GetStreamRangeUpdate() { check(HasStreamRangeUpdate()); return StreamRange.GetValue(); }
		const FBoxSphereBounds3f& GetLocalBoundsUpdate() const { check(HasLocalBoundsUpdate()); return LocalBounds.GetValue(); }
		FBoxSphereBounds3f& GetLocalBoundsUpdate() { check(HasLocalBoundsUpdate()); return LocalBounds.GetValue(); }

		FRealtimeMeshSectionConfig& AddConfigUpdate(const FRealtimeMeshSectionConfig& InConfig) { Config = InConfig; return Config.GetValue(); }
		FRealtimeMeshStreamRange& AddStreamRangeUpdate(const FRealtimeMeshStreamRange& InStreamRange) { StreamRange = InStreamRange; return StreamRange.GetValue(); }
		FBoxSphereBounds3f& AddLocalBoundsUpdate(const FBoxSphereBounds3f& InLocalBounds) { LocalBounds = InLocalBounds; return LocalBounds.GetValue(); }
	};

	struct FRealtimeMeshSectionGroupUpdateContext
	{
	private:
		TUniquePtr<FRealtimeMeshStreamSet> Streams;
		TSet<FRealtimeMeshStreamKey> StreamsToRemove;
		ERealtimeMeshUpdateType StreamsUpdateType;

		TMap<FRealtimeMeshSectionKey, FRealtimeMeshSectionUpdateContext> Sections;
		TSet<FRealtimeMeshSectionKey> SectionsToRemove;
		ERealtimeMeshUpdateType SectionsUpdateType;

	public:
		FRealtimeMeshSectionGroupUpdateContext()
			: StreamsUpdateType(ERealtimeMeshUpdateType::Update)
			, SectionsUpdateType(ERealtimeMeshUpdateType::Update)
		{ }

		bool ShouldReplaceExistingStreams() const { return  StreamsUpdateType == ERealtimeMeshUpdateType::Replace; }
		bool ShouldReplaceExistingSections() const { return SectionsUpdateType == ERealtimeMeshUpdateType::Replace; }

		bool HasStreamSet() const { return Streams.IsValid(); }
		const TSet<FRealtimeMeshStreamKey>& GetStreamsToRemove() const { return StreamsToRemove; }

		const FRealtimeMeshStreamSet& GetUpdatedStreams() const { return *Streams; }
		FRealtimeMeshStreamSet& GetUpdatedStreams() { return *Streams; }

		const TMap<FRealtimeMeshSectionKey, FRealtimeMeshSectionUpdateContext>& GetUpdatedSections() const { return Sections; }
		TMap<FRealtimeMeshSectionKey, FRealtimeMeshSectionUpdateContext>& GetUpdatedSections() { return Sections; }
		const TSet<FRealtimeMeshSectionKey>& GetSectionsToRemove() const { return SectionsToRemove; }
		
		
		void RemoveUntouchedStreams(bool bRemoveUntouched = true)
		{
			StreamsUpdateType = bRemoveUntouched? ERealtimeMeshUpdateType::Replace : ERealtimeMeshUpdateType::Update;
		}
		void RemoveStream(const FRealtimeMeshStreamKey& StreamKey) { StreamsToRemove.Add(StreamKey); }

		void RemoveUntouchedSections(bool bRemoveUntouched = true)
		{
			SectionsUpdateType = bRemoveUntouched ? ERealtimeMeshUpdateType::Replace : ERealtimeMeshUpdateType::Update;
		}
		void RemoveSection(const FRealtimeMeshSectionKey& SectionKey) { SectionsToRemove.Add(SectionKey); }


		void AddStreamUpdate(FRealtimeMeshDataStream&& Stream)
		{
			if (!Streams)
			{
				Streams = MakeUnique<FRealtimeMeshStreamSet>();
			}
			Streams->AddStream(MoveTemp(Stream));
		}


		FRealtimeMeshSectionUpdateContext& AddSectionUpdate(const FRealtimeMeshSectionKey& SectionKey, const FRealtimeMeshSectionUpdateContext& SectionUpdate)
		{
			Sections.Add(SectionKey, SectionUpdate);
			return Sections[SectionKey];
		}
		
		FRealtimeMeshSectionUpdateContext& AddSectionUpdate(const FRealtimeMeshSectionKey& SectionKey)
		{
			Sections.Add(SectionKey);
			return Sections[SectionKey];
		}	
	};

	struct FRealtimeMeshLODUpdateContext
	{
	private:
		TMap<FRealtimeMeshSectionGroupKey, FRealtimeMeshSectionGroupUpdateContext> SectionGroups;
		TSet<FRealtimeMeshSectionGroupKey> SectionGroupsToRemove;
		ERealtimeMeshUpdateType SectionGroupsUpdateType;
		TOptional<FRealtimeMeshLODConfig> Config;
	public:
		FRealtimeMeshLODUpdateContext()
			: SectionGroupsUpdateType(ERealtimeMeshUpdateType::Update)
		{ }

		bool ShouldReplaceExistingSectionGroups() const { return  SectionGroupsUpdateType == ERealtimeMeshUpdateType::Replace; }

		const TSet<FRealtimeMeshSectionGroupKey>& GetSectionGroupsToRemove() const { return SectionGroupsToRemove; }

		const TMap<FRealtimeMeshSectionGroupKey, FRealtimeMeshSectionGroupUpdateContext>& GetUpdatedSectionGroups() const { return SectionGroups; }
		TMap<FRealtimeMeshSectionGroupKey, FRealtimeMeshSectionGroupUpdateContext>& GetUpdatedSectionGroups() { return SectionGroups; }
		
		
		void RemoveUntouchedSectionGroups(bool bRemoveUntouched = true) { SectionGroupsUpdateType = bRemoveUntouched? ERealtimeMeshUpdateType::Replace : ERealtimeMeshUpdateType::Update; }
		void RemoveSectionGroup(const FRealtimeMeshSectionGroupKey& SectionGroupKey) { SectionGroupsToRemove.Add(SectionGroupKey); }

		FRealtimeMeshSectionGroupUpdateContext& AddSectionUpdate(const FRealtimeMeshSectionGroupKey& SectionGroupKey, const FRealtimeMeshSectionGroupUpdateContext& SectionGroupUpdate)
		{
			SectionGroups.Add(SectionGroupKey, SectionGroupUpdate);
			return SectionGroups[SectionGroupKey];
		}
		
		FRealtimeMeshSectionGroupUpdateContext& AddSectionUpdate(const FRealtimeMeshSectionGroupKey& SectionGroupKey)
		{
			SectionGroups.Add(SectionGroupKey);
			return SectionGroups[SectionGroupKey];
		}

		bool HasConfigUpdate() const { return Config.IsSet(); }
		const FRealtimeMeshLODConfig& GetConfigUpdate() const { check(HasConfigUpdate()); return Config.GetValue(); }
		FRealtimeMeshLODConfig& GetConfigUpdate() { check(HasConfigUpdate()); return Config.GetValue(); }
		FRealtimeMeshLODConfig& AddConfigUpdate(const FRealtimeMeshLODConfig& InConfig) { Config = InConfig; return Config.GetValue(); }
		
	};

	struct FRealtimeMeshUpdateContext
	{
	private:
		TSparseArray<FRealtimeMeshLODUpdateContext> LODs;
		int32 NewMaxLODs = INDEX_NONE;
		TOptional<FRealtimeMeshConfig> Config;
		TOptional<FRealtimeMeshCollisionConfiguration> CollisionConfig;
		TOptional<FRealtimeMeshSimpleGeometry> SimpleGeometry;
		TOptional<FRealtimeMeshTriMeshData> TriMeshData;
	public:

		bool HasNewMaxLODs() const { return NewMaxLODs != INDEX_NONE; }
		void SetNewMaxLODS(int32 InMaxLODS)
		{
			NewMaxLODs = InMaxLODS;
			while (LODs.GetMaxIndex() > NewMaxLODs - 1)
			{
				// TODO: Warn user?
				LODs.RemoveAt(LODs.GetMaxIndex());
			}
		}
		int32 GetNewMaxLODs() const { return NewMaxLODs; }

		bool HasLODUpdates() const { return LODs.Num() > 0; }
		const TSparseArray<FRealtimeMeshLODUpdateContext>& GetLODUpdates() const { return LODs; }
		TSparseArray<FRealtimeMeshLODUpdateContext>& GetLODUpdates() { return LODs; }
		bool HasLODUpdate(FRealtimeMeshLODKey LODKey) const { return LODs.IsValidIndex(LODKey); }
		const FRealtimeMeshLODUpdateContext& GetLODUpdate(FRealtimeMeshLODKey LODKey) const { return LODs[LODKey]; }
		FRealtimeMeshLODUpdateContext& GetLODUpdate(FRealtimeMeshLODKey LODKey) { return LODs[LODKey]; }
		
		

		bool HasConfigUpdate() const { return Config.IsSet(); }
		bool HasCollisionConfigUpdate() const { return CollisionConfig.IsSet(); }
		bool HasSimpleGeometryUpdate() const { return SimpleGeometry.IsSet(); }
		bool HasTriMeshData() const { return TriMeshData.IsSet(); }

		FRealtimeMeshConfig& GetConfigUpdate() { check(HasConfigUpdate()); return Config.GetValue(); }
		const FRealtimeMeshConfig& GetConfigUpdate() const { check(HasConfigUpdate()); return Config.GetValue(); }
		FRealtimeMeshCollisionConfiguration& GetCollisionConfigUpdate() { check(HasCollisionConfigUpdate()); return CollisionConfig.GetValue(); }
		const FRealtimeMeshCollisionConfiguration& GetCollisionConfigUpdate() const { check(HasCollisionConfigUpdate()); return CollisionConfig.GetValue(); }
		FRealtimeMeshSimpleGeometry& GetSimpleGeometryUpdate() { check(HasSimpleGeometryUpdate()); return SimpleGeometry.GetValue(); }
		const FRealtimeMeshSimpleGeometry& GetSimpleGeometryUpdate() const { check(HasSimpleGeometryUpdate()); return SimpleGeometry.GetValue(); }
		FRealtimeMeshTriMeshData& GetTriMeshData() { check(HasTriMeshData()); return TriMeshData.GetValue(); }
		const FRealtimeMeshTriMeshData& GetTriMeshData() const { check(HasTriMeshData()); return TriMeshData.GetValue(); }

		FRealtimeMeshConfig& AddConfigUpdate(const FRealtimeMeshConfig& InConfig) { Config = InConfig; return Config.GetValue(); }
		FRealtimeMeshCollisionConfiguration& AddCollisionConfigUpdate(const FRealtimeMeshCollisionConfiguration& InCollisionConfig) { CollisionConfig = InCollisionConfig; return CollisionConfig.GetValue(); }
		FRealtimeMeshSimpleGeometry& AddSimpleGeometryUpdate(const FRealtimeMeshSimpleGeometry& InSimpleGeometry) { SimpleGeometry = InSimpleGeometry; return SimpleGeometry.GetValue(); }
		FRealtimeMeshSimpleGeometry& AddSimpleGeometryUpdate(FRealtimeMeshSimpleGeometry&& InSimpleGeometry) { SimpleGeometry = MoveTemp(InSimpleGeometry); return SimpleGeometry.GetValue(); }
		FRealtimeMeshTriMeshData& AddTriMeshData(const FRealtimeMeshTriMeshData& InTriMeshData) { TriMeshData = InTriMeshData; return TriMeshData.GetValue(); }
		FRealtimeMeshTriMeshData& AddTriMeshData(FRealtimeMeshTriMeshData&& InTriMeshData) { TriMeshData = MoveTemp(InTriMeshData); return TriMeshData.GetValue(); }
	};
	*/
}
