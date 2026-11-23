#pragma once

#include "arut/core/character_controller.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <span>

namespace arut::core {

struct CharacterAabbObstacle final {
    Vec3 minimum{};
    Vec3 maximum{};

    [[nodiscard]] bool valid() const noexcept {
        return std::isfinite(minimum.x) && std::isfinite(minimum.y) &&
            std::isfinite(minimum.z) && std::isfinite(maximum.x) &&
            std::isfinite(maximum.y) && std::isfinite(maximum.z) &&
            maximum.x > minimum.x && maximum.y > minimum.y &&
            maximum.z > minimum.z;
    }
};

// Exact distance test for a vertical capsule segment against an AABB,
// including the rounded lower and upper caps.
[[nodiscard]] inline bool character_capsule_overlaps_aabb(
    const Vec3& feet_position, float radius, float height,
    const CharacterAabbObstacle& obstacle) noexcept {
    if (!obstacle.valid() || !std::isfinite(feet_position.x) ||
        !std::isfinite(feet_position.y) || !std::isfinite(feet_position.z) ||
        !std::isfinite(radius) || !std::isfinite(height) ||
        radius <= 0.0F || height < radius * 2.0F) return false;

    const float closest_x = std::clamp(
        feet_position.x, obstacle.minimum.x, obstacle.maximum.x);
    const float closest_z = std::clamp(
        feet_position.z, obstacle.minimum.z, obstacle.maximum.z);
    const float dx = feet_position.x - closest_x;
    const float dz = feet_position.z - closest_z;

    const float segment_bottom = feet_position.y + radius;
    const float segment_top = feet_position.y + height - radius;
    float dy = 0.0F;
    if (segment_top < obstacle.minimum.y)
        dy = obstacle.minimum.y - segment_top;
    else if (segment_bottom > obstacle.maximum.y)
        dy = segment_bottom - obstacle.maximum.y;

    return dx * dx + dy * dy + dz * dz < radius * radius;
}

[[nodiscard]] inline bool character_capsule_overlaps_obstacles(
    const Vec3& feet_position, float radius, float height,
    std::span<const CharacterAabbObstacle> obstacles) noexcept {
    for (const auto& obstacle : obstacles) {
        if (character_capsule_overlaps_aabb(
                feet_position, radius, height, obstacle)) return true;
    }
    return false;
}

} // namespace arut::core
