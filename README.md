# ARUT-HARDT Engine

An independent, Android-first open-world game engine built with C++ and Rust.

## Direction
- Primary platform: Android ARM64.
- Secondary platforms: Windows and Linux desktop.
- Native language: C++20.
- Systems language: Rust.
- Graphics API: Vulkan.
- Build: CMake + Ninja, Cargo and Gradle.
- C++/Rust interop: narrow, versioned C ABI.

## Current implementation
- C++20 core and headless desktop bootstrap.
- Rust workspace with a C ABI version symbol.
- Native Android NativeActivity application for ARM64.
- Native lifecycle, focus, window and input callbacks.
- Vulkan device capability enumeration when available.
- C++ calls into the Rust ABI at Android startup.
- Host CI and Android ARM64 APK CI.

The Android app is a platform smoke-test shell. It does not render a 3D scene yet; the Vulkan renderer begins in Phase 2. This is not yet a GTA-scale engine.

See docs/ANDROID_RUNTIME.md, docs/TECHNICAL_ARCHITECTURE.md and docs/ROADMAP.md.
