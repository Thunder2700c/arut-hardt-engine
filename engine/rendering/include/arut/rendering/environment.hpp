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

struct EnvironmentCubeF16Mip final {
    std::uint32_t width{0};
    std::uint32_t height{0};
    // IEEE-754 binary16 values, tightly packed RGBA for six cube faces.
    std::vector<std::uint16_t> pixels;
};

// Converts one RGBA32F cubemap mip to IEEE-754 binary16 for Vulkan RGBA16F upload.
// Rejects non-finite, negative or out-of-range values instead of silently clamping HDR.
[[nodiscard]] bool convert_environment_cube_to_rgba16f(
    const EnvironmentCubeF32Mip& source, EnvironmentCubeF16Mip& output) noexcept;

// GGX-prefilters a float HDR source cubemap into a full roughness mip chain.
[[nodiscard]] bool build_ggx_prefiltered_environment_cube(
    const EnvironmentCubeF32Mip& source_base, std::uint32_t sample_count,
    std::vector<EnvironmentCubeF32Mip>& output) noexcept;

// Integrates a float HDR cubemap over a cosine-weighted hemisphere.
// The result stores irradiance divided by PI for the Lambertian diffuse term.
[[nodiscard]] bool build_diffuse_irradiance_cube(
    const EnvironmentCubeF32Mip& source_base, std::uint32_t output_size,
    std::uint32_t sample_count, EnvironmentCubeF32Mip& output) noexcept;

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
