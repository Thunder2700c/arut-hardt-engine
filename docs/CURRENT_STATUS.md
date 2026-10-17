# Current Status — Resume Here First

Last updated: 2026-10-17.

## Direction reset

The engine is the product right now; PEAK/gameplay testing is not the priority. The user wants a C++-first engine with an original editor UX and high-quality mobile graphics. Do not ask for a phone test after each feature. Add coherent systems, build at integration milestones, and defer private-alpha testing until the engine has useful features.

## Naming

Umbrella: **ARUT-HARDT Engine**.
- **ARUT Mobile** — Android/mobile-first edition.
- **HARDT Studio** — desktop editor edition.

One shared core, API and project format.

## Permanent repository

Repository: https://github.com/Thunder2700c/arut-hardt-engine  
Default branch: main.  
Working branch: `engine-first-cpp-ux`.

Keep PEAK separate. Do not merge into PEAK or change its main branch as a side effect.

## What exists in this repo

- Initial C++ GDExtension class `ArutRuntime` under `native/arut_native/`.
- Isolated extension smoke-test project under `tests/native_smoke/`.
- Shader/material prototype and demo under `renderer/demo/`.
- Manual CI workflows for Android ARM64 editor/native extension and Linux x86_64 editor.
- Pinned intended upstreams: Godot `4.7-stable`; godot-cpp commit `2bb1acf6f8ae1ce96ac7b9ba3a10b499c59c49ec`.
- `engine/modules/arut/` contains built-in native C++ source for `ArutCar3D`, `ArutCharacter3D`, `ArutRagdollDummy3D` and `ArutRenderProfile3D`.
- `engine/patches/` contains source patches for ARUT theme/window/project-manager/About identity plus `0003-arut-runtime-identity.patch`, which changes the generated product name and Android run-window label. Branding assets are in `engine/branding/`.
- `templates/arut_character_lab/` contains a lab scene; the current character visual is still a procedural dummy, not a rigged animated mannequin.
- `docs/OPEN_SOURCE_STACK.md` records the renderer, physics, vehicle, asset and motion-matching shortlist.
- `docs/ANIMATION_SYSTEM_ARCHITECTURE.md` describes a GASP-inspired animation system and C++ port strategy.
- `ROADMAP.md`, `DEVELOPMENT_PROTOCOL.md`, `docs/PHYSICS_ARCHITECTURE.md` and `docs/EDITOR_UX_ARCHITECTURE.md` define product direction and status rules.

## Build truth

- Successful Android ARM64 workflow run: [39024274125](https://github.com/Thunder2700c/arut-hardt-engine/actions/runs/39024274125), source commit `158ea1efb2c897d52adb8f676376f59bbfc7aec5` (2026-10-17).
- At that commit, patch checks, launcher icon step as it existed at the time, C++ preflight, five ARUT module object builds, full Android editor compile, Gradle packaging and upload passed. The native GDExtension build/packaging jobs also passed.
- Latest branch commit `a342f26` adds the runtime identity patch, assigns `templates/arut_character_lab/icon.png` as that project's icon, and points its `project.godot` setting to it. The adaptive/legacy launcher-icon generator was also changed after the last successful build to write the shared Android library resources. **None of these newer source/workflow changes has yet been verified by a later CI run.**
- The user's `[ARUT] SMOKE TEST PASSED` runtime message confirms the separate C++ GDExtension smoke-test project loaded. It does not prove the built-in Car3D, Character3D, ragdoll, or RenderProfile behavior in the editor or in a character lab scene.
- The app icon, adaptive icon, project/game icon and exported game icon are separate resource paths. Successful generation/copy steps do not replace a device-side visual verification.
- A new source patch changes `version.py`'s human-facing name to `ARUT-HARDT Engine` while intentionally keeping `short_name = "godot"`, and changes the Android editor run-window label to `ARUT Run window`. This is **source-written, not yet built or visually verified**. Perform a wider source search for other product-facing strings; preserve Godot attribution and licence notices in About/docs.
- `ArutCar3D`, `ArutCharacter3D`, `ArutRagdollDummy3D` and `ArutRenderProfile3D` were source-compiled at the successful commit; their in-scene behavior, node-browser visibility and phone runtime behavior remain unverified.

## Selected engineering direction

- Renderer: keep Godot's Mobile renderer and scene/resource pipeline for Android; author an ARUT-owned anime/PBR material stack. Do not transplant Filament or bgfx as if either were a drop-in renderer. Use Forward+ for desktop high-quality profiles where appropriate, with realistic compatibility fallbacks.
- Physics: compare Godot Physics and built-in Jolt through ARUT car, ragdoll and character tests. Do not assume Jolt's native VehicleConstraint is exposed through Godot's high-level physics API; don't add multiple physics SDKs without evidence.
- Robot prototype: use Quaternius Animated LowPoly Robot (CC0, 14 clips, approximately 7 MB) for an import/playback smoke test. This asset is a prototype, not the final GASP-style humanoid mannequin.
- Production mannequin: use a consistent, skinned humanoid rig with an ARUT white/gray robot-material treatment and curated CC0 animation clips; do not bundle entire multi-hundred-megabyte source packs.
- GASP-style motion matching: use Epic's GASP as a behavior reference only. Audit the MIT-licensed `godot-motion-matching` C++ implementation as a port candidate, then integrate/validate against Godot 4.7 as a built-in ARUT module. The current project targets Godot 4.4-era APIs and must not be assumed drop-in compatible.
- Build process: core engine systems remain native C++ and first-class in the editor. The ordinary AnimationTree controller is the first baseline; motion matching, traversal/warping and ragdoll recovery are added incrementally after model/skeleton import is verified.

## Next actions

1. Run the Android ARM64 editor workflow on the current branch after the launcher/adaptive-icon and runtime-identity changes. Inspect the resulting APK/log and verify its launcher icon and run-window label before saying branding is fixed; distinguish the editor app icon from project/exported-game icons.
2. Add the compact CC0 animated robot asset to the character lab and validate GLB/FBX import, Skeleton3D hierarchy and clip names. Preserve the asset license/source in the manifest.
3. Build the real white ARUT mannequin scene using a consistent humanoid rig and a curated set of CC0 locomotion clips; drive it from the existing C++ Character3D movement rather than leaving the generated primitives as the primary visual.
4. Add a conventional C++-first AnimationTree controller for idle/start/stop/walk/run/sprint/strafe/turn/jump/fall/land.
5. Audit and port the MIT Godot motion-matching source to Godot 4.7 as a built-in C++ module; add pose/trajectory baking, bounded queries, debug tools and a state-machine fallback.
6. In a later coherent feature batch, advance Car3D handling, the mobile anime/PBR shader stack, physics backend comparisons and runtime telemetry.
7. Run Android ARM64 CI after the coherent source/asset batch, then report exact commit, artifact and test scope honestly.

Always follow `DEVELOPMENT_PROTOCOL.md`. Keep PEAK untouched and report what is written, compiled and tested accurately.
