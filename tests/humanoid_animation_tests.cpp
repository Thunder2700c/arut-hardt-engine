#include "arut/rendering/humanoid_animation.hpp"

#include <cassert>
#include <cmath>

int main() {
    using namespace arut::rendering;
    using B = arut::core::HumanoidBone;
    constexpr float dt = 1.0F / 60.0F;

    HumanoidAnimator animator;
    HumanoidAnimationInput input{};
    input.grounded = true;
    animator.update(input, dt);
    assert(animator.snapshot().state == HumanoidAnimationState::Idle);

    input.local_move_y = 1.0F;
    input.horizontal_speed = 2.0F;
    for (int i = 0; i < 30; ++i) animator.update(input, dt);
    assert(animator.snapshot().state == HumanoidAnimationState::Walk);
    assert(animator.snapshot().phase_seconds > 0.0F);
    const auto& walk_pose = animator.pose();
    const auto left_leg = walk_pose.local_rotations[static_cast<std::size_t>(B::LeftUpperLeg)];
    assert(std::abs(left_leg.x) > 0.001F);

    input.horizontal_speed = 4.4F;
    animator.update(input, dt);
    assert(animator.snapshot().state == HumanoidAnimationState::Run);

    input.local_move_x = -1.0F;
    input.local_move_y = 0.0F;
    input.horizontal_speed = 2.0F;
    animator.update(input, dt);
    assert(animator.snapshot().state == HumanoidAnimationState::StrafeLeft);

    input.local_move_x = 1.0F;
    animator.update(input, dt);
    assert(animator.snapshot().state == HumanoidAnimationState::StrafeRight);

    input.local_move_x = 0.0F;
    input.local_move_y = -1.0F;
    animator.update(input, dt);
    assert(animator.snapshot().state == HumanoidAnimationState::Backward);

    input.local_move_y = 0.0F;
    input.horizontal_speed = 0.0F;
    input.grounded = false;
    input.vertical_speed = 2.0F;
    animator.update(input, dt);
    assert(animator.snapshot().state == HumanoidAnimationState::Jump);

    input.vertical_speed = -1.0F;
    animator.update(input, dt);
    assert(animator.snapshot().state == HumanoidAnimationState::Fall);

    input.grounded = true;
    input.vertical_speed = 0.0F;
    animator.update(input, dt);
    assert(animator.snapshot().state == HumanoidAnimationState::Land);
    for (int i = 0; i < 30; ++i) animator.update(input, dt);
    assert(animator.snapshot().state == HumanoidAnimationState::Idle);

    const float phase_before = animator.snapshot().phase_seconds;
    animator.update(input, -1.0F);
    assert(animator.snapshot().phase_seconds == phase_before);
    assert(animator.snapshot().blend_alpha >= 0.0F &&
           animator.snapshot().blend_alpha <= 1.0F);
    assert(humanoid_animation_state_name(HumanoidAnimationState::Backward) == "backward");

    // The Android mannequin now has playable combat poses as well as locomotion.
    input.attack_light_pressed = true;
    animator.update(input, dt);
    assert(animator.snapshot().state == HumanoidAnimationState::AttackLight);
    input.attack_light_pressed = false;
    for (int i = 0; i < 20; ++i) animator.update(input, dt);
    assert(animator.snapshot().state == HumanoidAnimationState::AttackLight);
    const auto punch_arm = animator.pose().local_rotations[static_cast<std::size_t>(B::RightUpperArm)];
    assert(std::abs(punch_arm.x) > 0.01F);
    input.attack_heavy_pressed = true;
    animator.update(input, dt);
    assert(animator.snapshot().state == HumanoidAnimationState::AttackHeavy);
    input.attack_heavy_pressed = false;
    input.block_held = true;
    animator.update(input, dt);
    assert(animator.snapshot().state == HumanoidAnimationState::AttackHeavy);
    input.hit_reaction = true;
    animator.update(input, dt);
    assert(animator.snapshot().state == HumanoidAnimationState::HitReact);
    input.hit_reaction = false;
    input.dodge_pressed = true;
    animator.update(input, dt);
    assert(animator.snapshot().state == HumanoidAnimationState::Dodge);
    input.dodge_pressed = false;
    for (int i = 0; i < 30; ++i) animator.update(input, dt);
    assert(animator.snapshot().state == HumanoidAnimationState::Block);
    const auto guard_arm = animator.pose().local_rotations[static_cast<std::size_t>(B::RightUpperArm)];
    assert(std::abs(guard_arm.z) > 0.01F);
    input.block_held = false;
    animator.update(input, dt);
    assert(animator.snapshot().state == HumanoidAnimationState::Idle);

    return 0;
}
