// Copyright 2016-2018 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "Engine.h"
#include "RuntimeMeshCore.h"
#include "RuntimeMeshBuilder.h"

enum class ERuntimeMeshBuffersToUpdate : uint8;
struct FRuntimeMeshSectionVertexBufferParams;
struct FRuntimeMeshSectionIndexBufferParams;
class UMaterialInterface;

class RUNTIMEMESHCOMPONENT_API FRuntimeMeshSection
{
	struct FSectionVertexBuffer
	{
	private:
		const FRuntimeMeshVertexStreamStructure VertexStructure;
		const int32 Stride;
		TArray<uint8> Data;
	public:
		FSectionVertexBuffer(const FRuntimeMeshVertexStreamStructure& InVertexStructure)
			: VertexStructure(InVertexStructure)
			, Stride(InVertexStructure.CalculateStride())
		{

		}

		void SetData(TArray<uint8>& InVertices, bool bUseMove)
		{
			if (bUseMove)
			{
				Data = MoveTemp(InVertices);
			}
			else
			{
				Data = InVertices;
			}
		}

		template<typename VertexType>
		void SetData(const TArray<VertexType>& InVertices)
		{
			check(InVertices.GetTypeSize() == GetStride());
			check(VertexStructure.HasAnyElements());
			//check(VertexStructure == GetVertexStructure<VertexType>());

			Data.SetNum(InVertices.GetTypeSize() * InVertices.Num());
			FMemory::Memcpy(Data.GetData(), InVertices.GetData(), Data.Num());
		}

		int32 GetStride() const
		{
			return Stride;
		}

		int32 GetNumVertices() const
		{
			return Stride > 0 ? Data.Num() / Stride : 0;
		}

		const FRuntimeMeshVertexStreamStructure& GetStructure() const { return VertexStructure; }

		TArray<uint8>& GetData() { return Data; }

		void FillUpdateParams(FRuntimeMeshSectionVertexBufferParams& Params);

		bool IsEnabled() const { return VertexStructure.HasAnyElements(); }


		friend FArchive& operator <<(FArchive& Ar, FSectionVertexBuffer& Buffer)
		{
			Ar << const_cast<FRuntimeMeshVertexStreamStructure&>(Buffer.VertexStructure);
			Ar << const_cast<int32&>(Buffer.Stride);
			Ar << Buffer.Data;
			return Ar;
		}
	};

	struct FSectionIndexBuffer
	{
	private:
		const bool b32BitIndices;
		TArray<uint8> Data;
	public:
		FSectionIndexBuffer(bool bIn32BitIndices)
			: b32BitIndices(bIn32BitIndices)
		{

		}

		void SetData(TArray<uint8>& InIndices, bool bUseMove)
		{
			if (bUseMove)
			{
				Data = MoveTemp(InIndices);
			}
			else
			{
				Data = InIndices;
			}
		}

		template<typename IndexType>
		void SetData(const TArray<IndexType>& InIndices)
		{
			check(InIndices.GetTypeSize() == GetStride());

			Data.SetNum(InIndices.GetTypeSize() * InIndices.Num());
			FMemory::Memcpy(Data.GetData(), InIndices.GetData(), Data.Num());
		}

		int32 GetStride() const
		{
			return b32BitIndices ? 4 : 2;
		}

		bool Is32BitIndices() const
		{
			return b32BitIndices;
		}

		int32 GetNumIndices() const
		{
			return Data.Num() / GetStride();
		}

		TArray<uint8>& GetData() { return Data; }

		void FillUpdateParams(FRuntimeMeshSectionIndexBufferParams& Params);

		friend FArchive& operator <<(FArchive& Ar, FSectionIndexBuffer& Buffer)
		{
			Ar << const_cast<bool&>(Buffer.b32BitIndices);
			Ar << Buffer.Data;
			return Ar;
		}
	};


	const EUpdateFrequency UpdateFrequency;

	FSectionVertexBuffer VertexBuffer0;
	FSectionVertexBuffer VertexBuffer1;
	FSectionVertexBuffer VertexBuffer2;
	FSectionIndexBuffer IndexBuffer;
	FSectionIndexBuffer AdjacencyIndexBuffer;

	FBox LocalBoundingBox;

	bool bCollisionEnabled;

	bool bIsVisible;

	bool bCastsShadow;

//	TUniquePtr<FRuntimeMeshLockProvider> SyncRoot;
public:
	FRuntimeMeshSection(FArchive& Ar);
	FRuntimeMeshSection(const FRuntimeMeshVertexStreamStructure& InVertexStructure0, const FRuntimeMeshVertexStreamStructure& InVertexStructure1,
		const FRuntimeMeshVertexStreamStructure& InVertexStructure2, bool b32BitIndices, EUpdateFrequency InUpdateFrequency/*, FRuntimeMeshLockProvider* InSyncRoot*/);

// 	void SetNewLockProvider(FRuntimeMeshLockProvider* NewSyncRoot)
// 	{
// 		SyncRoot.Reset(NewSyncRoot);
// 	}

//	FRuntimeMeshLockProvider GetSyncRoot() { return SyncRoot->Get(); }

	bool IsCollisionEnabled() const { return bCollisionEnabled; }
	bool IsVisible() const { return bIsVisible; }
	bool ShouldRender() const { return IsVisible() && HasValidMeshData(); }
	bool CastsShadow() const { return bCastsShadow; }
	EUpdateFrequency GetUpdateFrequency() const { return UpdateFrequency; }
	FBox GetBoundingBox() const { return LocalBoundingBox; }

	int32 GetNumVertices() const { return VertexBuffer0.GetNumVertices(); }
	int32 GetNumIndices() const { return IndexBuffer.GetNumIndices(); }

	bool HasValidMeshData() const {
		if (IndexBuffer.GetNumIndices() <= 0)
			return false;
		if (VertexBuffer0.GetNumVertices() <= 0)
			return false;
		if (VertexBuffer1.IsEnabled() && VertexBuffer1.GetNumVertices() != VertexBuffer0.GetNumVertices())
			return false;
		if (VertexBuffer2.IsEnabled() && VertexBuffer2.GetNumVertices() != VertexBuffer2.GetNumVertices())
			return false;
		return true;
	}

	int32 NumVertexStreams() const;

	void SetVisible(bool bNewVisible)
	{
		bIsVisible = bNewVisible;
	}
	void SetCastsShadow(bool bNewCastsShadow)
	{
		bCastsShadow = bNewCastsShadow;
	}
	void SetCollisionEnabled(bool bNewCollision)
	{
		bCollisionEnabled = bNewCollision;
	}

	void UpdateVertexBuffer0(TArray<uint8>& InVertices, bool bUseMove)
	{
		VertexBuffer0.SetData(InVertices, bUseMove);
		UpdateBoundingBox();
	}

	template<typename VertexType>
	void UpdateVertexBuffer0(const TArray<VertexType>& InVertices, const FBox* BoundingBox = nullptr)
	{
		VertexBuffer0.SetData(InVertices);

		if (BoundingBox)
		{
			LocalBoundingBox = *BoundingBox;
		}
		else
		{
			UpdateBoundingBox();
		}
	}

	void UpdateVertexBuffer1(TArray<uint8>& InVertices, bool bUseMove)
	{
		VertexBuffer1.SetData(InVertices, bUseMove);
	}

	template<typename VertexType>
	void UpdateVertexBuffer1(const TArray<VertexType>& InVertices)
	{
		VertexBuffer1.SetData(InVertices);
	}

	void UpdateVertexBuffer2(TArray<uint8>& InVertices, bool bUseMove)
	{
		VertexBuffer2.SetData(InVertices, bUseMove);
	}

	template<typename VertexType>
	void UpdateVertexBuffer2(const TArray<VertexType>& InVertices)
	{
		VertexBuffer2.SetData(InVertices);
	}

	void UpdateIndexBuffer(TArray<uint8>& InIndices, bool bUseMove)
	{
		IndexBuffer.SetData(InIndices, bUseMove);
	}

	template<typename IndexType>
	void UpdateIndexBuffer(const TArray<IndexType>& InIndices)
	{
		IndexBuffer.SetData(InIndices);
	}

	template<typename IndexType>
	void UpdateAdjacencyIndexBuffer(const TArray<IndexType>& InIndices)
	{
		AdjacencyIndexBuffer.SetData(InIndices);
	}

	TSharedPtr<FRuntimeMeshAccessor> GetSectionMeshAccessor()
	{
		return MakeShared<FRuntimeMeshAccessor>(VertexBuffer0.GetStructure(),
			VertexBuffer1.GetStructure(), VertexBuffer2.GetStructure(), IndexBuffer.Is32BitIndices(),
			&VertexBuffer0.GetData(), &VertexBuffer1.GetData(), &VertexBuffer2.GetData(), &IndexBuffer.GetData());
	}

	TSharedPtr<FRuntimeMeshIndicesAccessor> GetTessellationIndexAccessor()
	{
		return MakeShared<FRuntimeMeshIndicesAccessor>(AdjacencyIndexBuffer.Is32BitIndices(), &AdjacencyIndexBuffer.GetData());
	}








	bool CheckBuffer0VertexType(const FRuntimeMeshVertexStreamStructure& Stream0Structure) const
	{
		return VertexBuffer0.GetStructure() == Stream0Structure;
	}

	bool CheckBuffer1VertexType(const FRuntimeMeshVertexStreamStructure& Stream1Structure) const
	{
		return VertexBuffer1.GetStructure() == Stream1Structure;
	}

	bool CheckBuffer2VertexType(const FRuntimeMeshVertexStreamStructure& Stream2Structure) const
	{
		return VertexBuffer2.GetStructure() == Stream2Structure;
	}

	bool CheckIndexBufferSize(bool b32BitIndices) const
	{
		return b32BitIndices == IndexBuffer.Is32BitIndices();
	}



	TSharedPtr<struct FRuntimeMeshSectionCreationParams, ESPMode::NotThreadSafe> GetSectionCreationParams();

	TSharedPtr<struct FRuntimeMeshSectionUpdateParams, ESPMode::NotThreadSafe> GetSectionUpdateData(ERuntimeMeshBuffersToUpdate BuffersToUpdate);

	TSharedPtr<struct FRuntimeMeshSectionPropertyUpdateParams, ESPMode::NotThreadSafe> GetSectionPropertyUpdateData();

	void UpdateBoundingBox();

	int32 GetCollisionData(TArray<FVector>& OutPositions, TArray<FTriIndices>& OutIndices, TArray<FVector2D>& OutUVs);


	friend FArchive& operator <<(FArchive& Ar, FRuntimeMeshSection& MeshData)
	{
		Ar << const_cast<EUpdateFrequency&>(MeshData.UpdateFrequency);

		Ar << MeshData.VertexBuffer0;
		Ar << MeshData.VertexBuffer1;
		Ar << MeshData.VertexBuffer2;

		Ar << MeshData.IndexBuffer;
		Ar << MeshData.AdjacencyIndexBuffer;

		Ar << MeshData.LocalBoundingBox;

		Ar << MeshData.bCollisionEnabled;
		Ar << MeshData.bIsVisible;
		Ar << MeshData.bCastsShadow;

		return Ar;
	}
};




/** Smart pointer to a Runtime Mesh Section */
using FRuntimeMeshSectionPtr = TSharedPtr<FRuntimeMeshSection, ESPMode::ThreadSafe>;




FORCEINLINE static FArchive& operator <<(FArchive& Ar, FRuntimeMeshSectionPtr& Section)
{
	if (Ar.IsSaving())
	{
		bool bHasSection = Section.IsValid();
		Ar << bHasSection;
		if (bHasSection)
		{
			Ar << *Section.Get();
		}
	}
	else if (Ar.IsLoading())
	{
		bool bHasSection;
		Ar << bHasSection;
		if (bHasSection)
		{
			Section = MakeShared<FRuntimeMeshSection, ESPMode::ThreadSafe>(Ar);
		}
	}
	return Ar;
}