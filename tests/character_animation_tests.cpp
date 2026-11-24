#include "arut/animation/character_animation.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

using namespace arut::animation;

static AnimationClip make_clip(const char* name, float offset, bool looping = true) {
    AnimationClip clip{};
    clip.name = name;
    clip.duration_seconds = 0.5F;
    clip.looping = looping;
    Pose a{}, b{};
    a.joint_count = b.joint_count = 2U;
    a.joints[0].translation.x = offset;
    b.joints[0].translation.x = offset + 1.0F;
    clip.keys.push_back({0.0F, a});
    clip.keys.push_back({0.5F, b});
    clip.events.push_back({0.1F, "marker", name});
    return clip;
}

int main() {
    Pose generated_rest{};
    generated_rest.joint_count = 6U;
    const std::vector<std::string> generated_names{
        "pelvis", "spine_01", "leftArm", "rightArm", "leftForeArm", "rightForeArm"};
    const auto generated = make_procedural_humanoid_clips(generated_rest, generated_names);
    assert(generated.size() == kCharacterAnimationStateCount);
    const auto generated_attack = std::find_if(generated.begin(), generated.end(),
        [](const AnimationClip& clip) { return clip.name == "ARUT_AttackLight"; });
    assert(generated_attack != generated.end());
    Pose generated_pose{};
    assert(generated_attack->sample(generated_attack->duration_seconds * 0.5F, generated_pose));
    assert(std::abs(generated_pose.joints[2].rotation.x) > 0.01F ||
           std::abs(generated_pose.joints[2].rotation.y) > 0.01F ||
           std::abs(generated_pose.joints[2].rotation.z) > 0.01F);
    auto idle = make_clip("idle", 0.0F);
    auto walk = make_clip("walk", 2.0F);
    auto run = make_clip("run", 4.0F);
    auto attack = make_clip("attack", 6.0F, true);
    auto dodge = make_clip("dodge", 8.0F, false);
    auto hit = make_clip("hit", 10.0F, false);
    auto heavy = make_clip("heavy", 12.0F, true);
    auto strafe = make_clip("strafe", 14.0F);
    auto jump = make_clip("jump", 16.0F);
    auto fall = make_clip("fall", 18.0F);
    auto land = make_clip("land", 20.0F, false);
    auto block = make_clip("block", 22.0F);
    auto recover = make_clip("recover", 24.0F, false);
    attack.events.push_back({0.15F, "combo_open", {}});
    attack.events.push_back({0.30F, "combo_close", {}});
    hit.events.push_back({0.15F, "cancel_open", {}});

    CharacterAnimationSet set{};
    set.set(CharacterAnimationState::Idle, &idle);
    set.set(CharacterAnimationState::Walk, &walk);
    set.set(CharacterAnimationState::Run, &run);
    set.set(CharacterAnimationState::AttackLight, &attack);
    set.set(CharacterAnimationState::Dodge, &dodge);
    set.set(CharacterAnimationState::HitReact, &hit);
    set.set(CharacterAnimationState::AttackHeavy, &heavy);
    set.set(CharacterAnimationState::StrafeRight, &strafe);
    set.set(CharacterAnimationState::Jump, &jump);
    set.set(CharacterAnimationState::Fall, &fall);
    set.set(CharacterAnimationState::Land, &land);
    set.set(CharacterAnimationState::Block, &block);
    set.set(CharacterAnimationState::Recover, &recover);

    CharacterAnimationController controller;
    assert(controller.configure(set));
    assert(attack.looping && heavy.looping); // Controller owns non-looping action copies.
    assert(controller.start());
    Pose pose{};
    std::vector<AnimationEvent> events;
    CharacterAnimationInput input{};
    assert(controller.update(0.1F, input, pose, events));
    assert(controller.state() == CharacterAnimationState::Idle);

    input.move_y = 1.0F;
    input.horizontal_speed = 2.0F;
    assert(controller.update(0.1F, input, pose, events));
    assert(controller.state() == CharacterAnimationState::Walk);

    input.attack_light_pressed = true;
    assert(controller.update(0.1F, input, pose, events));
    assert(controller.state() == CharacterAnimationState::AttackLight);
    input.attack_light_pressed = false;
    input.move_y = 0.0F;
    assert(controller.update(0.1F, input, pose, events));
    assert(controller.state() == CharacterAnimationState::AttackLight);
    bool combo_open_seen = false;
    for (const auto& event : events) combo_open_seen |= event.name == "combo_open";
    assert(combo_open_seen);

    // Buffer the same attack before the authored combo window and verify the
    // active clip restarts instead of remaining stuck in its first strike.
    input.attack_light_pressed = true;
    assert(controller.update(0.1F, input, pose, events));
    assert(controller.state() == CharacterAnimationState::AttackLight);
    assert(controller.state_time_seconds() < 0.15F);
    input.attack_light_pressed = false;
    // Advance into the restarted strike's next authored combo window.
    assert(controller.update(0.1F, input, pose, events));

    input.attack_heavy_pressed = true;
    assert(controller.update(0.1F, input, pose, events));
    assert(controller.state() == CharacterAnimationState::AttackHeavy);
    input.attack_heavy_pressed = false;

    input.hit_reaction = true;
    assert(controller.update(0.1F, input, pose, events));
    assert(controller.state() == CharacterAnimationState::HitReact);
    input.hit_reaction = false;
    assert(controller.update(0.1F, input, pose, events));
    bool cancel_open_seen = false;
    for (const auto& event : events) cancel_open_seen |= event.name == "cancel_open";
    assert(cancel_open_seen);

    input.dodge_pressed = true;
    assert(controller.update(0.1F, input, pose, events));
    assert(controller.state() == CharacterAnimationState::Dodge);
    input.dodge_pressed = false;
    assert(controller.update(0.25F, input, pose, events));
    assert(controller.state() == CharacterAnimationState::Dodge);
    assert(controller.update(0.25F, input, pose, events));
    assert(controller.state() == CharacterAnimationState::Idle);

    input.move_x = 1.0F;
    assert(controller.update(0.1F, input, pose, events));
    assert(controller.state() == CharacterAnimationState::StrafeRight);
    input.move_x = 0.0F;
    input.move_y = 1.0F;
    input.sprinting = true;
    input.horizontal_speed = 4.0F;
    assert(controller.update(0.1F, input, pose, events));
    assert(controller.state() == CharacterAnimationState::Run);
    input.grounded = false;
    input.vertical_speed = 2.0F;
    assert(controller.update(0.1F, input, pose, events));
    assert(controller.state() == CharacterAnimationState::Jump);
    input.vertical_speed = -1.0F;
    assert(controller.update(0.1F, input, pose, events));
    assert(controller.state() == CharacterAnimationState::Fall);
    input.grounded = true;
    input.vertical_speed = 0.0F;
    assert(controller.update(0.1F, input, pose, events));
    assert(controller.state() == CharacterAnimationState::Land);

    // Landing/recovery must respect a held guard when the timed action ends.
    input.block_held = true;
    assert(controller.update(0.1F, input, pose, events));
    assert(controller.state() == CharacterAnimationState::Land);
    assert(controller.update(0.4F, input, pose, events));
    assert(controller.state() == CharacterAnimationState::Block);
    assert(controller.update(0.1F, input, pose, events));
    assert(controller.state() == CharacterAnimationState::Block);

    // A hit reaction outranks guard, then the authored cancel window allows
    // a defensive dodge; releasing guard returns to the current locomotion.
    input.block_held = false;
    input.hit_reaction = true;
    assert(controller.update(0.1F, input, pose, events));
    assert(controller.state() == CharacterAnimationState::HitReact);
    input.hit_reaction = false;
    assert(controller.update(0.2F, input, pose, events));
    input.block_held = true;
    assert(controller.update(0.1F, input, pose, events));
    assert(controller.state() == CharacterAnimationState::Block);
    input.block_held = false;
    input.dodge_pressed = true;
    assert(controller.update(0.1F, input, pose, events));
    assert(controller.state() == CharacterAnimationState::Dodge);
    input.dodge_pressed = false;
    assert(controller.update(0.6F, input, pose, events));
    assert(controller.state() == CharacterAnimationState::Idle);

    std::cout << "CHARACTER_ANIMATION_TEST pass locomotion=1 combat_priority=1 timed_actions=1 guard_reentry=1 hit_cancel=1\n";
}
