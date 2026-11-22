# Vulkan Renderer Bring-up

Status: first-triangle pipeline source is implemented; Android ARM64 native compilation is being added to CI. No APK is built by normal CI.

## Current render path
1. Create Android Vulkan instance and surface.
2. Select a graphics/present queue pair and enable swapchain support.
3. Create FIFO swapchain and color image views.
4. Select a supported depth format (D32 float, D24/S8, or D16), allocate a device-local depth image and view.
5. Create a render pass with color and depth attachments and framebuffers.
6. Upload a one-pixel fallback sampled texture and create its image view/sampler.
7. Load mobile_pbr_forward.vert.spv and mobile_pbr_forward.frag.spv from Android assets.
8. Create frame/material descriptor-set layouts, a push-constant range for the model matrix, and the mobile forward PBR graphics pipeline.
9. Generate an indexed unit cube with position/normal/tangent/UV vertices; upload vertex and index buffers.
10. Create frame/material uniform buffers and descriptor sets. The three material texture bindings point to a valid fallback texture until asset texture streaming is implemented.
11. Each frame, bind the PBR pipeline and descriptor sets, push the model matrix, bind cube vertex/index buffers, draw the indexed cube, submit and present.

## Shader asset contract
GLSL source lives in engine/rendering/shaders/. Host CI compiles all PBR and triangle shaders with glslangValidator to validate syntax. The explicitly approved Android APK workflow compiles the two first-triangle shaders into app/src/main/assets/shaders/ before Gradle packaging. Android runtime loads these SPIR-V assets through AAssetManager.

## Geometry and camera foundations
engine/rendering/include/arut/rendering/mesh.hpp defines a PBR-ready interleaved vertex (position, normal, tangent, UV), a CPU unit-cube generator, column-major matrices, a right-handed look-at view matrix and a Vulkan zero-to-one perspective matrix with framebuffer Y inversion.

## Deliberate limits
- The active draw path is now a camera-driven indexed cube using the mobile metallic-roughness PBR shaders, GGX distribution, Schlick Fresnel, Smith geometry and ACES tone mapping.
- The PBR material currently uses constant factors and no texture maps. Descriptor slots are valid and point to a one-pixel fallback texture; real image decoding, upload, mipmaps and streaming remain pending.
- Ambient lighting is a small placeholder term; image-based lighting, environment probes, shadow maps and local lights remain pending.
- The original RGB triangle pipeline remains as a bootstrap/reference path but is no longer the active draw.
- One depth image is shared by swapchain framebuffers. The renderer currently has one frame in flight, so depth writes are serialized.
- No APK, emulator run or physical-device rendering test is performed by normal CI.

## Lifecycle
Swapchain recreation destroys and recreates the triangle pipeline, vertex buffer, depth target, render targets, command buffers and synchronization objects. Resource destruction waits for device idle before releasing GPU objects.
