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

    // The procedural mannequin must hold an early combo press until its
    // authored combo window opens, using the same 220 ms buffer as the graph path.
    HumanoidAnimator buffered_animator;
    HumanoidAnimationInput buffered_input{};
    buffered_input.grounded = true;
    buffered_input.attack_light_pressed = true;
    (void)buffered_animator.update(buffered_input, dt);
    assert(buffered_animator.snapshot().state == HumanoidAnimationState::AttackLight);
    buffered_input.attack_light_pressed = false;
    for (int i = 0; i < 10; ++i) (void)buffered_animator.update(buffered_input, dt);
    buffered_input.attack_heavy_pressed = true;
    (void)buffered_animator.update(buffered_input, dt);
    assert(buffered_animator.snapshot().state == HumanoidAnimationState::AttackLight);
    buffered_input.attack_heavy_pressed = false;
    for (int i = 0; i < 12 &&
         buffered_animator.snapshot().state == HumanoidAnimationState::AttackLight; ++i)
        (void)buffered_animator.update(buffered_input, dt);
    assert(buffered_animator.snapshot().state == HumanoidAnimationState::AttackHeavy);

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
    const auto jab_forearm = animator.pose().local_rotations[static_cast<std::size_t>(B::RightLowerArm)];
    const auto jab_chest = animator.pose().local_rotations[static_cast<std::size_t>(B::Chest)];
    assert(std::abs(jab_forearm.x) > 0.01F);
    assert(std::abs(jab_chest.y) > 0.01F);
    assert(std::abs(jab_chest.x) > 0.01F);
    // Sample the heavy punch on a fresh animator so its authored wind-up
    // and extension are measured independently of the preceding jab blend.
    HumanoidAnimator heavy_animator;
    HumanoidAnimationInput heavy_input{};
    heavy_input.grounded = true;
    heavy_input.attack_heavy_pressed = true;
    (void)heavy_animator.update(heavy_input, dt);
    assert(heavy_animator.snapshot().state == HumanoidAnimationState::AttackHeavy);
    heavy_input.attack_heavy_pressed = false;
    for (int i = 0; i < 20; ++i) (void)heavy_animator.update(heavy_input, dt);
    assert(heavy_animator.snapshot().action_phase > 0.40F);
    const auto heavy_arm = heavy_animator.pose().local_rotations[static_cast<std::size_t>(B::RightUpperArm)];
    const auto heavy_elbow = heavy_animator.pose().local_rotations[static_cast<std::size_t>(B::RightLowerArm)];
    assert(std::abs(heavy_arm.x) > 0.10F);
    assert(std::abs(heavy_elbow.x) > 0.10F);

    input.attack_heavy_pressed = true;
    (void)animator.update(input, dt);
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
    (void)animator.update(input, dt);
    assert(animator.snapshot().state == HumanoidAnimationState::Idle);

    // A kick uses the right leg and counterbalances with both arms. Guard may
    // cancel it after the initial commitment window, then recovery is explicit.
    input.attack_kick_pressed = true;
    (void)animator.update(input, dt);
    assert(animator.snapshot().state == HumanoidAnimationState::AttackKick);
    input.attack_kick_pressed = false;
    for (int i = 0; i < 18; ++i) (void)animator.update(input, dt);
    const auto kick_thigh = animator.pose().local_rotations[static_cast<std::size_t>(B::RightUpperLeg)];
    const auto kick_calf = animator.pose().local_rotations[static_cast<std::size_t>(B::RightLowerLeg)];
    const auto kick_foot = animator.pose().local_rotations[static_cast<std::size_t>(B::RightFoot)];
    assert(std::abs(kick_thigh.x) > 0.05F);
    assert(std::abs(kick_calf.x) > 0.05F);
    assert(std::abs(kick_foot.x) > 0.01F);
    input.block_held = true;
    input.attack_light_pressed = true;
    (void)animator.update(input, dt);
    assert(animator.snapshot().state == HumanoidAnimationState::AttackKick);
    (void)animator.update(input, dt);
    assert(animator.snapshot().state == HumanoidAnimationState::AttackKick);
    (void)animator.update(input, dt);
    assert(animator.snapshot().state == HumanoidAnimationState::AttackKick);
    (void)animator.update(input, dt);
    assert(animator.snapshot().state == HumanoidAnimationState::Block);
    input.block_held = false;
    input.attack_light_pressed = false;
    (void)animator.update(input, dt);
    assert(animator.snapshot().state == HumanoidAnimationState::Idle);
    input.recover_pressed = true;
    (void)animator.update(input, dt);
    assert(animator.snapshot().state == HumanoidAnimationState::Recover);

    return 0;
}
