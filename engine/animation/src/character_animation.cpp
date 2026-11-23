#include "arut/animation/character_animation.hpp"

#include <algorithm>
#include <cmath>

namespace arut::animation {
namespace {
constexpr std::uint32_t state_id(CharacterAnimationState state) noexcept {
    return static_cast<std::uint32_t>(state) + 1U;
}
}

bool CharacterAnimationController::configure(const CharacterAnimationSet& clips) {
    reset();
    clips_ = clips;
    graph_ = AnimationStateMachine{};
    for (std::size_t i = 0; i < clips_.clips.size(); ++i) {
        const auto state = static_cast<CharacterAnimationState>(i);
        const auto* clip = clips_.clips[i];
        if (!clip) continue;
        const std::size_t joints = clip->keys.empty() ? 0U : clip->keys.front().pose.joint_count;
        if (!clip->valid(joints)) return false;
        if (!graph_.add_state(state_id(state), std::to_string(i), clip)) return false;
    }
    configured_ = has_state(CharacterAnimationState::Idle);
    return configured_;
}

bool CharacterAnimationController::start() noexcept {
    if (!configured_ || !graph_.start(state_id(CharacterAnimationState::Idle))) return false;
    state_ = CharacterAnimationState::Idle;
    state_elapsed_ = 0.0F;
    was_grounded_ = true;
    started_ = true;
    return true;
}

bool CharacterAnimationController::has_state(CharacterAnimationState state) const noexcept {
    return clips_.get(state) != nullptr;
}

CharacterAnimationState CharacterAnimationController::locomotion_state(
    const CharacterAnimationInput& input) const noexcept {
    if (!input.grounded) {
        return input.vertical_speed > 0.15F ? CharacterAnimationState::Jump
                                            : CharacterAnimationState::Fall;
    }
    if (std::abs(input.move_x) < 0.12F && std::abs(input.move_y) < 0.12F)
        return CharacterAnimationState::Idle;
    if (input.move_y < -0.25F && std::abs(input.move_y) >= std::abs(input.move_x))
        return CharacterAnimationState::Backward;
    if (std::abs(input.move_x) > std::abs(input.move_y) * 1.15F)
        return input.move_x < 0.0F ? CharacterAnimationState::StrafeLeft
                                   : CharacterAnimationState::StrafeRight;
    return input.sprinting || input.horizontal_speed >= 3.25F
        ? CharacterAnimationState::Run : CharacterAnimationState::Walk;
}

CharacterAnimationState CharacterAnimationController::choose_state(
    const CharacterAnimationInput& input) const noexcept {
    if (input.hit_reaction && has_state(CharacterAnimationState::HitReact))
        return CharacterAnimationState::HitReact;
    if (input.dodge_pressed && has_state(CharacterAnimationState::Dodge))
        return CharacterAnimationState::Dodge;
    if (input.recover_pressed && has_state(CharacterAnimationState::Recover))
        return CharacterAnimationState::Recover;
    if (input.block_held && has_state(CharacterAnimationState::Block))
        return CharacterAnimationState::Block;
    if (input.attack_heavy_pressed && has_state(CharacterAnimationState::AttackHeavy))
        return CharacterAnimationState::AttackHeavy;
    if (input.attack_light_pressed && has_state(CharacterAnimationState::AttackLight))
        return CharacterAnimationState::AttackLight;

    const auto locomotion = locomotion_state(input);
    if (!was_grounded_ && input.grounded && has_state(CharacterAnimationState::Land))
        return CharacterAnimationState::Land;
    if (has_state(locomotion)) return locomotion;
    if (!input.grounded && has_state(CharacterAnimationState::Fall))
        return CharacterAnimationState::Fall;
    return CharacterAnimationState::Idle;
}

bool CharacterAnimationController::is_timed_action(CharacterAnimationState state) const noexcept {
    switch (state) {
        case CharacterAnimationState::Land:
        case CharacterAnimationState::AttackLight:
        case CharacterAnimationState::AttackHeavy:
        case CharacterAnimationState::Dodge:
        case CharacterAnimationState::HitReact:
        case CharacterAnimationState::Recover:
            return true;
        default: return false;
    }
}

float CharacterAnimationController::duration_of(CharacterAnimationState state) const noexcept {
    const auto* clip = clips_.get(state);
    return clip ? clip->duration_seconds : 0.0F;
}

bool CharacterAnimationController::enter(CharacterAnimationState next,
                                          float blend_seconds) noexcept {
    if (!has_state(next)) return false;
    if (next == state_) return true;
    if (!graph_.transition_to(state_id(next), blend_seconds)) return false;
    state_ = next;
    state_elapsed_ = 0.0F;
    combo_window_open_ = false;
    cancel_window_open_ = false;
    return true;
}

bool CharacterAnimationController::update(float delta_seconds,
    const CharacterAnimationInput& input, Pose& output,
    std::vector<AnimationEvent>& events) {
    if (!started_ || !std::isfinite(delta_seconds) || delta_seconds < 0.0F ||
        !std::isfinite(input.move_x) || !std::isfinite(input.move_y) ||
        !std::isfinite(input.horizontal_speed) || !std::isfinite(input.vertical_speed))
        return false;

    const float dt = std::min(delta_seconds, 0.25F);
    CharacterAnimationState desired = choose_state(input);
    if (is_timed_action(state_) && state_elapsed_ < duration_of(state_)) {
        // A held block may be interrupted by a hit/dodge; ordinary locomotion
        // cannot cancel a committed attack or recovery before its clip ends.
        const bool attack_requested = input.attack_light_pressed || input.attack_heavy_pressed;
        const bool combo_cancel = combo_window_open_ && attack_requested;
        const bool defensive_cancel = cancel_window_open_ &&
            (input.dodge_pressed || input.block_held || input.recover_pressed);
        const bool interrupt = input.hit_reaction || combo_cancel || defensive_cancel;
        if (!interrupt) desired = state_;
    }
    if (state_ == CharacterAnimationState::Block && !input.block_held &&
        !input.hit_reaction && !input.dodge_pressed)
        desired = locomotion_state(input);

    if (desired != state_ && !enter(desired, transition_seconds_)) {
        const auto fallback = has_state(CharacterAnimationState::Idle)
            ? CharacterAnimationState::Idle : state_;
        if (fallback != state_ && !enter(fallback, transition_seconds_)) return false;
    }

    const std::size_t event_begin = events.size();
    if (!graph_.update(dt, output, events)) return false;
    for (std::size_t i = event_begin; i < events.size(); ++i) {
        const auto& name = events[i].name;
        if (name == "combo_open" || name == "combo_window_open")
            combo_window_open_ = true;
        else if (name == "combo_close" || name == "combo_window_close")
            combo_window_open_ = false;
        else if (name == "cancel_open" || name == "cancel_window_open")
            cancel_window_open_ = true;
        else if (name == "cancel_close" || name == "cancel_window_close")
            cancel_window_open_ = false;
    }
    state_elapsed_ += dt;
    was_grounded_ = input.grounded;

    if (is_timed_action(state_) && state_elapsed_ >= duration_of(state_)) {
        const auto next = locomotion_state(input);
        if (next != state_) (void)enter(next, transition_seconds_);
    }
    return true;
}

void CharacterAnimationController::reset() noexcept {
    graph_.reset();
    clips_ = {};
    state_ = CharacterAnimationState::Idle;
    state_elapsed_ = 0.0F;
    configured_ = false;
    started_ = false;
    was_grounded_ = true;
    combo_window_open_ = false;
    cancel_window_open_ = false;
}

} // namespace arut::animation
