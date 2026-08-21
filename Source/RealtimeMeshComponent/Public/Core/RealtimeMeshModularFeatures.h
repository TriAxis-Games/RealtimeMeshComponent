// Copyright (c) 2015-2026 TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreFwd.h"
#include "Features/IModularFeature.h"
#include "Features/IModularFeatures.h"

namespace RealtimeMesh
{
	/*
	 * Helper to auto register/unregister a modular part of the interface
	 */
	template<typename Type>
	class TRealtimeMeshModularFeatureRegistration : FNoncopyable
	{
	private:
		Type Interface;
	public:
		TRealtimeMeshModularFeatureRegistration()
		{
			IModularFeatures::Get().RegisterModularFeature(Type::GetModularFeatureName(), &Interface);
		}

		~TRealtimeMeshModularFeatureRegistration()
		{
			IModularFeatures::Get().UnregisterModularFeature(Type::GetModularFeatureName(), &Interface);
		}
	};
}