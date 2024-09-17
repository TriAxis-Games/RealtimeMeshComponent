// Copyright (c) 2015-2024 TriAxis Games, L.L.C. All Rights Reserved.

/*=============================================================================
	RealtimeMeshVertexFactory.cpp: Local vertex factory implementation
=============================================================================*/

#include "RenderProxy/RealtimeMeshVertexFactory.h"


#include "SceneView.h"
#include "MeshBatch.h"
#include "SpeedTreeWind.h"
#include "Rendering/ColorVertexBuffer.h"
#include "MeshMaterialShader.h"
#include "SceneInterface.h"
#include "ProfilingDebugging/LoadTimeTracker.h"
#include "RenderProxy/RealtimeMeshProxyShared.h"

#if RMC_ENGINE_ABOVE_5_2
#include "MaterialDomain.h"
#include "MeshDrawShaderBindings.h"
#endif

namespace RealtimeMesh
{

	IMPLEMENT_TYPE_LAYOUT(FRealtimeMeshVertexFactoryShaderParameters);

	class FRealtimeMeshSpeedTreeWindNullUniformBuffer : public TUniformBuffer<FSpeedTreeUniformParameters>
	{
		typedef TUniformBuffer<FSpeedTreeUniformParameters> Super;

	public:
#if RMC_ENGINE_ABOVE_5_3
		virtual void InitRHI(FRHICommandListBase& RHICmdList) override;
#else
		virtual void InitDynamicRHI() override;
#endif
	};

#if RMC_ENGINE_ABOVE_5_3
	void FRealtimeMeshSpeedTreeWindNullUniformBuffer::InitRHI(FRHICommandListBase& RHICmdList)
#else
	void FRealtimeMeshSpeedTreeWindNullUniformBuffer::InitDynamicRHI()
#endif
	{
		FSpeedTreeUniformParameters Parameters;
		FMemory::Memzero(Parameters);
		SetContentsNoUpdate(Parameters);

#if RMC_ENGINE_ABOVE_5_3
		Super::InitRHI(RHICmdList);
#else
		Super::InitDynamicRHI();
#endif
	}

	static TGlobalResource<FRealtimeMeshSpeedTreeWindNullUniformBuffer> GSpeedTreeWindNullUniformBuffer;

#if RMC_ENGINE_ABOVE_5_3
	void FRealtimeMeshNullColorVertexBuffer::InitRHI(FRHICommandListBase& RHICmdList)
	{		
		// create a static vertex buffer
		FRHIResourceCreateInfo CreateInfo(TEXT("FRealtimeMeshNullColorVertexBuffer"));

		VertexBufferRHI = RHICmdList.CreateBuffer(sizeof(FColor), BUF_Static | BUF_VertexBuffer | BUF_ShaderResource, 0, ERHIAccess::VertexOrIndexBuffer | ERHIAccess::SRVMask, CreateInfo);
		FColor* Vertices = static_cast<FColor*>(RHICmdList.LockBuffer(VertexBufferRHI, 0, sizeof(FColor), RLM_WriteOnly));
		Vertices[0] = FColor(255, 255, 255, 255);
		RHICmdList.UnlockBuffer(VertexBufferRHI);
		VertexBufferSRV = RHICmdList.CreateShaderResourceView(VertexBufferRHI, sizeof(FColor), PF_R8G8B8A8);
		
		FVertexBuffer::InitRHI(RHICmdList);
	}
#else
	void FRealtimeMeshNullColorVertexBuffer::InitRHI()
	{
		FRHIResourceCreateInfo CreateInfo(TEXT("FNullColorVertexBuffer"));
		
		VertexBufferRHI = RHICreateBuffer(sizeof(FColor), BUF_Static | BUF_VertexBuffer | BUF_ShaderResource, 0, ERHIAccess::VertexOrIndexBuffer | ERHIAccess::SRVMask, CreateInfo);
		FColor* Vertices = static_cast<FColor*>(RHILockBuffer(VertexBufferRHI, 0, sizeof(FColor), RLM_WriteOnly));
		Vertices[0] = FColor(255, 255, 255, 255);
		RHIUnlockBuffer(VertexBufferRHI);
		VertexBufferSRV = RHICreateShaderResourceView(VertexBufferRHI, sizeof(FColor), PF_R8G8B8A8);		
	}
#endif

	void FRealtimeMeshNullColorVertexBuffer::ReleaseRHI()
	{
		VertexBufferSRV.SafeRelease();
		FVertexBuffer::ReleaseRHI();
	}

#if RMC_ENGINE_ABOVE_5_3
	void FRealtimeMeshNullTangentVertexBuffer::InitRHI(FRHICommandListBase& RHICmdList)
	{
		// create a static vertex buffer
		FRHIResourceCreateInfo CreateInfo(TEXT("FRealtimeMeshNullTangentVertexBuffer"));

		VertexBufferRHI = RHICmdList.CreateBuffer(sizeof(TRealtimeMeshTangents<FPackedRGBA16N>), BUF_Static | BUF_VertexBuffer | BUF_ShaderResource, 0, ERHIAccess::VertexOrIndexBuffer | ERHIAccess::SRVMask, CreateInfo);
		TRealtimeMeshTangents<FPackedRGBA16N>* Vertices = static_cast<TRealtimeMeshTangents<FPackedRGBA16N>*>(RHICmdList.LockBuffer(VertexBufferRHI, 0, sizeof(TRealtimeMeshTangents<FPackedRGBA16N>), RLM_WriteOnly));
		Vertices[0] = TRealtimeMeshTangents<FPackedRGBA16N>(FVector3f::ZAxisVector, FVector3f::YAxisVector, FVector3f::XAxisVector);
		RHICmdList.UnlockBuffer(VertexBufferRHI);
		VertexBufferSRV = RHICmdList.CreateShaderResourceView(VertexBufferRHI, sizeof(TRealtimeMeshTangents<FPackedRGBA16N>), PF_R16G16B16A16_SINT);
		
		FVertexBuffer::InitRHI(RHICmdList);
	}
#else
	void FRealtimeMeshNullTangentVertexBuffer::InitRHI()
	{
		FRHIResourceCreateInfo CreateInfo(TEXT("FNullColorVertexBuffer"));

		VertexBufferRHI = RHICreateBuffer(sizeof(TRealtimeMeshTangents<FPackedRGBA16N>), BUF_Static | BUF_VertexBuffer | BUF_ShaderResource, 0, ERHIAccess::VertexOrIndexBuffer | ERHIAccess::SRVMask, CreateInfo);
		TRealtimeMeshTangents<FPackedRGBA16N>* Vertices = static_cast<TRealtimeMeshTangents<FPackedRGBA16N>*>(RHILockBuffer(VertexBufferRHI, 0, sizeof(TRealtimeMeshTangents<FPackedRGBA16N>), RLM_WriteOnly));
		Vertices[0] = TRealtimeMeshTangents<FPackedRGBA16N>(FVector3f::ZAxisVector, FVector3f::YAxisVector, FVector3f::XAxisVector);
		RHIUnlockBuffer(VertexBufferRHI);
		VertexBufferSRV = RHICreateShaderResourceView(VertexBufferRHI, sizeof(TRealtimeMeshTangents<FPackedRGBA16N>), PF_R16G16B16A16_SINT);	
	}
#endif

	void FRealtimeMeshNullTangentVertexBuffer::ReleaseRHI()
	{
		VertexBufferSRV.SafeRelease();
		FVertexBuffer::ReleaseRHI();
	}

#if RMC_ENGINE_ABOVE_5_3
	void FRealtimeMeshNullTexCoordVertexBuffer::InitRHI(FRHICommandListBase& RHICmdList)
	{
		FRHIResourceCreateInfo CreateInfo(TEXT("FRealtimeMeshNullTexCoordVertexBuffer"));

		VertexBufferRHI = RHICmdList.CreateBuffer(sizeof(FVector2f), BUF_Static | BUF_VertexBuffer | BUF_ShaderResource, 0, ERHIAccess::VertexOrIndexBuffer | ERHIAccess::SRVMask, CreateInfo);
		FVector2f* Vertices = static_cast<FVector2f*>(RHICmdList.LockBuffer(VertexBufferRHI, 0, sizeof(FVector2f), RLM_WriteOnly));
		Vertices[0] = FVector2f::ZeroVector;
		RHICmdList.UnlockBuffer(VertexBufferRHI);
		VertexBufferSRV = RHICmdList.CreateShaderResourceView(VertexBufferRHI, sizeof(FVector2f), PF_G32R32F);
		
		FVertexBuffer::InitRHI(RHICmdList);
	}
#else
	void FRealtimeMeshNullTexCoordVertexBuffer::InitRHI()
	{
		FRHIResourceCreateInfo CreateInfo(TEXT("FNullColorVertexBuffer"));

		VertexBufferRHI = RHICreateBuffer(sizeof(FVector2f), BUF_Static | BUF_VertexBuffer | BUF_ShaderResource, 0, ERHIAccess::VertexOrIndexBuffer | ERHIAccess::SRVMask, CreateInfo);
		FVector2f* Vertices = static_cast<FVector2f*>(RHILockBuffer(VertexBufferRHI, 0, sizeof(FVector2f), RLM_WriteOnly));
		Vertices[0] = FVector2f::ZeroVector;
		RHIUnlockBuffer(VertexBufferRHI);
		VertexBufferSRV = RHICreateShaderResourceView(VertexBufferRHI, sizeof(FVector2f), PF_G32R32F);	
	}
#endif

	void FRealtimeMeshNullTexCoordVertexBuffer::ReleaseRHI()
	{
		VertexBufferSRV.SafeRelease();
		FVertexBuffer::ReleaseRHI();
	}

#if RMC_ENGINE_ABOVE_5_3
	TGlobalResource<FNullColorVertexBuffer, FRenderResource::EInitPhase::Pre> GRealtimeMeshNullColorVertexBuffer;
	TGlobalResource<FNullColorVertexBuffer, FRenderResource::EInitPhase::Pre> GRealtimeMeshNullTangentVertexBuffer;
	TGlobalResource<FNullColorVertexBuffer, FRenderResource::EInitPhase::Pre> GRealtimeMeshNullTexCoordVertexBuffer;
#else
	TGlobalResource<FNullColorVertexBuffer> GRealtimeMeshNullColorVertexBuffer;
	TGlobalResource<FNullColorVertexBuffer> GRealtimeMeshNullTangentVertexBuffer;
	TGlobalResource<FNullColorVertexBuffer> GRealtimeMeshNullTexCoordVertexBuffer;
#endif

	

	TUniformBufferRef<FLocalVertexFactoryUniformShaderParameters> CreateRealtimeMeshVFUniformBuffer(
		const FRealtimeMeshLocalVertexFactory* RealtimeMeshVertexFactory, uint32 LODLightmapDataIndex)
	{
		FLocalVertexFactoryUniformShaderParameters UniformParameters;

		constexpr FColorVertexBuffer* OverrideColorVertexBuffer = nullptr;
		constexpr int32 BaseVertexIndex = 0;
		constexpr int32 PreSkinBaseVertexIndex = 0;

		UniformParameters.LODLightmapDataIndex = LODLightmapDataIndex;
		int32 ColorIndexMask = 0;

		if (RHISupportsManualVertexFetch(GMaxRHIShaderPlatform))
		{
			UniformParameters.VertexFetch_PositionBuffer = RealtimeMeshVertexFactory->GetPositionsSRV();
			UniformParameters.VertexFetch_PreSkinPositionBuffer = RealtimeMeshVertexFactory->GetPreSkinPositionSRV();

			UniformParameters.VertexFetch_PackedTangentsBuffer = RealtimeMeshVertexFactory->GetTangentsSRV();
			UniformParameters.VertexFetch_TexCoordBuffer = RealtimeMeshVertexFactory->GetTextureCoordinatesSRV();

			if (OverrideColorVertexBuffer)
			{
				UniformParameters.VertexFetch_ColorComponentsBuffer = OverrideColorVertexBuffer->GetColorComponentsSRV();
				ColorIndexMask = OverrideColorVertexBuffer->GetNumVertices() > 1 ? ~0 : 0;
			}
			else
			{
				UniformParameters.VertexFetch_ColorComponentsBuffer = RealtimeMeshVertexFactory->GetColorComponentsSRV();
				ColorIndexMask = (int32)RealtimeMeshVertexFactory->GetColorIndexMask();
			}
		}
		else
		{
			UniformParameters.VertexFetch_PositionBuffer = GNullColorVertexBuffer.VertexBufferSRV;
			UniformParameters.VertexFetch_PreSkinPositionBuffer = GNullColorVertexBuffer.VertexBufferSRV;
			UniformParameters.VertexFetch_PackedTangentsBuffer = GNullColorVertexBuffer.VertexBufferSRV;
			UniformParameters.VertexFetch_TexCoordBuffer = GNullColorVertexBuffer.VertexBufferSRV;
		}

		if (!UniformParameters.VertexFetch_ColorComponentsBuffer)
		{
			UniformParameters.VertexFetch_ColorComponentsBuffer = GNullColorVertexBuffer.VertexBufferSRV;
		}

		const int32 NumTexCoords = RealtimeMeshVertexFactory->GetNumTexcoords();
		const int32 LightMapCoordinateIndex = RealtimeMeshVertexFactory->GetLightMapCoordinateIndex();
		const int32 EffectiveBaseVertexIndex = RHISupportsAbsoluteVertexID(GMaxRHIShaderPlatform) ? 0 : BaseVertexIndex;
		const int32 EffectivePreSkinBaseVertexIndex = RHISupportsAbsoluteVertexID(GMaxRHIShaderPlatform) ? 0 : PreSkinBaseVertexIndex;

		UniformParameters.VertexFetch_Parameters = {ColorIndexMask, NumTexCoords, LightMapCoordinateIndex, EffectiveBaseVertexIndex};
		UniformParameters.PreSkinBaseVertexIndex = EffectivePreSkinBaseVertexIndex;

		return TUniformBufferRef<FLocalVertexFactoryUniformShaderParameters>::CreateUniformBufferImmediate(UniformParameters, UniformBuffer_MultiFrame);
	}

	FIndexBuffer& FRealtimeMeshLocalVertexFactory::GetIndexBuffer(bool& bDepthOnly, bool& bMatrixInverted,
	                                                              TFunctionRef<void(const TSharedRef<FRenderResource>&)> ResourceSubmitter) const
	{
		if (bDepthOnly)
		{
			if (bMatrixInverted)
			{
				if (const auto ReversedDepth = ReversedDepthOnlyIndexBuffer.Pin())
				{
					ResourceSubmitter(ReversedDepth.ToSharedRef());
					return *ReversedDepth.Get();
				}
			}

			if (const auto DepthOnly = DepthOnlyIndexBuffer.Pin())
			{
				ResourceSubmitter(DepthOnly.ToSharedRef());
				bMatrixInverted = false;
				return *DepthOnly.Get();
			}
		}

		if (bMatrixInverted)
		{
			if (const auto Reversed = ReversedIndexBuffer.Pin())
			{
				ResourceSubmitter(Reversed.ToSharedRef());
				bDepthOnly = false;
				return *Reversed.Get();
			}
		}

		const auto Normal = IndexBuffer.Pin();
		check(Normal.IsValid());
		ResourceSubmitter(Normal.ToSharedRef());

		bMatrixInverted = false;
		bDepthOnly = false;
		return *Normal.Get();
	}

	FIndexBuffer& FRealtimeMeshLocalVertexFactory::GetIndexBuffer(bool& bDepthOnly, bool& bMatrixInverted, struct FRealtimeMeshResourceReferenceList& ActiveResources) const
	{
		if (bDepthOnly)
		{
			if (bMatrixInverted)
			{
				if (const auto ReversedDepth = ReversedDepthOnlyIndexBuffer.Pin())
				{
					ActiveResources.AddResource(ReversedDepth);
					return *ReversedDepth.Get();
				}
			}

			if (const auto DepthOnly = DepthOnlyIndexBuffer.Pin())
			{
				ActiveResources.AddResource(DepthOnly);
				bMatrixInverted = false;
				return *DepthOnly.Get();
			}
		}

		if (bMatrixInverted)
		{
			if (const auto Reversed = ReversedIndexBuffer.Pin())
			{
				ActiveResources.AddResource(Reversed);
				bDepthOnly = false;
				return *Reversed.Get();
			}
		}

		const auto Normal = IndexBuffer.Pin();
		check(Normal.IsValid());
		ActiveResources.AddResource(Normal);

		bMatrixInverted = false;
		bDepthOnly = false;
		return *Normal.Get();
	}

	
	bool FRealtimeMeshLocalVertexFactory::IsValidStreamRange(const FRealtimeMeshStreamRange& StreamRange) const
	{
		return ValidRange.Contains(StreamRange);
	}

	void FRealtimeMeshLocalVertexFactory::Initialize(FRHICommandListBase& RHICmdList, const TMap<FRealtimeMeshStreamKey, TSharedPtr<FRealtimeMeshGPUBuffer>>& Buffers)
	{
		FDataType DataType;

		InUseVertexBuffers.Empty();
		bool bIsValid = true;

		FInt32Range ValidVertexRange(0, TNumericLimits<int32>::Max());
		FInt32Range ValidIndexRange(0, TNumericLimits<int32>::Max());

		// Bind Position
		BindVertexBuffer(bIsValid, ValidVertexRange, InUseVertexBuffers, DataType.PositionComponent, Buffers,
		                 FRealtimeMeshStreams::PositionStreamName, EVertexStreamUsage::Default);
		BindVertexBufferSRV(bIsValid, DataType.PositionComponentSRV, Buffers, FRealtimeMeshStreams::PositionStreamName);

		// Bind Tangents

		const TSharedPtr<FRealtimeMeshGPUBuffer> TangentBuffer = Buffers.FindRef(FRealtimeMeshStreams::Tangents);
		if (TangentBuffer && TangentBuffer->Num() > 0)
		{
			BindVertexBuffer(bIsValid, ValidVertexRange, InUseVertexBuffers, DataType.TangentBasisComponents[0], Buffers,
							 FRealtimeMeshStreams::TangentsStreamName, EVertexStreamUsage::ManualFetch, false, 0);
			BindVertexBuffer(bIsValid, ValidVertexRange, InUseVertexBuffers, DataType.TangentBasisComponents[1], Buffers,
							 FRealtimeMeshStreams::TangentsStreamName, EVertexStreamUsage::ManualFetch, false, 1);
			BindVertexBufferSRV(bIsValid, DataType.TangentsSRV, Buffers, FRealtimeMeshStreams::TangentsStreamName);
		}
		else
		{			
			DataType.TangentBasisComponents[0] = FVertexStreamComponent(&GRealtimeMeshNullTangentVertexBuffer, 0, 0, VET_Short4, EVertexStreamUsage::ManualFetch);
			DataType.TangentBasisComponents[1] = FVertexStreamComponent(&GRealtimeMeshNullTangentVertexBuffer, sizeof(FPackedRGBA16N), 0, VET_Short4, EVertexStreamUsage::ManualFetch);
			DataType.TangentsSRV = GRealtimeMeshNullTangentVertexBuffer.VertexBufferSRV;
		}

		// Bind Color		
		const TSharedPtr<FRealtimeMeshGPUBuffer> ColorBuffer = Buffers.FindRef(FRealtimeMeshStreams::Color);
		if (ColorBuffer && ColorBuffer->Num() > 0)
		{
			BindVertexBuffer(bIsValid, ValidVertexRange, InUseVertexBuffers, DataType.ColorComponent, Buffers,
							 FRealtimeMeshStreams::ColorStreamName, EVertexStreamUsage::ManualFetch, true, 0, true);
			BindVertexBufferSRV(bIsValid, DataType.ColorComponentsSRV, Buffers, FRealtimeMeshStreams::ColorStreamName);
		}
		else
		{
			DataType.ColorComponent = FVertexStreamComponent(&GRealtimeMeshNullColorVertexBuffer, 0, 0, VET_Color, EVertexStreamUsage::ManualFetch);
			DataType.ColorComponentsSRV = GRealtimeMeshNullColorVertexBuffer.VertexBufferSRV;			
		}

		// Bind TexCoords
		DataType.NumTexCoords = 0;
		DataType.TextureCoordinates.Empty();		
		const TSharedPtr<FRealtimeMeshGPUBuffer> TexCoordBuffer = Buffers.FindRef(FRealtimeMeshStreams::TexCoords);
		if (TexCoordBuffer && TexCoordBuffer->Num() > 0)
		{
			BindTexCoordsBuffer(bIsValid, ValidVertexRange, InUseVertexBuffers, DataType.TextureCoordinates, DataType.NumTexCoords, Buffers,
							 FRealtimeMeshStreams::TexCoordsStreamName, EVertexStreamUsage::ManualFetch);
			BindVertexBufferSRV(bIsValid, DataType.TextureCoordinatesSRV, Buffers, FRealtimeMeshStreams::TexCoordsStreamName);
		}
		else
		{
			DataType.NumTexCoords = 1;
			DataType.TextureCoordinates.Add(FVertexStreamComponent(&GRealtimeMeshNullTexCoordVertexBuffer, 0, 0, VET_Float2, EVertexStreamUsage::ManualFetch));
			DataType.TextureCoordinatesSRV = GRealtimeMeshNullTexCoordVertexBuffer.VertexBufferSRV;	
		}

		// Bind all index buffers
		BindIndexBuffer(bIsValid, ValidIndexRange, IndexBuffer, Buffers, FRealtimeMeshStreams::TrianglesStreamName);
		//BindIndexBuffer(bIsValid, ValidIndexRange, ReversedIndexBuffer, Buffers, FRealtimeMeshStreamNames::ReversedTrianglesStreamName, true);
		//BindIndexBuffer(bIsValid, ValidIndexRange, DepthOnlyIndexBuffer, Buffers, FRealtimeMeshStreamNames::DepthOnlyTrianglesStreamName, true);
		//BindIndexBuffer(bIsValid, ValidIndexRange, ReversedDepthOnlyIndexBuffer, Buffers, FRealtimeMeshStreamNames::ReversedDepthOnlyTrianglesStreamName, true);

		// Update our valid range to the sum of the valid ranges

		ReleaseResource();

		if (bIsValid)
		{
			Data = DataType;
			ValidRange = FRealtimeMeshStreamRange(ValidVertexRange, ValidIndexRange);
			
#if RMC_ENGINE_ABOVE_5_3
			InitResource(RHICmdList);
#else
			InitResource();
#endif
		}
		else
		{
			Data = FDataType();
			ValidRange = FRealtimeMeshStreamRange(0, 0, 0, 0);
			InUseVertexBuffers.Empty();
		}
	}

	bool FRealtimeMeshLocalVertexFactory::GatherVertexBufferResources(TFunctionRef<void(const TSharedRef<FRenderResource>&)> ResourceSubmitter) const
	{
		TArray<TSharedPtr<FRealtimeMeshVertexBuffer>> TempBuffers;
		for (const auto& Buffer : InUseVertexBuffers)
		{
			auto PinnedBuffer = Buffer.Pin();
			if (!PinnedBuffer)
			{
				return false;
			}
			TempBuffers.Add(PinnedBuffer);
		}
		for (const auto& Buffer : TempBuffers)
		{
			ResourceSubmitter(Buffer.ToSharedRef());
		}
		return true;
	}

	bool FRealtimeMeshLocalVertexFactory::GatherVertexBufferResources(FRealtimeMeshResourceReferenceList& ActiveResources) const
	{
		TArray<TSharedPtr<FRealtimeMeshVertexBuffer>> TempBuffers;
		for (const auto& Buffer : InUseVertexBuffers)
		{
			auto PinnedBuffer = Buffer.Pin();
			if (!PinnedBuffer)
			{
				return false;
			}
			TempBuffers.Add(PinnedBuffer);
		}
		for (const auto& Buffer : TempBuffers)
		{
			ActiveResources.AddResource(Buffer);
		}
		return true;		
	}


	void FRealtimeMeshVertexFactoryShaderParameters::Bind(const FShaderParameterMap& ParameterMap)
	{
		LODParameter.Bind(ParameterMap, TEXT("SpeedTreeLODInfo"));
		bAnySpeedTreeParamIsBound = LODParameter.IsBound() || ParameterMap.ContainsParameterAllocation(TEXT("SpeedTreeData"));
	}

	void FRealtimeMeshVertexFactoryShaderParameters::GetElementShaderBindings(
		const FSceneInterface* Scene,
		const FSceneView* View,
		const FMeshMaterialShader* Shader,
		const EVertexInputStreamType InputStreamType,
		ERHIFeatureLevel::Type FeatureLevel,
		const FVertexFactory* VertexFactory,
		const FMeshBatchElement& BatchElement,
		FMeshDrawSingleShaderBindings& ShaderBindings,
		FVertexInputStreamArray& VertexStreams
	) const
	{
		// Decode VertexFactoryUserData as VertexFactoryUniformBuffer
		const FRHIUniformBuffer* VertexFactoryUniformBuffer = static_cast<FRHIUniformBuffer*>(BatchElement.VertexFactoryUserData);

		const FRealtimeMeshLocalVertexFactory* LocalVertexFactory = static_cast<const FRealtimeMeshLocalVertexFactory*>(VertexFactory);

		if (LocalVertexFactory->SupportsManualVertexFetch(FeatureLevel) || UseGPUScene(GMaxRHIShaderPlatform, FeatureLevel))
		{
			if (!VertexFactoryUniformBuffer)
			{
				// No batch element override
				VertexFactoryUniformBuffer = LocalVertexFactory->GetUniformBuffer();
			}

			ShaderBindings.Add(Shader->GetUniformBufferParameter<FLocalVertexFactoryUniformShaderParameters>(), VertexFactoryUniformBuffer);
		}

		//@todo - allow FMeshBatch to supply vertex streams (instead of requiring that they come from the vertex factory), and this userdata hack will no longer be needed for override vertex color
		if (BatchElement.bUserDataIsColorVertexBuffer)
		{
			const FColorVertexBuffer* OverrideColorVertexBuffer = (FColorVertexBuffer*)BatchElement.UserData;
			check(OverrideColorVertexBuffer);

			if (!LocalVertexFactory->SupportsManualVertexFetch(FeatureLevel))
			{
				LocalVertexFactory->GetColorOverrideStream(OverrideColorVertexBuffer, VertexStreams);
			}
		}

		if (bAnySpeedTreeParamIsBound)
		{
			QUICK_SCOPE_CYCLE_COUNTER(STAT_FLocalVertexFactoryShaderParameters_SetMesh_SpeedTree);
			const FRHIUniformBuffer* SpeedTreeUniformBuffer = Scene ? Scene->GetSpeedTreeUniformBuffer(VertexFactory) : nullptr;
			if (SpeedTreeUniformBuffer == nullptr)
			{
				SpeedTreeUniformBuffer = GSpeedTreeWindNullUniformBuffer.GetUniformBufferRHI();
			}
			check(SpeedTreeUniformBuffer != nullptr);

			ShaderBindings.Add(Shader->GetUniformBufferParameter<FSpeedTreeUniformParameters>(), SpeedTreeUniformBuffer);

			if (LODParameter.IsBound())
			{
				const FVector3f LODData(BatchElement.MinScreenSize, BatchElement.MaxScreenSize, BatchElement.MaxScreenSize - BatchElement.MinScreenSize);
				ShaderBindings.Add(LODParameter, LODData);
			}
		}
	}

	/**
	 * Should we cache the material's shadertype on this platform with this vertex factory? 
	 */
	bool FRealtimeMeshLocalVertexFactory::ShouldCompilePermutation(const FVertexFactoryShaderPermutationParameters& Parameters)
	{
		// Only compile this permutation inside the editor - it's not applicable in games, but occasionally the editor needs it.
		if (Parameters.MaterialParameters.MaterialDomain == MD_UI)
		{
			return !!WITH_EDITOR;
		}

		return true;
	}

	void FRealtimeMeshLocalVertexFactory::ModifyCompilationEnvironment(const FVertexFactoryShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
#if RMC_ENGINE_ABOVE_5_3
		OutEnvironment.SetDefineIfUnset(TEXT("VF_SUPPORTS_SPEEDTREE_WIND"), TEXT("1"));

		if (RHISupportsManualVertexFetch(Parameters.Platform))
		{
			OutEnvironment.SetDefineIfUnset(TEXT("MANUAL_VERTEX_FETCH"), TEXT("1"));
		}
#else
		if (!OutEnvironment.GetDefinitions().Contains("VF_SUPPORTS_SPEEDTREE_WIND"))
		{
			OutEnvironment.SetDefine(TEXT("VF_SUPPORTS_SPEEDTREE_WIND"), TEXT("1"));
		}
		const bool ContainsManualVertexFetch = OutEnvironment.GetDefinitions().Contains("MANUAL_VERTEX_FETCH");
		if (!ContainsManualVertexFetch && RHISupportsManualVertexFetch(Parameters.Platform))
		{
			OutEnvironment.SetDefine(TEXT("MANUAL_VERTEX_FETCH"), TEXT("1"));
		}
#endif
		
		const bool bVFSupportsPrimtiveSceneData = Parameters.VertexFactoryType->SupportsPrimitiveIdStream() && UseGPUScene(
			Parameters.Platform, GetMaxSupportedFeatureLevel(Parameters.Platform));
		OutEnvironment.SetDefine(TEXT("VF_SUPPORTS_PRIMITIVE_SCENE_DATA"), bVFSupportsPrimtiveSceneData);
		OutEnvironment.SetDefine(TEXT("RAY_TRACING_DYNAMIC_MESH_IN_LOCAL_SPACE"), TEXT("1"));
	}

	void FRealtimeMeshLocalVertexFactory::ValidateCompiledResult(const FVertexFactoryType* Type, EShaderPlatform Platform, const FShaderParameterMap& ParameterMap,
	                                                             TArray<FString>& OutErrors)
	{
		if (Type->SupportsPrimitiveIdStream()
			&& UseGPUScene(Platform, GetMaxSupportedFeatureLevel(Platform))
			&& !IsMobilePlatform(Platform) // On mobile VS may use PrimtiveUB while GPUScene is enabled
#if RMC_ENGINE_ABOVE_5_3		
			&& ParameterMap.ContainsParameterAllocation(FPrimitiveUniformShaderParameters::FTypeInfo::GetStructMetadata()->GetShaderVariableName()))
#else
			&& ParameterMap.ContainsParameterAllocation(FPrimitiveUniformShaderParameters::StaticStructMetadata.GetShaderVariableName()))
#endif
		{
			OutErrors.AddUnique(*FString::Printf(
				TEXT(
					"Shader attempted to bind the Primitive uniform buffer even though Vertex Factory %s computes a PrimitiveId per-instance.  This will break auto-instancing.  Shaders should use GetPrimitiveData(Parameters).Member instead of Primitive.Member."),
				Type->GetName()));
		}
	}

	void FRealtimeMeshLocalVertexFactory::GetPSOPrecacheVertexFetchElements(EVertexInputStreamType VertexInputStreamType, FVertexDeclarationElementList& Elements)
	{		
		Elements.Add(FVertexElement(0, 0, VET_Float3, 0, 12, false));

		switch (VertexInputStreamType)
		{
		case EVertexInputStreamType::Default:
			{
				Elements.Add(FVertexElement(1, 0, VET_UInt, 13, 0, true));
				break;
			}
		case EVertexInputStreamType::PositionOnly:
			{
				Elements.Add(FVertexElement(1, 0, VET_UInt, 1, 0, true));
				break;
			}
		case EVertexInputStreamType::PositionAndNormalOnly:
			{
				Elements.Add(FVertexElement(1, 4, VET_PackedNormal, 2, 0, false));
				Elements.Add(FVertexElement(2, 0, VET_UInt, 1, 0, true));
				break;
			}
		default:
			checkNoEntry();
		}
	}

#if RMC_ENGINE_ABOVE_5_2
	void FRealtimeMeshLocalVertexFactory::GetVertexElements(ERHIFeatureLevel::Type FeatureLevel, EVertexInputStreamType InputStreamType, bool bSupportsManualVertexFetch,
		FDataType& Data, FVertexDeclarationElementList& Elements)
	{
		FVertexStreamList VertexStreams;
		int32 ColorStreamIndex;
		GetVertexElements(FeatureLevel, InputStreamType, bSupportsManualVertexFetch, Data, Elements, VertexStreams, ColorStreamIndex);

		// For ES3.1 attribute ID needs to be done differently
		check(FeatureLevel > ERHIFeatureLevel::ES3_1);
		Elements.Add(FVertexElement(VertexStreams.Num(), 0, VET_UInt, 13, 0, true));
	}
#endif

	/**
	* Copy the data from another vertex factory
	* @param Other - factory to copy from
	*/
	void FRealtimeMeshLocalVertexFactory::Copy(const FRealtimeMeshLocalVertexFactory& Other)
	{
		FRealtimeMeshLocalVertexFactory* VertexFactory = this;
		const FDataType* DataCopy = &Other.Data;
		ENQUEUE_RENDER_COMMAND(FRealtimeMeshVertexFactoryCopyData)(
			[VertexFactory, DataCopy](FRHICommandListImmediate& RHICmdList)
			{
				VertexFactory->Data = *DataCopy;
			});
		BeginUpdateResourceRHI(this);
	}
	
#if RMC_ENGINE_ABOVE_5_3
	void FRealtimeMeshLocalVertexFactory::InitRHI(FRHICommandListBase& RHICmdList)
#else
	void FRealtimeMeshLocalVertexFactory::InitRHI()
#endif
	{
		SCOPED_LOADTIMER(FLocalVertexFactory_InitRHI);

		// We create different streams based on feature level
		check(HasValidFeatureLevel());

		// VertexFactory needs to be able to support max possible shader platform and feature level
		// in case if we switch feature level at runtime.
		const bool bCanUseGPUScene = UseGPUScene(GMaxRHIShaderPlatform, GMaxRHIFeatureLevel);

		// If the vertex buffer containing position is not the same vertex buffer containing the rest of the data,
		// then initialize PositionStream and PositionDeclaration.
		if (Data.PositionComponent.VertexBuffer != Data.TangentBasisComponents[0].VertexBuffer)
		{
			auto AddDeclaration = [this](EVertexInputStreamType InputStreamType, bool bAddNormal)
			{
				FVertexDeclarationElementList StreamElements;
				StreamElements.Add(AccessStreamComponent(Data.PositionComponent, 0, InputStreamType));

				bAddNormal = bAddNormal && Data.TangentBasisComponents[1].VertexBuffer != NULL;
				if (bAddNormal)
				{
					StreamElements.Add(AccessStreamComponent(Data.TangentBasisComponents[1], 2, InputStreamType));
				}
#if RMC_ENGINE_ABOVE_5_4
				AddPrimitiveIdStreamElement(InputStreamType, StreamElements, 1, 1);
#else
				AddPrimitiveIdStreamElement(InputStreamType, StreamElements, 1, 8);
#endif

				InitDeclaration(StreamElements, InputStreamType);
			};

			AddDeclaration(EVertexInputStreamType::PositionOnly, false);
			AddDeclaration(EVertexInputStreamType::PositionAndNormalOnly, true);
		}

		FVertexDeclarationElementList Elements;
		
#if RMC_ENGINE_ABOVE_5_1
		const bool bUseManualVertexFetch = SupportsManualVertexFetch(GetFeatureLevel());
#else		
		const bool bUseManualVertexFetch = false;
#endif
		
#if RMC_ENGINE_ABOVE_5_2		
		GetVertexElements(GetFeatureLevel(), EVertexInputStreamType::Default, bUseManualVertexFetch, Data, Elements, Streams, ColorStreamIndex);
#else		
		GetVertexElements(GetFeatureLevel(), EVertexInputStreamType::Default, bUseManualVertexFetch, Data, Elements, ColorStreamIndex);		
#endif
		
#if RMC_ENGINE_ABOVE_5_4
		AddPrimitiveIdStreamElement(EVertexInputStreamType::Default, Elements, 13, 13);
#else
		AddPrimitiveIdStreamElement(EVertexInputStreamType::Default, Elements, 13, 8);
#endif
		check(Streams.Num() > 0);

		InitDeclaration(Elements);
		check(IsValidRef(GetDeclaration()));

		if (RHISupportsManualVertexFetch(GMaxRHIShaderPlatform) || bCanUseGPUScene)
		{
			SCOPED_LOADTIMER(FLocalVertexFactory_InitRHI_CreateLocalVFUniformBuffer);
			UniformBuffer = CreateRealtimeMeshVFUniformBuffer(this, Data.LODLightmapDataIndex);
		}

		check(IsValidRef(GetDeclaration()));
	}

	void FRealtimeMeshLocalVertexFactory::GetVertexElements(ERHIFeatureLevel::Type VFFeatureLevel, EVertexInputStreamType InputStreamType, bool bSupportsManualVertexFetch,
	                                                        FDataType& VFData, FVertexDeclarationElementList& Elements,
#if RMC_ENGINE_ABOVE_5_2
	                                                        FVertexStreamList& InOutStreams,
#endif
	                                                        int32& OutColorStreamIndex)
	{
		check(InputStreamType == EVertexInputStreamType::Default);

		if (VFData.PositionComponent.VertexBuffer != nullptr)
		{
#if RMC_ENGINE_ABOVE_5_2
			Elements.Add(AccessStreamComponent(VFData.PositionComponent, 0, InOutStreams));
#else
			Elements.Add(AccessStreamComponent(VFData.PositionComponent, 0));
#endif
		}

#if !WITH_EDITOR
	// Can't rely on manual vertex fetch in the editor to not add the unused elements because vertex factories created
	// with manual vertex fetch support can somehow still be used when booting up in for example ES3.1 preview mode
	// The vertex factories are then used during mobile rendering and will cause PSO creation failure.
	// First need to fix invalid usage of these vertex factories before this can be enabled again. (UE-165187)
	if (!bSupportsManualVertexFetch)
#endif // WITH_EDITOR
		{
			// Only the tangent and normal are used by the stream; the bitangent is derived in the shader.
			uint8 TangentBasisAttributes[2] = {1, 2};
			for (int32 AxisIndex = 0; AxisIndex < 2; AxisIndex++)
			{
				if (VFData.TangentBasisComponents[AxisIndex].VertexBuffer != nullptr)
				{
#if RMC_ENGINE_ABOVE_5_2
					Elements.Add(AccessStreamComponent(VFData.TangentBasisComponents[AxisIndex], TangentBasisAttributes[AxisIndex], InOutStreams));
#else					
					Elements.Add(AccessStreamComponent(VFData.TangentBasisComponents[AxisIndex], TangentBasisAttributes[AxisIndex]));
#endif
				}
			}

			if (VFData.ColorComponentsSRV == nullptr)
			{
				VFData.ColorComponentsSRV = GNullColorVertexBuffer.VertexBufferSRV;
				VFData.ColorIndexMask = 0;
			}		

			if (VFData.ColorComponent.VertexBuffer)
			{
#if RMC_ENGINE_ABOVE_5_2
				Elements.Add(AccessStreamComponent(VFData.ColorComponent, 3, InOutStreams));
#else				
				Elements.Add(AccessStreamComponent(VFData.ColorComponent, 3));
#endif
			}
			else
			{
				// If the mesh has no color component, set the null color buffer on a new stream with a stride of 0.
				// This wastes 4 bytes per vertex, but prevents having to compile out twice the number of vertex factories.
				FVertexStreamComponent NullColorComponent(&GNullColorVertexBuffer, 0, 0, VET_Color, EVertexStreamUsage::ManualFetch);				
#if RMC_ENGINE_ABOVE_5_2
				Elements.Add(AccessStreamComponent(NullColorComponent, 3, InOutStreams));
#else
				Elements.Add(AccessStreamComponent(NullColorComponent, 3));
#endif
			}
			OutColorStreamIndex = Elements.Last().StreamIndex;

			if (VFData.TextureCoordinates.Num())
			{
				const int32 BaseTexCoordAttribute = 4;
				for (int32 CoordinateIndex = 0; CoordinateIndex < VFData.TextureCoordinates.Num(); ++CoordinateIndex)
				{
#if RMC_ENGINE_ABOVE_5_2
					Elements.Add(AccessStreamComponent(VFData.TextureCoordinates[CoordinateIndex], BaseTexCoordAttribute + CoordinateIndex, InOutStreams));
#else
					Elements.Add(AccessStreamComponent(VFData.TextureCoordinates[CoordinateIndex], BaseTexCoordAttribute + CoordinateIndex));
#endif
				}

				for (int32 CoordinateIndex = VFData.TextureCoordinates.Num(); CoordinateIndex < MAX_STATIC_TEXCOORDS / 2; ++CoordinateIndex)
				{
#if RMC_ENGINE_ABOVE_5_2
					Elements.Add(AccessStreamComponent(VFData.TextureCoordinates[VFData.TextureCoordinates.Num() - 1], BaseTexCoordAttribute + CoordinateIndex, InOutStreams));
#else
					Elements.Add(AccessStreamComponent(VFData.TextureCoordinates[VFData.TextureCoordinates.Num() - 1], BaseTexCoordAttribute + CoordinateIndex));
#endif
				}
			}

			// Fill PreSkinPosition slot for GPUSkinPassThrough vertex factory, or else use a dummy buffer.
			FVertexStreamComponent NullComponent(&GNullVertexBuffer, 0, 0, VET_Float4);
#if RMC_ENGINE_ABOVE_5_2
		Elements.Add(AccessStreamComponent(VFData.PreSkinPositionComponent.VertexBuffer ? VFData.PreSkinPositionComponent : NullComponent, 14, InOutStreams));
#else
		Elements.Add(AccessStreamComponent(VFData.PreSkinPositionComponent.VertexBuffer ? VFData.PreSkinPositionComponent : NullComponent, 14));
#endif

			if (VFData.LightMapCoordinateComponent.VertexBuffer)
			{
#if RMC_ENGINE_ABOVE_5_2
				Elements.Add(AccessStreamComponent(VFData.LightMapCoordinateComponent, 15, InOutStreams));
#else
				Elements.Add(AccessStreamComponent(VFData.LightMapCoordinateComponent, 15));
#endif
			}
			else if (VFData.TextureCoordinates.Num())
			{
#if RMC_ENGINE_ABOVE_5_2
				Elements.Add(AccessStreamComponent(VFData.TextureCoordinates[0], 15, InOutStreams));
#else
				Elements.Add(AccessStreamComponent(VFData.TextureCoordinates[0], 15));
#endif
			}
		}
	}

	void FRealtimeMeshLocalVertexFactory::ReleaseRHI()
	{
		UniformBuffer.SafeRelease();
		FVertexFactory::ReleaseRHI();
	}

	void FRealtimeMeshLocalVertexFactory::SetColorOverrideStream(FRHICommandList& RHICmdList, const FVertexBuffer* ColorVertexBuffer) const
	{
		checkf(ColorVertexBuffer->IsInitialized(), TEXT("Color Vertex buffer was not initialized! Name %s"), *ColorVertexBuffer->GetFriendlyName());
		checkf(IsInitialized() && EnumHasAnyFlags(EVertexStreamUsage::Overridden, Data.ColorComponent.VertexStreamUsage) && ColorStreamIndex > 0,
		       TEXT("Per-mesh colors with bad stream setup! Name %s"), *ColorVertexBuffer->GetFriendlyName());
		RHICmdList.SetStreamSource(ColorStreamIndex, ColorVertexBuffer->VertexBufferRHI, 0);
	}

	void FRealtimeMeshLocalVertexFactory::GetColorOverrideStream(const FVertexBuffer* ColorVertexBuffer, FVertexInputStreamArray& VertexStreams) const
	{
		checkf(ColorVertexBuffer->IsInitialized(), TEXT("Color Vertex buffer was not initialized! Name %s"), *ColorVertexBuffer->GetFriendlyName());
		checkf(IsInitialized() && EnumHasAnyFlags(EVertexStreamUsage::Overridden, Data.ColorComponent.VertexStreamUsage) && ColorStreamIndex > 0,
			   TEXT("Per-mesh colors with bad stream setup! Name %s"), *ColorVertexBuffer->GetFriendlyName());

		VertexStreams.Add(FVertexInputStream(ColorStreamIndex, 0, ColorVertexBuffer->VertexBufferRHI));
	}
}

using namespace RealtimeMesh;

IMPLEMENT_VERTEX_FACTORY_PARAMETER_TYPE(FRealtimeMeshLocalVertexFactory, SF_Vertex, FRealtimeMeshVertexFactoryShaderParameters);
#if RHI_RAYTRACING
IMPLEMENT_VERTEX_FACTORY_PARAMETER_TYPE(FRealtimeMeshLocalVertexFactory, SF_RayHitGroup, FRealtimeMeshVertexFactoryShaderParameters);

IMPLEMENT_VERTEX_FACTORY_PARAMETER_TYPE(FRealtimeMeshLocalVertexFactory, SF_Compute, FRealtimeMeshVertexFactoryShaderParameters);
#endif // RHI_RAYTRACING

#if RMC_ENGINE_ABOVE_5_2
IMPLEMENT_VERTEX_FACTORY_TYPE(FRealtimeMeshLocalVertexFactory, "/Engine/Private/LocalVertexFactory.ush",
                              EVertexFactoryFlags::UsedWithMaterials
                              | EVertexFactoryFlags::SupportsDynamicLighting
                              | EVertexFactoryFlags::SupportsPrecisePrevWorldPos
                              | EVertexFactoryFlags::SupportsPositionOnly
                              | EVertexFactoryFlags::SupportsCachingMeshDrawCommands
                              | EVertexFactoryFlags::SupportsPrimitiveIdStream
							  | EVertexFactoryFlags::SupportsRayTracing
                              | EVertexFactoryFlags::SupportsRayTracingDynamicGeometry
                              | EVertexFactoryFlags::SupportsManualVertexFetch
							  | EVertexFactoryFlags::SupportsPSOPrecaching
							  | EVertexFactoryFlags::SupportsLumenMeshCards
);
#else
IMPLEMENT_VERTEX_FACTORY_TYPE(FRealtimeMeshLocalVertexFactory, "/Engine/Private/LocalVertexFactory.ush",
							  EVertexFactoryFlags::UsedWithMaterials
							  | EVertexFactoryFlags::SupportsDynamicLighting
							  | EVertexFactoryFlags::SupportsPrecisePrevWorldPos
							  | EVertexFactoryFlags::SupportsPositionOnly
							  | EVertexFactoryFlags::SupportsCachingMeshDrawCommands
							  | EVertexFactoryFlags::SupportsPrimitiveIdStream
							  | EVertexFactoryFlags::SupportsRayTracing
							  | EVertexFactoryFlags::SupportsRayTracingDynamicGeometry
							  | EVertexFactoryFlags::SupportsManualVertexFetch
);
#endif