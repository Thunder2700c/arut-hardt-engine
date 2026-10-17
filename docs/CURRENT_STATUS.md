# Current Status — Resume Here First

Last updated: 2026-10-17.

## Direction reset
The engine is the product right now; PEAK/gameplay testing is not the priority. The user wants C++-first engine features and an original editor UX. Do not ask for a phone test after each feature. Aim to build a private-alpha package for five people after several coherent features exist.

## Naming
Umbrella: **ARUT-HARDT Engine**.
- **ARUT Mobile** — Android/mobile-first edition.
- **HARDT Studio** — desktop editor edition.
One shared core, API and project format.

## Permanent repository
Repository: https://github.com/Thunder2700c/arut-hardt-engine
Default branch: main.
Working branch for this direction reset: `engine-first-cpp-ux`.
The engine-only foundation has its own history. Do not continue work in the old PEAK staging branch and do not merge it into PEAK main.

## What exists in this repo
- Initial C++ GDExtension class `ArutRuntime` under `native/arut_native/`.
- Isolated extension smoke-test project under `tests/native_smoke/`.
- Shader/material prototype and demo under `renderer/demo/`.
- Manual CI workflows for Android ARM64 editor/native extension and Linux x86_64 editor.
- Pinned intended upstreams: Godot `4.7-stable`; godot-cpp commit `2bb1acf6f8ae1ce96ac7b9ba3a10b499c59c49ec`.
- `DEVELOPMENT_PROTOCOL.md` is updated to engine-first, C++-first development.
- `ROADMAP.md` is restructured around built-in native components, editor redesign and backend evaluation.
- `docs/PHYSICS_ARCHITECTURE.md` now compares backend candidates without declaring Jolt the winner.
- `docs/EDITOR_UX_ARCHITECTURE.md` describes a genuinely branded editor redesign.
- `docs/ENGINE_COMPONENT_CATALOG.md` specifies first-class nodes, starting with Car3D.
- `engine/modules/arut/` contains built-in native C++ source for `ArutCar3D`, `ArutCharacter3D`, `ArutRagdollDummy3D` and `ArutRenderProfile3D`. Both editor workflows copy this module into the pinned Godot source before SCons runs. This is source-level implementation only so far.
- `engine/patches/0001-arut-editor-identity.patch` defines the initial ARUT-HARDT theme preset and window/project-manager identity. `0002-arut-about-and-project-manager.patch` brands the About dialog/welcome copy while preserving Godot attribution. The build overlays original mark, About logo, title-bar wordmark and custom node icons into the editor source.

## Build truth
- Successful Android ARM64 workflow run: [39024274125](https://github.com/Thunder2700c/arut-hardt-engine/actions/runs/39024274125), source commit `158ea1efb2c897d52adb8f676376f59bbfc7aec5` (2026-10-17).
- That run passed the ARUT patch checks, icon-generation step as it existed at that commit, preflight, five ARUT C++ module object builds, full Android editor compile, Gradle packaging and artifact upload. The native GDExtension build and packaging jobs also passed.
- This confirms the source at that commit can compile/package in CI. It does not confirm that the built-in classes behave correctly in a scene, are discoverable in the node picker, or that the custom launcher icon rendered correctly on a device.
- The launcher-icon pipeline fix and the runtime-branding patch added in the next commit are new changes; they require a new manual Android workflow run before claiming they work in an APK.
- The renderer shader is still a prototype. The C++ Car3D, Character3D, RagdollDummy3D and RenderProfile3D systems need runtime/behavior validation.
- Research decisions are recorded in `docs/OPEN_SOURCE_STACK.md`.

## Physics findings
- The starter ragdoll uses the active Godot `PhysicsServer3D` through standard rigid bodies and joints; this is not a separate ARUT physics solver.
- Current project baseline is Godot 4.7 with built-in Godot Physics/Jolt options; Jolt is not a permanent decision.
- Native `ArutCar3D` source provides Inspector properties, editable compact/sedan/off-road presets, internally managed wheel raycasts, spring/damper force, simple steering, drive/braking inputs and contact/speed telemetry. Native `ArutCharacter3D` now generates its own humanoid dummy, capsule collider and third-person camera; provides WASD/arrow movement, sprint, jump and phone touch controls; and hands off to a procedural articulated ragdoll on R/touch. `ArutRagdollDummy3D` builds connected rigid bodies/cone-twist joints. `ArutRenderProfile3D` applies Mobile/Balanced/High viewport profiles with render scaling, AA, occlusion culling, mesh LOD threshold, shadow atlas, texture mip bias and debanding. None is compiled or runtime-verified yet.
- The ARUT-HARDT theme/identity patch and custom SVG brand/node icons are written, but source application/build is not yet verified.
- A community Godot 4.x PhysX 5 backend exists as work-in-progress and needs a source/commit, license/notice, Android ARM64 and build-path audit before adoption.
- Bullet and ReactPhysics3D remain comparison candidates; neither is a drop-in Godot 4.7 backend.
- Next physics milestone is evaluating the existing PhysX backend and designing native, backend-aware ARUT components—not blindly embedding another solver.

## Next actions
1. Add the neutral white ARUT Test Mannequin with a real humanoid skeleton and default locomotion clips; curate CC0 animation assets.
2. Run the new Android branding/icon workflow once, then verify version banner, launcher icon, project-manager identity and play-window label from the resulting APK.
3. Evaluate Jolt versus Godot Physics using car, character and ragdoll regression scenes; keep PhysX as research until its exact Android build/license path is verified.
4. Keep renderer development as a separate track instead of letting it crowd out components/editor work.
5. After this first coherent component + identity batch is complete, run internal CI to catch native/API errors; do not ask the user to install/test the engine yet.

Always follow `DEVELOPMENT_PROTOCOL.md`. Keep PEAK untouched and report what is written, compiled and tested accurately.
