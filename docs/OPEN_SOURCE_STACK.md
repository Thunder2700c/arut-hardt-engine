# Open-source engine stack research

Last reviewed: 2026-10-17  
Target: Android ARM64 first, with a Linux desktop editor; Godot 4.7 source baseline; C++-first engine systems; Vulkan/Mobile renderer on devices that support it.

## Decision summary

Do not replace Godot's whole renderer with another engine in the current milestone. Keep Godot's scene/resource importer, renderer abstraction and Android platform layer, and add ARUT-owned systems and a curated, licensed starter asset library. A full renderer transplant would also require new material import, scene integration, resource lifetime rules, editor tooling, shader compilation and platform validation.

The first target is a polished *mobile stylized-PBR / anime-hybrid* pipeline—not a claim that ARUT can reproduce Arknights: Endfield's proprietary renderer. Results depend on art direction, textures, lighting, animation, and disciplined budgets as much as shader code.

## Stack shortlist

| Area | Recommended starting point | Why it fits | Status / guardrail |
|---|---|---|---|
| Base renderer | Godot 4.7 Mobile renderer over Vulkan; keep Compatibility as a fallback target | Existing Android renderer, device-aware raster pipeline and existing scene/material workflows | Keep; profile on real Android hardware. Do not enable desktop-only features by assumption. |
| PBR foundation | Godot's built-in PBR materials and renderer | Provides metallic/roughness, normal maps, ORM, toon shading modes, fog, glow and other engine-level material features without another renderer | Keep and author ARUT presets rather than importing a second renderer. |
| Anime material stack | ARUT-owned toon/PBR hybrid shaders, outline/rim-light options, artist-adjustable shadow thresholds, optional face/hair presets | Gives users a cohesive style and Inspector controls; can be adapted to mobile raster constraints | Build as a small shader library; every effect needs a Mobile profile and a low-cost switch. |
| Physics backend | Jolt Physics already integrated into modern Godot as an alternative backend; compare against Godot Physics using ARUT tests | C++ physics library, MIT license upstream, has vehicle constraints and strong rigid-body features | Preferred candidate to evaluate, not yet a declaration that every ARUT system works with it. Test car, ragdoll and joints first. |
| Ready-made car | ARUT-native C++ `ArutCar3D` with internally managed wheel probes, simple drivetrain/steering/brakes, touch/gamepad/keyboard maps and a sample vehicle scene | The user needs a component that works by adding one node, not a separate physics SDK project | Improve and test this native component. Do not depend on Godot's `VehicleBody3D` as the final realistic vehicle system; upstream docs note known limitations. |
| Humanoid animation | Quaternius Universal Base Characters + Universal Animation Library (and optional Animation Library 2) as CC0 source assets | Publicly documented humanoid rigs and 120+/130+ animation packs, including locomotion, sprint, combat and death; GLB/glTF assets fit Godot import | Curate only the needed assets and preserve an asset manifest. These packs are stylized human characters, not a robot mannequin out of the box. |
| Default test dummy | Prototype with Quaternius's **Animated LowPoly Robot** (CC0; 14 clips, about 7 MB via OpenGameArt), with a matte-white material override; separately build an engine-owned human-proportioned ARUT Test Mannequin | Closest small-pack match to a robot-like unskinned placeholder; keeps the first test asset small | Test the downloaded rig/clip names and FBX import first. The pack is stylized, not a blank human mannequin; use it as a prototype, not a hard dependency. For human rigs, use Quaternius Universal Base Characters + Universal Animation Library and retarget where compatible. |
| Reusable examples | Selected Godot demo-project ideas/assets under their MIT license, only after checking each project and dependency | Useful source of examples and regression scenes | Treat as references/templates, not as a replacement for the native C++ foundation. |
| Optional research references | Google Filament (Apache-2.0) and bgfx (BSD-2-Clause) | High-quality PBR renderer / cross-platform rendering library with Android and Vulkan support | Do not embed now. Both would need a serious renderer/resource/editor integration layer; neither is a drop-in Godot renderer upgrade. |
| PhysX | Keep as research only | Capable C++ SDK, but not a drop-in Android-ready Godot PhysicsServer backend | Do not add a second physics SDK before the Jolt-vs-Godot evaluation. Audit the precise release's platform build and license files; the public PhysX repository has conflicting license statements across repository-level and SDK readme materials. |

## Renderer limits that must shape the design

Godot's official rendering documentation distinguishes the Mobile renderer from Forward+. Mobile uses a traditional raster/forward path aimed at mobile-GPU bandwidth and power constraints. It does not provide all desktop rendering features; in particular, do not build the target around Forward+-only SDFGI/SSR/SSAO, volumetric fog, or TAA assumptions.

Practical ARUT profile targets:

- **Mobile Anime:** Vulkan/Mobile renderer; stylized PBR, outlines and rim light, restrained glow, baked lighting/light probes where appropriate, fog, LODs and instancing.
- **Mobile High:** higher-quality shadows/textures, carefully limited transparent effects and lights; optional depth-of-field only after profiling.
- **Compatibility:** simpler materials, no dependence on Vulkan-only compositor/compute effects, and a separate validation pass.

Do not enable every post effect simultaneously. Screen-reading effects, outlines, transparent hair and extra render passes can increase bandwidth and overdraw on tile-based mobile GPUs. Add features behind explicit quality controls, then benchmark frame time, memory, thermal behavior and visual artifacts on multiple devices. The target phone alone is not a substitute for a compatibility plan.

## Character asset plan

1. Prototype quickly with [Quaternius Animated LowPoly Robot](https://opengameart.org/content/animated-lowpoly-robot) (CC0, 14 animations, reported 7 MB archive) and apply a matte-white material override. In parallel, build the engine-owned neutral white humanoid mannequin with human-scale proportions, clear joints, a proper skeleton and locomotion animations. The white dummy should remain readable with no downloadable skin or texture pack.
2. Use a glTF/GLB-based asset pipeline. Keep mesh, skeleton and named animations together where practical; validate bone names, scale, root motion and import settings.
3. Integrate only a small locomotion subset first: idle, walk, run/sprint, jump/fall/land. Combat and parkour clips can be optional content packs.
4. Provide a C++-native character/animation component with an Inspector interface for locomotion speed, acceleration, jump, camera and animation mapping. Animation data can be an imported asset; engine behavior stays C++-first.
5. Keep source/attribution/licence metadata for any model, animation, shader or code bundle. CC0 means the original pack's license permits reuse, but the engine should still document where each asset came from.

## Vehicle plan

The starter should contain a chassis mesh and collision shape, visible wheels, wheel probes, suspension/damping, steering, engine torque, brakes, drive layout, speed telemetry, reset action and mobile touch controls. A prefab/scene should run without the user manually assembling 15 child nodes. The high-level ARUT API should target Godot's physics server rather than hard-code the Jolt-specific API; this preserves the ability to test backend options.

The existing custom `ArutCar3D` code is only a first pass. The last successful workflow proves its source compiled into an Android editor build; it does **not** prove that steering, tire grip, suspension or touch controls behave correctly at runtime.

## Source links reviewed

- [Godot 4.7 rendering architecture](https://docs.godotengine.org/en/4.7/engine_details/architecture/internal_rendering_architecture.html)
- [Godot 4.7 rendering feature list](https://docs.godotengine.org/en/4.7/about/list_of_features.html)
- [Godot 4.7 physics documentation / Jolt](https://docs.godotengine.org/en/4.7/tutorials/physics/index.html)
- [Godot VehicleBody3D limitations](https://docs.godotengine.org/en/4.4/classes/class_vehiclebody3d.html)
- [Jolt Physics upstream (MIT)](https://github.com/jrouwe/JoltPhysics)
- [Quaternius Animated Robot Pack (CC0)](https://quaternius.com/packs/animatedrobot.html)
- [Animated LowPoly Robot pack download/source listing (CC0)](https://opengameart.org/content/animated-lowpoly-robot)
- [Quaternius Universal Base Characters (CC0)](https://quaternius.com/packs/universalbasecharacters.html)
- [Quaternius Universal Animation Library (CC0)](https://quaternius.com/packs/universalanimationlibrary.html)
- [Quaternius Universal Animation Library 2 (CC0)](https://quaternius.com/packs/universalanimationlibrary2.html)
- [Godot demo projects (MIT)](https://github.com/godotengine/godot-demo-projects)
- [Google Filament (Apache-2.0)](https://github.com/google/filament)
- [bgfx (BSD-2-Clause)](https://github.com/bkaradzic/bgfx)
- [NVIDIA PhysX repository](https://github.com/NVIDIA-Omniverse/PhysX)

## What we should not do

- Do not bundle full copies of several competing renderers or physics solvers just because each is high quality.
- Do not claim Nanite, Endfield-level rendering, a working Jolt vehicle backend or realistic car physics without implementation and benchmarks.
- Do not dump a 100+ MB source asset pack into the editor APK. Keep engine/runtime small and offer a compact starter content pack.
- Do not remove upstream licence/copyright notices; rename the product-facing UI while preserving proper attribution in About and distribution notices.
