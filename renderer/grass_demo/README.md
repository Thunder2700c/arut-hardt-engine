# ARUT-HARDT Engine — Mobile Grass/Foliage Renderer Prototype

## Product ownership and architectural status

This is a first-party **ARUT-HARDT Engine renderer feature prototype**. ARUT-HARDT Engine is the product; Godot 4.7-stable is the technical foundation whose spatial-shader and rendering APIs this prototype targets.

The `grass_blade.gdshader` resource is authored for the engine's rendering workstream. The accompanying `project.godot` and `main.tscn` exist only as a small, reproducible **validation harness** so the shader and instance data can be inspected in isolation. That harness is not the engine product, a game project, a Godot add-on/plugin, a GDExtension, or the intended distribution architecture. Its presence does not mean the renderer feature has already been integrated into the ARUT-HARDT editor packaging/build pipeline.

This scoped shader milestone intentionally does not introduce a native C++ class: the task is the GPU material/vertex-shader prototype, and Godot's existing MultiMesh and shader-resource interfaces are sufficient to explore it. No GDScript is used or required; there are no `.gd` files, script-based runtime controllers, or hidden script dependencies. A later first-class foliage component or editor workflow should be added only if the engine architecture demonstrates a real need for native management/authoring beyond these existing primitives.

## Validate with ARUT-HARDT Engine

1. Open the `renderer/grass_demo` validation-harness directory as a project in an **ARUT-HARDT Engine editor build** based on the pinned Godot 4.7-stable source.
2. Let the editor import the resources, then run the main scene (`F6`) or project (`F5`).
3. Select `GrassPatch` in the scene tree. Its `Material Override` references the first-party `ShaderMaterial`; tune the shader parameters in the Inspector.

Using an upstream Godot 4.7 editor may help check upstream resource/shader compatibility, but it alone does not prove integration, packaging, or runtime correctness in ARUT-HARDT Engine. The project selects the Mobile renderer as its intended prototype target; this is not proof that every Android GPU or driver supports it or that this scene has been run on a device.

## Shader controls

| Parameter | Purpose |
| --- | --- |
| `wind_strength` | Maximum vertex displacement in scene units; set to `0` for still grass. |
| `wind_speed` | Oscillation rate used by the time-driven wind phase. |
| `spatial_variation` | Changes the phase across world-space position so the field does not sway as one rigid sheet. |
| `wind_direction` | Shared wind direction in world X/Z; normalized in the shader. |
| `blade_width` / `tip_width` | Controls the procedural cutout profile from base to tip. |
| `base_color` / `tip_color` | First-party grass color gradient. |
| `blade_roughness` | PBR roughness for the blade surface. |

## Geometry, cutout and instancing

- The validation scene contains one `MultiMeshInstance3D` patch with **529** quad instances, deterministic placement/yaw, and per-instance height, width, phase and color variation stored in the MultiMesh buffer.
- The shader uses `ALPHA` with `ALPHA_SCISSOR_THRESHOLD`, not smooth alpha blending, and requests opaque depth drawing. The silhouette is generated from `UV`; there is no texture dependency.
- Wind is evaluated in the vertex stage. UVs are retained for the blade profile, and normals are adjusted approximately for the bend so the cards do not keep a completely rigid lighting response.
- A single MultiMesh does not provide individual per-blade frustum culling. For larger scenes, split foliage into spatial patches (a reasonable prototype starting point is roughly **8–16 m** per patch), reduce instance counts with distance, and use simpler/different LOD meshes or density tiers for far patches. Tune patch sizes and counts through profiling rather than assuming one universal budget.
- Cutout edges may alias at small screen sizes. Alpha-to-coverage/MSAA could be evaluated as an optional future quality path, but this prototype deliberately stays with a simple alpha-scissor fallback.
- Wind deformation affects rendered vertices; it does not create collision, physical bending, interaction or a foliage-simulation system. The validation scene has no runtime scripts.

## Compatibility and validation

The shader uses spatial-shader built-ins supplied by the pinned engine foundation (`TIME`, `MODEL_MATRIX`, `INSTANCE_CUSTOM`, `UV`, `NORMAL`, `FRONT_FACING`) and the standard Mobile-compatible material path. Per-instance phase and tint are passed from vertex to fragment through varyings because `INSTANCE_CUSTOM` is a vertex-stage built-in. The validation scene stores rigid yaw-only instance transforms; the shader uses the transpose of the model basis to convert its shared world-space wind vector to local space. Revisit that conversion if transforms change to include non-uniform scale or arbitrary rotations.

No FPS, overdraw, thermal, memory, visual-quality or Android device-compatibility claim is made. Shader compilation and runtime rendering still need to be verified in a current ARUT-HARDT Engine editor build. Profile representative device GPUs before adopting larger density budgets.

## Source and license

- The shader, scene, project settings and this documentation are original first-party ARUT-HARDT prototype files covered by the repository's MIT license and its standard notice requirements.
- **No external model, texture, shader, code library or other asset is used.** All foliage geometry comes from an authored QuadMesh instanced through the existing engine resource interface.
- The underlying Godot foundation remains subject to its upstream copyright and license notices; this prototype does not copy or redistribute Godot source.

## Integration boundary

This change intentionally stays within `renderer/grass_demo/`. It does not modify the existing `renderer/demo/` anime-material prototype, add an add-on/plugin or GDExtension, introduce GDScript, or change shared build workflows, module registration, roadmap or status files. Chat 5 owns any later decision about engine-workflow packaging or shared integration. The validation harness should not be mistaken for completion of that integration.
