# Vulkan Renderer Bring-up

Status: the source contains a mobile forward PBR path, real base-colour texture upload, a GGX-prefiltered procedural environment cubemap, CPU frustum culling, instanced geometry, a directional shadow pass and optional GPU timestamps. CI compiles host targets and the Android ARM64 native library without packaging an APK.

## Frame setup
1. Create Android Vulkan instance/surface, choose graphics/presentation queues and create the FIFO swapchain.
2. Create colour image views, depth attachment, render pass and swapchain framebuffers.
3. Decode app/src/main/assets/textures/demo_base_color.png into RGBA8, generate sRGB-aware CPU mip levels and upload through a staging buffer.
4. Generate a six-face linear-light procedural sky/ground cubemap. GGX importance sampling builds specular roughness mips; upload all mip/face layers into a cube-compatible Vulkan image.
5. Create fallback sampled resources for normal/metallic-roughness maps and, when needed, an all-lit depth fallback.
6. Load PBR and shadow SPIR-V shader assets from Android assets.
7. Create frame/material descriptor layouts, PBR and shadow pipelines, and frame/material uniform buffers.
8. Generate an indexed unit cube and upload mesh buffers.
9. Compute a camera frustum, cull sample instances with bounding spheres, and upload visible model matrices to an instance-rate vertex stream.

## Per-frame commands
1. Reset and begin the command buffer.
2. If a sampled depth format is also depth-attachment capable, clear and render the directional shadow map using the shared indexed instanced mesh.
3. Transition the shadow map to depth-read-only layout at the end of the shadow render pass.
4. Begin the main colour/depth pass, bind PBR pipeline, frame/material descriptor sets, mesh and visible instance buffers, and issue an indexed instanced draw.
5. Submit and present. Optional Vulkan timestamp queries measure total GPU frame time and shadow-pass GPU time separately.

## Shader contracts
- PBR vertex locations 0–3: position, normal, tangent, UV.
- PBR vertex locations 4–7: four columns of the per-instance model matrix.
- Frame set 0 binding 0: view-projection, camera position, directional-light direction/radiance and light view-projection matrices.
- Frame set 0 binding 1: comparison-sampled directional shadow map.
- Frame set 0 binding 2: linear-light environment cubemap with GGX-prefiltered roughness mips.
- Frame set 0 binding 3: 64x64 RGBA8 split-sum GGX BRDF integration LUT (R=scale, G=bias).
- Material set 1 binding 0: material factors/flags; bindings 1–3: base-colour, normal and metallic-roughness sampled images.
- Shadow vertex stage uses frame set 0 binding 0 and vertex locations 0, 4–7.

## Visibility and quality
CPU culling uses Vulkan zero-to-one clip-space frustum planes and bounding spheres. One 1024x1024 directional shadow map is the current default; cascades and shadow-resolution selection from Low/Medium/High/Ultra presets are not yet connected. Diffuse environment lighting is approximated from the broadest environment mip. Specular IBL uses GGX-prefiltered environment mips and the uploaded split-sum BRDF LUT. Cosine-convolved diffuse irradiance and HDR environment import remain future refinements. GPU/indirect culling is pending.

## Diagnostics
Runtime profile logs include CPU frame duration, GPU timestamp duration when available, shadow-pass duration, draw calls, visible instances and triangle count. Unsupported timestamp queries are gracefully disabled. GPU memory totals and detailed per-material breakdowns remain pending.

## Limits
- The scene is a small renderer test fixture, not a game world.
- Only the base-colour map uses a real image asset; normal and metallic-roughness maps use fallback textures. The split-sum BRDF LUT is procedurally generated on CPU and uploaded as an RGBA8 lookup texture.
- The procedural cubemap is a bounded bootstrap environment, not an HDR environment asset or probe system.
- The shadow map is a single directional map; no cascades or local-light shadows yet.
- No APK, emulator or physical-device testing is performed by normal CI. The manual Android APK workflow remains separately gated.
- Android GPU visual correctness and measured costs remain unverified until the user authorizes testing.
