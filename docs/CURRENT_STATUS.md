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

## Build truth
The permanent repository's Android and Linux workflows have not been verified by a successful run in this branch. The earlier installed editor APK came from the previous PEAK workflow; it does not prove this repo's workflows or native code compiled. The renderer shader remains a prototype. No custom ARUT Car3D node is implemented yet.

## Physics findings
- Current project baseline is Godot 4.7 with built-in Godot Physics/Jolt options; Jolt is not a permanent decision.
- A community Godot 4.x PhysX 5 backend exists as work-in-progress and needs a source/commit, license/notice, Android ARM64 and build-path audit before adoption.
- Bullet and ReactPhysics3D remain comparison candidates; neither is a drop-in Godot 4.7 backend.
- Next physics milestone is evaluating the existing PhysX backend and designing native, backend-aware ARUT components—not blindly embedding another solver.

## Next actions
1. Start implementing the first native high-level component (`ArutCar3D`) and establish the path to a built-in C++ node in the source-build process.
2. Start ARUT-HARDT editor identity work: original theme/icons, project manager and node categories, then dedicated inspector/panel improvements.
3. Continue physics backend comparison, checking Android/build details before choosing a primary solver.
4. Keep renderer work moving as a separate workstream instead of letting it crowd out core engine features.
5. Batch CI/build checks when a coherent source change is ready. Do not ask the user to install/test the engine yet.

Always follow `DEVELOPMENT_PROTOCOL.md`. Keep PEAK untouched and report what is written, compiled and tested accurately.
