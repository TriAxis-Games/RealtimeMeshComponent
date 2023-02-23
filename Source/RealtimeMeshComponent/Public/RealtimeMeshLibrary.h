// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Data/RealtimeMeshConfig.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RealtimeMeshLibrary.generated.h"


UCLASS(meta=(ScriptName="RealtimeMeshLibrary"))
class REALTIMEMESHCOMPONENT_API URealtimeMeshBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, meta = (DisplayName = "LODIndex to LODKey", CompactNodeTitle = "->", BlueprintAutocast), Category = "RealtimeMesh|Key")
	static FRealtimeMeshLODKey Conv_IntToRealtimeMeshLODKey(int32 LODIndex);

	
};