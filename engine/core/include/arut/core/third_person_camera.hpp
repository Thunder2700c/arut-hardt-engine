#pragma once

#include "arut/core/character_controller.hpp"
#include "arut/core/character_collision.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <span>

namespace arut::core {

struct ThirdPersonCameraConfig final {
    float follow_distance{4.2F};
    float minimum_distance{0.8F};
    float terrain_clearance{0.35F};
    float collision_backoff{0.12F};
    float collision_radius{0.18F};
    std::uint32_t sweep_samples{16U};

    [[nodiscard]] bool valid() const noexcept {
        return std::isfinite(follow_distance) && follow_distance > 0.0F &&
            std::isfinite(minimum_distance) && minimum_distance > 0.0F &&
            minimum_distance <= follow_distance &&
            std::isfinite(terrain_clearance) && terrain_clearance >= 0.0F &&
            std::isfinite(collision_backoff) && collision_backoff >= 0.0F &&
            std::isfinite(collision_radius) && collision_radius >= 0.0F &&
            sweep_samples >= 2U && sweep_samples <= 128U;
    }
};

struct ThirdPersonCameraResult final {
    std::array<float, 3> position{};
    float distance{0.0F};
    bool terrain_obstructed{false};
    bool geometry_obstructed{false};
    bool valid{false};
};

[[nodiscard]] inline bool third_person_segment_hits_aabb(
    const std::array<float, 3>& start,
    const std::array<float, 3>& end,
    const CharacterAabbObstacle& obstacle,
    float radius,
    float& first_hit_fraction) noexcept {
    first_hit_fraction = 1.0F;
    if (!obstacle.valid() || !std::isfinite(radius) || radius < 0.0F)
        return false;
    const std::array<float, 3> minimum{
        obstacle.minimum.x - radius,
        obstacle.minimum.y - radius,
        obstacle.minimum.z - radius
    };
    const std::array<float, 3> maximum{
        obstacle.maximum.x + radius,
        obstacle.maximum.y + radius,
        obstacle.maximum.z + radius
    };
    float enter = 0.0F;
    float leave = 1.0F;
    for (std::size_t axis = 0; axis < 3U; ++axis) {
        const float direction = end[axis] - start[axis];
        if (std::abs(direction) <= 1.0e-7F) {
            if (start[axis] < minimum[axis] || start[axis] > maximum[axis])
                return false;
            continue;
        }
        float near_value = (minimum[axis] - start[axis]) / direction;
        float far_value = (maximum[axis] - start[axis]) / direction;
        if (near_value > far_value) std::swap(near_value, far_value);
        enter = std::max(enter, near_value);
        leave = std::min(leave, far_value);
        if (enter > leave) return false;
    }
    if (leave < 0.0F || enter > 1.0F) return false;
    first_hit_fraction = std::clamp(enter, 0.0F, 1.0F);
    return true;
}

[[nodiscard]] inline ThirdPersonCameraResult solve_third_person_camera(
    const std::array<float, 3>& target,
    float yaw_radians,
    float pitch_radians,
    CharacterGroundQuery ground_query,
    void* ground_context = nullptr,
    ThirdPersonCameraConfig config = {},
    std::span<const CharacterAabbObstacle> obstacles = {}) noexcept {
    ThirdPersonCameraResult result{};
    if (!config.valid() || !std::isfinite(yaw_radians) ||
        !std::isfinite(pitch_radians) || !ground_query ||
        !std::isfinite(target[0]) || !std::isfinite(target[1]) ||
        !std::isfinite(target[2])) return result;

    const float clamped_pitch = std::clamp(pitch_radians, -1.45F, 1.45F);
    const float cosine_pitch = std::cos(clamped_pitch);
    const std::array<float, 3> forward{
        std::cos(yaw_radians) * cosine_pitch,
        std::sin(clamped_pitch),
        std::sin(yaw_radians) * cosine_pitch
    };

    float safe_distance = config.follow_distance;
    const float sample_step = config.follow_distance /
                              static_cast<float>(config.sweep_samples);
    for (std::uint32_t sample = 1U; sample <= config.sweep_samples; ++sample) {
        const float distance = sample_step * static_cast<float>(sample);
        const float x = target[0] - forward[0] * distance;
        const float y = target[1] - forward[1] * distance;
        const float z = target[2] - forward[2] * distance;
        const auto ground = ground_query(x, z, ground_context);
        if (!ground.valid || !std::isfinite(ground.height)) continue;
        if (y < ground.height + config.terrain_clearance) {
            const float previous_distance =
                sample_step * static_cast<float>(sample - 1U);
            safe_distance = std::max(config.minimum_distance,
                previous_distance - config.collision_backoff);
            result.terrain_obstructed = true;
            break;
        }
    }

    std::array<float, 3> camera_end{
        target[0] - forward[0] * safe_distance,
        target[1] - forward[1] * safe_distance,
        target[2] - forward[2] * safe_distance
    };
    for (const auto& obstacle : obstacles) {
        float hit_fraction = 1.0F;
        if (!third_person_segment_hits_aabb(
                target, camera_end, obstacle, config.collision_radius,
                hit_fraction)) continue;
        const float obstacle_distance = safe_distance * hit_fraction;
        safe_distance = std::max(config.minimum_distance,
            obstacle_distance - config.collision_backoff);
        result.geometry_obstructed = true;
        camera_end = {
            target[0] - forward[0] * safe_distance,
            target[1] - forward[1] * safe_distance,
            target[2] - forward[2] * safe_distance
        };
    }
    result.distance = safe_distance;
    result.position = camera_end;
    result.valid = std::isfinite(result.position[0]) &&
        std::isfinite(result.position[1]) && std::isfinite(result.position[2]);
    return result;
}

} // namespace arut::core
