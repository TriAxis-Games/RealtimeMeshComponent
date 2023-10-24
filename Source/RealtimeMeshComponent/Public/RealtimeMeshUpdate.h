/*
// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "RealtimeMeshConfig.h"
#include "RealtimeMeshCore.h"
#include "Mesh/RealtimeMeshDataStream.h"

namespace RealtimeMesh
{
	struct REALTIMEMESHCOMPONENT_API FRealtimeMeshSectionUpdate
	{
	private:
		TOptional<FRealtimeMeshSectionConfig> Config;
		TOptional<FRealtimeMeshStreamRange> StreamRange;
		TOptional<FBox> LocalBounds;

	public:
		void UpdateConfig(const FRealtimeMeshSectionConfig& InConfig) { Config = InConfig; }
		void UpdateStreamRange(const FRealtimeMeshStreamRange& InStreamRange) { StreamRange = InStreamRange; }
		void UpdateLocalBounds(const FBox& InLocalBounds) { LocalBounds = InLocalBounds; }
	};


	struct REALTIMEMESHCOMPONENT_API FRealtimeMeshSectionGroupUpdate
	{
	private:
		TMap<FRealtimeMeshStreamKey, FRealtimeMeshSectionGroupStreamUpdateDataRef> StreamUpdates;
		TSet<FRealtimeMeshStreamKey> StreamsToRemove;
		TSparseArray<FRealtimeMeshSectionUpdate> SectionUpdates;
		TBitArray<> SectionsToRemove;

		uint32 bRemoveRemainingSections : 1;
		uint32 bRemoveRemainingStreams : 1;

	public:
		void ClearAllExistingSections() { bRemoveRemainingSections = true; }
		void ClearAllExistingStreams() { bRemoveRemainingStreams = true; }
		//void RemoveSection(uint32 )
	};
}
*/
