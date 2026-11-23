#pragma once

#include "arut/core/humanoid_skeleton.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

namespace arut::core {

struct CharacterInput final {
    // Local axes: +X is right, +Y is forward, both in [-1, 1].
    float move_x{0.0F};
    float move_y{0.0F};
    bool jump_pressed{false};
};

struct CharacterGroundSample final {
    float height{0.0F};
    Vec3 normal{0.0F, 1.0F, 0.0F};
    bool valid{false};
};

using CharacterGroundQuery =
    CharacterGroundSample (*)(float x, float z, void* context) noexcept;

struct CharacterControllerConfig final {
    float capsule_radius{0.32F};
    float capsule_height{1.80F};
    float maximum_speed{4.5F};
    float acceleration{18.0F};
    float deceleration{22.0F};
    float jump_speed{5.2F};
    float gravity{15.0F};
    float maximum_slope_degrees{48.0F};
    float maximum_step_height{0.32F};
    float fixed_timestep{1.0F / 60.0F};
    float maximum_catchup_seconds{0.20F};
    std::uint32_t maximum_steps_per_update{8U};

    [[nodiscard]] bool valid() const noexcept {
        return std::isfinite(capsule_radius) && capsule_radius > 0.0F &&
            std::isfinite(capsule_height) && capsule_height >= capsule_radius * 2.0F &&
            std::isfinite(maximum_speed) && maximum_speed > 0.0F &&
            std::isfinite(acceleration) && acceleration > 0.0F &&
            std::isfinite(deceleration) && deceleration > 0.0F &&
            std::isfinite(jump_speed) && jump_speed > 0.0F &&
            std::isfinite(gravity) && gravity > 0.0F &&
            std::isfinite(maximum_slope_degrees) &&
                maximum_slope_degrees > 0.0F && maximum_slope_degrees < 89.0F &&
            std::isfinite(maximum_step_height) && maximum_step_height >= 0.0F &&
            std::isfinite(fixed_timestep) && fixed_timestep > 0.0F &&
            std::isfinite(maximum_catchup_seconds) &&
                maximum_catchup_seconds >= fixed_timestep &&
            maximum_steps_per_update > 0U;
    }
};

struct CharacterControllerState final {
    // Character origin is at the capsule feet; world units are metres.
    Vec3 position{};
    Vec3 velocity{};
    float facing_yaw_radians{0.0F};
    bool grounded{false};
};

class CharacterController final {
public:
    explicit CharacterController(
        CharacterControllerConfig config = {}) noexcept : config_(config) {
        if (!config_.valid()) config_ = CharacterControllerConfig{};
    }

    void reset(const Vec3& feet_position = {}, float facing_yaw_radians = 0.0F) noexcept {
        state_ = {};
        state_.position = feet_position;
        state_.facing_yaw_radians =
            std::isfinite(facing_yaw_radians) ? facing_yaw_radians : 0.0F;
        accumulator_seconds_ = 0.0F;
        jump_queued_ = false;
    }

    void set_facing_yaw(float yaw_radians) noexcept {
        if (std::isfinite(yaw_radians)) state_.facing_yaw_radians = yaw_radians;
    }

    [[nodiscard]] const CharacterControllerState& state() const noexcept { return state_; }
    [[nodiscard]] const CharacterControllerConfig& config() const noexcept { return config_; }
    [[nodiscard]] float interpolation_alpha() const noexcept {
        return std::clamp(accumulator_seconds_ / config_.fixed_timestep, 0.0F, 1.0F);
    }

    // Returns the number of fixed physics steps executed. A jump edge is consumed
    // by the first step only, even when one render update catches up several steps.
    [[nodiscard]] std::uint32_t update(
        const CharacterInput& input, float frame_delta_seconds,
        CharacterGroundQuery ground_query, void* ground_context = nullptr) noexcept {
        if (!ground_query || !std::isfinite(frame_delta_seconds) ||
            frame_delta_seconds <= 0.0F) return 0U;

        accumulator_seconds_ += std::min(frame_delta_seconds,
                                         config_.maximum_catchup_seconds);
        if (input.jump_pressed) jump_queued_ = true;
        std::uint32_t steps = 0U;
        CharacterInput step_input = input;
        while (accumulator_seconds_ >= config_.fixed_timestep &&
               steps < config_.maximum_steps_per_update) {
            step_input.jump_pressed = jump_queued_;
            (void)fixed_step(step_input, ground_query, ground_context);
            jump_queued_ = false;
            step_input.jump_pressed = false;
            accumulator_seconds_ -= config_.fixed_timestep;
            ++steps;
        }
        // Drop excess accumulated time after a severe hitch rather than causing
        // an unbounded catch-up spiral on the next frame.
        if (steps == config_.maximum_steps_per_update &&
            accumulator_seconds_ >= config_.fixed_timestep) {
            accumulator_seconds_ = std::fmod(accumulator_seconds_,
                                              config_.fixed_timestep);
        }
        return steps;
    }

    [[nodiscard]] bool fixed_step(
        const CharacterInput& input, CharacterGroundQuery ground_query,
        void* ground_context = nullptr) noexcept {
        if (!ground_query) return false;
        const float dt = config_.fixed_timestep;
        auto current_ground = ground_query(state_.position.x, state_.position.z,
                                           ground_context);
        if (current_ground.valid && is_walkable(current_ground.normal) &&
            state_.velocity.y <= 0.0F &&
            state_.position.y <= current_ground.height + 0.025F) {
            state_.position.y = current_ground.height;
            state_.velocity.y = 0.0F;
            state_.grounded = true;
        }

        float move_x = std::clamp(input.move_x, -1.0F, 1.0F);
        float move_y = std::clamp(input.move_y, -1.0F, 1.0F);
        const float input_length = std::sqrt(move_x * move_x + move_y * move_y);
        if (input_length > 1.0F) {
            move_x /= input_length;
            move_y /= input_length;
        }

        if (input.jump_pressed && state_.grounded) {
            state_.velocity.y = config_.jump_speed;
            state_.grounded = false;
        }

        const float cosine = std::cos(state_.facing_yaw_radians);
        const float sine = std::sin(state_.facing_yaw_radians);
        // At yaw=0 the character faces -Z; +X is its right side.
        const float desired_x = (cosine * move_x + sine * move_y) *
                                config_.maximum_speed;
        const float desired_z = (sine * move_x - cosine * move_y) *
                                config_.maximum_speed;
        const float acceleration = input_length > 1.0e-4F
            ? config_.acceleration : config_.deceleration;
        approach_horizontal_velocity(desired_x, desired_z, acceleration * dt);

        const float proposed_x = state_.position.x + state_.velocity.x * dt;
        const float proposed_z = state_.position.z + state_.velocity.z * dt;
        const auto next_ground = ground_query(proposed_x, proposed_z, ground_context);
        bool accept_horizontal = true;
        if (next_ground.valid) {
            if (!is_walkable(next_ground.normal)) accept_horizontal = false;
            if (state_.grounded &&
                next_ground.height - state_.position.y > config_.maximum_step_height)
                accept_horizontal = false;
        }
        if (accept_horizontal) {
            state_.position.x = proposed_x;
            state_.position.z = proposed_z;
        } else {
            state_.velocity.x = 0.0F;
            state_.velocity.z = 0.0F;
        }

        state_.velocity.y -= config_.gravity * dt;
        state_.position.y += state_.velocity.y * dt;
        const auto landing = ground_query(state_.position.x, state_.position.z,
                                          ground_context);
        if (landing.valid && is_walkable(landing.normal) &&
            state_.velocity.y <= 0.0F &&
            state_.position.y <= landing.height) {
            state_.position.y = landing.height;
            state_.velocity.y = 0.0F;
            state_.grounded = true;
        } else {
            state_.grounded = false;
        }
        return true;
    }

private:
    [[nodiscard]] bool is_walkable(const Vec3& normal) const noexcept {
        const float length_squared = normal.x*normal.x + normal.y*normal.y +
                                     normal.z*normal.z;
        if (!std::isfinite(length_squared) || length_squared <= 1.0e-8F)
            return false;
        const float inverse_length = 1.0F / std::sqrt(length_squared);
        constexpr float radians_per_degree = 0.017453292519943295F;
        const float minimum_up = std::cos(
            config_.maximum_slope_degrees * radians_per_degree);
        return normal.y * inverse_length >= minimum_up;
    }

    void approach_horizontal_velocity(float desired_x, float desired_z,
                                      float maximum_delta) noexcept {
        const float delta_x = desired_x - state_.velocity.x;
        const float delta_z = desired_z - state_.velocity.z;
        const float length = std::sqrt(delta_x*delta_x + delta_z*delta_z);
        if (length <= maximum_delta || length <= 1.0e-6F) {
            state_.velocity.x = desired_x;
            state_.velocity.z = desired_z;
            return;
        }
        const float scale = maximum_delta / length;
        state_.velocity.x += delta_x * scale;
        state_.velocity.z += delta_z * scale;
    }

    CharacterControllerConfig config_{};
    CharacterControllerState state_{};
    float accumulator_seconds_{0.0F};
    bool jump_queued_{false};
};

} // namespace arut::core
