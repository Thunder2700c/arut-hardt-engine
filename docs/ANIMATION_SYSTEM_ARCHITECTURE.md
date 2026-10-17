# ARUT-HARDT Animation System Architecture

Last reviewed: 2026-10-17.

## Goal

Ship a preconfigured humanoid mannequin whose motion feels responsive and grounded, using the *type* of workflow demonstrated by Unreal Engine's Game Animation Sample Project (GASP). The ARUT implementation must be native C++ on Android ARM64, editable from ARUT's Inspector/editor, and compatible with the ARUT character controller, ragdoll and mobile input.

This document records the proposed architecture. It does not claim motion matching is integrated or that the mannequin is runtime-ready yet.

## What we mean by GASP-style

Epic describes GASP as a high-quality character traversal sample built around motion matching. Its system queries animation poses against character position/momentum/trajectory and uses contextual selection to choose suitable pose databases. The sample also demonstrates ledge traversal, vaulting and related actions.

The ARUT target feature set is:
- natural idle / start / stop / walk / jog / sprint and directional movement;
- smooth turns, strafing and direction changes with less visible foot sliding;
- trajectory-aware pose choice rather than only fixed animation-state switches;
- responsive jump, fall, landing and crouch behavior;
- authored action selection for attacks and other context-sensitive actions;
- traversal hooks for vaulting and ledge climb, followed by animation warping to adapt the movement to the environment;
- a debug view for current pose/database, query cost, selected clip, trajectory and foot contact;
- a conventional AnimationTree state machine as a deterministic fallback for devices, datasets or situations where motion matching is unsuitable.

This is inspired by GASP's documented behavior; it is not an attempt to copy GASP's Unreal Engine source code or redistribute its animation assets. GASP is not an engine-independent open-source implementation.

## Source shortlist

### Motion matching implementation candidate

Repository: https://github.com/GuilhermeGSousa/godot-motion-matching

The project is MIT-licensed and has a C++ implementation integrated with Godot's AnimationTree ecosystem. Its release notes describe a motion-matching animation node, a C++ feature-baking/editor workflow and KD-tree query acceleration. This makes it a much stronger starting point than implementing the entire algorithm from scratch.

**Do not drop its current source straight into the 4.7 module.** The project describes its implementation as Godot 4.4-era code and uses Godot/godot-cpp APIs. Audit the exact commit, engine-only API changes, transitive code and asset/data licenses; then port the required functionality into ARUT's first-class built-in module and editor tooling. The C++ algorithm and serialization should be separated from editor visualization where possible. Preserve the MIT notice and any third-party notices.

Official upstream release and feature references:
- https://github.com/GuilhermeGSousa/godot-motion-matching
- https://github.com/GuilhermeGSousa/godot-motion-matching/releases
- https://github.com/GuilhermeGSousa/godot-motion-matching/discussions/52

### Animation assets

Use CC0 animation data so the engine can lawfully ship a starter subset:
- Quaternius Universal Base Characters (CC0): https://quaternius.com/packs/universalbasecharacters.html
- Quaternius Universal Animation Library (CC0): https://quaternius.itch.io/universal-animation-library
- Quaternius Universal Animation Library 2 (CC0, 130+ clips): https://quaternius.itch.io/universal-animation-library-2
- Compact robot prototype with 14 clips (CC0): https://quaternius.itch.io/lowpoly-robot and https://opengameart.org/content/animated-lowpoly-robot

Use the compact robot pack for an early import/playback proof because it is around 7 MB and already animated. It is not a replacement for the larger humanoid dataset required to create a convincing motion-matching locomotion system.

For the real GASP-style mannequin, use a single consistent rig/retarget target and curate only the clips needed for the first mobile release. Begin with in-place movement clips driven by ARUT's CharacterBody3D motion. Use root-motion clips for vaults/climbs only after root-motion extraction, collision handling and animation warping are implemented. Do not assume separate character and animation pack skeletons match: inspect bone names, rest pose, root bone, scale, clip names and foot contacts before packaging.

## Runtime architecture

### Layer A — ARUTHumanoid / ARUTMannequin scene

The starter mannequin is a proper skinned humanoid with Skeleton3D, not generated capsules and rigid-body primitives. Give it a simple original synthetic/robot look: matte white/light-gray shell, contrasting joint collars/seams, blank faceplate and no optional skin-download dependency. Keep the visual skin replaceable so users can assign their own mesh with a compatible rig.

The scene contains the mesh/skeleton, animation player/library, AnimationTree, controller binding, character collision and camera attachment points. The low-level skeleton and clips remain normal Godot resources for import/edit compatibility; the engine-side logic and default configuration are C++-first.

### Layer B — C++ character movement

ARUTCharacter3D remains responsible for physics and input: movement velocity, grounded state, acceleration, sprint, jump request, aim/strafe mode, crouch request and grounded / falling / landing transitions. It exposes a compact typed interface consumed by the animation driver. Rendering an animation must never be the source of collision truth.

Character motion and visual root are separate responsibilities:
- in-place locomotion follows the physics controller;
- root-motion actions may request displacement from an animation, but the movement layer still resolves collisions and reports accepted movement;
- any root-motion scaling/warping must be bounded and must not teleport through geometry.

### Layer C — animation controller

First deliver a working conventional AnimationTree using the curated asset set. This is the baseline for validating imports and controls before adding motion matching. It should provide locomotion, start/stop, turn, strafe, jump, fall, land and ragdoll handoff, with Inspector mappings for action names and animation references.

For motion matching, build a pose/trajectory feature database offline or during an explicit editor bake operation—not every game frame. At runtime, query the database using current/future character trajectory, velocity and pose features, choose the best candidate using the spatial index, then blend/correct it. Query frequency, database size and feature weights must be tunable; use a conventional state/animation fallback for unsupported states.

Do not execute expensive feature-baking jobs inside normal gameplay. Keep thread ownership and resource lifetime explicit if database generation is parallelized.

### Layer D — traversal and action selection

Add this only after locomotion is stable:
1. detect and validate a traversable ledge/vault using physics queries;
2. select a suitable pre-authored action based on obstacle height/width and approach trajectory;
3. align/warp the animation to the target while clamping displacement and checking collision;
4. let the C++ movement layer own collision response and completion/cancel behavior;
5. recover into motion matching with a compatible pose/trajectory.

Action/pose database selection should be contextual (e.g. locomotion, combat, traversal), similar in intent to GASP's chooser workflow. Use explicit tags and predictable fallbacks before adding more advanced chooser authoring.

### Layer E — ragdoll transition and recovery

Reuse the existing ARUT ragdoll as a separate physics representation. Add blended handoff only after its joint setup is correct. Recovery should find a safe grounded pose, choose get-up animation/orientation, blend back to the animation skeleton, then return control to ARUTCharacter3D. Never teleport the animation mesh to a recovered position that has not passed a collision query.

## Android performance requirements

Motion matching is not automatically faster than an AnimationTree. Query time, sample count, memory and blending cost can be significant on mobile. Start with a small database and a bounded update rate, then benchmark on the target ARM64 device.

Add explicit diagnostics for:
- motion-matching query time and queries per second;
- database sample count and memory estimate;
- current selected animation / database / pose;
- animation blend and trajectory errors where measurable;
- main-thread and physics-frame time, so animation cost can be distinguished from physics/rendering cost.

Use one test scene with known movement cases (walk-stop, sprint-turn, strafe, jump-land, vault, fall-recover). Automated checks should validate clip availability, skeleton references, asset license manifest and database generation before the full Android editor build.

## Staged implementation

1. Import and play the small CC0 animated robot; verify skeleton and all packaged clips. This is a smoke test only.
2. Package the full humanoid rig and curated CC0 in-place locomotion subset. Replace the procedural primitive dummy in the starter template with this skinned scene; keep an intentional primitive fallback for missing asset/resource errors.
3. Add conventional AnimationTree control driven by ARUTCharacter3D's physics state and mobile input.
4. Audit and port the MIT motion-matching candidate to Godot 4.7 as a built-in C++ module. Add asset baking, database serialization, bounded runtime queries and debug tools.
5. Add trajectory correction and foot-contact/planting improvements, then contextual action databases and traversal/animation warping.
6. Add animation-to-ragdoll and ragdoll-to-animation recovery.
7. Run Android ARM64 CI; then perform one runtime validation session with locomotion, turns, jump/land and query-cost logging before calling the system functional.

## Integration and license rules

- All engine runtime behavior belongs in the native module/source where practical. Do not ship the motion matcher as an optional GDExtension if the promise is a built-in engine feature.
- Keep all original notices for MIT code and document licenses for data, models and clips.
- GASP is a feature reference only. Do not port Epic's engine-dependent implementation or ship GASP assets in ARUT without explicit license/permission confirmation.
- Separate proposed, source-integrated, compiled and runtime-verified status in docs.
