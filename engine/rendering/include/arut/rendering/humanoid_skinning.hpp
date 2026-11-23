#pragma once

#include "arut/core/humanoid_skeleton.hpp"
#include "arut/animation/runtime.hpp"
#include "arut/rendering/mesh.hpp"

#include <array>
#include <cmath>
#include <cstddef>

namespace arut::rendering {

inline constexpr std::size_t kHumanoidShaderBoneCount = animation::kMaxPoseJoints;
static_assert(core::kHumanoidBoneCount <= kHumanoidShaderBoneCount,
              "Reference mannequin must fit in the scalable GLSL bone palette");

struct HumanoidQuaternion final {
    float x{0.0F};
    float y{0.0F};
    float z{0.0F};
    float w{1.0F};
};

struct HumanoidPose final {
    std::array<core::Vec3, core::kHumanoidBoneCount> local_translations{};
    std::array<HumanoidQuaternion, core::kHumanoidBoneCount> local_rotations{};
};

struct HumanoidSkinningPalette final {
    std::array<Mat4, kHumanoidShaderBoneCount> matrices{};
};

[[nodiscard]] inline HumanoidPose make_rest_humanoid_pose(
    const core::HumanoidSkeleton& skeleton = core::make_robot_mannequin_skeleton()) noexcept {
    HumanoidPose pose{};
    for (std::size_t i = 0; i < skeleton.bones.size(); ++i) {
        pose.local_translations[i] = skeleton.bones[i].rest_translation;
        pose.local_rotations[i] = {};
    }
    return pose;
}

[[nodiscard]] inline HumanoidQuaternion quaternion_from_axis_angle(
    const std::array<float, 3>& axis, float radians) noexcept {
    const float length_squared = axis[0]*axis[0] + axis[1]*axis[1] + axis[2]*axis[2];
    if (!std::isfinite(radians) || !std::isfinite(length_squared) ||
        length_squared <= 1.0e-12F) return {};
    const float inverse_length = 1.0F / std::sqrt(length_squared);
    const float half = radians * 0.5F;
    const float sine = std::sin(half);
    return {axis[0]*inverse_length*sine, axis[1]*inverse_length*sine,
            axis[2]*inverse_length*sine, std::cos(half)};
}

[[nodiscard]] inline Mat4 humanoid_local_transform(
    const core::Vec3& translation, const HumanoidQuaternion& rotation) noexcept {
    const float length_squared = rotation.x*rotation.x + rotation.y*rotation.y +
        rotation.z*rotation.z + rotation.w*rotation.w;
    if (!std::isfinite(length_squared) || length_squared <= 1.0e-12F) return {};
    const float inverse_length = 1.0F / std::sqrt(length_squared);
    const float x = rotation.x*inverse_length;
    const float y = rotation.y*inverse_length;
    const float z = rotation.z*inverse_length;
    const float w = rotation.w*inverse_length;
    Mat4 result = Mat4::identity();
    result.value[0] = 1.0F - 2.0F*(y*y + z*z);
    result.value[1] = 2.0F*(x*y + z*w);
    result.value[2] = 2.0F*(x*z - y*w);
    result.value[4] = 2.0F*(x*y - z*w);
    result.value[5] = 1.0F - 2.0F*(x*x + z*z);
    result.value[6] = 2.0F*(y*z + x*w);
    result.value[8] = 2.0F*(x*z + y*w);
    result.value[9] = 2.0F*(y*z - x*w);
    result.value[10] = 1.0F - 2.0F*(x*x + y*y);
    result.value[12] = translation.x;
    result.value[13] = translation.y;
    result.value[14] = translation.z;
    return result;
}

[[nodiscard]] inline bool build_humanoid_skinning_palette(
    const core::HumanoidSkeleton& skeleton,
    const HumanoidPose& pose,
    HumanoidSkinningPalette& output) noexcept {
    output = {};
    for (auto& matrix : output.matrices) matrix = Mat4::identity();
    if (!skeleton.valid()) return false;
    std::array<Mat4, core::kHumanoidBoneCount> global_pose{};
    std::array<std::array<float, 3>, core::kHumanoidBoneCount> bind_positions{};

    for (std::size_t i = 0; i < skeleton.bones.size(); ++i) {
        const auto& t = pose.local_translations[i];
        const auto& q = pose.local_rotations[i];
        const float q_length_squared = q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w;
        if (!std::isfinite(t.x) || !std::isfinite(t.y) || !std::isfinite(t.z) ||
            !std::isfinite(q.x) || !std::isfinite(q.y) ||
            !std::isfinite(q.z) || !std::isfinite(q.w) ||
            !std::isfinite(q_length_squared) || q_length_squared <= 1.0e-12F)
            return false;

        const Mat4 local = humanoid_local_transform(t, q);
        const auto parent = skeleton.bones[i].parent;
        global_pose[i] = parent < 0
            ? local
            : multiply(global_pose[static_cast<std::size_t>(parent)], local);

        const auto& rest = skeleton.bones[i].rest_translation;
        if (parent < 0) {
            bind_positions[i] = {rest.x, rest.y, rest.z};
        } else {
            const auto& parent_position = bind_positions[static_cast<std::size_t>(parent)];
            bind_positions[i] = {parent_position[0] + rest.x,
                                 parent_position[1] + rest.y,
                                 parent_position[2] + rest.z};
        }
    }

    for (std::size_t i = 0; i < skeleton.bones.size(); ++i) {
        Mat4 inverse_bind = Mat4::identity();
        inverse_bind.value[12] = -bind_positions[i][0];
        inverse_bind.value[13] = -bind_positions[i][1];
        inverse_bind.value[14] = -bind_positions[i][2];
        output.matrices[i] = multiply(global_pose[i], inverse_bind);
        for (const float value : output.matrices[i].value)
            if (!std::isfinite(value)) {
                output = {};
                return false;
            }
    }
    return true;
}

[[nodiscard]] inline std::array<float, 3> transform_humanoid_point(
    const Mat4& matrix, const std::array<float, 3>& point) noexcept {
    return {
        matrix.value[0]*point[0] + matrix.value[4]*point[1] + matrix.value[8]*point[2] + matrix.value[12],
        matrix.value[1]*point[0] + matrix.value[5]*point[1] + matrix.value[9]*point[2] + matrix.value[13],
        matrix.value[2]*point[0] + matrix.value[6]*point[1] + matrix.value[10]*point[2] + matrix.value[14]
    };
}

static_assert(sizeof(HumanoidSkinningPalette) ==
              sizeof(Mat4) * kHumanoidShaderBoneCount);

} // namespace arut::rendering
