#pragma once

#include "arut/rendering/humanoid_skinning.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string_view>

namespace arut::rendering {

inline constexpr float kAttackInputBufferSeconds = 0.22F;

enum class HumanoidAnimationState : std::uint8_t {
    Idle, Walk, Run, StrafeLeft, StrafeRight, Backward, Jump, Fall, Land,
    AttackLight, AttackHeavy, AttackKick, Block, Dodge, HitReact, Recover
};

struct HumanoidAnimationInput final {
    float local_move_x{0.0F};
    float local_move_y{0.0F};
    float horizontal_speed{0.0F};
    float vertical_speed{0.0F};
    bool grounded{true};
    bool attack_light_pressed{false};
    bool attack_heavy_pressed{false};
    bool attack_kick_pressed{false};
    bool block_held{false};
    bool dodge_pressed{false};
    bool hit_reaction{false};
    bool recover_pressed{false};
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
        case HumanoidAnimationState::AttackLight: return "attack_light";
        case HumanoidAnimationState::AttackHeavy: return "attack_heavy";
        case HumanoidAnimationState::AttackKick: return "attack_kick";
        case HumanoidAnimationState::Block: return "block";
        case HumanoidAnimationState::Dodge: return "dodge";
        case HumanoidAnimationState::HitReact: return "hit_react";
        case HumanoidAnimationState::Recover: return "recover";
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

        const auto begin_action = [this](HumanoidAnimationState next, float duration) {
            state_ = next;
            action_elapsed_ = 0.0F;
            action_duration_ = duration;
            action_remaining_ = duration;
        };
        const bool attack_pressed = raw_input.attack_light_pressed ||
            raw_input.attack_heavy_pressed || raw_input.attack_kick_pressed;
        if (attack_pressed) {
            buffered_attack_ = raw_input.attack_heavy_pressed
                ? HumanoidAnimationState::AttackHeavy
                : raw_input.attack_kick_pressed
                    ? HumanoidAnimationState::AttackKick
                    : HumanoidAnimationState::AttackLight;
            attack_buffer_remaining_ = kAttackInputBufferSeconds;
        } else if (attack_buffer_remaining_ > 0.0F) {
            attack_buffer_remaining_ = std::max(0.0F, attack_buffer_remaining_ - dt);
            if (attack_buffer_remaining_ == 0.0F)
                buffered_attack_ = HumanoidAnimationState::Idle;
        }
        const bool defensive_input = raw_input.hit_reaction || raw_input.dodge_pressed ||
            raw_input.block_held || raw_input.recover_pressed;
        const bool committed_action = action_remaining_ > 0.0F &&
            state_ != HumanoidAnimationState::Block;
        const bool attack_state = state_ == HumanoidAnimationState::AttackLight ||
                                  state_ == HumanoidAnimationState::AttackHeavy ||
                                  state_ == HumanoidAnimationState::AttackKick;
        const bool combo_window = committed_action && attack_state &&
                                  action_elapsed_ >= action_duration_ * 0.60F;
        const auto clear_attack_buffer = [this]() {
            buffered_attack_ = HumanoidAnimationState::Idle;
            attack_buffer_remaining_ = 0.0F;
        };
        if (raw_input.hit_reaction) {
            clear_attack_buffer();
            begin_action(HumanoidAnimationState::HitReact, 0.42F);
        } else if (raw_input.dodge_pressed) {
            clear_attack_buffer();
            begin_action(HumanoidAnimationState::Dodge, 0.46F);
        } else if (raw_input.block_held && raw_input.grounded &&
                   (!committed_action || (attack_state &&
                    action_elapsed_ >= action_duration_ * 0.40F))) {
            clear_attack_buffer();
            state_ = HumanoidAnimationState::Block;
            action_elapsed_ = action_duration_ = action_remaining_ = 0.0F;
        } else if (raw_input.recover_pressed && raw_input.grounded &&
                   (!committed_action ||
                    action_elapsed_ >= action_duration_ * 0.68F)) {
            clear_attack_buffer();
            begin_action(HumanoidAnimationState::Recover, 0.55F);
        } else if (raw_input.grounded && !defensive_input &&
                   attack_buffer_remaining_ > 0.0F &&
                   (!committed_action || combo_window)) {
            const auto next = buffered_attack_;
            const float duration = next == HumanoidAnimationState::AttackHeavy ? 0.72F
                : next == HumanoidAnimationState::AttackKick ? 0.68F : 0.52F;
            clear_attack_buffer();
            begin_action(next, duration);
        } else if (committed_action) {
            action_elapsed_ += dt;
            action_remaining_ = std::max(0.0F, action_duration_ - action_elapsed_);
        } else if (raw_input.block_held && raw_input.grounded) {
            clear_attack_buffer();
            state_ = HumanoidAnimationState::Block;
            action_remaining_ = 0.0F;
        } else if (!raw_input.grounded) {
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
        if (action_remaining_ > 0.0F &&
            state_ != HumanoidAnimationState::Block &&
            state_ != HumanoidAnimationState::AttackLight &&
            state_ != HumanoidAnimationState::AttackHeavy &&
            state_ != HumanoidAnimationState::AttackKick &&
            state_ != HumanoidAnimationState::Dodge &&
            state_ != HumanoidAnimationState::HitReact &&
            state_ != HumanoidAnimationState::Recover) {
            action_remaining_ = 0.0F;
        }

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
            case HumanoidAnimationState::AttackLight: {
                const float strike = std::clamp(action_elapsed_ / 0.22F, 0.0F, 1.0F);
                const float retract = std::clamp((action_elapsed_ - 0.28F) / 0.20F, 0.0F, 1.0F);
                const float punch = strike * (1.0F - retract);
                set_rotation(target_pose_, B::Chest, {0.0F, 1.0F, 0.0F}, 0.18F * punch);
                set_rotation(target_pose_, B::Chest, {1.0F, 0.0F, 0.0F}, -0.10F * punch);
                set_rotation(target_pose_, B::RightUpperArm, {1.0F, 0.0F, 0.0F}, -1.05F * punch);
                set_rotation(target_pose_, B::RightLowerArm, {1.0F, 0.0F, 0.0F}, -0.28F * punch);
                set_rotation(target_pose_, B::LeftUpperArm, {0.0F, 0.0F, 1.0F}, -0.24F);
                break;
            }
            case HumanoidAnimationState::AttackHeavy: {
                const float phase = std::clamp(action_elapsed_ / 0.72F, 0.0F, 1.0F);
                const float windup = std::clamp(phase / 0.34F, 0.0F, 1.0F);
                const float strike = std::clamp((phase - 0.34F) / 0.30F, 0.0F, 1.0F);
                const float recover = std::clamp((phase - 0.70F) / 0.30F, 0.0F, 1.0F);
                const float arm = 0.75F * windup - 1.28F * strike + 0.53F * recover;
                set_rotation(target_pose_, B::Chest, {0.0F, 1.0F, 0.0F}, -0.28F * windup + 0.34F * strike);
                set_rotation(target_pose_, B::Chest, {1.0F, 0.0F, 0.0F}, -0.16F * strike);
                set_rotation(target_pose_, B::RightUpperArm, {1.0F, 0.0F, 0.0F}, arm);
                set_rotation(target_pose_, B::RightLowerArm, {1.0F, 0.0F, 0.0F}, -0.72F * strike);
                set_rotation(target_pose_, B::LeftUpperArm, {0.0F, 0.0F, 1.0F}, -0.38F);
                target_pose_.local_translations[static_cast<std::size_t>(B::Pelvis)].y -= 0.055F * strike;
                break;
            }
            case HumanoidAnimationState::AttackKick: {
                const float phase = std::clamp(action_elapsed_ / 0.68F, 0.0F, 1.0F);
                const float windup = std::clamp(phase / 0.26F, 0.0F, 1.0F);
                const float strike_phase = std::clamp((phase - 0.26F) / 0.32F, 0.0F, 1.0F);
                const float strike = std::sin(strike_phase * 3.14159265F);
                const float recover = std::clamp((phase - 0.58F) / 0.42F, 0.0F, 1.0F);
                const float thigh_lift = 0.18F * windup + 0.95F * strike - 0.18F * recover;
                set_rotation(target_pose_, B::Pelvis, {0.0F, 1.0F, 0.0F}, -0.24F * strike);
                set_rotation(target_pose_, B::Chest, {0.0F, 1.0F, 0.0F}, 0.20F * strike);
                set_rotation(target_pose_, B::RightUpperLeg, {1.0F, 0.0F, 0.0F}, thigh_lift);
                set_rotation(target_pose_, B::RightLowerLeg, {1.0F, 0.0F, 0.0F},
                             0.52F * windup - 1.25F * strike);
                set_rotation(target_pose_, B::LeftUpperLeg, {1.0F, 0.0F, 0.0F}, -0.12F * strike);
                set_rotation(target_pose_, B::LeftUpperArm, {0.0F, 0.0F, 1.0F}, -0.58F * strike);
                set_rotation(target_pose_, B::RightUpperArm, {0.0F, 0.0F, 1.0F}, 0.42F * strike);
                target_pose_.local_translations[static_cast<std::size_t>(B::Pelvis)].y -=
                    0.025F * strike;
                break;
            }
            case HumanoidAnimationState::Block:
                set_rotation(target_pose_, B::LeftUpperArm, {0.0F, 0.0F, 1.0F}, -0.72F);
                set_rotation(target_pose_, B::RightUpperArm, {0.0F, 0.0F, 1.0F}, 0.72F);
                set_rotation(target_pose_, B::LeftLowerArm, {1.0F, 0.0F, 0.0F}, -0.85F);
                set_rotation(target_pose_, B::RightLowerArm, {1.0F, 0.0F, 0.0F}, -0.85F);
                set_rotation(target_pose_, B::Chest, {1.0F, 0.0F, 0.0F}, -0.08F);
                break;
            case HumanoidAnimationState::Dodge: {
                const float phase = std::clamp(action_elapsed_ / 0.46F, 0.0F, 1.0F);
                const float crouch = std::sin(phase * 3.14159265F);
                set_rotation(target_pose_, B::LeftUpperLeg, {1.0F, 0.0F, 0.0F}, 0.48F * crouch);
                set_rotation(target_pose_, B::RightUpperLeg, {1.0F, 0.0F, 0.0F}, 0.48F * crouch);
                set_rotation(target_pose_, B::LeftLowerLeg, {1.0F, 0.0F, 0.0F}, 0.62F * crouch);
                set_rotation(target_pose_, B::RightLowerLeg, {1.0F, 0.0F, 0.0F}, 0.62F * crouch);
                set_rotation(target_pose_, B::Chest, {0.0F, 0.0F, 1.0F}, 0.34F * crouch);
                target_pose_.local_translations[static_cast<std::size_t>(B::Pelvis)].y -= 0.10F * crouch;
                break;
            }
            case HumanoidAnimationState::HitReact:
                set_rotation(target_pose_, B::Chest, {1.0F, 0.0F, 0.0F}, 0.38F);
                set_rotation(target_pose_, B::Chest, {0.0F, 0.0F, 1.0F}, 0.18F);
                set_rotation(target_pose_, B::LeftUpperArm, {0.0F, 0.0F, 1.0F}, -0.92F);
                set_rotation(target_pose_, B::RightUpperArm, {0.0F, 0.0F, 1.0F}, 0.92F);
                set_rotation(target_pose_, B::LeftUpperLeg, {1.0F, 0.0F, 0.0F}, -0.16F);
                set_rotation(target_pose_, B::RightUpperLeg, {1.0F, 0.0F, 0.0F}, -0.16F);
                break;
            case HumanoidAnimationState::Recover: {
                const float amount = 1.0F - std::clamp(action_elapsed_ / 0.55F, 0.0F, 1.0F);
                set_rotation(target_pose_, B::Chest, {1.0F, 0.0F, 0.0F}, 0.32F * amount);
                set_rotation(target_pose_, B::LeftUpperArm, {0.0F, 0.0F, 1.0F}, -0.45F * amount);
                set_rotation(target_pose_, B::RightUpperArm, {0.0F, 0.0F, 1.0F}, 0.45F * amount);
                break;
            }
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
    float action_elapsed_{0.0F};
    float action_duration_{0.0F};
    float action_remaining_{0.0F};
    HumanoidAnimationState buffered_attack_{HumanoidAnimationState::Idle};
    float attack_buffer_remaining_{0.0F};
};

} // namespace arut::rendering
