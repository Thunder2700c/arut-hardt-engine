# Dependency Evaluation

**Status:** cgltf 1.15 is now vendored and linked for the initial GLB skeletal animation importer. Other entries remain candidates unless noted in their owning subsystem docs.

Before adoption, record exact version/commit, license, transitive dependencies, Android ARM64 support, binary size, maintenance state, build procedure, and benchmark results on target devices.

| Area | Candidate | Evaluation notes |
|---|---|---|
| Graphics API | Vulkan headers/loader, volk | Validate NDK/loader integration and extension support |
| GPU allocation | Vulkan Memory Allocator | Verify license, Android support, ownership |
| Physics | Jolt Physics | Compare Android performance, vehicle needs, binary cost |
| ECS | EnTT or custom data-oriented core | Benchmark query patterns before committing |
| Animation | Ozz-animation | Audit runtime/import workflow and Android build |
| Navigation | Recast & Detour | Validate streaming/rebuild needs and license |
| Audio | miniaudio | Evaluate Android output, spatial audio and license |
| Asset processing | meshoptimizer | Evaluate pipeline and attribution |
| Texture containers | KTX-Software | Confirm KTX2/BasisU path and device support |
| glTF import | cgltf 1.15 | Integrated for embedded GLB 2.0 skeletal meshes/skins/animation clips; MIT; external .gltf URIs and meshopt/Draco compression are not enabled |
| Editor UI | Dear ImGui or custom UI | Desktop authoring first; keep editor separate from runtime |

Prefer permissive licenses where capabilities are comparable. Preserve exact notices for shipped dependencies. Avoid copying proprietary engine code, assets, or shaders. A dependency is not integrated until the build links it and tests exercise it.
