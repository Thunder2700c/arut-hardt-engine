# Vulkan Renderer Bring-up

Status: the source now contains a mobile forward PBR path, real base-colour texture upload, CPU frustum culling, instanced geometry, a directional shadow pass and optional GPU timestamps. CI compiles the host targets and Android ARM64 native library without packaging an APK.

## Frame setup
1. Create Android Vulkan instance/surface, choose graphics and presentation queues, and create the FIFO swapchain.
2. Create colour image views, depth attachment, render pass and swapchain framebuffers.
3. Decode app/src/main/assets/textures/demo_base_color.png into RGBA8.
4. Generate sRGB-aware mip levels on CPU and upload them through a host-visible staging buffer into an optimal tiled sampled image.
5. Create fallback sampled resources for normal/metallic-roughness maps and, when needed, an all-lit depth fallback.
6. Load PBR and shadow SPIR-V shader assets from Android assets.
7. Create frame/material descriptor layouts, PBR and shadow pipelines, and frame/material uniform buffers.
8. Generate an indexed unit cube and upload mesh buffers.
9. Compute a camera frustum, cull the sample instances with bounding spheres and upload visible model matrices to an instance-rate vertex stream.

## Per-frame commands
1. Reset and begin the command buffer.
2. If a sampled depth format is also depth-attachment capable, clear and render the directional shadow map using the shared indexed instanced mesh.
3. Transition the shadow map to depth-read-only layout at the end of the shadow render pass.
4. Begin the main colour/depth pass, bind the PBR pipeline, frame/material descriptor sets, mesh and visible instance buffers, and issue an indexed instanced draw.
5. Submit and present. Optional Vulkan timestamp queries measure total GPU frame time and shadow-pass GPU time separately.

## Shader contracts
- PBR vertex locations 0–3: position, normal, tangent, UV.
- PBR vertex locations 4–7: four columns of the per-instance model matrix.
- Frame set 0 binding 0: view-projection, camera position, directional-light direction/radiance and light view-projection matrices.
- Frame set 0 binding 1: comparison-sampled directional shadow map.
- Material set 1 binding 0: material factors/flags; bindings 1–3: base colour, normal, metallic-roughness sampled images.
- Shadow vertex stage uses frame set 0 binding 0 and vertex locations 0, 4–7.

## Visibility and quality
CPU culling currently uses Vulkan zero-to-one clip-space frustum planes and bounding spheres. One 1024x1024 directional shadow map is the current default; cascades and shadow-resolution selection from Low/Medium/High/Ultra presets are not yet connected. GPU/indirect culling is pending.

## Diagnostics
Runtime profile logs include CPU frame duration, GPU timestamp duration when available, shadow-pass duration, draw calls, visible instances and triangle count. Unsupported timestamp queries are gracefully disabled. GPU memory totals and detailed pass breakdowns remain pending.

## Limits
- The scene is a small renderer test fixture, not a game world.
- Only the base-colour map uses a real image asset; normal and metallic-roughness maps use fallback textures.
- IBL uses a temporary ambient term; environment probes, irradiance, prefiltered specular maps and BRDF LUT are not implemented.
- No APK, emulator or physical-device testing is performed by normal CI. The manual Android APK workflow remains separately gated.
- Android GPU visual correctness and measured costs remain unverified until the user authorizes testing.
