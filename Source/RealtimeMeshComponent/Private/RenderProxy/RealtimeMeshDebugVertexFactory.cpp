// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#include "RenderProxy/RealtimeMeshDebugVertexFactory.h"
#include "Engine/Engine.h"
#include "MeshMaterialShader.h"
#include "ShaderParameterUtils.h"
#include "DataDrivenShaderPlatformInfo.h"
#include "MaterialDomain.h"
#include "MeshDrawShaderBindings.h"
#include "RenderProxy/RealtimeMeshProxyShared.h"
#include "GlobalRenderResources.h"

// Declare external console variables from ComponentProxy.cpp
extern TAutoConsoleVariable<int32> CVarRealtimeMeshShowNormals;
extern TAutoConsoleVariable<int32> CVarRealtimeMeshShowTangents; 
extern TAutoConsoleVariable<int32> CVarRealtimeMeshShowBinormals;
extern TAutoConsoleVariable<float> CVarRealtimeMeshDebugLineLength;

namespace RealtimeMesh
{
	//////////////////////////////////////////////////////////////////////////
	// Debug Line Index Buffer Implementation

	void FRealtimeMeshDebugLineIndexBuffer::InitRHI(FRHICommandListBase& RHICmdList)
	{
		const uint32 NumIndices = MaxDebugVertices * 2;

#if RMC_ENGINE_ABOVE_5_6
		FRHIBufferCreateDesc IndexBufferDesc = FRHIBufferCreateDesc::CreateIndex(TEXT("RealtimeMeshDebugLineIndexBuffer"))
			.SetStride(sizeof(uint16))
			.SetSize(NumIndices * sizeof(uint16))
			.SetUsage(BUF_Static);		
		IndexBufferRHI = RHICmdList.CreateBuffer(IndexBufferDesc);
#else		
		FRHIResourceCreateInfo CreateInfo(TEXT("RealtimeMeshDebugLineIndexBuffer"));
		IndexBufferRHI = RHICmdList.CreateIndexBuffer(sizeof(uint16), NumIndices * sizeof(uint16), BUF_Static, CreateInfo);
#endif
		
		// Linear indices [0, 1, 2, ...]; the shader derives each line's start/end vertex.
		uint16* IndexData = static_cast<uint16*>(RHICmdList.LockBuffer(IndexBufferRHI, 0, NumIndices * sizeof(uint16), RLM_WriteOnly));
		for (uint32 i = 0; i < NumIndices; ++i)
		{
			IndexData[i] = static_cast<uint16>(i);
		}
		RHICmdList.UnlockBuffer(IndexBufferRHI);
	}

	void FRealtimeMeshDebugLineIndexBuffer::ReleaseRHI()
	{
		IndexBufferRHI.SafeRelease();
		FIndexBuffer::ReleaseRHI();
	}

	FRealtimeMeshDebugVertexFactory::~FRealtimeMeshDebugVertexFactory()
	{
		FRenderResource::ReleaseResource();
	}

	bool FRealtimeMeshDebugVertexFactory::ShouldCompilePermutation(const FVertexFactoryShaderPermutationParameters& Parameters)
	{
		return (Parameters.MaterialParameters.MaterialDomain == MD_Surface ||
			    Parameters.MaterialParameters.MaterialDomain == MD_DeferredDecal) &&
		       Parameters.MaterialParameters.bIsUsedWithStaticLighting == false &&
		       IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	void FRealtimeMeshDebugVertexFactory::ModifyCompilationEnvironment(const FVertexFactoryShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		if (RHISupportsManualVertexFetch(Parameters.Platform))
		{
			OutEnvironment.SetDefineIfUnset(TEXT("MANUAL_VERTEX_FETCH"), TEXT("1"));
		}

		// Must match the scene proxy's bVFRequiresPrimitiveUniformBuffer decision or GPUScene binding breaks.
		const bool bVFSupportsPrimitiveSceneData = Parameters.VertexFactoryType->SupportsPrimitiveIdStream() && UseGPUScene(Parameters.Platform, GetMaxSupportedFeatureLevel(Parameters.Platform));
		OutEnvironment.SetDefine(TEXT("VF_SUPPORTS_PRIMITIVE_SCENE_DATA"), bVFSupportsPrimitiveSceneData);

		OutEnvironment.SetDefine(TEXT("REALTIME_MESH_DEBUG_VERTEX_FACTORY"), 1);
		OutEnvironment.SetDefine(TEXT("SHOW_NORMALS"), 1);
		OutEnvironment.SetDefine(TEXT("SHOW_TANGENTS"), 1);
		OutEnvironment.SetDefine(TEXT("SHOW_BINORMALS"), 1);
		OutEnvironment.SetDefine(TEXT("DEBUG_LINE_LENGTH"), 10.0f);

		FRealtimeMeshVertexFactory::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	}

	void FRealtimeMeshDebugVertexFactory::ValidateCompiledResult(const FVertexFactoryType* Type, EShaderPlatform Platform, const FShaderParameterMap& ParameterMap, TArray<FString>& OutErrors)
	{
		if (Type->SupportsPrimitiveIdStream() 
			&& UseGPUScene(Platform, GetMaxSupportedFeatureLevel(Platform)) 
			&& !IsMobilePlatform(Platform) // On mobile VS may use PrimitiveUB while GPUScene is enabled
			&& ParameterMap.ContainsParameterAllocation(FPrimitiveUniformShaderParameters::FTypeInfo::GetStructMetadata()->GetShaderVariableName()))
		{
			OutErrors.AddUnique(*FString::Printf(TEXT("Shader attempted to bind the Primitive uniform buffer even though Vertex Factory %s computes a PrimitiveId per-instance. This will break auto-instancing. Shaders should use GetPrimitiveData(Parameters).Member instead of Primitive.Member."), Type->GetName()));
		}
	}

	void FRealtimeMeshDebugVertexFactory::Initialize(FRHICommandListBase& RHICmdList, const TMap<FRealtimeMeshStreamKey, TSharedPtr<FRealtimeMeshGPUBuffer>>& Buffers)
	{
		InUseVertexBuffers.Empty();
		bool bIsValid = true;

		FInt32Range ValidVertexRange(0, TNumericLimits<int32>::Max());

		// Position is required; tangents/color fall back to null buffers when absent.
		BindVertexBuffer(bIsValid, ValidVertexRange, InUseVertexBuffers, Data.PositionComponent, Buffers,
		                 FRealtimeMeshStreams::PositionStreamName, EVertexStreamUsage::Default);
		BindVertexBufferSRV(bIsValid, Data.PositionComponentSRV, Buffers, FRealtimeMeshStreams::PositionStreamName);

		const TSharedPtr<FRealtimeMeshGPUBuffer> TangentBuffer = Buffers.FindRef(FRealtimeMeshStreams::Tangents);
		if (TangentBuffer && TangentBuffer->Num() > 0)
		{
			BindVertexBuffer(bIsValid, ValidVertexRange, InUseVertexBuffers, Data.TangentBasisComponents[0], Buffers,
							 FRealtimeMeshStreams::TangentsStreamName, EVertexStreamUsage::ManualFetch, false, 0);
			BindVertexBuffer(bIsValid, ValidVertexRange, InUseVertexBuffers, Data.TangentBasisComponents[1], Buffers,
							 FRealtimeMeshStreams::TangentsStreamName, EVertexStreamUsage::ManualFetch, false, 1);
			BindVertexBufferSRV(bIsValid, Data.TangentsSRV, Buffers, FRealtimeMeshStreams::TangentsStreamName);
		}
		else
		{			
			Data.TangentBasisComponents[0] = FVertexStreamComponent(&GRealtimeMeshNullTangentVertexBuffer, 0, 0, VET_Short4, EVertexStreamUsage::ManualFetch);
			Data.TangentBasisComponents[1] = FVertexStreamComponent(&GRealtimeMeshNullTangentVertexBuffer, sizeof(FPackedRGBA16N), 0, VET_Short4, EVertexStreamUsage::ManualFetch);
			Data.TangentsSRV = GRealtimeMeshNullTangentVertexBuffer.VertexBufferSRV;
		}

		const TSharedPtr<FRealtimeMeshGPUBuffer> ColorBuffer = Buffers.FindRef(FRealtimeMeshStreams::Color);
		if (ColorBuffer && ColorBuffer->Num() > 0)
		{
			BindVertexBuffer(bIsValid, ValidVertexRange, InUseVertexBuffers, Data.ColorComponent, Buffers,
							 FRealtimeMeshStreams::ColorStreamName, EVertexStreamUsage::ManualFetch, true, 0, true);
			BindVertexBufferSRV(bIsValid, Data.ColorComponentsSRV, Buffers, FRealtimeMeshStreams::ColorStreamName);
		}
		else
		{
			Data.ColorComponent = FVertexStreamComponent(&GRealtimeMeshNullColorVertexBuffer, 0, 0, VET_Color, EVertexStreamUsage::ManualFetch);
			Data.ColorComponentsSRV = GRealtimeMeshNullColorVertexBuffer.VertexBufferSRV;			
		}

		if (bIsValid && ValidVertexRange.GetLowerBoundValue() < ValidVertexRange.GetUpperBoundValue())
		{
			ValidRange = FRealtimeMeshStreamRange(ValidVertexRange, ValidVertexRange);
		}
		else
		{
			ValidRange = FRealtimeMeshStreamRange::Empty();
		}

		UniformBuffer = CreateDebugVertexFactoryUniformBuffer();

		// Shared across all debug factories; lazily initialized on first use.
		DebugIndexBuffer = FRealtimeMeshDebugLineIndexBuffer::Get();
		if (!DebugIndexBuffer->IsInitialized())
		{
			DebugIndexBuffer->InitResource(RHICmdList);
		}

		InitResource(RHICmdList);
	}

	void FRealtimeMeshDebugVertexFactory::InitRHI(FRHICommandListBase& RHICmdList)
	{
		FVertexDeclarationElementList Elements;

		if (Data.PositionComponent.VertexBuffer)
		{
			Elements.Add(AccessStreamComponent(Data.PositionComponent, 0));
		}

		if (Data.TangentBasisComponents[0].VertexBuffer)
		{
			Elements.Add(AccessStreamComponent(Data.TangentBasisComponents[0], 1));
		}

		if (Data.TangentBasisComponents[1].VertexBuffer)
		{
			Elements.Add(AccessStreamComponent(Data.TangentBasisComponents[1], 2));
		}

		if (Data.ColorComponent.VertexBuffer)
		{
			Elements.Add(AccessStreamComponent(Data.ColorComponent, 3));
		}

		// Primitive ID stream must match the scene proxy's bVFRequiresPrimitiveUniformBuffer setting.
		if (GetType()->SupportsPrimitiveIdStream() && UseGPUScene(GMaxRHIShaderPlatform, GetFeatureLevel()))
		{
			AddPrimitiveIdStreamElement(EVertexInputStreamType::Default, Elements, 13, 13);
		}

		check(Elements.Num() > 0);
		InitDeclaration(Elements);

		FVertexFactory::InitRHI(RHICmdList);
	}

	FVertexFactoryShaderParameters* FRealtimeMeshDebugVertexFactory::ConstructShaderParameters(EShaderFrequency ShaderFrequency)
	{
		return ShaderFrequency == SF_Vertex ? new FRealtimeMeshDebugVertexFactoryShaderParameters() : nullptr;
	}

	bool FRealtimeMeshDebugVertexFactory::GatherVertexBufferResources(struct FRealtimeMeshResourceReferenceList& ActiveResources) const
	{
		bool bHasValidResources = false;
		
		for (const auto& VertexBuffer : InUseVertexBuffers)
		{
			if (auto Buffer = VertexBuffer.Pin())
			{
				ActiveResources.AddResource(Buffer);
				bHasValidResources = true;
			}
		}
		
		return bHasValidResources;
	}

	FIndexBuffer& FRealtimeMeshDebugVertexFactory::GetIndexBuffer(bool& bDepthOnly, bool& bMatrixInverted, struct FRealtimeMeshResourceReferenceList& ActiveResources) const
	{
		bDepthOnly = false;
		bMatrixInverted = false;

		check(DebugIndexBuffer.IsValid() && DebugIndexBuffer->IsInitialized());

		return *DebugIndexBuffer;
	}

	TUniformBufferRef<FLocalVertexFactoryUniformShaderParameters> FRealtimeMeshDebugVertexFactory::CreateDebugVertexFactoryUniformBuffer() const
	{
		FLocalVertexFactoryUniformShaderParameters UniformParameters;

		const bool bShowNormals = CVarRealtimeMeshShowNormals.GetValueOnRenderThread() != 0;
		const bool bShowTangents = CVarRealtimeMeshShowTangents.GetValueOnRenderThread() != 0;
		const bool bShowBinormals = CVarRealtimeMeshShowBinormals.GetValueOnRenderThread() != 0;
		const float DebugLineLength = CVarRealtimeMeshDebugLineLength.GetValueOnRenderThread();
		
		// Pack debug parameters into VertexFetch_Parameters:
		// x = vertex offset (0 for debug)
		// y = debug mode bitmask (1=normals, 2=tangents, 4=binormals)  
		// z = debug line length (as int, will be converted back to float in shader)
		// w = number of active debug channels
		uint32 DebugModeBitmask = 0;
		uint32 NumActiveChannels = 0;
		if (bShowNormals) { DebugModeBitmask |= 1; NumActiveChannels++; }
		if (bShowTangents) { DebugModeBitmask |= 2; NumActiveChannels++; }
		if (bShowBinormals) { DebugModeBitmask |= 4; NumActiveChannels++; }
		
		UniformParameters.VertexFetch_Parameters = FIntVector4(
			0,  // x = vertex offset
			DebugModeBitmask, // y = debug mode bitmask
			*(uint32*)&DebugLineLength, // z = debug line length (float as uint32)
			NumActiveChannels // w = number of active channels
		);
		
		UniformParameters.VertexFetch_PositionBuffer = Data.PositionComponentSRV ? Data.PositionComponentSRV : GNullColorVertexBuffer.VertexBufferSRV.GetReference();
		UniformParameters.VertexFetch_PreSkinPositionBuffer = Data.PositionComponentSRV ? Data.PositionComponentSRV : GNullColorVertexBuffer.VertexBufferSRV.GetReference();
		UniformParameters.VertexFetch_PackedTangentsBuffer = Data.TangentsSRV ? Data.TangentsSRV : GNullColorVertexBuffer.VertexBufferSRV.GetReference();
		UniformParameters.VertexFetch_TexCoordBuffer = GNullColorVertexBuffer.VertexBufferSRV.GetReference(); // Debug doesn't use tex coords
		UniformParameters.VertexFetch_ColorComponentsBuffer = Data.ColorComponentsSRV ? Data.ColorComponentsSRV : GNullColorVertexBuffer.VertexBufferSRV.GetReference();

		UniformParameters.LODLightmapDataIndex = 0;
		UniformParameters.PreSkinBaseVertexIndex = 0;
		
		return TUniformBufferRef<FLocalVertexFactoryUniformShaderParameters>::CreateUniformBufferImmediate(UniformParameters, UniformBuffer_MultiFrame);
	}

	FRHIUniformBuffer* FRealtimeMeshDebugVertexFactory::GetUniformBuffer() const
	{
		return UniformBuffer.GetReference();
	}

	void FRealtimeMeshDebugVertexFactory::RefreshUniformBuffer()
	{
		// PROXY-F16: the uniform buffer packs the debug params (mode bitmask + line
		// length) read from the debug CVars at build time. When a cached VF is reused
		// after those params change, rebuild it so the new params are actually
		// consumed (SetDebugMode/SetLineLength alone only update members nothing
		// reads). Same construction as Initialize (see CreateDebugVertexFactoryUniformBuffer).
		UniformBuffer = CreateDebugVertexFactoryUniformBuffer();
	}

	//////////////////////////////////////////////////////////////////////////
	// Shader Parameters

	void FRealtimeMeshDebugVertexFactoryShaderParameters::Bind(const FShaderParameterMap& ParameterMap)
	{
		// No individual parameters to bind - using uniform buffer approach
	}

	void FRealtimeMeshDebugVertexFactoryShaderParameters::GetElementShaderBindings(
		const FSceneInterface* Scene,
		const FSceneView* View,
		const FMeshMaterialShader* Shader,
		const EVertexInputStreamType InputStreamType,
		ERHIFeatureLevel::Type FeatureLevel,
		const FVertexFactory* VertexFactory,
		const FMeshBatchElement& BatchElement,
		FMeshDrawSingleShaderBindings& ShaderBindings,
		FVertexInputStreamArray& VertexStreams) const
	{
		const FRealtimeMeshDebugVertexFactory* DebugVF = static_cast<const FRealtimeMeshDebugVertexFactory*>(VertexFactory);
		
		// Bind the LocalVF uniform buffer that contains vertex fetch parameters
		FRHIUniformBuffer* VertexFactoryUniformBuffer = DebugVF->GetUniformBuffer();
		if (VertexFactoryUniformBuffer)
		{
			ShaderBindings.Add(Shader->GetUniformBufferParameter<FLocalVertexFactoryUniformShaderParameters>(), VertexFactoryUniformBuffer);
		}
	}
}

using namespace RealtimeMesh;

IMPLEMENT_TYPE_LAYOUT(FRealtimeMeshDebugVertexFactoryShaderParameters);

// Register the vertex factory parameter type for proper uniform buffer binding
IMPLEMENT_VERTEX_FACTORY_PARAMETER_TYPE(FRealtimeMeshDebugVertexFactory, SF_Vertex, FRealtimeMeshDebugVertexFactoryShaderParameters);

// Register the vertex factory type
IMPLEMENT_VERTEX_FACTORY_TYPE(FRealtimeMeshDebugVertexFactory, "/Plugin/RealtimeMeshComponent/RealtimeMeshDebugVertexFactory.ush",
	EVertexFactoryFlags::UsedWithMaterials
	| EVertexFactoryFlags::SupportsDynamicLighting
	| EVertexFactoryFlags::SupportsPositionOnly
	| EVertexFactoryFlags::SupportsPrimitiveIdStream
	| EVertexFactoryFlags::SupportsManualVertexFetch
);