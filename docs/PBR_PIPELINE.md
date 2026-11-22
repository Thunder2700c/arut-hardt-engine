# Mobile PBR Pipeline

Status: implementation in progress. The Android Vulkan source now includes a forward metallic-roughness PBR pipeline, a bundled sRGB base-colour texture with CPU-generated mip levels and Vulkan upload, a procedural six-face environment cubemap with GGX importance-sampled specular mips, instanced geometry, CPU frustum culling, a single directional shadow map and optional GPU timestamp profiling. Host and Android ARM64 native compilation are checked by CI without building an APK. Physical-device rendering remains deferred.

## Shading model
The forward shader uses linear-light material and lighting inputs, tangent-space normal mapping, GGX/Trowbridge-Reitz normal distribution, Schlick Fresnel, Schlick-GGX/Smith geometry, Cook-Torrance direct lighting and ACES tone mapping. Base-colour uses an sRGB image format; metallic/roughness and normal maps remain fallback-bound until those assets are added. The output applies a transfer function for the current UNORM swapchain path.

## Shader and descriptor contract
- `mobile_pbr_forward.vert`: mesh attributes at locations 0–3 (position, normal, tangent, UV) and per-instance model matrix columns at locations 4–7.
- `mobile_pbr_forward.frag`: frame set 0 binding 0 uniform block, directional shadow comparison sampler at set 0 binding 1, material set 1 binding 0 uniform and sampled textures at bindings 1–3.
- Frame uniform contains view-projection, camera position, directional-light direction/radiance and light view-projection matrices.
- The shadow depth vertex shader uses the same frame uniform and instance transform stream.
- The material uses a model-matrix instance stream rather than a per-object push constant.
- Set 0 binding 2: procedural linear-light environment cubemap with six faces and GGX-prefiltered roughness mips.

The shader interface is currently an engine-internal prototype, not a stable external material asset format.

## Texture pipeline
- stb_image v2.34 is pinned under third_party/stb; the decoder enables PNG, JPEG, BMP and TGA input and normalizes to RGBA8.
- Decode is bounded to 8192 pixels per side and 16 million total pixels; a texture asset larger than 64 MiB is rejected before decoding.
- CPU mip generation filters sRGB colour channels in linear light and treats alpha as linear.
- Vulkan uploads mip levels through a staging buffer, transitions to shader-read layout, creates a view and a sampler.
- A bundled sRGB base-colour PNG exercises this path. Normal and metallic-roughness bindings still use a white fallback texture.

Texture cache/handles, async decode/upload queues, more texture maps, KTX2 and texture streaming are pending.

## Shadows and visibility
- One directional depth map is created when a depth format supports both depth attachment and sampled-image use.
- The shadow pass draws the same indexed instanced mesh using an orthographic light camera.
- The PBR fragment shader uses a depth-comparison sampler with 3x3 PCF and a slope-dependent bias.
- When the selected depth format cannot be used as a depth attachment but is sampleable, a 1x1 fully-lit shadow fallback is uploaded instead.
- CPU view-frustum extraction uses Vulkan zero-to-one clip depth; bounding-sphere tests cull instances before the instance buffer upload.

Cascaded shadows, resolution selection from quality tiers, cascade stabilization, spot/point-light shadows and GPU/indirect visibility are pending.

## Profiling
The runtime profile snapshot now includes CPU frame time, GPU timestamp duration when supported, shadow-pass GPU duration, draw calls, visible instances and triangle count. The GPU timestamp query pool is optional and disabled when the selected queue does not expose valid timestamp bits. GPU memory totals and detailed per-pass/per-material counters are not yet implemented.

## Validation status and limits
- Host CI compiles PBR and shadow GLSL and runs C++/Rust checks.
- CI also compiles the Android ARM64 native library without running Gradle or building an APK.
- No physical Android device has yet validated shadow-map sampling, visual energy behaviour, image uploads or performance.
- The procedural environment cube now has GGX importance-sampled specular roughness mips. Diffuse irradiance is approximated from its broadest mip, and the split-sum BRDF response uses a small analytic approximation. Cosine-convolved irradiance, a BRDF integration LUT, HDR environment import and probe blending remain future refinements.

## Remaining integration work
1. Add normal and metallic-roughness sample assets and real per-material descriptor binding.
2. Improve IBL with cosine-convolved irradiance, a split-sum BRDF integration LUT, HDR environment import and environment probe blending.
3. Add cascade shadow quality tiers and light-space stabilization.
4. Add GPU/indirect visibility, GPU-memory accounting and per-pass profiling.
5. During the separately agreed testing stage, compare GPU output with the CPU BRDF reference on two Android GPU families.
