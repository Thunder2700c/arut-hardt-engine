# ARUT-HARDT Engine — Grass/Foliage Shader Prototype

An isolated, first-party Godot 4.7 Mobile-renderer prototype for cutout grass, vertex wind and MultiMesh instancing. This is a renderer feature demo, not a playable game and not a native engine module.

## Open and run

1. Open `renderer/grass_demo/project.godot` with Godot **4.7-stable**.
2. Let Godot import the project, then run the main scene (`F6`) or project (`F5`).
3. Select `GrassPatch` in the scene tree. Its `Material Override` points at the `ShaderMaterial`; edit `Shader Parameters` to tune wind and the blade silhouette.

The project explicitly selects the Mobile renderer. This is a renderer target selection, not proof that every Android GPU/driver supports a particular rendering API or that this scene has been tested on a device.

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

- The scene contains one `MultiMeshInstance3D` patch with **529** quad instances, deterministic placement/yaw, and per-instance height, width, phase and color variation stored in its MultiMesh buffer.
- The shader uses `ALPHA` with `ALPHA_SCISSOR_THRESHOLD`, not smooth alpha blending, and requests opaque depth drawing. The silhouette is generated from `UV`; there is no texture dependency.
- Wind is evaluated in the vertex stage. UVs are retained for the blade profile, and normals are adjusted approximately for the bend so the cards do not keep a completely rigid lighting response.
- A single MultiMesh does not provide individual per-blade frustum culling. For larger scenes, split foliage into spatial patches (a reasonable prototype starting point is roughly **8–16 m** per patch), reduce instance counts with distance, and use simpler/different LOD meshes or density tiers for far patches. Tune patch sizes and counts through profiling rather than assuming one universal budget.
- Cutout edges may alias at small screen sizes. Alpha-to-coverage/MSAA could be evaluated as an optional future quality path, but this prototype deliberately stays with a simple alpha-scissor fallback.
- Wind deformation affects rendering vertices; it does not create collision, physical bending, interaction or a foliage simulation system. This scene has no runtime controls or scripts.

## Compatibility and validation

The shader uses Godot spatial-shader built-ins (`TIME`, `MODEL_MATRIX`, `INSTANCE_CUSTOM`, `UV`, `NORMAL`, `FRONT_FACING`) and the standard Mobile-compatible material path. Per-instance phase and tint are passed from vertex to fragment via varyings because `INSTANCE_CUSTOM` is a vertex-stage built-in. The static instance transforms use rigid yaw rotations; the shader uses the transpose of the model basis to convert its shared wind vector to local space. If the transform setup is changed to non-uniform scale or arbitrary rotations, revisit that conversion.

No FPS, overdraw, thermal, memory, visual quality or Android device-compatibility claim is made. Validate the project in the pinned Godot 4.7 editor, inspect the scene on the Mobile and Compatibility renderers as applicable, and profile representative device GPUs before adopting larger budgets.

## Source and license

- Shader, scene, project settings and this documentation are original first-party ARUT-HARDT prototype files; they are covered by the repository's MIT license and its standard notice requirements.
- **No external model, texture, shader, code library or other asset is used** in this demo. All foliage geometry is a QuadMesh instanced from authored scene data.
- The underlying Godot engine remains subject to its own upstream copyright and license notices; this prototype does not copy or redistribute Godot source.

## Scope boundary

This prototype intentionally does not modify `renderer/demo/`, anime material semantics, shared workflows, module registration, `ROADMAP.md`, or `docs/CURRENT_STATUS.md`. Chat 5 can decide later whether and how to package this directory in a larger demo/integration milestone.