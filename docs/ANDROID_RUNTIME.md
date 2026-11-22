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

The Android Vulkan renderer now includes the swapchain and surface lifecycle, depth-tested mesh rendering, a mobile forward PBR pipeline, texture and mesh uploads, HDR environment lighting, directional shadow cascades, instanced drawing, CPU/GPU frustum culling and optional GPU timestamp profiling. The current device stress fixture contains 5,000 cubes.

Surface-bound resources are destroyed on `APP_CMD_TERM_WINDOW` and rebuilt on `APP_CMD_INIT_WINDOW`. Swapchain out-of-date/suboptimal results trigger recreation.

The Galaxy S23 FE has been used for runtime testing. This does not establish cross-vendor correctness: the Phase 2 validation gate still requires a second Android GPU family, graceful quality-fallback checks and a GPU-shader-versus-CPU-reference comparison.
