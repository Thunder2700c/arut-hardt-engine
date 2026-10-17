# ARUT-HARDT Engine Roadmap

Priority: build a usable engine, not make PEAK test every prototype. Features are C++-first. Internal compilation checks happen at sensible integration milestones; user phone testing is deferred until a useful private-alpha engine package exists.

## Phase 0 — Independent engine foundation (in progress)
- [x] Name the engine and distributions.
- [x] Transfer the engine-only tree into its own repository/history.
- [x] Keep PEAK game assets/scenes outside this repo.
- [x] Add manual Android editor, Linux editor and native-extension workflows.
- [x] Pin intended Godot/godot-cpp versions.
- [x] Add initial C++ extension scaffold and renderer/material prototype.
- [ ] Define the path from current source-build overlay to first-class built-in engine modules and editor changes.
- [ ] Batch-check workflows at a sensible integration milestone.

## Phase 1 — First-class C++ engine components
- [ ] Define ARUT node names/categories and Inspector conventions.
- [x] Add first-pass built-in C++ `ArutCar3D` source and module-registration scaffold (not yet compiled/runtime-verified).
- [ ] Refine high-level `ArutCar3D`: validate module integration and physics behavior; manage wheel probes/suspension, steering, drive layout, engine force, brakes, tire grip and control mapping through Inspector properties.
- [x] Add source-level compact/sedan/off-road preset property (not yet compiled/verified).
- [ ] Add native FPS/third-person character movement with tunable acceleration, ground friction, steps/slopes, air control and jumping.
- [ ] Add C++ interactable/rigid-body and physical-prop helpers.
- [ ] Add physics/vehicle diagnostics: contacts, speed, wheel compression and frame-time telemetry.
- [ ] Promote first-class nodes from extension prototypes into the engine source/module build so every new project has them.

**Gate:** core components are searchable in the node browser and configurable without writing GDScript.

## Phase 2 — ARUT-HARDT editor identity and UI redesign
- [ ] Replace default branding in project manager, splash/about surfaces, icons and application identity with ARUT-HARDT branding; retain Godot attribution in notices/docs.
- [ ] Create original theme, icons, typography and spacing—not only different colours.
- [ ] Establish a useful default 3D layout: node/scene panel, viewport, Inspector, file/assets panel and diagnostics.
- [ ] Add node-browser groups for Vehicles, Characters, Interaction, Destruction, Cameras and Rendering.
- [ ] Give ARUT nodes clear Inspector groups, presets and advanced settings.
- [ ] Add Physics/Vehicle and Mobile Performance panels.

**Gate:** the editor feels like ARUT-HARDT rather than the unmodified Godot editor while retaining the underlying scene/resource workflow.

## Phase 3 — Physics backend choice and integration
- [ ] Compare Godot Physics, integrated Jolt, PhysX 5, Bullet and ReactPhysics3D against Android ARM64 support, features, integration cost, performance and license/notices.
- [ ] Audit the existing Godot 4.x PhysX 5 backend work, exact source revision, dependency build and Android support.
- [ ] Keep high-level ARUT systems backend-neutral where practical; do not hard-code Jolt as the only solver.
- [ ] Maintain Godot Physics and Jolt as comparison/fallback options during evaluation.
- [ ] Select one practical primary backend and at most one maintained alternative once the evidence is clear.
- [ ] Add a project setting for backend choice only after real adapters exist.
- [ ] Enrich C++ vehicle dynamics with tire slip, combined grip, wheel speed, drivetrain, gearbox, differential, separate brakes, ABS/TCS and steering response.
- [ ] Add character movement, ragdoll/physical bones, material-based interaction and selective breakable props.

**Gate:** the selected backend builds on target platforms and serves the standard body/collision/query APIs.

## Phase 4 — Original editor workflows and templates
- [ ] Create C++-ready starter projects with default input actions, character and Car3D templates.
- [ ] Make scene/component creation, mesh/material assignment, physics tuning and Android packaging discoverable without command-line work.
- [ ] Add native CPU/GPU frame-time, physics-cost, memory and quality-profile diagnostics.
- [ ] Provide helpful errors for missing native libraries/resources.

## Phase 5 — Rendering and mobile performance
- [x] Create isolated shader/material and procedural demo prototypes.
- [ ] Verify shader/API semantics against the exact Godot version; preserve UV/triplanar, normal and AO behavior.
- [ ] Create Mobile/Balanced/High profiles with explicit costs.
- [ ] Add centralized lighting/environment conventions, fog, LOD/culling, instancing and texture budgets.
- [ ] Build a stylized anime/PBR hybrid material stack with optional outlines/rim light.
- [ ] Benchmark before making performance claims.

## Phase 6 — Private alpha for five people
- [ ] Batch built-in C++ node work, editor identity changes and the initial physics decision into coherent source changes.
- [ ] Run Android ARM64 and Linux x86_64 cloud builds; report actual logs/artifacts.
- [ ] Prepare private installation/import/rollback instructions and versioned artifacts.
- [ ] Ask the user and four other intended testers to try the engine only when it has enough real features to evaluate.

## Existing prototypes (not yet validated)
- `engine/modules/arut/`: initial built-in C++ module and `ArutCar3D` source; not compiled yet.
- `native/arut_native/`: initial optional C++ GDExtension smoke-test scaffold.
- `renderer/demo/`: isolated anime surface shader/demo prototype.
- `docs/PHYSICS_ARCHITECTURE.md`: open physics-backend evaluation, not a selected or integrated backend.

**Status rule:** never claim source is compiled or tested without evidence. That rule does not prevent feature-first development; it prevents unsupported claims.
