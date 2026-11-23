#pragma once

#include "arut/animation/runtime.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace arut::animation {

// A compact data-driven state graph. State definitions are immutable after
// registration; each character owns its own player and transition state.
struct AnimationState final {
    std::uint32_t id{0U};
    std::string name;
    const AnimationClip* clip{nullptr};
};

class AnimationStateMachine final {
public:
    [[nodiscard]] bool add_state(std::uint32_t id, std::string name,
                                 const AnimationClip* clip);
    [[nodiscard]] bool start(std::uint32_t state_id) noexcept;
    [[nodiscard]] bool transition_to(std::uint32_t state_id,
                                     float blend_seconds) noexcept;
    [[nodiscard]] bool update(float delta_seconds, Pose& output) noexcept;
    [[nodiscard]] std::uint32_t current_state() const noexcept { return current_state_id_; }
    [[nodiscard]] bool transitioning() const noexcept { return transition_active_; }
    void reset() noexcept;

private:
    [[nodiscard]] const AnimationState* find_state(std::uint32_t id) const noexcept;
    std::vector<AnimationState> states_;
    AnimationPlayer player_;
    AnimationPlayer target_player_;
    Pose last_pose_{};
    Pose transition_source_{};
    std::uint32_t current_state_id_{0U};
    std::uint32_t target_state_id_{0U};
    float blend_duration_{0.0F};
    float blend_elapsed_{0.0F};
    bool started_{false};
    bool transition_active_{false};
};

} // namespace arut::animation
