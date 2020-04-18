// Copyright 2016-2020 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "RuntimeMeshCore.h"
#include "Containers/StaticArray.h"
#include "Interfaces/Interface_CollisionDataProvider.h"
#include "RuntimeMeshRenderable.generated.h"

USTRUCT(BlueprintType)
struct RUNTIMEMESHCOMPONENT_API FRuntimeMeshVertexPositionStream
{
	GENERATED_USTRUCT_BODY();

private:
	TArray<uint8> Data;

public:
	FRuntimeMeshVertexPositionStream() { }

	void SetNum(int32 NewNum, bool bAllowShrinking = true)
	{
		Data.SetNum(NewNum * sizeof(FVector), bAllowShrinking);
	}

	int32 Num() const
	{
		return Data.Num() / sizeof(FVector);
	}

	const uint8* GetData() const { return reinterpret_cast<const uint8*>(Data.GetData()); }

	const TArray<FVector> GetCopy() const
	{
		TArray<FVector> OutData;
		OutData.SetNum(Num());
		FMemory::Memcpy(OutData.GetData(), Data.GetData(), Data.Num());
		return OutData;
	}

	void Empty(int32 Slack = 0)
	{
		Data.Empty(Slack * sizeof(FVector));
	}

	int32 Add(const FVector& InPosition)
	{
		int32 Index = Data.Num();
		Data.AddUninitialized(sizeof(FVector));
		*((FVector*)&Data[Index]) = InPosition;
		return Index / sizeof(FVector);
	}

	void Append(const FRuntimeMeshVertexPositionStream& InOther)
	{
		Data.Append(InOther.Data);
	}

	void Append(const TArray<FVector>& InPositions)
	{
		int32 StartIndex = Data.Num();
		Data.SetNum(StartIndex + sizeof(FVector) * InPositions.Num());
		FMemory::Memcpy(Data.GetData() + StartIndex, InPositions.GetData(), InPositions.Num() * sizeof(FVector));
	}

	const FVector& GetPosition(int32 Index) const
	{
		return *((FVector*)&Data[Index * sizeof(FVector)]);
	}

	void SetPosition(int32 Index, const FVector& NewPosition)
	{
		*((FVector*)&Data[Index * sizeof(FVector)]) = NewPosition;
	}

	FBox GetBounds() const
	{
		FBox NewBox(ForceInit);
		int32 Count = Num();
		for (int32 Index = 0; Index < Count; Index++)
		{
			NewBox += GetPosition(Index);
		}
		return NewBox;
	}

	bool Serialize(FArchive& Ar)
	{
		Ar << Data;
		return true;
	}
};

template<> struct TStructOpsTypeTraits<FRuntimeMeshVertexPositionStream> : public TStructOpsTypeTraitsBase2<FRuntimeMeshVertexPositionStream>
{
	enum
	{
		WithSerializer = true
	};
};

USTRUCT(BlueprintType)
struct RUNTIMEMESHCOMPONENT_API FRuntimeMeshVertexTangentStream
{
	GENERATED_USTRUCT_BODY();
	/*
		This is always laid out tangent first, normal second, and it's either FPackedNormal for normal precision or FPackedRGBA16N for high precision.
	*/

private:
	TArray<uint8> Data;
	bool bIsHighPrecision;

public:
	bool IsHighPrecision() const { return bIsHighPrecision; }
private:
	int32 GetElementSize() const { return (bIsHighPrecision ? sizeof(FPackedRGBA16N) : sizeof(FPackedNormal)); }
	int32 GetStride() const { return GetElementSize() * 2; }

public:
	FRuntimeMeshVertexTangentStream(bool bInUseHighPrecisionTangentBasis = false)
		: bIsHighPrecision(bInUseHighPrecisionTangentBasis)
	{
	}

	void SetNum(int32 NewNum, bool bAllowShrinking = true)
	{
		Data.SetNum(NewNum * GetStride(), bAllowShrinking);
	}

	int32 Num() const
	{
		return Data.Num() / GetStride();
	}

	const uint8* GetData() const { return reinterpret_cast<const uint8*>(Data.GetData()); }

	void Empty(int32 Slack = 0)
	{
		Data.Empty(Slack * GetStride());
	}

	int32 Add(const FVector& InNormal, const FVector& InTangent)
	{
		int32 Index = Data.Num();
		int32 Stride = GetStride();
		Data.AddUninitialized(Stride);
		if (bIsHighPrecision)
		{
			*((FPackedRGBA16N*)&Data[Index]) = FPackedRGBA16N(InTangent);
			*((FPackedRGBA16N*)&Data[Index + sizeof(FPackedRGBA16N)]) = FPackedRGBA16N(InNormal);
		}
		else
		{
			*((FPackedNormal*)&Data[Index]) = FPackedNormal(InTangent);
			*((FPackedNormal*)&Data[Index + sizeof(FPackedNormal)]) = FPackedNormal(InNormal);
		}
		return Index / Stride;
	}

	int32 Add(const FVector4& InNormal, const FVector& InTangent)
	{
		int32 Index = Data.Num();
		int32 Stride = GetStride();
		Data.AddUninitialized(Stride);
		if (bIsHighPrecision)
		{
			*((FPackedRGBA16N*)&Data[Index]) = FPackedRGBA16N(InTangent);
			*((FPackedRGBA16N*)&Data[Index + sizeof(FPackedRGBA16N)]) = FPackedRGBA16N(InNormal);
		}
		else
		{
			*((FPackedNormal*)&Data[Index]) = FPackedNormal(InTangent);
			*((FPackedNormal*)&Data[Index + sizeof(FPackedNormal)]) = FPackedNormal(InNormal);
		}
		return Index / Stride;
	}

	int32 Add(const FVector& InTangentX, const FVector& InTangentY, const FVector& InTangentZ)
	{
		int32 Index = Data.Num();
		int32 Stride = GetStride();
		Data.AddUninitialized(Stride);
		if (bIsHighPrecision)
		{
			*((FPackedRGBA16N*)&Data[Index]) = FPackedRGBA16N(InTangentX);
			*((FPackedRGBA16N*)&Data[Index + sizeof(FPackedRGBA16N)]) = FPackedRGBA16N(FVector4(InTangentZ, GetBasisDeterminantSign(InTangentX, InTangentY, InTangentZ)));
		}
		else
		{
			*((FPackedNormal*)&Data[Index]) = FPackedNormal(InTangentX);
			*((FPackedNormal*)&Data[Index + sizeof(FPackedNormal)]) = FPackedNormal(FVector4(InTangentZ, GetBasisDeterminantSign(InTangentX, InTangentY, InTangentZ)));
		}
		return Index / Stride;
	}

	void Append(const FRuntimeMeshVertexTangentStream& InOther)
	{
		Data.Append(InOther.Data);
	}

	void Append(const TArray<FVector>& InNormals, const TArray<FRuntimeMeshTangent>& InTangents)
	{
		int32 MaxCount = FMath::Max(InNormals.Num(), InTangents.Num());

		for (int32 Index = 0; Index < MaxCount; Index++)
		{
			const int32 NormalIndex = FMath::Min(Index, InNormals.Num());
			const int32 TangentIndex = FMath::Min(Index, InTangents.Num());

			FVector4 Normal;
			FVector Tangent;

			if (InNormals.IsValidIndex(NormalIndex))
			{
				Normal = InNormals[NormalIndex];
			}
			else
			{
				Normal = FVector(0, 0, 1);
			}

			if (InTangents.IsValidIndex(TangentIndex))
			{
				Tangent = InTangents[TangentIndex].TangentX;
				Normal.W = InTangents[TangentIndex].bFlipTangentY ? -1 : 1;
			}
			else
			{
				Tangent = FVector(1, 0, 0);
				Normal.W = 1.0f;
			}
			
			Add(Normal, Tangent);
		}
	}

	FVector GetNormal(int32 Index) const
	{
		const int32 EntryIndex = Index * 2 + 1;
		if (bIsHighPrecision)
		{
			return (*((FPackedRGBA16N*)&Data[EntryIndex * sizeof(FPackedRGBA16N)])).ToFVector();
		}
		else
		{
			return (*((FPackedNormal*)&Data[EntryIndex * sizeof(FPackedNormal)])).ToFVector();
		}
	}

	void SetNormal(int32 Index, const FVector& NewNormal)
	{
		const int32 EntryIndex = Index * 2 + 1;
		if (bIsHighPrecision)
		{
			*((FPackedRGBA16N*)&Data[EntryIndex * sizeof(FPackedRGBA16N)]) = FPackedRGBA16N(NewNormal);
		}
		else
		{
			*((FPackedNormal*)&Data[EntryIndex * sizeof(FPackedNormal)]) = FPackedNormal(NewNormal);
		}
	}

	FVector GetTangent(int32 Index) const
	{
		const int32 EntryIndex = Index * 2;
		if (bIsHighPrecision)
		{
			return (*((FPackedRGBA16N*)&Data[EntryIndex * sizeof(FPackedRGBA16N)])).ToFVector();
		}
		else
		{
			return (*((FPackedNormal*)&Data[EntryIndex * sizeof(FPackedNormal)])).ToFVector();
		}
	}

	void SetTangent(int32 Index, const FVector& NewTangent)
	{
		const int32 EntryIndex = Index * 2;
		if (bIsHighPrecision)
		{
			*((FPackedRGBA16N*)&Data[EntryIndex * sizeof(FPackedRGBA16N)]) = FPackedRGBA16N(NewTangent);
		}
		else
		{
			*((FPackedNormal*)&Data[EntryIndex * sizeof(FPackedNormal)]) = FPackedNormal(NewTangent);
		}
	}

	void GetTangents(int32 Index, FVector& OutTangentX, FVector& OutTangentY, FVector& OutTangentZ) const
	{
		const int32 EntryIndex = Index * 2;
		if (bIsHighPrecision)
		{
			FPackedRGBA16N TempTangentX = *((FPackedRGBA16N*)&Data[EntryIndex * sizeof(FPackedRGBA16N)]);
			FPackedRGBA16N TempTangentZ = *((FPackedRGBA16N*)&Data[(EntryIndex + 1) * sizeof(FPackedRGBA16N)]);
			OutTangentX = TempTangentX.ToFVector();
			OutTangentY = GenerateYAxis(TempTangentX, TempTangentZ);
			OutTangentZ = TempTangentZ.ToFVector();
		}
		else
		{
			FPackedNormal TempTangentX = *((FPackedNormal*)&Data[EntryIndex * sizeof(FPackedNormal)]);
			FPackedNormal TempTangentZ = *((FPackedNormal*)&Data[(EntryIndex + 1) * sizeof(FPackedNormal)]);
			OutTangentX = TempTangentX.ToFVector();
			OutTangentY = GenerateYAxis(TempTangentX, TempTangentZ);
			OutTangentZ = TempTangentZ.ToFVector();
		}
	}

	void SetTangents(int32 Index, const FVector& InTangentX, const FVector& InTangentY, const FVector& InTangentZ)
	{
		const int32 EntryIndex = Index * 2;
		if (bIsHighPrecision)
		{
			*((FPackedRGBA16N*)&Data[EntryIndex * sizeof(FPackedRGBA16N)]) = FPackedRGBA16N(InTangentX);
			*((FPackedRGBA16N*)&Data[(EntryIndex + 1) * sizeof(FPackedRGBA16N)]) = FPackedRGBA16N(FVector4(InTangentZ, GetBasisDeterminantSign(InTangentX, InTangentY, InTangentZ)));
		}
		else
		{
			*((FPackedNormal*)&Data[EntryIndex * sizeof(FPackedNormal)]) = FPackedNormal(InTangentX);
			*((FPackedNormal*)&Data[(EntryIndex + 1) * sizeof(FPackedNormal)]) = FPackedNormal(FVector4(InTangentZ, GetBasisDeterminantSign(InTangentX, InTangentY, InTangentZ)));
		}
	}


	bool Serialize(FArchive& Ar)
	{
		Ar << bIsHighPrecision;
		Ar << Data;
		return true;
	}
};

template<> struct TStructOpsTypeTraits<FRuntimeMeshVertexTangentStream> : public TStructOpsTypeTraitsBase2<FRuntimeMeshVertexTangentStream>
{
	enum
	{
		WithSerializer = true
	};
};

USTRUCT(BlueprintType)
struct RUNTIMEMESHCOMPONENT_API FRuntimeMeshVertexTexCoordStream
{
	GENERATED_USTRUCT_BODY();

private:
	TArray<uint8> Data;
	int32 ChannelCount;
	bool bIsHighPrecision;

public:
	bool IsHighPrecision() const { return bIsHighPrecision; }
private:
	int32 GetElementSize() const { return (bIsHighPrecision ? sizeof(FVector2D) : sizeof(FVector2DHalf)); }
	int32 GetStride() const { return GetElementSize() * ChannelCount; }

public:
	FRuntimeMeshVertexTexCoordStream(int32 InChannelCount = 1, bool bInShouldUseHighPrecision = false)
		: ChannelCount(InChannelCount), bIsHighPrecision(bInShouldUseHighPrecision)
	{
	}

	void SetNum(int32 NewNum, bool bAllowShrinking = true)
	{
		Data.SetNum(NewNum * GetStride(), bAllowShrinking);
	}

	int32 Num() const
	{
		return Data.Num() / GetStride();
	}

	int32 NumChannels() const
	{
		return ChannelCount;
	}

	const uint8* GetData() const { return reinterpret_cast<const uint8*>(Data.GetData()); }

	void Empty(int32 Slack = 0)
	{
		Data.Empty(Slack * GetStride());
	}

	int32 Add(const FVector2D& InTexCoord, int32 ChannelId = 0)
	{
		int32 Index = Num();
		int32 Stride = GetStride();
		Data.AddZeroed(Stride);

		if (bIsHighPrecision)
		{
			static const int32 ElementSize = sizeof(FVector2D);
			*((FVector2D*)&Data[(Index * Stride) + (ChannelId * ElementSize)]) = InTexCoord;
		}
		else
		{
			static const int32 ElementSize = sizeof(FVector2DHalf);
			*((FVector2DHalf*)&Data[(Index * Stride) + (ChannelId * ElementSize)]) = InTexCoord;
		}

		return Index / Stride;
	}

	void Append(const FRuntimeMeshVertexTexCoordStream& InOther)
	{
		Data.Append(InOther.Data);
	}

	void FillIn(int32 StartIndex, const TArray<FVector2D>& InChannelData, int32 ChannelId = 0)
	{
		if (Num() < (StartIndex + InChannelData.Num()))
		{
			SetNum(StartIndex + InChannelData.Num());
		}

		for (int32 Index = 0; Index < InChannelData.Num(); Index++)
		{
			SetTexCoord(StartIndex + Index, InChannelData[Index], ChannelId);
		}
	}

	const FVector2D GetTexCoord(int32 Index, int32 ChannelId = 0) const
	{
		if (bIsHighPrecision)
		{
			static const int32 ElementSize = sizeof(FVector2D);
			return *((FVector2D*)&Data[(Index * ElementSize * ChannelCount) + (ChannelId * ElementSize)]);
		}
		else
		{
			static const int32 ElementSize = sizeof(FVector2DHalf);
			return *((FVector2DHalf*)&Data[(Index * ElementSize * ChannelCount) + (ChannelId * ElementSize)]);
		}
	}

	void SetTexCoord(int32 Index, const FVector2D& NewTexCoord, int32 ChannelId = 0)
	{
		if (bIsHighPrecision)
		{
			static const int32 ElementSize = sizeof(FVector2D);
			*((FVector2D*)&Data[(Index * ElementSize * ChannelCount) + (ChannelId * ElementSize)]) = NewTexCoord;
		}
		else
		{
			static const int32 ElementSize = sizeof(FVector2DHalf);
			*((FVector2DHalf*)&Data[(Index * ElementSize * ChannelCount) + (ChannelId * ElementSize)]) = NewTexCoord;
		}
	}

// 	friend FArchive& operator <<(FArchive& Ar, FRuntimeMeshVertexTexCoordStream& Stream)
// 	{
// 		Ar << Stream.bIsHighPrecision;
// 		Ar << Stream.ChannelCount;
// 		Ar << Stream.Data;
// 		return Ar;
// 	}
	
	bool Serialize(FArchive& Ar)
	{
		Ar << bIsHighPrecision;
		Ar << ChannelCount;
		Ar << Data;
		return true;
	}
};

template<> struct TStructOpsTypeTraits<FRuntimeMeshVertexTexCoordStream> : public TStructOpsTypeTraitsBase2<FRuntimeMeshVertexTexCoordStream>
{
	enum
	{
		WithSerializer = true
	};
};

USTRUCT(BlueprintType)
struct RUNTIMEMESHCOMPONENT_API FRuntimeMeshVertexColorStream
{
	GENERATED_USTRUCT_BODY();

private:
	TArray<uint8> Data;

public:
	FRuntimeMeshVertexColorStream() { }

	void SetNum(int32 NewNum, bool bAllowShrinking = true)
	{
		Data.SetNum(NewNum * sizeof(FColor), bAllowShrinking);
	}

	int32 Num() const
	{
		return Data.Num() / sizeof(FColor);
	}

	const uint8* GetData() const { return reinterpret_cast<const uint8*>(Data.GetData()); }

	void Empty(int32 Slack = 0)
	{
		Data.Empty(Slack * sizeof(FColor));
	}

	int32 Add(const FColor& InColor)
	{
		int32 Index = Data.Num();
		Data.AddUninitialized(sizeof(FColor));
		*((FColor*)&Data[Index]) = InColor;
		return Index / sizeof(FColor);
	}

	void Append(const FRuntimeMeshVertexColorStream& InOther)
	{
		Data.Append(InOther.Data);
	}

	void Append(const TArray<FColor>& InColors)
	{
		int32 StartIndex = Data.Num();
		Data.SetNum(StartIndex + sizeof(FColor) * InColors.Num());
		FMemory::Memcpy(Data.GetData() + StartIndex, InColors.GetData(), InColors.Num() * sizeof(FColor));
	}

	void Append(const TArray<FLinearColor>& InColors)
	{
		int32 StartIndex = Num();
		SetNum(Num() + InColors.Num());
		for (int32 Index = 0; Index < InColors.Num(); Index++)
		{
			SetColor(StartIndex + Index, InColors[Index].ToFColor(true));
		}
	}

	const FColor& GetColor(int32 Index) const
	{
		return *((FColor*)&Data[Index * sizeof(FColor)]);
	}

	void SetColor(int32 Index, const FColor& NewColor)
	{
		*((FColor*)&Data[Index * sizeof(FColor)]) = NewColor;
	}

	bool Serialize(FArchive& Ar)
	{
		Ar << Data;
		return true;
	}
};

template<> struct TStructOpsTypeTraits<FRuntimeMeshVertexColorStream> : public TStructOpsTypeTraitsBase2<FRuntimeMeshVertexColorStream>
{
	enum
	{
		WithSerializer = true
	};
};

USTRUCT(BlueprintType)
struct RUNTIMEMESHCOMPONENT_API FRuntimeMeshTriangleStream
{
	GENERATED_USTRUCT_BODY();

private:
	TArray<uint8> Data;
	bool bIsUsing32BitIndices;
	uint8 Stride;

public:
	bool IsHighPrecision() const { return bIsUsing32BitIndices; }

public:
	FRuntimeMeshTriangleStream(bool bInUse32BitIndices = false)
		: bIsUsing32BitIndices(bInUse32BitIndices), Stride(bInUse32BitIndices ? sizeof(uint32) : sizeof(uint16))
	{

	}

	void SetNum(int32 NewNum, bool bAllowShrinking = true)
	{
		Data.SetNum(NewNum * Stride, bAllowShrinking);
	}

	int32 Num() const
	{
		return Data.Num() / Stride;
	}

	const uint8* GetData() const { return reinterpret_cast<const uint8*>(Data.GetData()); }

	int32 NumTriangles() const
	{
		return (Num() / 3);
	}

	void Empty(int32 Slack = 0)
	{
		Data.Empty(Slack * Stride);
	}

	int32 Add(uint32 NewIndex)
	{
		int32 Index = Data.Num();
		Data.AddUninitialized(Stride);

		if (bIsUsing32BitIndices)
		{
			*((uint32*)&Data[Index]) = NewIndex;
		}
		else
		{
			*((uint16*)&Data[Index]) = NewIndex;
		}

		return Index / Stride;
	}

	int32 AddTriangle(uint32 NewIndexA, uint32 NewIndexB, uint32 NewIndexC)
	{
		int32 Index = Data.Num();
		Data.AddUninitialized(Stride * 3);

		if (bIsUsing32BitIndices)
		{
			*((uint32*)&Data[Index]) = NewIndexA;
			Index += Stride;
			*((uint32*)&Data[Index]) = NewIndexB;
			Index += Stride;
			*((uint32*)&Data[Index]) = NewIndexC;
		}
		else
		{
			*((uint16*)&Data[Index]) = NewIndexA;
			Index += Stride;
			*((uint16*)&Data[Index]) = NewIndexB;
			Index += Stride;
			*((uint16*)&Data[Index]) = NewIndexC;
		}

		return Index / Stride;
	}

	void Append(const FRuntimeMeshTriangleStream& InOther)
	{
		Data.Append(InOther.Data);
	}

	void Append(const TArray<int32>& InTriangles)
	{
		int32 StartIndex = Num();

		if (bIsUsing32BitIndices)
		{
			Data.SetNum(StartIndex + sizeof(int32) * InTriangles.Num());
			FMemory::Memcpy(Data.GetData() + StartIndex, InTriangles.GetData(), InTriangles.Num() * sizeof(int32));
		}
		else
		{
			SetNum(Num() + InTriangles.Num());
			for (int32 Index = 0; Index < InTriangles.Num(); Index++)
			{
				SetVertexIndex(StartIndex + Index, InTriangles[Index]);
			}
		}
	}

	void Append(const TArray<uint16>& InTriangles)
	{
		int32 StartIndex = Num();

		if (!bIsUsing32BitIndices)
		{
			Data.SetNum(StartIndex + sizeof(uint16) * InTriangles.Num());
			FMemory::Memcpy(Data.GetData() + StartIndex, InTriangles.GetData(), InTriangles.Num() * sizeof(uint16));
		}
		else
		{
			SetNum(Num() + InTriangles.Num());
			for (int32 Index = 0; Index < InTriangles.Num(); Index++)
			{
				SetVertexIndex(StartIndex + Index, InTriangles[Index]);
			}
		}
	}

	uint32 GetVertexIndex(int32 Index) const
	{
		if (bIsUsing32BitIndices)
		{
			return *((uint32*)&Data[Index * Stride]);
		}
		else
		{
			return *((uint16*)&Data[Index * Stride]);
		}
	}

	void SetVertexIndex(int32 Index, uint32 NewIndex)
	{
		if (bIsUsing32BitIndices)
		{
			*((uint32*)&Data[Index * Stride]) = NewIndex;
		}
		else
		{
			*((uint16*)&Data[Index * Stride]) = NewIndex;
		}
	}

	bool Serialize(FArchive& Ar)
	{
		Ar << bIsUsing32BitIndices;
		Ar << Stride;
		Ar << Data;
		return true;
	}
};

template<> struct TStructOpsTypeTraits<FRuntimeMeshTriangleStream> : public TStructOpsTypeTraitsBase2<FRuntimeMeshTriangleStream>
{
	enum
	{
		WithSerializer = true
	};
};


USTRUCT(BlueprintType)
struct RUNTIMEMESHCOMPONENT_API FRuntimeMeshSectionProperties
{
	GENERATED_USTRUCT_BODY();
public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeMesh|Rendering|SectionProperties")
	ERuntimeMeshUpdateFrequency UpdateFrequency;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeMesh|Rendering|SectionProperties")
	int32 MaterialSlot;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeMesh|Rendering|SectionProperties")
	bool bIsVisible;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeMesh|Rendering|SectionProperties")
	bool bCastsShadow;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeMesh|Rendering|SectionProperties")
	bool bUseHighPrecisionTangents;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeMesh|Rendering|SectionProperties")
	bool bUseHighPrecisionTexCoords;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeMesh|Rendering|SectionProperties")
	uint8 NumTexCoords;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeMesh|Rendering|SectionProperties")
	bool bWants32BitIndices;

	FRuntimeMeshSectionProperties();

	friend FArchive& operator<<(FArchive& Ar, FRuntimeMeshSectionProperties& Properties)
	{
		Ar << Properties.UpdateFrequency;
		Ar << Properties.MaterialSlot;

		Ar << Properties.bIsVisible;
		Ar << Properties.bCastsShadow;

		Ar << Properties.bUseHighPrecisionTangents;
		Ar << Properties.bUseHighPrecisionTexCoords;
		Ar << Properties.NumTexCoords;

		Ar << Properties.bWants32BitIndices;

		return Ar;
	}

};

USTRUCT(BlueprintType)
struct RUNTIMEMESHCOMPONENT_API FRuntimeMeshLODProperties
{
	GENERATED_USTRUCT_BODY();
public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeMesh|Rendering|LODProperties")
	float ScreenSize;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeMesh|Rendering|LODProperties")
	bool bCanGetSectionsIndependently;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeMesh|Rendering|LODProperties")
	bool bCanGetAllSectionsAtOnce;

	FRuntimeMeshLODProperties()
		: ScreenSize(1.0f)
		, bCanGetSectionsIndependently(true)
		, bCanGetAllSectionsAtOnce(false)
	{

	}

	friend FArchive& operator<<(FArchive& Ar, FRuntimeMeshLODProperties& Properties)
	{
		Ar << Properties.ScreenSize;
		Ar << Properties.bCanGetSectionsIndependently;
		Ar << Properties.bCanGetAllSectionsAtOnce;

		return Ar;
	}

};

struct RUNTIMEMESHCOMPONENT_API FRuntimeMeshLOD
{
	FRuntimeMeshLODProperties Properties;
	TMap<int32, FRuntimeMeshSectionProperties> Sections;
};

USTRUCT(BlueprintType)
struct RUNTIMEMESHCOMPONENT_API FRuntimeMeshMaterialSlot
{
	GENERATED_USTRUCT_BODY();
public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeMesh|Rendering|MaterialSlot")
	FName SlotName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeMesh|Rendering|MaterialSlot")
	UMaterialInterface* Material;

	FRuntimeMeshMaterialSlot() { }

	FRuntimeMeshMaterialSlot(const FName& InSlotName, UMaterialInterface* InMaterial)
		: SlotName(InSlotName), Material(InMaterial) { }


	friend FArchive& operator<<(FArchive& Ar, FRuntimeMeshMaterialSlot& Slot)
	{
		Ar << Slot.SlotName;
		Ar << Slot.Material;
		return Ar;
	}

};

USTRUCT(BlueprintType, Meta=(DontUseGenericSpawnObject))
struct RUNTIMEMESHCOMPONENT_API FRuntimeMeshRenderableMeshData
{
	GENERATED_USTRUCT_BODY();

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RuntimeMesh|Rendering|MeshData")
	FRuntimeMeshVertexPositionStream Positions;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RuntimeMesh|Rendering|MeshData")
	FRuntimeMeshVertexTangentStream Tangents;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RuntimeMesh|Rendering|MeshData")
	FRuntimeMeshVertexTexCoordStream TexCoords;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RuntimeMesh|Rendering|MeshData")
	FRuntimeMeshVertexColorStream Colors;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RuntimeMesh|Rendering|MeshData")
	FRuntimeMeshTriangleStream Triangles;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RuntimeMesh|Rendering|MeshData")
	FRuntimeMeshTriangleStream AdjacencyTriangles;

	FRuntimeMeshRenderableMeshData()
		: Tangents(false), TexCoords(1, false), Triangles(false), AdjacencyTriangles(false)
	{

	}
	FRuntimeMeshRenderableMeshData(FRuntimeMeshSectionProperties SectionProps)
		: Tangents(SectionProps.bUseHighPrecisionTangents), TexCoords(SectionProps.NumTexCoords, SectionProps.bUseHighPrecisionTexCoords)
		, Triangles(SectionProps.bWants32BitIndices), AdjacencyTriangles(SectionProps.bWants32BitIndices)
	{

	}
	FRuntimeMeshRenderableMeshData(bool bWantsHighPrecisionTangents, bool bWantsHighPrecisionTexCoords, uint8 NumTexCoords, bool bWants32BitIndices)
		: Tangents(bWantsHighPrecisionTangents), TexCoords(NumTexCoords, bWantsHighPrecisionTexCoords)
		, Triangles(bWants32BitIndices), AdjacencyTriangles(bWants32BitIndices)
	{

	}

	void Reset()
	{
		Positions.Empty();
		Tangents.Empty();
		TexCoords.Empty();
		Colors.Empty();
		Triangles.Empty();
		AdjacencyTriangles.Empty();
	}

	bool HasValidMeshData(bool bPrintErrorMessage = false) const;


	friend FArchive& operator<<(FArchive& Ar, FRuntimeMeshRenderableMeshData& MeshData)
	{
		MeshData.Positions.Serialize(Ar);
		MeshData.Tangents.Serialize(Ar);
		MeshData.TexCoords.Serialize(Ar);
		MeshData.Colors.Serialize(Ar);

		MeshData.Triangles.Serialize(Ar);
		MeshData.AdjacencyTriangles.Serialize(Ar);

		return Ar;
	}
};

