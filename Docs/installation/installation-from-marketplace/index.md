---
title: Install from Fab
description: Install the RealtimeMeshComponent from Fab, Epic's marketplace for Unreal Engine content.
---

The simplest way to get RMC is through [Fab](https://www.fab.com/), Epic's marketplace for Unreal Engine content. It replaced the old Unreal Engine Marketplace.

1. Find "Realtime Mesh Component Pro" on Fab and add it to your library.
2. In the Epic Games Launcher, install the plugin to the engine version you are using.
3. Open your project. If the plugin is not already switched on, go to `Edit -> Plugins`, search for "Realtime Mesh Component", and tick the box. Restart the editor if it asks you to.

That is everything you need to start using RMC from Blueprints.

## Using RMC from C++

If you want to call RMC from C++ as well, add it as a dependency in your project module's `Build.cs`:

```csharp
PublicDependencyModuleNames.Add("RealtimeMeshComponent");
```

Recompile, and the RMC headers (`RealtimeMeshActor.h`, `RealtimeMeshSimple.h`, and the rest) become available to your module.

Next up: the [Quickstart](../../quickstart/).
