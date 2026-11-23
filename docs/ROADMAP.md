# ARUT-HARDT Engine Development Roadmap

> **Canonical roadmap:** This document follows the user's approved Phase 0–10 plan exactly. Phase numbers, names, scope and deliverables must not be renumbered, merged, silently redefined or replaced. Progress notes may be added without changing the canonical scope.

## Phase 0 — Architecture and infrastructure

- [ ] Technical specifications.
- [ ] C++ coding standards.
- [ ] CMake configuration.
- [ ] Dependency and license audit.
- [ ] CI/CD Android ARM64 build.
- [ ] Runtime module interfaces.

**Deliverable:** clean engine skeleton that compiles.

## Phase 1 — Native Android runtime

- [ ] Android lifecycle.
- [ ] JNI integration.
- [ ] Input and touch.
- [ ] Surface creation.
- [ ] Logging and crash handling.
- [ ] Vulkan initialization.

**Deliverable:** native ARUT application running on S23 FE.

## Phase 2 — Vulkan renderer

- [x] Swapchain.
- [x] GPU memory management.
- [x] Shader pipeline.
- [x] Textures and meshes.
- [x] PBR materials.
- [x] Camera and lighting.

**Deliverable:** textured 3D environment on Android.

## Phase 3 — Core world system

- [x] ECS (generational handles, packed sparse-set pools, typed component iteration, `CubeSpawnSystem` creates 5,000 cubes, and ECS-derived renderer extraction; host stress test and S23 FE/Xclipse 920 APK runtime test passed).
- [x] Scene graph (base hierarchy, cycle rejection, local/world transforms, subtree deletion and ECS entity bindings); the 5,000-cube runtime now synchronizes ECS local transforms into scene nodes and renderer extraction consumes scene-graph world transforms. New integration CI/device validation is pending.
- [x] Asset manager core cache, path validation, immutable asset handles, size limits, eviction, Android packaged-asset adapter and startup smoke test; CI passed. Runtime asset load is logged; broader streaming integration remains part of world integration.
- [x] Terrain heightfield mesh generation with finite-difference normals and indexed grids; active resident cell meshes are uploaded and drawn through Vulkan PBR in main and shadow passes. Camera movement updates the active cell window and refreshes GPU meshes when cells enter/leave. World-space height/normals are sampled consistently at cell boundaries; adjacent-edge tests passed (run `45086397194`). Android ARM64 CI passed for cell rendering and camera-driven refresh (runs `45086075726`, `45086182936`); S23 FE device validation is still pending.
- [x] Distance-based four-tier LOD selection with hysteresis; resident terrain cells select 32/16/8/4 grid resolutions and rebuild their mesh when the camera crosses LOD thresholds. Android ARM64 CI passed for this runtime path (run `45086320170`); device validation is pending.
- [x] Spatial cell mapping and bounded active-cell window with activation/eviction; `WorldCellStreamer` owns generated terrain payloads, invokes load/unload callbacks, refreshes resident cell LOD, and supplies active meshes to the renderer. Host residency/eviction/LOD tests passed (run `45086275112`). Current loading is synchronous procedural generation plus GPU buffer refresh; asynchronous disk/network asset streaming is not implemented.

**Deliverable:** navigable streamed environment.

## Phase 4 — Character and animation

- [ ] Robot mannequin.
- [ ] Locomotion.
- [ ] Animation blending.
- [ ] IK.
- [ ] Character controller.

**Deliverable:** playable third-person character.

## Phase 5 — Vehicle system

- [ ] Vehicle physics.
- [ ] Suspension.
- [ ] Steering.
- [ ] Transmission.
- [ ] Wheel animation.
- [ ] Vehicle interaction.

**Deliverable:** drivable car with realistic handling.

## Phase 6 — Open-world simulation

- [ ] Road networks.
- [ ] Traffic.
- [ ] Pedestrians.
- [ ] Navigation.
- [ ] Police AI.
- [ ] Background simulation.

**Deliverable:** living miniature city.

## Phase 7 — Environment and atmosphere

- [ ] Weather.
- [ ] Day/night.
- [ ] Water.
- [ ] Fog.
- [ ] Vegetation.
- [ ] Destruction.

**Deliverable:** dynamic environment.

## Phase 8 — Audio and gameplay framework

- [ ] 3D audio.
- [ ] Missions.
- [ ] Interactions.
- [ ] Combat.
- [ ] Save system.

**Deliverable:** complete gameplay prototype.

## Phase 9 — ARUT Studio

- [ ] World editor.
- [ ] Material tools.
- [ ] Animation tools.
- [ ] Asset pipeline.
- [ ] Profiling tools.

**Deliverable:** usable independent engine editor.

## Phase 10 — Optimization and production

- [ ] Memory optimization.
- [ ] GPU optimization.
- [ ] Thermal testing.
- [ ] Stability testing.
- [ ] Android and desktop packaging.

**Deliverable:** production-oriented engine release.

---

## Progress and phase gates (tracking only; does not alter the canonical scope)

Status is based on repository inspection, CI evidence and user-provided device logs. A source file or successful CI build is not by itself proof of a device-level exit gate.

| Canonical phase | Current assessment |
|---|---|
| Phase 0 — Architecture and infrastructure | Foundation established; audit each listed specification and governance item before marking the whole phase complete. |
| Phase 1 — Native Android runtime | Substantial implementation and S23 FE runtime evidence; lifecycle repetition, surface recreation and full smoke-test gate need explicit verification. |
| Phase 2 — Vulkan renderer | **Complete for the canonical Android deliverable on S23 FE/Xclipse 920.** All six scope items are implemented; CI passed C++/Rust/Android ARM64 builds, shader compilation and four-case Vulkan GPU-to-CPU PBR readback. User logs verify the 5,000-instance renderer, HDR/probe path, GPU culling, camera controls and repeated CPU/GPU visibility parity. Graceful CPU fallback is implemented. Second-GPU-family validation remains a non-blocking compatibility check before making a multi-vendor support claim. |
| Phase 3 — Core world system | In progress. ECS 5,000-cube spawn/extraction passed host tests and S23 FE/Xclipse 920 runtime validation. Scene graph-to-render-extraction integration is implemented and awaiting its newest CI/device validation. Asset cache/source and Android packaged-asset adapter passed CI. Terrain cell meshes now render in Vulkan main/shadow passes; camera-driven cell residency and four-tier terrain mesh LOD are implemented and passed Android ARM64 CI. Seam continuity tests passed. The latest integration still needs S23 FE validation, and world loading remains synchronous procedural generation rather than asynchronous asset streaming. Do not mark the Phase 3 deliverable complete until the complete build is tested on the S23 FE and its logs confirm terrain rendering, cell transitions, LOD changes and scene-graph extraction. |
| Phases 4–10 | Not started; remain locked behind their preceding phase exit gates. |

### Current execution order

1. Keep the canonical Phase 0–10 scope unchanged.
2. Record second-GPU-family validation as an outstanding compatibility check; do not misrepresent it as tested.
3. The 5,000-cube ECS gate is complete: `CubeSpawnSystem` creates the grid, ECS components produce render instances, host stress tests pass, and the S23 FE/Xclipse 920 APK log confirms runtime rendering and repeated GPU/CPU visibility parity.
4. Continue Phase 3 integration: finish CI validation of scene-graph extraction, add/verify asset-backed cell loading if required by the deliverable, then build and device-test the complete Phase 3 APK. Preserve the rule that this APK is not built until Phase 3 implementation and CI gates are complete. Preserve the cross-GPU limitation in release notes until another device is tested.
5. Do not start Phase 4 — Character and animation until Phase 3's navigable streamed environment is implemented and validated.

### Validation evidence discipline

- Record source changes and commits separately from CI build results.
- Record CI build/test results separately from physical-device results.
- Preserve runtime logs for each device test.
- Do not mark a phase complete until its stated deliverable and exit checks are demonstrated.
- For multi-GPU claims, identify each tested GPU/device; do not infer cross-device support from one phone.
