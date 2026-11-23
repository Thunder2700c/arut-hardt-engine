# Phase 4 GASP-style animation audit log

Date: 2026-11-23
Branch: main

## Repository audit

- Existing reference skeleton: 20-bone humanoid.
- Existing skinning: GPU skinning with four vertex influences and a 20-matrix palette.
- Existing animation: procedural state/pose generation for Idle, Walk, Run, StrafeLeft, StrafeRight, Backward, Jump, Fall and Land.
- Existing IK: terrain-aware analytic two-bone foot IK.
- Missing from current implementation: imported skeletal animation clips, glTF skeletal-animation importer, animation graph, pose history, predicted trajectory search, pose-search database, motion matching, retargeting pipeline, full-body IK, powered ragdoll.
- Existing Android renderer currently owns the mannequin animation update path; this must be decoupled into a reusable runtime system.
- Physics status: kinematic character controller/static AABB test arena only; no integrated general rigid-body backend or ragdoll.

## Reference study

Epic's public GASP documentation describes Motion Matching using pose search databases and trajectory/pose history, plus contextual database selection, orientation warping, leg IK, root offsets and traversal examples. The project is an animation sample rather than a single portable subsystem. ARUT-HARDT will implement original C++ equivalents and use appropriately licensed assets.

## Scope reminder

The forgotten adjacent feature is the reusable vehicle/car system already listed as Phase 5 in docs/ROADMAP.md. It remains after the Phase 4 character/animation gate.

## Current result

- Added docs/GASP_STYLE_ANIMATION_PLAN.md as the architecture and implementation baseline.
- No runtime feature is claimed complete by this planning commit.

## 4A implementation slice committed

- Added `engine/animation/include/arut/animation/runtime.hpp`: renderer-independent pose, transform, quaternion, keyframe clip, animation player, pose blend API.
- Added `engine/animation/src/runtime.cpp`: clip validation, keyframe sampling, quaternion spherical interpolation, playback-rate/looping behavior and pose blending.
- Added `tests/animation_runtime_tests.cpp`: clip sampling, playback wrap, blend interpolation and invalid-key ordering checks.
- Updated root `CMakeLists.txt`: builds `arut_animation`, links it into Android native target, and registers host test.
- Commits: `b077670` API; `33e2b58` implementation; `3bbdc1b` tests; `f9c701e` CMake integration.
- Verification status at log update: source committed to main; CI result not yet observed. Android device behavior not tested.
- Remaining 4A work: bridge existing Android character animator to this runtime, move pose ownership out of renderer-specific code, add deterministic transition graph/state-machine and root-motion contracts. This is a first implementation slice, not 4A completion.

## Motion matching foundation implementation

- Added renderer-independent pose-search schema with selected-joint feature weights and three trajectory horizons.
- Added offline database builder that samples source animation clips and stores pose + future root-displacement features.
- Added weighted nearest-pose search with required/forbidden tag filters and continuity cost.
- Added fixed-capacity 32-frame pose-history ring buffer.
- Added three-point trajectory prediction from world velocity, acceleration, facing and angular velocity.
- Added independent host tests for database generation/search/tag filtering, pose-history retention and trajectory prediction.
- CMake now compiles both new modules and registers their tests.
- Commits: `c3584f4` motion matching API; `2a90eda` database/search; `3076afc` tests; `db76ea2` CMake; `e00dec1` allocation exception correctness; `6acf905` source correction; `1557320` pose history API; `7631d33` history/trajectory implementation; `37b8c1c` tests; `67a5884` CMake integration.
- Verification: source committed. CI/test result not yet observed. Android runtime is not yet wired to motion matching. Do not call GASP parity complete.

## Licensing decision

- Recreate techniques from public documentation using original C++ implementations.
- Do not copy Unreal Engine source code, Blueprint graphs as serialized assets, or sample content into ARUT-HARDT.
- Only use sample assets/Examples under the exact license that applies to those specific files. Fab-distributed Content may have separate restrictions. Prefer independently licensed mocap/GLB assets and preserve attribution/license files.
- An Unreal-only distribution restriction does not automatically grant rights to bundle Epic-owned assets in an independent engine. A UE-only plugin can be a separate product, but its code/content still needs a compatible license and cannot be assumed reusable by ARUT-HARDT.

## Runtime integration milestone — 2026-11-23

- Added `AnimationPlayer::seek()` so database-selected clip times can be played without restarting from zero.
- Added `MotionMatchingPlayer`: consumes database search results, retains active clip playback, hands off to a new selected clip, and crossfades from the previous output pose.
- Corrected trajectory speed limiting: predicted horizontal displacement now integrates speed-clamped start/end velocities; the trajectory remains expressed in the current character-local frame for consistent search features.
- Extended motion-matching tests to cover clip selection, clip switching, and crossfade progression.
- Extended trajectory tests to verify that a 20 m/s input with a 5 m/s cap does not exceed the corresponding 0.8-second displacement bound.
- Connected Android's existing procedural + foot-IK final pose to the renderer-independent 32-frame pose-history buffer.
- Android now estimates acceleration and yaw rate from consecutive character states, predicts three future trajectory samples, and emits `ANIMATION_RUNTIME_HISTORY` diagnostics with history depth and predicted offsets.
- Changed files: `runtime.hpp`, `runtime.cpp`, `motion_matching.hpp`, `motion_matching.cpp`, `pose_history.cpp`, `tests/motion_matching_tests.cpp`, `tests/pose_history_trajectory_tests.cpp`, `engine/platform/android/src/android_main.cpp`, and progress docs.
- Key commits: `0d61f02` seek API; `45b4892` seek implementation; `0ef2843` motion-matching player API; `2829a82` playback/crossfade implementation; `4c57091` trajectory speed correction; `785ef72` motion-matching playback tests; `dca3197` trajectory clamp test; `dd2d505` Android live pose-history integration; `985c520` plan update.
- Verification status: changes are committed to `main`. The CI workflow is configured to run on every push to `main`, but this session has not obtained a workflow run/job result. The commit-status endpoint returned no status entries. Therefore compile/test success is **not verified yet**.
- Limitations: Android currently records procedural animation plus IK into pose history and predicts trajectory. It does not yet run motion matching in-game because no imported/retargeted animation clip library or built pose-search database is wired into the app. No GASP parity claim is made.
