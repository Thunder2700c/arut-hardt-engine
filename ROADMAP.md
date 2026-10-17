# Roadmap

Statuses apply to the initial foundation; nothing is marked built or tested without actual results.

## Phase 0 — Repository/build foundation (in progress)
- [x] Set umbrella name and two edition names.
- [x] Stage an engine-only tree without game files or assets.
- [x] Add manual CI definitions for Android editor, native extension/smoke bundle and Linux editor.
- [x] Add C++ GDExtension scaffold and on-screen smoke-test project.
- [x] Add a renderer demo project and first shader prototype.
- [ ] Move this clean staging tree to a new standalone GitHub repository.
- [ ] Pin upstream revisions and verify license bundle.
**Gate:** dedicated engine repository, no PEAK assets, coherent/manual build setup.

## Phase 1 — Native C++ proof (scaffolded, not runtime-verified)
- [x] Implement ArutRuntime::get_build_info().
- [x] Add on-screen smoke test.
- [ ] Build artifact from the engine repository.
- [ ] Confirm class registration, instantiation and returned build info on target.
**Gate:** extension loads in matching Godot 4.7 editor; smoke-test UI says SMOKE TEST PASSED.

## Phase 2 — First stylized visual slice (prototype written, not shader-compiled)
- [x] Create an isolated mesh/light/environment demo without PEAK assets.
- [x] Implement a first anime-surface shader with colour tint, albedo, optional normal/AO maps, UV and triplanar modes.
- [x] Keep texture scale explicit with a world-triplanar default of 1.0.
- [x] Keep expensive GI/reflection features off by default.
- [ ] Compile and visually inspect shader on matching Godot 4.7.
- [ ] Record FPS/frame time before promoting material to PEAK.
**Gate:** visible art-direction improvement without a severe performance regression on S23 FE.

## Phase 3 — Mobile renderer baseline
- [ ] Mobile/Balanced/High profiles.
- [ ] Centralized environment/light ownership for streamed chunks.
- [ ] Resolution scaling, shadow distance, LOD/culling, instancing and texture budgets.
- [ ] Measure cold start, frame time, memory and thermal behavior.
**Gate:** stable documented baseline on target hardware.

## Phase 4 — Engine-source renderer/editor work
- [ ] Check if each feature needs a Godot patch or can use shader/GDExtension.
- [ ] Add small reviewed patches only for justified limitations.
- [ ] Pin upstream revision and validate patches with git apply --check.
- [ ] Rebuild after a coherent patch batch and compare with unmodified Godot using the same scene.
**Gate:** reproducible benefit shown by benchmark or authoring workflow.

## Phase 5 — Beginner workflows and releases
- [ ] Mobile 3D template, starter controller, material/lighting presets and diagnostics.
- [ ] Test that a beginner can create/run a small project without local command-line setup.
- [ ] Versioned artifacts, checksums, changelog, notices and rollback steps.
- [ ] ARUT Mobile Android ARM64; HARDT Studio desktop first, other platforms later.

**Test policy:** don't request an on-device APK test until Phase 1 + Phase 2 form a complete vertical slice. Then run one combined cloud build and report actual results.
