#pragma once

#include "arut/rendering/mesh.hpp"

#include <array>
#include <cmath>

namespace arut::rendering {

struct FrustumPlane final {
    std::array<float, 3> normal{0.0F, 0.0F, 1.0F};
    float distance{0.0F};
};

struct ViewFrustum final {
    // left, right, bottom, top, near, far. Vulkan clip depth is [0, 1].
    std::array<FrustumPlane, 6> planes{};
};

struct BoundingSphere final {
    std::array<float, 3> center{};
    float radius{0.0F};
};

[[nodiscard]] inline ViewFrustum extract_vulkan_frustum(const Mat4& view_projection) noexcept {
    const auto row = [&view_projection](std::size_t index) {
        return std::array<float, 4>{
            view_projection.value[index],
            view_projection.value[4U + index],
            view_projection.value[8U + index],
            view_projection.value[12U + index]
        };
    };
    const auto r0 = row(0);
    const auto r1 = row(1);
    const auto r2 = row(2);
    const auto r3 = row(3);
    const std::array<std::array<float, 4>, 6> raw{{
        {{r3[0] + r0[0], r3[1] + r0[1], r3[2] + r0[2], r3[3] + r0[3]}},
        {{r3[0] - r0[0], r3[1] - r0[1], r3[2] - r0[2], r3[3] - r0[3]}},
        {{r3[0] + r1[0], r3[1] + r1[1], r3[2] + r1[2], r3[3] + r1[3]}},
        {{r3[0] - r1[0], r3[1] - r1[1], r3[2] - r1[2], r3[3] - r1[3]}},
        {{r2[0], r2[1], r2[2], r2[3]}},
        {{r3[0] - r2[0], r3[1] - r2[1], r3[2] - r2[2], r3[3] - r2[3]}}
    }};
    ViewFrustum frustum{};
    for (std::size_t i = 0; i < raw.size(); ++i) {
        const float length_squared = raw[i][0] * raw[i][0] +
                                     raw[i][1] * raw[i][1] +
                                     raw[i][2] * raw[i][2];
        if (!std::isfinite(length_squared) || length_squared <= 1.0e-12F) {
            frustum.planes[i] = {{0.0F, 0.0F, 0.0F}, std::numeric_limits<float>::infinity()};
            continue;
        }
        const float inverse_length = 1.0F / std::sqrt(length_squared);
        frustum.planes[i].normal = {
            raw[i][0] * inverse_length,
            raw[i][1] * inverse_length,
            raw[i][2] * inverse_length
        };
        frustum.planes[i].distance = raw[i][3] * inverse_length;
    }
    return frustum;
}

[[nodiscard]] inline bool sphere_visible(const ViewFrustum& frustum,
                                         const BoundingSphere& sphere) noexcept {
    if (!std::isfinite(sphere.radius) || sphere.radius < 0.0F) return false;
    for (const auto& plane : frustum.planes) {
        const float signed_distance =
            plane.normal[0] * sphere.center[0] +
            plane.normal[1] * sphere.center[1] +
            plane.normal[2] * sphere.center[2] + plane.distance;
        if (std::isfinite(signed_distance) && signed_distance < -sphere.radius) return false;
    }
    return true;
}

} // namespace arut::rendering
