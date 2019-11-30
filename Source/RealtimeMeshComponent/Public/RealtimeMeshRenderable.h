// Copyright 2016-2019 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "RealtimeMeshCore.h"
#include "Containers/StaticArray.h"
#include "Interfaces/Interface_CollisionDataProvider.h"

struct FRealtimeMeshVertexPositionStream
{
private:
	TArray<uint8> Data;

public:
	FRealtimeMeshVertexPositionStream() { }

	void SetNum(int32 NewNum, bool bAllowShrinking)
	{
		Data.SetNum(NewNum * sizeof(FVector), bAllowShrinking);
	}

	int32 Num() const
	{
		return Data.Num() / sizeof(FVector);
	}

	const uint8* GetData() const { return reinterpret_cast<const uint8*>(Data.GetData()); }

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

	void Append(const FRealtimeMeshVertexPositionStream& InOther)
	{
		Data.Append(InOther.Data);
	}

	const FVector& GetPosition(int32 Index) const
	{
		return *((FVector*)&Data[Index * sizeof(FVector)]);
	}

	void SetPosition(int32 Index, const FVector& NewPosition)
	{
		*((FVector*)&Data[Index * sizeof(FVector)]) = NewPosition;
	}


	friend FArchive& operator <<(FArchive& Ar, FRealtimeMeshVertexPositionStream& Stream)
	{
		Ar << Stream.Data;
	}
};

struct FRealtimeMeshVertexTangentStream
{
	/*
		This is always laid out tangent first, normal second, and it's either FPackedNormal for normal precision or FPackedRGBA16N for high precision.
	*/

private:
	TArray<uint8> Data;
	bool bIsHighPrecision;

	int32 GetElementSize() const { return (bIsHighPrecision ? sizeof(FPackedRGBA16N) : sizeof(FPackedNormal)); }
	int32 GetStride() const { return GetElementSize() * 2; }

public:
	FRealtimeMeshVertexTangentStream(bool bInUseHighPrecisionTangentBasis = false)
		: bIsHighPrecision(bInUseHighPrecisionTangentBasis)
	{
	}

	void SetNum(int32 NewNum, bool bAllowShrinking)
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

	void Append(const FRealtimeMeshVertexTangentStream& InOther)
	{
		Data.Append(InOther.Data);
	}

	const FVector& GetNormal(int32 Index) const
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

	const FVector& GetTangent(int32 Index) const
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
			FPackedRGBA16N TempTangentX = *((FPackedRGBA16N*)&Data[Index * sizeof(FPackedRGBA16N)]);
			FPackedRGBA16N TempTangentZ = *((FPackedRGBA16N*)&Data[(Index + 1) * sizeof(FPackedRGBA16N)]);
			OutTangentX = TempTangentX.ToFVector();
			OutTangentY = GenerateYAxis(TempTangentX, TempTangentZ);
			OutTangentZ = TempTangentZ.ToFVector();
		}
		else
		{
			FPackedNormal TempTangentX = *((FPackedNormal*)&Data[Index * sizeof(FPackedNormal)]);
			FPackedNormal TempTangentZ = *((FPackedNormal*)&Data[(Index + 1) * sizeof(FPackedNormal)]);
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
			*((FPackedRGBA16N*)&Data[Index * sizeof(FPackedRGBA16N)]) = FPackedRGBA16N(InTangentX);
			*((FPackedRGBA16N*)&Data[(Index + 1) * sizeof(FPackedRGBA16N)]) = FPackedRGBA16N(FVector4(InTangentZ, GetBasisDeterminantSign(InTangentX, InTangentY, InTangentZ)));
		}
		else
		{
			*((FPackedNormal*)&Data[Index * sizeof(FPackedNormal)]) = FPackedNormal(InTangentX);
			*((FPackedNormal*)&Data[(Index + 1) * sizeof(FPackedNormal)]) = FPackedNormal(FVector4(InTangentZ, GetBasisDeterminantSign(InTangentX, InTangentY, InTangentZ)));
		}
	}


	friend FArchive& operator <<(FArchive& Ar, FRealtimeMeshVertexTangentStream& Stream)
	{
		Ar << Stream.bIsHighPrecision;
		Ar << Stream.Data;
	}
};

struct FRealtimeMeshVertexTexCoordStream
{
private:
	TArray<uint8> Data;
	int32 ChannelCount;
	bool bIsHighPrecision;

	int32 GetElementSize() const { return (bIsHighPrecision ? sizeof(FVector2D) : sizeof(FVector2DHalf)); }
	int32 GetStride() const { return GetElementSize() * ChannelCount; }

public:
	FRealtimeMeshVertexTexCoordStream(int32 InChannelCount = 1, bool bInShouldUseHighPrecision = false)
		: ChannelCount(InChannelCount), bIsHighPrecision(bInShouldUseHighPrecision)
	{
	}

	void SetNum(int32 NewNum, bool bAllowShrinking)
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

	int32 Add(const FVector2D& InTexCoord)
	{
		int32 Index = Data.Num();
		int32 Stride = GetStride();
		Data.AddUninitialized(Stride);

		if (bIsHighPrecision)
		{
			static const int32 ElementSize = sizeof(FVector2D);
			*((FVector2D*)&Data[Index]) = InTexCoord;
		}
		else
		{
			static const int32 ElementSize = sizeof(FVector2DHalf);
			*((FVector2DHalf*)&Data[Index]) = InTexCoord;
		}

		return Index / Stride;
	}

	void Append(const FRealtimeMeshVertexTexCoordStream& InOther)
	{
		Data.Append(InOther.Data);
	}

	const FVector2D& GetTexCoord(int32 Index, int32 ChannelId = 0) const
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

	friend FArchive& operator <<(FArchive& Ar, FRealtimeMeshVertexTexCoordStream& Stream)
	{
		Ar << Stream.bIsHighPrecision;
		Ar << Stream.ChannelCount;
		Ar << Stream.Data;
	}
};

struct FRealtimeMeshVertexColorStream
{
private:
	TArray<uint8> Data;

public:
	FRealtimeMeshVertexColorStream() { }

	void SetNum(int32 NewNum, bool bAllowShrinking)
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

	void Append(const FRealtimeMeshVertexColorStream& InOther)
	{
		Data.Append(InOther.Data);
	}

	const FColor& GetColor(int32 Index) const
	{
		return *((FColor*)&Data[Index * sizeof(FColor)]);
	}

	void SetColor(int32 Index, const FColor& NewPosition)
	{
		*((FColor*)&Data[Index * sizeof(FColor)]) = NewPosition;
	}

	friend FArchive& operator <<(FArchive& Ar, FRealtimeMeshVertexColorStream& Stream)
	{
		Ar << Stream.Data;
	}
};

struct FRealtimeMeshTriangleStream
{
private:
	TArray<uint8> Data;
	bool bIsUsing32BitIndices;
	uint8 Stride;

public:
	FRealtimeMeshTriangleStream(bool bInUse32BitIndices = false)
		: bIsUsing32BitIndices(bInUse32BitIndices), Stride(bInUse32BitIndices ? sizeof(uint32) : sizeof(uint16))
	{

	}

	void SetNum(int32 NewNum, bool bAllowShrinking)
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

	void Append(const FRealtimeMeshTriangleStream& InOther)
	{
		Data.Append(InOther.Data);
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

	friend FArchive& operator <<(FArchive& Ar, FRealtimeMeshTriangleStream& Stream)
	{
		Ar << Stream.bIsUsing32BitIndices;
		Ar << Stream.Stride;
		Ar << Stream.Data;
	}
};



struct FRealtimeMeshSectionProperties
{
	ERealtimeMeshUpdateFrequency UpdateFrequency;

	int32 MaterialSlot;

	bool bIsVisible;
	bool bCastsShadow;

	bool bUseHighPrecisionTangents;
	bool bUseHighPrecisionTexCoords;
	uint8 NumTexCoords;

	bool bWants32BitIndices;

	FRealtimeMeshSectionProperties()
		: UpdateFrequency(ERealtimeMeshUpdateFrequency::Infrequent)
		, bIsVisible(true)
		, bCastsShadow(true)
		, bUseHighPrecisionTangents(false)
		, bUseHighPrecisionTexCoords(false)
		, NumTexCoords(1)
		, bWants32BitIndices(false)
	{

	}



	// 	friend FArchive& operator <<(FArchive& Ar, FRealtimeMeshSectionProperties& SectionProps)
	// 	{
	// 		//Ar << SectionProps.UpdateFrequency;
	// 
	// 		Ar << SectionProps.MaterialSlot;
	// 		
	// 		Ar << SectionProps.bIsVisible;
	// 		Ar << SectionProps.bCastsShadow;
	// 
	// 		Ar << SectionProps.bUseHighPrecisionTangents;
	// 		Ar << SectionProps.bUseHighPrecisionTexCoords;
	// 		Ar << SectionProps.NumTexCoords;
	// 
	// 		Ar << SectionProps.bWants32BitIndices;
	// 	}
};

struct FRealtimeMeshLODProperties
{
	float ScreenSize;

	FRealtimeMeshLODProperties()
		: ScreenSize(0.0)
	{

	}

	// 	friend FArchive& operator <<(FArchive& Ar, FRealtimeMeshLODProperties& LODProps)
	// 	{
	// 		Ar << LODProps.ScreenSize;
	// 	}
};

struct FRealtimeMeshLOD
{
	FRealtimeMeshLODProperties Properties;
	TMap<int32, FRealtimeMeshSectionProperties> Sections;

	// 	friend FArchive& operator <<(FArchive& Ar, FRealtimeMeshLOD& LOD)
	// 	{
	// 		Ar << LOD.Properties;
	// 		Ar << LOD.Sections;
	// 	}
};

struct FRealtimeMeshMaterialSlot
{
	FName SlotName;
	TWeakObjectPtr<UMaterialInterface> Material;

	FRealtimeMeshMaterialSlot() { }

	FRealtimeMeshMaterialSlot(const FName& InSlotName, const TWeakObjectPtr<UMaterialInterface>& InMaterial)
		: SlotName(InSlotName), Material(InMaterial) { }

	// 	friend FArchive& operator <<(FArchive& Ar, FRealtimeMeshMaterialSlot& MaterialSlot)
	// 	{
	// 		Ar << MaterialSlot.SlotName;
	// 		//Ar << MaterialSlot.Material;
	// 	}
};

struct FRealtimeMeshRenderableMeshData
{
	FRealtimeMeshVertexPositionStream Positions;
	FRealtimeMeshVertexTangentStream Tangents;
	FRealtimeMeshVertexTexCoordStream TexCoords;
	FRealtimeMeshVertexColorStream Colors;

	FRealtimeMeshTriangleStream Triangles;
	FRealtimeMeshTriangleStream AdjacencyTriangles;

	FRealtimeMeshRenderableMeshData(bool bWantsHighPrecisionTangents, bool bWantsHighPrecisionTexCoords, uint8 NumTexCoords, bool bWants32BitIndices)
		: Tangents(bWantsHighPrecisionTangents), TexCoords(NumTexCoords, bWantsHighPrecisionTexCoords)
		, Triangles(bWants32BitIndices), AdjacencyTriangles(bWants32BitIndices)
	{

	}

	// 	friend FArchive& operator <<(FArchive& Ar, FRealtimeMeshRenderableMeshData& RenderableMeshData)
	// 	{
	// 		Ar << RenderableMeshData.Positions;
	// 		Ar << RenderableMeshData.Tangents;
	// 		Ar << RenderableMeshData.TexCoords;
	// 		Ar << RenderableMeshData.Colors;
	// 
	// 		Ar << RenderableMeshData.Triangles;
	// 		Ar << RenderableMeshData.AdjacencyTriangles;
	// 	}
};
