# ARUT-HARDT Rendering Architecture

## Current baseline

The current engine source build starts from Godot 4.7 and its existing mobile renderer. The first objective is to add useful native rendering controls and authoring conventions without pretending to have implemented a wholly new GPU renderer.

## Native quality-profile node

`ArutRenderProfile3D` is implemented in C++ under `engine/modules/arut/`. Add it once to a scene to apply a named profile to its viewport. Current presets:

| Option | Mobile | Balanced | High |
|---|---:|---:|---:|
| 3D render scale | 0.80 | 0.90 | 1.00 |
| Scaling | FSR | FSR | Bilinear |
| 3D MSAA | Off | 2x | Off |
| Screen-space AA | FXAA | Off | Off |
| Temporal AA | Off | Off | On |
| Occlusion culling | On | On | On |
| Mesh LOD threshold | 1.5 px | 1.0 px | 0.5 px |
| Shadow atlas | 1024 | 2048 | 4096 |
| Debanding | On | On | On |

These are starting presets, not measured performance guarantees. Some viewport options depend on the renderer, driver and platform; particularly advanced scaling and TAA options must be checked on the final device. The node's Inspector can override values after choosing a preset. Set the preset to Custom before making a set of manual adjustments if you want to communicate that intention clearly.

### Runtime profile switching (native C++)

`ArutRenderProfile3D` can update its active viewport when profile properties change at runtime while the node is inside the scene tree. Automatic application is suppressed while editing scenes in the editor.

- `quality_apply_on_ready` controls initial application when the node becomes ready.
- `quality_apply_on_change` (enabled by default) controls automatic application of subsequent property and preset changes at runtime.
- `apply_to_current_viewport()` is an explicit native API for a C++ quality/settings controller. It applies the current values regardless of the two automatic-apply switches and safely does nothing if the node has no viewport.
- Scene properties are loaded before the node enters the tree, so deserializing a saved profile does not trigger partial applications during scene loading.

This is a viewport-profile feature, not a new rendering backend. It uses supported Godot viewport controls and does not claim new GPU effects, a Nanite replacement, or measured performance gains.

## Mobile-friendly optimization strategy

### Implemented in source (not build-verified)
- Named viewport profiles with internal C++ application.
- FSR/render-scale, anti-aliasing, occlusion-culling, mesh-LOD, texture-mipmap-bias, debanding and positional shadow atlas controls exposed in the Inspector.
- A project template that includes the Mobile profile.

### Next source-level systems
1. **Automatic LOD/visibility budgets:** derive quality distances from profile, provide reusable visibility/LOD authoring helpers and document mesh LOD generation.
2. **Occlusion and instancing workflow:** provide clear setup examples and editor diagnostics; runtime occlusion culling only helps when occluders are actually authored.
3. **Material authoring:** a low-cost stylized/PBR hybrid, consistent normal/AO/UV/triplanar semantics, per-profile texture/shader controls and optional outlines.
4. **Lighting and environment presets:** shadow distance/quality, fog and sky defaults, ambient fill, light-count guidance.
5. **Diagnostics:** frame-time/CPU/GPU approximation where available, draw calls, primitive counts, memory and visible object statistics.
6. **A deeper renderer fork only when justified:** add reviewed render-device/rendering-server patches after the baseline is measurable. Keep patches pinned to the exact Godot source revision.

## About Nanite-like virtualized geometry

Nanite is not a single mesh-LOD toggle. It is a specialised virtualized-geometry rendering and streaming architecture. Do not advertise ARUT as having Nanite from ordinary LOD or occlusion culling.

The practical mobile path is generated mesh LODs, occlusion culling, visibility ranges, instancing/multi-mesh, streaming/texture budgets, shadow limits, resolution scaling and a renderer that suits the GPU. A true virtualized-geometry system would be a separate major renderer project with GPU-driven visibility, clustered/virtualized mesh storage, page/cluster streaming, material integration, editor tooling and device-specific validation.

## About GASP and animation

Epic's Game Animation Sample Project (GASP) is an animation/motion-matching sample, not a physics solver. ARUT can later build native C++ animation state/locomotion, motion matching or animation-to-ragdoll blending as separate systems. The present character/ragdoll lab is a passive physical ragdoll and has no animation recovery, motion matching or active muscle controller.

## Verification truth

The baseline C++ profile compiled in the successful Android editor workflow at commit `158ea1efb2c897d52adb8f676376f59bbfc7aec5`. The live-switching addition is source-reviewed but has not yet been compiled in a new workflow run or exercised on Android. Device testing is still required to validate renderer-option support and actual performance. No Nanite replacement or custom GPU renderer is claimed.
