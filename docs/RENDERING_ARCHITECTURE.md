# ARUT-HARDT Rendering Architecture

## Current baseline

The engine source build starts from Godot 4.7 and its existing renderer/resource/import pipeline. ARUT adds a native C++ control layer first; it does not claim to be a replacement renderer or to reproduce a proprietary game's full rendering stack.

The Android-first target is a mobile stylized-PBR/anime hybrid. Keep Godot's Mobile renderer for Android initially, retain Compatibility as a lower-feature fallback, and evaluate Forward+ for desktop profiles. A deeper renderer fork must be justified by measured gaps, not by feature-list comparisons alone.

## Native quality-profile node

`ArutRenderProfile3D` is a built-in C++ node in `engine/modules/arut/`. Add it once to a scene to apply a named profile to its viewport. It exposes quality settings to the Inspector and can also be controlled by native C++ editor tools or gameplay code.

| Option | Mobile | Balanced | High |
|---|---:|---:|---:|
| 3D render scale | 0.80 | 0.90 | 1.00 |
| Scaling | Bilinear | Bilinear | Bilinear |
| 3D MSAA | Off | 2x | Off |
| Screen-space AA | FXAA | Off | Off |
| Temporal AA | Off | Off | On |
| Occlusion culling | On | On | On |
| Mesh LOD threshold | 1.5 px | 1.0 px | 0.5 px |
| Anisotropic filtering | 4x | 8x | 16x |
| Shadow atlas | 1024 | 2048 | 4096 |
| Debanding | On | On | On |

These are starting profiles, not measured performance guarantees. Quality presets expose settings, not magic performance multipliers: geometry, materials, lights, transparency, shader complexity, asset sizes and the device driver still determine actual frame time and visual quality.

### Runtime profile switching (native C++)

`ArutRenderProfile3D` applies its initial profile when the node becomes ready if `quality_apply_on_ready` is enabled. Once the node is active in the scene tree, automatic updates to later profile/preset changes are controlled separately by `quality_apply_on_change`, which defaults to enabled. Automatic application is suppressed in the editor so changing Inspector values while authoring a scene does not unintentionally apply the profile to the editor viewport.

`apply_to_current_viewport()` is an explicit native API for a C++ quality/settings controller. It applies the current values regardless of the automatic flags and safely does nothing when the node has no viewport. The existing `apply_now()` method remains as a backward-compatible alias. Scene properties are loaded before the node enters the tree, so deserializing a saved profile does not trigger partial application during scene loading.

### Renderer-aware upscaling

Godot 4.7 does not support every viewport upscaling mode on every renderer. In particular, the Mobile renderer falls back to bilinear scaling for FSR/temporal upscalers, and the Compatibility renderer cannot provide FSR/MetalFX upscaling. MetalFX is specific to supported Apple rendering devices. Selecting one of these unsupported modes must not be presented as if ARUT had delivered that technique.

The native profile therefore:
- uses bilinear scaling in its built-in Mobile/Balanced presets, so its default profile does not request a mode the Android Mobile renderer silently downgrades;
- calculates an effective scaling mode for the known renderer/platform compatibility cases before applying the setting;
- exposes read-only `renderer_effective_scaling_mode` and `renderer_scaling_status` Inspector properties so an unsupported request is visible;
- applies the profile on node readiness when `quality_apply_on_ready` is enabled; subsequent changes are controlled independently by `quality_apply_on_change` (enabled by default);
- exposes `apply_to_current_viewport()` as an explicit native API and keeps `apply_now()` as a compatibility alias;
- keeps the user's requested mode visible separately from the effective mode. GPU/driver feature checks can still trigger additional Godot-side fallbacks, particularly for MetalFX; the diagnostic property explains that when ARUT passes a request through.

Bilinear at a render scale below 1.0 still renders fewer 3D pixels and upscales the result, but it does not provide FSR reconstruction/sharpening. Do not advertise FSR quality or performance unless the selected renderer actually supports it.

## Mobile-friendly optimization strategy

### Implemented in source (latest profile changes not yet compiled)

- Named viewport profiles with native C++ application.
- Resolution scaling, renderer-aware scaling fallback/status, 3D MSAA, screen-space AA, TAA, anisotropic filtering, occlusion culling, mesh-LOD threshold, texture-mipmap bias, debanding, FSR sharpness and positional shadow-atlas controls exposed in the Inspector.
- Live application of Inspector/runtime changes while the profile node is inside the scene tree.
- No GDScript dependency is part of this feature.

### Next source-level systems

1. **Automatic LOD/visibility budgets:** derive quality distances from a profile and provide reusable visibility/LOD authoring helpers.
2. **Occlusion and instancing workflow:** provide clear setup options and editor diagnostics; runtime occlusion culling only helps when occluders are authored effectively.
3. **Material authoring:** a low-cost stylized/PBR hybrid, consistent normal/AO/UV/triplanar semantics, per-profile material controls and optional outlines.
4. **Lighting and environment presets:** native authoring conventions for shadow distance/quality, fog, sky, ambient fill and light-count budgets.
5. **Diagnostics:** frame-time/CPU data where available, draw calls, primitive counts, memory and visible-object statistics.
6. **A deeper renderer fork only when justified:** add reviewed render-device/rendering-server C++ patches after the baseline is measurable and a concrete quality/performance gap is established. Keep patches pinned to the exact Godot source revision.

## About Nanite-like virtualized geometry

Nanite is not a single mesh-LOD toggle. It is a specialised virtualized-geometry rendering and streaming architecture. Do not advertise ARUT as having Nanite from ordinary LOD or occlusion culling.

The practical mobile path is generated mesh LODs, occlusion culling, visibility ranges, instancing/MultiMesh, streaming/texture budgets, shadow limits, resolution scaling and a renderer that suits the GPU. A true virtualized-geometry system would be a separate major renderer project with GPU-driven visibility, clustered/virtualized mesh storage, page/cluster streaming, material integration, editor tooling and device-specific validation.

## Style target: anime plus realistic environments

Start with Godot's built-in PBR renderer and a curated ARUT-owned shader/material layer. Anime characters need stable face shadowing, readable hair/skin highlights, tunable toon thresholds and outlines; environments need sensible PBR materials, lighting, atmospheric perspective, shadows and texture/geometry budgets. A shader cannot reproduce a complete art direction by itself.

Do not transplant Google Filament or bgfx into this project as if either were a drop-in Godot renderer. Both would require substantial resource/material/editor/render-graph integration. Any external code or shader pack must be reviewed for exact licensing, supported APIs and Android suitability before inclusion.

## About GASP and animation

Epic's Game Animation Sample Project is an animation/motion-matching reference, not a physics solver or a portable engine module. ARUT can later build native C++ animation state/locomotion, motion matching and animation-to-ragdoll blending as separate systems. The present character/ragdoll lab remains a separate workstream; this rendering feature does not create a playable character demo.

## Verification truth

The renderer-aware profile and runtime profile-switching integration has not yet been compiled in a fresh build, and no device-side behavior or performance result is claimed. The next integration check should compile the ARUT C++ module against the pinned Godot 4.7 source without producing a game/demo package. Device profiling is still needed for frame time, thermal behavior, shader artifacts and actual visual quality. No custom GPU renderer, Nanite replacement, FSR reconstruction on the Mobile renderer, or Endfield-equivalent renderer is claimed.
