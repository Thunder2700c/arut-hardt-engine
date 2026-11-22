# ARUT-HARDT Engine

**An independent, Android-first open-world game engine built with C++ and Rust.**

ARUT-HARDT is being restarted as a native engine project. It does not use Godot, Unreal Engine, RAGE, REDengine, IW, or another game engine as its runtime foundation. Commercial engines may inform publicly observable design goals, but all engine implementation here will be original or come from explicitly audited open-source dependencies.

## Direction

- **Primary platform:** Android ARM64.
- **Secondary platforms:** Windows and Linux desktop.
- **Native language:** C++20.
- **Systems language:** Rust.
- **Graphics API:** Vulkan, introduced in the renderer milestone.
- **Build:** CMake + Ninja for C++; Cargo for Rust.
- **Interop:** A narrow, versioned C ABI. No C++ objects or Rust-owned allocations cross the boundary.
- **Product:** A reusable game engine first; the open-world game built with it comes later.

## Current state

This repository is a clean-slate bootstrap, not a functioning GTA-scale engine yet.

Included now:
- A small C++20 core library and headless runtime bootstrap.
- A Rust workspace with a minimal ABI/version surface.
- CMake and Cargo build entry points.
- Initial tests and CI.
- Architecture, dependency, and development documents.

Not implemented yet:
- Vulkan renderer or Android APK.
- World partitioning or asset streaming.
- Vehicle physics, character animation, traffic, pedestrian AI, or editor.
- GTA-scale content, tools, or production pipeline.

Do not describe any of those systems as complete until implemented, built, and tested.

## Quick start

Requirements: CMake 3.25+, a C++20 compiler, Ninja, and stable Rust/Cargo.

### C++

\`\`\`sh
cmake --preset dev
cmake --build --preset dev
ctest --preset dev
./build/dev/arut_runtime
\`\`\`

### Rust

\`\`\`sh
cargo test --workspace
cargo fmt --all -- --check
\`\`\`

See [docs/DEVELOPMENT.md](docs/DEVELOPMENT.md) and [docs/TECHNICAL_ARCHITECTURE.md](docs/TECHNICAL_ARCHITECTURE.md).

The authoritative implementation status and acceptance gates are in [docs/ROADMAP.md](docs/ROADMAP.md). The current milestone is **Phase 0: clean native foundation**.

## License

Original ARUT-HARDT project code is licensed under the MIT License. Third-party dependencies and assets retain their own licenses. See [LICENSE](LICENSE) and [docs/DEPENDENCIES.md](docs/DEPENDENCIES.md).
