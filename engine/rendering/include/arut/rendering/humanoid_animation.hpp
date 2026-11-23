#pragma once

#include "arut/rendering/humanoid_skinning.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string_view>

namespace arut::rendering {

enum class HumanoidAnimationState : std::uint8_t {
    Idle, Walk, Run, StrafeLeft, StrafeRight, Backward, Jump, Fall, Land
};

struct HumanoidAnimationInput final {
    float local_move_x{0.0F};
    float local_move_y{0.0F};
    float horizontal_speed{0.0F};
    float vertical_speed{0.0F};
    bool grounded{true};
};

struct HumanoidAnimationSnapshot final {
    HumanoidAnimationState state{HumanoidAnimationState::Idle};
    float phase_seconds{0.0F};
    float blend_alpha{0.0F};
    float horizontal_speed{0.0F};
};

[[nodiscard]] constexpr std::string_view humanoid_animation_state_name(
    HumanoidAnimationState state) noexcept {
    switch (state) {
        case HumanoidAnimationState::Idle: return "idle";
        case HumanoidAnimationState::Walk: return "walk";
        case HumanoidAnimationState::Run: return "run";
        case HumanoidAnimationState::StrafeLeft: return "strafe_left";
        case HumanoidAnimationState::StrafeRight: return "strafe_right";
        case HumanoidAnimationState::Backward: return "backward";
        case HumanoidAnimationState::Jump: return "jump";
        case HumanoidAnimationState::Fall: return "fall";
        case HumanoidAnimationState::Land: return "land";
    }
    return "invalid";
}

class HumanoidAnimator final {
public:
    explicit HumanoidAnimator(
        core::HumanoidSkeleton skeleton = core::make_robot_mannequin_skeleton())
        : skeleton_(skeleton.valid() ? skeleton : core::make_robot_mannequin_skeleton()),
          current_pose_(make_rest_humanoid_pose(skeleton_)),
          target_pose_(current_pose_) {}

    [[nodiscard]] const HumanoidPose& update(
        const HumanoidAnimationInput& raw_input, float delta_seconds) noexcept {
        if (!std::isfinite(delta_seconds) || delta_seconds <= 0.0F)
            return current_pose_;
        const float dt = std::clamp(delta_seconds, 0.0F, 0.05F);
        const float move_x = std::clamp(raw_input.local_move_x, -1.0F, 1.0F);
        const float move_y = std::clamp(raw_input.local_move_y, -1.0F, 1.0F);
        const float speed = std::isfinite(raw_input.horizontal_speed)
            ? std::max(0.0F, raw_input.horizontal_speed) : 0.0F;
        const float vertical_speed = std::isfinite(raw_input.vertical_speed)
            ? raw_input.vertical_speed : 0.0F;

        if (has_ground_sample_ && !was_grounded_ && raw_input.grounded)
            land_time_remaining_ = kLandDuration;
        was_grounded_ = raw_input.grounded;
        has_ground_sample_ = true;

        if (!raw_input.grounded) {
            state_ = vertical_speed > 0.20F
                ? HumanoidAnimationState::Jump : HumanoidAnimationState::Fall;
        } else if (land_time_remaining_ > 0.0F) {
            state_ = HumanoidAnimationState::Land;
        } else if (speed < 0.12F) {
            state_ = HumanoidAnimationState::Idle;
        } else if (move_y < -0.15F && std::abs(move_y) >= std::abs(move_x)) {
            state_ = HumanoidAnimationState::Backward;
        } else if (std::abs(move_x) > std::abs(move_y) * 1.15F) {
            state_ = move_x < 0.0F
                ? HumanoidAnimationState::StrafeLeft
                : HumanoidAnimationState::StrafeRight;
        } else if (speed >= kRunThreshold) {
            state_ = HumanoidAnimationState::Run;
        } else {
            state_ = HumanoidAnimationState::Walk;
        }

        const float frequency = state_ == HumanoidAnimationState::Run ? 10.5F
            : (state_ == HumanoidAnimationState::Idle ? 1.2F : 7.2F);
        if (state_ != HumanoidAnimationState::Jump &&
            state_ != HumanoidAnimationState::Fall &&
            state_ != HumanoidAnimationState::Land) {
            phase_seconds_ += dt * frequency;
        }
        build_target_pose(move_x, move_y, speed, vertical_speed);
        blend_pose(dt);
        if (land_time_remaining_ > 0.0F)
            land_time_remaining_ = std::max(0.0F, land_time_remaining_ - dt);

        snapshot_.state = state_;
        snapshot_.phase_seconds = phase_seconds_;
        snapshot_.blend_alpha = last_blend_alpha_;
        snapshot_.horizontal_speed = speed;
        return current_pose_;
    }

    [[nodiscard]] const HumanoidPose& pose() const noexcept { return current_pose_; }
    [[nodiscard]] const HumanoidAnimationSnapshot& snapshot() const noexcept {
        return snapshot_;
    }

private:
    static constexpr float kRunThreshold = 3.2F;
    static constexpr float kLandDuration = 0.18F;
    static constexpr float kBlendRate = 10.0F;

    void set_rotation(HumanoidPose& pose, core::HumanoidBone bone,
                      const std::array<float, 3>& axis, float angle) const noexcept {
        pose.local_rotations[static_cast<std::size_t>(bone)] =
            quaternion_from_axis_angle(axis, angle);
    }

    void build_target_pose(float move_x, float move_y, float speed,
                           float vertical_speed) noexcept {
        target_pose_ = make_rest_humanoid_pose(skeleton_);
        const float cycle = std::sin(phase_seconds_);
        const float inverse_cycle = -cycle;
        const float stride = state_ == HumanoidAnimationState::Run ? 0.72F : 0.48F;
        const float travel_direction = state_ == HumanoidAnimationState::Backward
            ? -1.0F : 1.0F;
        const float activity = std::clamp(speed / kRunThreshold, 0.25F, 1.35F);
        using B = core::HumanoidBone;

        switch (state_) {
            case HumanoidAnimationState::Idle: {
                target_pose_.local_translations[static_cast<std::size_t>(B::Pelvis)].y +=
                    0.008F * (0.5F + 0.5F * std::sin(phase_seconds_ * 2.0F));
                set_rotation(target_pose_, B::Chest, {0.0F, 0.0F, 1.0F},
                             0.012F * std::sin(phase_seconds_ * 2.0F));
                break;
            }
            case HumanoidAnimationState::Walk:
            case HumanoidAnimationState::Run:
            case HumanoidAnimationState::StrafeLeft:
            case HumanoidAnimationState::StrafeRight:
            case HumanoidAnimationState::Backward: {
                const float leg_swing = stride * activity * cycle * travel_direction;
                const float knee_left = std::max(0.0F, -cycle) * 0.62F * activity;
                const float knee_right = std::max(0.0F, cycle) * 0.62F * activity;
                set_rotation(target_pose_, B::LeftUpperLeg, {1.0F, 0.0F, 0.0F}, leg_swing);
                set_rotation(target_pose_, B::RightUpperLeg, {1.0F, 0.0F, 0.0F}, -leg_swing);
                set_rotation(target_pose_, B::LeftLowerLeg, {1.0F, 0.0F, 0.0F}, knee_left);
                set_rotation(target_pose_, B::RightLowerLeg, {1.0F, 0.0F, 0.0F}, knee_right);
                set_rotation(target_pose_, B::LeftUpperArm, {0.0F, 1.0F, 0.0F},
                             -0.30F * leg_swing);
                set_rotation(target_pose_, B::RightUpperArm, {0.0F, 1.0F, 0.0F},
                             0.30F * leg_swing);
                if (state_ == HumanoidAnimationState::StrafeLeft ||
                    state_ == HumanoidAnimationState::StrafeRight) {
                    const float side = state_ == HumanoidAnimationState::StrafeLeft
                        ? -1.0F : 1.0F;
                    set_rotation(target_pose_, B::Chest, {0.0F, 0.0F, 1.0F},
                                 side * 0.08F);
                }
                if (state_ == HumanoidAnimationState::Run)
                    target_pose_.local_translations[static_cast<std::size_t>(B::Pelvis)].y +=
                        0.035F * std::abs(cycle);
                else
                    target_pose_.local_translations[static_cast<std::size_t>(B::Pelvis)].y +=
                        0.018F * std::abs(cycle);
                break;
            }
            case HumanoidAnimationState::Jump:
                set_rotation(target_pose_, B::LeftUpperLeg, {1.0F, 0.0F, 0.0F}, 0.25F);
                set_rotation(target_pose_, B::RightUpperLeg, {1.0F, 0.0F, 0.0F}, 0.25F);
                set_rotation(target_pose_, B::LeftLowerLeg, {1.0F, 0.0F, 0.0F}, 0.55F);
                set_rotation(target_pose_, B::RightLowerLeg, {1.0F, 0.0F, 0.0F}, 0.55F);
                set_rotation(target_pose_, B::LeftUpperArm, {0.0F, 0.0F, 1.0F}, -0.45F);
                set_rotation(target_pose_, B::RightUpperArm, {0.0F, 0.0F, 1.0F}, 0.45F);
                break;
            case HumanoidAnimationState::Fall:
                set_rotation(target_pose_, B::LeftUpperLeg, {1.0F, 0.0F, 0.0F}, -0.12F);
                set_rotation(target_pose_, B::RightUpperLeg, {1.0F, 0.0F, 0.0F}, -0.12F);
                set_rotation(target_pose_, B::LeftUpperArm, {0.0F, 0.0F, 1.0F}, -0.72F);
                set_rotation(target_pose_, B::RightUpperArm, {0.0F, 0.0F, 1.0F}, 0.72F);
                set_rotation(target_pose_, B::Chest, {1.0F, 0.0F, 0.0F},
                             std::clamp(-vertical_speed * 0.02F, -0.20F, 0.20F));
                break;
            case HumanoidAnimationState::Land: {
                const float bend = 0.62F *
                    std::clamp(land_time_remaining_ / kLandDuration, 0.0F, 1.0F);
                set_rotation(target_pose_, B::LeftUpperLeg, {1.0F, 0.0F, 0.0F}, bend);
                set_rotation(target_pose_, B::RightUpperLeg, {1.0F, 0.0F, 0.0F}, bend);
                set_rotation(target_pose_, B::LeftLowerLeg, {1.0F, 0.0F, 0.0F}, bend * 0.8F);
                set_rotation(target_pose_, B::RightLowerLeg, {1.0F, 0.0F, 0.0F}, bend * 0.8F);
                break;
            }
        }
        (void)move_x;
        (void)move_y;
    }

    static HumanoidQuaternion normalized_lerp(
        const HumanoidQuaternion& from, const HumanoidQuaternion& to,
        float alpha) noexcept {
        float direction = from.x*to.x + from.y*to.y +
                          from.z*to.z + from.w*to.w;
        const float sign = direction < 0.0F ? -1.0F : 1.0F;
        HumanoidQuaternion q{
            from.x + (to.x*sign - from.x)*alpha,
            from.y + (to.y*sign - from.y)*alpha,
            from.z + (to.z*sign - from.z)*alpha,
            from.w + (to.w*sign - from.w)*alpha
        };
        const float length_squared = q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w;
        if (!(length_squared > 1.0e-12F) || !std::isfinite(length_squared))
            return {};
        const float inverse = 1.0F / std::sqrt(length_squared);
        q.x *= inverse; q.y *= inverse; q.z *= inverse; q.w *= inverse;
        return q;
    }

    void blend_pose(float delta_seconds) noexcept {
        last_blend_alpha_ = 1.0F - std::exp(-kBlendRate * delta_seconds);
        for (std::size_t i = 0; i < current_pose_.local_translations.size(); ++i) {
            auto& current = current_pose_.local_translations[i];
            const auto& target = target_pose_.local_translations[i];
            current.x += (target.x - current.x) * last_blend_alpha_;
            current.y += (target.y - current.y) * last_blend_alpha_;
            current.z += (target.z - current.z) * last_blend_alpha_;
            current_pose_.local_rotations[i] = normalized_lerp(
                current_pose_.local_rotations[i],
                target_pose_.local_rotations[i], last_blend_alpha_);
        }
    }

    core::HumanoidSkeleton skeleton_{};
    HumanoidPose current_pose_{};
    HumanoidPose target_pose_{};
    HumanoidAnimationSnapshot snapshot_{};
    HumanoidAnimationState state_{HumanoidAnimationState::Idle};
    float phase_seconds_{0.0F};
    float land_time_remaining_{0.0F};
    float last_blend_alpha_{0.0F};
    bool was_grounded_{false};
    bool has_ground_sample_{false};
};

} // namespace arut::rendering
