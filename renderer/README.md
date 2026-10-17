# Renderer workspace

The first prototype lives at:
- `demo/project.godot`
- `demo/main.tscn`
- `demo/main.gd`
- `demo/shaders/arut_anime_surface.gdshader`

The scene procedurally creates its own checker texture, ground plane, sphere, boxes, camera, light and environment. It has no dependency on PEAK's assets or scene tree.

The shader exposes:
- UV or world-space triplanar projection;
- explicit texture scale (default 1.0 for the current world projection);
- optional normal map and AO inputs;
- roughness/specular controls and a restrained rim option;
- toon diffuse lighting without requiring SDFGI/SSAO/SSR.

Status: **written but not compiled or visually tested**. Do not attach it to the production PEAK Landmass yet. First open the demo in a matching Godot 4.7 editor, resolve compiler issues, compare texture density at scale 1.0 versus 0.1, and check performance.

The normal-map triplanar basis is an initial approximation and requires visual validation against the original StandardMaterial3D before we claim a faithful replacement.
