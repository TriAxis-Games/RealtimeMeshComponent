// Copyright 2016 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "Engine.h"
#include "Components/MeshComponent.h"
#include "RuntimeMeshProfiling.h"
#include "RuntimeMeshCore.generated.h"

class FRuntimeMeshVertexFactory;

#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 12
/** Structure definition of a vertex */
using RuntimeMeshVertexStructure = FLocalVertexFactory::FDataType;
#else
/** Structure definition of a vertex */
using RuntimeMeshVertexStructure = FLocalVertexFactory::DataType;
#endif

/* Update frequency for a section. Used to optimize for update or render speed*/
UENUM(BlueprintType)
enum class EUpdateFrequency
{
	/* Tries to skip recreating the scene proxy if possible. */
	Average UMETA(DisplayName = "Average"),
	/* Tries to skip recreating the scene proxy if possible and optimizes the buffers for frequent updates. */
	Frequent UMETA(DisplayName = "Frequent"),
	/* If the component is static it will try to use the static rendering path (this will force a recreate of the scene proxy) */
	Infrequent UMETA(DisplayName = "Infrequent")
};

/* Update frequency for a section. Used to optimize for update or render speed*/
enum class ESectionUpdateFlags
{
	None = 0x0,

	/** 
		This will use move-assignment when copying the supplied vertices/triangles into the section.
		This is faster as it doesn't require copying the data.

		CAUTION: This means that your copy of the arrays will be cleared!
	*/
	MoveArrays = 0x1,


	//CalculateNormalTangent = 0x2,
	
};
ENUM_CLASS_FLAGS(ESectionUpdateFlags)


/**
*	Struct used to specify a tangent vector for a vertex
*	The Y tangent is computed from the cross product of the vertex normal (Tangent Z) and the TangentX member.
*/
USTRUCT(BlueprintType)
struct FRuntimeMeshTangent
{
	GENERATED_USTRUCT_BODY()

	/** Direction of X tangent for this vertex */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Tangent)
	FVector TangentX;

	/** Bool that indicates whether we should flip the Y tangent when we compute it using cross product */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Tangent)
	bool bFlipTangentY;

	FRuntimeMeshTangent()
		: TangentX(1.f, 0.f, 0.f)
		, bFlipTangentY(false)
	{}

	FRuntimeMeshTangent(float X, float Y, float Z)
		: TangentX(X, Y, Z)
		, bFlipTangentY(false)
	{}

	FRuntimeMeshTangent(FVector InTangentX, bool bInFlipTangentY)
		: TangentX(InTangentX)
		, bFlipTangentY(bInFlipTangentY)
	{}

	void AdjustNormal(FPackedNormal& Normal) const
	{
		Normal.Vector.W = bFlipTangentY ? 0 : 255;
	}
};





/** Interface class for update data sent to the RT for updating a single mesh section */
class FRuntimeMeshSectionUpdateDataInterface
{
private:
	/** Section to update */
	int32 TargetSection;
public:
	FRuntimeMeshSectionUpdateDataInterface() { }
	virtual ~FRuntimeMeshSectionUpdateDataInterface() { }

	virtual void SetTargetSection(int32 InTargetSection) { TargetSection = InTargetSection; }
	virtual int32 GetTargetSection() { return TargetSection; }

	virtual class FRuntimeMeshSectionProxyInterface* GetNewProxy() = 0;

	/* Cast the update data to the specific type of update data */
	template <typename Type>
	Type* As()
	{
		return static_cast<Type*>(this);
	}
}; 

/** Templated class for update data sent to the RT for updating a single mesh section */
template<typename VertexType>
class FRuntimeMeshSectionCreateData : public FRuntimeMeshSectionUpdateDataInterface
{
public:
	/* The new proxy to be used for section creation */
	class FRuntimeMeshSectionProxyInterface* NewProxy;

	/** Whether this section is currently visible */
	bool bIsVisible;

	/** Should this section cast a shadow */
	bool bCastsShadow;

	/* Updated vertex buffer for the section */
	TArray<VertexType> VertexBuffer;

	/* Updated index buffer for the section */
	TArray<int32> IndexBuffer;

	/* Material for this one section (only filled for section creation) */
	UMaterialInterface* Material;

	/** Update frequency of this section */
	EUpdateFrequency UpdateFrequency;


	FRuntimeMeshSectionCreateData() {}
	virtual ~FRuntimeMeshSectionCreateData() override { }

	virtual class FRuntimeMeshSectionProxyInterface* GetNewProxy() override { return NewProxy; }

};

/** Templated class for update data sent to the RT for updating a single mesh section */
template<typename VertexType>
class FRuntimeMeshSectionUpdateData : public FRuntimeMeshSectionUpdateDataInterface
{
public:
	/* Updated vertex buffer for the section */
	TArray<VertexType> VertexBuffer;

	/* Updated index buffer for the section */
	TArray<int32> IndexBuffer;

	/* Should we apply the indices as an update */
	bool bIncludeIndices;

	FRuntimeMeshSectionUpdateData() {}
	virtual ~FRuntimeMeshSectionUpdateData() override { }

	virtual class FRuntimeMeshSectionProxyInterface* GetNewProxy() override { return nullptr; }
};








/** Vertex Buffer for one section. Templated to support different vertex types */
template<typename VertexType>
class FRuntimeMeshVertexBuffer : public FVertexBuffer
{
	/* The number of vertices this buffer is currently allocated to hold */
	int32 VertexCount;
	EBufferUsageFlags UsageFlags;
public:

	FRuntimeMeshVertexBuffer(EUpdateFrequency SectionUpdateFrequency) : VertexCount(0)
	{
		UsageFlags = SectionUpdateFrequency == EUpdateFrequency::Frequent ? BUF_Dynamic : BUF_Static;
	}

	virtual void InitRHI() override
	{
		// Create the vertex buffer
		FRHIResourceCreateInfo CreateInfo;
		VertexBufferRHI = RHICreateVertexBuffer(sizeof(VertexType) * VertexCount, UsageFlags, CreateInfo);
	}

	/* Get the size of the vertex buffer */
	int32 Num() { return VertexCount; }
	
	/* Set the size of the vertex buffer */
	void SetNum(int32 NewVertexCount)
	{
		check(NewVertexCount != 0);

		// Make sure we're not already the right size
		if (NewVertexCount != VertexCount)
		{
			VertexCount = NewVertexCount;
			
			// Rebuild resource
			ReleaseResource();
			InitResource();
		}
	}

	/* Set the data for the vertex buffer */
	void SetData(const TArray<VertexType>& Data)
	{
		check(Data.Num() == VertexCount);

		// Lock the vertex buffer
 		void* Buffer = RHILockVertexBuffer(VertexBufferRHI, 0, Data.Num() * sizeof(VertexType), RLM_WriteOnly);
 		 
 		// Write the vertices to the vertex buffer
 		FMemory::Memcpy(Buffer, Data.GetData(), Data.Num() * sizeof(VertexType));

		// Unlock the vertex buffer
 		RHIUnlockVertexBuffer(VertexBufferRHI);
	}
};

/** Index Buffer */
class FRuntimeMeshIndexBuffer : public FIndexBuffer
{
	/* The number of indices this buffer is currently allocated to hold */
	int32 IndexCount;
	EBufferUsageFlags UsageFlags;
public:

	FRuntimeMeshIndexBuffer(EUpdateFrequency SectionUpdateFrequency) : IndexCount(0)
	{
		UsageFlags = SectionUpdateFrequency == EUpdateFrequency::Frequent ? BUF_Dynamic : BUF_Static;
	}

	virtual void InitRHI() override
	{
		// Create the index buffer
		FRHIResourceCreateInfo CreateInfo;
		IndexBufferRHI = RHICreateIndexBuffer(sizeof(int32), IndexCount * sizeof(int32), BUF_Dynamic, CreateInfo);
	}

	/* Get the size of the index buffer */
	int32 Num() { return IndexCount; }

	/* Set the size of the index buffer */
	void SetNum(int32 NewIndexCount)
	{
		check(NewIndexCount != 0);

		// Make sure we're not already the right size
		if (NewIndexCount != IndexCount)
		{
			IndexCount = NewIndexCount;

			// Rebuild resource
			ReleaseResource();
			InitResource();
		}
	}

	/* Set the data for the index buffer */
	void SetData(const TArray<int32>& Data)
	{
		check(Data.Num() == IndexCount);

		// Lock the index buffer
		void* Buffer = RHILockIndexBuffer(IndexBufferRHI, 0, IndexCount * sizeof(int32), RLM_WriteOnly);

		// Write the indices to the vertex buffer	
		FMemory::Memcpy(Buffer, Data.GetData(), Data.Num() * sizeof(int32));

		// Unlock the index buffer
		RHIUnlockIndexBuffer(IndexBufferRHI);
	}
};

/** Interface class for the RT proxy of a single mesh section */
class FRuntimeMeshSectionProxyInterface
{
public:

	FRuntimeMeshSectionProxyInterface() {}
	virtual ~FRuntimeMeshSectionProxyInterface() {}

	virtual void SetIsVisible(bool NewVisible) = 0;
	virtual bool IsVisible() = 0;
	bool ShouldRender() { return IsVisible() && GetVertexCount() > 0 && GetIndexCount(); }

	virtual void SetCastsShadow(bool bCastsShadow) = 0;
	virtual bool CastsShadow() = 0;

	virtual int32 GetVertexCount() = 0;
	virtual FVertexBuffer& GetVertexBuffer() = 0;

	virtual int32 GetIndexCount() = 0;
	virtual FIndexBuffer& GetIndexBuffer() = 0;

	virtual FRuntimeMeshVertexFactory& GetVertexFactory() = 0;

	virtual UMaterialInterface* GetMaterial() = 0;

	virtual EUpdateFrequency GetUpdateFrequency() = 0;

	bool ShouldRenderInStaticIfPossible() { return GetUpdateFrequency() == EUpdateFrequency::Infrequent; }

	virtual void FinishCreate_RenderThread(FRuntimeMeshSectionUpdateDataInterface* UpdateData) = 0;
	virtual void FinishUpdate_RenderThread(FRuntimeMeshSectionUpdateDataInterface* UpdateData) = 0;
};

/** Vertex Factory */
class FRuntimeMeshVertexFactory : public FLocalVertexFactory
{
	FRuntimeMeshSectionProxyInterface* SectionParent;

public:

	FRuntimeMeshVertexFactory(FRuntimeMeshSectionProxyInterface* InSectionParent) : SectionParent(InSectionParent) { }
		
	/** Init function that can be called on any thread, and will do the right thing (enqueue command if called on main thread) */
	void Init(const RuntimeMeshVertexStructure VertexStructure)
	{
		if (IsInRenderingThread())
		{
			SetData(VertexStructure);
		}
		else
		{
			// Send the command to the render thread
			ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
				InitRuntimeMeshVertexFactory,
				FRuntimeMeshVertexFactory*, VertexFactory, this,
				const RuntimeMeshVertexStructure, VertexStructure, VertexStructure,
				{
					VertexFactory->Init(VertexStructure);
				});
		}
	}

	/* Gets the section visibility for static sections */
	virtual uint64 GetStaticBatchElementVisibility(const class FSceneView& View, const struct FMeshBatch* Batch) const override
	{
		return SectionParent->ShouldRender();
	}
};

/** Templated class for the RT proxy of a single mesh section */
template <typename VertexType>
class FRuntimeMeshSectionProxy : public FRuntimeMeshSectionProxyInterface
{
protected:
	/** Whether this section is currently visible */
	bool bIsVisible;

	/** Should this section cast a shadow */
	bool bCastsShadow;

	/** Material applied to this section */
	UMaterialInterface* Material;

	/** Vertex buffer for this section */
	FRuntimeMeshVertexBuffer<VertexType> VertexBuffer;

	/** Index buffer for this section */
	FRuntimeMeshIndexBuffer IndexBuffer;

	/** Vertex factory for this section */
	FRuntimeMeshVertexFactory VertexFactory;

	/** Update frequency of this section */
	const EUpdateFrequency UpdateFrequency;

public:
	FRuntimeMeshSectionProxy(EUpdateFrequency InUpdateFrequency) : bIsVisible(true), bCastsShadow(true), Material(nullptr), VertexBuffer(InUpdateFrequency), IndexBuffer(InUpdateFrequency), VertexFactory(this), UpdateFrequency(InUpdateFrequency) { }
	virtual ~FRuntimeMeshSectionProxy() override
	{
		VertexBuffer.ReleaseResource();
		IndexBuffer.ReleaseResource();
		VertexFactory.ReleaseResource();
	}

	void SetIsVisible(bool bNewVisibility) override { bIsVisible = bNewVisibility; }
	virtual bool IsVisible() { return bIsVisible; }

	virtual void SetCastsShadow(bool bNewCastsShadow) override { bCastsShadow = bNewCastsShadow; };
	virtual bool CastsShadow() override { return bCastsShadow; }

	virtual int32 GetVertexCount() override { return VertexBuffer.Num(); }
	FVertexBuffer& GetVertexBuffer() override { return VertexBuffer; }
	
	virtual int32 GetIndexCount() override { return IndexBuffer.Num(); }
	virtual FIndexBuffer& GetIndexBuffer() override { return IndexBuffer; }

	virtual FRuntimeMeshVertexFactory& GetVertexFactory() override { return VertexFactory; }

	virtual UMaterialInterface* GetMaterial() override { return Material; }

	virtual EUpdateFrequency GetUpdateFrequency() override { return UpdateFrequency; }

	virtual void FinishCreate_RenderThread(FRuntimeMeshSectionUpdateDataInterface* UpdateData)
	{
 		check(IsInRenderingThread());

		auto* SectionUpdateData = UpdateData->As<FRuntimeMeshSectionCreateData<VertexType>>();
		check(SectionUpdateData);

		// Copy visibility/shadow/update frequency info
		bIsVisible = SectionUpdateData->bIsVisible;
		bCastsShadow = SectionUpdateData->bCastsShadow;

		// Populate the material
		Material = SectionUpdateData->Material;
		if (Material == nullptr)
		{
			Material = UMaterial::GetDefaultMaterial(MD_Surface);
		}

		// Initialize the vertex factory
		VertexFactory.Init(VertexType::GetVertexStructure(VertexBuffer));
		VertexFactory.InitResource();

		auto& Vertices = SectionUpdateData->VertexBuffer;
		VertexBuffer.SetNum(Vertices.Num());
		VertexBuffer.SetData(Vertices);

		auto& Indices = SectionUpdateData->IndexBuffer;
		IndexBuffer.SetNum(Indices.Num());
		IndexBuffer.SetData(Indices);
	}
	
	virtual void FinishUpdate_RenderThread(FRuntimeMeshSectionUpdateDataInterface* UpdateData) override
	{
		check(UpdateData);
		check(IsInRenderingThread());

		auto* NewData = UpdateData->As<FRuntimeMeshSectionUpdateData<VertexType>>();
		check(NewData);

		auto& VertexBufferData = NewData->VertexBuffer;
		VertexBuffer.SetNum(VertexBufferData.Num());
		VertexBuffer.SetData(VertexBufferData);

		if (NewData->bIncludeIndices)
		{
			auto& IndexBufferData = NewData->IndexBuffer;
			IndexBuffer.SetNum(IndexBufferData.Num());
			IndexBuffer.SetData(IndexBufferData);
		}
	}
};





/** Interface class for a single mesh section */
class FRuntimeMeshSectionInterface
{
public:

	/** Index buffer for this section */
	TArray<int32> IndexBuffer;

	/** Local bounding box of section */
	FBox LocalBoundingBox;

	/** Should we build collision data for triangles in this section */
	bool CollisionEnabled;

	/** Should we display this section */
	bool bIsVisible;

	/** Should this section cast a shadow */
	bool bCastsShadow;

	/** Is this an internal section type. */
	bool bIsInternalSectionType;

	/** Update frequency of this section */
	EUpdateFrequency UpdateFrequency;

	FRuntimeMeshSectionInterface() : 
		LocalBoundingBox(0),
		CollisionEnabled(false),
		bIsVisible(true),
		bCastsShadow(true),
		bIsInternalSectionType(false)
	{}

	void UpdateIndexBuffer(const TArray<int32>& Triangles)
	{
		IndexBuffer = Triangles;
	}
	void UpdateIndexBufferMove(TArray<int32>& Triangles)
	{
		IndexBuffer = MoveTemp(Triangles);
	}
	
	virtual FRuntimeMeshSectionUpdateDataInterface* GetSectionCreationData(UMaterialInterface* InMaterial) const  = 0;

	virtual FRuntimeMeshSectionUpdateDataInterface* GetSectionUpdateData(bool bIncludeIndices) const = 0;

	virtual int32 GetAllVertexPositions(TArray<FVector>& Positions) = 0;
	

	virtual void GetInternalVertexComponents(int32& NumUVChannels, bool& WantsHalfPrecisionUVs) { }

	virtual void Serialize(FArchive& Ar)
	{
		Ar << IndexBuffer;
		Ar << LocalBoundingBox;
		Ar << CollisionEnabled;
		Ar << bIsVisible;
		int32 UpdateFreq = (int32)UpdateFrequency;
		Ar << UpdateFreq;
		UpdateFrequency = (EUpdateFrequency)UpdateFreq;
	}
protected:
	// This is only meant for internal use for supporting the old style create/update sections
	virtual void UpdateVertexBufferInternal(const TArray<FVector>& Positions, const TArray<FVector>& Normals, const TArray<FRuntimeMeshTangent>& Tangents, const TArray<FVector2D>& UV0, const TArray<FVector2D>& UV1, const TArray<FColor>& Colors) { }

	friend class URuntimeMeshComponent;


	friend FArchive& operator <<(FArchive& Ar, FRuntimeMeshSectionInterface& Section)
	{
		Section.Serialize(Ar);
		return Ar;
	}
};

/** Templated class for a single mesh section */
template<typename VertexType>
class FRuntimeMeshSection : public FRuntimeMeshSectionInterface
{
public:
	/** Vertex buffer for this section */
	TArray<VertexType> VertexBuffer;
	
	virtual FRuntimeMeshSectionUpdateDataInterface* GetSectionCreationData(UMaterialInterface* InMaterial) const override
	{
		auto UpdateData = new FRuntimeMeshSectionCreateData<VertexType>();
		UpdateData->NewProxy = new FRuntimeMeshSectionProxy<VertexType>(UpdateFrequency);
		UpdateData->bIsVisible = bIsVisible;
		UpdateData->bCastsShadow = bCastsShadow;
		UpdateData->VertexBuffer = VertexBuffer;
		UpdateData->IndexBuffer = IndexBuffer;
		UpdateData->Material = InMaterial;
		return UpdateData;
	}

	virtual FRuntimeMeshSectionUpdateDataInterface* GetSectionUpdateData(bool bIncludeIndices) const override
	{
		auto UpdateData = new FRuntimeMeshSectionUpdateData<VertexType>();
		UpdateData->VertexBuffer = VertexBuffer;
		UpdateData->bIncludeIndices = bIncludeIndices;
		if (bIncludeIndices)
		{
			UpdateData->IndexBuffer = IndexBuffer;
		}
		return UpdateData;
	}


	virtual int32 GetAllVertexPositions(TArray<FVector>& Positions) override
	{
		int32 VertexCount = VertexBuffer.Num();
		for (int32 VertIdx = 0; VertIdx < VertexCount; VertIdx++)
		{
			Positions.Add(VertexBuffer[VertIdx].Position);
		}
		return VertexCount;
	}



	void UpdateVertexBuffer(const TArray<VertexType>& Vertices)
	{
		LocalBoundingBox.Init();
		int32 NumVertices = Vertices.Num();
		VertexBuffer.SetNumUninitialized(NumVertices);
		for (int32 VertexIdx = 0; VertexIdx < NumVertices; VertexIdx++)
		{
			LocalBoundingBox += Vertices[VertexIdx].Position;
			VertexBuffer[VertexIdx] = Vertices[VertexIdx];
		}
	}

	bool UpdateVertexBuffer(const TArray<VertexType>& Vertices, const FBox& BoundingBox)
	{
		VertexBuffer = Vertices;

		// Update the bounding box if necessary and alert our caller if we did
		if (!(LocalBoundingBox == BoundingBox))
		{
			LocalBoundingBox = BoundingBox;
			return true;
		}

		return false;
	}

	void UpdateVertexBufferMove(TArray<VertexType>& Vertices)
	{
		VertexBuffer = MoveTemp(Vertices);

		LocalBoundingBox.Init();
		int32 NumVertices = VertexBuffer.Num();
		for (int32 VertexIdx = 0; VertexIdx < NumVertices; VertexIdx++)
		{
			LocalBoundingBox += VertexBuffer[VertexIdx].Position;
		}
	}

	bool UpdateVertexBufferMove(TArray<VertexType>& Vertices, const FBox& BoundingBox)
	{
		VertexBuffer = MoveTemp(Vertices);

		// Update the bounding box if necessary and alert our caller if we did
		if (!(LocalBoundingBox == BoundingBox))
		{
			LocalBoundingBox = BoundingBox;
			return true;
		}

		return false;
	}
};





USTRUCT()
struct FRuntimeMeshCollisionSection
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FVector> VertexBuffer;

	UPROPERTY()
	TArray<int32> IndexBuffer;

	void Reset()
	{
		VertexBuffer.Empty();
		IndexBuffer.Empty();
	}
};



/** Smart pointer to a Runtime Mesh Section */
using RuntimeMeshSectionPtr = TSharedPtr<FRuntimeMeshSectionInterface>;


#define RUNTIMEMESH_VERTEXCOMPONENT(VertexBuffer, VertexType, Member, MemberType) \
	STRUCTMEMBER_VERTEXSTREAMCOMPONENT(&VertexBuffer, VertexType, Member, MemberType)