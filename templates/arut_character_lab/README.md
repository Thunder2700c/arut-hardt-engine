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
- Rig audit from the committed source glTF: the scene root is `RootNode`,
  the armature node is `RobotArmature`, and `Bone` is the first/root joint
  in each 43-joint skin record. The common joint hierarchy contains torso,
  head, both arms/legs, feet, pole targets, and finger chains (thumb, index,
  middle and ring) on both hands. The articulated hand meshes contain
  `JOINTS_0` and `WEIGHTS_0`; the robot's hard-shell sections are separate
  authored mesh pieces attached to their joints, not engine-generated
  capsules/cubes or a continuous soft-human skin. The source contains two skin
  records pointing to the same 43-joint armature. Animation clips use that same
  armature, so no cross-rig retargeting is introduced for the baseline.
- Rest pose/scale notes: the source glTF's node transforms and bind data are
  preserved, rather than regenerated or re-named. The reusable scene currently
  scales the imported model by `0.82` to fit the existing approximately
  1.78 m character capsule. That scene scale is a starting alignment choice,
  **not visually verified calibration**; check feet-on-ground, head height,
  forward axis and rest pose in an actual ARUT-HARDT Engine import before
  calling the mannequin runtime-validated. Source-glTF joint/clip inspection
  does not by itself prove Godot's imported `Skeleton3D` and AnimationPlayer
  track paths have been runtime-tested.
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
