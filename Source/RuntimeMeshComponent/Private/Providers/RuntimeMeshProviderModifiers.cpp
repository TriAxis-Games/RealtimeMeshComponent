// Copyright 2016-2020 Chris Conway (Koderz). All Rights Reserved.


#include "Providers/RuntimeMeshProviderModifiers.h"



void URuntimeMeshProviderModifiers::AddModifier(URuntimeMeshModifier* NewModifier)
{
	Modifiers.Add(NewModifier);
}

void URuntimeMeshProviderModifiers::RemoveModifier(URuntimeMeshModifier* ModifierToRemove)
{
	Modifiers.Remove(ModifierToRemove);
}

bool URuntimeMeshProviderModifiers::GetSectionMeshForLOD_Implementation(int32 LODIndex, int32 SectionId, FRuntimeMeshRenderableMeshData& MeshData)
{
	URuntimeMeshProvider* ChildProviderTemp = GetChildProvider();
	if (ChildProviderTemp)
	{
		if (ChildProviderTemp->GetSectionMeshForLOD_Implementation(LODIndex, SectionId, MeshData))
		{
			ApplyModifiers(MeshData);
			return true;
		}
	}
	return false;
}

bool URuntimeMeshProviderModifiers::GetAllSectionsMeshForLOD_Implementation(int32 LODIndex, TMap<int32, FRuntimeMeshSectionData>& MeshDatas)
{
	URuntimeMeshProvider* ChildProviderTemp = GetChildProvider();
	if (ChildProviderTemp)
	{
		if (ChildProviderTemp->GetAllSectionsMeshForLOD_Implementation(LODIndex, MeshDatas))
		{
			for (auto& Entry : MeshDatas)
			{
				ApplyModifiers(Entry.Value.MeshData);
				return true;
			}
		}
	}
	return false;
}

bool URuntimeMeshProviderModifiers::GetCollisionMesh_Implementation(FRuntimeMeshCollisionData& CollisionData)
{
	URuntimeMeshProvider* ChildProviderTemp = GetChildProvider();
	if (ChildProviderTemp)
	{
		if (ChildProviderTemp->GetCollisionMesh_Implementation(CollisionData))
		{
			ApplyModifiers(CollisionData);
			return true;
		}
	}
	return false;
}

void URuntimeMeshProviderModifiers::ApplyModifiers(FRuntimeMeshRenderableMeshData& MeshData)
{
	if (MeshData.HasValidMeshData())
	{
		for (int32 Index = 0; Index < Modifiers.Num(); Index++)
		{
			Modifiers[Index]->ApplyToMesh(MeshData);
		}
	}
}

void URuntimeMeshProviderModifiers::ApplyModifiers(FRuntimeMeshCollisionData& CollisionData)
{
	if (CollisionData.HasValidMeshData())
	{
		for (int32 Index = 0; Index < Modifiers.Num(); Index++)
		{
			Modifiers[Index]->ApplyToCollisionMesh(CollisionData);
		}
	}
}
