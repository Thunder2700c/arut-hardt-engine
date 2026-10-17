# Roadmap

Statuses apply to the initial foundation; nothing is marked built or tested without actual results.

## Phase 0 — Repository/build foundation (in progress)
- [x] Set umbrella name and two edition names.
- [x] Stage an engine-only tree without game files or assets.
- [x] Add manual CI definitions for Android editor, native extension/smoke bundle and Linux editor.
- [x] Add C++ GDExtension scaffold and on-screen smoke-test project.
- [x] Add a renderer demo project and first shader prototype.
- [x] Transfer the clean engine-only tree into its own repository with a separate history.
- [x] Record Godot/godot-cpp source revisions and include initial license/notice files.
- [ ] Review exact third-party notice bundle before public binary releases.
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


## Phase 3 — Physics systems (architecture selected; implementation not started)
- [x] Inspect current PEAK physics selection and vehicle force model.
- [x] Choose Jolt as the default general-purpose physics backend; keep it rather than adding a competing solver.
- [x] Identify MIT Godot Advanced Vehicle as a reference for raycast suspension, brush/Pacejka tire response and drivetrain concepts.
- [x] Record Chrono as an optional C++ vehicle-dynamics research reference, not the default mobile dependency.
- [ ] Build an isolated vehicle test scene and baseline telemetry.
- [ ] Implement the first native C++ vehicle dynamics slice with slip-aware tire forces and suspension.
- [ ] Validate normal physics tick rates on mobile before considering higher rates.
- [ ] Add drivetrain, gears, differential, brakes, ABS/TCS and steering refinements incrementally.
- [ ] Tune FPS character acceleration, friction, slope/step response and selective ragdoll/prop interactions separately.
**Gate:** stable, tunable vehicle/character feel with measured target-device cost.

## Phase 4 — Mobile renderer baseline
- [ ] Mobile/Balanced/High profiles.
- [ ] Centralized environment/light ownership for streamed chunks.
- [ ] Resolution scaling, shadow distance, LOD/culling, instancing and texture budgets.
- [ ] Measure cold start, frame time, memory and thermal behavior.
**Gate:** stable documented baseline on target hardware.

## Phase 5 — Engine-source renderer/editor work
- [ ] Check if each feature needs a Godot patch or can use shader/GDExtension.
- [ ] Add small reviewed patches only for justified limitations.
- [ ] Pin upstream revision and validate patches with git apply --check.
- [ ] Rebuild after a coherent patch batch and compare with unmodified Godot using the same scene.
**Gate:** reproducible benefit shown by benchmark or authoring workflow.

## Phase 6 — Beginner workflows and releases
- [ ] Mobile 3D template, starter controller, material/lighting presets and diagnostics.
- [ ] Test that a beginner can create/run a small project without local command-line setup.
- [ ] Versioned artifacts, checksums, changelog, notices and rollback steps.
- [ ] ARUT Mobile Android ARM64; HARDT Studio desktop first, other platforms later.

**Test policy:** don't request an on-device APK test until Phase 1 + Phase 2 form a complete vertical slice. Then run one combined cloud build and report actual results.
