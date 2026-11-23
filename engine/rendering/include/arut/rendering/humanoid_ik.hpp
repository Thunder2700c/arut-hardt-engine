#pragma once

#include "arut/core/humanoid_skeleton.hpp"
#include "arut/rendering/humanoid_skinning.hpp"

#include <algorithm>
#include <cmath>

namespace arut::rendering {

struct HumanoidLegIkResult final {
    float upper_leg_pitch{0.0F};
    float lower_leg_pitch{0.0F};
    float foot_pitch{0.0F};
    float foot_roll{0.0F};
    float requested_distance{0.0F};
    float solved_distance{0.0F};
    bool valid{false};
    bool reach_clamped{false};
};

[[nodiscard]] inline HumanoidLegIkResult solve_two_bone_leg_ik(
    float target_down,
    float target_forward,
    float upper_length,
    float lower_down,
    float lower_forward,
    const core::Vec3& local_ground_normal) noexcept {
    HumanoidLegIkResult result{};
    const float lower_length = std::sqrt(
        lower_down * lower_down + lower_forward * lower_forward);
    const float normal_length_squared =
        local_ground_normal.x * local_ground_normal.x +
        local_ground_normal.y * local_ground_normal.y +
        local_ground_normal.z * local_ground_normal.z;
    if (!std::isfinite(target_down) || !std::isfinite(target_forward) ||
        !std::isfinite(upper_length) || !std::isfinite(lower_length) ||
        !std::isfinite(lower_down) || !std::isfinite(lower_forward) ||
        !std::isfinite(normal_length_squared) ||
        upper_length <= 1.0e-4F || lower_length <= 1.0e-4F ||
        lower_down <= 0.0F || normal_length_squared <= 1.0e-8F ||
        !std::isfinite(local_ground_normal.x) ||
        !std::isfinite(local_ground_normal.y) ||
        !std::isfinite(local_ground_normal.z)) return result;

    const float distance = std::sqrt(
        target_down * target_down + target_forward * target_forward);
    if (!std::isfinite(distance) || distance <= 1.0e-5F) return result;
    result.requested_distance = distance;
    const float minimum_reach = std::abs(upper_length - lower_length) + 1.0e-4F;
    const float maximum_reach = upper_length + lower_length - 1.0e-4F;
    const float solved_distance = std::clamp(distance, minimum_reach, maximum_reach);
    result.solved_distance = solved_distance;
    result.reach_clamped = std::abs(solved_distance - distance) > 1.0e-5F;

    constexpr float pi = 3.14159265358979323846F;
    const float cosine_hip_triangle = std::clamp(
        (upper_length * upper_length + solved_distance * solved_distance -
         lower_length * lower_length) /
        (2.0F * upper_length * solved_distance), -1.0F, 1.0F);
    const float cosine_knee_internal = std::clamp(
        (upper_length * upper_length + lower_length * lower_length -
         solved_distance * solved_distance) /
        (2.0F * upper_length * lower_length), -1.0F, 1.0F);
    const float target_pitch = std::atan2(target_forward, target_down);
    const float hip_triangle_angle = std::acos(cosine_hip_triangle);
    const float knee_internal_angle = std::acos(cosine_knee_internal);
    const float knee_flexion = pi - knee_internal_angle;
    const float lower_rest_pitch = std::atan2(lower_forward, lower_down);

    result.upper_leg_pitch = target_pitch - hip_triangle_angle;
    result.lower_leg_pitch = knee_flexion - lower_rest_pitch;

    const float inverse_normal_length = 1.0F / std::sqrt(normal_length_squared);
    const float normal_x = local_ground_normal.x * inverse_normal_length;
    const float normal_y = local_ground_normal.y * inverse_normal_length;
    const float normal_z = local_ground_normal.z * inverse_normal_length;
    const float terrain_pitch = std::atan2(-normal_z, normal_y);
    const float terrain_roll = std::atan2(-normal_x, normal_y);
    result.foot_pitch = terrain_pitch -
        (result.upper_leg_pitch + result.lower_leg_pitch);
    result.foot_roll = terrain_roll;
    result.valid = std::isfinite(result.upper_leg_pitch) &&
        std::isfinite(result.lower_leg_pitch) &&
        std::isfinite(result.foot_pitch) && std::isfinite(result.foot_roll);
    return result;
}

[[nodiscard]] inline HumanoidQuaternion blend_humanoid_rotation(
    const HumanoidQuaternion& from, const HumanoidQuaternion& to,
    float weight) noexcept {
    const float alpha = std::clamp(weight, 0.0F, 1.0F);
    float dot = from.x * to.x + from.y * to.y +
                from.z * to.z + from.w * to.w;
    const float sign = dot < 0.0F ? -1.0F : 1.0F;
    HumanoidQuaternion result{
        from.x + (to.x * sign - from.x) * alpha,
        from.y + (to.y * sign - from.y) * alpha,
        from.z + (to.z * sign - from.z) * alpha,
        from.w + (to.w * sign - from.w) * alpha
    };
    const float length_squared = result.x * result.x + result.y * result.y +
        result.z * result.z + result.w * result.w;
    if (!std::isfinite(length_squared) || length_squared <= 1.0e-12F)
        return {};
    const float inverse = 1.0F / std::sqrt(length_squared);
    result.x *= inverse;
    result.y *= inverse;
    result.z *= inverse;
    result.w *= inverse;
    return result;
}

} // namespace arut::rendering
