#include "arut/animation/graph.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace arut::animation {

bool AnimationStateMachine::add_state(std::uint32_t id, std::string name,
                                      const AnimationClip* clip) {
    if (id == 0U || name.empty() || !clip ||
        !clip->valid(clip->keys.empty() ? 0U : clip->keys.front().pose.joint_count) ||
        find_state(id)) return false;
    states_.push_back({id, std::move(name), clip});
    return true;
}

const AnimationState* AnimationStateMachine::find_state(std::uint32_t id) const noexcept {
    const auto it = std::find_if(states_.begin(), states_.end(),
        [id](const AnimationState& state) { return state.id == id; });
    return it == states_.end() ? nullptr : &*it;
}

bool AnimationStateMachine::start(std::uint32_t state_id) noexcept {
    const auto* state = find_state(state_id);
    if (!state) return false;
    player_.set_clip(state->clip, true);
    current_state_id_ = state_id;
    target_state_id_ = 0U;
    transition_active_ = false;
    blend_duration_ = blend_elapsed_ = 0.0F;
    started_ = true;
    return true;
}

bool AnimationStateMachine::transition_to(std::uint32_t state_id,
                                          float blend_seconds) noexcept {
    if (!started_ || !std::isfinite(blend_seconds) || blend_seconds < 0.0F)
        return false;
    const auto* target = find_state(state_id);
    if (!target) return false;
    if (state_id == current_state_id_ && !transition_active_) return true;
    if (!last_pose_.joint_count) return false;

    transition_source_ = last_pose_;
    target_player_.set_clip(target->clip, true);
    target_state_id_ = state_id;
    blend_duration_ = blend_seconds;
    blend_elapsed_ = 0.0F;
    transition_active_ = blend_seconds > 0.0F;
    if (!transition_active_) {
        player_ = target_player_;
        current_state_id_ = target_state_id_;
        target_state_id_ = 0U;
    }
    return true;
}

bool AnimationStateMachine::restart_current(float blend_seconds) noexcept {
    if (!started_ || !std::isfinite(blend_seconds) || blend_seconds < 0.0F ||
        !last_pose_.joint_count) return false;
    const auto* state = find_state(current_state_id_);
    if (!state || !state->clip) return false;
    transition_source_ = last_pose_;
    target_player_.set_clip(state->clip, true);
    target_state_id_ = current_state_id_;
    blend_duration_ = blend_seconds;
    blend_elapsed_ = 0.0F;
    transition_active_ = blend_seconds > 0.0F;
    if (!transition_active_) {
        player_ = target_player_;
        target_state_id_ = 0U;
    }
    return true;
}

bool AnimationStateMachine::update(float delta_seconds, Pose& output) noexcept {
    if (!started_ || !std::isfinite(delta_seconds) || delta_seconds < 0.0F)
        return false;
    if (!transition_active_) {
        if (!player_.update(delta_seconds, output)) return false;
        last_pose_ = output;
        return true;
    }

    Pose target_pose{};
    if (!target_player_.update(delta_seconds, target_pose)) return false;
    blend_elapsed_ = std::min(blend_duration_, blend_elapsed_ + delta_seconds);
    const float alpha = blend_duration_ > 0.0F
        ? blend_elapsed_ / blend_duration_ : 1.0F;
    if (!blend_poses(transition_source_, target_pose, alpha, output)) return false;
    last_pose_ = output;
    if (alpha >= 1.0F) {
        player_ = target_player_;
        current_state_id_ = target_state_id_;
        target_state_id_ = 0U;
        transition_active_ = false;
    }
    return true;
}

bool AnimationStateMachine::update(float delta_seconds, Pose& output,
                                   std::vector<AnimationEvent>& output_events) {
    if (!started_ || !std::isfinite(delta_seconds) || delta_seconds < 0.0F) return false;
    if (!transition_active_) {
        if (!player_.update(delta_seconds, output, output_events)) return false;
        last_pose_ = output;
        return true;
    }
    Pose target_pose{};
    if (!target_player_.update(delta_seconds, target_pose, output_events)) return false;
    blend_elapsed_ = std::min(blend_duration_, blend_elapsed_ + delta_seconds);
    const float alpha = blend_duration_ > 0.0F ? blend_elapsed_ / blend_duration_ : 1.0F;
    if (!blend_poses(transition_source_, target_pose, alpha, output)) return false;
    last_pose_ = output;
    if (alpha >= 1.0F) {
        player_ = target_player_;
        current_state_id_ = target_state_id_;
        target_state_id_ = 0U;
        transition_active_ = false;
    }
    return true;
}
void AnimationStateMachine::reset() noexcept {
    player_ = AnimationPlayer{};
    target_player_ = AnimationPlayer{};
    last_pose_ = Pose{};
    transition_source_ = Pose{};
    current_state_id_ = target_state_id_ = 0U;
    blend_duration_ = blend_elapsed_ = 0.0F;
    started_ = transition_active_ = false;
}

} // namespace arut::animation
