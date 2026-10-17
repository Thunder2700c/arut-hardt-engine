# Architecture

## Product profiles
- ARUT Mobile: Android ARM64, phone-first editor/runtime defaults.
- HARDT Studio: desktop editor, initially Linux x86_64.
Both use the same engine API, project format, version and resource compatibility.

## Source assembly
This repository stores reviewed patches, native extension sources, renderer resources, tests, docs and workflows. A manual CI job fetches matching Godot upstream source to a temporary folder, checks/applies `engine/patches/*.patch`, compiles, and uploads artifacts. Generated Godot source and game assets are not stored here.

This is currently an upstream source-build overlay, not yet a full mirrored copy of godotengine/godot. Reassess a true maintained source fork when patches become substantial.

## Layers
1. Godot baseline: scene tree, editor, physics, importers, platform runtime.
2. Built-in ARUT C++ module overlay (`engine/modules/arut`): first-class native component classes registered directly in the engine.
3. Optional native C++ GDExtension: CPU-side systems that do not need built-in engine/editor integration.
4. Engine patch layer: renderer/editor/runtime internals unavailable through GDExtension.
5. Renderer assets: tested shaders and material presets.
6. Quality profiles: Mobile/Balanced/High with explicitly costed options.

## Graphics order
1. Material/colour consistency and low-cost key light.
2. Sky/ambient fill and distance atmosphere.
3. Albedo, normal/AO and UV/triplanar behaviour.
4. Optional outlines/rim light only if useful and cheap.
5. Expensive GI/reflections only after profiling.

The aim is a distinctive indie-game renderer, not a claim of automatic UE5/NTE parity.
