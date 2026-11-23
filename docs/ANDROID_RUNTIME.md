# Android Native Runtime — Phase 1

The Android app uses android.app.NativeActivity and Android NDK native_app_glue. The Phase 2 renderer creates a Vulkan Android surface and swapchain when the native window appears.

Implemented in this phase:
- ARM64 (arm64-v8a), minimum Android API 26.
- Native lifecycle, focus, window, configuration and low-memory callbacks.
- Native touch and key input delivery.
- Vulkan 1.0 instance and physical-device capability discovery.
- GPU name, vendor/device IDs, API/driver versions, device-local heap total, queue-family and extension counts in logcat.
- Optional Vulkan startup: a missing Vulkan implementation is logged and does not prevent the native activity from starting.
- Rust static library built for aarch64-linux-android; C++ queries its ABI version on startup.

Build prerequisites: Android SDK, NDK 27.2.12479018, CMake 3.22.1, Java 17, Gradle 8.9, Rust stable and the aarch64-linux-android target.

Build steps:
1. Add Rust target aarch64-linux-android.
2. Set ANDROID_NDK_HOME to the installed NDK.
3. Add the NDK LLVM toolchain bin directory to PATH.
4. Run cargo build --workspace --locked --release --target aarch64-linux-android.
5. Run gradle --no-daemon :app:assembleDebug.

APK output: app/build/outputs/apk/debug/app-debug.apk.

Device smoke test:
- Install and launch ARUT-HARDT Engine on an ARM64 Android device.
- Capture logs with adb logcat -s ARUT-HARDT.
- Confirm engine and Rust ABI startup logs.
- Confirm pause/resume, stop/start and window create/destroy callbacks.
- Confirm touch and hardware key events.
- Confirm Vulkan capability output on supported devices, or a graceful no-Vulkan log.

A successful CI APK build is not proof of device behaviour. Physical-device lifecycle, input and Vulkan validation remain required before Phase 1 is complete.


## Vulkan renderer bootstrap — Phase 2

The Android Vulkan renderer now includes the swapchain and surface lifecycle, depth-tested mesh rendering, a mobile forward PBR pipeline, texture and mesh uploads, HDR environment lighting, directional shadow cascades, instanced drawing, CPU/GPU frustum culling and optional GPU timestamp profiling. The 5,000-cube stress grid is now created by the core ECS `CubeSpawnSystem`; `CubeRenderExtractionSystem` reads transform, mesh and bounds components and hands renderer-neutral instances to Vulkan. The renderer does not create cube entities or generate the stress-grid transforms.

The host stress test verifies 5,000 live ECS entities, all required components, idempotent spawning, grid bounds and 5,000 extracted instances. Android ARM64 native compilation passed in CI. S23 FE APK/runtime validation is pending; the app startup log reports the ECS entity and extraction counts before renderer initialization.

Surface-bound resources are destroyed on `APP_CMD_TERM_WINDOW` and rebuilt on `APP_CMD_INIT_WINDOW`. Swapchain out-of-date/suboptimal results trigger recreation.

The Galaxy S23 FE has been used for runtime testing. This does not establish cross-vendor correctness: the Phase 2 validation gate still requires a second Android GPU family, graceful quality-fallback checks and a GPU-shader-versus-CPU-reference comparison.

## Development Test Laboratory

The NativeActivity subclass `com.arut.engine.TestLabActivity` displays the Android profile picker before scene startup. Select one profile:

- **ECS Stress Test** creates the 5,000-entity ECS stress scene and does not submit the humanoid mannequin.
- **Humanoid Test** skips creation of the 5,000 ECS entities and submits the procedural mannequin with the streamed terrain arena.
- **Combined Integration** creates both as a separate combined test.

The selection is passed to native C++ through a small JNI callback and stored as a stable `TestLabProfile` ID. Runtime logs include `TEST_LAB selected=...`, `TEST_LAB_RENDER_PROFILE`, `TEST_ARENA_COLLISION`, and `TEST_ARENA_RENDER_INSTANCES`. Restart the app to select a different profile.

In the Humanoid Test, use the left-side touch stick to move, drag in the upper-right area to orbit the follow camera, and tap the lower-right area to jump. A transparent on-screen guide marks the move/look/jump zones without consuming the NativeActivity touch events. The runtime feeds fixed-step terrain-grounded movement into procedural Idle/Walk/Run/Strafe/Backward/Jump/Fall/Land poses, performs stance-phase two-bone terrain foot placement, then uploads the model transform and bone palette for Vulkan skinning. Logs include `CHARACTER_CONTROLLER`, `ANIMATION_STATE`, and `HUMANOID_IK`.

The Android Humanoid and Combined scenes include visible static AABB arena walls and two obstacle blocks. The capsule collision query uses those same AABBs and supports axis sliding. This is a small static test arena; dynamic rigid bodies and general world collision are not integrated yet. The follow camera shortens its distance against the terrain heightfield and static test-arena AABBs. Arbitrary non-AABB geometry, moving obstacles and dynamic rigid bodies are not tested. Desktop keyboard/mouse controls are not implemented. Visual and touch verification on the S23 FE remains outstanding.
