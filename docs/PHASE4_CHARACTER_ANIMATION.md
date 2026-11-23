# Phase 4 — Character and animation

**Status:** In progress. The first implementation slice is the engine-owned humanoid skeleton contract. This is not yet a rendered or playable character.

## Canonical scope

- Robot mannequin.
- Locomotion.
- Animation blending.
- Inverse kinematics.
- Character controller.

**Deliverable:** playable third-person character.

## Implementation slices

1. **4.1 — Rig foundation:** define a stable humanoid bone order, parent hierarchy, metre-based rest translations and per-joint visual radii. Keep the rig procedural and engine-owned so no licensed character asset is required.
2. **4.2 — Mannequin geometry and rendering:** generate articulated robot body geometry from the rig, add GPU skinning/bone palette support to the Vulkan path, and validate on Android.
3. **4.3 — Character movement:** fixed-step controller, capsule collision, gravity, grounded state, slope handling, jump and movement acceleration/deceleration.
4. **4.4 — Locomotion and blending:** in-place idle/walk/run/strafe/backward/jump/fall/land clips, state machine and smooth directional blend.
5. **4.5 — IK:** two-bone leg solving and terrain-aware foot placement/orientation.
6. **4.6 — Third-person integration:** follow/orbit camera, camera collision, Android touch controls, desktop keyboard/mouse controls, test arena and runtime diagnostics.

## Technical constraints

- Use metres, +Y up, and a documented forward-axis convention throughout.
- Character movement is authoritative; in-place animation follows controller velocity.
- Physics runs at a fixed timestep. Rendering interpolates presentation transforms.
- Evaluate and benchmark a single physics dependency for Android ARM64 before adopting it; Jolt remains a candidate, not an integrated dependency.
- Animation runtime/import dependencies must be license-audited and built for Android ARM64 before adoption.
- Do not claim skeletal rendering, IK, physics collision or playable controls until implemented and tested.

## First slice: humanoid skeleton contract

The humanoid_skeleton.hpp header defines a 20-joint humanoid hierarchy with validated parent ordering, finite rest translations and positive visual radii. The character_skeleton_tests.cpp test checks the built-in rig and rejects invalid parent references. The CMake test is host-only and does not imply Android rendering support.

## Exit criteria

- Host C++ tests and Android ARM64 native build pass.
- Mannequin is visible using the skeletal rendering path on the S23 FE.
- Character can walk, run, strafe, move backward and jump with collision and stable grounding.
- Animation transitions are smooth and responsive.
- Foot IK follows uneven terrain without visible foot sliding or penetration in the test scene.
- Third-person camera and Android controls are usable.
- Runtime logs expose controller, animation, blend, IK and camera state.
- Physical-device evidence is reviewed before Phase 4 is marked complete.
