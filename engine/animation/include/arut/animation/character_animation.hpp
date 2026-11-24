#pragma once

#include "arut/animation/graph.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace arut::animation {

enum class CharacterAnimationState : std::uint8_t {
    Idle, Walk, Run, StrafeLeft, StrafeRight, Backward, Jump, Fall, Land,
    AttackLight, AttackHeavy, AttackKick, Block, Dodge, HitReact, Recover, Count
};

inline constexpr std::size_t kCharacterAnimationStateCount =
    static_cast<std::size_t>(CharacterAnimationState::Count);

struct CharacterAnimationSet final {
    std::array<const AnimationClip*, kCharacterAnimationStateCount> clips{};

    void set(CharacterAnimationState state, const AnimationClip* clip) noexcept {
        const auto index = static_cast<std::size_t>(state);
        if (index < clips.size()) clips[index] = clip;
    }
    [[nodiscard]] const AnimationClip* get(CharacterAnimationState state) const noexcept {
        const auto index = static_cast<std::size_t>(state);
        return index < clips.size() ? clips[index] : nullptr;
    }
};

struct CharacterAnimationInput final {
    float move_x{0.0F};
    float move_y{0.0F};
    float horizontal_speed{0.0F};
    float vertical_speed{0.0F};
    bool grounded{true};
    bool sprinting{false};
    bool attack_light_pressed{false};
    bool attack_heavy_pressed{false};
    bool attack_kick_pressed{false};
    bool block_held{false};
    bool dodge_pressed{false};
    bool hit_reaction{false};
    bool recover_pressed{false};
};

// Builds an original, lightweight keyframed fallback library for rigs whose
// imported GLB contains only an idle clip. Bone matching is name-based and
// safely leaves unmatched joints at their authored rest transforms.
[[nodiscard]] std::vector<AnimationClip> make_procedural_humanoid_clips(
    const Pose& rest_pose, std::span<const std::string> joint_names);

class CharacterAnimationController final {
public:
    CharacterAnimationController() = default;
    CharacterAnimationController(const CharacterAnimationController&) = delete;
    CharacterAnimationController& operator=(const CharacterAnimationController&) = delete;
    CharacterAnimationController(CharacterAnimationController&&) = delete;
    CharacterAnimationController& operator=(CharacterAnimationController&&) = delete;

    [[nodiscard]] bool configure(const CharacterAnimationSet& clips);
    [[nodiscard]] bool start() noexcept;
    [[nodiscard]] bool update(float delta_seconds, const CharacterAnimationInput& input,
                              Pose& output, std::vector<AnimationEvent>& events);
    void reset() noexcept;

    [[nodiscard]] CharacterAnimationState state() const noexcept { return state_; }
    [[nodiscard]] bool active() const noexcept { return configured_ && started_; }
    [[nodiscard]] float state_time_seconds() const noexcept { return state_elapsed_; }
    [[nodiscard]] bool has_state(CharacterAnimationState state) const noexcept;

private:
    [[nodiscard]] CharacterAnimationState locomotion_state(
        const CharacterAnimationInput& input) const noexcept;
    [[nodiscard]] CharacterAnimationState choose_state(
        const CharacterAnimationInput& input) const noexcept;
    [[nodiscard]] bool is_timed_action(CharacterAnimationState state) const noexcept;
    [[nodiscard]] float duration_of(CharacterAnimationState state) const noexcept;
    [[nodiscard]] bool enter(CharacterAnimationState next, float blend_seconds) noexcept;

    CharacterAnimationSet clips_{};
    std::array<AnimationClip, kCharacterAnimationStateCount> owned_clips_{};
    AnimationStateMachine graph_{};
    CharacterAnimationState state_{CharacterAnimationState::Idle};
    float state_elapsed_{0.0F};
    float transition_seconds_{0.12F};
    float input_buffer_seconds_{0.0F};
    CharacterAnimationState buffered_action_{CharacterAnimationState::Count};
    bool configured_{false};
    bool started_{false};
    bool was_grounded_{true};
    bool combo_window_open_{false};
    bool cancel_window_open_{false};
};

} // namespace arut::animation
