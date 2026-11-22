# Development Roadmap

Each phase has an acceptance gate. Scope changes only when benchmarks or platform constraints provide evidence.

## Phase 0 — Clean native foundation (active)

- [x] Remove the Godot implementation from the current branch.
- [x] Establish a C++20 CMake project.
- [x] Establish a Rust Cargo workspace and initial ABI version symbol.
- [x] Add basic C++ and Rust tests.
- [x] Add Linux CI for C++ and Rust.
- [ ] Verify the CI run on the clean-slate commit.
- [ ] Add Android NDK CI after an Android native target exists.

**Exit gate:** Clean checkout builds both workspaces and tests pass; no Godot/GDExtension/SCons runtime dependency remains.

## Phase 1 — Android native runtime
Android app/native entry point, ARM64 NDK integration, lifecycle, native window/input, logging/crash handling, pause/resume, surface recreation, GPU capability report, initial C++/Rust linkage.

**Exit gate:** Native Android app starts, handles lifecycle/input, reports GPU capabilities, and passes repeated pause/resume tests on a physical device.

## Phase 2 — Vulkan renderer bootstrap
Vulkan instance/device, development validation, surface/swapchain, synchronization, GPU resources, shader build/validation, mesh/texture upload, depth, camera, first lit scene.

**Exit gate:** Android build renders a textured 3D scene with clean validation output and frame/GPU timing.

## Phase 3 — Rendering and materials
PBR materials, mobile shadows, instancing, visibility culling, environment lighting, tone mapping, quality tiers, dynamic resolution and profiling.

**Exit gate:** Test scene renders across at least two Android GPU families with graceful fallback.

## Phase 4 — Resources and world streaming
Resource handles, asynchronous asset loading, world coordinates/cells, cancellation, prioritization, eviction, budgets, terrain, LOD/HLOD.

**Exit gate:** Test world streams without stalls, leaks, missing dependencies, or invalid resource access.

## Phase 5 — Character and animation
Character controller, skeleton/clip import, playback, blend/state machine, IK, foot placement, ragdoll handoff.

**Exit gate:** Third-person character walks, runs, turns, jumps, and traverses an Android test environment.

## Phase 6 — Vehicle and physics
Physics backend evaluation, collision queries, fixed-step integration, raycast-wheel prototype, steering, suspension, tire response, transmission, damage hooks, vehicle entry/exit.

**Exit gate:** Drivable vehicle handles repeatable benchmark routes with measured stability and frame cost.

## Phase 7 — Open-world simulation
Road graph, lane representation, traffic, intersections, pedestrians, navigation, police pursuit, distance-based simulation tiers, persistent world state.

**Exit gate:** Small city block runs concurrent vehicles and pedestrians in repeatable stress scenarios.

## Phase 8 — Environment, audio, gameplay
Time-of-day, weather, water, fog, wind, vegetation, spatial audio, interactions, combat primitives, mission API, saves, destruction experiments.

**Exit gate:** Coherent playable vertical slice with measured Android performance budget.

## Phase 9 — ARUT Studio
Desktop editor, viewport, scene/world hierarchy, inspectors, asset import/browser, material/terrain/vehicle/animation tools, profiler, packaging.

**Exit gate:** Small world can be authored, imported, configured, packaged and run without editing engine internals.

## Phase 10 — Production hardening
Automated regression scenes, device matrix, thermal soak, memory/battery/load-time/frame pacing optimization, crash recovery, asset validation, reproducible release packages.

**Exit gate:** Release candidate passes its declared Android device matrix and desktop smoke tests.

## Rule

Do not skip to ambitious features because they are visually exciting. Keep the next milestone small, verifiable, and buildable from a clean checkout.
