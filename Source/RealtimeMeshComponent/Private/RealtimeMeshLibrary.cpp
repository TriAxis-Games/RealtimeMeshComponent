// Copyright TriAxis Games, L.L.C. All Rights Reserved.

#include "RealtimeMeshLibrary.h"

FRealtimeMeshLODKey URealtimeMeshBlueprintFunctionLibrary::Conv_IntToRealtimeMeshLODKey(int32 LODIndex)
{
	return FRealtimeMeshLODKey(LODIndex);
}
