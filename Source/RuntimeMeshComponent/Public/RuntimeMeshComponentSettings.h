// Copyright 2016-2020 TriAxis Games L.L.C. All Rights Reserved.

#pragma once

#include "Engine/DeveloperSettings.h"
#include "RuntimeMeshCore.h"
#include "RuntimeMeshComponentSettings.generated.h"



/**
 * Settings for Runtime Mesh Component
 */
UCLASS(Config = RuntimeMeshComponent, DefaultConfig, Meta = (DisplayName = "Runtime Mesh Component"))
class RUNTIMEMESHCOMPONENT_API URuntimeMeshComponentSettings : public UDeveloperSettings
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(Config, EditAnywhere, Category = MeshData)
	ERuntimeMeshUpdateFrequency DefaultUpdateFrequency;

	UPROPERTY(Config, EditAnywhere, Category = MeshData)
	bool bUse32BitIndicesByDefault;

	UPROPERTY(Config, EditAnywhere, Category = MeshData)
	bool bUseHighPrecisionTexCoordsByDefault;

	UPROPERTY(Config, EditAnywhere, Category = MeshData)
	bool bUseHighPrecisionTangentsByDefault;



	UPROPERTY(Config, EditAnywhere, Category = Collision)
	bool bCookCollisionAsync;

	UPROPERTY(Config, EditAnywhere, Category = Collision)
	ERuntimeMeshCollisionCookingMode DefaultCookingMode;



	UPROPERTY(Config, EditAnywhere, Category = Threading, Meta = (ClampMin=1, ClampMax = 32))
	FInt32Range MinMaxThreadPoolThreads;

	UPROPERTY(Config, EditAnywhere, Category = Threading, Meta = (ClampMin=1, ClampMax = 8))
	int32 SystemThreadDivisor;

	UPROPERTY(Config, EditAnywhere, Category = Threading)
	ERuntimeMeshThreadingPriority ThreadPriority;

	UPROPERTY(Config, EditAnywhere, Category = Threading)
	int32 ThreadStackSize;

	UPROPERTY(Config, EditAnywhere, Category = Threading)
	float MaxAllowedTimePerTick;




	FName GetCategoryName() const override;

#if WITH_EDITOR
	FText GetSectionText() const override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	
	
	DECLARE_MULTICAST_DELEGATE_TwoParams(FOnRuntimeMeshComponentSettingsChanged, const FString&, const URuntimeMeshComponentSettings*);

	static FOnRuntimeMeshComponentSettingsChanged& OnSettingsChanged();
protected:
	static FOnRuntimeMeshComponentSettingsChanged SettingsChangedDelegate;
#endif

};
