// Copyright TriAxis Games, L.L.C. All Rights Reserved.


#include "Mesh/RealtimeMeshSimpleData.h"


bool FRealtimeMeshSimpleMeshData::CopyToStreamSet(FRealtimeMeshStreamSet& Streams, bool bCreateMissingStreams) const
{
	// If we have no valid data, we cannot copy anything
	if (Positions.Num() < 3 || Triangles.Num() < 3)
	{
		return false;
	}

		
	// Copy positions
	TRealtimeMeshStreamBuilder<FVector3f> PositionStream(Streams.FindOrAdd(FRealtimeMeshStreams::Position, GetRealtimeMeshBufferLayout<FVector3f>()));
	const int32 StartVertex = PositionStream.Num();
	PositionStream.AppendGenerator(Positions.Num(), [this](int32 Index, int32 FinalIndex)
	{
		return FVector3f(Positions[Index]);
	});

	// Copy tangents
	TRealtimeMeshStreamBuilder<TRealtimeMeshTangents<FVector4f>, void> TangentStream(Streams.FindOrAdd(FRealtimeMeshStreams::Tangents, 
		bUseHighPrecisionTangents?
		GetRealtimeMeshBufferLayout<FRealtimeMeshTangentsHighPrecision>():
		GetRealtimeMeshBufferLayout<FRealtimeMeshTangentsNormalPrecision>()
	));
	// Zero any unfilled tangents up to start of this group
	if (StartVertex > 0 && TangentStream.Num() != StartVertex)
	{
		TangentStream.SetNumWithFill(StartVertex, TRealtimeMeshTangents<FVector4f>(FVector3f::ZAxisVector, FVector3f::XAxisVector));
	}
	TangentStream.AppendGenerator(Positions.Num(), [this](int32 Index, int32 FinalIndex)
	{
		const FVector Normal = Normals.IsValidIndex(Index) ? Normals[Index] : FVector::ZAxisVector;
		const FVector Tangent = Tangents.IsValidIndex(Index) ? Tangents[Index] : FVector::XAxisVector;
		if (Binormals.IsValidIndex(Index))
		{
			const FVector Binormal = Binormals[Index];
			return TRealtimeMeshTangents<FPackedNormal>(Normal, Binormal, Tangent);
		}
		else
		{
			return TRealtimeMeshTangents<FPackedNormal>(Normal, Tangent);
		}
	});

	// Copy Colors
	TRealtimeMeshStreamBuilder<FColor> ColorStream(Streams.FindOrAdd(FRealtimeMeshStreams::Color, GetRealtimeMeshBufferLayout<FColor>()));
	// Zero any unfilled tangents up to start of this group
	if (StartVertex > 0 && ColorStream.Num() != StartVertex)
	{
		ColorStream.SetNumWithFill(StartVertex, FColor::White);
	}
	if (LinearColors.Num() > 0)
	{
		ColorStream.AppendGenerator(Positions.Num(), [this](int32 Index, int32 FinalIndex)
		{
			return LinearColors.IsValidIndex(Index)? LinearColors[Index].ToFColor(true) : FColor::White;
		});
	}
	else
	{
		ColorStream.AppendGenerator(Positions.Num(), [this](int32 Index, int32 FinalIndex)
		{
			return Colors.IsValidIndex(Index)? Colors[Index] : FColor::White;
		});			
	}

	// Copy TexCoords
	const TArray<FVector2D>* TexCoords[4] = { &UV0, &UV1, &UV2, &UV3 };
	const int32 NumTexCoordsSupplied = UV3.Num() > 0 ? 4 : UV2.Num() > 0 ? 3 : UV1.Num() > 0 ? 2 : UV0.Num() > 0 ? 1 : 0;

	FRealtimeMeshStream& TexCoordStream = Streams.FindOrAdd(FRealtimeMeshStreams::TexCoords,
		bUseHighPrecisionTexCoords ?
		GetRealtimeMeshBufferLayout<FVector2f>(NumTexCoordsSupplied) :
		GetRealtimeMeshBufferLayout<FVector2DHalf>(NumTexCoordsSupplied)
	);
	if (StartVertex > 0 && TexCoordStream.Num() != StartVertex)
	{
		TexCoordStream.SetNumZeroed(StartVertex);
	}
	const int32 NumTexCoordChannels = FMath::Min(TexCoordStream.GetNumElements(), NumTexCoordsSupplied);
	for (int32 TexIndex = 0; TexIndex < NumTexCoordChannels; TexIndex++)
	{
		const TArray<FVector2D>& UVs = *TexCoords[TexIndex];
		if (UVs.Num() > 0)
		{
			// Copy tex coords
			TRealtimeMeshStridedStreamBuilder<FVector2f, void> TexCoordBuilder(TexCoordStream, TexIndex);

			if (TexCoordBuilder.Num() < StartVertex + UVs.Num())
			{
				TexCoordBuilder.SetNumZeroed(StartVertex + UVs.Num());
			}
			TexCoordBuilder.SetGenerator(StartVertex, Positions.Num(), [&](int32 Index, int32 FinalIndex)
			{
				return UVs.IsValidIndex(Index)? FVector2f(UVs[Index]) : FVector2f::ZeroVector;
			});
		}		
	}

		
	TRealtimeMeshStreamBuilder<TIndex3<uint32>, void> TriangleStream(Streams.FindOrAdd(FRealtimeMeshStreams::Triangles, GetRealtimeMeshBufferLayout<TIndex3<uint32>>()));
	const int32 StartTriangle = TriangleStream.Num();
	TriangleStream.AppendGenerator(Triangles.Num() / 3, [&](int32 Index, int32 FinalIndex)
	{
		const uint32 V0 = Triangles[Index * 3 + 0] + StartVertex;
		const uint32 V1 = Triangles[Index * 3 + 1] + StartVertex;
		const uint32 V2 = Triangles[Index * 3 + 2] + StartVertex;
		return TIndex3<uint32>(V0, V1, V2);
	});


	if (MaterialIndex.Num())
	{
		TRealtimeMeshStreamBuilder<uint32, void> PolyGroupStream(Streams.FindOrAdd(FRealtimeMeshStreams::PolyGroups, GetRealtimeMeshBufferLayout<uint32>()));
		if (StartTriangle > 0 && PolyGroupStream.Num() != StartTriangle)
		{
			PolyGroupStream.SetNumZeroed(StartTriangle);
		}
		PolyGroupStream.AppendGenerator(Triangles.Num() / 3, [this](int32 Index, int32 FinalIndex)
		{
			return MaterialIndex.IsValidIndex(Index)? MaterialIndex[Index] : 0;
		});
	}

	return true;
}

PRAGMA_DISABLE_DEPRECATION_WARNINGS
FRealtimeMeshSimpleMeshData URealtimeMeshSimpleBlueprintFunctionLibrary::MakeRealtimeMeshSimpleStream(const TArray<int32>& Triangles, const TArray<FVector>& Positions,
	const TArray<FVector>& Normals, const TArray<FVector>& Tangents, const TArray<FVector>& Binormals, const TArray<FLinearColor>& LinearColors, const TArray<FVector2D>& UV0,
	const TArray<FVector2D>& UV1, const TArray<FVector2D>& UV2, const TArray<FVector2D>& UV3, const TArray<FColor>& Colors, bool bUseHighPrecisionTangents, bool bUseHighPrecisionTexCoords)
{
	FRealtimeMeshSimpleMeshData NewMeshData;
	NewMeshData.Triangles = Triangles;
	NewMeshData.Positions = Positions;
	NewMeshData.Normals = Normals;
	NewMeshData.Tangents = Tangents;
	NewMeshData.Binormals = Binormals;
	NewMeshData.LinearColors = LinearColors;
	NewMeshData.UV0 = UV0;
	NewMeshData.UV1 = UV1;
	NewMeshData.UV2 = UV2;
	NewMeshData.UV3 = UV3;
	NewMeshData.Colors = Colors;
	NewMeshData.bUseHighPrecisionTangents = bUseHighPrecisionTangents;
	NewMeshData.bUseHighPrecisionTexCoords = bUseHighPrecisionTexCoords;
	return NewMeshData;
}
PRAGMA_ENABLE_DEPRECATION_WARNINGS
