#include "arut/animation/character_animation.hpp"

#include <cassert>
#include <iostream>

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
    auto idle = make_clip("idle", 0.0F);
    auto walk = make_clip("walk", 2.0F);
    auto run = make_clip("run", 4.0F);
    auto attack = make_clip("attack", 6.0F, false);
    auto dodge = make_clip("dodge", 8.0F, false);
    auto hit = make_clip("hit", 10.0F, false);
    CharacterAnimationSet set{};
    set.set(CharacterAnimationState::Idle, &idle);
    set.set(CharacterAnimationState::Walk, &walk);
    set.set(CharacterAnimationState::Run, &run);
    set.set(CharacterAnimationState::AttackLight, &attack);
    set.set(CharacterAnimationState::Dodge, &dodge);
    set.set(CharacterAnimationState::HitReact, &hit);

    CharacterAnimationController controller;
    assert(controller.configure(set));
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

    input.hit_reaction = true;
    assert(controller.update(0.1F, input, pose, events));
    assert(controller.state() == CharacterAnimationState::HitReact);
    input.hit_reaction = false;
    input.dodge_pressed = true;
    assert(controller.update(0.1F, input, pose, events));
    assert(controller.state() == CharacterAnimationState::Dodge);
    input.dodge_pressed = false;
    assert(controller.update(0.5F, input, pose, events));
    assert(controller.state() == CharacterAnimationState::Idle ||
           controller.state() == CharacterAnimationState::Walk);
    std::cout << "CHARACTER_ANIMATION_TEST pass locomotion=1 combat_priority=1 timed_actions=1\n";
}
