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
    std::size_t outward_triangles = 0U;
    for (std::size_t i = 0; i + 2U < mannequin.indices.size(); i += 3U) {
        const auto& a = mannequin.vertices[mannequin.indices[i]];
        const auto& b = mannequin.vertices[mannequin.indices[i + 1U]];
        const auto& c = mannequin.vertices[mannequin.indices[i + 2U]];
        const std::array<float, 3> ab{b.position[0]-a.position[0], b.position[1]-a.position[1], b.position[2]-a.position[2]};
        const std::array<float, 3> ac{c.position[0]-a.position[0], c.position[1]-a.position[1], c.position[2]-a.position[2]};
        const std::array<float, 3> face{
            ab[1]*ac[2]-ab[2]*ac[1], ab[2]*ac[0]-ab[0]*ac[2], ab[0]*ac[1]-ab[1]*ac[0]};
        const float area2 = face[0]*face[0] + face[1]*face[1] + face[2]*face[2];
        if (area2 < 1.0e-10F) continue;
        const float facing = face[0]*a.normal[0] + face[1]*a.normal[1] + face[2]*a.normal[2];
        if (facing <= 0.0F) {
            std::cerr << "mannequin triangle winding is not outward-facing\\n";
            return 9;
        }
        ++outward_triangles;
    }
    if (outward_triangles == 0U) return 10;
    if (!arut::rendering::make_robot_mannequin_mesh(skeleton, 4U).indices.empty()) {
        std::cerr << "invalid radial segment count was accepted\n";
        return 8;
    }
    std::cout << "Phase 4 mannequin tests passed: bones=" << skeleton.bones.size()
              << " vertices=" << mannequin.vertices.size()
              << " triangles=" << mannequin.indices.size()/3U << '\n';
    return 0;
}
