// Copyright 2016-2019 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "RuntimeMeshCore.h"
#include "Containers/StaticArray.h"
#include "Interfaces/Interface_CollisionDataProvider.h"

struct FRuntimeMeshVertexPositionStream
{
private:
	TArray<uint8> Data;

public:
	FRuntimeMeshVertexPositionStream() { }

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

	void Append(const FRuntimeMeshVertexPositionStream& InOther)
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


	friend FArchive& operator <<(FArchive& Ar, FRuntimeMeshVertexPositionStream& Stream)
	{
		Ar << Stream.Data;
	}
};

struct FRuntimeMeshVertexTangentStream
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
	FRuntimeMeshVertexTangentStream(bool bInUseHighPrecisionTangentBasis = false)
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

	void Append(const FRuntimeMeshVertexTangentStream& InOther)
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


	friend FArchive& operator <<(FArchive& Ar, FRuntimeMeshVertexTangentStream& Stream)
	{
		Ar << Stream.bIsHighPrecision;
		Ar << Stream.Data;
	}
};

struct FRuntimeMeshVertexTexCoordStream
{
private:
	TArray<uint8> Data;
	int32 ChannelCount;
	bool bIsHighPrecision;

	int32 GetElementSize() const { return (bIsHighPrecision ? sizeof(FVector2D) : sizeof(FVector2DHalf)); }
	int32 GetStride() const { return GetElementSize() * ChannelCount; }

public:
	FRuntimeMeshVertexTexCoordStream(int32 InChannelCount = 1, bool bInShouldUseHighPrecision = false)
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

	void Append(const FRuntimeMeshVertexTexCoordStream& InOther)
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

	friend FArchive& operator <<(FArchive& Ar, FRuntimeMeshVertexTexCoordStream& Stream)
	{
		Ar << Stream.bIsHighPrecision;
		Ar << Stream.ChannelCount;
		Ar << Stream.Data;
	}
};

struct FRuntimeMeshVertexColorStream
{
private:
	TArray<uint8> Data;

public:
	FRuntimeMeshVertexColorStream() { }

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

	void Append(const FRuntimeMeshVertexColorStream& InOther)
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

	friend FArchive& operator <<(FArchive& Ar, FRuntimeMeshVertexColorStream& Stream)
	{
		Ar << Stream.Data;
	}
};

struct FRuntimeMeshTriangleStream
{
private:
	TArray<uint8> Data;
	bool bIsUsing32BitIndices;
	uint8 Stride;

public:
	FRuntimeMeshTriangleStream(bool bInUse32BitIndices = false)
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

	void Append(const FRuntimeMeshTriangleStream& InOther)
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

	friend FArchive& operator <<(FArchive& Ar, FRuntimeMeshTriangleStream& Stream)
	{
		Ar << Stream.bIsUsing32BitIndices;
		Ar << Stream.Stride;
		Ar << Stream.Data;
	}
};



struct FRuntimeMeshSectionProperties
{
	ERuntimeMeshUpdateFrequency UpdateFrequency;

	int32 MaterialSlot;

	bool bIsVisible;
	bool bCastsShadow;

	bool bUseHighPrecisionTangents;
	bool bUseHighPrecisionTexCoords;
	uint8 NumTexCoords;

	bool bWants32BitIndices;

	FRuntimeMeshSectionProperties()
		: UpdateFrequency(ERuntimeMeshUpdateFrequency::Infrequent)
		, bIsVisible(true)
		, bCastsShadow(true)
		, bUseHighPrecisionTangents(false)
		, bUseHighPrecisionTexCoords(false)
		, NumTexCoords(1)
		, bWants32BitIndices(false)
	{

	}



	// 	friend FArchive& operator <<(FArchive& Ar, FRuntimeMeshSectionProperties& SectionProps)
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

struct FRuntimeMeshLODProperties
{
	float ScreenSize;

	FRuntimeMeshLODProperties()
		: ScreenSize(0.0)
	{

	}

	// 	friend FArchive& operator <<(FArchive& Ar, FRuntimeMeshLODProperties& LODProps)
	// 	{
	// 		Ar << LODProps.ScreenSize;
	// 	}
};

struct FRuntimeMeshLOD
{
	FRuntimeMeshLODProperties Properties;
	TMap<int32, FRuntimeMeshSectionProperties> Sections;

	// 	friend FArchive& operator <<(FArchive& Ar, FRuntimeMeshLOD& LOD)
	// 	{
	// 		Ar << LOD.Properties;
	// 		Ar << LOD.Sections;
	// 	}
};

struct FRuntimeMeshMaterialSlot
{
	FName SlotName;
	TWeakObjectPtr<UMaterialInterface> Material;

	FRuntimeMeshMaterialSlot() { }

	FRuntimeMeshMaterialSlot(const FName& InSlotName, const TWeakObjectPtr<UMaterialInterface>& InMaterial)
		: SlotName(InSlotName), Material(InMaterial) { }

	// 	friend FArchive& operator <<(FArchive& Ar, FRuntimeMeshMaterialSlot& MaterialSlot)
	// 	{
	// 		Ar << MaterialSlot.SlotName;
	// 		//Ar << MaterialSlot.Material;
	// 	}
};

struct FRuntimeMeshRenderableMeshData
{
	FRuntimeMeshVertexPositionStream Positions;
	FRuntimeMeshVertexTangentStream Tangents;
	FRuntimeMeshVertexTexCoordStream TexCoords;
	FRuntimeMeshVertexColorStream Colors;

	FRuntimeMeshTriangleStream Triangles;
	FRuntimeMeshTriangleStream AdjacencyTriangles;

	FRuntimeMeshRenderableMeshData(bool bWantsHighPrecisionTangents, bool bWantsHighPrecisionTexCoords, uint8 NumTexCoords, bool bWants32BitIndices)
		: Tangents(bWantsHighPrecisionTangents), TexCoords(NumTexCoords, bWantsHighPrecisionTexCoords)
		, Triangles(bWants32BitIndices), AdjacencyTriangles(bWants32BitIndices)
	{

	}

	// 	friend FArchive& operator <<(FArchive& Ar, FRuntimeMeshRenderableMeshData& RenderableMeshData)
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
