#pragma once

#include "arut/animation/runtime.hpp"

#include <string_view>

namespace arut::animation {

// True for common locomotion-root names used by GLB humanoid rigs.
[[nodiscard]] bool is_root_motion_joint_name(std::string_view name) noexcept;

struct RootMotionDelta final {
    core::Vec3 local_translation{};
    float yaw_radians{0.0F};
};

// Computes planar root displacement and heading change between two sampled
// poses. Vertical root movement is intentionally left in the pose for jumps.
[[nodiscard]] bool extract_root_motion(const Pose& previous, const Pose& current,
                                       std::size_t root_joint,
                                       RootMotionDelta& output) noexcept;

// Applies local planar displacement in the actor's current world heading.
[[nodiscard]] bool apply_root_motion(const RootMotionDelta& delta,
                                     core::Vec3& world_position,
                                     float& world_yaw_radians) noexcept;

// Removes planar root displacement from a pose while preserving its height.
[[nodiscard]] bool strip_root_motion(Pose& pose, std::size_t root_joint,
                                     const core::Vec3& reference_translation) noexcept;

} // namespace arut::animation
