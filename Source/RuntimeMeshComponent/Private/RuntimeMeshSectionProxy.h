// Copyright 2016-2019 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "Engine/Engine.h"
#include "Components/MeshComponent.h"
#include "RuntimeMeshRendering.h"
#include "RuntimeMeshRenderable.h"

class FRuntimeMeshSectionProxy;


struct FRuntimeMeshSectionNullBufferElement
{
	FPackedNormal Normal;
	FPackedNormal Tangent;
	FColor Color;
	FVector2DHalf UV0;

	FRuntimeMeshSectionNullBufferElement()
		: Normal(FVector(0.0f, 0.0f, 1.0f))
		, Tangent(FVector(1.0f, 0.0f, 0.0f))
		, Color(FColor::Transparent)
		, UV0(FVector2D::ZeroVector)
	{ }
};


class FRuntimeMeshSectionProxy
{
	/** This stores all this sections config*/
	FRuntimeMeshSectionProperties Properties;

	/** Feature level in use, needed for recreating LODs vertex factories */
	ERHIFeatureLevel::Type FeatureLevel;

	/** Vertex factory for this section */
	FRuntimeMeshVertexFactory VertexFactory;

	/** Vertex buffer containing the positions for this section */
	FRuntimeMeshPositionVertexBuffer PositionBuffer;

	/** Vertex buffer containing the tangents for this section */
	FRuntimeMeshTangentsVertexBuffer TangentsBuffer;

	/** Vertex buffer containing the UVs for this section */
	FRuntimeMeshUVsVertexBuffer UVsBuffer;

	/** Vertex buffer containing the colors for this section */
	FRuntimeMeshColorVertexBuffer ColorBuffer;

	/** Index buffer for this section */
	FRuntimeMeshIndexBuffer IndexBuffer;

	/** Index buffer for this section */
	FRuntimeMeshIndexBuffer AdjacencyIndexBuffer;

public:

	FRuntimeMeshSectionProxy(ERHIFeatureLevel::Type InFeatureLevel, const FRuntimeMeshSectionProperties& SectionProperties)
		: Properties(SectionProperties)
		, FeatureLevel(InFeatureLevel)
		, VertexFactory(InFeatureLevel, this)
		, PositionBuffer(SectionProperties.UpdateFrequency)
		, TangentsBuffer(SectionProperties.UpdateFrequency, SectionProperties.bUseHighPrecisionTangents)
		, UVsBuffer(SectionProperties.UpdateFrequency, SectionProperties.bUseHighPrecisionTexCoords, SectionProperties.NumTexCoords)
		, ColorBuffer(SectionProperties.UpdateFrequency)
		, IndexBuffer(SectionProperties.UpdateFrequency, SectionProperties.bWants32BitIndices)
		, AdjacencyIndexBuffer(SectionProperties.UpdateFrequency, SectionProperties.bWants32BitIndices)
	{

	}

	~FRuntimeMeshSectionProxy()
	{
		check(IsInRenderingThread());

		Reset();
	}
	
	void Reset();

	bool CanRender() const;
	bool ShouldRender() const { return Properties.bIsVisible && CanRender(); }
	bool WantsToRenderInStaticPath() const { return Properties.UpdateFrequency == ERuntimeMeshUpdateFrequency::Infrequent; }
	bool CastsShadow() const { return Properties.bCastsShadow; }

	FRuntimeMeshVertexFactory* GetVertexFactory() { return &VertexFactory; }
	void BuildVertexDataType(FLocalVertexFactory::FDataType& DataType);

	void CreateMeshBatch(FMeshBatch& MeshBatch, bool bCastsShadow, bool bWantsAdjacencyInfo) const;

	void UpdateProperties_RenderThread(const FRuntimeMeshSectionProperties& SectionProperties);
	void UpdateSection_RenderThread(const FRuntimeMeshRenderableMeshData& MeshData);
	void ClearSection_RenderThread();
};




class FRuntimeMeshLODProxy : public TSharedFromThis<FRuntimeMeshLODProxy>
{
	/** Sections for this LOD, these do not have to be configured the same as sections in other LODs */
	TMap<int32, FRuntimeMeshSectionProxyPtr> Sections;

	FRuntimeMeshLODProperties Properties;

	ERHIFeatureLevel::Type FeatureLevel;

public:
	FRuntimeMeshLODProxy(ERHIFeatureLevel::Type InFeatureLevel);
	~FRuntimeMeshLODProxy();

	TMap<int32, FRuntimeMeshSectionProxyPtr>& GetSections() { return Sections; }

	bool CanRender() const;
	bool HasAnyStaticPath() const;
	bool HasAnyDynamicPath() const;
	bool HasAnyShadowCasters() const;

	float GetMaxScreenSize() const { return Properties.ScreenSize; }

	void UpdateProperties_RenderThread(const FRuntimeMeshLODProperties& InProperties);
	void Clear_RenderThread();

	void CreateSection_RenderThread(int32 SectionId, const FRuntimeMeshSectionProperties& SectionProperties);
	void RemoveSection_RenderThread(int32 SectionId);

	void UpdateSectionProperties_RenderThread(int32 SectionId, const FRuntimeMeshSectionProperties& SectionProperties);
	void UpdateSectionMesh_RenderThread(int32 SectionId, const FRuntimeMeshRenderableMeshData& MeshData);
	void ClearSectionMesh_RenderThread(int32 SectionId);
};


