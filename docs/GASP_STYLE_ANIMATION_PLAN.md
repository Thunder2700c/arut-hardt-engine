# ARUT-HARDT GASP-Style Animation System

**Status:** Active implementation — renderer-independent animation runtime, pose-search/playback foundations, Android pose-history feed, GLB skeletal mesh/clip importer, name-based retargeting, 128-joint CPU poses, a crossfading animation state graph with event routing, additive pose composition, standalone planar root-motion extraction/application, and a generic CPU imported-skeleton skinning palette builder committed; material import, Vulkan GPU integration, root-motion graph nodes, combat graph and full GASP-style feature set remain incomplete  
**Language policy:** Native C++ only. No GDScript.  
**Targets:** Android ARM64 first; desktop support must remain portable.  
**Reference:** Unreal Engine Game Animation Sample Project (GASP), recreated as an original engine subsystem. This document describes desired behavior and architecture; it does not claim feature parity or completed implementation.

## 1. Goal

Build a reusable, data-driven character animation framework inspired by the techniques demonstrated by Epic's GASP. Do not copy Epic proprietary source code, project files, or restricted assets. Implement original C++ systems and use animation data whose licenses permit redistribution and use.

The system must work with the engine's replaceable metallic robot mannequin and with developer-imported skeletal characters. The mannequin is a reference asset, not a hard-coded player character.

## 2. Current repository baseline

- The reference mannequin remains a fixed 20-bone humanoid skeleton.
- The Android renderer now allocates a 128-matrix std140 skinning palette for both PBR and shadow vertex stages, with four vertex influences. The reference mannequin still has 20 joints; imported GLB meshes are not yet uploaded or drawn by the renderer.
- The procedural animator currently selects Idle, Walk, Run, StrafeLeft, StrafeRight, Backward, Jump, Fall and Land.
- Renderer-independent animation poses, clips, playback and blending now support up to 128 joints.
- Terrain-aware two-bone foot IK exists for the reference mannequin.
- The character controller is kinematic and the current test arena uses static AABBs. A general rigid-body physics backend and ragdoll system are not yet integrated.
- The imported GLB mesh/clip path is CPU-side only; it is not yet connected to Android renderer GPU buffers or a general scene-graph skinning palette.

## 3. Target architecture

### 3.1 Asset and skeleton layer

- Import glTF 2.0 / GLB skeletal meshes, skins, joint hierarchies, inverse bind matrices, vertex weights, animation samplers/channels, PBR materials and textures. Initial importer reads embedded GLB 2.0, one skin (up to 128 joints), triangle primitives, four joint/weight influences, inverse bind matrices, and translation/rotation/scale animation channels. PBR material/texture import, external .gltf URI resolution, matrix-authored joint transforms and compressed geometry remain future work.
- Preserve source skeleton hierarchy and stable bone names/indices.
- Validate malformed assets, unsupported interpolation modes, invalid weights, missing joints and out-of-range accessors.
- Support skeleton profiles and retarget maps so different character proportions can use compatible animation libraries. Initial name/alias matching and rest-pose-relative rotation, translation-scale and scale retargeting now support rigs up to 128 joints.
- Keep the existing procedural mannequin as a fallback/test asset until imported assets are fully supported.

### 3.2 Animation runtime

- Animation clips with duration, sample rate, tracks, interpolation and root-motion metadata.
- Pose representation independent of the renderer, currently capped at 128 joints.
- Animation graph primitives: clip player, blend, additive layer, state machine, transition, animation event and output pose. Initial C++ state machine now registers named clip states, starts a state, performs timed crossfades, supports immediate transitions and owns per-character playback state; event tracks now validate ordered timestamps and dispatch named events with optional payloads during forward/reverse playback, including loop-boundary crossings, and the state graph routes events from active/transition-target players; weighted local-space additive translation, rotation and scale layers are available relative to a reference pose; transition conditions and root-motion nodes remain future work. The combat layer will add attack chains, blocks, dodges, hit reactions, recoveries, root-motion windows and authored cancel/combo windows.
- Inertialized/smooth transitions to avoid visible pose pops.
- Root-motion extraction and controlled application, with an explicit choice between animation-driven and capsule-driven movement.
- Thread-safe evaluation with immutable asset data and per-character runtime state.

### 3.3 Motion matching

- Offline pose database builder that samples licensed animation clips.
- Configurable pose-search schema: selected bone positions/rotations/velocities, trajectory samples, weights and normalization.
- Pose history cache and predicted movement trajectory.
- Search query built from current pose, velocity, facing and future trajectory.
- Cost-based pose selection with continuity bias, database tags/context filters, search throttling and minimum-cost reselect thresholds.
- Runtime playback from the selected clip/frame, with controlled transition and continuing-pose behavior.
- Database LOD / reduced search density for mobile performance.
- Deterministic CPU implementation first; SIMD or parallel acceleration only after profiling.

### 3.4 Locomotion and traversal

- Walk/run/sprint, forward/backward/strafe, starts, stops, pivots, turns in place and direction changes.
- Jump start, airborne loop, light/heavy landing and recovery.
- Orientation warping, stride warping, steering and root-offset correction.
- Traversal framework for step-up, vault, mantle and ledge interactions, driven by gameplay queries and motion-warp targets.
- Aim offsets, upper-body additive layers, look-at and lean as optional graph features.

### 3.5 IK and physical animation

- Retain and generalize two-bone foot IK.
- Add foot locking, pelvis adjustment, hand IK, look-at and reusable full-body solver interfaces.
- Animation-to-physics blending and physical hit reactions.
- Ragdoll profiles mapping skeleton bones to rigid bodies, shapes and joint constraints.
- Powered ragdoll / active physical animation, impact response, fall protection and recovery-to-animation.
- Physics integration must be reusable by vehicles and other gameplay objects; do not create a mannequin-only physics hack.

### 3.6 Tooling and debugging

- Inspect skeleton hierarchy, clips, tracks, root motion, pose history and trajectory.
- Visualize sampled pose features, motion-matching query and selected candidate/cost.
- Toggle IK, warping, root offset, additive layers and ragdoll blend.
- Record animation state, selected database/clip/frame, blend weights, evaluation time and physics state in structured logs.
- Expose authoring data through engine/editor APIs; no runtime-only hard-coded animation tables.

## 4. Implementation sequence

1. **4A — Runtime separation and asset contracts (in progress):** renderer-independent pose/clip runtime, sampling, quaternion interpolation and blending are implemented with host tests. Root motion foundation: extract planar translation and yaw deltas between sampled poses, apply local displacement in actor world heading, and strip planar root translation for in-place playback; vertical root movement remains in the pose. This is a standalone utility and is not yet wired into the Android controller or graph update loop. Animation event tracks now dispatch footstep/impact-style markers across loop boundaries and reverse playback. Remaining: loop-boundary root-motion accumulation, root-motion blending during crossfades, graph transition conditions and direct additive-node integration, renderer integration and device validation. Weighted additive pose composition and state-graph event routing are now available as runtime primitives.
2. **4B — Imported animation assets (initial importer + retargeting foundation implemented):** cgltf 1.15 is vendored under MIT; GLB memory import creates a skeletal hierarchy, inverse bind matrices, skinned triangle vertex/index data and animation clips with LINEAR, STEP and CUBICSPLINE sampling. Name/alias-based retarget maps and rest-pose-relative retargeting are implemented. CPU animation and retargeting now accept 128-joint rigs. A generic CPU skinning-palette builder now composes imported TRS hierarchies with source inverse-bind matrices, handles parent joints appearing later in the skin array, and rejects cycles/invalid transforms. An import-to-render vertex adapter now converts imported positions/normals/UVs and four joint influences into the exact 80-byte vertex layout used by the mobile PBR pipeline, validates indices and finite values, and normalizes valid influence weights. AndroidRenderer now exposes `load_imported_character_glb(path, out_asset, model)` for packaged Android assets, `set_imported_character(asset, pose, model)` for already-imported assets, and `set_imported_character_pose(asset, pose, model)` for submitting updated poses without re-uploading geometry: the loader reads a bounded GLB asset, imports it, returns the full skeletal asset and animation clips to the caller, creates its rest pose, uploads the first skinned mesh into host-visible Vulkan vertex/index buffers, installs its 128-joint palette in the existing uniform descriptor, and submits the imported mesh in both shadow and forward PBR passes while suppressing the procedural mannequin draw. Replacements wait for device idle and release previous imported buffers. This first renderer bridge intentionally draws only the first imported mesh and starts at rest pose; imported animation clips are not yet advanced by the Android renderer. Remaining: support multiple skins and matrix-authored joints, draw every mesh primitive with per-material descriptors, import textures/material parameters, validate with real licensed assets, and connect AnimationPlayer/state-graph updates to the pose API on Android. The renderer currently uses its existing shared material descriptor, so imported materials and textures are not yet represented.
3. **4C — Animation graph, locomotion and combat:** clip sampling, blending, state machine, transitions, root motion, starts/stops/turns and trajectory generation; then layered martial-arts combat (attacks, blocks, dodges, hit reactions, recoveries and combo/cancel windows). This is where Shadow Fight 3-inspired movement and combat animation gets integrated after mannequin, importer, retargeting and renderer integration.
4. **4D — Motion matching (foundation implemented):** pose-search schema, offline clip sampling/database generation, weighted feature scoring, tag filtering, continuity cost, pose-history ring buffer and trajectory prediction are implemented. Remaining: robust feature normalization, search profiling/LOD, imported clip database validation, and actual Android motion-matching playback once licensed clips are imported. Stateful clip selection and crossfade handoff are implemented; Android now records live pose history and predicts three future trajectory points.
5. **4E — IK and traversal:** foot locking, warping, look-at, hand IK, vault/mantle interaction framework.
6. **4F — Physics animation and ragdoll:** integrate the chosen physics backend; build reusable bone-body/joint profiles, powered ragdoll and recovery.
7. **4G — Android validation:** host tests, Android ARM64 CI, then S23 FE runtime tests with logs and visual evidence. Measure CPU time, memory, database size and frame-time impact.

## 5. Non-negotiable constraints

- C++ owns runtime animation, import validation, graph evaluation, motion matching, IK and physics integration. No GDScript.
- Android ARM64 must build in CI; do not assume desktop-only dependencies or tools.
- Do not bundle proprietary Epic animation assets or copy Epic source. Use original code and properly licensed animation data.
- The mannequin remains replaceable. The legacy renderer's former 20-joint shader palette must not become the limit of the general animation runtime; shader and uniform palette capacity is now 128 joints.
- Do not call the system GASP-complete until imported clips, trajectory/pose search, runtime selection, transitions, retargeting and device validation are demonstrated.
- Do not mark Phase 4 complete until its canonical character deliverable is validated. Vehicle system remains Phase 5.

## 6. Exit criteria

- A developer can import a licensed GLB character with a different skeleton and play compatible clips.
- Animation evaluation is independent of Android renderer implementation.
- Locomotion responds to current and predicted movement without obvious foot sliding or abrupt pose pops.
- Motion matching selects poses from a real database using pose history and trajectory features.
- IK, traversal and physical animation are modular and can be enabled/disabled independently.
- Ragdoll uses physics bodies and constraints, and can recover to animation.
- Host tests and Android ARM64 CI pass; S23 FE runtime evidence confirms visual behavior and acceptable performance.
