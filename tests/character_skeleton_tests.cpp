#include "arut/core/humanoid_skeleton.hpp"
#include "arut/rendering/robot_mannequin.hpp"

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

    const auto mannequin = arut::rendering::make_robot_mannequin_mesh(skeleton);
    if (mannequin.vertices.empty() || mannequin.indices.empty() ||
        mannequin.indices.size() % 3U != 0U) {
        std::cerr << "procedural mannequin mesh is empty or not triangular\n";
        return 4;
    }
    for (const auto index : mannequin.indices) {
        if (index >= mannequin.vertices.size()) {
            std::cerr << "procedural mannequin index is out of range\n";
            return 5;
        }
    }
    for (const auto& vertex : mannequin.vertices) {
        for (float value : vertex.position) if (!std::isfinite(value)) return 6;
        for (float value : vertex.normal) if (!std::isfinite(value)) return 7;
    }
    if (!arut::rendering::make_robot_mannequin_mesh(skeleton, 4U).indices.empty()) {
        std::cerr << "invalid radial segment count was accepted\n";
        return 8;
    }
    std::cout << "Phase 4 mannequin tests passed: bones=" << skeleton.bones.size()
              << " vertices=" << mannequin.vertices.size()
              << " triangles=" << mannequin.indices.size()/3U << '\n';
    return 0;
}
