// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#include "Mesh/RealtimeMeshTangentUtils.h"

#include "Core/RealtimeMeshBuilder.h"
#include "Core/RealtimeMeshDataStream.h"
#include "Core/RealtimeMeshDataTypes.h"
#include "Core/RealtimeMeshLayoutDispatch.h"

using namespace RealtimeMesh;

void RealtimeMeshAlgo::GenerateTangents(RealtimeMesh::FRealtimeMeshStreamSet& StreamSet, bool bComputeSmoothNormals)
{
	if (!StreamSet.Contains(FRealtimeMeshStreams::Triangles) || !StreamSet.Contains(FRealtimeMeshStreams::Position))
	{
		return;
	}

	const FRealtimeMeshStream& PosStream = StreamSet.FindChecked(FRealtimeMeshStreams::Position);
	const FRealtimeMeshStream& TriStream = StreamSet.FindChecked(FRealtimeMeshStreams::Triangles);

	// Snapshot positions into a local FVector3f array, converting from FVector3d
	// if the source stream is double-precision. The algorithm itself operates
	// on single-precision throughout.
	const uint32 NumVertices = PosStream.Num();
	if (NumVertices == 0)
	{
		// Nothing to generate. Guard is also required for correctness: with
		// triangles present the `NumVertices - 1` clamp below would wrap
		// (unsigned) to 0xFFFFFFFF and index out of bounds.
		return;
	}

	TArray<FVector3f> Positions;
	Positions.SetNumUninitialized(NumVertices);
	const bool bGotPositions = TLayoutDispatch<FVector3f, FVector3d>::Visit(PosStream,
		[&](auto View)
		{
			for (uint32 i = 0; i < NumVertices; ++i) Positions[i] = FVector3f(View[i]);
		});
	checkf(bGotPositions, TEXT("GenerateTangents: unsupported position stream layout"));

	// Snapshot triangle indices into a flat uint32 array (3 indices per
	// triangle row). Index format is dispatched: uint16/int16/uint32/int32 are
	// supported and converted to uint32 on read.
	const uint32 NumTris = TriStream.Num();
	const uint32 NumIndices = NumTris * 3;
	TArray<uint32> Indices;
	Indices.SetNumUninitialized(NumIndices);
	const bool bGotIndices = TElementDispatch<uint16, int16, uint32, int32>::Visit(TriStream,
		[&](auto View)
		{
			for (int32 i = 0; i < View.Num(); ++i) Indices[i] = static_cast<uint32>(View[i]);
		});
	checkf(bGotIndices, TEXT("GenerateTangents: unsupported triangle stream layout"));

	TOptional<TRealtimeMeshStridedStreamBuilder<FVector2f, void>> TexCoords;
	if (StreamSet.Contains(FRealtimeMeshStreams::TexCoords))
	{
		TexCoords = TRealtimeMeshStridedStreamBuilder<FVector2f, void>(StreamSet.FindChecked(FRealtimeMeshStreams::TexCoords));
	}

	StreamSet.Remove(FRealtimeMeshStreams::Tangents);
	TRealtimeMeshStreamBuilder<FRealtimeMeshTangentsNormalPrecision> Tangents(StreamSet.AddStream<FRealtimeMeshTangentsNormalPrecision>(FRealtimeMeshStreams::Tangents));
	Tangents.SetNumZeroed(NumVertices);

	// Delegate to the shared array-view/callback core so the tangent algorithm lives
	// in exactly one place. Positions/indices were snapshotted above; here we expose
	// them as views, adapt the optional TexCoords stream to a UV getter, and write the
	// full basis (normal Z, binormal Y, tangent X) back through the stream builder,
	// preserving the binormal-handedness sign this overload has always produced.
	TFunction<FVector2f(int32)> UVGetter;
	if (TexCoords.IsSet())
	{
		UVGetter = [&TexCoords](int32 Index) { return TexCoords->GetValue(Index); };
	}

	RealtimeMeshAlgo::GenerateTangentsImpl<uint32>(
		TConstArrayView<const uint32>(Indices),
		TConstArrayView<const FVector3f>(Positions),
		UVGetter,
		[&Tangents](int32 VertIdx, FVector3f TangentX, FVector3f TangentY, FVector3f TangentZ)
		{
			Tangents.Set(VertIdx, FRealtimeMeshTangentsNormalPrecision(TangentZ, TangentY, TangentX));
		},
		bComputeSmoothNormals);
}
