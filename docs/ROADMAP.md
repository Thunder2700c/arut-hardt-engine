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
- [ ] Scene graph (base hierarchy, cycle rejection, local/world transforms and subtree deletion are host-tested; ECS entity binding and synchronization system are now implemented, awaiting CI validation and expanded edge-case tests).
- [ ] Asset manager.
- [ ] Terrain.
- [ ] LOD.
- [ ] World partitioning.

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
| Phase 3 — Core world system | In progress. ECS 5,000-cube spawn/extraction passed host tests and S23 FE/Xclipse 920 runtime validation. Scene graph base and ECS synchronization are implemented; synchronization CI validation is in progress. Asset manager, terrain, LOD and world partitioning remain outstanding; the navigable streamed environment deliverable is not yet met. |
| Phases 4–10 | Not started; remain locked behind their preceding phase exit gates. |

### Current execution order

1. Keep the canonical Phase 0–10 scope unchanged.
2. Record second-GPU-family validation as an outstanding compatibility check; do not misrepresent it as tested.
3. The 5,000-cube ECS gate is complete: `CubeSpawnSystem` creates the grid, ECS components produce render instances, host stress tests pass, and the S23 FE/Xclipse 920 APK log confirms runtime rendering and repeated GPU/CPU visibility parity.
4. Continue Phase 3 in order: finish ECS/scene-graph integration and tests, then asset manager, terrain, LOD and world partitioning. Preserve the cross-GPU limitation in release notes until another device is tested.
5. Do not start Phase 4 — Character and animation until Phase 3's navigable streamed environment is implemented and validated.

### Validation evidence discipline

- Record source changes and commits separately from CI build results.
- Record CI build/test results separately from physical-device results.
- Preserve runtime logs for each device test.
- Do not mark a phase complete until its stated deliverable and exit checks are demonstrated.
- For multi-GPU claims, identify each tested GPU/device; do not infer cross-device support from one phone.
