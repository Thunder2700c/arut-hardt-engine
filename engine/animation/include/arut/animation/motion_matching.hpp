#pragma once

#include "arut/animation/runtime.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

namespace arut::animation {

inline constexpr std::size_t kMotionTrajectorySamples = 3U;

struct MotionTrajectoryPoint final {
    float time_seconds{0.0F};
    core::Vec3 position_offset{};
};

struct PoseSearchSchema final {
    std::vector<std::uint16_t> joints;
    std::vector<float> joint_weights;
    std::array<float, kMotionTrajectorySamples> trajectory_times{0.2F, 0.4F, 0.8F};
    float trajectory_weight{1.0F};
    float continuity_penalty{0.15F};
    float minimum_reselect_seconds{0.12F};

    [[nodiscard]] bool valid(std::size_t skeleton_joint_count) const noexcept;
};

struct MotionClipSource final {
    const AnimationClip* clip{nullptr};
    std::uint32_t tags{0U};
};

struct MotionMatchQuery final {
    Pose current_pose{};
    std::array<MotionTrajectoryPoint, kMotionTrajectorySamples> trajectory{};
    std::uint32_t required_tags{0U};
    std::uint32_t forbidden_tags{0U};
    std::uint32_t previous_clip_index{std::numeric_limits<std::uint32_t>::max()};
    float previous_time_seconds{-1.0F};
};

struct MotionMatchResult final {
    bool found{false};
    std::uint32_t clip_index{std::numeric_limits<std::uint32_t>::max()};
    float time_seconds{0.0F};
    float cost{std::numeric_limits<float>::infinity()};
    std::size_t database_sample_index{0U};
};

class MotionMatchingDatabase final {
public:
    [[nodiscard]] bool build(
        const std::vector<MotionClipSource>& clips,
        const PoseSearchSchema& schema,
        float sample_rate_hz) noexcept;

    [[nodiscard]] MotionMatchResult search(const MotionMatchQuery& query) const noexcept;
    [[nodiscard]] std::size_t sample_count() const noexcept { return samples_.size(); }
    [[nodiscard]] std::size_t feature_count() const noexcept { return feature_count_; }
    [[nodiscard]] bool empty() const noexcept { return samples_.empty(); }

private:
    struct Sample final {
        std::uint32_t clip_index{0U};
        std::uint32_t tags{0U};
        float time_seconds{0.0F};
        std::vector<float> features;
    };

    [[nodiscard]] bool make_features(
        const Pose& pose,
        const std::array<MotionTrajectoryPoint, kMotionTrajectorySamples>& trajectory,
        const PoseSearchSchema& schema,
        std::vector<float>& output) const noexcept;

    PoseSearchSchema schema_{};
    std::vector<Sample> samples_;
    std::size_t feature_count_{0U};
};

} // namespace arut::animation
