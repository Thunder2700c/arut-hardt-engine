# ARUT-HARDT Physics Architecture and Backend Evaluation

## Goal
Build engine-level physics workflows, not only a game controller. A user should be able to add a high-level native C++ `ArutCar3D` node, attach a mesh/collider, adjust Inspector properties, and get a basic vehicle without manually assembling every wheel and force calculation.

**No final backend is selected. Jolt is one candidate, not a permanent commitment.**

## Keep four layers separate
1. **Backend:** body/area simulation, collision detection, constraints and queries.
2. **ARUT physics API:** surface/material conventions, common queries, telemetry and project settings.
3. **High-level C++ components:** `ArutCar3D`, `ArutCharacter3D`, `ArutInteractable3D`, `ArutBreakable3D` and physical-interaction helpers.
4. **Editor workflows:** node-browser categories, Inspector properties, presets, diagnostic panels and starter scenes.

The solver alone does not make vehicle creation easy. ARUT needs first-class components and an editor designed around them.

## Candidate backends

### 1. PhysX 5 — leading alternative to investigate
NVIDIA publishes PhysX 5 as an open-source C++ SDK. The official repository documents multi-platform support, including Android ARM, and has an upstream BSD-3-Clause license file; individual SDK components/dependencies may have additional notices, which must be checked for the exact pinned revision. PhysX has mature rigid-body/joint and vehicle-related systems. GPU/CUDA features must not be assumed to work on Android.

A Godot 4.x PhysX 5 backend was linked from a discussion dated 2026-09-03. The author described it as WIP, then reported an expanding set of features: static/rigid/kinematic bodies, common collision shapes, ray/shape queries, contact reporting, Area3D triggers, Joint3D families and CharacterBody3D motion testing. An optional GPU rigid-body path uses CUDA; do not assume it works on Android or on the user's Xclipse GPU. The author later described a separate fork/module-based integration route. This makes it a serious candidate to inspect, **not a ready-to-ship Android backend**. The exact commit, build recipe, CPU ARM64 path, Godot 4.7 compatibility, nested dependency notices and license files still need auditing before adoption.

- SDK: https://github.com/NVIDIA-Omniverse/PhysX
- Godot module branch: https://github.com/uno1982/godot/tree/feature/physx5-module
- Example/benchmark: https://github.com/uno1982/godot-physx-example
- Discussion (2026-09-03): https://github.com/godotengine/godot-proposals/discussions/15471

### 2. Jolt Physics — strong existing option, not automatically “best”
Godot has included Jolt as a 3D backend since Godot 4.4. Upstream Jolt has vehicle constraints, wheel abstractions, engines, transmissions, differentials and anti-roll bars. Those primitives are useful, but selecting Jolt does not itself add an easy-to-author ARUT car node. The Godot-facing API may not expose every upstream-specific feature.

- https://github.com/jrouwe/JoltPhysics
- https://github.com/godot-jolt/godot-jolt
- https://docs.godotengine.org/en/4.7/tutorials/physics/index.html

### 3. Godot Physics — built-in compatibility baseline
Already integrated with standard Godot body, shape, area, joint and query nodes. This is the lowest-friction fallback while alternate backends are investigated. A better C++ node layer can still add useful vehicle/character behavior above it.

### 4. Bullet — mature C++ alternative
Bullet is a widely used real-time C++ collision/rigid-body SDK with a permissive zlib license. Being open source does not make it a drop-in Godot 4.7 backend; a maintained PhysicsServer adapter and platform builds would still be required.

- https://github.com/bulletphysics/bullet3

### 5. ReactPhysics3D — smaller focused library
A C++ library with rigid-body dynamics, common collision shapes, joints, raycasts and sleeping, released under the zlib license. It could suit a focused subsystem or reference implementation, but is not equivalent to a complete Godot-ready backend.

- https://www.reactphysics3d.com/

### 6. Project Chrono — vehicle-dynamics research
A substantial C++ multibody/multiphysics toolkit useful for high-fidelity vehicle/tire/suspension research or a desktop tuning tool. It is probably too much integration/runtime cost for the first always-on mobile backend.

- https://github.com/projectchrono/chrono

## Evaluation matrix

| Criterion | Required evidence |
|---|---|
| Godot compatibility | Exact source base, adapter completeness and maintenance status |
| Android ARM64 | Compiles and runs CPU physics without desktop-only dependencies |
| Core simulation | Bodies, areas, shapes, queries, joints, sleeping and collision filtering |
| Gameplay | Character controller, ragdoll/physical bones and high-speed collision support |
| Vehicles | Useful wheel/suspension/tire/drivetrain APIs or a feasible native ARUT layer |
| Editor | Standard nodes and Inspector continue to work |
| Cost | CPU, memory, threading, contacts and binary size on mobile |
| Build | Reproducible dependency downloads and pinned revisions |
| License | Exact version and nested third-party notices verified |

## Decision path
1. Inspect the existing Godot PhysX 5 module work before assuming Jolt is the winner.
2. Keep Godot Physics and integrated Jolt as comparison/fallback options during that inspection.
3. Choose one primary backend and at most one maintained alternative after Android buildability and feature coverage are understood. Do not ship several full solvers by default.
4. Design ARUT components against standard Godot body/force/query APIs wherever possible. Declare backend requirements for specialised features.
5. Run internal cloud builds at sensible integration milestones; do not ask the user to install an APK after every feature.

## Native C++ Car3D node: first-pass source and planned evolution
- Appears in the Create Node dialog under an ARUT Vehicles category.
- Creates/manages its basic four wheel probes and suspension setup.
- Exposes wheelbase/track, radius, suspension travel/stiffness/damping, steering, drive layout, engine force/torque, service brake, handbrake, tire grip and control actions in the Inspector.
- Includes front/rear/all-wheel drive and compact/sedan/off-road presets.
- Uses an inexpensive initial suspension/tire-friction model through standard engine physics APIs.
- Exposes native C++ controls for player input/AI and telemetry so controller logic need not be written in GDScript.
- Later grows to wheel angular speed, longitudinal/lateral slip, combined grip limits, torque curve, gearbox, differential, ABS/TCS and specialised backend vehicle APIs.

This is the implementation target, not a claim that the node already exists.

## Other planned native components
- `ArutCharacter3D`: FPS/third-person movement, acceleration, ground friction, steps/slopes, air control and jumping.
- `ArutInteractable3D`: mass/friction-aware pushes, pickup/throw hooks and impact feedback.
- `ArutBreakable3D`: damage thresholds, visual states and nearby-only debris physics.
- `ArutRagdollProfile`: selective physical-bone/ragdoll setup.
- `ArutPhysicsMaterial`: accessible friction, bounce, density and surface tags.
- Physics/vehicle panel: backend, body/contact counts, timing and per-car telemetry.

No backend is selected or integrated by this design document.
