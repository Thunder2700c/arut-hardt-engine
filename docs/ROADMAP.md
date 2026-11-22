# Development Roadmap

## Phase 0 — Clean native foundation
- [x] C++20 CMake project.
- [x] Rust workspace and initial ABI symbol.
- [x] C++ and Rust tests.
- [x] Linux CI verified.
- [x] Android NDK CI added with native target.
Status: Foundation established.

## Phase 1 — Android native runtime (active)
- [x] NativeActivity app shell and ARM64 Gradle/NDK build.
- [x] Native C++ entry point and Android NDK glue.
- [x] Start/resume/pause/stop, focus, window, configuration and low-memory callbacks.
- [x] Native touch and key input queue.
- [x] Vulkan instance and physical-device capability query.
- [x] Rust ARM64 static library and C++ ABI version call.
- [x] Android CI and APK artifact upload.
- [ ] Confirm CI builds the APK.
- [ ] Install and smoke-test on a physical ARM64 Android device.
- [ ] Verify repeated pause/resume and surface recreation.
- [ ] Verify touch/key input and Vulkan reporting in device logs.

Exit gate: APK installs and starts on a physical ARM64 device, handles lifecycle/input/surface changes, reports GPU capabilities when available, and calls the Rust ABI. CI alone is not physical-device validation.

## Phase 2 — Vulkan renderer bootstrap (in progress)
- [x] Android Vulkan instance with surface extensions.
- [x] Android window surface creation and graphics/present queue selection.
- [x] VK_KHR_swapchain device selection and FIFO swapchain creation.
- [x] Swapchain image views, clear render pass, framebuffers and command buffers.
- [x] Frame synchronization with acquire/submit/present and out-of-date swapchain recreation.
- [x] Connect renderer lifetime to NativeActivity window lifecycle.
- [ ] Confirm Android ARM64 CI compiles and packages the renderer.
- [ ] Install and run on the Galaxy S23 FE; verify the blue-black clear frame and rotation/resume.
- [ ] Add shader modules, graphics pipeline and first triangle.
- [ ] Add vertex/index buffers, depth, camera, textures and first lit 3D scene.
Exit gate: Android renders a textured 3D scene with validation and frame/GPU timing.

## Phase 3 — Rendering and materials (foundation started)
- [x] Define a PBR material data contract (base colour, metallic, roughness, emissive, normal scale, occlusion and alpha cutoff).
- [x] Add finite-value handling and safe material parameter normalization.
- [x] Define Low/Medium/High/Ultra mobile quality presets with render scale, shadow budget and light limits.
- [x] Define a renderer profiling snapshot contract.
- [ ] Implement the PBR shader pipeline and validate energy-conserving BRDF behaviour.
- [ ] Add mobile shadow maps and cascade quality tiers.
- [ ] Add GPU instancing and CPU/GPU visibility culling.
- [ ] Add environment lighting / image-based lighting and tone mapping.
- [ ] Connect profile snapshots to actual renderer counters and GPU timestamp queries.
- [ ] Validate graceful quality fallback on two Android GPU families.
Exit gate: PBR test scene works across two Android GPU families with graceful fallback and measured CPU/GPU costs.

## Phase 4 — Resources and world streaming
Resource handles, asynchronous asset loading, world coordinates/cells, cancellation, prioritization, eviction, budgets, terrain and LOD.
Exit gate: test world streams without stalls, leaks or invalid resource access.

## Phase 5 — Character and animation
Character controller, skeleton/clip import, playback, blending, state machines, IK, foot placement and ragdoll handoff.
Exit gate: third-person character traverses a test environment on Android.

## Phase 6 — Vehicle and physics
Physics backend, collision, fixed-step integration, vehicle prototype, steering, suspension, tire response, transmission and entry/exit.
Exit gate: drivable vehicle handles repeatable benchmark routes with measured stability and frame cost.

## Phase 7 — Open-world simulation
Road graph, traffic, intersections, pedestrians, navigation, police pursuit, distance-based simulation and persistent world state.
Exit gate: small city block runs concurrent agents in repeatable stress scenarios.

## Phase 8 — Environment, audio and gameplay
Weather, water, fog, wind, vegetation, spatial audio, interactions, combat primitives, mission API and saves.
Exit gate: playable vertical slice with measured Android performance budget.

## Phase 9 — ARUT Studio
Desktop editor, viewport, hierarchy, inspectors, asset import/browser, world tools, profiler and packaging.
Exit gate: small world can be authored and packaged without editing engine internals.

## Phase 10 — Production hardening
Regression scenes, device matrix, thermal soak, memory/battery/load-time/frame pacing, crash recovery and reproducible releases.
Exit gate: release candidate passes declared Android device matrix and desktop smoke tests.
