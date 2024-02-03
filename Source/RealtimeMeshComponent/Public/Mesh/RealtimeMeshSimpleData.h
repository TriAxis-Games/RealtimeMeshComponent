// Copyright TriAxis Games, L.L.C. All Rights Reserved.


#pragma once

#include "RealtimeMeshCore.h"
#include "Mesh/RealtimeMeshDataStream.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RealtimeMeshSimpleData.generated.h"

using namespace RealtimeMesh;

USTRUCT(BlueprintType, meta=(HasNativeMake="RealtimeMeshComponent.RealtimeMeshSimpleBlueprintFunctionLibrary.MakeRealtimeMeshSimpleStream"))
struct REALTIMEMESHCOMPONENT_API FRealtimeMeshSimpleMeshData
{
	GENERATED_BODY()
public:
	FRealtimeMeshSimpleMeshData()
		: bUseHighPrecisionTangents(false)
		, bUseHighPrecisionTexCoords(false)
	{
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealtimeMesh")
	TArray<int32> Triangles;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealtimeMesh")
	TArray<FVector> Positions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealtimeMesh")
	TArray<FVector> Normals;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealtimeMesh")
	TArray<FVector> Tangents;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealtimeMesh", AdvancedDisplay)
	TArray<FVector> Binormals;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealtimeMesh")
	TArray<FLinearColor> LinearColors;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealtimeMesh")
	TArray<FVector2D> UV0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealtimeMesh", AdvancedDisplay)
	TArray<FVector2D> UV1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealtimeMesh", AdvancedDisplay)
	TArray<FVector2D> UV2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealtimeMesh", AdvancedDisplay)
	TArray<FVector2D> UV3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealtimeMesh", AdvancedDisplay)
	TArray<FColor> Colors;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealtimeMesh", AdvancedDisplay)
	TArray<int32> MaterialIndex;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealtimeMesh", AdvancedDisplay)
	bool bUseHighPrecisionTangents;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealtimeMesh", AdvancedDisplay)
	bool bUseHighPrecisionTexCoords;


	bool CopyToStreamSet(FRealtimeMeshStreamSet& Streams, bool bCreateMissingStreams) const
	{
		const auto FindStream = [&Streams, bCreateMissingStreams](const FRealtimeMeshStreamKey& StreamKey, const FRealtimeMeshBufferLayout& Layout)
		{
			if (auto Stream = Streams.Find(StreamKey))
			{
				return Stream;
			}

			if (bCreateMissingStreams)
			{
				return Streams.AddStream(StreamKey, Layout);
			}

			return static_cast<FRealtimeMeshStream*>(nullptr);
		};
		
		FRealtimeMeshStream* TriangleStream = FindStream(FRealtimeMeshStreams::Triangles, GetRealtimeMeshBufferLayout<TIndex3<uint32>>());

		// We must have positions/triangles at min
		if (!TriangleStream || !(
			TriangleStream->GetLayout() == GetRealtimeMeshBufferLayout<TIndex3<uint16>>() ||
			TriangleStream->GetLayout() == GetRealtimeMeshBufferLayout<TIndex3<uint32>>() ||
			TriangleStream->GetLayout() == GetRealtimeMeshBufferLayout<uint16>() ||
			TriangleStream->GetLayout() == GetRealtimeMeshBufferLayout<uint32>() ||
			TriangleStream->GetLayout() == GetRealtimeMeshBufferLayout<TIndex3<int16>>() ||
			TriangleStream->GetLayout() == GetRealtimeMeshBufferLayout<TIndex3<int32>>() ||
			TriangleStream->GetLayout() == GetRealtimeMeshBufferLayout<int16>() ||
			TriangleStream->GetLayout() == GetRealtimeMeshBufferLayout<int32>()))
		{
			return false;
		}


		int32 StartIndex = INDEX_NONE;
		if (FRealtimeMeshStream* PositionsStream = FindStream(FRealtimeMeshStreams::Position, GetRealtimeMeshBufferLayout<FVector3f>()); PositionsStream)
		{
			if (PositionsStream->GetLayout() == GetRealtimeMeshBufferLayout<FVector3f>())
			{
				StartIndex = PositionsStream->Num();
				PositionsStream->AppendGenerated<FVector3f>(Positions.Num(), [this](int32 Index)
				{
					return FVector3f(Positions[Index]);
				});
			}
		}

		// Cannot copy data without at least positions being present
		if (StartIndex == INDEX_NONE)
		{
			return false;
		}

		if (FRealtimeMeshStream* TangentsStream = FindStream(FRealtimeMeshStreams::Tangents, GetRealtimeMeshBufferLayout<TRealtimeMeshTangents<FPackedNormal>>()); TangentsStream)
		{
			const auto ResizeTangents = [this, &TangentsStream, StartIndex]()
			{
				TangentsStream->Reserve(StartIndex + Positions.Num());
				if (StartIndex > 0 && TangentsStream->Num() != StartIndex)
				{
					TangentsStream->SetNumZeroed(StartIndex);
				}
			};

			if (TangentsStream->GetLayout() == GetRealtimeMeshBufferLayout<TRealtimeMeshTangents<FPackedNormal>>())
			{
				ResizeTangents();
				TangentsStream->AppendGenerated<TRealtimeMeshTangents<FPackedNormal>>(Positions.Num(), [this](int32 Index)
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
			}
			else if (TangentsStream->GetLayout() == GetRealtimeMeshBufferLayout<TRealtimeMeshTangents<FPackedRGBA16N>>())
			{
				ResizeTangents();
				TangentsStream->AppendGenerated<TRealtimeMeshTangents<FPackedRGBA16N>>(Positions.Num(), [this](int32 Index)
				{
					const FVector3d Normal = Normals.IsValidIndex(Index) ? Normals[Index] : FVector::ZAxisVector;
					const FVector3d Tangent = Tangents.IsValidIndex(Index) ? Tangents[Index] : FVector::XAxisVector;
					if (Binormals.IsValidIndex(Index))
					{
						const FVector3d Binormal = Binormals[Index];
						return TRealtimeMeshTangents<FPackedRGBA16N>(Normal, Binormal, Tangent);
					}
					else
					{
						return TRealtimeMeshTangents<FPackedRGBA16N>(Normal, Tangent);
					}
				});
			}
		}

		if (FRealtimeMeshStream* TexCoordsStream = FindStream(FRealtimeMeshStreams::TexCoords, GetRealtimeMeshBufferLayout<FVector2DHalf>()); TexCoordsStream)
		{
			const auto ResizeTexCoords = [this, &TexCoordsStream, StartIndex]()
			{
				TexCoordsStream->Reserve(StartIndex + Positions.Num());
				if (StartIndex > 0 && TexCoordsStream->Num() != StartIndex)
				{
					TexCoordsStream->SetNumZeroed(StartIndex);
				}
			};
			const int32 NumTexCoordChannels = FMath::Min(4, TexCoordsStream->GetNumElements());
			const TArray<FVector2D>* UVs[4] = {&UV0, &UV1, &UV2, &UV3};

			if (TexCoordsStream->GetLayout() == GetRealtimeMeshBufferLayout<FVector2DHalf>())
			{
				ResizeTexCoords();
				TexCoordsStream->SetNumUninitialized(StartIndex + Positions.Num());
				for (int32 UVIndex = 0; UVIndex < NumTexCoordChannels; UVIndex++)
				{
					const TArray<FVector2D>& UVChannel = *UVs[UVIndex];
					TexCoordsStream->SetGeneratedElement<FVector2DHalf>(UVIndex, StartIndex, Positions.Num(), [this, &UVChannel](int32 Index)
					{
						return FVector2DHalf(UVChannel[Index]);
					});
				}
			}
			else if (TexCoordsStream->GetLayout() == GetRealtimeMeshBufferLayout<FVector2f>())
			{
				ResizeTexCoords();
				TexCoordsStream->SetNumUninitialized(StartIndex + Positions.Num());
				for (int32 UVIndex = 0; UVIndex < NumTexCoordChannels; UVIndex++)
				{
					const TArray<FVector2D>& UVChannel = *UVs[UVIndex];
					TexCoordsStream->SetGeneratedElement<FVector2f>(UVIndex, StartIndex, Positions.Num(), [this, &UVChannel](int32 Index)
					{
						return FVector2f(UVChannel[Index]);
					});
				}
			}
		}

		if (FRealtimeMeshStream* ColorsStream = FindStream(FRealtimeMeshStreams::Color, GetRealtimeMeshBufferLayout<FColor>()); ColorsStream)
		{
			if (ColorsStream->GetLayout() == GetRealtimeMeshBufferLayout<FColor>())
			{
				const bool bUseLinearColors = LinearColors.Num() > 0;
				const bool bColorStreamHasMultiple = ColorsStream->Num() > 1;
				const bool bColorsHasMultiple = bUseLinearColors ? LinearColors.Num() > 1 : Colors.Num() > 1;

				const FColor bColorStreamDefault = ColorsStream->Num() == 1 ? *ColorsStream->GetDataAtVertex<FColor>(0) : FColor::White;
				const FColor bColorsDefault = bUseLinearColors
					                              ? (LinearColors.Num() == 1 ? LinearColors[0].ToFColor(true) : FColor::White)
					                              : (Colors.Num() == 1 ? Colors[0] : FColor::White);

				if (bColorStreamHasMultiple || bColorsHasMultiple || bColorStreamDefault != bColorsDefault)
				{
					ColorsStream->Reserve(StartIndex + Positions.Num());

					// Do we need to fill preceding elements					
					if (StartIndex > 0 && ColorsStream->Num() != StartIndex)
					{
						const int32 PreviousLength = ColorsStream->Num();
						if (ColorsStream->Num() == 1)
						{
							ColorsStream->SetNumUninitialized(StartIndex);
							if (StartIndex > PreviousLength)
							{
								ColorsStream->SetGenerated<FColor>(PreviousLength, StartIndex - PreviousLength, [this](int32 Index)
								{
									return Colors[0];
								});
							}
						}
						else
						{
							ColorsStream->SetNumUninitialized(StartIndex);
							if (StartIndex > PreviousLength)
							{
								ColorsStream->SetGenerated<FColor>(PreviousLength, StartIndex - PreviousLength, [this](int32 Index)
								{
									return FColor::White;
								});
							}
						}
					}

					if (bUseLinearColors)
					{
						ColorsStream->AppendGenerated<FColor>(Positions.Num(), [this](int32 Index)
						{
							return LinearColors[Index].ToFColor(true);
						});
					}
					else
					{
						ColorsStream->AppendGenerated<FColor>(Positions.Num(), [this](int32 Index)
						{
							return Colors[Index];
						});
					}
				}
				else
				{
					ColorsStream->SetNumUninitialized(1);
					*ColorsStream->GetDataAtVertex<FColor>(0) = FColor::White;
				}
			}
		}

		if (TriangleStream->GetLayout() == GetRealtimeMeshBufferLayout<TIndex3<uint16>>())
		{
			const int32 NumTriangles = Triangles.Num() / 3;
			TriangleStream->Reserve(TriangleStream->Num() + NumTriangles);
			TriangleStream->AppendGenerated<TIndex3<uint16>>(NumTriangles, [this, StartIndex](int32 Index)
			{
				return TIndex3<uint16>(Triangles[Index * 3 + 0] + StartIndex, Triangles[Index * 3 + 1] + StartIndex, Triangles[Index * 3 + 2] + StartIndex);
			});
		}
		else if (TriangleStream->GetLayout() == GetRealtimeMeshBufferLayout<TIndex3<uint32>>())
		{
			const int32 NumTriangles = Triangles.Num() / 3;
			TriangleStream->Reserve(TriangleStream->Num() + NumTriangles);
			TriangleStream->AppendGenerated<TIndex3<uint32>>(NumTriangles, [this, StartIndex](int32 Index)
			{
				return TIndex3<uint32>(Triangles[Index * 3 + 0] + StartIndex, Triangles[Index * 3 + 1] + StartIndex, Triangles[Index * 3 + 2] + StartIndex);
			});
		}
		else if (TriangleStream->GetLayout() == GetRealtimeMeshBufferLayout<uint16>())
		{
			const int32 NumTriangles = Triangles.Num();
			TriangleStream->Reserve(TriangleStream->Num() + NumTriangles);
			TriangleStream->AppendGenerated<uint16>(NumTriangles, [this, StartIndex](int32 Index)
			{
				return Triangles[Index] + StartIndex;
			});
		}
		else if (TriangleStream->GetLayout() == GetRealtimeMeshBufferLayout<uint32>())
		{
			const int32 NumTriangles = Triangles.Num();
			TriangleStream->Reserve(TriangleStream->Num() + NumTriangles);
			TriangleStream->AppendGenerated<uint32>(NumTriangles, [this, StartIndex](int32 Index)
			{
				return Triangles[Index] + StartIndex;
			});
		}

		else if (TriangleStream->GetLayout() == GetRealtimeMeshBufferLayout<TIndex3<int16>>())
		{
			const int32 NumTriangles = Triangles.Num() / 3;
			TriangleStream->Reserve(TriangleStream->Num() + NumTriangles);
			TriangleStream->AppendGenerated<TIndex3<int16>>(NumTriangles, [this, StartIndex](int32 Index)
			{
				return TIndex3<int16>(Triangles[Index * 3 + 0] + StartIndex, Triangles[Index * 3 + 1] + StartIndex, Triangles[Index * 3 + 2] + StartIndex);
			});
		}
		else if (TriangleStream->GetLayout() == GetRealtimeMeshBufferLayout<TIndex3<int32>>())
		{
			const int32 NumTriangles = Triangles.Num() / 3;
			TriangleStream->Reserve(TriangleStream->Num() + NumTriangles);
			TriangleStream->AppendGenerated<TIndex3<int32>>(NumTriangles, [this, StartIndex](int32 Index)
			{
				return TIndex3<int32>(Triangles[Index * 3 + 0] + StartIndex, Triangles[Index * 3 + 1] + StartIndex, Triangles[Index * 3 + 2] + StartIndex);
			});
		}
		else if (TriangleStream->GetLayout() == GetRealtimeMeshBufferLayout<int16>())
		{
			const int32 NumTriangles = Triangles.Num();
			TriangleStream->Reserve(TriangleStream->Num() + NumTriangles);
			TriangleStream->AppendGenerated<int16>(NumTriangles, [this, StartIndex](int32 Index)
			{
				return Triangles[Index] + StartIndex;
			});
		}
		else if (TriangleStream->GetLayout() == GetRealtimeMeshBufferLayout<int32>())
		{
			const int32 NumTriangles = Triangles.Num();
			TriangleStream->Reserve(TriangleStream->Num() + NumTriangles);
			TriangleStream->AppendGenerated<int32>(NumTriangles, [this, StartIndex](int32 Index)
			{
				return Triangles[Index] + StartIndex;
			});
		}

		
		if (FRealtimeMeshStream* PolygonGroupIndices = FindStream(FRealtimeMeshStreams::PolyGroups, GetRealtimeMeshBufferLayout<int32>()); PolygonGroupIndices)
		{
			if (PolygonGroupIndices->GetLayout() == GetRealtimeMeshBufferLayout<uint16>())
			{
				const int32 NumPolyGroupIndices = MaterialIndex.Num();
				PolygonGroupIndices->Reserve(PolygonGroupIndices->Num() + NumPolyGroupIndices);
				PolygonGroupIndices->AppendGenerated<uint16>(NumPolyGroupIndices, [this, StartIndex](int32 Index)
				{
					return MaterialIndex[Index];
				});
			}
			else if (PolygonGroupIndices->GetLayout() == GetRealtimeMeshBufferLayout<int16>())
			{
				const int32 NumPolyGroupIndices = MaterialIndex.Num();
				PolygonGroupIndices->Reserve(PolygonGroupIndices->Num() + NumPolyGroupIndices);
				PolygonGroupIndices->AppendGenerated<int16>(NumPolyGroupIndices, [this, StartIndex](int32 Index)
				{
					return MaterialIndex[Index];
				});
			}
			else if (PolygonGroupIndices->GetLayout() == GetRealtimeMeshBufferLayout<uint32>())
			{
				const int32 NumPolyGroupIndices = MaterialIndex.Num();
				PolygonGroupIndices->Reserve(PolygonGroupIndices->Num() + NumPolyGroupIndices);
				PolygonGroupIndices->AppendGenerated<uint32>(NumPolyGroupIndices, [this, StartIndex](int32 Index)
				{
					return MaterialIndex[Index];
				});
			}
			else if (PolygonGroupIndices->GetLayout() == GetRealtimeMeshBufferLayout<int32>())
			{
				const int32 NumPolyGroupIndices = MaterialIndex.Num();
				PolygonGroupIndices->Reserve(PolygonGroupIndices->Num() + NumPolyGroupIndices);
				PolygonGroupIndices->AppendGenerated<int32>(NumPolyGroupIndices, [this, StartIndex](int32 Index)
				{
					return MaterialIndex[Index];
				});
			}
		}

		return true;
	}
};

UCLASS(meta=(ScriptName="RealtimeMeshSimpleLibrary"))
class REALTIMEMESHCOMPONENT_API URealtimeMeshSimpleBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	
	UE_DEPRECATED(all, "Use FRealtimeMeshStreamSet instead")
	UFUNCTION(BlueprintCallable, Category = "RealtimeMesh|Simple",
		meta=(AdvancedDisplay="Triangles,Positions,Normals,Tangents,Binormals,LinearColors,UV0,UV1,UV2,UV3,Colors,bUseHighPrecisionTangents,bUseHighPrecisionTexCoords",
			AutoCreateRefTerm="Triangles,Positions,Normals,Tangents,Binormals,LinearColors,UV0,UV1,UV2,UV3,Colors"))
	static FRealtimeMeshSimpleMeshData MakeRealtimeMeshSimpleStream(
		const TArray<int32>& Triangles,
		const TArray<FVector>& Positions,
		const TArray<FVector>& Normals,
		const TArray<FVector>& Tangents,
		const TArray<FVector>& Binormals,
		const TArray<FLinearColor>& LinearColors,
		const TArray<FVector2D>& UV0,
		const TArray<FVector2D>& UV1,
		const TArray<FVector2D>& UV2,
		const TArray<FVector2D>& UV3,
		const TArray<FColor>& Colors,
		bool bUseHighPrecisionTangents,
		bool bUseHighPrecisionTexCoords);
};