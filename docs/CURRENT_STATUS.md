# Current Status — Resume Here First

Last updated: 2026-10-17.

## Naming
Umbrella: **ARUT-HARDT Engine**.
- **ARUT Mobile**: Android/mobile-first edition.
- **HARDT Studio**: desktop editor edition.
They must remain one shared codebase and project format.

## Repository situation
The engine-only working tree is currently staged on branch `arut-hardt-engine` inside `Thunder2700c/Peak`:
https://github.com/Thunder2700c/Peak/tree/arut-hardt-engine

The branch HEAD contains engine files only (no PEAK levels, assets or root game project), but its Git history has the PEAK repository as an ancestor. Treat it as temporary staging, not the permanent engine repository. Before any public release or normal cloning, create a dedicated `Thunder2700c/arut-hardt-engine` repository and transfer only this branch's current tree to it with a fresh history. Do not merge this branch into the PEAK main branch.

## Implemented in staging tree
- Project README, MIT license for original files, upstream notices and version pin document.
- `DEVELOPMENT_PROTOCOL.md`: mandatory durable process rules; read at every resume.
- `ROADMAP.md`: milestone status; no unverified item should be marked built/tested.
- Manual Android editor/native extension/demo workflow: `.github/workflows/build-arut-mobile.yml`.
- Manual Linux desktop editor/native extension workflow: `.github/workflows/build-hardt-studio.yml`.
- C++ GDExtension class `ArutRuntime` under `native/arut_native/`.
- Isolated runtime smoke project at `tests/native_smoke/`.
- Shader prototype `renderer/demo/shaders/arut_anime_surface.gdshader`.
- Isolated procedural demo project at `renderer/demo/`.
- Upstream selection: Godot tag `4.7-stable`; godot-cpp commit `2bb1acf6f8ae1ce96ac7b9ba3a10b499c59c49ec`.

## Verification truth
Nothing in this staging branch has been compiled or run as part of this new work. The earlier editor APK the user has installed came from their existing Peak workflow; it is not proof that this staging branch's workflows, native extension, or shader work. The smoke-test and shader demo source have not yet been compiled in the target editor.

## User test policy
The user wants to avoid repetitive APK compilation and will test only after the development is about halfway complete. Don't ask them to compile now. Complete the repository separation, review scripts/workflows, and finish the first shader/material vertical slice first. At that gate, run one combined cloud build and give the user one test package with clear UI steps.

## Immediate next action
Ask the user to create a new repository named `arut-hardt-engine` under their GitHub account, with a README initialized. Then transfer only the current engine branch tree into its `main` branch, check the transferred file list, and keep the PEAK repo unchanged. Once moved, work only in the new engine repository.
