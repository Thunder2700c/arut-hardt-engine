# ARUT-HARDT Native Component Catalog

This catalog describes the first native C++ systems planned for the engine. It is not a claim that every listed class is already implemented.

## First component: ArutCar3D

**Authoring goal:** add one Car3D node, assign the body mesh and chassis collision shape, choose a preset, and tune only the properties needed for the vehicle.

### Basic inspector groups
- **Vehicle Setup:** preset, mass, center of mass, wheelbase, track width and wheel radius.
- **Suspension:** rest length/travel, spring stiffness, damping and anti-roll tuning.
- **Drive:** front/rear/all-wheel drive, engine force/torque, power curve, transmission, gears and differential.
- **Steering:** maximum steering angle, speed-sensitive steering and response.
- **Brakes:** service brake, handbrake, optional ABS and traction control.
- **Tires:** surface grip, rolling resistance and optional slip curves.
- **Controls:** named input actions, player/AI control mode, throttle/brake/steer/handbrake C++ setters.
- **Debug:** speed, wheel contact, suspension compression, slip, RPM/gear and physics cost.

### Default setup
- Generate/manage four wheel probes internally instead of requiring the user to add and position four RayCast nodes manually.
- Require a chassis collision shape for collisions and a visual mesh for appearance; don't create meaningless default meshes that users must delete.
- Provide compact, sedan and off-road presets as editable starting points.
- Use safe, inexpensive standard-API suspension and tire-force behavior first; deeper drivetrain fidelity can be added without changing the scene authoring pattern.

### Implementation layers
- Car configuration/property API and editor exposure.
- Wheel contact queries and spring/damper response.
- Tire lateral grip and longitudinal drive/braking forces with a combined grip cap.
- Wheel speed and torque/power curve.
- Gearbox/differential/drive-layout behavior and ABS/TCS.
- Backend-specific optimizations only where the selected solver provides a clear benefit.

## Follow-on native components

| Node / resource | Purpose |
|---|---|
| `ArutCharacter3D` | Responsive FPS/third-person movement with acceleration, friction, slope/step handling, air control and jumping |
| `ArutCameraRig3D` | Camera follow, body/head motion, camera collision, recoil and configurable lag |
| `ArutInteractable3D` | Consistent interaction, push/pickup/throw hooks and mass/friction-aware feedback |
| `ArutBreakable3D` | Damage thresholds, visual state changes and selective nearby debris |
| `ArutRagdollProfile` | Reusable physical-bone and ragdoll configuration |
| `ArutPhysicsMaterial` | Friendly surface presets for friction, bounce, density and sound/FX tags |
| `ArutDestructionProfile` | Reusable fracture/debris settings with mobile-friendly limits |
| `ArutPhysicsDiagnostics` | Body/contact counts, step time and optional per-component telemetry |
| `ArutQualityProfile` | Named Mobile/Balanced/High rendering and simulation settings |

## C++-first contract
Core behavior belongs in native C++ classes or engine modules. Properties should be editable from the Inspector. Gameplay projects should not require the user to write GDScript just to get a basic car or character moving. Native control methods must remain available for more advanced users and AI systems.


## Implementation status (2026-10-17)

The first `engine/modules/arut/ArutCar3D` source has been added to the engine repository branch. It currently provides editable geometry/suspension/drive/tire/control properties, three presets, auto-managed four-corner raycast probes and a simple native spring/damper plus bounded tire-force model. Its source has not yet been compiled or run. Visual wheel meshes/rotation, a torque curve, reverse-gear model, detailed slip curves, differential, ABS/TCS, and a dedicated debug panel are not implemented yet.


## ArutCharacter3D first-pass source

A built-in C++ `ArutCharacter3D` class has also been added to the ARUT module. It exposes walk/sprint speed, ground acceleration/friction, air acceleration/control, jump velocity, gravity and action mappings; native setter methods allow touch input and AI integration without GDScript. This source is not yet compiled or runtime-verified. Camera rig, animation, input buffering, crouch, ledge handling and polished slope/step defaults remain future work.
