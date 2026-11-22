# Development Guide

## Host requirements

- CMake 3.25 or newer.
- Ninja.
- C++20 compiler (Clang, GCC, or MSVC).
- Stable Rust toolchain with Cargo and rustfmt.

## Android requirements (Phase 1 onward)

- Android SDK and NDK installed through Android Studio or command-line tools.
- ANDROID_NDK_HOME pointing to the NDK.
- Android platform 26 or newer for the initial ARM64 target.
- Physical-device validation for graphics, thermal, and lifecycle behaviour.

## Build C++

\`\`\`sh
cmake --preset dev
cmake --build --preset dev
ctest --preset dev
\`\`\`

This produces a headless executable and small static core library, not an Android APK.

## Build and test Rust

\`\`\`sh
cargo test --workspace
cargo fmt --all -- --check
\`\`\`

## Configure the Android native core

After installing the NDK and setting ANDROID_NDK_HOME:

\`\`\`sh
cmake --preset android-arm64
cmake --build --preset android-arm64
\`\`\`

The preset currently configures the portable native core for Android ARM64. It does not package an Android application until the runtime/app target is implemented.

## Working rules

1. Read the architecture and roadmap before changing a subsystem.
2. Keep a clean checkout buildable.
3. Prefer small, reviewable changes with tests.
4. Do not add dependencies without license, Android, build-cost, and maintenance review.
5. Keep C++/Rust calls behind a narrow C ABI.
6. Do not add game-specific content to the engine repository.
7. Report exactly what was written, built, tested, and device-verified.
8. Never commit build output, APKs, AABs, caches, or credentials.

A low-memory Chromebook is not the primary build machine for a large native engine. Keep local builds small and use CI for reproducible host builds; use the NDK/CI for Android and a physical device for runtime validation.
