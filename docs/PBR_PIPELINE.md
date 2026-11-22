# Mobile PBR Pipeline

Status: implementation in progress. The Android Vulkan source now includes a forward metallic-roughness PBR pipeline, a bundled sRGB base-colour texture with CPU-generated mip levels and Vulkan upload, a procedural six-face environment cubemap with GGX importance-sampled specular mips, instanced geometry, CPU frustum culling, a single directional shadow map and optional GPU timestamp profiling. Host and Android ARM64 native compilation are checked by CI without building an APK. Physical-device rendering remains deferred.

## Shading model
The forward shader uses linear-light material and lighting inputs, tangent-space normal mapping, GGX/Trowbridge-Reitz normal distribution, Schlick Fresnel, Schlick-GGX/Smith geometry, Cook-Torrance direct lighting and ACES tone mapping. Base-colour uses an sRGB image format; metallic/roughness and normal maps remain fallback-bound until those assets are added. The output applies a transfer function for the current UNORM swapchain path.

## Shader and descriptor contract
- `mobile_pbr_forward.vert`: mesh attributes at locations 0–3 (position, normal, tangent, UV) and per-instance model matrix columns at locations 4–7.
- `mobile_pbr_forward.frag`: frame set 0 binding 0 uniform block, directional shadow comparison sampler at binding 1, environment cubemap at binding 2, split-sum BRDF integration LUT at binding 3, material set 1 binding 0 uniform and sampled textures at bindings 1–3.
- Frame uniform contains view-projection, camera position, directional-light direction/radiance and light view-projection matrices.
- The shadow depth vertex shader uses the same frame uniform and instance transform stream.
- The material uses a model-matrix instance stream rather than a per-object push constant.
- Set 0 binding 2: procedural linear-light environment cubemap with six faces and GGX-prefiltered roughness mips.
- Set 0 binding 3: 64x64 RGBA8 split-sum GGX BRDF LUT (R=scale coefficient A, G=bias coefficient B), sampled at mip zero with linear filtering.
- Set 0 binding 3: 64x64 RGBA8 split-sum GGX BRDF integration LUT; R and G channels store the scale and bias coefficients.

The shader interface is currently an engine-internal prototype, not a stable external material asset format.

## Texture pipeline
- stb_image v2.34 is pinned under third_party/stb; LDR PNG, JPEG, BMP and TGA decode to RGBA8, while Radiance RGBE HDR decodes to linear RGBA32F and preserves values above 1.0.
- HDR decode is bounded to the same 8192-pixel side and 16-million-pixel limits; malformed, non-finite and negative decoded values are rejected.
- Linear equirectangular HDR images can be converted on CPU into six RGBA32F cubemap faces. Longitude sampling wraps across the seam, latitude sampling clamps at the poles, and radiance above 1.0 remains unclamped.
- Float HDR cubemaps retain radiance through CPU GGX specular roughness prefiltering and cosine-convolved diffuse irradiance. The results can be converted to validated IEEE-754 binary16 RGBA16F data; values outside the finite half-float range are rejected rather than clamped.
- Android optionally loads `app/src/main/assets/environments/default.hdr`, uploads the specular and diffuse cubemaps as `VK_FORMAT_R16G16B16A16_SFLOAT`, and binds them through the existing PBR cubemap descriptors. If that asset is absent or the GPU lacks sampled/transfer support for RGBA16F, the renderer falls back to the procedural RGBA8 environment. No sample HDR file is currently bundled.
- GPU environment-probe arrays/storage, per-object probe selection and runtime multi-probe blending remain pending.
- Decode is bounded to 8192 pixels per side and 16 million total pixels; a texture asset larger than 64 MiB is rejected before decoding.
- CPU mip generation filters sRGB colour channels in linear light and treats alpha as linear.
- Vulkan uploads mip levels through a staging buffer, transitions to shader-read layout, creates a view and a sampler.
- A bundled sRGB base-colour PNG, a tangent-space normal TGA and a linear metallic-roughness TGA exercise all three material map bindings. The test material sets the base-colour, normal and metallic-roughness texture flags.

Texture cache/handles, async decode/upload queues, KTX2, more production-resolution textures and texture streaming are pending.

## Shadows and visibility
- A four-layer directional depth array is created when a depth format supports both depth attachment and sampled-image use.
- The shadow pass draws the same indexed instanced mesh into one framebuffer per active cascade using per-cascade orthographic light cameras.
- Cascade selection uses camera-forward view depth and split distances; the shader samples the selected depth-array layer with 3x3 PCF and slope-dependent bias.
- The PBR fragment shader uses a depth-array comparison sampler with 3x3 PCF and a slope-dependent bias.
- When the selected depth format cannot be used as a depth attachment but is sampleable, a 1x1 fully-lit shadow fallback is uploaded instead.
- CPU view-frustum extraction uses Vulkan zero-to-one clip depth; bounding-sphere tests provide the fallback path.
- When the selected graphics queue supports compute and the shader/pipeline/limits are available, a compute dispatch tests source spheres, compacts visible model matrices into the vertex-rate storage buffer, and atomically updates the indexed indirect command's instance count.
- Explicit host-to-compute, compute-to-vertex/indirect and compute-to-host barriers protect the shared buffers. The previous frame fence is waited before host readback of the completed visible count.
- Devices without compute support or a valid compute pipeline keep the CPU-generated instance buffer and indirect command.
- The current scene contains only three sample instances, so the compute path validates the architecture but does not establish a performance win.

Quality presets select 1/2/4 active cascades and 512/512/1024/1536 map resolutions for Low/Medium/High/Ultra. Light-space projection translation is texel-snapped and adjacent cascades cross-fade over the final 10% of each split. Spot/point-light shadows remain pending.

## Profiling
The runtime profile snapshot includes CPU frame time, separate GPU visibility-compute, cascaded-shadow and main-scene durations, total GPU frame time when supported, draw calls, visible instances and triangle count. The GPU timestamp query pool is optional and disabled when the selected queue does not expose valid timestamp bits. Engine-owned Vulkan allocations are tracked separately from optional driver-reported device-local heap usage/budget through VK_EXT_memory_budget; unsupported devices report that budget data is unavailable. Per-material timing remains pending.

## Validation status and limits
- Host CI compiles PBR, shadow and compute-culling GLSL and runs C++/Rust checks.
- The CPU BRDF reference has a hemispherical energy-budget regression across metallic values 0, 0.5 and 1.0 and roughness values 0.1, 0.5 and 0.9.
- The split-sum LUT is generated on CPU with Hammersley/GGX sampling (64x64, 128 samples per texel), uploaded through Vulkan and sampled by the PBR fragment shader.

- CI also compiles the Android ARM64 native library without running Gradle or building an APK.
- No physical Android device has yet validated shadow-map sampling, visual energy behaviour, image uploads or performance.
- The procedural environment cube has GGX importance-sampled specular roughness mips. Diffuse irradiance uses a separate cosine-convolved cubemap.
- A CPU environment-probe influence contract computes smooth AABB edge fades, normalizes overlapping probe weights and resolves overlaps by priority.
- Android uploads a separate four-slot std140 probe uniform buffer. The PBR fragment shader selects the highest-priority probes with non-zero AABB influence, normalizes their smooth edge weights, and blends their specular and diffuse cubemaps using fixed-index accesses into four-element descriptor arrays; no optional descriptor-indexing feature is required.
- The sample scene currently activates one local probe. Optional independent HDR assets can be placed at `environments/probes/probe_0.hdr` through `probe_3.hdr`; each is decoded, converted to a cubemap, GGX-prefiltered, diffuse-convolved and uploaded as RGBA16F. Missing or unsupported assets fall back per slot to the global environment/irradiance maps.

## Remaining integration work
1. Add a material asset/cache/handle layer and asynchronous decode/upload queues.
2. Add scene/runtime probe registration and per-object probe selection, provide representative independent probe assets, and validate multi-probe blending and RGBA16F rendering on device.
3. Add cascade stabilization, split blending and local-light shadows.
4. Add GPU/indirect visibility, GPU-memory accounting and per-pass profiling.
5. During the separately agreed testing stage, compare GPU output with the CPU BRDF reference on two Android GPU families.
