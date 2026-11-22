#pragma once

#include <cstdint>
#include <vector>

namespace arut::rendering {

struct Rgba32fImage;

struct EnvironmentCubeMip final {
    std::uint32_t width{0};
    std::uint32_t height{0};
    // Six tightly packed RGBA8 faces in +X, -X, +Y, -Y, +Z, -Z order.
    // Values are linear light, not sRGB encoded.
    std::vector<std::uint8_t> pixels;
};

struct EnvironmentCubeF32Mip final {
    std::uint32_t width{0};
    std::uint32_t height{0};
    // Six tightly packed RGBA32F faces in +X, -X, +Y, -Y, +Z, -Z order.
    // RGB stores linear radiance without an LDR clamp.
    std::vector<float> pixels;
};

// Converts a linear equirectangular HDR image into a float cubemap.
// Horizontal sampling wraps across the longitude seam; vertical sampling clamps at the poles.
[[nodiscard]] bool build_environment_cube_from_equirectangular(
    const Rgba32fImage& source, std::uint32_t cube_size,
    EnvironmentCubeF32Mip& output) noexcept;

// Builds a compact linear-light procedural sky/ground cubemap and box-filtered mips.
// This is a bootstrap IBL source; GGX importance prefiltering is a later refinement.
[[nodiscard]] bool build_procedural_environment_cube(
    std::uint32_t base_size, std::vector<EnvironmentCubeMip>& output) noexcept;

// Integrates incoming radiance over a cosine-weighted hemisphere for each output normal.
// The resulting cubemap stores irradiance divided by PI for the Lambertian diffuse term.
[[nodiscard]] bool build_diffuse_irradiance_cube(
    const EnvironmentCubeMip& source_base, std::uint32_t output_size,
    std::uint32_t sample_count, EnvironmentCubeMip& output) noexcept;

} // namespace arut::rendering
