#include "arut/animation/character_animation.hpp"

#include <array>
#include <cassert>
#include <cmath>
#include <string>
#include <vector>

using namespace arut::animation;

int main() {
    // Match the default imported humanoid's joint count while using stable,
    // representative humanoid names for the joints that drive the fallback.
    constexpr std::size_t kDefaultHumanJointCount = 66U;
    Pose rest{};
    rest.joint_count = kDefaultHumanJointCount;
    std::vector<std::string> names(kDefaultHumanJointCount);
    for (std::size_t i = 0; i < names.size(); ++i)
        names[i] = "unmapped_joint_" + std::to_string(i);

    names[0] = "pelvis";
    names[1] = "spine_01";
    names[2] = "chest";
    names[3] = "neck";
    names[4] = "leftArm";
    names[5] = "rightArm";
    names[6] = "leftForeArm";
    names[7] = "rightForeArm";
    names[8] = "leftUpLeg";
    names[9] = "rightUpLeg";
    names[10] = "leftLeg";
    names[11] = "rightLeg";

    auto clips = make_procedural_humanoid_clips(rest, names);
    assert(clips.size() == kCharacterAnimationStateCount);

    CharacterAnimationSet set{};
    constexpr std::array<CharacterAnimationState, kCharacterAnimationStateCount> states{
        CharacterAnimationState::Idle, CharacterAnimationState::Walk,
        CharacterAnimationState::Run, CharacterAnimationState::StrafeLeft,
        CharacterAnimationState::StrafeRight, CharacterAnimationState::Backward,
        CharacterAnimationState::Jump, CharacterAnimationState::Fall,
        CharacterAnimationState::Land, CharacterAnimationState::AttackLight,
        CharacterAnimationState::AttackHeavy, CharacterAnimationState::AttackKick,
        CharacterAnimationState::Block, CharacterAnimationState::Dodge,
        CharacterAnimationState::HitReact, CharacterAnimationState::Recover
    };

    for (std::size_t i = 0; i < clips.size(); ++i) {
        Pose sampled{};
        assert(clips[i].valid(kDefaultHumanJointCount));
        assert(clips[i].sample(clips[i].duration_seconds * 0.5F, sampled));
        assert(sampled.joint_count == kDefaultHumanJointCount);
        for (std::size_t joint = 0; joint < sampled.joint_count; ++joint) {
            const auto& transform = sampled.joints[joint];
            assert(std::isfinite(transform.translation.x));
            assert(std::isfinite(transform.translation.y));
            assert(std::isfinite(transform.translation.z));
            assert(std::isfinite(transform.rotation.x));
            assert(std::isfinite(transform.rotation.y));
            assert(std::isfinite(transform.rotation.z));
            assert(std::isfinite(transform.rotation.w));
        }
        set.set(states[i], &clips[i]);
    }

    CharacterAnimationController controller;
    assert(controller.configure(set));
    assert(controller.start());

    Pose output{};
    std::vector<AnimationEvent> events;
    CharacterAnimationInput input{};
    assert(controller.update(1.0F / 60.0F, input, output, events));
    assert(output.joint_count == kDefaultHumanJointCount);

    input.move_y = 1.0F;
    input.horizontal_speed = 2.0F;
    for (int frame = 0; frame < 20; ++frame)
        assert(controller.update(1.0F / 60.0F, input, output, events));
    assert(controller.state() == CharacterAnimationState::Walk);
    assert(output.joint_count == kDefaultHumanJointCount);

    input.attack_light_pressed = true;
    assert(controller.update(1.0F / 60.0F, input, output, events));
    input.attack_light_pressed = false;
    assert(controller.state() == CharacterAnimationState::AttackLight);
    assert(output.joint_count == kDefaultHumanJointCount);

    // A hit reaction interrupts the attack through the existing authored
    // cancel path; the 66-joint pose remains valid throughout the transition.
    for (int frame = 0; frame < 20; ++frame)
        assert(controller.update(1.0F / 60.0F, input, output, events));
    input.hit_reaction = true;
    assert(controller.update(1.0F / 60.0F, input, output, events));
    assert(controller.state() == CharacterAnimationState::HitReact);
    assert(output.joint_count == kDefaultHumanJointCount);

    return 0;
}
