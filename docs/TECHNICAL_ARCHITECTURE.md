# ARUT-HARDT Technical Architecture

Status: Architecture baseline, revision 0.5. Android runtime and Vulkan swapchain bootstrap exist. The renderer source contains an indexed PBR mesh path, decoded material textures, GGX environment/BRDF integration, cosine-convolved diffuse irradiance, quality-selected directional cascades, CPU visibility culling and optional GPU timestamps. Host and Android ARM64 native compile checks run without APK packaging; physical GPU behaviour remains unverified.

## Platform targets
Android ARM64 is the primary runtime. Windows and Linux are secondary development/runtime targets. The editor is initially desktop-based. Commercial engines are design references only; no proprietary source, shaders or assets are used.

## Language ownership
C++20 owns platform runtime, Vulkan renderer/resource lifetime, physics adapters, vehicle dynamics, animation evaluation and latency-sensitive APIs.
Rust is first-class but not a blanket rewrite: task scheduling, asset/world streaming orchestration, offline processing and selected data-oriented systems where profiling justifies it.

## C++/Rust boundary
Use a narrow, versioned C ABI. Exchange fixed-layout POD structures, integer handles, byte spans and explicit error codes. Define allocation ownership on one side. Never pass STL containers, Rust references, trait objects, C++ classes, exceptions or Rust panics across the boundary. Batch calls and catch failures before crossing the ABI.

## Android platform layer
The initial app uses android.app.NativeActivity and NDK native_app_glue. The adapter owns lifecycle/focus, native-window availability, touch/key delivery, logging, low-memory notifications, Vulkan capability discovery and Android packaging. Phase 1 discovers Vulkan; Phase 2 creates a surface, swapchain and renderer. Vulkan is optional at startup.

## Engine layers
1. Platform: lifecycle, filesystem, input, clocks, threads, display and native window.
2. Core: logging, memory, jobs, handles, events, serialization and diagnostics.
3. Resources: import, validation, dependency tracking, compression and asynchronous residency.
4. World: coordinates, cells, streaming, terrain, roads, interiors, vegetation and LOD.
5. Rendering: Vulkan, render graph, materials, lighting, shadows, visibility and profiling.
6. Physics: collision, fixed-step simulation, character controller, vehicles and joints.
7. Animation: skeletons, clips, blending, state machines, IK and procedural motion.
8. AI: navigation, traffic, pedestrians, perception and distant simulation.
9. Audio: spatialization, ambience, occlusion and streaming.
10. Gameplay: interactions, missions, saves and game-facing APIs.
11. Tools: desktop editor, importers, inspectors, world tools and profiling.

## Android-first graphics
Query device, driver, extensions, formats, limits and memory budgets at runtime. Enable optional capabilities only after feature checks. Start with a mobile forward metallic-roughness path using GGX distribution, Schlick Fresnel and Smith geometry. The initial shader contract is documented in `docs/PBR_PIPELINE.md`; CPU reference functions are in `engine/rendering/include/arut/rendering/pbr_brdf.hpp`. Current Vulkan bring-up is documented in `docs/VULKAN_RENDERER.md`. The independent triangle pipeline is a bootstrap path; indexed meshes, camera uniforms and PBR descriptor/pipeline integration remain future work. Ray tracing, virtualized geometry and expensive GI are optional experiments.

## World and performance
Separate high-precision world coordinates from local float render/physics coordinates. Stream cells asynchronously with cancellation, prioritization and safe eviction. Use tiered simulation at distance. Keep rendering and simulation clocks separate; physics uses a fixed step. Frame pacing and input latency are first-class Android concerns.

## Assets and validation
glTF 2.0 and KTX2 are initial interchange candidates, not mandatory runtime formats. Define coordinate, unit, tangent and color-space conventions before content tools. Evaluate dependencies for license, maintenance, Android support, binary size and measured performance. Distinguish written, compiled, tested, device-tested and profiled.
