#pragma once

#include "arut/core/humanoid_skeleton.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace arut::animation {

inline constexpr std::size_t kMaxPoseJoints = core::kHumanoidBoneCount;

struct Quaternion final {
    float x{0.0F}, y{0.0F}, z{0.0F}, w{1.0F};
};

struct Transform final {
    core::Vec3 translation{};
    Quaternion rotation{};
};

struct Pose final {
    std::array<Transform, kMaxPoseJoints> joints{};
    std::size_t joint_count{0U};
};

struct PoseKey final {
    float time_seconds{0.0F};
    Pose pose{};
};

struct AnimationClip final {
    std::string name;
    float duration_seconds{0.0F};
    bool looping{true};
    std::vector<PoseKey> keys;

    [[nodiscard]] bool valid(std::size_t expected_joint_count) const noexcept;
    [[nodiscard]] bool sample(float time_seconds, Pose& output) const noexcept;
};

class AnimationPlayer final {
public:
    void set_clip(const AnimationClip* clip, bool restart = true) noexcept;
    [[nodiscard]] bool update(float delta_seconds, Pose& output) noexcept;
    void set_playback_rate(float rate) noexcept;
    void set_looping(bool looping) noexcept;
    [[nodiscard]] float time_seconds() const noexcept { return time_seconds_; }
    [[nodiscard]] const AnimationClip* clip() const noexcept { return clip_; }

private:
    const AnimationClip* clip_{nullptr};
    float time_seconds_{0.0F};
    float playback_rate_{1.0F};
    bool looping_{true};
};

[[nodiscard]] bool blend_poses(
    const Pose& from, const Pose& to, float alpha, Pose& output) noexcept;

[[nodiscard]] Pose make_reference_pose(const core::HumanoidSkeleton& skeleton) noexcept;

} // namespace arut::animation
