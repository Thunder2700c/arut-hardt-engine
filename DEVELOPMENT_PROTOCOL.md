# Mandatory Development Protocol

Read this file, ROADMAP.md, and docs/CURRENT_STATUS.md whenever work resumes.

## Product direction
- Build **ARUT-HARDT Engine first**. The game made with it comes later; do not let PEAK's immediate gameplay needs dictate the entire engine.
- The initial audience is a private group of five. A public release is not the current goal.
- The user wants native C++ engine features and does not want GDScript to be the foundation for engine systems.
- Prefer high-level nodes and inspector workflows so a beginner can add a ready-made system (such as ArutCar3D) and adjust properties instead of assembling dozens of low-level nodes.
- Do not ask the user to test every feature or install frequent APKs. Add coherent features first. Use internal CI/builds at integration milestones; defer user testing until a useful private-alpha engine package exists.
- Be precise about what is written, committed, compiled, integrated, and runtime-tested.

## Repository boundaries
- Engine repo: https://github.com/Thunder2700c/arut-hardt-engine.
- Keep PEAK separate. Do not change its main branch as a side effect.
- Never copy PEAK scenes, assets, levels, textures, models, sounds, saves, or game-specific scripts into this repository.
- ARUT Mobile and HARDT Studio share one engine core and project format.
- Prefer native Godot modules for first-class node types and editor features. GDExtension is useful for prototypes and optional extensions, but does not automatically make a class a built-in engine feature.
- Keep manual CI during development; do not commit generated binaries.

## Required work sequence
1. Read this file, the roadmap, current status, and affected source files.
2. Check APIs against the pinned Godot 4.7 source/API and target platform.
3. Implement the smallest useful engine-level feature in C++.
4. Review ownership/lifetime, threading, API/ABI, performance, editor exposure, and platform assumptions.
5. Make features discoverable in the node browser and Inspector, with sensible defaults and documentation.
6. Update roadmap/status honestly.
7. Batch compile validation at sensible milestones. Do not make every edit a user phone-test request or stop adding features while waiting for the user to test.
8. Ask the user for an action only when a private-alpha package is ready or a true blocker requires their device.

## C++ policy
- Engine features are C++-first.
- First-class node types, physics backends, editor panels and engine internals should be integrated into the Godot source/module/patch build when appropriate.
- Do not hide a supposedly native feature's core implementation in GDScript.
- C++ does not automatically solve GPU-bound rendering, draw calls, asset streaming or memory pressure.
- Pin and align Godot source, godot-cpp, native ABI and library paths.

## Editor UX policy
- Do more than recolour Godot. Create ARUT-HARDT branding, icons, theme, project-manager/editor identity, useful default panel layouts, native node-browser categories and focused inspector layouts.
- Make custom systems discoverable under ARUT categories.
- Keep upstream attribution and required license notices. Do not copy leaked/decompiled proprietary engine source; use independent implementations and permissively licensed code.

## Physics policy
- Jolt is an option, not a permanent decision or assumed winner.
- Compare Godot Physics, Jolt, PhysX 5 integration work, Bullet and ReactPhysics3D by feature coverage, Android ARM64 support, build/dependency costs, licensing, performance and compatibility with Godot's PhysicsServer API.
- Separate the solver from ARUT's C++ gameplay systems: vehicles, character movement, interaction, joints and breakable props.
- Do not bundle multiple full solvers or promise hot-swapping unless real backend adapters exist. Choose a practical default after evaluation.
- Preserve exact notices for every included dependency.

## Graphics policy
- Keep graphics as one engine workstream; it must not crowd out node systems and editor UX.
- Never enable SDFGI, SSAO and SSR together by default.
- Preserve existing texture density, normal/AO inputs and UV/triplanar behavior where possible. Triplanar scale 1.0 looked much closer to the original than 0.1.
- Use named quality profiles with documented costs.

## Resume checklist
Inspect the latest branch/commit, workflows, roadmap, native class list, editor UX changes, physics-backend research and outstanding blockers. Continue the highest-priority incomplete engine milestone instead of restarting or defaulting to shader/test work.
