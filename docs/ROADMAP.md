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
- [ ] Confirm Android ARM64 native compilation (source is present; no APK build has been requested).
- [ ] Install and run on the Galaxy S23 FE; verify the triangle, depth attachment and rotation/resume (deferred until testing stage).
- [x] Add packaged SPIR-V loading, shader modules and a first-triangle graphics pipeline.
- [x] Add a host-visible vertex buffer and a depth image/view/render-pass attachment with depth testing enabled.
- [x] Add a standard PBR-ready vertex layout, procedural unit-cube mesh data and Vulkan-compatible perspective matrix helper.
- [x] Add indexed unit-cube mesh buffers, camera/view transforms, frame/material uniforms and an active mobile PBR draw path.
- [ ] Add real texture asset decoding/upload, mipmaps and texture streaming.
- [ ] Compile the Android-native renderer and validate on device (deferred until the agreed testing stage).
Exit gate: Android renders a textured 3D scene with validation and frame/GPU timing.

## Phase 3 — Rendering and materials (implementation in progress)
- [x] Define a PBR material data contract (base colour, metallic, roughness, emissive, normal scale, occlusion and alpha cutoff).
- [x] Add finite-value handling and safe material parameter normalization.
- [x] Define Low/Medium/High/Ultra mobile quality presets with render scale, shadow budget and light limits.
- [x] Define a renderer profiling snapshot contract.
- [x] Add a CPU reference implementation for Schlick Fresnel, GGX distribution, Smith geometry, Cook-Torrance direct lighting and ACES tone mapping.
- [x] Add mobile forward PBR vertex/fragment shader source and host-CI GLSL compilation (not connected to a Vulkan PBR pipeline).
- [x] Add host tests for BRDF limits, front/back lighting and tone mapping.
- [ ] Integrate shader compilation, descriptor layouts, vertex formats and graphics pipeline into Vulkan.
- [ ] Validate energy conservation and compare GPU output against the CPU reference.
- [ ] Add mobile shadow maps and cascade quality tiers.
- [ ] Add GPU instancing and CPU/GPU visibility culling.
- [ ] Add environment lighting / image-based lighting (ACES tone-map source exists; runtime integration remains pending).
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
