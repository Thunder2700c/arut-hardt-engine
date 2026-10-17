# ARUT-HARDT Engine — Character & Ragdoll Lab

This engine-owned validation scene exercises the built-in C++ `ArutCharacter3D`,
the reusable ARUT mannequin visual, mobile/desktop movement, and the existing
ragdoll implementation. It is an engine template, not a playable game or
add-on.

## Primary humanoid mannequin

- Reusable scene: `arut_test_mannequin.tscn`.
- Source asset: `assets/mannequin/source/Humanoid Robot 01.glb` (the uploaded
  15.2 MB source model). The wrapper scene now instances this model directly,
  preserving the source's meshes, materials, skeleton and animation data.
- The source listing describes the model as a game-ready rigged and animated
  humanoid. The build workflow now checks the binary GLB header, skin/joint
  presence and animation list. That check is source-added but has **not yet
  been run** in a fresh build; it does not substitute for Godot importer or
  runtime validation.
- The wrapper keeps an initial uniform scale of `0.82` from the previous
  mannequin setup. This is a provisional alignment value, not a verified
  calibration. Confirm floor contact, forward axis, capsule alignment and
  animation track paths in the actual Godot 4.7 editor before calling it
  runtime-ready.
- Model credit and licensing record:
  `assets/mannequin/source/ATTRIBUTION-Humanoid-Robot-01.txt`.
  The source listing identifies the creator as Vavtrudner and the license as
  Creative Commons Attribution. Preserve that credit and verify the exact
  license/version in the original download record before public redistribution.

## Native C++ animation baseline

`ArutCharacter3D` discovers the mannequin's `AnimationPlayer` using the
Inspector-exposed node path and recursive fallback. Its native C++ controller
first tries the configured clip name and library-qualified suffix, then uses
semantic clip-name matching for Idle, Walk, Run/Sprint/Jog and Jump/Fall. If
a run clip is absent it falls back to walk; if a jump/fall clip is absent it
falls back to idle. These matching rules allow different model-specific
animation library prefixes without rewriting the character controller.

Animation transitions use `AnimationPlayer::play()` with a configurable
cross-fade. Gait clips loop, while the jump clip plays once and holds until
grounded locomotion resumes. Animation never drives the collision capsule or
overrides movement/collision truth. The actual imported clip names and
AnimationPlayer track paths remain unverified until the model passes Godot
import and runtime checks.

This is the conventional controller baseline only. It does not yet implement
AnimationTree state graphs, trajectory-aware motion matching, foot planting,
root-motion locomotion, traversal/warping, or ragdoll-to-animation recovery.
GASP-inspired motion matching remains future work and is not claimed to be
implemented.

## Existing movement and ragdoll behavior

- Native C++ movement, acceleration/friction, sprint, jump, air control, desktop
  keyboard input and mobile touch controls are preserved.
- The character capsule, third-person camera, crate test and existing
  procedural rigid-body ragdoll are retained.
- When ragdoll starts, the mannequin visual is hidden and the separate physics
  ragdoll activates. Bone-matched ragdoll and get-up/recovery are not yet
  implemented.

## Open and run

1. Build/open the repository's ARUT-HARDT Engine editor from the current
   `main` source, then import `templates/arut_character_lab/project.godot`.
2. Run `main.tscn`. The source GLB and attribution file are included in this
   repository; no manual mesh assembly or separate download is required.
3. Desktop: WASD or arrow keys move, Space/Enter jumps, Shift sprints; press R
   to activate ragdoll.
4. Phone: use the existing left-side movement drag, right-side look drag,
   bottom-right jump, mid-right sprint hold, and top-right ragdoll touch zones.

The scene uses built-in ARUT C++ classes from the engine source module. The
workflow now includes static model/asset checks, but compilation, Godot resource
import, the correct displayed mesh, clip track binding, animation playback,
visual scale and in-scene runtime behavior are not yet verified in a fresh
Godot 4.7 build.
