// Copyright 2016 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "RuntimeMeshCore.h"
#include "RuntimeMeshComponent.h"

#define RUNTIMEMESHCOMPONENTASYNC_ENQUEUETASK(TaskType, DataType, Data, RuntimeMesh, Code)														\
	class FParallelRuntimeMeshComponentTask_##TaskType																							\
	{																																			\
		TWeakObjectPtr<URuntimeMeshComponent> RuntimeMeshComponent;																				\
		DataType* Data;																															\
	public:																																		\
		FParallelRuntimeMeshComponentTask_##TaskType(TWeakObjectPtr<URuntimeMeshComponent> InRuntimeMeshComponent, DataType* InData)			\
			: RuntimeMeshComponent(InRuntimeMeshComponent), Data(InData)																		\
		{																																		\
		}																																		\
																																				\
		~FParallelRuntimeMeshComponentTask_##TaskType()																							\
		{																																		\
			delete Data;																														\
		}																																		\
																																				\
		FORCEINLINE TStatId GetStatId() const																									\
		{																																		\
			RETURN_QUICK_DECLARE_CYCLE_STAT(FParallelRuntimeMeshComponentTask_##TaskType, STATGROUP_TaskGraphTasks);							\
		}																																		\
																																				\
		static ENamedThreads::Type GetDesiredThread()																							\
		{																																		\
			return ENamedThreads::GameThread;																									\
		}																																		\
																																				\
		static ESubsequentsMode::Type GetSubsequentsMode()																						\
		{																																		\
			return ESubsequentsMode::FireAndForget;																								\
		}																																		\
																																				\
		void DoTask(ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)											\
		{																																		\
			if (URuntimeMeshComponent* Mesh = RuntimeMeshComponent.Get())																		\
			{																																	\
				Code																															\
			}																																	\
		}																																		\
	};																																			\
	TGraphTask<FParallelRuntimeMeshComponentTask_##TaskType>::CreateTask(nullptr).ConstructAndDispatchWhenReady(RuntimeMesh, Data);




class RUNTIMEMESHCOMPONENT_API FRuntimeMeshAsync
{



public:

	/**
	*	Create/replace a section.
	*	@param	SectionIndex		Index of the section to create or replace.
	*	@param	Vertices			Vertex buffer all vertex data for this section.
	*	@param	Triangles			Index buffer indicating which vertices make up each triangle. Length must be a multiple of 3.
	*	@param	bCreateCollision	Indicates whether collision should be created for this section. This adds significant cost.
	*	@param	UpdateFrequency		Indicates how frequently the section will be updated. Allows the RMC to optimize itself to a particular use.
	*	@param	UpdateFlags			Flags pertaining to this particular update.
	*/
	template<typename VertexType>
	static void CreateMeshSection(TWeakObjectPtr<URuntimeMeshComponent> InRuntimeMeshComponent, int32 SectionIndex, TArray<VertexType>& Vertices, TArray<int32>& Triangles, 
		bool bCreateCollision = false, EUpdateFrequency UpdateFrequency = EUpdateFrequency::Average, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		struct FRMCAsyncData
		{
			int32 SectionIndex;
			TArray<VertexType> Vertices;
			TArray<int32> Triangles;
			bool bCreateCollision;
			EUpdateFrequency UpdateFrequency;
			ESectionUpdateFlags UpdateFlags;
		};

		FRMCAsyncData* Data = new FRMCAsyncData;
		Data->SectionIndex = SectionIndex;

		if (!!(UpdateFlags & ESectionUpdateFlags::MoveArrays))
		{
			Data->Vertices = MoveTemp(Vertices);
			Data->Triangles = MoveTemp(Triangles);
		}
		else
		{
			Data->Vertices = Vertices;
			Data->Triangles = Triangles;
		}
		Data->bCreateCollision = bCreateCollision;
		Data->UpdateFrequency = UpdateFrequency;
		Data->UpdateFlags = UpdateFlags | ESectionUpdateFlags::MoveArrays; // We can always use move arrays here since we either just copied it, or moved it from the original

		RUNTIMEMESHCOMPONENTASYNC_ENQUEUETASK(CreateMeshSection, FRMCAsyncData, Data, InRuntimeMeshComponent,
		{
			Mesh->CreateMeshSection(Data->SectionIndex, Data->Vertices, Data->Triangles, Data->bCreateCollision, Data->UpdateFrequency, Data->UpdateFlags);
		});
	}


	/**
	*	Create/replace a section.
	*	@param	SectionIndex		Index of the section to create or replace.
	*	@param	Vertices			Vertex buffer all vertex data for this section.
	*	@param	Triangles			Index buffer indicating which vertices make up each triangle. Length must be a multiple of 3.
	*	@param	BoundingBox			The bounds of this section. Faster than the RMC automatically calculating it.
	*	@param	bCreateCollision	Indicates whether collision should be created for this section. This adds significant cost.
	*	@param	UpdateFrequency		Indicates how frequently the section will be updated. Allows the RMC to optimize itself to a particular use.
	*	@param	UpdateFlags			Flags pertaining to this particular update.
	*/
	template<typename VertexType>
	void CreateMeshSection(TWeakObjectPtr<URuntimeMeshComponent> InRuntimeMeshComponent, int32 SectionIndex, TArray<VertexType>& Vertices, TArray<int32>& Triangles, 
		const FBox& BoundingBox, bool bCreateCollision = false, EUpdateFrequency UpdateFrequency = EUpdateFrequency::Average, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		struct FRMCAsyncData
		{
			int32 SectionIndex;
			TArray<VertexType> Vertices;
			TArray<int32> Triangles;
			FBox BoundingBox;
			bool bCreateCollision;
			EUpdateFrequency UpdateFrequency;
			ESectionUpdateFlags UpdateFlags;
		};

		FRMCAsyncData* Data = new FRMCAsyncData;
		Data->SectionIndex = SectionIndex;

		if (!!(UpdateFlags & ESectionUpdateFlags::MoveArrays))
		{
			Data->Vertices = MoveTemp(Vertices);
			Data->Triangles = MoveTemp(Triangles);
		}
		else
		{
			Data->Vertices = Vertices;
			Data->Triangles = Triangles;
		}
		Data->BoundingBox = BoundingBox;
		Data->bCreateCollision = bCreateCollision;
		Data->UpdateFrequency = UpdateFrequency;
		Data->UpdateFlags = UpdateFlags | ESectionUpdateFlags::MoveArrays; // We can always use move arrays here since we either just copied it, or moved it from the original

		RUNTIMEMESHCOMPONENTASYNC_ENQUEUETASK(CreateMeshSection, FRMCAsyncData, Data, InRuntimeMeshComponent,
		{
			Mesh->CreateMeshSection(Data->SectionIndex, Data->Vertices, Data->Triangles, Data->BoundingBox, Data->bCreateCollision, Data->UpdateFrequency, Data->UpdateFlags);
		});
	}

	/**
	*	Create/replace a section using 2 vertex buffers. One contains positions only, the other contains all other data. This allows for very efficient updates of the positions of a mesh.
	*	@param	SectionIndex		Index of the section to create or replace.
	*	@param	VertexPositions		Vertex buffer containing only the position information for each vertex.
	*	@param	VertexData			Vertex buffer containing everything except position for each vertex.
	*	@param	Triangles			Index buffer indicating which vertices make up each triangle. Length must be a multiple of 3.
	*	@param	bCreateCollision	Indicates whether collision should be created for this section. This adds significant cost.
	*	@param	UpdateFrequency		Indicates how frequently the section will be updated. Allows the RMC to optimize itself to a particular use.
	*	@param	UpdateFlags			Flags pertaining to this particular update.
	*/
	template<typename VertexType>
	void CreateMeshSectionDualBuffer(TWeakObjectPtr<URuntimeMeshComponent> InRuntimeMeshComponent, int32 SectionIndex, TArray<FVector>& VertexPositions, 
		TArray<VertexType>& VertexData, TArray<int32>& Triangles, bool bCreateCollision = false,
		EUpdateFrequency UpdateFrequency = EUpdateFrequency::Average, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		struct FRMCAsyncData
		{
			int32 SectionIndex;
			TArray<FVector> VertexPositions;
			TArray<VertexType> Vertices;
			TArray<int32> Triangles;
			bool bCreateCollision;
			EUpdateFrequency UpdateFrequency;
			ESectionUpdateFlags UpdateFlags;
		};

		FRMCAsyncData* Data = new FRMCAsyncData;
		Data->SectionIndex = SectionIndex;

		if (!!(UpdateFlags & ESectionUpdateFlags::MoveArrays))
		{
			Data->VertexPositions = MoveTemp(VertexPositions);
			Data->Vertices = MoveTemp(Vertices);
			Data->Triangles = MoveTemp(Triangles);
		}
		else
		{
			Data->VertexPositions = VertexPositions;
			Data->Vertices = Vertices;
			Data->Triangles = Triangles;
		}
		Data->bCreateCollision = bCreateCollision;
		Data->UpdateFrequency = UpdateFrequency;
		Data->UpdateFlags = UpdateFlags | ESectionUpdateFlags::MoveArrays; // We can always use move arrays here since we either just copied it, or moved it from the original

		RUNTIMEMESHCOMPONENTASYNC_ENQUEUETASK(CreateMeshSection, FRMCAsyncData, Data, InRuntimeMeshComponent,
		{
			Mesh->CreateMeshSection(Data->SectionIndex, Data->VertexPositions, Data->Vertices, Data->Triangles, Data->bCreateCollision, Data->UpdateFrequency, Data->UpdateFlags);
		});
	}

	/**
	*	Create/replace a section using 2 vertex buffers. One contains positions only, the other contains all other data. This allows for very efficient updates of the positions of a mesh.
	*	@param	SectionIndex		Index of the section to create or replace.
	*	@param	VertexPositions		Vertex buffer containing only the position information for each vertex.
	*	@param	VertexData			Vertex buffer containing everything except position for each vertex.
	*	@param	Triangles			Index buffer indicating which vertices make up each triangle. Length must be a multiple of 3.
	*	@param	BoundingBox			The bounds of this section. Faster than the RMC automatically calculating it.
	*	@param	bCreateCollision	Indicates whether collision should be created for this section. This adds significant cost.
	*	@param	UpdateFrequency		Indicates how frequently the section will be updated. Allows the RMC to optimize itself to a particular use.
	*	@param	UpdateFlags			Flags pertaining to this particular update.
	*/
	template<typename VertexType>
	void CreateMeshSectionDualBuffer(TWeakObjectPtr<URuntimeMeshComponent> InRuntimeMeshComponent, int32 SectionIndex, TArray<FVector>& VertexPositions, 
		TArray<VertexType>& VertexData, TArray<int32>& Triangles, const FBox& BoundingBox,
		bool bCreateCollision = false, EUpdateFrequency UpdateFrequency = EUpdateFrequency::Average, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		struct FRMCAsyncData
		{
			int32 SectionIndex;
			TArray<FVector> VertexPositions;
			TArray<VertexType> Vertices;
			TArray<int32> Triangles;
			FBox BoundingBox;
			bool bCreateCollision;
			EUpdateFrequency UpdateFrequency;
			ESectionUpdateFlags UpdateFlags;
		};

		FRMCAsyncData* Data = new FRMCAsyncData;
		Data->SectionIndex = SectionIndex;

		if (!!(UpdateFlags & ESectionUpdateFlags::MoveArrays))
		{
			Data->VertexPositions = MoveTemp(VertexPositions);
			Data->Vertices = MoveTemp(Vertices);
			Data->Triangles = MoveTemp(Triangles);
		}
		else
		{
			Data->VertexPositions = VertexPositions;
			Data->Vertices = Vertices;
			Data->Triangles = Triangles;
		}
		Data->BoundingBox = BoundingBox;
		Data->bCreateCollision = bCreateCollision;
		Data->UpdateFrequency = UpdateFrequency;
		Data->UpdateFlags = UpdateFlags | ESectionUpdateFlags::MoveArrays; // We can always use move arrays here since we either just copied it, or moved it from the original

		RUNTIMEMESHCOMPONENTASYNC_ENQUEUETASK(CreateMeshSection, FRMCAsyncData, Data, InRuntimeMeshComponent,
		{
			Mesh->CreateMeshSection(Data->SectionIndex, Data->VertexPositions, Data->Vertices, Data->Triangles, Data->BoundingBox, Data->bCreateCollision, Data->UpdateFrequency, Data->UpdateFlags);
		});
	}


	/**
	*	Updates a section. This is faster than CreateMeshSection. If this is a dual buffer section, you cannot change the length of the vertices.
	*	@param	SectionIndex		Index of the section to update.
	*	@param	Vertices			Vertex buffer all vertex data for this section, or in the case of dual buffer section it contains everything but position.
	*	@param	UpdateFlags			Flags pertaining to this particular update.
	*/
	template<typename VertexType>
	void UpdateMeshSection(TWeakObjectPtr<URuntimeMeshComponent> InRuntimeMeshComponent, int32 SectionIndex, TArray<VertexType>& Vertices, 
		ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		struct FRMCAsyncData
		{
			int32 SectionIndex;
			TArray<VertexType> Vertices;
			ESectionUpdateFlags UpdateFlags;
		};

		FRMCAsyncData* Data = new FRMCAsyncData;
		Data->SectionIndex = SectionIndex;

		if (!!(UpdateFlags & ESectionUpdateFlags::MoveArrays))
		{
			Data->Vertices = MoveTemp(Vertices);
		}
		else
		{
			Data->Vertices = Vertices;
		}

		Data->UpdateFlags = UpdateFlags | ESectionUpdateFlags::MoveArrays; // We can always use move arrays here since we either just copied it, or moved it from the original

		RUNTIMEMESHCOMPONENTASYNC_ENQUEUETASK(UpdateMeshSection, FRMCAsyncData, Data, InRuntimeMeshComponent,
		{
			Mesh->UpdateMeshSection(Data->SectionIndex, Data->Vertices, Data->UpdateFlags);
		});
	}

	/**
	*	Updates a section. This is faster than CreateMeshSection. If this is a dual buffer section, you cannot change the length of the vertices.
	*	@param	SectionIndex		Index of the section to update.
	*	@param	Vertices			Vertex buffer all vertex data for this section, or in the case of dual buffer section it contains everything but position.
	*	@param	BoundingBox			The bounds of this section. Faster than the RMC automatically calculating it.
	*	@param	UpdateFlags			Flags pertaining to this particular update.
	*/
	template<typename VertexType>
	void UpdateMeshSection(TWeakObjectPtr<URuntimeMeshComponent> InRuntimeMeshComponent, int32 SectionIndex, TArray<VertexType>& Vertices, 
		const FBox& BoundingBox, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		struct FRMCAsyncData
		{
			int32 SectionIndex;
			TArray<VertexType> Vertices;
			FBox BoundingBox;
			ESectionUpdateFlags UpdateFlags;
		};

		FRMCAsyncData* Data = new FRMCAsyncData;
		Data->SectionIndex = SectionIndex;

		if (!!(UpdateFlags & ESectionUpdateFlags::MoveArrays))
		{
			Data->Vertices = MoveTemp(Vertices);
		}
		else
		{
			Data->Vertices = Vertices;
		}
		
		Data->BoundingBox = BoundingBox;
		Data->UpdateFlags = UpdateFlags | ESectionUpdateFlags::MoveArrays; // We can always use move arrays here since we either just copied it, or moved it from the original

		RUNTIMEMESHCOMPONENTASYNC_ENQUEUETASK(UpdateMeshSection, FRMCAsyncData, Data, InRuntimeMeshComponent,
		{
			Mesh->UpdateMeshSection(Data->SectionIndex, Data->Vertices, Data->BoundingBox, Data->UpdateFlags);
		});
	}

	/**
	*	Updates a section. This is faster than CreateMeshSection. If this is a dual buffer section, you cannot change the length of the vertices.
	*	@param	SectionIndex		Index of the section to update.
	*	@param	Vertices			Vertex buffer all vertex data for this section, or in the case of dual buffer section it contains everything but position.
	*	@param	Triangles			Index buffer indicating which vertices make up each triangle. Length must be a multiple of 3.
	*	@param	UpdateFlags			Flags pertaining to this particular update.
	*/
	template<typename VertexType>
	void UpdateMeshSection(TWeakObjectPtr<URuntimeMeshComponent> InRuntimeMeshComponent, int32 SectionIndex, TArray<VertexType>& Vertices, 
		TArray<int32>& Triangles, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		struct FRMCAsyncData
		{
			int32 SectionIndex;
			TArray<VertexType> Vertices;
			TArray<int32> Triangles;
			ESectionUpdateFlags UpdateFlags;
		};

		FRMCAsyncData* Data = new FRMCAsyncData;
		Data->SectionIndex = SectionIndex;

		if (!!(UpdateFlags & ESectionUpdateFlags::MoveArrays))
		{
			Data->Vertices = MoveTemp(Vertices);
			Data->Triangles = MoveTemp(Triangles);
		}
		else
		{
			Data->Vertices = Vertices;
			Data->Triangles = Triangles;
		}

		Data->UpdateFlags = UpdateFlags | ESectionUpdateFlags::MoveArrays; // We can always use move arrays here since we either just copied it, or moved it from the original

		RUNTIMEMESHCOMPONENTASYNC_ENQUEUETASK(UpdateMeshSection, FRMCAsyncData, Data, InRuntimeMeshComponent,
		{
			Mesh->UpdateMeshSection(Data->SectionIndex, Data->Vertices, Data->Triangles, Data->UpdateFlags);
		});
	}

	/**
	*	Updates a section. This is faster than CreateMeshSection. If this is a dual buffer section, you cannot change the length of the vertices.
	*	@param	SectionIndex		Index of the section to update.
	*	@param	Vertices			Vertex buffer all vertex data for this section, or in the case of dual buffer section it contains everything but position.
	*	@param	Triangles			Index buffer indicating which vertices make up each triangle. Length must be a multiple of 3.
	*	@param	BoundingBox			The bounds of this section. Faster than the RMC automatically calculating it.
	*	@param	UpdateFlags			Flags pertaining to this particular update.
	*/
	template<typename VertexType>
	void UpdateMeshSection(TWeakObjectPtr<URuntimeMeshComponent> InRuntimeMeshComponent, int32 SectionIndex, TArray<VertexType>& Vertices, 
		TArray<int32>& Triangles, const FBox& BoundingBox, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		struct FRMCAsyncData
		{
			int32 SectionIndex;
			TArray<VertexType> Vertices;
			TArray<int32> Triangles;
			FBox BoundingBox;
			ESectionUpdateFlags UpdateFlags;
		};

		FRMCAsyncData* Data = new FRMCAsyncData;
		Data->SectionIndex = SectionIndex;

		if (!!(UpdateFlags & ESectionUpdateFlags::MoveArrays))
		{
			Data->Vertices = MoveTemp(Vertices);
			Data->Triangles = MoveTemp(Triangles);
		}
		else
		{
			Data->Vertices = Vertices;
			Data->Triangles = Triangles;
		}

		Data->BoundingBox = BoundingBox;
		Data->UpdateFlags = UpdateFlags | ESectionUpdateFlags::MoveArrays; // We can always use move arrays here since we either just copied it, or moved it from the original

		RUNTIMEMESHCOMPONENTASYNC_ENQUEUETASK(UpdateMeshSection, FRMCAsyncData, Data, InRuntimeMeshComponent,
		{
			Mesh->UpdateMeshSection(Data->SectionIndex, Data->Vertices, Data->Triangles, Data->BoundingBox, Data->UpdateFlags);
		});
	}


	/**
	*	Updates a section. This is faster than CreateMeshSection. This is only for dual buffer sections. You cannot change the length of positions or vertex data unless you specify both together.
	*	@param	SectionIndex		Index of the section to update.
	*	@param	VertexPositions		Vertex buffer containing only the position information for each vertex.
	*	@param	Vertices			Vertex buffer containing everything except position for each vertex.
	*	@param	UpdateFlags			Flags pertaining to this particular update.
	*/
	template<typename VertexType>
	void UpdateMeshSection(TWeakObjectPtr<URuntimeMeshComponent> InRuntimeMeshComponent, int32 SectionIndex, TArray<FVector>& VertexPositions, 
		TArray<VertexType>& Vertices, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		struct FRMCAsyncData
		{
			int32 SectionIndex;
			TArray<FVector> VertexPositions;
			TArray<VertexType> Vertices;
			ESectionUpdateFlags UpdateFlags;
		};

		FRMCAsyncData* Data = new FRMCAsyncData;
		Data->SectionIndex = SectionIndex;

		if (!!(UpdateFlags & ESectionUpdateFlags::MoveArrays))
		{
			Data->VertexPositions = MoveTemp(VertexPositions);
			Data->Vertices = MoveTemp(Vertices);
		}
		else
		{
			Data->VertexPositions = VertexPositions;
			Data->Vertices = Vertices;
		}

		Data->UpdateFlags = UpdateFlags | ESectionUpdateFlags::MoveArrays; // We can always use move arrays here since we either just copied it, or moved it from the original

		RUNTIMEMESHCOMPONENTASYNC_ENQUEUETASK(UpdateMeshSection, FRMCAsyncData, Data, InRuntimeMeshComponent,
		{
			Mesh->UpdateMeshSection(Data->SectionIndex, Data->VertexPositions, Data->Vertices, Data->UpdateFlags);
		});
	}

	/**
	*	Updates a section. This is faster than CreateMeshSection. This is only for dual buffer sections. You cannot change the length of positions or vertex data unless you specify both together.
	*	@param	SectionIndex		Index of the section to update.
	*	@param	VertexPositions		Vertex buffer containing only the position information for each vertex.
	*	@param	Vertices			Vertex buffer containing everything except position for each vertex.
	*	@param	BoundingBox			The bounds of this section. Faster than the RMC automatically calculating it.
	*	@param	UpdateFlags			Flags pertaining to this particular update.
	*/
	template<typename VertexType>
	void UpdateMeshSection(TWeakObjectPtr<URuntimeMeshComponent> InRuntimeMeshComponent, int32 SectionIndex, TArray<FVector>& VertexPositions, 
		TArray<VertexType>& Vertices, const FBox& BoundingBox, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		struct FRMCAsyncData
		{
			int32 SectionIndex;
			TArray<FVector> VertexPositions;
			TArray<VertexType> Vertices;
			FBox BoundingBox;
			ESectionUpdateFlags UpdateFlags;
		};

		FRMCAsyncData* Data = new FRMCAsyncData;
		Data->SectionIndex = SectionIndex;

		if (!!(UpdateFlags & ESectionUpdateFlags::MoveArrays))
		{
			Data->VertexPositions = MoveTemp(VertexPositions);
			Data->Vertices = MoveTemp(Vertices);
		}
		else
		{
			Data->VertexPositions = VertexPositions;
			Data->Vertices = Vertices;
		}

		Data->BoundingBox = BoundingBox;
		Data->UpdateFlags = UpdateFlags | ESectionUpdateFlags::MoveArrays; // We can always use move arrays here since we either just copied it, or moved it from the original

		RUNTIMEMESHCOMPONENTASYNC_ENQUEUETASK(UpdateMeshSection, FRMCAsyncData, Data, InRuntimeMeshComponent,
		{
			Mesh->UpdateMeshSection(Data->SectionIndex, Data->VertexPositions, Data->Vertices, Data->BoundingBox, Data->UpdateFlags);
		});
	}

	/**
	*	Updates a section. This is faster than CreateMeshSection. This is only for dual buffer sections. You cannot change the length of positions or vertex data unless you specify both together.
	*	@param	SectionIndex		Index of the section to update.
	*	@param	VertexPositions		Vertex buffer containing only the position information for each vertex.
	*	@param	Vertices			Vertex buffer containing everything except position for each vertex.
	*	@param	Triangles			Index buffer indicating which vertices make up each triangle. Length must be a multiple of 3.
	*	@param	UpdateFlags			Flags pertaining to this particular update.
	*/
	template<typename VertexType>
	void UpdateMeshSection(TWeakObjectPtr<URuntimeMeshComponent> InRuntimeMeshComponent, int32 SectionIndex, TArray<FVector>& VertexPositions, 
		TArray<VertexType>& Vertices, TArray<int32>& Triangles, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		struct FRMCAsyncData
		{
			int32 SectionIndex;
			TArray<FVector> VertexPositions;
			TArray<VertexType> Vertices;
			TArray<int32> Triangles;
			ESectionUpdateFlags UpdateFlags;
		};

		FRMCAsyncData* Data = new FRMCAsyncData;
		Data->SectionIndex = SectionIndex;

		if (!!(UpdateFlags & ESectionUpdateFlags::MoveArrays))
		{
			Data->VertexPositions = MoveTemp(VertexPositions);
			Data->Vertices = MoveTemp(Vertices);
			Data->Triangles = MoveTemp(Triangles);
		}
		else
		{
			Data->VertexPositions = VertexPositions;
			Data->Vertices = Vertices;
			Data->Triangles = Triangles;
		}

		Data->UpdateFlags = UpdateFlags | ESectionUpdateFlags::MoveArrays; // We can always use move arrays here since we either just copied it, or moved it from the original

		RUNTIMEMESHCOMPONENTASYNC_ENQUEUETASK(UpdateMeshSection, FRMCAsyncData, Data, InRuntimeMeshComponent,
		{
			Mesh->UpdateMeshSection(Data->SectionIndex, Data->VertexPositions, Data->Vertices, Data->Triangles, Data->UpdateFlags);
		});
	}

	/**
	*	Updates a section. This is faster than CreateMeshSection. This is only for dual buffer sections. You cannot change the length of positions or vertex data unless you specify both together.
	*	@param	SectionIndex		Index of the section to update.
	*	@param	VertexPositions		Vertex buffer containing only the position information for each vertex.
	*	@param	Vertices			Vertex buffer containing everything except position for each vertex.
	*	@param	Triangles			Index buffer indicating which vertices make up each triangle. Length must be a multiple of 3.
	*	@param	BoundingBox			The bounds of this section. Faster than the RMC automatically calculating it.
	*	@param	UpdateFlags			Flags pertaining to this particular update.
	*/
	template<typename VertexType>
	void UpdateMeshSection(TWeakObjectPtr<URuntimeMeshComponent> InRuntimeMeshComponent, int32 SectionIndex, TArray<FVector>& VertexPositions, 
		TArray<VertexType>& Vertices, TArray<int32>& Triangles, const FBox& BoundingBox, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		struct FRMCAsyncData
		{
			int32 SectionIndex;
			TArray<FVector> VertexPositions;
			TArray<VertexType> Vertices;
			TArray<int32> Triangles;
			FBox BoundingBox;
			ESectionUpdateFlags UpdateFlags;
		};

		FRMCAsyncData* Data = new FRMCAsyncData;
		Data->SectionIndex = SectionIndex;

		if (!!(UpdateFlags & ESectionUpdateFlags::MoveArrays))
		{
			Data->VertexPositions = MoveTemp(VertexPositions);
			Data->Vertices = MoveTemp(Vertices);
			Data->Triangles = MoveTemp(Triangles);
		}
		else
		{
			Data->VertexPositions = VertexPositions;
			Data->Vertices = Vertices;
			Data->Triangles = Triangles;
		}

		Data->BoundingBox = BoundingBox;
		Data->UpdateFlags = UpdateFlags | ESectionUpdateFlags::MoveArrays; // We can always use move arrays here since we either just copied it, or moved it from the original

		RUNTIMEMESHCOMPONENTASYNC_ENQUEUETASK(UpdateMeshSection, FRMCAsyncData, Data, InRuntimeMeshComponent,
		{
			Mesh->UpdateMeshSection(Data->SectionIndex, Data->VertexPositions, Data->Vertices, Data->Triangles, Data->BoundingBox, Data->UpdateFlags);
		});
	}


	/**
	*	Updates a sections position buffer only. This cannot be used on a non-dual buffer section. You cannot change the length of the vertex position buffer with this function.
	*	@param	SectionIndex		Index of the section to update.
	*	@param	VertexPositions		Vertex buffer containing only the position information for each vertex.
	*	@param	UpdateFlags			Flags pertaining to this particular update.
	*/
	void UpdateMeshSectionPositionsImmediate(TWeakObjectPtr<URuntimeMeshComponent> InRuntimeMeshComponent, int32 SectionIndex,
		TArray<FVector>& VertexPositions, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		struct FRMCAsyncData
		{
			int32 SectionIndex;
			TArray<FVector> VertexPositions;
			ESectionUpdateFlags UpdateFlags;
		};

		FRMCAsyncData* Data = new FRMCAsyncData;
		Data->SectionIndex = SectionIndex;

		if (!!(UpdateFlags & ESectionUpdateFlags::MoveArrays))
		{
			Data->VertexPositions = MoveTemp(VertexPositions);
		}
		else
		{
			Data->VertexPositions = VertexPositions;
		}

		Data->UpdateFlags = UpdateFlags | ESectionUpdateFlags::MoveArrays; // We can always use move arrays here since we either just copied it, or moved it from the original

		RUNTIMEMESHCOMPONENTASYNC_ENQUEUETASK(UpdateMeshSectionPositionsImmediate, FRMCAsyncData, Data, InRuntimeMeshComponent,
		{
			Mesh->UpdateMeshSectionPositionsImmediate(Data->SectionIndex, Data->VertexPositions, Data->UpdateFlags);
		});
	}

	/**
	*	Updates a sections position buffer only. This cannot be used on a non-dual buffer section. You cannot change the length of the vertex position buffer with this function.
	*	@param	SectionIndex		Index of the section to update.
	*	@param	VertexPositions		Vertex buffer containing only the position information for each vertex.
	*	@param	BoundingBox			The bounds of this section. Faster than the RMC automatically calculating it.
	*	@param	UpdateFlags			Flags pertaining to this particular update.
	*/
	void UpdateMeshSectionPositionsImmediate(TWeakObjectPtr<URuntimeMeshComponent> InRuntimeMeshComponent, int32 SectionIndex,
		TArray<FVector>& VertexPositions, const FBox& BoundingBox, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		struct FRMCAsyncData
		{
			int32 SectionIndex;
			TArray<FVector> VertexPositions;
			FBox BoundingBox;
			ESectionUpdateFlags UpdateFlags;
		};

		FRMCAsyncData* Data = new FRMCAsyncData;
		Data->SectionIndex = SectionIndex;

		if (!!(UpdateFlags & ESectionUpdateFlags::MoveArrays))
		{
			Data->VertexPositions = MoveTemp(VertexPositions);
		}
		else
		{
			Data->VertexPositions = VertexPositions;
		}
		
		Data->BoundingBox = BoundingBox;
		Data->UpdateFlags = UpdateFlags | ESectionUpdateFlags::MoveArrays; // We can always use move arrays here since we either just copied it, or moved it from the original

		RUNTIMEMESHCOMPONENTASYNC_ENQUEUETASK(UpdateMeshSectionPositionsImmediate, FRMCAsyncData, Data, InRuntimeMeshComponent,
		{
			Mesh->UpdateMeshSectionPositionsImmediate(Data->SectionIndex, Data->VertexPositions, Data->BoundingBox, Data->UpdateFlags);
		});
	}





	/**
	*	Create/replace a section.
	*	@param	SectionIndex		Index of the section to create or replace.
	*	@param	Vertices			Vertex buffer of all vertex positions to use for this mesh section.
	*	@param	Triangles			Index buffer indicating which vertices make up each triangle. Length must be a multiple of 3.
	*	@param	Normals				Optional array of normal vectors for each vertex. If supplied, must be same length as Vertices array.
	*	@param	UV0					Optional array of texture co-ordinates for each vertex (UV Channel 0). If supplied, must be same length as Vertices array.
	*	@param	Colors				Optional array of colors for each vertex. If supplied, must be same length as Vertices array.
	*	@param	Tangents			Optional array of tangent vector for each vertex. If supplied, must be same length as Vertices array.
	*	@param	bCreateCollision	Indicates whether collision should be created for this section. This adds significant cost.
	*	@param	UpdateFrequency		Indicates how frequently the section will be updated. Allows the RMC to optimize itself to a particular use.
	*/
	void CreateMeshSection(TWeakObjectPtr<URuntimeMeshComponent> InRuntimeMeshComponent, int32 SectionIndex, const TArray<FVector>& Vertices,
		const TArray<int32>& Triangles, const TArray<FVector>& Normals, const TArray<FVector2D>& UV0, const TArray<FColor>& Colors,
		const TArray<FRuntimeMeshTangent>& Tangents, bool bCreateCollision = false,
		EUpdateFrequency UpdateFrequency = EUpdateFrequency::Average, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		struct FRMCAsyncData
		{
			int32 SectionIndex;
			TArray<FVector> Vertices;
			TArray<int32> Triangles;
			TArray<FVector> Normals;
			TArray<FVector2D> UV0;
			TArray<FColor> Colors;
			TArray<FRuntimeMeshTangent> Tangents;
			bool bCreateCollision = false;
			EUpdateFrequency UpdateFrequency;
			ESectionUpdateFlags UpdateFlags;
		};

		FRMCAsyncData* Data = new FRMCAsyncData;
		Data->SectionIndex = SectionIndex;

		if (!!(UpdateFlags & ESectionUpdateFlags::MoveArrays))
		{
			Data->Vertices = MoveTemp(Vertices);
			Data->Triangles = MoveTemp(Triangles);
			Data->Normals = MoveTemp(Normals);
			Data->UV0 = MoveTemp(UV0);
			Data->Colors = MoveTemp(Colors);
			Data->Tangents = MoveTemp(Tangents);
		}
		else
		{
			Data->Vertices = Vertices;
			Data->Triangles = Triangles;
			Data->Normals = Normals;
			Data->UV0 = UV0;
			Data->Colors = Colors;
			Data->Tangents = Tangents;
		}

		Data->bCreateCollision = bCreateCollision;
		Data->UpdateFrequency = UpdateFrequency;
		Data->UpdateFlags = UpdateFlags | ESectionUpdateFlags::MoveArrays; // We can always use move arrays here since we either just copied it, or moved it from the original

		RUNTIMEMESHCOMPONENTASYNC_ENQUEUETASK(CreateMeshSection, FRMCAsyncData, Data, InRuntimeMeshComponent,
		{
			Mesh->CreateMeshSection(Data->SectionIndex, Data->Vertices, Data->Triangles, Data->Normals, Data->UV0, Data->Colors, 
			Data->Tangents, Data->bCreateCollision, Data->UpdateFrequency, Data->UpdateFlags);
		});
	}

	/**
	*	Create/replace a section.
	*	@param	SectionIndex		Index of the section to create or replace.
	*	@param	Vertices			Vertex buffer of all vertex positions to use for this mesh section.
	*	@param	Triangles			Index buffer indicating which vertices make up each triangle. Length must be a multiple of 3.
	*	@param	Normals				Optional array of normal vectors for each vertex. If supplied, must be same length as Vertices array.
	*	@param	UV0					Optional array of texture co-ordinates for each vertex (UV Channel 0). If supplied, must be same length as Vertices array.
	*	@param	UV1					Optional array of texture co-ordinates for each vertex (UV Channel 1). If supplied, must be same length as Vertices array.
	*	@param	Colors				Optional array of colors for each vertex. If supplied, must be same length as Vertices array.
	*	@param	Tangents			Optional array of tangent vector for each vertex. If supplied, must be same length as Vertices array.
	*	@param	bCreateCollision	Indicates whether collision should be created for this section. This adds significant cost.
	*	@param	UpdateFrequency		Indicates how frequently the section will be updated. Allows the RMC to optimize itself to a particular use.
	*/
	void CreateMeshSection(TWeakObjectPtr<URuntimeMeshComponent> InRuntimeMeshComponent, int32 SectionIndex, const TArray<FVector>& Vertices, 
		const TArray<int32>& Triangles, const TArray<FVector>& Normals,	const TArray<FVector2D>& UV0, const TArray<FVector2D>& UV1, 
		const TArray<FColor>& Colors, const TArray<FRuntimeMeshTangent>& Tangents,
		bool bCreateCollision = false, EUpdateFrequency UpdateFrequency = EUpdateFrequency::Average, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		struct FRMCAsyncData
		{
			int32 SectionIndex;
			TArray<FVector> Vertices;
			TArray<int32> Triangles;
			TArray<FVector> Normals;
			TArray<FVector2D> UV0;
			TArray<FVector2D> UV1;
			TArray<FColor> Colors;
			TArray<FRuntimeMeshTangent> Tangents;
			bool bCreateCollision = false;
			EUpdateFrequency UpdateFrequency;
			ESectionUpdateFlags UpdateFlags;
		};

		FRMCAsyncData* Data = new FRMCAsyncData;
		Data->SectionIndex = SectionIndex;

		if (!!(UpdateFlags & ESectionUpdateFlags::MoveArrays))
		{
			Data->Vertices = MoveTemp(Vertices);
			Data->Triangles = MoveTemp(Triangles);
			Data->Normals = MoveTemp(Normals);
			Data->UV0 = MoveTemp(UV0);
			Data->UV1 = MoveTemp(UV1);
			Data->Colors = MoveTemp(Colors);
			Data->Tangents = MoveTemp(Tangents);
		}
		else
		{
			Data->Vertices = Vertices;
			Data->Triangles = Triangles;
			Data->Normals = Normals;
			Data->UV0 = UV0;
			Data->UV1 = UV1;
			Data->Colors = Colors;
			Data->Tangents = Tangents;
		}

		Data->bCreateCollision = bCreateCollision;
		Data->UpdateFrequency = UpdateFrequency;
		Data->UpdateFlags = UpdateFlags | ESectionUpdateFlags::MoveArrays; // We can always use move arrays here since we either just copied it, or moved it from the original

		RUNTIMEMESHCOMPONENTASYNC_ENQUEUETASK(CreateMeshSection, FRMCAsyncData, Data, InRuntimeMeshComponent,
		{
			Mesh->CreateMeshSection(Data->SectionIndex, Data->Vertices, Data->Triangles, Data->Normals, Data->UV0, Data->UV1, Data->Colors,
			Data->Tangents, Data->bCreateCollision, Data->UpdateFrequency, Data->UpdateFlags);
		});
	}


	/**
	*	Updates a section. This is faster than CreateMeshSection.
	*	@param	SectionIndex		Index of the section to update.
	*	@param	Vertices			Vertex buffer of all vertex positions to use for this mesh section.
	*	@param	Normals				Optional array of normal vectors for each vertex. If supplied, must be same length as Vertices array.
	*	@param	UV1					Optional array of texture co-ordinates for each vertex (UV Channel 1). If supplied, must be same length as Vertices array.
	*	@param	Colors				Optional array of colors for each vertex. If supplied, must be same length as Vertices array.
	*	@param	Tangents			Optional array of tangent vector for each vertex. If supplied, must be same length as Vertices array.
	*/
	void UpdateMeshSection(TWeakObjectPtr<URuntimeMeshComponent> InRuntimeMeshComponent, int32 SectionIndex, const TArray<FVector>& Vertices, 
		const TArray<FVector>& Normals, const TArray<FVector2D>& UV0, const TArray<FColor>& Colors, const TArray<FRuntimeMeshTangent>& Tangents, 
		ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		struct FRMCAsyncData
		{
			int32 SectionIndex;
			TArray<FVector> Vertices;
			TArray<FVector> Normals;
			TArray<FVector2D> UV0;
			TArray<FVector2D> UV1;
			TArray<FColor> Colors;
			TArray<FRuntimeMeshTangent> Tangents;
			bool bCreateCollision = false;
			EUpdateFrequency UpdateFrequency;
			ESectionUpdateFlags UpdateFlags;
		};

		FRMCAsyncData* Data = new FRMCAsyncData;
		Data->SectionIndex = SectionIndex;

		if (!!(UpdateFlags & ESectionUpdateFlags::MoveArrays))
		{
			Data->Vertices = MoveTemp(Vertices);
			Data->Normals = MoveTemp(Normals);
			Data->UV0 = MoveTemp(UV0);
			Data->Colors = MoveTemp(Colors);
			Data->Tangents = MoveTemp(Tangents);
		}
		else
		{
			Data->Vertices = Vertices;
			Data->Normals = Normals;
			Data->UV0 = UV0;
			Data->Colors = Colors;
			Data->Tangents = Tangents;
		}

		Data->UpdateFlags = UpdateFlags | ESectionUpdateFlags::MoveArrays; // We can always use move arrays here since we either just copied it, or moved it from the original

		RUNTIMEMESHCOMPONENTASYNC_ENQUEUETASK(UpdateMeshSection, FRMCAsyncData, Data, InRuntimeMeshComponent,
		{
			Mesh->UpdateMeshSection(Data->SectionIndex, Data->Vertices, Data->Normals, Data->UV0, Data->Colors,	Data->Tangents, Data->UpdateFlags);
		});
	}

	/**
	*	Updates a section. This is faster than CreateMeshSection.
	*	@param	SectionIndex		Index of the section to update.
	*	@param	Vertices			Vertex buffer of all vertex positions to use for this mesh section.
	*	@param	Normals				Optional array of normal vectors for each vertex. If supplied, must be same length as Vertices array.
	*	@param	UV0					Optional array of texture co-ordinates for each vertex (UV Channel 0). If supplied, must be same length as Vertices array.
	*	@param	UV1					Optional array of texture co-ordinates for each vertex (UV Channel 1). If supplied, must be same length as Vertices array.
	*	@param	Colors				Optional array of colors for each vertex. If supplied, must be same length as Vertices array.
	*	@param	Tangents			Optional array of tangent vector for each vertex. If supplied, must be same length as Vertices array.
	*/
	void UpdateMeshSection(TWeakObjectPtr<URuntimeMeshComponent> InRuntimeMeshComponent, int32 SectionIndex, const TArray<FVector>& Vertices, 
		const TArray<FVector>& Normals, const TArray<FVector2D>& UV0, const TArray<FVector2D>& UV1, const TArray<FColor>& Colors, 
		const TArray<FRuntimeMeshTangent>& Tangents, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		struct FRMCAsyncData
		{
			int32 SectionIndex;
			TArray<FVector> Vertices;
			TArray<FVector> Normals;
			TArray<FVector2D> UV0;
			TArray<FVector2D> UV1;
			TArray<FColor> Colors;
			TArray<FRuntimeMeshTangent> Tangents;
			bool bCreateCollision = false;
			EUpdateFrequency UpdateFrequency;
			ESectionUpdateFlags UpdateFlags;
		};

		FRMCAsyncData* Data = new FRMCAsyncData;
		Data->SectionIndex = SectionIndex;

		if (!!(UpdateFlags & ESectionUpdateFlags::MoveArrays))
		{
			Data->Vertices = MoveTemp(Vertices);
			Data->Normals = MoveTemp(Normals);
			Data->UV0 = MoveTemp(UV0);
			Data->UV1 = MoveTemp(UV1);
			Data->Colors = MoveTemp(Colors);
			Data->Tangents = MoveTemp(Tangents);
		}
		else
		{
			Data->Vertices = Vertices;
			Data->Normals = Normals;
			Data->UV0 = UV0;
			Data->UV1 = UV1;
			Data->Colors = Colors;
			Data->Tangents = Tangents;
		}

		Data->UpdateFlags = UpdateFlags | ESectionUpdateFlags::MoveArrays; // We can always use move arrays here since we either just copied it, or moved it from the original

		RUNTIMEMESHCOMPONENTASYNC_ENQUEUETASK(UpdateMeshSection, FRMCAsyncData, Data, InRuntimeMeshComponent,
		{
			Mesh->UpdateMeshSection(Data->SectionIndex, Data->Vertices, Data->Normals, Data->UV0, Data->UV1, Data->Colors, Data->Tangents, Data->UpdateFlags);
		});
	}

	/**
	*	Updates a section. This is faster than CreateMeshSection.
	*	@param	SectionIndex		Index of the section to update.
	*	@param	Vertices			Vertex buffer of all vertex positions to use for this mesh section.
	*	@param	Triangles			Index buffer indicating which vertices make up each triangle. Length must be a multiple of 3.
	*	@param	Normals				Optional array of normal vectors for each vertex. If supplied, must be same length as Vertices array.
	*	@param	UV0					Optional array of texture co-ordinates for each vertex (UV Channel 0). If supplied, must be same length as Vertices array.
	*	@param	Colors				Optional array of colors for each vertex. If supplied, must be same length as Vertices array.
	*	@param	Tangents			Optional array of tangent vector for each vertex. If supplied, must be same length as Vertices array.
	*/
	void UpdateMeshSection(TWeakObjectPtr<URuntimeMeshComponent> InRuntimeMeshComponent, int32 SectionIndex, const TArray<FVector>& Vertices, 
		const TArray<int32>& Triangles, const TArray<FVector>& Normals,	const TArray<FVector2D>& UV0, const TArray<FColor>& Colors, 
		const TArray<FRuntimeMeshTangent>& Tangents, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		struct FRMCAsyncData
		{
			int32 SectionIndex;
			TArray<FVector> Vertices;
			TArray<int32> Triangles;
			TArray<FVector> Normals;
			TArray<FVector2D> UV0;
			TArray<FColor> Colors;
			TArray<FRuntimeMeshTangent> Tangents;
			bool bCreateCollision = false;
			EUpdateFrequency UpdateFrequency;
			ESectionUpdateFlags UpdateFlags;
		};

		FRMCAsyncData* Data = new FRMCAsyncData;
		Data->SectionIndex = SectionIndex;

		if (!!(UpdateFlags & ESectionUpdateFlags::MoveArrays))
		{
			Data->Vertices = MoveTemp(Vertices);
			Data->Triangles = MoveTemp(Triangles);
			Data->Normals = MoveTemp(Normals);
			Data->UV0 = MoveTemp(UV0);
			Data->Colors = MoveTemp(Colors);
			Data->Tangents = MoveTemp(Tangents);
		}
		else
		{
			Data->Vertices = Vertices;
			Data->Triangles = Triangles;
			Data->Normals = Normals;
			Data->UV0 = UV0;
			Data->Colors = Colors;
			Data->Tangents = Tangents;
		}

		Data->UpdateFlags = UpdateFlags | ESectionUpdateFlags::MoveArrays; // We can always use move arrays here since we either just copied it, or moved it from the original

		RUNTIMEMESHCOMPONENTASYNC_ENQUEUETASK(UpdateMeshSection, FRMCAsyncData, Data, InRuntimeMeshComponent,
		{
			Mesh->UpdateMeshSection(Data->SectionIndex, Data->Vertices, Data->Triangles, Data->Normals, Data->UV0, Data->Colors, Data->Tangents, Data->UpdateFlags);
		});
	}

	/**
	*	Updates a section. This is faster than CreateMeshSection.
	*	@param	SectionIndex		Index of the section to update.
	*	@param	Vertices			Vertex buffer of all vertex positions to use for this mesh section.
	*	@param	Triangles			Index buffer indicating which vertices make up each triangle. Length must be a multiple of 3.
	*	@param	Normals				Optional array of normal vectors for each vertex. If supplied, must be same length as Vertices array.
	*	@param	UV0					Optional array of texture co-ordinates for each vertex (UV Channel 0). If supplied, must be same length as Vertices array.
	*	@param	UV1					Optional array of texture co-ordinates for each vertex (UV Channel 1). If supplied, must be same length as Vertices array.
	*	@param	Colors				Optional array of colors for each vertex. If supplied, must be same length as Vertices array.
	*	@param	Tangents			Optional array of tangent vector for each vertex. If supplied, must be same length as Vertices array.
	*/
	void UpdateMeshSection(TWeakObjectPtr<URuntimeMeshComponent> InRuntimeMeshComponent, int32 SectionIndex, const TArray<FVector>& Vertices, 
		const TArray<int32>& Triangles, const TArray<FVector>& Normals,	const TArray<FVector2D>& UV0, const TArray<FVector2D>& UV1, 
		const TArray<FColor>& Colors, const TArray<FRuntimeMeshTangent>& Tangents, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		struct FRMCAsyncData
		{
			int32 SectionIndex;
			TArray<FVector> Vertices;
			TArray<int32> Triangles;
			TArray<FVector> Normals;
			TArray<FVector2D> UV0;
			TArray<FVector2D> UV1;
			TArray<FColor> Colors;
			TArray<FRuntimeMeshTangent> Tangents;
			bool bCreateCollision = false;
			EUpdateFrequency UpdateFrequency;
			ESectionUpdateFlags UpdateFlags;
		};

		FRMCAsyncData* Data = new FRMCAsyncData;
		Data->SectionIndex = SectionIndex;

		if (!!(UpdateFlags & ESectionUpdateFlags::MoveArrays))
		{
			Data->Vertices = MoveTemp(Vertices);
			Data->Triangles = MoveTemp(Triangles);
			Data->Normals = MoveTemp(Normals);
			Data->UV0 = MoveTemp(UV0);
			Data->UV1 = MoveTemp(UV1);
			Data->Colors = MoveTemp(Colors);
			Data->Tangents = MoveTemp(Tangents);
		}
		else
		{
			Data->Vertices = Vertices;
			Data->Triangles = Triangles;
			Data->Normals = Normals;
			Data->UV0 = UV0;
			Data->UV1 = UV1;
			Data->Colors = Colors;
			Data->Tangents = Tangents;
		}

		Data->UpdateFlags = UpdateFlags | ESectionUpdateFlags::MoveArrays; // We can always use move arrays here since we either just copied it, or moved it from the original

		RUNTIMEMESHCOMPONENTASYNC_ENQUEUETASK(UpdateMeshSection, FRMCAsyncData, Data, InRuntimeMeshComponent,
		{
			Mesh->UpdateMeshSection(Data->SectionIndex, Data->Vertices, Data->Triangles, Data->Normals, Data->UV0, Data->UV1, Data->Colors,	Data->Tangents, Data->UpdateFlags);
		});
	}

};
