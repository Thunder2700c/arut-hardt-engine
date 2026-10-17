# Current Status — Resume Here First

Last updated: 2026-10-17.

## Naming
Umbrella: **ARUT-HARDT Engine**.
- **ARUT Mobile** — Android/mobile-first edition.
- **HARDT Studio** — desktop editor edition.
One shared engine core, API and project format.

## Permanent repository
Repository: https://github.com/Thunder2700c/arut-hardt-engine
Default branch: `main`.
The clean engine-only foundation has been transferred here as a new commit based on this repo's own initial commit. No PEAK game levels, assets, root project or game-specific files are in the current tree.
The temporary staging branch remains under the PEAK repository for history/recovery only. Do not develop further in it and do not merge it into PEAK main.

## Implemented in the permanent repository
- Project README, license for original files, third-party notices and upstream version document.
- `DEVELOPMENT_PROTOCOL.md`: mandatory durable process; read this and ROADMAP at every resume.
- `ROADMAP.md`: milestones and truthful build/test status.
- Manual Android editor, native extension/smoke-bundle and shader demo workflow: `.github/workflows/build-arut-mobile.yml`.
- Manual Linux desktop editor/native extension workflow: `.github/workflows/build-hardt-studio.yml`.
- Initial C++ GDExtension class `ArutRuntime` under `native/arut_native/`.
- Isolated smoke-test project at `tests/native_smoke/`.
- Shader prototype `renderer/demo/shaders/arut_anime_surface.gdshader` and isolated procedural demo project under `renderer/demo/`.
- Upstream: Godot tag `4.7-stable`; godot-cpp commit `2bb1acf6f8ae1ce96ac7b9ba3a10b499c59c49ec`.

## Verification truth
The repository contents have not yet been compiled or run as part of this engine work. The earlier APK the user installed was built from the previous Peak workflow; it does not prove the new ARUT-HARDT workflow, C++ extension or shader compiles. The new source files remain prototypes until a combined cloud build gives actual evidence.

## User test policy
The user asked to avoid frequent APK builds and test only after development is roughly halfway done. Do not ask the user to compile/test now. Review and fix repository configuration and shader/demo code first. Trigger one combined cloud build at the vertical-slice gate, then give one clear phone test package.

## Immediate next actions
1. Review/fix both CI workflows and native-library paths; pin all relevant build versions.
2. Review shader syntax and demo code against Godot 4.7, keeping expensive GI/reflections off.
3. Add a reproducible smoke-test project artifact and make its status unambiguous.
4. Update ROADMAP only for completed work that is actually present.
5. At the planned gate, run the Android editor/native-extension/demo build once. Inspect all job logs and artifacts; report successes/failures honestly.
6. Only after that test gate, begin mobile rendering profiles and engine-source changes as separate small steps.

Always follow `DEVELOPMENT_PROTOCOL.md`. Never overwrite the PEAK project or install anything on the user's phone without a deliberate verification step.
