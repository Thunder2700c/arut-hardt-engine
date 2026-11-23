#pragma once

#include "arut/animation/motion_matching.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace arut::animation {

inline constexpr std::size_t kPoseHistoryCapacity = 32U;

struct PoseHistoryFrame final {
    double timestamp_seconds{0.0};
    Pose pose{};
    core::Vec3 root_velocity{};
};

class PoseHistory final {
public:
    [[nodiscard]] bool push(double timestamp_seconds,const Pose& pose,
                            const core::Vec3& root_velocity) noexcept;
    [[nodiscard]] const PoseHistoryFrame* latest() const noexcept;
    [[nodiscard]] const PoseHistoryFrame* previous(std::size_t frames_back) const noexcept;
    [[nodiscard]] std::size_t size() const noexcept { return size_; }
    void clear() noexcept { size_=0U; next_=0U; }

private:
    std::array<PoseHistoryFrame,kPoseHistoryCapacity> frames_{};
    std::size_t next_{0U};
    std::size_t size_{0U};
};

struct TrajectoryPredictionInput final {
    core::Vec3 world_position{};
    core::Vec3 world_velocity{};
    core::Vec3 world_acceleration{};
    float facing_yaw_radians{0.0F};
    float angular_velocity_radians{0.0F};
    float maximum_speed{8.0F};
};

class TrajectoryPredictor final {
public:
    [[nodiscard]] static bool predict(
        const TrajectoryPredictionInput& input,
        const std::array<float,kMotionTrajectorySamples>& sample_times,
        std::array<MotionTrajectoryPoint,kMotionTrajectorySamples>& output) noexcept;
};

} // namespace arut::animation
