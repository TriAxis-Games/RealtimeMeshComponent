---
title: Quickstart
description: Build your first RealtimeMesh actor in C++. A single triangle, from project setup to a rendered mesh.
---

This is the shortest path to something on screen: a C++ actor that builds its own triangle.

It follows the same steps as the plugin's own `ARealtimeMeshExample_Simple_HelloTriangle` example (`Source/RealtimeMeshExamples/Private/Simple/RealtimeMeshExample_Simple_HelloTriangle.cpp`), so if you get stuck you can always open the working version and compare.

## Before you start

You need a C++ ready Unreal project with the plugin installed (see [Installation](../installation/)), and `RealtimeMeshComponent` added to your module's `Build.cs`:

```csharp
PublicDependencyModuleNames.Add("RealtimeMeshComponent");
```

## 1. Make an actor

`ARealtimeMeshActor` is a base class that already owns a `URealtimeMeshComponent` and has the mobility, collision profile, and root component set up for you. Derive from it:

```cpp
// MyFirstMesh.h
#pragma once

#include "CoreMinimal.h"
#include "RealtimeMeshActor.h"
#include "MyFirstMesh.generated.h"

UCLASS()
class MYGAME_API AMyFirstMesh : public ARealtimeMeshActor
{
	GENERATED_BODY()

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
};
```

Building the mesh in `OnConstruction` means it appears the moment you drag the actor into a level, and rebuilds as you move it around. That is what all the shipped examples do.

If your geometry is expensive to generate, you probably do not want it rebuilding every time you nudge the actor in the editor. In that case set `bDeferGeneration = true` (a property on `ARealtimeMeshActor`) and put your code in `OnGenerateMesh_Implementation()` instead. That only runs when something explicitly asks for a rebuild.

## 2. Build the geometry

In `OnConstruction`, tell the component which kind of mesh you want. `URealtimeMeshSimple` is the general purpose one, and the closest in feel to `ProceduralMeshComponent`:

```cpp
// MyFirstMesh.cpp
#include "MyFirstMesh.h"
#include "RealtimeMeshSimple.h"

using namespace RealtimeMesh;

void AMyFirstMesh::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	URealtimeMeshSimple* RealtimeMesh = GetRealtimeMeshComponent()->InitializeRealtimeMesh<URealtimeMeshSimple>();

	// A StreamSet holds the raw mesh data: positions, normals, UVs, triangles, and so on.
	FRealtimeMeshStreamSet StreamSet;

	// TRealtimeMeshBuilderLocal is a helper that fills a StreamSet in for you.
	// The template arguments pick the storage format: index type, tangent type,
	// UV type, and how many UV channels you want.
	TRealtimeMeshBuilderLocal<uint16, FPackedNormal, FVector2DHalf, 1> Builder(StreamSet);

	Builder.EnableTangents();
	Builder.EnableTexCoords();
	Builder.EnableColors();
	Builder.EnablePolyGroups();

	const int32 V0 = Builder.AddVertex(FVector3f(-50.0f, 0.0f, 0.0f))
		.SetNormalAndTangent(FVector3f(0.0f, -1.0f, 0.0f), FVector3f(1.0f, 0.0f, 0.0f))
		.SetColor(FColor::Red)
		.SetTexCoord(FVector2f(0.0f, 0.0f));

	const int32 V1 = Builder.AddVertex(FVector3f(0.0f, 0.0f, 100.0f))
		.SetNormalAndTangent(FVector3f(0.0f, -1.0f, 0.0f), FVector3f(1.0f, 0.0f, 0.0f))
		.SetColor(FColor::Green)
		.SetTexCoord(FVector2f(0.5f, 1.0f));

	const int32 V2 = Builder.AddVertex(FVector3f(50.0f, 0.0f, 0.0f))
		.SetNormalAndTangent(FVector3f(0.0f, -1.0f, 0.0f), FVector3f(1.0f, 0.0f, 0.0f))
		.SetColor(FColor::Blue)
		.SetTexCoord(FVector2f(1.0f, 0.0f));

	// Add the triangle, tagged into poly group 0.
	Builder.AddTriangle(V0, V1, V2, 0);
```

Three things worth knowing about that block:

* You need `using namespace RealtimeMesh;` for `FRealtimeMeshStreamSet` and the builder types to resolve.
* `AddVertex` hands back a small handle you can chain `.Set...()` calls onto, which is why the vertex setup reads as one statement.
* `EnablePolyGroups()` lets you tag each triangle with a group number. One set of buffers can then be split into several separately rendered pieces. See [Poly Groups](../keyconcepts/meshes/#polygroups).

## 3. Set up a material and hand over the geometry

Material slots go on the mesh object. Then you create a **buffer set**, which is the container that owns the GPU buffers for a chunk of geometry:

```cpp
	// One material slot per poly group used above.
	RealtimeMesh->SetupMaterialSlot(0, "PrimaryMaterial");

	const FRealtimeMeshBufferSetKey GroupKey = FRealtimeMeshBufferSetKey::Create(0, FName("MyTriangle"));

	// Because the StreamSet has poly groups, this creates one section per group automatically.
	RealtimeMesh->CreateBufferSet(GroupKey, MoveTemp(StreamSet));
}
```

`CreateBufferSet` takes the `StreamSet` by rvalue reference, so pass it with `MoveTemp` as shown. The buffer set then takes ownership of the data instead of copying it, which matters a lot once your meshes get big.

## 4. See it

Compile, drop `AMyFirstMesh` into a level, and you have a triangle. Assign a material to the `PrimaryMaterial` slot in the actor's details panel and it will use that.

## Where to go next

* [Key Concepts](../keyconcepts/) goes deeper on streams, stream sets, and the builders.
* [Component Core](../component-core/) covers materials, LODs, sections, and changing geometry after it is built.
* [Examples](../examples/) walks through every example actor, from this triangle up to Nanite and spatial streaming.
