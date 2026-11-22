#pragma once

#include "arut/rendering/image.hpp"

#include <cstdint>

namespace arut::rendering {

// Stores the split-sum GGX BRDF integration coefficients A and B in RG channels.
[[nodiscard]] bool build_brdf_integration_lut(
    std::uint32_t dimension, std::uint32_t sample_count,
    Rgba8Image& output) noexcept;

} // namespace arut::rendering
