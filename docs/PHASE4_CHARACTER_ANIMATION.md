# Phase 4 — Character and animation

**Status:** In progress. The rig, weighted mannequin mesh, CPU pose/inverse-bind palette builder, Vulkan GPU skinning path, isolated Android Test Laboratory profiles, fixed-step terrain controller, procedural locomotion blending and terrain-aware two-bone foot IK are implemented. Physical-device movement, animation, foot contact and visual quality remain unverified.

## Canonical scope

- Robot mannequin.
- Locomotion.
- Animation blending.
- Inverse kinematics.
- Character controller.

**Deliverable:** playable third-person character.

## Implementation slices

1. **4.1 — Rig foundation:** define a stable humanoid bone order, parent hierarchy, metre-based rest translations and per-joint visual radii. Keep the rig procedural and engine-owned so no licensed character asset is required.
2. **4.2 — Mannequin rendering:** connect the generated bind-pose mesh to the Vulkan draw path, add GPU skinning/bone palette support, and validate visibility on Android.
3. **4.3 — Character movement:** fixed-step controller, capsule collision, gravity, grounded state, slope handling, jump and movement acceleration/deceleration.
4. **4.4 — Locomotion and blending:** in-place idle/walk/run/strafe/backward/jump/fall/land clips, state machine and smooth directional blend.
5. **4.5 — IK:** two-bone leg solving and terrain-aware foot placement/orientation.
6. **4.6 — Third-person integration:** follow/orbit camera, camera collision, Android touch controls, desktop keyboard/mouse controls, test arena and runtime diagnostics.

## Android Test Laboratory profiles

The Android launcher now presents three explicit runtime choices:

- `ecs_stress` — constructs and renders the 5,000-entity ECS stress scene without the humanoid mannequin.
- `humanoid` — skips ECS stress-entity creation and renders the mannequin with the streamed terrain test arena.
- `combined` — runs both systems together as a separate integration profile.

The Android profile picker is implemented by `app/src/main/java/com/arut/engine/TestLabActivity.java`. Stable profile IDs and inclusion rules live in `engine/core/include/arut/core/test_lab_profile.hpp`, with host tests in `tests/test_lab_profile_tests.cpp`. The native log records the selected profile and the renderer's stress/mannequin switches. Selecting the humanoid profile must not be mistaken for skeletal animation: it currently renders a static bind pose.

## Technical constraints

- Use metres, +Y up, and a documented forward-axis convention throughout.
- Character movement is authoritative; in-place animation follows controller velocity.
- Physics runs at a fixed timestep. Rendering interpolates presentation transforms.
- Evaluate and benchmark a single physics dependency for Android ARM64 before adopting it; Jolt remains a candidate, not an integrated dependency.
- Animation runtime/import dependencies must be license-audited and built for Android ARM64 before adoption.
- Do not claim skeletal rendering, IK, physics collision or playable controls until implemented and tested.

## Implemented: rig and procedural bind-pose mesh foundation

- `engine/core/include/arut/core/humanoid_skeleton.hpp` defines a 20-joint humanoid hierarchy with validated parent ordering, finite rest translations and positive visual radii.
- `engine/rendering/include/arut/rendering/robot_mannequin.hpp` builds a deterministic triangle mesh from bone segments and joint spheres, with outward-facing triangle winding for back-face culling.\n- `engine/rendering/src/android_renderer.cpp` uploads the mesh and its instance transform to Vulkan buffers, submits it in the main and shadow passes, and emits a `HUMANOID_RENDERER` readiness log.
- `MeshVertex` carries four joint indices and four normalized weights. The mannequin generator blends influence from each bone's parent to child along segmented limb geometry and assigns joint spheres to their owning joint.
- `engine/rendering/include/arut/rendering/humanoid_skinning.hpp` builds hierarchical local/global pose transforms and the inverse-bind palette. Host tests verify that the rest pose produces identity skin matrices, a chest rotation moves descendants, and invalid pose values are rejected.
- The mobile PBR and shadow vertex shaders read the same 20-matrix UBO and apply four-influence linear blend skinning. The renderer updates a small procedural idle-breathing/arm-sway pose each frame; it is a skinning smoke animation, not the locomotion clip/blend system.
- The Android CI job compiles the exact six SPIR-V shader assets into `app/src/main/assets/shaders/` and validates that each output is non-empty, while still stopping short of assembling an APK. The explicit Android APK workflow runs the same shader compilation before Gradle packaging. The completed validation run also passed all 11 C++ tests, Rust checks, Android ARM64 native compilation and Java Activity compilation.
- `tests/character_skeleton_tests.cpp` checks the rig contract, mesh non-emptiness, triangle/index validity and finite vertex positions/normals.
- The host CMake test links `arut_core` and `arut_rendering`. Host tests prove procedural geometry and winding. Android CI compiles the Vulkan path; only an on-device log and visual check can prove that the mannequin is actually visible.

## Exit criteria

- Host C++ tests and Android ARM64 native build pass.
- Mannequin is visible using the skeletal rendering path on the S23 FE.
- Character can walk, run, strafe, move backward and jump with collision and stable grounding.
- Animation transitions are smooth and responsive.
- Foot IK follows uneven terrain without visible foot sliding or penetration in the test scene.
- Third-person camera and Android controls are usable.
- Runtime logs expose controller, animation, blend, IK and camera state.
- Physical-device evidence is reviewed before Phase 4 is marked complete.

## Locomotion foundation

- `engine/core/include/arut/core/character_controller.hpp` provides a fixed-timestep kinematic controller with a metre-based capsule profile, horizontal acceleration/deceleration, jump buffering, gravity, walkable-slope checks, maximum step-up height and bounded catch-up steps.
- Ground contact is queried through a terrain callback. An optional capsule-overlap callback can supply obstacle tests; `character_collision.hpp` provides vertical-capsule/AABB overlap checks and the controller performs horizontal axis-separated sliding when a proposed move is blocked. Host tests exercise side-wall blocking, diagonal wall sliding, and vertical ceiling clearance.
- The Android Humanoid and Combined scenes now include a small AABB test arena with matching rendered cube walls/blocks; the same AABBs are supplied to the controller collision query. This is a static test-arena collision path, not yet dynamic rigid-body physics or a general-purpose scene collision world.
- `engine/rendering/include/arut/rendering/humanoid_animation.hpp` provides Idle, Walk, Run, StrafeLeft, StrafeRight, Backward, Jump, Fall and Land states. The current poses are procedural and blended smoothly; they are not imported motion-capture clips.
- `engine/core/include/arut/core/character_input.hpp` merges analog axes with keyboard states and buffers jump edges in a platform-neutral router. The Android backend maps WASD/arrows, Space/gamepad A, right-mouse drag, and gamepad left/right sticks. This makes Android HID input testable, but the repo still has no desktop window/event backend, so a desktop runtime is not claimed.
- Host tests cover acceleration/deceleration, jump/landing, steep-slope and step-up rejection, fixed-step catch-up limits, animation state selection and pose blending.

## Terrain-aware foot IK

- `engine/rendering/include/arut/rendering/humanoid_ik.hpp` implements an analytic two-bone leg solve using the upper-leg length and the lower-leg's metre-based rest offset. It returns hip/knee angles plus terrain pitch and roll, clamps targets to the reachable range, and blends rotations without quaternion sign flips.
- The Android Humanoid profile captures a world-space foot anchor at the start of each stance phase, samples the terrain height/normal at that fixed point, and solves the leg toward that anchor. During swing or airborne states, the animator retains its procedural gait pose. IK logs report each foot's planted state and reach clamping.
- `engine/core/include/arut/core/third_person_camera.hpp` sweeps the follow-camera segment against the terrain heightfield and static AABB obstacles. It shortens the camera distance before passing below terrain or entering a test-arena wall/block. It does not yet detect arbitrary non-AABB geometry, moving obstacles or dynamic rigid bodies.
- The reference humanoid uses +Y up and -Z forward. Foot rest offsets, controller forward movement, mannequin geometry and third-person camera convention must stay consistent with this axis.
- Host tests verify the rest pose produces near-zero hip/knee correction, adjusted targets bend the leg, slope normals orient the foot, unreachable targets clamp, and invalid inputs are rejected.
- The current IK is a heightfield foot-placement system, not general-purpose full-body IK. It does not yet solve foot collision against arbitrary props, dynamic bodies, ledges or overhangs, and it has not yet been visually validated on the S23 FE.


## Default packaged character asset

The Android humanoid profile now packages a default rigged GLB as `Characters/hero.glb` before Gradle assembles the APK. The pinned source is KinemIQ's 35-animation CC0 release (66-bone humanoid skeleton plus mannequin); the asset-fetch script verifies the release archive SHA-256 and validates the extracted GLB v2 header/length. The runtime already probes this path and installs the imported character through the Vulkan skinned-mesh path. When installation succeeds, the imported character draw suppresses the procedural mannequin draw. User-provided GLB skins remain optional replacements.

The binary is fetched at build time rather than committed as a Git binary. This keeps the source repository small while ensuring the APK contains the model. The source pack's published CC0 terms permit redistribution and modification. The importer now gathers every mesh node that uses the same ordered joint rig and identical inverse-bind matrices, and the renderer combines those compatible submeshes into one skinned draw. Incompatible skeleton/bind layouts are rejected rather than silently omitted. Remaining limits include no imported PBR texture/material binding, one compatible skeleton per SkeletalAsset, and animation playback only for clips supported by the importer and runtime. The Android build log must show the asset-fetch step succeeding; an APK build alone does not prove visual correctness on a physical device.


## Phase 4C — procedural fallback animation library

The imported character may ship with fewer animation clips than the controller needs. Android startup now preserves recognized authored GLB clips, then fills missing states with an original keyframed fallback library generated against the imported rig's rest pose and joint names.

The current fallback library authors 15 states: idle, walk, run, left/right strafe, backward, jump, fall, land, light attack, heavy attack, block, dodge, hit reaction and recovery. It uses local joint rotations for pelvis/spine/chest/neck, upper arms/forearms and upper/lower legs where corresponding names are found. Unmatched joints remain at rest pose. It does not fabricate a retargeted animation asset or require a second GLB.

Combat clips carry combo/cancel window events. The controller buffers attack input for 220 ms, permits attacks to chain only during authored combo windows, permits defensive interrupts only during authored cancel windows, and restarts the same attack clip for repeat chains. Hits retain highest interruption priority.

This is a functional original baseline, not a claim of authored AAA motion-capture quality. The generated poses need visual review on the 66-joint default rig; missing bone-name conventions may reduce motion on some skeletons. The host test covers clip generation, joint changes, action transitions and combo-window restart. Device validation must check silhouette, joint deformation, foot sliding, timing and touch/gamepad response.

## Follow-up engineering pass — ground stability, device profiling and Phase 4C

- Ground support now has a dedicated grounded branch: while the capsule remains within the configured step envelope of a valid walkable support sample, the controller snaps its feet origin to that support and does not integrate gravity. Falling still begins when support is lost, the surface is invalid/unwalkable, the character jumps, or the support-height change exceeds the step envelope.
- Added a 10-second fixed-step grounded-idle regression (600 steps at 60 Hz) to detect gravity-induced vertical drift. This is a host regression, not proof of physical-device contact quality.
- Android diagnostics now emit CHARACTER_GROUND with feet height, sampled terrain height, clearance, normal and walkability alongside controller velocity/grounded state.
- Performance windows are now 30 seconds instead of 5 seconds. Each window reports FPS, average/max frame interval, frame count, renderer CPU/GPU pass timings, draw/triangle counts and tracked GPU memory. Thermal sampling reads exposed /sys/class/thermal/thermal_zone*/temp and type entries and logs the hottest readable zone; some vendors restrict or omit these nodes, in which case thermal availability is explicitly false.
- Vulkan device selection already logs GPU name, vendor ID, device ID, API version, driver version and device type. Capture separate 30-second Humanoid-profile runs on the Mali-G615 MC2 and Xclipse 920, with the same resolution, quality tier, camera position and movement script. Do not compare a 5-second snapshot from one device with a 30-second run from another.
- Phase 4C remains in progress. The built-in 20-joint procedural animator has locomotion and combat states with blended poses; the renderer-independent imported-rig controller has 15 fallback states, combo/cancel events and buffered chaining. The imported-rig controller is not yet the default procedural character's animation path, and neither system has been visually validated for combat quality on both GPUs.
- Remaining 4C work: make transition priorities and input buffering consistent across the built-in and imported-rig paths; verify attack-to-block/dodge/hit-reaction interruption, repeated combo timing, landing recovery and locomotion re-entry; add pose/foot-contact visual review before declaring the slice complete.
