// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Core/RealtimeMeshDataStream.h"
#include "Core/RealtimeMeshLayoutDispatch.h"
#include "Math/Box.h"

// General stream-set utilities. (Split out of the former RealtimeMeshAlgo.h; the
// namespace name is retained for source compatibility.)
namespace RealtimeMeshAlgo
{
	/**
	 * Compute the axis-aligned bounding box of a stream set's position stream.
	 * Supports both single- and double-precision position layouts; reads happen
	 * through TLayoutDispatch so the inner loop is monomorphized per type.
	 * Returns an empty box if there's no position stream or it's empty.
	 */
	inline FBox3f ComputeBounds(const RealtimeMesh::FRealtimeMeshStreamSet& Streams)
	{
		FBox3f Box(ForceInit);
		const RealtimeMesh::FRealtimeMeshStream* PosStream = Streams.Find(RealtimeMesh::FRealtimeMeshStreams::Position);
		if (!PosStream || PosStream->Num() == 0)
		{
			return Box;
		}

		RealtimeMesh::TLayoutDispatch<FVector3f, FVector3d>::Visit(*PosStream,
			[&Box](auto View)
			{
				const int32 Count = View.Num();
				for (int32 i = 0; i < Count; ++i)
				{
					Box += FVector3f(View[i]);
				}
			});

		return Box;
	}
}
