---
title: Install from GitHub
description: Install the RealtimeMeshComponent from GitHub source, either Core (free) or Pro.
---

Installing RMC from [GitHub](https://github.com/TriAxis-Games/RealtimeMeshComponent) takes a few more steps than Fab, but gives you the source.

1. **You need a C++ ready project.** If your project has no C++ in it yet, use `Tools -> New C++ Class` in the editor and create any template actor. That converts the project to a C++ project and sets up a Visual Studio solution for you. Epic's [C++ quick start](https://docs.unrealengine.com/5.0/en-US/unreal-engine-cpp-quick-start/) is worth a read if this is new to you.

2. **Download the plugin.** There are two versions:
   * **Core** is free, and lives in the [RealtimeMeshComponent](https://github.com/TriAxis-Games/RealtimeMeshComponent) repository. Grab a [tagged release](https://github.com/TriAxis-Games/RealtimeMeshComponent/releases) or clone the repository.
   * **Pro** is the paid version, with the full feature set: Nanite, distance fields and Lumen cards, spatial streaming, and compute providers. You get access through your purchase. See the repository for current details.

3. **Put it in the right place.** If you downloaded a zip, unzip it into:
   ```
   {YourProject}/Plugins/RealtimeMeshComponent/
   ```
   If you cloned the repository, clone or move it to that same path.

4. **Build it.** Open your `.uproject` file, or open the project from your IDE. The engine will offer to build the new plugin modules for you.

5. **Add it to your module.** To call RMC from C++, add it to your `Build.cs` like any other dependency:
   ```csharp
   PublicDependencyModuleNames.Add("RealtimeMeshComponent");
   ```

From here, try the [Quickstart](../../quickstart/) to build your first mesh, or look at the [Examples](../examples/) that ship with the plugin.
