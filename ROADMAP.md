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
- [x] Write the first open-source stack decision record in `docs/OPEN_SOURCE_STACK.md`.
- [x] Document the GASP-inspired humanoid animation architecture in `docs/ANIMATION_SYSTEM_ARCHITECTURE.md`.
- [ ] Batch-check workflows after each coherent implementation group.

## Phase 1 — First-class C++ engine components
- [ ] Define ARUT node names/categories and Inspector conventions.
- [x] Add first-pass built-in C++ `ArutCar3D` source and module-registration scaffold.
- [ ] Refine high-level `ArutCar3D`: validate runtime physics; manage wheel probes/suspension, steering, drive layout, engine force, brakes, tire grip and control mapping through Inspector properties.
- [x] Add source-level compact/sedan/off-road preset property.
- [x] Add first-pass native `ArutCharacter3D` movement plus generated mannequin/collision/camera source.
- [x] Add `ArutRagdollDummy3D` with procedural rigid-body limbs and cone-twist joints; connect it to the character's ragdoll trigger.
- [ ] Replace the procedural primitive dummy as the primary visual with a real skinned humanoid mannequin; keep primitives only as a safe fallback.
- [ ] Refine character movement with tuned step/slope handling and authoring defaults.
- [ ] Add C++ interactable/rigid-body and physical-prop helpers.
- [ ] Add physics/vehicle diagnostics: contacts, speed, wheel compression and frame-time telemetry.
- [x] Add the source-build overlay for first-class native C++ module classes so every project using the custom editor can create them.
- [x] Compile the ARUT built-in C++ module in the Android ARM64 editor workflow at commit `158ea1e` (2026-10-17).
- [ ] Verify ARUT classes appear in Create Node and validate runtime behavior.

**Gate:** core components are searchable in the node browser and configurable without writing GDScript.

## Phase 2 — ARUT-HARDT editor identity and UI redesign
- [x] Add ARUT-HARDT theme/window/project-manager identity patches, branded About/welcome text, original mark/logos, and native component icons.
- [x] Add the source patch that sets the product-facing version name to ARUT-HARDT Engine, changes the Android run-window label, and configures the character lab project's icon path (new source changes are not yet build/visually verified).
- [ ] Re-run the Android ARM64 workflow after removing the overlapping duplicate `0003-arut-runtime-branding.patch`; run `39051212243` failed at patch validation before C++ compilation.
- [ ] Search for additional product-facing Godot labels and verify the generated launcher/adaptive icon on a new Android APK; editor app icon, project icon and exported game's app icon are separate assets/settings.
- [ ] Create original theme, icons, typography and spacing—not only different colours.
- [ ] Establish a useful default 3D layout: node/scene panel, viewport, Inspector, file/assets panel and diagnostics.
- [ ] Add node-browser groups for Vehicles, Characters, Interaction, Destruction, Cameras and Rendering.
- [ ] Give ARUT nodes clear Inspector groups, presets and advanced settings.
- [ ] Add Physics/Vehicle and Mobile Performance panels.

**Gate:** the editor feels like ARUT-HARDT rather than the unmodified Godot editor while retaining the underlying scene/resource workflow.

## Phase 3 — Physics backend choice and integration
- [ ] Compare Godot Physics and built-in Jolt for Android ARM64 using ARUT car, character, ragdoll and joint test scenes; do not assume the Jolt-specific VehicleConstraint is exposed by Godot's high-level API.
- [ ] Audit the existing Godot 4.x PhysX 5 backend work, exact source revision, license and Android build path; keep it as research until evidence justifies adoption.
- [ ] Keep high-level ARUT systems backend-neutral where practical; do not hard-code Jolt as the only solver.
- [ ] Select one practical primary backend and at most one maintained alternative after evidence is collected.
- [ ] Add a project setting for backend choice only after real adapters exist.
- [ ] Enrich C++ vehicle dynamics with tire slip, combined grip, wheel speed, drivetrain, gearbox, differential, separate brakes, ABS/TCS and steering response.
- [x] Add first-pass native character movement and passive ragdoll hand-off source.
- [ ] Fix/validate ragdoll joint paths and physical behavior before animation handoff work.
- [ ] Add animation-to-ragdoll blending/recovery, richer character physical-bone rigs, material-based interaction and selective breakable props.

**Gate:** the selected backend builds on target platforms and serves the standard body/collision/query APIs.

## Phase 4 — Ready-made character, animation and vehicle templates
- [ ] Prototype the compact Quaternius Animated LowPoly Robot (CC0, 14 clips, approximately 7 MB source pack) with a white material override; confirm import and clip names. This is an asset/import smoke test, not the final GASP-style system.
- [ ] Create the primary ARUT Test Mannequin: skinned humanoid mesh + Skeleton3D, neutral white/gray robotic materials and faceplate, and curated CC0 locomotion clips; don't ship complete multi-hundred-megabyte source asset packs.
- [ ] Audit a CC0 humanoid rig and Quaternius Universal Animation Library / Library 2 clip compatibility, root bone, scale, rest pose and root-motion mode.
- [ ] Add a C++-first conventional AnimationTree/AnimationPlayer controller for idle, start/stop, walk/jog/sprint, strafe, turn, jump, fall, land and crouch.
- [ ] Audit and port the MIT-licensed `godot-motion-matching` implementation to the pinned Godot 4.7 source as a built-in C++ module; do not treat its Godot 4.4 implementation as drop-in compatible.
- [ ] Implement a baked pose/trajectory database, spatial-index query, tunable query rate/feature weights, AnimationTree blending and conventional state-machine fallback.
- [ ] Add GASP-inspired trajectory-aware locomotion, contextual action selection, debug view and then traversal hooks (vault/ledge climb) with collision-aware root motion/animation warping.
- [ ] Keep GASP as a feature reference only; use CC0 animation assets and independently implemented ARUT code rather than bundling unverified Unreal-only content.
- [x] Add a C++ character/ragdoll starter project with default input actions and mobile touch controls (scene exists; character animation still needs integration).
- [ ] Add a Car3D starter scene with actual chassis mesh/collider, visible wheels, mobile controls, reset and telemetry.
- [ ] Make scene/component creation, mesh/material assignment, physics tuning and Android packaging discoverable without command-line work.
- [ ] Add native CPU/GPU/frame-time, physics-cost, animation-query and memory diagnostics.
- [ ] Provide helpful errors for missing native libraries/resources or missing animation clips.

**Gate:** a new project can add an animated mannequin or configured car and run it without manually wiring dozens of low-level nodes.

## Phase 5 — Rendering and mobile performance
- [x] Create isolated shader/material and procedural demo prototypes.
- [ ] Verify shader/API semantics against the exact Godot version; preserve UV/triplanar, normal and AO behavior.
- [x] Add native `ArutRenderProfile3D` scene node with Mobile/Balanced/High viewport presets (baseline source compiled in the successful Android CI build; live-switching port committed as `3d0c138c245fed208b65120cacb59a0ee17220e7` and awaiting a fresh build/runtime check).
- [ ] Profile actual cost on the target renderer and tune preset values.
- [ ] Add centralized lighting/environment conventions, fog, LOD/culling, instancing and texture budgets.
- [ ] Build a mobile-first grass/foliage shader prototype with wind animation, alpha-scissor cutouts, correct normal/UV behavior and practical instancing/distance budgets; keep it as an isolated feature demo until its API and costs are verified.
- [ ] Build a stylized anime/PBR hybrid material stack with optional outlines/rim light, face/hair controls and sensible fallbacks for the Mobile renderer.
- [ ] Add reusable first-party ARUT Anime Lit and ARUT PBR material presets with clear Inspector controls.
- [ ] Keep desktop Forward+ features from leaking into unsupported mobile profiles.
- [ ] Benchmark frame time, memory and thermal behavior before performance claims.

**Gate:** an ARUT scene can target mobile stylized PBR/anime visuals without enabling every expensive effect or requiring desktop-only rendering features.

## Phase 6 — Private alpha for five people
- [ ] Batch built-in C++ component work, mannequin/animation, editor identity and the initial physics decision into coherent source changes.
- [ ] Run Android ARM64 and Linux x86_64 cloud builds; report actual source commit, logs and artifacts.
- [ ] Run focused internal runtime tests for mannequin locomotion/motion matching, car handling, ragdoll, renderer profiles and branding.
- [ ] Prepare private installation/import/rollback instructions and versioned artifacts.
- [ ] Ask the user and four other intended testers to try the engine only when it has enough real features to evaluate.

## Existing prototypes (status tracked separately)
- `engine/modules/arut/`: built-in C++ components for Car3D, Character3D, RagdollDummy3D and RenderProfile3D. Source compiled in the successful Android CI run at `158ea1e`; runtime behavior is not yet verified.
- `templates/arut_character_lab/`: importable character/ragdoll lab packaged by the Android workflow; currently uses the procedural dummy and awaits model/animation integration.
- `native/arut_native/`: initial optional C++ GDExtension smoke-test scaffold; its smoke test does not verify the built-in character/car/render systems.
- `renderer/demo/`: isolated anime surface shader/demo prototype.
- `docs/OPEN_SOURCE_STACK.md`: renderer, physics, vehicle and CC0 asset evaluation.
- `docs/ANIMATION_SYSTEM_ARCHITECTURE.md`: GASP-inspired C++ motion matching and mannequin implementation plan.
- `docs/PHYSICS_ARCHITECTURE.md`: open physics-backend evaluation.

**Status rule:** never claim source is compiled or tested without evidence. That rule does not prevent feature-first development; it prevents unsupported claims.
