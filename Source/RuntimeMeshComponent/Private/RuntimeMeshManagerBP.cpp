// Copyright 2016-2019 Gabriel Zerbib (Moddingear). All rights reserved.


#include "RuntimeMeshManagerBP.h"

URuntimeMeshManagerBP::URuntimeMeshManagerBP()
{

};

bool URuntimeMeshManagerBP::HasSubmesh(int32 id)
{
	return RuntimeMeshManager.HasMesh(id);
}

FRuntimeMeshDataStructBP URuntimeMeshManagerBP::GetMesh()
{
	return FRuntimeMeshDataStructBP(RuntimeMeshManager.GetMesh());
}

FRuntimeMeshDataStructBP URuntimeMeshManagerBP::GetSubmesh(int32 id)
{
	return RuntimeMeshManager.GetMesh(id);
}

void URuntimeMeshManagerBP::AddSubmesh(int32 id, FRuntimeMeshDataStructBP mesh)
{
	RuntimeMeshManager.AddMesh(id, mesh);
}

void URuntimeMeshManagerBP::RemoveSubmesh(int32 id)
{
	RuntimeMeshManager.RemoveMesh(id);
}
