#pragma once

#include "arut/rendering/mesh.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace arut::rendering {

// Stabilize an orthographic shadow projection by snapping its clip-space translation
// to shadow texels. The matrix uses column-major storage and Vulkan NDC coordinates.
[[nodiscard]] inline Mat4 stabilize_shadow_matrix(Mat4 light_view_projection,
                                                    std::uint32_t resolution) noexcept {
    if (resolution == 0U) return light_view_projection;
    const float clip_texel = 2.0F / static_cast<float>(resolution);
    if (!std::isfinite(clip_texel) || clip_texel <= 0.0F) return light_view_projection;
    light_view_projection.value[12] =
        std::round(light_view_projection.value[12] / clip_texel) * clip_texel;
    light_view_projection.value[13] =
        std::round(light_view_projection.value[13] / clip_texel) * clip_texel;
    return light_view_projection;
}

[[nodiscard]] inline float cascade_blend_factor(float view_depth, float split_depth,
                                                 float blend_fraction = 0.1F) noexcept {
    if (!std::isfinite(view_depth) || !std::isfinite(split_depth) || split_depth <= 0.0F)
        return 0.0F;
    const float fraction = std::clamp(blend_fraction, 0.0F, 0.5F);
    const float blend_start = split_depth * (1.0F - fraction);
    const float span = split_depth - blend_start;
    if (span <= 1.0e-6F) return view_depth >= split_depth ? 1.0F : 0.0F;
    return std::clamp((view_depth - blend_start) / span, 0.0F, 1.0F);
}

} // namespace arut::rendering
