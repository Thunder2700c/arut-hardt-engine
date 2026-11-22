# Vulkan Renderer Bring-up

Status: mobile PBR renderer, PNG base-colour texture upload and indexed instancing are implemented in source. Host and Android-native CI compile checks run without packaging an APK.

## Current render path
1. Create Android Vulkan instance and surface.
2. Select a graphics/present queue pair and enable swapchain support.
3. Create FIFO swapchain and color image views.
4. Select a supported depth format (D32 float, D24/S8, or D16), allocate a device-local depth image and view.
5. Create a render pass with color and depth attachments and framebuffers.
6. Decode the bundled PNG base-colour asset into RGBA8, generate CPU mip levels in linear-light space for the sRGB image, and upload all mip levels with a staging buffer.
7. Upload a 1x1 fallback texture for optional normal/metallic-roughness maps.
8. Load mobile_pbr_forward.vert.spv and mobile_pbr_forward.frag.spv from Android assets.
9. Create frame/material descriptor layouts, bind the sRGB base-colour texture and fallback data textures, and create the mobile forward PBR graphics pipeline.
10. Generate an indexed unit cube with position/normal/tangent/UV vertices and upload shared vertex/index buffers.
11. Upload three model matrices to an instance-rate vertex buffer. Each frame, issue one indexed instanced draw for all three cube instances, submit and present.

## Shader asset contract
GLSL source lives in engine/rendering/shaders/. Host CI compiles all PBR and triangle shaders with glslangValidator to validate syntax. The explicitly approved Android APK workflow compiles the two first-triangle shaders into app/src/main/assets/shaders/ before Gradle packaging. Android runtime loads these SPIR-V assets through AAssetManager.

## Geometry and camera foundations
engine/rendering/include/arut/rendering/mesh.hpp defines a PBR-ready interleaved vertex (position, normal, tangent, UV), a CPU unit-cube generator, column-major matrices, a right-handed look-at view matrix and a Vulkan zero-to-one perspective matrix with framebuffer Y inversion.

## Deliberate limits
- The active draw path is now three camera-driven indexed cube instances sharing one mesh and sRGB checker base-colour texture. It uses mobile metallic-roughness PBR shaders, GGX distribution, Schlick Fresnel, Smith geometry and ACES tone mapping.
- The PBR material currently uses constant factors and no texture maps. Descriptor slots are valid and point to a one-pixel fallback texture; real image decoding, upload, mipmaps and streaming remain pending.
- Ambient lighting is a small placeholder term; image-based lighting, environment probes, shadow maps and local lights remain pending.
- The original RGB triangle pipeline remains as a bootstrap/reference path but is no longer the active draw.
- One depth image is shared by swapchain framebuffers. The renderer currently has one frame in flight, so depth writes are serialized.
- No APK, emulator run or physical-device rendering test is performed by normal CI.

## Lifecycle
Swapchain recreation destroys and recreates triangle/PBR pipelines, mesh/instance buffers, textures, descriptor resources, depth target, render targets, command buffers and synchronization objects. Resource destruction waits for device idle before releasing GPU objects.
