# Mandatory Development Protocol

This file is the durable project instruction set. Whenever work resumes, read this file and ROADMAP.md first.

## User constraints
- User is a beginner, Android-only on a Samsung Galaxy S23 FE (Exynos 2200 / Xclipse 920).
- Give exact Godot Android UI steps and copy-pasteable code. No Termux/terminal steps unless explicitly requested.
- Keep user testing infrequent: do not request an APK build/test until the planned vertical slice is ready, unless a blocker cannot be resolved otherwise.
- Be honest about what was committed, compiled, and tested. Never claim a build succeeded without logs/artifact evidence.

## Repository boundaries
- Engine repo contains engine code, tools, templates, documentation, shaders, tests and build scripts only.
- Never copy PEAK scenes/assets/textures/models/audio/saves/game-specific code here.
- The PEAK game repo remains separate and recoverable; do not edit its main branch as a side effect.
- One shared engine core. ARUT Mobile and HARDT Studio are distribution profiles, not duplicate engines.
- Manual CI only until a release candidate is ready; generated binaries are not committed.

## Required order for every task
1. Read this file, ROADMAP.md and affected files.
2. Verify exact Godot source/API version and relevant upstream API/documentation.
3. State the smallest change and acceptance criteria.
4. Preserve behavior and compatibility unless intentionally changing it.
5. Implement one coherent slice, not a broad rewrite.
6. Inspect resulting files, ABI/platform paths, API compatibility and workflow assumptions.
7. Update docs/roadmap.
8. Separate committed, built, tested and unverified status.
9. Avoid full Android builds after every edit; batch coherent work and run a manual cloud build at the verification gate.
10. Give the user no more than one immediate action at a time.

## C++ policy
- Prefer GDExtension for native gameplay classes until a real requirement needs engine-source access.
- Use engine patches only for renderer/editor/runtime changes that GDExtension cannot express.
- C++ may help CPU-bound code; it does not automatically fix GPU-bound rendering, shader cost, draw calls, streaming stutter or memory pressure.
- Match godot-cpp API, Godot source revision, ABI and .gdextension library paths.
- Migrate existing scripts one measured, self-contained system at a time; no wholesale rewrite.

## Graphics policy
- Baseline mobile renderer and good material/art direction first.
- Never enable SDFGI, SSAO and SSR together by default.
- Add stylized features one at a time, with an isolated demo and performance check.
- Preserve existing texture density, triplanar mapping, normal and AO maps where possible. PEAK's observed triplanar scale 1.0 looked much closer to the old texture than 0.1.
- Don't replace production materials en masse. Named Mobile/Balanced/High presets; Ultra is optional and device-dependent.

## First on-device test gate
Before asking the user to install/test, prepare:
- reproducible Android ARM64 editor build;
- freshly built C++ extension with registered-class smoke test;
- isolated, lightweight stylized shader/material demo;
- simple install/import and rollback instructions.
Then run one combined cloud build and report actual results.

## Resume checklist
At every new chat: inspect latest branch/commit, ROADMAP status, workflow results/artifacts, extension manifest/library names, renderer test status and blockers. Continue the first incomplete milestone; don't restart.
