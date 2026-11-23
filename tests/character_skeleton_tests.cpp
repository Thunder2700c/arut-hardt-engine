#include "arut/core/humanoid_skeleton.hpp"

#include <cmath>
#include <iostream>

int main() {
    using namespace arut::core;
    const auto skeleton = make_robot_mannequin_skeleton();
    if (!skeleton.valid() || skeleton.bones.size() != 20U) {
        std::cerr << "built-in humanoid skeleton is invalid\n";
        return 1;
    }
    const auto& root = skeleton.bones[static_cast<std::size_t>(HumanoidBone::Root)];
    const auto& head = skeleton.bones[static_cast<std::size_t>(HumanoidBone::Head)];
    const auto& left_foot = skeleton.bones[static_cast<std::size_t>(HumanoidBone::LeftFoot)];
    if (root.parent != -1 || head.parent != static_cast<std::int16_t>(HumanoidBone::Neck) ||
        left_foot.parent != static_cast<std::int16_t>(HumanoidBone::LeftLowerLeg) ||
        !(head.rest_translation.y > 0.0F) || !(left_foot.visual_radius > 0.0F)) {
        std::cerr << "humanoid hierarchy or rest pose contract failed\n";
        return 2;
    }
    auto invalid = skeleton;
    invalid.bones[3].parent = 19;
    if (invalid.valid()) {
        std::cerr << "invalid forward/cyclic parent reference was accepted\n";
        return 3;
    }
    std::cout << "Phase 4 skeleton tests passed: bones=" << skeleton.bones.size() << '\n';
    return 0;
}
