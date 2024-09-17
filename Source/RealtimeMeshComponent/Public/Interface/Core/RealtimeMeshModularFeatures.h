// Copyright (c) 2015-2024 TriAxis Games, L.L.C. All Rights Reserved.

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


	/*
	 * Helper to auto capture/release a modular part of the interface
	 */
	template<typename Type>
	struct TRealtimeMeshModularInterface
	{	
		Type* Interface;

		FDelegateHandle OnRegisteredHandle;
		FDelegateHandle OnUnregisteredHandle;
	public:
		TRealtimeMeshModularInterface(bool bAutoBind = true)
			: Interface(nullptr)
		{
			if (bAutoBind)
			{
				Bind();
			}
		}

		~TRealtimeMeshModularInterface()
		{
			Release();
		}

		Type& operator*()
		{
			check(Interface);
			return *Interface;
		}

		Type* operator->()
		{
			check(Interface);
			return Interface;
		}

		bool IsAvailable() const
		{
			return Interface != nullptr || IModularFeatures::Get().IsModularFeatureAvailable(Type::GetModularFeatureName());
		}

		bool IsConnected() const
		{
			return Interface != nullptr;
		}

	private:
		void Bind()
		{
			IModularFeatures& ModularFeatures = IModularFeatures::Get();
			OnRegisteredHandle = ModularFeatures.OnModularFeatureRegistered().AddRaw(this, &TRealtimeMeshModularInterface::OnRegisteredFeature);
			OnUnregisteredHandle = ModularFeatures.OnModularFeatureUnregistered().AddRaw(this, &TRealtimeMeshModularInterface::OnUnregisteredFeature);
		
			if (ModularFeatures.IsModularFeatureAvailable(Type::GetModularFeatureName()))
			{
				Interface = &ModularFeatures.GetModularFeature<Type>(Type::GetModularFeatureName());
			}
		}

		void Release()
		{
			Interface = nullptr;
			if (OnRegisteredHandle.IsValid())
			{
				IModularFeatures& ModularFeatures = IModularFeatures::Get();
				ModularFeatures.OnModularFeatureRegistered().Remove(OnRegisteredHandle);
				ModularFeatures.OnModularFeatureUnregistered().Remove(OnUnregisteredHandle);
				OnRegisteredHandle.Reset();
				OnUnregisteredHandle.Reset();
			}
		}

	
		void OnRegisteredFeature(const FName& Name, IModularFeature* Feature)
		{
			if (Name == Type::GetModularFeatureName())
			{
				Interface = static_cast<Type*>(Feature);
			}
		}
		void OnUnregisteredFeature(const FName& Name, IModularFeature* Feature)
		{
			if (Name == Type::GetModularFeatureName())
			{
				Interface = nullptr;
			}
		}
	};
}