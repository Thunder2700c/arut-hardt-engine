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

- `engine/core/include/arut/core/character_controller.hpp` provides a fixed-timestep kinematic controller with a metre-based capsule profile, horizontal acceleration/deceleration, jump impulse, gravity, walkable-slope checks, maximum step-up height and bounded catch-up steps.
- Ground contact is queried through a small callback so terrain and other world implementations can supply height and normal samples. The current implementation handles heightfield grounding and slope/step rejection; it does **not** yet collide the capsule against arbitrary walls, props or dynamic rigid bodies.
- `engine/rendering/include/arut/rendering/humanoid_animation.hpp` provides Idle, Walk, Run, StrafeLeft, StrafeRight, Backward, Jump, Fall and Land states. The current poses are procedural and blended smoothly; they are not imported motion-capture clips.
- Host tests cover acceleration/deceleration, jump/landing, steep-slope and step-up rejection, fixed-step catch-up limits, animation state selection and pose blending.

## Terrain-aware foot IK

- `engine/rendering/include/arut/rendering/humanoid_ik.hpp` implements an analytic two-bone leg solve using the upper-leg length and the lower-leg's metre-based rest offset. It returns hip/knee angles plus terrain pitch and roll, clamps targets to the reachable range, and blends rotations without quaternion sign flips.
- The Android Humanoid profile captures a world-space foot anchor at the start of each stance phase, samples the terrain height/normal at that fixed point, and solves the leg toward that anchor. During swing or airborne states, the animator retains its procedural gait pose. IK logs report each foot's planted state and reach clamping.
- The reference humanoid uses +Y up and -Z forward. Foot rest offsets, controller forward movement, mannequin geometry and third-person camera convention must stay consistent with this axis.
- Host tests verify the rest pose produces near-zero hip/knee correction, adjusted targets bend the leg, slope normals orient the foot, unreachable targets clamp, and invalid inputs are rejected.
- The current IK is a heightfield foot-placement system, not general-purpose full-body IK. It does not yet solve foot collision against arbitrary props, dynamic bodies, ledges or overhangs, and it has not yet been visually validated on the S23 FE.
