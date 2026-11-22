# Development Guide

Host prerequisites: CMake 3.25+, Ninja, C++20 compiler, stable Rust/Cargo and rustfmt.

Host build: cmake --preset dev; cmake --build --preset dev; ctest --preset dev; cargo test --workspace --locked; cargo fmt --all -- --check.

Android prerequisites: SDK, NDK 27.2.12479018, CMake 3.22.1, Java 17, Gradle 8.9, Rust stable and target aarch64-linux-android.

Android build:
1. Install Rust target aarch64-linux-android.
2. Set ANDROID_NDK_HOME and add its LLVM toolchain bin directory to PATH.
3. Build Rust staticlib: cargo build --workspace --locked --release --target aarch64-linux-android.
4. Build APK: gradle --no-daemon :app:assembleDebug.

APK: app/build/outputs/apk/debug/app-debug.apk. Device smoke tests are documented in docs/ANDROID_RUNTIME.md.

Keep clean checkouts buildable. Review licenses and Android support before adding dependencies. Keep C++/Rust calls behind the C ABI. Do not commit build output, APKs, caches or credentials. A low-memory Chromebook is not the primary build machine; use CI for reproducible builds and a physical phone for runtime validation.
