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
Default/canonical working branch: `main`. Keep all ongoing work on `main`; do not create feature branches or reopen superseded pull requests. Older branch refs remain on GitHub as historical work because branch deletion is not available through the connected operation set; do not use or force-reset them.  
Most recent code fix covered by this status snapshot: `2bcc1a372378178ae1e8c848278c44e1fce295a5` (2026-10-17) — fixes the Android runtime-branding patch hunk. This status synchronization is a documentation-only follow-up; consult GitHub `main` for the exact current HEAD.

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
- The runtime identity patch, character-lab project icon wiring, adaptive/legacy launcher-icon generation, and uploaded mannequin GLB checks were added after the successful Android run above and still need a passing build.
- Android workflow failure: [run 39050293595](https://github.com/Thunder2700c/arut-hardt-engine/actions/runs/39050293595), source commit `680e6bee5e1f9def46514f94e1d406408e43798d` (2026-10-17), failed at `Validate and apply ARUT-HARDT engine patches` while checking `0003-arut-runtime-branding.patch` against `platform/android/java/editor/src/main/res/values/strings.xml`. The three-line hunk lacked trailing context; integration commit `2bcc1a372378178ae1e8c848278c44e1fce295a5` adds the following resource line as context. A matching local `git apply --check` reproduction passed with that context. The failed run skipped C++ preflight, Android compilation, Gradle packaging and artifact upload. A fresh workflow after the fix has not been run; build/package status remains **unverified**.
- The user's `[ARUT] SMOKE TEST PASSED` runtime message confirms the separate C++ GDExtension smoke-test project loaded. It does not prove the built-in Car3D, Character3D, ragdoll, or RenderProfile behavior in the editor or in a character lab scene.
- The app icon, adaptive icon, project/game icon and exported game icon are separate resource paths. Successful generation/copy steps do not replace a device-side visual verification.
- A new source patch changes `version.py`'s human-facing name to `ARUT-HARDT Engine` while intentionally keeping `short_name = "godot"`, and changes the Android editor run-window label to `ARUT Play window`. This is **source-written, not yet built or visually verified**. Perform a wider source search for other product-facing strings; preserve Godot attribution and licence notices in About/docs.
- `ArutCar3D`, `ArutCharacter3D`, `ArutRagdollDummy3D` and the baseline `ArutRenderProfile3D` were source-compiled at the successful commit; their in-scene behavior, node-browser visibility and phone runtime behavior remain unverified.
- Main commit `3d0c138c245fed208b65120cacb59a0ee17220e7` ports live render-profile switching by adding independent `quality_apply_on_change`, editor-safe automatic application, and `apply_to_current_viewport()` while retaining `apply_now()` compatibility. The source commit is present, but GitHub reported no CI status or workflow run for it; this change is **not yet build-verified**.

## Selected engineering direction

- Renderer: keep Godot's Mobile renderer and scene/resource pipeline for Android; author an ARUT-owned anime/PBR material stack. Do not transplant Filament or bgfx as if either were a drop-in renderer. Use Forward+ for desktop high-quality profiles where appropriate, with realistic compatibility fallbacks.
- Physics: compare Godot Physics and built-in Jolt through ARUT car, ragdoll and character tests. Do not assume Jolt's native VehicleConstraint is exposed through Godot's high-level physics API; don't add multiple physics SDKs without evidence.
- Robot prototype: use Quaternius Animated LowPoly Robot (CC0, 14 clips, approximately 7 MB) for an import/playback smoke test. This asset is a prototype, not the final GASP-style humanoid mannequin.
- Production mannequin: use a consistent, skinned humanoid rig with an ARUT white/gray robot-material treatment and curated CC0 animation clips; do not bundle entire multi-hundred-megabyte source packs.
- GASP-style motion matching: use Epic's GASP as a behavior reference only. Audit the MIT-licensed `godot-motion-matching` C++ implementation as a port candidate, then integrate/validate against Godot 4.7 as a built-in ARUT module. The current project targets Godot 4.4-era APIs and must not be assumed drop-in compatible.
- Build process: core engine systems remain native C++ and first-class in the editor. The ordinary AnimationTree controller is the first baseline; motion matching, traversal/warping and ragdoll recovery are added incrementally after model/skeleton import is verified.

## Next actions

1. Dispatch `Build ARUT Mobile (Android ARM64)` on current `main` after the runtime-branding patch-context fix. Run `39050293595` failed before C++ compilation, so verify a fresh run reaches patch validation, C++ preflight/object builds, full Android editor compilation, Gradle packaging and artifact upload before reporting a successful APK/editor build.
2. Build a mobile-first grass/foliage shader prototype in an isolated renderer demo, including wind, cutout, sensible distance/instance budgets, and preserved normals/UV behavior.
3. Audit and complete the first-party anime/PBR material path without creating a duplicate of the existing `ArutAnimeLitMaterial3D` or replacing the current renderer-compatibility logic.
4. Add the compact CC0 animated robot asset to the character lab and validate GLB/FBX import, Skeleton3D hierarchy and clip names. Preserve the asset license/source in the manifest.
5. Build the real white ARUT mannequin scene using a consistent humanoid rig and curated CC0 locomotion clips, driven by the existing C++ `ArutCharacter3D` movement instead of leaving generated primitives as the primary visual.
6. Add the conventional C++-first AnimationTree controller for idle/start/stop/walk/run/sprint/strafe/turn/jump/fall/land. Audit and port motion matching to Godot 4.7 only after the rig/controller baseline works.
7. At a coherent integration milestone, run both `Build ARUT Mobile (Android ARM64)` and `Build HARDT Studio (Linux x86_64)`, then report exact commits, artifacts and verified scope. The workflows are manual; if the connected tools cannot dispatch them, provide the precise UI steps rather than claiming a run.

Always follow `DEVELOPMENT_PROTOCOL.md`. Keep PEAK untouched and report what is written, compiled and tested accurately.
