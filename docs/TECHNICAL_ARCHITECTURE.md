# ARUT-HARDT Technical Architecture

**Status:** Initial architecture baseline, revision 0.1.  
**Implementation state:** Bootstrap only; most systems below are design targets, not implemented features.

## Product and platforms

ARUT-HARDT is an independent, Android-first 3D engine intended for large, simulation-heavy open worlds. RAGE, REDengine, IW, and modern commercial engines are high-level design references only. This project will not use their proprietary source, shaders, assets, or undocumented internals.

- Android ARM64 is the primary runtime target.
- Windows and Linux are secondary development/runtime targets.
- The editor is initially a desktop tool; Android-first describes the game runtime, not a requirement to author worlds on a phone.
- The engine is developed before the game built with it.

## Language ownership

### C++20
C++ owns latency-sensitive native interfaces and systems that need direct access to graphics, platform, and physics APIs: Android/desktop platform runtime, Vulkan renderer and GPU resource lifetime, physics adapters and vehicle dynamics, character/animation evaluation, native engine API, profiling, and renderer-facing data.

### Rust
Rust is a first-class systems language, not a blanket rewrite: job/task infrastructure where safe concurrency is beneficial, world/asset streaming orchestration, offline asset processing and validation tools, and selected simulation/data-processing systems after profiling.

A subsystem's language is chosen by measured performance, API fit, complexity, and maintainability. Avoid duplicate implementations without a benchmark-driven reason.

### C++/Rust boundary
- Expose a narrow, versioned C ABI.
- Exchange fixed-layout POD structures, integer handles, byte spans, and explicit error codes.
- Define allocation ownership on one side; memory allocated by one language is released by that same side.
- Never pass STL containers, Rust references, trait objects, C++ classes, exceptions, or Rust panics across the boundary.
- Batch operations; do not cross the ABI once per entity or tiny per-frame operation.
- Keep ABI headers in a dedicated bridge module and test version compatibility.
- Catch C++ exceptions and Rust panics before they can cross the C ABI.

## Engine layers

1. **Platform:** Android lifecycle, filesystem, input, clocks, threads, display, native window, desktop adapters.
2. **Core:** logging, memory, jobs, handles, events, serialization, reflection, diagnostics.
3. **Resources/assets:** import, validation, dependency tracking, compression, packaging, asynchronous residency.
4. **World:** coordinates, cell metadata, streaming, terrain, roads, interiors, vegetation, LOD.
5. **Rendering:** Vulkan device abstraction, render graph, materials, lighting, shadows, visibility, post-processing, GPU profiling.
6. **Physics:** backend adapter, collision queries, fixed-step simulation, character controller, vehicles, joints, destruction interfaces.
7. **Animation:** skeletons, clips, blending, state machines, IK, procedural motion, ragdoll handoff.
8. **AI/simulation:** navigation, traffic, pedestrians, perception, police response, distant-world simulation.
9. **Audio:** spatialization, ambience, occlusion, streaming.
10. **Gameplay:** interactions, missions, saves, game-facing APIs.
11. **Tools:** desktop editor, importers, inspectors, world tools, profiling, packaging.

Modules depend inward through explicit interfaces. Core must not depend on gameplay; platform-specific code must not leak into simulation APIs.

## Android-first graphics

Vulkan is the planned renderer API. Query device, driver, extensions, formats, limits, and memory budget at runtime. Enable optional capabilities only after feature checks.

Initial sequence: Android surface and Vulkan device/swapchain; command submission and synchronization; GPU allocation/lifetime; shader compilation and SPIR-V validation; mesh/texture upload; depth, camera and a directional light; then PBR, shadows, instancing, visibility and profiling.

Begin with a mobile-oriented forward path. Advanced paths are considered only after measuring real devices. Ray tracing, virtualized geometry, and expensive GI are optional experiments, never baseline requirements.

## World representation

- Separate high-precision world positions from local float render/physics coordinates.
- Stream cells asynchronously through explicit states: unloaded, requested, loading, ready, active, evicting, failed.
- Choose cell size by benchmark; do not hard-code an unvalidated guess.
- Prioritize residency using distance, visibility, memory pressure, and gameplay relevance.
- Use tiered simulation: full near the player, simplified farther away, metadata-only at long range.
- Support cancellation, prioritization, and safe unload.

## Simulation and performance

Rendering and simulation clocks are separate. Physics uses a fixed step with an accumulator; the frequency is configurable and measured on target hardware. Frame pacing and input latency are first-class Android concerns. Background tasks require cancellation and shutdown semantics. Avoid unbounded queues and per-frame heap churn.

Track CPU heap, GPU allocations, asset residency, streaming queues, and per-system frame costs. Use pools/arenas only when profiling justifies them. Emulators do not replace real-device GPU and thermal validation.

## Asset conventions

glTF 2.0 is the initial candidate for static/skinned interchange and KTX2 for GPU texture containers; final runtime formats require loading and memory tests. Define handedness, up axis, units, winding, tangent basis, and color-space policy before content tools are built. Preserve source assets and attribution separately from derived runtime assets.

## Dependency policy

Evaluate every dependency for license, maintenance, Android ARM64 support, binary size, API stability, build cost, and target-device performance. See [DEPENDENCIES.md](DEPENDENCIES.md). A candidate is not integrated merely because it appears on that list.

## Validation language

Reports must distinguish written, committed, compiled, unit-tested, integration-tested, device-tested, and profiled. Never report an untested source change as a working feature.
