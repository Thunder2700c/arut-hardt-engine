# Mobile PBR Pipeline

Status: mobile PBR GLSL is compiled in host CI and packaged by the explicitly triggered Android workflow. Android renderer source now creates the PBR pipeline, descriptors, frame/material uniforms, real decoded sRGB base-colour PNG, CPU-generated mip chains, Vulkan texture upload, indexed cube geometry and instanced draw path. Physical-device validation remains deferred.

## Rendering model
The initial Android path is a forward metallic-roughness renderer. It uses linear-light material and light values, tangent-space normal maps, a GGX/Trowbridge-Reitz normal distribution, Schlick Fresnel, Schlick-GGX geometry and Cook-Torrance direct lighting. The fragment shader applies an ACES fitted tone curve and an output transfer suitable for the current UNORM swapchain selection.

## Shader files
- `engine/rendering/shaders/mobile_pbr_forward.vert`: transforms position, normal and tangent into world space and projects vertices.
- `engine/rendering/shaders/mobile_pbr_forward.frag`: material factors/maps, alpha mask, normal mapping, direct directional lighting, emissive and tone mapping.

## Descriptor contract (shader source)
- Set 0, binding 0: frame uniform block (`viewProjection`, camera position, directional light direction and radiance).
- Set 1, binding 0: material uniform block (base colour, emissive/metallic, roughness/normal-scale/occlusion-strength/alpha-cutoff, flags).
- Set 1, bindings 1–3: base-colour, tangent-space normal and metallic-roughness textures.
- Push constants: per-object model matrix.
- Vertex locations: position 0, normal 1, tangent 2, UV 3.

The shader contract is a starting point, not yet a stable public asset format. Vulkan descriptor layouts, pipeline layouts, shader compilation and actual vertex buffer formats must be implemented and validated before runtime use.

## CPU reference
`engine/rendering/include/arut/rendering/pbr_brdf.hpp` supplies deterministic host-side reference functions for the BRDF and ACES curve. It exists for unit testing and future numerical comparisons; it is not a substitute for GPU shader validation.

## Known limitations
- The Vulkan renderer now has an asset-backed first-triangle pipeline, one interleaved vertex buffer and a depth attachment. The PBR shaders are now connected to the active indexed-cube draw path. The original triangle pipeline remains only as a bootstrap/reference path.
- Ambient illumination is a temporary constant; image-based lighting, irradiance/prefiltered environment maps and BRDF integration LUT are not implemented.
- The material contract has an occlusion-strength scalar but no occlusion texture descriptor yet. The shader does not pretend that a scalar alone is an occlusion map.
- Per-instance model-matrix vertex input and indexed instanced drawing are implemented. Shadow maps, cascades, IBL environment maps, CPU/GPU visibility culling and local lights remain pending.
- Shader compilation is not part of CI yet. Do not mark shader code as compiled until a GLSL compiler and validation step are added.

## Integration order
1. Complete Phase 2: verify the active indexed PBR cube scene on a physical Android device during the agreed testing phase.
2. Add reproducible GLSL-to-SPIR-V compilation and validate shader interfaces.
3. Create descriptor set layouts, pipeline layout, render state and graphics pipeline.
4. Render a known material test mesh and compare GPU results with CPU reference cases.
5. Add texture color-space handling, AO, IBL, shadows, instancing and profiling incrementally.
6. Measure and tune quality presets on more than one Android GPU family.
