#include "arut/animation/root_motion.hpp"

#include <cassert>
#include <cmath>
#include <iostream>

int main() {
    using namespace arut::animation;
    Pose a{}, b{};
    a.joint_count = b.joint_count = 2U;
    b.joints[0].translation = {2.0F, 1.5F, 1.0F};
    constexpr float half_yaw = 0.25F;
    b.joints[0].rotation = {0.0F, std::sin(half_yaw), 0.0F, std::cos(half_yaw)};
    RootMotionDelta delta{};
    assert(extract_root_motion(a, b, 0U, delta));
    assert(std::abs(delta.local_translation.x - 2.0F) < 0.001F);
    assert(std::abs(delta.local_translation.z - 1.0F) < 0.001F);
    assert(std::abs(delta.yaw_radians - 0.5F) < 0.001F);

    arut::core::Vec3 position{};
    float yaw = 0.0F;
    assert(apply_root_motion(delta, position, yaw));
    assert(std::abs(position.x - 2.0F) < 0.001F);
    assert(std::abs(position.z - 1.0F) < 0.001F);
    assert(std::abs(yaw - 0.5F) < 0.001F);

    Pose in_place = b;
    assert(strip_root_motion(in_place, 0U, {0.0F, 0.0F, 0.0F}));
    assert(in_place.joints[0].translation.x == 0.0F);
    assert(in_place.joints[0].translation.y == 1.5F);
    assert(in_place.joints[0].translation.z == 0.0F);
    assert(!extract_root_motion(a, b, 9U, delta));
    std::cout << "ROOT_MOTION_TEST pass planar_translation=yaw vertical_preserved\n";
}
