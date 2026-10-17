# ARUT-HARDT Physics Architecture

## Goal

Build convincing, responsive, mobile-friendly game physics inspired by the feel of GTA-style vehicles and modern military FPS movement. These are design references, not a promise of bit-for-bit replication. No proprietary engine code or data is required.

## What the current PEAK project already does

Inspection of the public PEAK repository found:

- `project.godot` selects the built-in `Jolt Physics` engine.
- `vehicles/car.tscn` uses a 1200 kg `RigidBody3D`, a chassis box collider, and four `RayCast3D` wheel probes.
- `vehicles/VehicleController.gd` applies spring and damping forces at each wheel probe, a simple lateral-grip force, and a throttle force.
- The existing car controller does not yet implement wheel angular speed, longitudinal tire slip, slip-angle response, a drivetrain/power curve, selectable gears, a differential, ABS/traction control, or a calibrated brake/handbrake model.
- The player uses `CharacterBody3D`, which is appropriate for a responsive, controller-driven FPS character; it should not be converted into a free rigid body just to make movement "more realistic".

Therefore, do not replace Jolt first. Jolt is already configured and is suitable for general rigid-body collision, joints/constraints, and character-adjacent physical interactions. Realism depends on the game-specific vehicle and movement models, collision shapes, contact material tuning, animation and feedback, not the engine name alone.

## Open-source projects worth using responsibly

### 1. Jolt Physics — default general-purpose solver
Repository: https://github.com/jrouwe/JoltPhysics
License: MIT.
Already integrated into Godot 4.4+ as a built-in engine module, and selected by the current PEAK project. Use it for rigid-body simulation, collision queries, constraints, and the physical response of props. Avoid embedding a second full physics solver unless a measured requirement demands it.

### 2. Godot Advanced Vehicle — vehicle-controller reference
Repository: https://github.com/Dechode/Godot-Advanced-Vehicle
License: MIT (the repository notes that its included models, textures and sounds have their own licenses).
Its controller demonstrates raycast suspension, Pacejka/brush-style tire models, tire wear, drivetrain choices, differentials, clutch and gears. This is closer to the vehicle system PEAK needs than swapping out Jolt. Review and port only code that is needed, preserve its attribution/license, and exclude its demo assets unless their separate licenses are verified.
Caution: its README recommends a 240 Hz physics setting for its own demo. Do not adopt that default for mobile. First create a reduced model and benchmark at the engine's normal physics rate, then compare 60/90/120 Hz only where supported and useful.

### 3. Project Chrono — advanced vehicle-dynamics reference
Repository: https://github.com/projectchrono/chrono
License: BSD-3-Clause.
Chrono is a serious multibody/multiphysics C++ framework and includes vehicle-dynamics capabilities and tire/suspension models. It is worth studying for high-fidelity simulation or a desktop vehicle-tuning tool. Its scope and integration cost make embedding all of Chrono into an Android open-world game a poor first move. Keep it as an optional later experiment, not the default mobile physics core.

## Physics architecture for the engine

### Layer A — rigid-body world
- Keep the Godot-integrated Jolt backend for collisions, rigid bodies and constraints.
- Define consistent mass, friction, bounce and collision-layer conventions.
- Use primitive/convex collision shapes for moving bodies when possible; do not use detailed render meshes as every moving object's collider.
- Add high-speed collision handling only for the bodies that need it, as continuous collision detection costs more.

### Layer B — vehicle dynamics (C++)
Create an `ArutVehicleBody` / vehicle component initially as a C++ GDExtension. Promote it to a Godot engine module only if profiling or required low-level hooks prove GDExtension insufficient.

Minimum planned features:
1. Four-corner raycast suspension with spring force, damper velocity, travel limits and force applied at each contact point.
2. Tire contact model with longitudinal slip and lateral slip angle. Start with a stable brush-inspired / saturating friction model; keep Pacejka-style curves optional.
3. Combined grip limit (friction-circle/ellipse approximation) so longitudinal and lateral forces cannot both exceed available contact grip.
4. Wheel angular speed, drive/brake torque, rolling resistance and tire radius.
5. Engine torque-vs-RPM curve, gears, final-drive ratio and drivetrain mode.
6. FWD/RWD/AWD distribution and configurable open/limited-slip/locked differential behavior.
7. Distinct service brake and handbrake behaviour; optional ABS and traction control.
8. Steering response by speed and approximate Ackermann geometry for front wheels.
9. Tunable centre of mass, suspension travel, anti-roll coupling and tire/surface friction.
10. Telemetry overlay in test scenes: speed, RPM, gear, wheel contact, suspension travel, slip and frame time.

The first milestone is one car driving over a flat surface, a low-speed curb, a bump and a turn without jittering, flipping randomly or requiring a 240 Hz simulation.

### Layer C — first-person/action movement
- Retain `CharacterBody3D` for player movement and use a tuned acceleration/deceleration model, ground friction, slope/step handling, air control and jump/gravity settings.
- Keep responsive input and camera feedback distinct from physical mass simulation; feel matters more than making the FPS character a rigid body.
- Use animation/pose systems and hit-reaction impulses for combat feedback.
- Use ragdoll/physical-bone simulation selectively for enemies/deaths, not for every character continuously.
- Add mass/friction-based prop interactions and impact reactions only after basic character movement is stable.

### Layer D — breakable/interactive props (later)
- Use pooled debris, lightweight fracture proxies, and thresholds for breakable items.
- Keep full rigid-body debris for near-field events only; distant destruction can use prebuilt visual swaps.
- Don't begin city-wide destruction or soft-body simulation until the base vehicle/character physics is benchmarked.

## Performance policy for mobile

- Use one solver (Jolt) as the default.
- Start with Godot's standard fixed physics tick rate; do not jump straight to 240 ticks per second because an example project recommends it.
- Use distance-based activation/sleeping and avoid simulating off-screen/distant debris and abandoned vehicles unnecessarily.
- Profile in release-like builds on the actual Xclipse 920 target.
- Record physics frame time, main-thread frame time, collision body count and worst-case contact/vehicle count.
- Prefer predictable and tunable physics over maximum mathematical fidelity. Warzone-like character feel and GTA-like vehicle handling need carefully authored systems and tuning, not simply more simulation steps.

## Proprietary engines and reverse engineering

Do not use decompiled, leaked, or otherwise unauthorized Unity, Unreal, IW/Call of Duty, RAGE or GTA engine source/code as a source to copy into ARUT-HARDT.

- Unity's currently published Editor Source Code Terms restrict using Unity source to create a competing engine or replicate Unity features, and restrict providing that source to coding assistants/AI agents. Use independent implementations and genuinely open-source components instead.
- Unreal Engine source is available under Epic's license, not a blanket OSI open-source license allowing its engine internals to be transplanted into an unrelated engine. Follow the applicable EULA and distribution restrictions.
- IW/RAGE are proprietary. Tools that inspect game files or reverse engineer formats are not equivalent to reusable source implementations; assets and file formats may also be protected by separate terms.

The allowed and practical strategy is to reproduce general techniques from public technical explanations, then implement them independently or reuse code under a compatible license (such as MIT/BSD). Preserve attribution and include all required notices.

## Validation sequence

1. Create a standalone vehicle test scene separate from PEAK.
2. Capture baseline behaviour and frame time for the current controller.
3. Implement one feature at a time, beginning with force calculation and tire grip.
4. Test normal 60 Hz physics before any higher tick rate.
5. Add drivetrain, gears, differentials and assists after handling is stable.
6. Add an action-movement test scene for player acceleration, slope handling, jump, air control and impact response.
7. Only request the user's combined APK/phone test after the engine's first native + visual + physics slice is ready.

No physics library or vehicle controller has yet been integrated into ARUT-HARDT by this document. It is the reviewed plan, not a claim of completed implementation.
