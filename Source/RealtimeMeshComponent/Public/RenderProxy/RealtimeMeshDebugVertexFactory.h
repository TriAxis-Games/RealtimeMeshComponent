// Copyright (c) 2015-2025 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "RealtimeMeshVertexFactory.h"
#include "Core/RealtimeMeshStreamRange.h"
#include "RenderResource.h"

namespace RealtimeMesh
{
	/**
	 * Debug line index buffer that generates linear indices for line rendering
	 * Pattern: [0, 1, 2, 3, 4, 5, ...] for shader-based line generation
	 */
	class FRealtimeMeshDebugLineIndexBuffer : public FIndexBuffer
	{
	public:
		static const uint32 MaxDebugVertices = 32768; // Support up to 32k vertices
		
		virtual void InitRHI(FRHICommandListBase& RHICmdList) override;
		virtual void ReleaseRHI() override;
		
		static TSharedPtr<FRealtimeMeshDebugLineIndexBuffer> Get()
		{
			static TWeakPtr<FRealtimeMeshDebugLineIndexBuffer> WeakInstance;
			TSharedPtr<FRealtimeMeshDebugLineIndexBuffer> SharedInstance = WeakInstance.Pin();
			
			if (!SharedInstance.IsValid())
			{
				// Custom deleter that releases render resources before deletion
				auto Deleter = [](FRealtimeMeshDebugLineIndexBuffer* Buffer)
				{
					if (Buffer->IsInitialized())
					{
						Buffer->ReleaseResource();
					}
					delete Buffer;
				};
				
				SharedInstance = MakeShareable(new FRealtimeMeshDebugLineIndexBuffer(), MoveTemp(Deleter));
				WeakInstance = SharedInstance;
			}
			
			return SharedInstance;
		}
	};
	/**
	 * Debug vertex factory for rendering normals, tangents, and binormals as lines
	 * Generates line geometry on-the-fly from existing mesh vertex data
	 */
	class REALTIMEMESHCOMPONENT_API FRealtimeMeshDebugVertexFactory : public FRealtimeMeshVertexFactory
	{
		DECLARE_VERTEX_FACTORY_TYPE(FRealtimeMeshDebugVertexFactory);

	public:
		FRealtimeMeshDebugVertexFactory(ERHIFeatureLevel::Type InFeatureLevel)
			: FRealtimeMeshVertexFactory(InFeatureLevel)
			, DebugMode(0)
			, LineLength(5.0f)
			, ValidRange(FRealtimeMeshStreamRange::Empty())
		{
		}

		struct FDataType
		{
			FVertexStreamComponent PositionComponent;
			FVertexStreamComponent TangentBasisComponents[2];
			FVertexStreamComponent ColorComponent;
			
			FRHIShaderResourceView* PositionComponentSRV = nullptr;
			FRHIShaderResourceView* TangentsSRV = nullptr;
			FRHIShaderResourceView* ColorComponentsSRV = nullptr;
		};

		virtual ~FRealtimeMeshDebugVertexFactory() override;

		// Debug mode flags
		enum EDebugMode
		{
			None = 0,
			Normals = 1 << 0,
			Tangents = 1 << 1, 
			Binormals = 1 << 2,
			VertexColors = 1 << 3
		};

		void SetDebugMode(uint32 InDebugMode) { DebugMode = InDebugMode; }
		void SetLineLength(float InLineLength) { LineLength = InLineLength; }

		/**
		 * Should we cache the material's shadertype on this platform with this vertex factory? 
		 */
		static bool ShouldCompilePermutation(const FVertexFactoryShaderPermutationParameters& Parameters);

		static void ModifyCompilationEnvironment(const FVertexFactoryShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment);

	static void ValidateCompiledResult(const FVertexFactoryType* Type, EShaderPlatform Platform, const FShaderParameterMap& ParameterMap, TArray<FString>& OutErrors);

		/**
		 * Initialize the vertex factory with GPU buffers for debug rendering
		 */
		virtual void Initialize(FRHICommandListBase& RHICmdList, const TMap<FRealtimeMeshStreamKey, TSharedPtr<FRealtimeMeshGPUBuffer>>& Buffers) override;

		virtual void InitRHI(FRHICommandListBase& RHICmdList) override;

		static FVertexFactoryShaderParameters* ConstructShaderParameters(EShaderFrequency ShaderFrequency);

		virtual EPrimitiveType GetPrimitiveType() const override { return PT_LineList; }
		
		virtual FRealtimeMeshStreamRange GetValidRange() const override { return ValidRange; }
		virtual bool IsValidStreamRange(const FRealtimeMeshStreamRange& StreamRange) const override { return ValidRange.Contains(StreamRange); }

		virtual FIndexBuffer& GetIndexBuffer(bool& bDepthOnly, bool& bMatrixInverted, struct FRealtimeMeshResourceReferenceList& ActiveResources) const override;
		virtual FRHIUniformBuffer* GetUniformBuffer() const override;

		virtual bool GatherVertexBufferResources(struct FRealtimeMeshResourceReferenceList& ActiveResources) const override;

		// Getters for shader parameter access
		uint32 GetDebugMode() const { return DebugMode; }
		float GetLineLength() const { return LineLength; }

	private:
		// Create uniform buffer specific to debug vertex factory needs
		TUniformBufferRef<FLocalVertexFactoryUniformShaderParameters> CreateDebugVertexFactoryUniformBuffer() const;

	public:
		FDataType Data; // Make public so shader parameters can access it
		
	protected:
		uint32 DebugMode;
		float LineLength;
		FRealtimeMeshStreamRange ValidRange;
		
		TSet<TWeakPtr<FRealtimeMeshVertexBuffer>> InUseVertexBuffers;
		TUniformBufferRef<FLocalVertexFactoryUniformShaderParameters> UniformBuffer;
		
		// Hold shared reference to keep the debug index buffer alive
		TSharedPtr<FRealtimeMeshDebugLineIndexBuffer> DebugIndexBuffer;
	};

	/** Shader parameter class for debug vertex factory */
	class REALTIMEMESHCOMPONENT_API FRealtimeMeshDebugVertexFactoryShaderParameters : public FVertexFactoryShaderParameters
	{
		DECLARE_TYPE_LAYOUT(FRealtimeMeshDebugVertexFactoryShaderParameters, NonVirtual);

	public:
		void Bind(const FShaderParameterMap& ParameterMap);

		void GetElementShaderBindings(
			const FSceneInterface* Scene,
			const FSceneView* View,
			const FMeshMaterialShader* Shader,
			const EVertexInputStreamType InputStreamType,
			ERHIFeatureLevel::Type FeatureLevel,
			const FVertexFactory* VertexFactory,
			const FMeshBatchElement& BatchElement,
			FMeshDrawSingleShaderBindings& ShaderBindings,
			FVertexInputStreamArray& VertexStreams) const;

	private:
		// No individual parameters - we'll use the uniform buffer approach like LocalVertexFactory
	};
}