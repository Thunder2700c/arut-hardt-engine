#include "arut/animation/character_animation.hpp"

#include <algorithm>
#include <cmath>
#include <cctype>
#include <string>
#include <utility>

namespace arut::animation {
namespace {
constexpr float kPi = 3.14159265358979323846F;
Quaternion multiply(Quaternion a, Quaternion b) noexcept {
    return {
        a.w*b.x + a.x*b.w + a.y*b.z - a.z*b.y,
        a.w*b.y - a.x*b.z + a.y*b.w + a.z*b.x,
        a.w*b.z + a.x*b.y - a.y*b.x + a.z*b.w,
        a.w*b.w - a.x*b.x - a.y*b.y - a.z*b.z
    };
}
Quaternion axis_angle(float x, float y, float z, float radians) noexcept {
    const float half = radians * 0.5F;
    const float sine = std::sin(half);
    return {x*sine, y*sine, z*sine, std::cos(half)};
}
std::string normalized(std::string value) {
    value.erase(std::remove_if(value.begin(), value.end(), [](unsigned char c) {
        return !std::isalnum(c);
    }), value.end());
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}
}

std::vector<AnimationClip> make_procedural_humanoid_clips(
    const Pose& rest_pose, std::span<const std::string> joint_names) {
    std::vector<AnimationClip> result;
    if (rest_pose.joint_count == 0U || rest_pose.joint_count > kMaxPoseJoints ||
        joint_names.size() != rest_pose.joint_count) return result;

    std::vector<std::string> names;
    names.reserve(joint_names.size());
    for (const auto& name : joint_names) names.push_back(normalized(name));
    auto joint = [&](const char* token) -> std::size_t {
        const std::string needle(token);
        for (std::size_t i = 0; i < names.size(); ++i)
            if (names[i].find(needle) != std::string::npos) return i;
        return rest_pose.joint_count;
    };
    const auto pelvis = joint("pelvis");
    const auto spine = joint("spine");
    const auto chest = joint("chest");
    const auto neck = joint("neck");
    const auto left_upper_arm = joint("leftarm");
    const auto right_upper_arm = joint("rightarm");
    const auto left_forearm = joint("leftforearm");
    const auto right_forearm = joint("rightforearm");
    const auto left_thigh = joint("leftupleg");
    const auto right_thigh = joint("rightupleg");
    const auto left_calf = joint("leftleg");
    const auto right_calf = joint("rightleg");

    struct Motion { const char* name; CharacterAnimationState state; float duration; bool loop; };
    constexpr Motion motions[] = {
        {"ARUT_Idle", CharacterAnimationState::Idle, 1.2F, true},
        {"ARUT_Walk", CharacterAnimationState::Walk, 0.8F, true},
        {"ARUT_Run", CharacterAnimationState::Run, 0.58F, true},
        {"ARUT_StrafeLeft", CharacterAnimationState::StrafeLeft, 0.72F, true},
        {"ARUT_StrafeRight", CharacterAnimationState::StrafeRight, 0.72F, true},
        {"ARUT_Backward", CharacterAnimationState::Backward, 0.82F, true},
        {"ARUT_Jump", CharacterAnimationState::Jump, 0.62F, false},
        {"ARUT_Fall", CharacterAnimationState::Fall, 0.7F, true},
        {"ARUT_Land", CharacterAnimationState::Land, 0.42F, false},
        {"ARUT_AttackLight", CharacterAnimationState::AttackLight, 0.48F, false},
        {"ARUT_AttackHeavy", CharacterAnimationState::AttackHeavy, 0.78F, false},
        {"ARUT_Block", CharacterAnimationState::Block, 0.8F, true},
        {"ARUT_Dodge", CharacterAnimationState::Dodge, 0.56F, false},
        {"ARUT_HitReact", CharacterAnimationState::HitReact, 0.5F, false},
        {"ARUT_Recover", CharacterAnimationState::Recover, 0.9F, false}
    };

    for (const auto& motion : motions) {
        AnimationClip clip{};
        clip.name = motion.name;
        clip.duration_seconds = motion.duration;
        clip.looping = motion.loop;
        constexpr float phases[] = {0.0F, 0.25F, 0.5F, 0.75F, 1.0F};
        for (const float phase : phases) {
            Pose pose = rest_pose;
            auto rotate = [&](std::size_t index, float x, float y, float z, float degrees) {
                if (index >= pose.joint_count || std::abs(degrees) < 0.0001F) return;
                const auto delta = axis_angle(x, y, z, degrees * kPi / 180.0F);
                pose.joints[index].rotation = multiply(rest_pose.joints[index].rotation, delta);
            };
            const float wave = std::sin(phase * 2.0F * kPi);
            const float smooth = std::sin(phase * kPi);
            switch (motion.state) {
                case CharacterAnimationState::Idle:
                    rotate(spine, 1,0,0, 1.2F*wave);
                    rotate(chest, 1,0,0, 0.8F*wave);
                    rotate(left_upper_arm, 0,0,1, -3.0F);
                    rotate(right_upper_arm, 0,0,1, 3.0F);
                    break;
                case CharacterAnimationState::Walk:
                case CharacterAnimationState::Run:
                case CharacterAnimationState::Backward: {
                    const float scale = motion.state == CharacterAnimationState::Run ? 42.0F :
                                        motion.state == CharacterAnimationState::Backward ? 18.0F : 25.0F;
                    const float direction = motion.state == CharacterAnimationState::Backward ? -1.0F : 1.0F;
                    rotate(left_thigh, 1,0,0, scale*wave*direction);
                    rotate(right_thigh, 1,0,0, -scale*wave*direction);
                    rotate(left_calf, 1,0,0, std::max(0.0F,-wave)*scale*0.8F);
                    rotate(right_calf, 1,0,0, std::max(0.0F,wave)*scale*0.8F);
                    rotate(left_upper_arm, 1,0,0, -scale*0.48F*wave);
                    rotate(right_upper_arm, 1,0,0, scale*0.48F*wave);
                    rotate(spine, 1,0,0, motion.state == CharacterAnimationState::Run ? -5.0F : -2.0F);
                    break;
                }
                case CharacterAnimationState::StrafeLeft:
                case CharacterAnimationState::StrafeRight: {
                    const float side = motion.state == CharacterAnimationState::StrafeLeft ? -1.0F : 1.0F;
                    rotate(pelvis, 0,0,1, side*5.0F*smooth);
                    rotate(spine, 0,0,1, -side*7.0F*smooth);
                    rotate(left_thigh, 0,1,0, side*10.0F*wave);
                    rotate(right_thigh, 0,1,0, side*10.0F*wave);
                    rotate(left_upper_arm, 0,0,1, -side*5.0F);
                    rotate(right_upper_arm, 0,0,1, -side*5.0F);
                    break;
                }
                case CharacterAnimationState::Jump:
                    rotate(left_thigh,1,0,0,-22.0F*smooth);
                    rotate(right_thigh,1,0,0,-22.0F*smooth);
                    rotate(left_calf,1,0,0,28.0F*smooth);
                    rotate(right_calf,1,0,0,28.0F*smooth);
                    rotate(left_upper_arm,1,0,0,-35.0F*smooth);
                    rotate(right_upper_arm,1,0,0,-35.0F*smooth);
                    rotate(spine,1,0,0,-8.0F*smooth);
                    break;
                case CharacterAnimationState::Fall:
                    rotate(left_upper_arm,0,0,1,-35.0F);
                    rotate(right_upper_arm,0,0,1,35.0F);
                    rotate(left_thigh,1,0,0,-12.0F);
                    rotate(right_thigh,1,0,0,-12.0F);
                    break;
                case CharacterAnimationState::Land:
                    rotate(spine,1,0,0,24.0F*smooth);
                    rotate(left_thigh,1,0,0,30.0F*smooth);
                    rotate(right_thigh,1,0,0,30.0F*smooth);
                    rotate(left_calf,1,0,0,35.0F*smooth);
                    rotate(right_calf,1,0,0,35.0F*smooth);
                    break;
                case CharacterAnimationState::AttackLight:
                    rotate(pelvis,0,1,0,-22.0F*smooth);
                    rotate(spine,0,1,0,-28.0F*smooth);
                    rotate(chest,0,1,0,-18.0F*smooth);
                    rotate(right_upper_arm,1,0,0,-105.0F*smooth);
                    rotate(right_forearm,1,0,0,-65.0F*smooth);
                    rotate(left_upper_arm,1,0,0,-22.0F*smooth);
                    if (phase == 0.5F) clip.events.push_back({motion.duration*0.5F,"combo_open",{}});
                    if (phase == 0.75F) clip.events.push_back({motion.duration*0.75F,"combo_close",{}});
                    break;
                case CharacterAnimationState::AttackHeavy:
                    rotate(pelvis,0,1,0,-35.0F*smooth);
                    rotate(spine,0,1,0,-42.0F*smooth);
                    rotate(chest,0,1,0,-25.0F*smooth);
                    rotate(right_upper_arm,1,0,0,-135.0F*smooth);
                    rotate(right_forearm,1,0,0,-85.0F*smooth);
                    rotate(left_upper_arm,1,0,0,-45.0F*smooth);
                    rotate(left_forearm,1,0,0,-30.0F*smooth);
                    if (phase == 0.5F) clip.events.push_back({motion.duration*0.5F,"combo_open",{}});
                    if (phase == 0.75F) clip.events.push_back({motion.duration*0.75F,"combo_close",{}});
                    break;
                case CharacterAnimationState::Block:
                    rotate(left_upper_arm,1,0,0,-65.0F);
                    rotate(right_upper_arm,1,0,0,-65.0F);
                    rotate(left_forearm,1,0,0,-75.0F);
                    rotate(right_forearm,1,0,0,-75.0F);
                    rotate(spine,1,0,0,8.0F);
                    break;
                case CharacterAnimationState::Dodge:
                    rotate(pelvis,0,0,1,32.0F*smooth);
                    rotate(spine,0,0,1,28.0F*smooth);
                    rotate(left_thigh,1,0,0,38.0F*smooth);
                    rotate(right_thigh,1,0,0,38.0F*smooth);
                    rotate(left_upper_arm,0,0,1,-25.0F);
                    rotate(right_upper_arm,0,0,1,25.0F);
                    break;
                case CharacterAnimationState::HitReact:
                    rotate(pelvis,0,1,0,18.0F*smooth);
                    rotate(spine,1,0,0,32.0F*smooth);
                    rotate(chest,1,0,0,24.0F*smooth);
                    rotate(neck,1,0,0,-18.0F*smooth);
                    rotate(left_upper_arm,0,0,1,-35.0F*smooth);
                    rotate(right_upper_arm,0,0,1,35.0F*smooth);
                    if (phase == 0.25F) clip.events.push_back({motion.duration*0.25F,"cancel_open",{}});
                    if (phase == 0.75F) clip.events.push_back({motion.duration*0.75F,"cancel_close",{}});
                    break;
                case CharacterAnimationState::Recover:
                    rotate(spine,1,0,0,26.0F*(1.0F-phase));
                    rotate(left_thigh,1,0,0,38.0F*(1.0F-phase));
                    rotate(right_thigh,1,0,0,38.0F*(1.0F-phase));
                    rotate(left_upper_arm,0,0,1,-18.0F*(1.0F-phase));
                    rotate(right_upper_arm,0,0,1,18.0F*(1.0F-phase));
                    break;
                case CharacterAnimationState::Count: break;
            }
            clip.keys.push_back({motion.duration*phase, pose});
        }
        result.push_back(std::move(clip));
    }
    return result;
}

constexpr std::uint32_t state_id(CharacterAnimationState state) noexcept {
    return static_cast<std::uint32_t>(state) + 1U;
}

bool CharacterAnimationController::configure(const CharacterAnimationSet& clips) {
    reset();
    clips_ = {};
    owned_clips_ = {};
    graph_ = AnimationStateMachine{};
    for (std::size_t i = 0; i < clips.clips.size(); ++i) {
        const auto state = static_cast<CharacterAnimationState>(i);
        const auto* source = clips.clips[i];
        if (!source) continue;
        owned_clips_[i] = *source;
        switch (state) {
            case CharacterAnimationState::Jump:
            case CharacterAnimationState::Land:
            case CharacterAnimationState::AttackLight:
            case CharacterAnimationState::AttackHeavy:
            case CharacterAnimationState::Dodge:
            case CharacterAnimationState::HitReact:
            case CharacterAnimationState::Recover:
                owned_clips_[i].looping = false;
                break;
            default: owned_clips_[i].looping = true; break;
        }
        clips_.clips[i] = &owned_clips_[i];
        const std::size_t joints = owned_clips_[i].keys.empty()
            ? 0U : owned_clips_[i].keys.front().pose.joint_count;
        if (!owned_clips_[i].valid(joints) ||
            !graph_.add_state(state_id(state), std::to_string(i), &owned_clips_[i])) {
            reset();
            return false;
        }
    }
    configured_ = has_state(CharacterAnimationState::Idle);
    return configured_;
}

bool CharacterAnimationController::start() noexcept {
    if (!configured_ || !graph_.start(state_id(CharacterAnimationState::Idle))) return false;
    state_ = CharacterAnimationState::Idle;
    state_elapsed_ = input_buffer_seconds_ = 0.0F;
    buffered_action_ = CharacterAnimationState::Count;
    was_grounded_ = true;
    started_ = true;
    return true;
}
bool CharacterAnimationController::has_state(CharacterAnimationState state) const noexcept {
    return clips_.get(state) != nullptr;
}
CharacterAnimationState CharacterAnimationController::locomotion_state(
    const CharacterAnimationInput& input) const noexcept {
    if (!input.grounded) return input.vertical_speed > 0.15F
        ? CharacterAnimationState::Jump : CharacterAnimationState::Fall;
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
    if (input.hit_reaction && has_state(CharacterAnimationState::HitReact)) return CharacterAnimationState::HitReact;
    if (input.dodge_pressed && has_state(CharacterAnimationState::Dodge)) return CharacterAnimationState::Dodge;
    if (input.recover_pressed && has_state(CharacterAnimationState::Recover)) return CharacterAnimationState::Recover;
    if (input.block_held && has_state(CharacterAnimationState::Block)) return CharacterAnimationState::Block;
    if (input.attack_heavy_pressed && has_state(CharacterAnimationState::AttackHeavy)) return CharacterAnimationState::AttackHeavy;
    if (input.attack_light_pressed && has_state(CharacterAnimationState::AttackLight)) return CharacterAnimationState::AttackLight;
    const auto locomotion = locomotion_state(input);
    if (!was_grounded_ && input.grounded && has_state(CharacterAnimationState::Land)) return CharacterAnimationState::Land;
    if (has_state(locomotion)) return locomotion;
    if (!input.grounded && has_state(CharacterAnimationState::Fall)) return CharacterAnimationState::Fall;
    return CharacterAnimationState::Idle;
}
bool CharacterAnimationController::is_timed_action(CharacterAnimationState state) const noexcept {
    switch (state) {
        case CharacterAnimationState::Land: case CharacterAnimationState::AttackLight:
        case CharacterAnimationState::AttackHeavy: case CharacterAnimationState::Dodge:
        case CharacterAnimationState::HitReact: case CharacterAnimationState::Recover: return true;
        default: return false;
    }
}
float CharacterAnimationController::duration_of(CharacterAnimationState state) const noexcept {
    const auto* clip = clips_.get(state);
    return clip ? clip->duration_seconds : 0.0F;
}
bool CharacterAnimationController::enter(CharacterAnimationState next, float blend_seconds) noexcept {
    if (!has_state(next)) return false;
    if (next == state_) return true;
    if (!graph_.transition_to(state_id(next), blend_seconds)) return false;
    state_ = next;
    state_elapsed_ = 0.0F;
    combo_window_open_ = cancel_window_open_ = false;
    return true;
}
bool CharacterAnimationController::update(float delta_seconds, const CharacterAnimationInput& input,
    Pose& output, std::vector<AnimationEvent>& events) {
    if (!started_ || !std::isfinite(delta_seconds) || delta_seconds < 0.0F ||
        !std::isfinite(input.move_x) || !std::isfinite(input.move_y) ||
        !std::isfinite(input.horizontal_speed) || !std::isfinite(input.vertical_speed)) return false;
    const float dt = std::min(delta_seconds, 0.25F);
    const bool attack_pressed = input.attack_light_pressed || input.attack_heavy_pressed;
    if (attack_pressed) {
        buffered_action_ = input.attack_heavy_pressed ? CharacterAnimationState::AttackHeavy
                                                       : CharacterAnimationState::AttackLight;
        input_buffer_seconds_ = 0.22F;
    } else if (input_buffer_seconds_ > 0.0F) {
        input_buffer_seconds_ = std::max(0.0F, input_buffer_seconds_ - dt);
        if (input_buffer_seconds_ == 0.0F) buffered_action_ = CharacterAnimationState::Count;
    }
    CharacterAnimationState desired = choose_state(input);
    const bool buffered_attack = buffered_action_ != CharacterAnimationState::Count &&
        has_state(buffered_action_) && input_buffer_seconds_ > 0.0F;
    if (is_timed_action(state_) && state_elapsed_ < duration_of(state_)) {
        const bool combo_cancel = combo_window_open_ && buffered_attack;
        const bool defensive_cancel = cancel_window_open_ &&
            (input.dodge_pressed || input.block_held || input.recover_pressed);
        if (input.hit_reaction) desired = CharacterAnimationState::HitReact;
        // Defensive cancels outrank an offensive combo buffer. If both are
        // pressed in the same frame, dodge/guard/recovery must not be eaten
        // by the queued next strike.
        else if (defensive_cancel) desired = input.dodge_pressed
            ? CharacterAnimationState::Dodge : input.block_held
            ? CharacterAnimationState::Block : CharacterAnimationState::Recover;
        else if (combo_cancel) desired = buffered_action_;
        else desired = state_;
    }
    if (state_ == CharacterAnimationState::Block && !input.block_held &&
        !input.hit_reaction && !input.dodge_pressed) desired = locomotion_state(input);
    if (desired == state_ && is_timed_action(state_) &&
        combo_window_open_ && buffered_attack &&
        (state_ == CharacterAnimationState::AttackLight ||
         state_ == CharacterAnimationState::AttackHeavy)) {
        if (!graph_.restart_current(transition_seconds_)) return false;
        state_elapsed_ = 0.0F;
        combo_window_open_ = cancel_window_open_ = false;
        buffered_action_ = CharacterAnimationState::Count;
        input_buffer_seconds_ = 0.0F;
    } else if (desired != state_) {
        if (!enter(desired, transition_seconds_)) {
            const auto fallback = has_state(CharacterAnimationState::Idle)
                ? CharacterAnimationState::Idle : state_;
            if (fallback != state_ && !enter(fallback, transition_seconds_)) return false;
        } else if (desired == CharacterAnimationState::AttackLight ||
                   desired == CharacterAnimationState::AttackHeavy) {
            buffered_action_ = CharacterAnimationState::Count;
            input_buffer_seconds_ = 0.0F;
        }
    }
    const std::size_t event_begin = events.size();
    if (!graph_.update(dt, output, events)) return false;
    for (std::size_t i = event_begin; i < events.size(); ++i) {
        const auto& name = events[i].name;
        if (name == "combo_open" || name == "combo_window_open") combo_window_open_ = true;
        else if (name == "combo_close" || name == "combo_window_close") combo_window_open_ = false;
        else if (name == "cancel_open" || name == "cancel_window_open") cancel_window_open_ = true;
        else if (name == "cancel_close" || name == "cancel_window_close") cancel_window_open_ = false;
    }
    state_elapsed_ += dt;
    was_grounded_ = input.grounded;
    if (is_timed_action(state_) && state_elapsed_ >= duration_of(state_)) {
        // A held guard is a persistent stance, not a one-shot action. Return
        // directly to it after an attack/landing/recovery instead of briefly
        // dropping to locomotion for one frame.
        const auto next = input.block_held && input.grounded &&
                          has_state(CharacterAnimationState::Block)
            ? CharacterAnimationState::Block : locomotion_state(input);
        if (next != state_) (void)enter(next, transition_seconds_);
    }
    return true;
}
void CharacterAnimationController::reset() noexcept {
    graph_.reset(); clips_ = {}; owned_clips_ = {};
    state_ = CharacterAnimationState::Idle;
    state_elapsed_ = input_buffer_seconds_ = 0.0F;
    buffered_action_ = CharacterAnimationState::Count;
    configured_ = started_ = false;
    was_grounded_ = true;
    combo_window_open_ = cancel_window_open_ = false;
}
} // namespace arut::animation
