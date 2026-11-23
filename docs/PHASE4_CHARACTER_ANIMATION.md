# Phase 4 — Character and animation

**Status:** In progress. The rig, procedural bind-pose mesh generator and Vulkan static-mesh submission path are implemented. Skeletal deformation and animation are still pending; physical-device visibility has not yet been verified.

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

## Technical constraints

- Use metres, +Y up, and a documented forward-axis convention throughout.
- Character movement is authoritative; in-place animation follows controller velocity.
- Physics runs at a fixed timestep. Rendering interpolates presentation transforms.
- Evaluate and benchmark a single physics dependency for Android ARM64 before adopting it; Jolt remains a candidate, not an integrated dependency.
- Animation runtime/import dependencies must be license-audited and built for Android ARM64 before adoption.
- Do not claim skeletal rendering, IK, physics collision or playable controls until implemented and tested.

## Implemented: rig and procedural bind-pose mesh foundation

- `engine/core/include/arut/core/humanoid_skeleton.hpp` defines a 20-joint humanoid hierarchy with validated parent ordering, finite rest translations and positive visual radii.
- `engine/rendering/include/arut/rendering/robot_mannequin.hpp` builds a deterministic triangle mesh from bone segments and joint spheres, with outward-facing triangle winding for back-face culling.\n- `engine/rendering/src/android_renderer.cpp` uploads the mesh and its instance transform to Vulkan buffers, submits it in the main and shadow passes, and emits a `HUMANOID_RENDERER` readiness log. This is static bind-pose drawing only; no bone palette or GPU skinning exists yet.
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
