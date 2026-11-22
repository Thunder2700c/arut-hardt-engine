#pragma once

#include <cstdint>
#include <vector>

namespace arut::rendering {

struct EnvironmentCubeMip final {
    std::uint32_t width{0};
    std::uint32_t height{0};
    // Six tightly packed RGBA8 faces in +X, -X, +Y, -Y, +Z, -Z order.
    // Values are linear light, not sRGB encoded.
    std::vector<std::uint8_t> pixels;
};

// Builds a compact linear-light procedural sky/ground cubemap and box-filtered mips.
// This is a bootstrap IBL source; GGX importance prefiltering is a later refinement.
[[nodiscard]] bool build_procedural_environment_cube(
    std::uint32_t base_size, std::vector<EnvironmentCubeMip>& output) noexcept;

} // namespace arut::rendering
