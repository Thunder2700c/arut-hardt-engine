# ARUT-HARDT Engine

A Godot-based C++-first engine project focused on approachable native components, an original editor experience, configurable physics backends and mobile-aware rendering.

## One engine, two editions
- **ARUT Mobile:** Android-first editor/runtime and mobile authoring workflows.
- **HARDT Studio:** desktop editor distribution, initially Linux x86_64.

Both share the same core, APIs and project format. They are not separate engines.

## Product focus
ARUT-HARDT is being built as an engine before it is used to develop the user's game. The initial native source batch includes C++ `ArutCar3D`, `ArutCharacter3D`, `ArutRagdollDummy3D`, and `ArutRenderProfile3D` classes. The character generates a starter mannequin/collision/camera, supports basic desktop/touch movement, and can hand off to a connected procedural ragdoll. The render node applies named viewport scaling/AA/occlusion/LOD profiles. This code is not compiled or runtime-verified yet; the ragdoll still uses the currently selected Godot physics backend, not an independent ARUT solver.

The initial intended audience is a private group of five people. The current priority is functional engine features and usability—not a public launch.

## Architecture and status
The project currently uses a source-build overlay that fetches a pinned Godot 4.7 source tree at build time, applies reviewed patches, builds native code, and packages editor artifacts. It is an early foundation, not yet a complete maintained source fork or custom renderer.

Read [DEVELOPMENT_PROTOCOL.md](DEVELOPMENT_PROTOCOL.md), [ROADMAP.md](ROADMAP.md), [docs/ENGINE_COMPONENT_CATALOG.md](docs/ENGINE_COMPONENT_CATALOG.md), [docs/EDITOR_UX_ARCHITECTURE.md](docs/EDITOR_UX_ARCHITECTURE.md), [docs/PHYSICS_ARCHITECTURE.md](docs/PHYSICS_ARCHITECTURE.md), and [docs/RENDERING_ARCHITECTURE.md](docs/RENDERING_ARCHITECTURE.md) before continuing work.

## Repository boundaries
This repository must not contain PEAK game assets or game-specific files. The game remains separate. Keep required Godot/third-party notices for any dependencies actually included; see [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
