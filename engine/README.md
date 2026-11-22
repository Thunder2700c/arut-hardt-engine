# Engine source layout

- core/: shared native primitives, diagnostics, handles, memory, and scheduling interfaces.
- platform/: Android and desktop platform adapters.
- bridge/: versioned C ABI between C++ and Rust.
- rendering/: Vulkan renderer and GPU resources.
- world/: coordinates, cells, streaming, and environment.
- physics/: backend adapter, vehicle dynamics, and collision.
- animation/: skeletons, animation graph, and IK.
- ai/: navigation and simulation agents.
- audio/: spatial audio and streaming.
- gameplay/: engine-facing gameplay APIs.

Only core/version bootstrap code is implemented. Other systems are architectural areas, not completed modules.
