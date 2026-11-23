#include "arut/rendering/humanoid_ik.hpp"

#include <cassert>
#include <cmath>
#include <limits>

int main() {
    using namespace arut::rendering;
    const arut::core::Vec3 flat_normal{0.0F, 1.0F, 0.0F};

    const auto rest = solve_two_bone_leg_ik(
        0.85F, -0.10F, 0.43F, 0.42F, -0.10F, flat_normal);
    assert(rest.valid);
    assert(!rest.reach_clamped);
    assert(std::abs(rest.upper_leg_pitch) < 1.0e-4F);
    assert(std::abs(rest.lower_leg_pitch) < 1.0e-4F);
    assert(std::abs(rest.foot_pitch) < 1.0e-4F);
    assert(std::abs(rest.foot_roll) < 1.0e-4F);

    const auto adjusted = solve_two_bone_leg_ik(
        0.77F, -0.18F, 0.43F, 0.42F, -0.10F, flat_normal);
    assert(adjusted.valid);
    assert(std::abs(adjusted.upper_leg_pitch) > 0.01F);
    assert(std::abs(adjusted.lower_leg_pitch) > 0.01F);

    const arut::core::Vec3 sloped_normal{-0.20F, 0.96F, -0.18F};
    const auto slope = solve_two_bone_leg_ik(
        0.82F, -0.12F, 0.43F, 0.42F, -0.10F, sloped_normal);
    assert(slope.valid);
    assert(std::abs(slope.foot_pitch) > 0.01F);
    assert(std::abs(slope.foot_roll) > 0.01F);

    const auto too_far = solve_two_bone_leg_ik(
        2.0F, 0.0F, 0.43F, 0.42F, -0.10F, flat_normal);
    assert(too_far.valid && too_far.reach_clamped);
    assert(too_far.solved_distance < too_far.requested_distance);

    const auto invalid = solve_two_bone_leg_ik(
        std::numeric_limits<float>::quiet_NaN(), 0.0F,
        0.43F, 0.42F, -0.10F, flat_normal);
    assert(!invalid.valid);

    const auto first = quaternion_from_axis_angle({1.0F, 0.0F, 0.0F}, 0.0F);
    const auto second = quaternion_from_axis_angle({1.0F, 0.0F, 0.0F}, 1.0F);
    const auto blended = blend_humanoid_rotation(first, second, 0.5F);
    const float blended_length = std::sqrt(
        blended.x*blended.x + blended.y*blended.y +
        blended.z*blended.z + blended.w*blended.w);
    assert(std::abs(blended_length - 1.0F) < 1.0e-5F);
    return 0;
}
