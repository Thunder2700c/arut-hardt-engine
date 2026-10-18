# Current Status — Resume Here First

Last updated: 2026-10-18.

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
Latest verified integration commit (2026-10-18): `6d7eaa11cacb49fb89b6962c9cb4639dda3b9041` — removes the duplicate Android branding patch, enforces unique runtime-identity patch ownership, and has a successful Android ARM64 editor workflow run.

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

- **Successful Android ARM64 integration workflow:** [run 39051791844](https://github.com/Thunder2700c/arut-hardt-engine/actions/runs/39051791844), source commit `6d7eaa11cacb49fb89b6962c9cb4639dda3b9041` (2026-10-17). All four jobs completed successfully.
- The Android editor job passed runtime-patch ownership preflight, all three patch checks/applications, branding asset copy, legacy/adaptive icon generation, mannequin GLB preflight, ARUT C++ header/lifecycle preflight, the five ARUT module object builds, full Android ARM64 editor compilation, Gradle `generateGodotEditor` packaging and artifact upload. Uploaded editor artifact: `arut-mobile-editor-android-arm64` (176,777,028 bytes; artifact ID 12108945339 at the time checked).
- The separate native-extension job successfully built and packaged the Android ARM64 GDExtension debug/release smoke-test libraries. This is distinct from the built-in `engine/modules/arut/` module.
- The run verifies that the current ARUT C++ module sources compile into the **Android editor binary**. It does not by itself prove character/car/render runtime behavior on a device, nor does it prove that games exported as standalone Android APKs contain the module.
- **Export-template gap:** `.github/workflows/build-arut-mobile.yml` currently compiles `target=editor` with `engine/modules/arut/` copied into Godot. It does not yet compile Android `target=template_debug` and `target=template_release` engine binaries with that built-in module. Until those templates are built and made available to the export workflow/editor, do not claim standalone exported Android projects can use the first-class ARUT module classes.
- Historical patch failures were resolved by removing the overlapping `engine/patches/0003-arut-runtime-branding.patch`; `0003-arut-runtime-identity.patch` is the only owner of `version.py` and `platform/android/java/editor/src/main/res/values/strings.xml`. The ownership preflight now catches accidental duplicate patch owners before applying the patch set. The prior failed runs were [39050293595](https://github.com/Thunder2700c/arut-hardt-engine/actions/runs/39050293595) and [39051212243](https://github.com/Thunder2700c/arut-hardt-engine/actions/runs/39051212243).
- The workflow validates that the humanoid source GLB has a skin and animation data. It does not visually verify its imported appearance, animation playback, or runtime rig compatibility.
- The app icon, adaptive icon, project/game icon and exported game icon are separate resources. CI confirms icon-resource generation and packaging inputs, not device-side visual appearance.
- The prior successful Android build at [39024274125](https://github.com/Thunder2700c/arut-hardt-engine/actions/runs/39024274125) remains useful historical evidence for the earlier source revision; prefer run 39051791844 for the current patch/module-editor integration.
## Selected engineering direction

- Renderer: keep Godot's Mobile renderer and scene/resource pipeline for Android; author an ARUT-owned anime/PBR material stack. Do not transplant Filament or bgfx as if either were a drop-in renderer. Use Forward+ for desktop high-quality profiles where appropriate, with realistic compatibility fallbacks.
- Physics: compare Godot Physics and built-in Jolt through ARUT car, ragdoll and character tests. Do not assume Jolt's native VehicleConstraint is exposed through Godot's high-level physics API; don't add multiple physics SDKs without evidence.
- Robot prototype: use Quaternius Animated LowPoly Robot (CC0, 14 clips, approximately 7 MB) for an import/playback smoke test. This asset is a prototype, not the final GASP-style humanoid mannequin.
- Production mannequin: use a consistent, skinned humanoid rig with an ARUT white/gray robot-material treatment and curated CC0 animation clips; do not bundle entire multi-hundred-megabyte source packs.
- GASP-style motion matching: use Epic's GASP as a behavior reference only. Audit the MIT-licensed `godot-motion-matching` C++ implementation as a port candidate, then integrate/validate against Godot 4.7 as a built-in ARUT module. The current project targets Godot 4.4-era APIs and must not be assumed drop-in compatible.
- Build process: core engine systems remain native C++ and first-class in the editor. The ordinary AnimationTree controller is the first baseline; motion matching, traversal/warping and ragdoll recovery are added incrementally after model/skeleton import is verified.

## Next actions

1. Build Android ARM64 `target=template_debug` and `target=template_release` Godot engine export templates with `engine/modules/arut/` included. Package and make those templates available to ARUT Mobile export; verify the exact Godot 4.7 Android template naming and installation path.
2. Run a native C++ component smoke-test through a standalone Android export using the newly built templates. Until then, the editor build is verified, but exported-game integration/runtime is not.
3. Build and verify the Linux x86_64 HARDT Studio editor at a coherent milestone.
4. Build a mobile-first grass/foliage shader prototype in an isolated renderer demo, including wind, cutout, sensible distance/instance budgets, and preserved normals/UV behavior.
5. Audit and complete the first-party anime/PBR material path without creating a duplicate of the existing `ArutAnimeLitMaterial3D` or replacing the current renderer-compatibility logic.
6. Validate the imported humanoid mannequin appearance, Skeleton3D hierarchy and animation clips; then continue the C++-first conventional controller and GASP-inspired motion-matching work.

Always follow `DEVELOPMENT_PROTOCOL.md`. Keep PEAK untouched and report what is written, compiled and tested accurately.


## Character animation integration — not yet verified

Source integration now embeds the attributed robot GLB in the native module and makes `ArutCharacter3D` instantiate it via `GLTFDocument`, rather than relying on a Character Lab scene. This has not yet been compiled or runtime-tested. Animation playback still relies on the existing semantic clip-name resolver and needs real clip verification; motion matching is not integrated. The MIT `GuilhermeGSousa/godot-motion-matching` project remains a port candidate requiring Godot 4.7 API audit. GASP is a behavioral reference unless Epic explicitly authorizes the intended reuse. Skeleton-bound ragdoll and Android/runtime verification remain incomplete.
