# ARUT-HARDT Engine — Character & Ragdoll Lab

This is the engine-owned character workflow test scene for the built-in C++
`ArutCharacter3D`, the reusable ARUT Test Mannequin resource, and the existing
ragdoll implementation. The mannequin is now the primary visual in this lab;
the previous primitive dummy remains available only as the character node's
optional fallback when `character_create_default_dummy` is enabled.

## Default rigged robot mannequin

- Reusable scene: `arut_test_mannequin.tscn`.
- Source model: `assets/mannequin/arut_test_mannequin.gltf`, self-contained
  with embedded geometry data, PBR material values, a skeletal rig and animation
  data. There are no companion `.bin` files or external texture downloads.
- Robot material treatment: white shell, light titanium joints, dark graphite
  faceplate. Palette overrides are stored in the model itself.
- Rig audit: two glTF skin records share a 43-joint rig; named joints include
  torso, head, both arms/legs, feet and finger chains (thumb, index, middle,
  ring) on both hands. The articulated hand primitives contain `JOINTS_0` and
  `WEIGHTS_0` attributes. The rigid shell pieces are modeled mesh sections
  attached to skeletal joints, rather than engine-created capsules/cubes.
- Included animations (14): `Robot_Idle`, `Robot_Walking`,
  `Robot_Running`, `Robot_Jump`, `Robot_WalkJump`, `Robot_Standing`,
  `Robot_Sitting`, `Robot_Dance`, `Robot_Death`, `Robot_Punch`,
  `Robot_ThumbsUp`, `Robot_Wave`, `Robot_Yes`, and `Robot_No`.
  Imported clip names can retain the `RobotArmature|` prefix; the C++ driver
  resolves exact names first and falls back to a matching clip suffix.
- Provenance/license: `assets/mannequin/LICENSE-Quaternius-Animated-Robot.txt`.
  The original Quaternius pack is explicitly distributed under CC0 1.0.

## Native C++ conventional animation baseline

`ArutCharacter3D` finds the mannequin's `AnimationPlayer` through the
Inspector-exposed path (with a recursive fallback). Its native C++ controller
selects Idle, Walking, Running or Jump from the existing movement controller's
grounded state, horizontal velocity and sprint flag. Transitions use
`AnimationPlayer::play()` with a configurable cross-fade; gait clips loop,
while Jump plays once and holds until the next grounded state. Animation does
not drive the capsule, collision, or movement velocity.

The pack contains no separate fall or landing clip. This baseline does not
implement an AnimationTree state graph, trajectory-aware motion matching, foot
planting, root-motion locomotion, traversal, or ragdoll-to-animation recovery.
GASP-inspired motion matching remains future work and is **not** claimed to be
implemented here.

## Existing movement and ragdoll behavior

- Native C++ movement, acceleration/friction, sprint, jump, air control,
  desktop keyboard input and mobile touch controls are preserved.
- The character capsule, third-person camera, crate test and existing
  procedural rigid-body ragdoll are retained.
- When ragdoll starts, the mannequin visual is hidden and the existing ragdoll
  activates. Ragdoll bodies remain a separate physics representation; bone
  matching and get-up/recovery are not implemented in this task.

## Open and run

1. Build/open the repository's ARUT-HARDT Engine editor from the current
   `main` source, then import `templates/arut_character_lab/project.godot`.
2. Run `main.tscn`. The mannequin resources are committed in this repository;
   no GDScript setup or separate asset download is required.
3. Desktop: WASD or arrow keys move, Space/Enter jumps, Shift sprints; press R
   to activate ragdoll.
4. Phone: use the existing left-side movement drag, right-side look drag,
   bottom-right jump, mid-right sprint hold, and top-right ragdoll touch zones.

The scene uses built-in ARUT C++ classes from the engine source module. It is
an engine validation template, not a game or a separate add-on. Compilation,
resource import and runtime behavior still require confirmation in an actual
ARUT-HARDT Engine editor build; no such success is claimed in this commit.
