// Copyright 2016-2020 TriAxis Games L.L.C. All Rights Reserved.

#pragma once

#include "RuntimeMeshCore.h"
#include "Containers/StaticArray.h"
#include "Interfaces/Interface_CollisionDataProvider.h"
#include "RuntimeMeshRenderable.generated.h"

USTRUCT(BlueprintType)
struct RUNTIMEMESHCOMPONENT_API FRuntimeMeshVertexPositionStream 
{
	GENERATED_BODY();

private:
	TArray<uint8> Data;

public:
	FRuntimeMeshVertexPositionStream()
	{ }

	FORCEINLINE static int32 GetStride() { return sizeof(FVector3f) / sizeof(uint8); }

	//Number of positions stored inside this stream
	FORCEINLINE int32 Num() const
	{
		return Data.Num() / GetStride();
	}

	//Resize this stream to store NewNum number of positions
	void SetNum(int32 NewNum, bool bAllowShrinking = true)
	{
		Data.SetNum(NewNum * GetStride(), bAllowShrinking);
	}
	
	void Reserve(int32 Number)
	{
		Data.Reserve(Number * GetStride());
	}
	
	void Empty(int32 Slack = 0)
	{
		Data.Empty(Slack * GetStride());
	}
	
	void SetFPosition(int32 Index, const FVector3f& NewPosition)
	{
		*((FVector3f*)&Data[Index*GetStride()]) = NewPosition;
	}

	UE_DEPRECATED(5.0, "Use SetFPosition instead.")
	void SetPosition(int32 Index, const FVector& NewPosition)
	{
		SetFPosition(Index, FVector3f(NewPosition));
	}

	int32 AddF(const FVector3f& InPosition)
	{
		const int32 Index = Num();
		Data.AddUninitialized(GetStride());
		SetFPosition(Index, InPosition);
		return Index;
	}

	UE_DEPRECATED(5.0, "Use AddF instead.")
	int32 Add(const FVector& InPosition)
	{
		return AddF(FVector3f(InPosition));
	}
	
	void Append(const FRuntimeMeshVertexPositionStream& InOther)
	{
		Data.Append(InOther.Data);
	}
	
	int AppendF(const TArray<FVector3f>& InPositions)
	{
		int32 StartIndex = Data.Num();
		Data.SetNum(StartIndex + GetStride() * InPositions.Num());
		FMemory::Memcpy(Data.GetData() + StartIndex, InPositions.GetData(), InPositions.Num() * GetStride());
		return StartIndex;
	}

	UE_DEPRECATED(5.0, "Use AppendF instead.")
	int Append(const TArray<FVector>& InPositions)
	{
		TArray<FVector3f> InPositions2;
		InPositions2.SetNum(InPositions.Num());
		for (int i = 0; i < InPositions.Num(); ++i)
		{
			InPositions2[i] = FVector3f(InPositions[i]);
		}
		return AppendF(InPositions2);
	}

	const FVector3f& GetFPosition(int32 Index) const
	{
		return *((FVector3f*)&Data[Index * GetStride()]);
	}

	UE_DEPRECATED(5.0, "Use GetFPosition instead.")
	const FVector& GetPosition(int32 Index) const
	{
		return FVector(GetFPosition(Index));
	}
	
	FBox3f GetFBounds() const
	{
		FBox3f NewBox(ForceInit);
		const int32 Count = Num();
		for (int32 Index = 0; Index < Count; Index++)
		{
			NewBox += GetFPosition(Index);
		}
		return NewBox;
	}

	UE_DEPRECATED(5.0, "Use GetFBounds instead.")
	FBox GetBounds() const
	{
		return FBox(GetFBounds());
	}

	const uint8* GetData() const { return Data.GetData(); }
	
	TArray<uint8>&& TakeData()&& { return MoveTemp(Data); }
	
	const TArray<FVector3f> GetFCopy() const
	{
		TArray<FVector3f> OutData;
		OutData.SetNum(Num());
		FMemory::Memcpy(OutData.GetData(), Data.GetData(), Data.Num());
		return OutData;
	}

	UE_DEPRECATED(5.0, "Use GetFCopy instead.")
	const TArray<FVector> GetCopy() const
	{
		TArray<FVector3f> OutData3f = GetFCopy();
		TArray<FVector> OutData; OutData.SetNum(OutData3f.Num());
		//cast to fvector
		for (int i = 0; i < OutData3f.Num(); ++i)
		{
			OutData[i] = FVector(OutData3f[i]);
		}
		return OutData;
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

private:
	// This is always laid out tangent first, normal second, and it's 
	// either FPackedNormal for normal precision or FPackedRGBA16N for high precision.
	TArray<uint8> Data;
	bool bIsHighPrecision;

public:
	FRuntimeMeshVertexTangentStream(bool bInUseHighPrecisionTangentBasis = false)
		: bIsHighPrecision(bInUseHighPrecisionTangentBasis)
	{
	}

	FORCEINLINE bool IsHighPrecision() const { return bIsHighPrecision; }

	FORCEINLINE int32 GetElementSize() const { return (bIsHighPrecision ? sizeof(FPackedRGBA16N) : sizeof(FPackedNormal)); }
	FORCEINLINE int32 GetStride() const { return GetElementSize() * 2; }

	FORCEINLINE int32 Num() const
	{
		return Data.Num() / GetStride();
	}
	void SetNum(int32 NewNum, bool bAllowShrinking = true)
	{
		Data.SetNum(NewNum * GetStride(), bAllowShrinking);
	}
	void Reserve(int32 Number)
	{
		Data.Reserve(Number * GetStride());
	}
	void Empty(int32 Slack = 0)
	{
		Data.Empty(Slack * GetStride());
	}

	int32 AddF(const FVector3f& InNormal, const FVector3f& InTangent)
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

	UE_DEPRECATED(5.0, "Use AddF instead.")
	int32 Add(const FVector& InNormal, const FVector& InTangent)
	{
		return AddF(FVector3f(InNormal), FVector3f(InTangent));
	}
	
	int32 AddF(const FVector4f& InNormal, const FVector3f& InTangent)
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

	UE_DEPRECATED(5.0, "Use AddF instead.")
	int32 Add(const FVector4& InNormal, const FVector& InTangent)
	{
		return AddF(FVector4f(InNormal), FVector3f(InTangent));
	}
	
	int32 AddF(const FVector3f& InTangentX, const FVector3f& InTangentY, const FVector3f& InTangentZ)
	{
		int32 Index = Data.Num();
		int32 Stride = GetStride();
		Data.AddUninitialized(Stride);
		if (bIsHighPrecision)
		{
			*((FPackedRGBA16N*)&Data[Index]) = FPackedRGBA16N(InTangentX);
			*((FPackedRGBA16N*)&Data[Index + sizeof(FPackedRGBA16N)]) = FPackedRGBA16N(FVector4f(InTangentZ, GetBasisDeterminantSign(InTangentX, InTangentY, InTangentZ)));
		}
		else
		{
			*((FPackedNormal*)&Data[Index]) = FPackedNormal(InTangentX);
			*((FPackedNormal*)&Data[Index + sizeof(FPackedNormal)]) = FPackedNormal(FVector4f(InTangentZ, GetBasisDeterminantSign(InTangentX, InTangentY, InTangentZ)));
		}
		return Index / Stride;
	}

	UE_DEPRECATED(5.0, "Use AddF instead.")
	int32 Add(const FVector& InTangentX, const FVector& InTangentY, const FVector& InTangentZ)
	{
		return AddF(FVector3f(InTangentX), FVector3f(InTangentY), FVector3f(InTangentZ));
	}

	//F version for consistency
	void AppendF(const FRuntimeMeshVertexTangentStream& InOther)
	{
		Data.Append(InOther.Data);
	}

	void Append(const FRuntimeMeshVertexTangentStream& InOther)
	{
		Data.Append(InOther.Data);
	}
	
	void AppendF(const TArray<FVector3f>& InNormals, const TArray<FRuntimeMeshTangent>& InTangents)
	{
		int32 MaxCount = FMath::Max(InNormals.Num(), InTangents.Num());

		for (int32 Index = 0; Index < MaxCount; Index++)
		{
			const int32 NormalIndex = FMath::Min(Index, InNormals.Num());
			const int32 TangentIndex = FMath::Min(Index, InTangents.Num());

			FVector4f Normal;
			FVector3f Tangent;

			if (InNormals.IsValidIndex(NormalIndex))
			{
				Normal = InNormals[NormalIndex];
			}
			else
			{
				Normal = FVector3f(0, 0, 1);
			}

			if (InTangents.IsValidIndex(TangentIndex))
			{
				Tangent = InTangents[TangentIndex].TangentX;
				Normal.W = InTangents[TangentIndex].bFlipTangentY ? -1 : 1;
			}
			else
			{
				Tangent = FVector3f(1, 0, 0);
				Normal.W = 1.0f;
			}

			AddF(Normal, Tangent);
		}
	}

	UE_DEPRECATED(5.0, "Use AppendF instead.")
	void Append(const TArray<FVector>& InNormals, const TArray<FRuntimeMeshTangent>& InTangents)
	{
		TArray<FVector3f> InNormals3f(InNormals);
		AppendF(InNormals3f, InTangents);
	}

	void SetFNormal(int32 Index, const FVector3f& NewNormal)
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

	UE_DEPRECATED(5.0, "Use SetFNormal instead.")
	void SetNormal(int32 Index, const FVector& NewNormal)
	{
		SetFNormal(Index, FVector3f(NewNormal));
	}
	
	void SetFTangent(int32 Index, const FVector3f& NewTangent)
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

	UE_DEPRECATED(5.0, "Use SetFTangent instead.")
	void SetTangent(int32 Index, const FVector& NewTangent)
	{
		SetFNormal(Index, FVector3f(NewTangent));
	}
	
	void SetFTangents(int32 Index, const FVector3f& InTangentX, const FVector3f& InTangentY, const FVector3f& InTangentZ)
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

	UE_DEPRECATED(5.0, "Use SetFTangents instead.")
	void SetTangents(int32 Index, const FVector& InTangentX, const FVector& InTangentY, const FVector& InTangentZ)
	{
		SetFTangents(Index, FVector3f(InTangentX), FVector3f(InTangentY), FVector3f(InTangentZ));
	}

	FVector3f GetFNormal(int32 Index) const
	{
		const int32 EntryIndex = Index * 2 + 1;
		if (bIsHighPrecision)
		{
			return (*((FPackedRGBA16N*)&Data[EntryIndex * sizeof(FPackedRGBA16N)])).ToFVector3f();
		}
		else
		{
			return (*((FPackedNormal*)&Data[EntryIndex * sizeof(FPackedNormal)])).ToFVector3f();
		}
	}

	UE_DEPRECATED(5.0, "Use GetFNormal instead.")
	FVector GetNormal(int32 Index) const
	{
		return FVector(GetFNormal(Index));
	}
	
	FVector3f GetFTangent(int32 Index) const
	{
		const int32 EntryIndex = Index * 2;
		if (bIsHighPrecision)
		{
			return (*((FPackedRGBA16N*)&Data[EntryIndex * sizeof(FPackedRGBA16N)])).ToFVector3f();
		}
		else
		{
			return (*((FPackedNormal*)&Data[EntryIndex * sizeof(FPackedNormal)])).ToFVector3f();
		}
	}

	UE_DEPRECATED(5.0, "Use GetFTangent instead.")
	FVector GetTangent(int32 Index) const
	{
		return FVector(GetFTangent(Index));
	}
	
	void GetFTangents(int32 Index, FVector3f& OutTangentX, FVector3f& OutTangentY, FVector3f& OutTangentZ) const
	{
		const int32 EntryIndex = Index * 2;
		if (bIsHighPrecision)
		{
			FPackedRGBA16N TempTangentX = *((FPackedRGBA16N*)&Data[EntryIndex * sizeof(FPackedRGBA16N)]);
			FPackedRGBA16N TempTangentZ = *((FPackedRGBA16N*)&Data[(EntryIndex + 1) * sizeof(FPackedRGBA16N)]);
			OutTangentX = TempTangentX.ToFVector3f();
			OutTangentY = GenerateYAxisf<FPackedRGBA16N>(TempTangentX, TempTangentZ); //TODO : fix this to use FVector3f as base
			OutTangentZ = TempTangentZ.ToFVector3f();
		}
		else
		{
			FPackedNormal TempTangentX = *((FPackedNormal*)&Data[EntryIndex * sizeof(FPackedNormal)]);
			FPackedNormal TempTangentZ = *((FPackedNormal*)&Data[(EntryIndex + 1) * sizeof(FPackedNormal)]);
			OutTangentX = TempTangentX.ToFVector3f();
			OutTangentY = GenerateYAxisf<FPackedNormal>(TempTangentX, TempTangentZ);
			OutTangentZ = TempTangentZ.ToFVector3f();
		}
	}

	UE_DEPRECATED(5.0, "Use GetFTangents instead.")
	void GetTangents(int32 Index, FVector& OutTangentX, FVector& OutTangentY, FVector& OutTangentZ) const
	{
		FVector3f TX, TY, TZ;
		GetFTangents(Index, TX, TY, TZ);
		OutTangentX = FVector(TX);
		OutTangentY = FVector(TY);
		OutTangentZ = FVector(TZ);
	}

	const uint8* GetData() const { return Data.GetData(); }
	TArray<uint8>&& TakeData()&& { return MoveTemp(Data); }
// 	const TArray<TArray<FVector2D>> GetCopy() const
// 	{
// 		TArray<FVector> OutData;
// 		OutData.SetNum(Num());
// 		FMemory::Memcpy(OutData.GetData(), Data.GetData(), Data.Num());
// 		return OutData;
// 	}

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
	FRuntimeMeshVertexTexCoordStream(int32 InChannelCount = 1, bool bInShouldUseHighPrecision = false)
		: ChannelCount(InChannelCount), bIsHighPrecision(bInShouldUseHighPrecision)
	{
	}

	FORCEINLINE bool IsHighPrecision() const { return bIsHighPrecision; }
	FORCEINLINE int32 NumChannels() const
	{
		return ChannelCount;
	}

	FORCEINLINE int32 GetElementSize() const { return (bIsHighPrecision ? sizeof(FVector2f) : sizeof(FVector2DHalf)); } //size in bytes
	FORCEINLINE int32 GetStride() const { return GetElementSize() * ChannelCount; } //num of bytes of UV per vertex in data

	FORCEINLINE int32 Num() const
	{
		return Data.Num() / GetStride();
	}
	void SetNum(int32 NewNum, bool bAllowShrinking = true)
	{
		Data.SetNum(NewNum * GetStride(), bAllowShrinking);
	}
	void Reserve(int32 Number)
	{
		Data.Reserve(Number * GetStride());
	}
	void Empty(int32 Slack = 0)
	{
		Data.Empty(Slack * GetStride());
	}

	/* Prefer using SetNum and SetTexCoord when you have more than 1 UV
	Add appends a new element to the end of the array, but will only work if you cycle through each UV 
	Example with 3 UVs :
	Add at channel 0
	Add at channel 1
	Add at channel 2
	Repeat
	If you skip any it'll crash UE
	(if you're here because it crashed, told you !)
	*/
	int32 AddF(const FVector2f& InTexCoord, int32 ChannelId = 0)
	{
		int32 Index = Data.Num();
		checkf((Index / GetElementSize()) % ChannelCount == ChannelId, TEXT("[FRuntimeMeshVertexTexCoordStream::Add] UVs have been added out of order, aborting..."));
		Data.AddZeroed(GetElementSize());

		if (bIsHighPrecision)
		{
			static const int32 ElementSize = sizeof(FVector2f);
			*((FVector2f*)&Data[Index]) = InTexCoord;
		}
		else
		{
			static const int32 ElementSize = sizeof(FVector2DHalf);
			*((FVector2DHalf*)&Data[Index]) = InTexCoord;
		}

		return Index / GetStride();
	}

	int32 Add(const FVector2D& InTexCoord, int32 ChannelId = 0)
	{
		return AddF(FVector2f(InTexCoord), ChannelId);
	}

	/* Add a UV at the end of the array
	Current UV Array must end with the last UV (meaning the previous vert must have had all of it's UVs registered)
	Given array must be of the same length as the number of UV channels
	*/
	int32 AddF(const TArray<FVector2f>& InTexCoords)
	{
		const int oldNum = Data.Num();
		const int32 Index = Num();
		const int32 Stride = GetStride();
		checkf(InTexCoords.Num() == ChannelCount, TEXT("[FRuntimeMeshVertexTexCoordStream::Add] Given array of UVs doesn't match UV channel count, aborting..."));
		checkf((oldNum / GetElementSize()) % ChannelCount == 0, TEXT("[FRuntimeMeshVertexTexCoordStream::Add] Current array of UVs didn't end with the last UV channel, aborting..."));
		Data.AddZeroed(Stride);

		if (bIsHighPrecision)
		{
			static const int32 ElementSize = sizeof(FVector2f);

			for (int32 ChannelId = 0; ChannelId < ChannelCount; ChannelId++)
			{
				*((FVector2f*)&Data[(Index * Stride) + (ChannelId * ElementSize)]) = InTexCoords[ChannelId];
			}
		}
		else
		{
			static const int32 ElementSize = sizeof(FVector2DHalf);

			for (int32 ChannelId = 0; ChannelId < ChannelCount; ChannelId++)
			{
				*((FVector2DHalf*)&Data[(Index * Stride) + (ChannelId * ElementSize)]) = InTexCoords[ChannelId];
			}
			
		}

		return Index;
	}

	int32 Add(const TArray<FVector2D>& InTexCoords)
	{
		TArray<FVector2f> InTextCoords2f(InTexCoords);
		return AddF(InTextCoords2f);
	}

	void Append(const FRuntimeMeshVertexTexCoordStream& InOther)
	{
		checkf(InOther.bIsHighPrecision == bIsHighPrecision, TEXT("[FRuntimeMeshVertexTexCoordStream::Append] Tried to merge two UV stream of different precision, aborting... (merge %s into %s)"),
			InOther.bIsHighPrecision ? TEXT("HighPrecision") : TEXT("LowPrecision"),
			bIsHighPrecision ? TEXT("HighPrecision") : TEXT("LowPrecision"));
		Data.Append(InOther.Data);
	}
	
	void AppendF(const FRuntimeMeshVertexTexCoordStream& InOther)
	{
		Append(InOther);
	}
	
	void FillFIn(int32 StartIndex, const TArray<FVector2f>& InChannelData, int32 ChannelId = 0)
	{
		if (Num() < (StartIndex + InChannelData.Num()))
		{
			SetNum(StartIndex + InChannelData.Num());
		}

		for (int32 Index = 0; Index < InChannelData.Num(); Index++)
		{
			SetFTexCoord(StartIndex + Index, InChannelData[Index], ChannelId);
		}
	}

	void FillIn(int32 StartIndex, const TArray<FVector2D>& InChannelData, int32 ChannelId = 0)
	{
		TArray<FVector2f> InChannelData2f(InChannelData);
		FillFIn(StartIndex, InChannelData2f, ChannelId);
	}

	void SetFTexCoord(int32 Index, const FVector2f& NewTexCoord, int32 ChannelId = 0)
	{
		if (bIsHighPrecision)
		{
			static const int32 ElementSize = sizeof(FVector2f);
			*((FVector2f*)&Data[(Index * ElementSize * ChannelCount) + (ChannelId * ElementSize)]) = NewTexCoord;
		}
		else
		{
			static const int32 ElementSize = sizeof(FVector2DHalf);
			*((FVector2DHalf*)&Data[(Index * ElementSize * ChannelCount) + (ChannelId * ElementSize)]) = NewTexCoord;
		}
	}
	
	void SetFTexCoord(int32 Index, const FVector2D& NewTexCoord, int32 ChannelId = 0)
	{
		SetFTexCoord(Index, FVector2f(NewTexCoord), ChannelId);
	}
	
	const FVector2f GetFTexCoord(int32 Index, int32 ChannelId = 0) const
	{
		if (bIsHighPrecision)
		{
			static const int32 ElementSize = sizeof(FVector2f);
			return *((FVector2f*)&Data[(Index * ElementSize * ChannelCount) + (ChannelId * ElementSize)]);
		}
		else
		{
			static const int32 ElementSize = sizeof(FVector2DHalf);
			return *((FVector2DHalf*)&Data[(Index * ElementSize * ChannelCount) + (ChannelId * ElementSize)]);
		}
	}

	const FVector2D GetTexCoord(int32 Index, int32 ChannelId = 0) const
	{
		return FVector2D(GetFTexCoord(Index, ChannelId));
	}

	const uint8* GetData() const { return Data.GetData(); }
	TArray<uint8>&& TakeData()&& { return MoveTemp(Data); }
	// 	const TArray<TArray<FVector2D>> GetCopy() const
	// 	{
	// 		TArray<FVector> OutData;
	// 		OutData.SetNum(Num());
	// 		FMemory::Memcpy(OutData.GetData(), Data.GetData(), Data.Num());
	// 		return OutData;
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

	FORCEINLINE int32 GetStride() const { return sizeof(FColor); }

	FORCEINLINE int32 Num() const
	{
		return Data.Num() / GetStride();
	}
	void SetNum(int32 NewNum, bool bAllowShrinking = true)
	{
		Data.SetNum(NewNum * GetStride(), bAllowShrinking);
	}
	void Reserve(int32 Number)
	{
		Data.Reserve(Number * GetStride());
	}
	void Empty(int32 Slack = 0)
	{
		Data.Empty(Slack * GetStride());
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

	void SetColor(int32 Index, const FColor& NewColor)
	{
		*((FColor*)&Data[Index * sizeof(FColor)]) = NewColor;
	}
	const FColor& GetColor(int32 Index) const
	{
		return *((FColor*)&Data[Index * sizeof(FColor)]);
	}

	const uint8* GetData() const { return Data.GetData(); }
	TArray<uint8>&& TakeData()&& { return MoveTemp(Data); }
	// 	const TArray<TArray<FVector2D>> GetCopy() const
	// 	{
	// 		TArray<FVector> OutData;
	// 		OutData.SetNum(Num());
	// 		FMemory::Memcpy(OutData.GetData(), Data.GetData(), Data.Num());
	// 		return OutData;
	// 	}

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
	FRuntimeMeshTriangleStream(bool bInUse32BitIndices = false)
		: bIsUsing32BitIndices(bInUse32BitIndices), Stride(bInUse32BitIndices ? sizeof(uint32) : sizeof(uint16))
	{

	}

	bool IsHighPrecision() const { return bIsUsing32BitIndices; }

	FORCEINLINE int32 GetStride() const { return Stride; }

	FORCEINLINE int32 Num() const
	{
		return Data.Num() / GetStride();
	}
	int32 NumTriangles() const
	{
		return (Num() / 3);
	}
	void SetNum(int32 NewNum, bool bAllowShrinking = true)
	{
		Data.SetNum(NewNum * GetStride(), bAllowShrinking);
	}
	void Reserve(int32 Number)
	{
		Data.Reserve(Number * GetStride());
	}
	void Empty(int32 Slack = 0)
	{
		Data.Empty(Slack * GetStride());
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

	const uint8* GetData() const { return Data.GetData(); }
	TArray<uint8>&& TakeData()&& { return MoveTemp(Data); }
	// 	const TArray<TArray<FVector2D>> GetCopy() const
	// 	{
	// 		TArray<FVector> OutData;
	// 		OutData.SetNum(Num());
	// 		FMemory::Memcpy(OutData.GetData(), Data.GetData(), Data.Num());
	// 		return OutData;
	// 	}

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
	bool bIsMainPassRenderable;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeMesh|Rendering|SectionProperties")
	bool bCastsShadow;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeMesh|Rendering|SectionProperties")
	bool bForceOpaque;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RuntimeMesh|Rendering|LODProperties")
	bool bShouldMergeStaticSectionBuffers;

	FRuntimeMeshLODProperties()
		: ScreenSize(1.0f)
		, bCanGetSectionsIndependently(true)
		, bCanGetAllSectionsAtOnce(false)
		, bShouldMergeStaticSectionBuffers(false)
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

	FRuntimeMeshMaterialSlot() : SlotName(NAME_None), Material(nullptr) { }

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
		: Tangents(false)
		, TexCoords(1, false)
		, Triangles(false)
		, AdjacencyTriangles(false)
	{

	}
	FRuntimeMeshRenderableMeshData(FRuntimeMeshSectionProperties SectionProps)
		: Tangents(SectionProps.bUseHighPrecisionTangents)
		, TexCoords(SectionProps.NumTexCoords, SectionProps.bUseHighPrecisionTexCoords)
		, Triangles(SectionProps.bWants32BitIndices)
		, AdjacencyTriangles(SectionProps.bWants32BitIndices)
	{

	}
	FRuntimeMeshRenderableMeshData(bool bWantsHighPrecisionTangents, bool bWantsHighPrecisionTexCoords, uint8 NumTexCoords, bool bWants32BitIndices)
		: Tangents(bWantsHighPrecisionTangents)
		, TexCoords(NumTexCoords, bWantsHighPrecisionTexCoords)
		, Triangles(bWants32BitIndices)
		, AdjacencyTriangles(bWants32BitIndices)
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

	void ReserveVertices(int32 Number)
	{
		Positions.Reserve(Number);
		Tangents.Reserve(Number);
		TexCoords.Reserve(Number);
		Colors.Reserve(Number);
	}

	FRuntimeMeshRenderableMeshData CopyStructure() const
	{
		return FRuntimeMeshRenderableMeshData(Tangents.IsHighPrecision(),
			TexCoords.IsHighPrecision(),
			TexCoords.NumChannels(),
			Triangles.IsHighPrecision());
	}

	void CopyStructureToSettings(FRuntimeMeshSectionProperties& OutProperties)
	{
		OutProperties.bUseHighPrecisionTangents = Tangents.IsHighPrecision();
		OutProperties.bUseHighPrecisionTexCoords = TexCoords.IsHighPrecision();
		OutProperties.NumTexCoords = TexCoords.NumChannels();
		OutProperties.bWants32BitIndices = Triangles.IsHighPrecision();
	}

	bool HasValidMeshData(bool bPrintErrorMessage = false) const;

	friend FArchive& operator<<(FArchive& Ar, FRuntimeMeshRenderableMeshData& MeshData)
	{
		MeshData.Positions.Serialize(Ar);
		MeshData.Tangents.Serialize(Ar);
		MeshData.TexCoords.Serialize(Ar);
		MeshData.Colors.Serialize(Ar);

		MeshData.Triangles.Serialize(Ar);

		if (Ar.CustomVer(FRuntimeMeshVersion::GUID) > FRuntimeMeshVersion::AddedExtraIndexBuffers)
		{
			FRuntimeMeshTriangleStream NullTriangles(false);
			NullTriangles.Serialize(Ar);
			NullTriangles.Serialize(Ar);
			NullTriangles.Serialize(Ar);
		}

		MeshData.AdjacencyTriangles.Serialize(Ar);

		return Ar;
	}
};



USTRUCT(BlueprintType)
struct RUNTIMEMESHCOMPONENT_API FRuntimeMeshSectionData
{
	GENERATED_BODY()
public:
	FRuntimeMeshSectionProperties Properties;
	FRuntimeMeshRenderableMeshData MeshData;

	FRuntimeMeshSectionData()
		: MeshData(Properties)
	{}

	FRuntimeMeshSectionData(const FRuntimeMeshSectionProperties& InProperties)
		: Properties(InProperties)
		, MeshData(Properties)
	{}
};


#if WITH_EDITOR

namespace __RuntimeMeshNatVisRenderableTypes
{
	//
	//	These types are here specifically for .natvis as they align 
	//	with the structure of the data used within the stream types.
	//

	struct RUNTIMEMESHCOMPONENT_API FRuntimeMeshTriangleIndicesHighPrecision
	{
		int32 Index0;
		int32 Index1;
		int32 Index2;
	};
	struct RUNTIMEMESHCOMPONENT_API FRuntimeMeshTriangleIndicesLowPrecision
	{
		uint16 Index0;
		uint16 Index1;
		uint16 Index2;
	};

	struct RUNTIMEMESHCOMPONENT_API FRuntimeMeshTangentHighPrecision
	{
		FPackedRGBA16N Tangent;
		FPackedRGBA16N Normal;
	};
	struct RUNTIMEMESHCOMPONENT_API FRuntimeMeshTangentLowPrecision
	{
		FPackedNormal Tangent;
		FPackedNormal Normal;
	};

	struct RUNTIMEMESHCOMPONENT_API FRuntimeMeshTexCoordLowPrecision1
	{
		FVector2DHalf UV0;
	};

	struct RUNTIMEMESHCOMPONENT_API FRuntimeMeshTexCoordLowPrecision2
	{
		FVector2DHalf UV0;
		FVector2DHalf UV1;
	};

	struct RUNTIMEMESHCOMPONENT_API FRuntimeMeshTexCoordLowPrecision3
	{
		FVector2DHalf UV0;
		FVector2DHalf UV1;
		FVector2DHalf UV2;
	};

	struct RUNTIMEMESHCOMPONENT_API FRuntimeMeshTexCoordLowPrecision4
	{
		FVector2DHalf UV0;
		FVector2DHalf UV1;
		FVector2DHalf UV2;
		FVector2DHalf UV3;
	};

	struct RUNTIMEMESHCOMPONENT_API FRuntimeMeshTexCoordLowPrecision5
	{
		FVector2DHalf UV0;
		FVector2DHalf UV1;
		FVector2DHalf UV2;
		FVector2DHalf UV3;
		FVector2DHalf UV4;
	};

	struct RUNTIMEMESHCOMPONENT_API FRuntimeMeshTexCoordLowPrecision6
	{
		FVector2DHalf UV0;
		FVector2DHalf UV1;
		FVector2DHalf UV2;
		FVector2DHalf UV3;
		FVector2DHalf UV4;
		FVector2DHalf UV5;
	};

	struct RUNTIMEMESHCOMPONENT_API FRuntimeMeshTexCoordLowPrecision7
	{
		FVector2DHalf UV0;
		FVector2DHalf UV1;
		FVector2DHalf UV2;
		FVector2DHalf UV3;
		FVector2DHalf UV4;
		FVector2DHalf UV5;
		FVector2DHalf UV6;
	};

	struct RUNTIMEMESHCOMPONENT_API FRuntimeMeshTexCoordLowPrecision8
	{
		FVector2DHalf UV0;
		FVector2DHalf UV1;
		FVector2DHalf UV2;
		FVector2DHalf UV3;
		FVector2DHalf UV4;
		FVector2DHalf UV5;
		FVector2DHalf UV6;
		FVector2DHalf UV7;
	};

	struct RUNTIMEMESHCOMPONENT_API FRuntimeMeshTexCoordHighPrecision1
	{
		FVector2D UV0;
	};

	struct RUNTIMEMESHCOMPONENT_API FRuntimeMeshTexCoordHighPrecision2
	{
		FVector2D UV0;
		FVector2D UV1;
	};

	struct RUNTIMEMESHCOMPONENT_API FRuntimeMeshTexCoordHighPrecision3
	{
		FVector2D UV0;
		FVector2D UV1;
		FVector2D UV2;
	};

	struct RUNTIMEMESHCOMPONENT_API FRuntimeMeshTexCoordHighPrecision4
	{
		FVector2D UV0;
		FVector2D UV1;
		FVector2D UV2;
		FVector2D UV3;
	};

	struct RUNTIMEMESHCOMPONENT_API FRuntimeMeshTexCoordHighPrecision5
	{
		FVector2D UV0;
		FVector2D UV1;
		FVector2D UV2;
		FVector2D UV3;
		FVector2D UV4;
	};

	struct RUNTIMEMESHCOMPONENT_API FRuntimeMeshTexCoordHighPrecision6
	{
		FVector2D UV0;
		FVector2D UV1;
		FVector2D UV2;
		FVector2D UV3;
		FVector2D UV4;
		FVector2D UV5;
	};

	struct RUNTIMEMESHCOMPONENT_API FRuntimeMeshTexCoordHighPrecision7
	{
		FVector2D UV0;
		FVector2D UV1;
		FVector2D UV2;
		FVector2D UV3;
		FVector2D UV4;
		FVector2D UV5;
		FVector2D UV6;
	};

	struct RUNTIMEMESHCOMPONENT_API FRuntimeMeshTexCoordHighPrecision8
	{
		FVector2D UV0;
		FVector2D UV1;
		FVector2D UV2;
		FVector2D UV3;
		FVector2D UV4;
		FVector2D UV5;
		FVector2D UV6;
		FVector2D UV7;
	};

}

#endif