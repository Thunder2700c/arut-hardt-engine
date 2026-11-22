# Vulkan Renderer Bring-up

Status: first-triangle pipeline source is implemented; Android ARM64 native compilation is being added to CI. No APK is built by normal CI.

## Current render path
1. Create Android Vulkan instance and surface.
2. Select a graphics/present queue pair and enable swapchain support.
3. Create FIFO swapchain and color image views.
4. Select a supported depth format (D32 float, D24/S8, or D16), allocate a device-local depth image and view.
5. Create a render pass with color and depth attachments and framebuffers.
6. Load SPIR-V from Android assets: shaders/first_triangle.vert.spv and shaders/first_triangle.frag.spv.
7. Create a graphics pipeline with position/color vertex attributes, triangle-list topology, dynamic viewport/scissor and depth test/write.
8. Upload a three-vertex RGB triangle into a host-visible/coherent vertex buffer.
9. Acquire, clear color/depth, bind pipeline and vertex buffer, draw three vertices, submit and present.

## Shader asset contract
GLSL source lives in engine/rendering/shaders/. Host CI compiles all PBR and triangle shaders with glslangValidator to validate syntax. The explicitly approved Android APK workflow compiles the two first-triangle shaders into app/src/main/assets/shaders/ before Gradle packaging. Android runtime loads these SPIR-V assets through AAssetManager.

## Geometry and camera foundations
engine/rendering/include/arut/rendering/mesh.hpp defines a PBR-ready interleaved vertex (position, normal, tangent, UV), a CPU unit-cube generator, column-major matrices, a right-handed look-at view matrix and a Vulkan zero-to-one perspective matrix with framebuffer Y inversion.

## Deliberate limits
- First triangle uses clip-space coordinates and interpolated RGB. It is a bring-up test, not a camera-driven scene.
- The triangle path currently uses one non-indexed vertex buffer. Indexed mesh buffers, camera uniforms, texture descriptors and PBR pipeline integration remain pending.
- One depth image is shared by swapchain framebuffers. The renderer currently has one frame in flight, so depth writes are serialized.
- No APK, emulator run or physical-device rendering test is performed by normal CI.
- The PBR shaders are compiled in host CI but are not yet loaded by the Android runtime.

## Lifecycle
Swapchain recreation destroys and recreates the triangle pipeline, vertex buffer, depth target, render targets, command buffers and synchronization objects. Resource destruction waits for device idle before releasing GPU objects.
