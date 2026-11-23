# ARUT-HARDT GASP-Style Animation System

**Status:** Active implementation — runtime, pose-search/playback foundations, Android pose-history feed, and initial cgltf-backed GLB skeletal mesh/clip importer committed; retargeting, material import, runtime integration and full GASP-style feature set remain incomplete  
**Language policy:** Native C++ only. No GDScript.  
**Targets:** Android ARM64 first; desktop support must remain portable.  
**Reference:** Unreal Engine Game Animation Sample Project (GASP), recreated as an original engine subsystem. This document describes desired behavior and architecture; it does not claim feature parity or completed implementation.

## 1. Goal

Build a reusable, data-driven character animation framework inspired by the techniques demonstrated by Epic's GASP. Do not copy Epic proprietary source code, project files, or restricted assets. Implement original C++ systems and use animation data whose licenses permit redistribution and use.

The system must work with the engine's replaceable metallic robot mannequin and with developer-imported skeletal characters. The mannequin is a reference asset, not a hard-coded player character.

## 2. Current repository baseline (verified before this plan)

- A fixed 20-bone humanoid skeleton and procedural mannequin mesh exist.
- GPU skinning uses a 20-matrix palette and four vertex influences.
- A procedural animator currently selects Idle, Walk, Run, StrafeLeft, StrafeRight, Backward, Jump, Fall and Land.
- The current animation poses are synthesized procedurally. There is no imported animation-clip playback or motion-matching database in the current implementation.
- Terrain-aware two-bone foot IK exists.
- The character controller is kinematic and the current test arena uses static AABBs. A general rigid-body physics backend and ragdoll system are not yet integrated.
- The current renderer updates the mannequin pose directly. The long-term design must move animation evaluation behind reusable runtime interfaces rather than hard-code a single mannequin animator into the Android renderer.

## 3. Target architecture

### 3.1 Asset and skeleton layer

- Import glTF 2.0 / GLB skeletal meshes, skins, joint hierarchies, inverse bind matrices, vertex weights, animation samplers/channels, PBR materials and textures. Initial importer now reads embedded GLB 2.0, one skin (up to the current 20-joint pose limit), triangle primitives, four joint/weight influences, inverse bind matrices, and translation/rotation/scale animation channels. PBR material/texture import, external .gltf URI resolution, matrix-authored joint transforms and compressed geometry remain future work.
- Preserve source skeleton hierarchy and stable bone names/indices.
- Validate malformed assets, unsupported interpolation modes, invalid weights, missing joints and out-of-range accessors.
- Support skeleton profiles and retarget maps so different character proportions can use compatible animation libraries.
- Keep the existing procedural mannequin as a fallback/test asset until imported assets are fully supported.

### 3.2 Animation runtime

- Animation clips with duration, sample rate, tracks, interpolation and root-motion metadata.
- Pose representation independent of the renderer.
- Animation graph primitives: clip player, blend, additive layer, state machine, transition, animation event and output pose.
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

1. **4A — Runtime separation and asset contracts (in progress):** renderer-independent pose/clip runtime, sampling, quaternion interpolation and blending are implemented with host tests. Remaining: move final pose ownership out of renderer-specific code, add graph/state-machine transitions and root-motion contracts, and validate on device.
2. **4B — Imported animation assets (initial importer implemented):** cgltf 1.15 is vendored under MIT; GLB memory import creates a skeletal hierarchy, inverse bind matrices, skinned triangle vertex/index data and animation clips with LINEAR, STEP and CUBICSPLINE sampling. A generated GLB host test is registered. Remaining: broaden beyond 20 joints, support multiple skins and matrix transforms, import materials/textures, test real licensed assets, and connect imported meshes to renderer upload.
3. **4C — Animation graph and locomotion:** clip sampling, blending, state machine, transitions, root motion, starts/stops/turns and trajectory generation.
4. **4D — Motion matching (foundation implemented):** pose-search schema, offline clip sampling/database generation, weighted feature scoring, tag filtering, continuity cost, pose-history ring buffer and trajectory prediction are implemented. Remaining: robust feature normalization, search profiling/LOD, imported clip database validation, and actual Android motion-matching playback once licensed clips are imported. Stateful clip selection and crossfade handoff are implemented; Android now records live pose history and predicts three future trajectory points.
5. **4E — IK and traversal:** foot locking, warping, look-at, hand IK, vault/mantle interaction framework.
6. **4F — Physics animation and ragdoll:** integrate the chosen physics backend; build reusable bone-body/joint profiles, powered ragdoll and recovery.
7. **4G — Android validation:** host tests, Android ARM64 CI, then S23 FE runtime tests with logs and visual evidence. Measure CPU time, memory, database size and frame-time impact.

## 5. Non-negotiable constraints

- C++ owns runtime animation, import validation, graph evaluation, motion matching, IK and physics integration. No GDScript.
- Android ARM64 must build in CI; do not assume desktop-only dependencies or tools.
- Do not bundle proprietary Epic animation assets or copy Epic source. Use original code and properly licensed animation data.
- The mannequin remains replaceable. No renderer code may assume every character uses the 20-bone reference rig.
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
