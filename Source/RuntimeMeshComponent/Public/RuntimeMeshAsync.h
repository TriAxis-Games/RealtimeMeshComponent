// Copyright 2016 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "RuntimeMeshCore.h"

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
	static void CreateMeshSection(TWeakObjectPtr<RuntimeMeshComponent*> InRuntimeMeshComponent, int32 SectionIndex, TArray<VertexType>& Vertices, TArray<int32>& Triangles, bool bCreateCollision = false,
		EUpdateFrequency UpdateFrequency = EUpdateFrequency::Average, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{

	}





};
