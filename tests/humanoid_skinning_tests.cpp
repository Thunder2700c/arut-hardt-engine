#include "arut/rendering/humanoid_skinning.hpp"
#include "arut/rendering/robot_mannequin.hpp"

#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <limits>
#include <algorithm>

int main() {
    using namespace arut::core;
    using namespace arut::rendering;

    const auto skeleton = make_robot_mannequin_skeleton();
    const auto mesh = make_robot_mannequin_mesh(skeleton);
    assert(!mesh.vertices.empty() && !mesh.indices.empty());
    // The default model must be a complete high-resolution humanoid silhouette,
    // not the former sparse set of low-poly bone tubes and joint balls.
    assert(mesh.vertices.size() > 5000U);
    assert(mesh.indices.size() > 20000U);
    std::array<float, 3> minimum{
        std::numeric_limits<float>::max(), std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max()};
    std::array<float, 3> maximum{
        std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(),
        std::numeric_limits<float>::lowest()};
    for (const auto& vertex : mesh.vertices) {
        for (std::size_t axis = 0; axis < 3U; ++axis) {
            assert(std::isfinite(vertex.position[axis]));
            assert(std::isfinite(vertex.normal[axis]));
            minimum[axis] = std::min(minimum[axis], vertex.position[axis]);
            maximum[axis] = std::max(maximum[axis], vertex.position[axis]);
        }
    }
    assert(maximum[1] - minimum[1] > 1.5F);
    assert(maximum[0] - minimum[0] > 1.0F);
    // A-pose arms descend from the shoulder line rather than extending flat.
    const auto& rig = skeleton.bones;
    assert(rig[static_cast<std::size_t>(HumanoidBone::LeftUpperArm)].rest_translation.y < 0.0F);
    assert(rig[static_cast<std::size_t>(HumanoidBone::RightUpperArm)].rest_translation.y < 0.0F);

    std::size_t blended_vertices = 0U;
    for (const auto& vertex : mesh.vertices) {
        float sum = 0.0F;
        for (std::size_t influence = 0; influence < 4U; ++influence) {
            const float weight = vertex.weights[influence];
            assert(std::isfinite(weight) && weight >= 0.0F);
            if (weight > 0.0F) assert(vertex.joints[influence] < kHumanoidBoneCount);
            sum += weight;
        }
        assert(std::abs(sum - 1.0F) < 1.0e-5F);
        if (vertex.weights[0] > 0.0F && vertex.weights[1] > 0.0F)
            ++blended_vertices;
    }
    assert(blended_vertices > 0U);

    auto pose = make_rest_humanoid_pose(skeleton);
    HumanoidSkinningPalette rest_palette{};
    assert(build_humanoid_skinning_palette(skeleton, pose, rest_palette));
    assert(rest_palette.matrices.size() == 128U);
    for (const auto& matrix : rest_palette.matrices) {
        for (std::size_t i = 0; i < 16U; ++i) {
            const float expected = Mat4::identity().value[i];
            assert(std::abs(matrix.value[i] - expected) < 1.0e-5F);
        }
    }

    constexpr std::size_t chest_index = static_cast<std::size_t>(HumanoidBone::Chest);
    constexpr std::size_t head_index = static_cast<std::size_t>(HumanoidBone::Head);
    const std::array<float, 3> head_bind_position{0.0F, 1.60F, 0.0F};
    const auto head_before = transform_humanoid_point(
        rest_palette.matrices[head_index], head_bind_position);
    pose.local_rotations[chest_index] =
        quaternion_from_axis_angle({0.0F, 0.0F, 1.0F}, 0.45F);

    HumanoidSkinningPalette animated_palette{};
    assert(build_humanoid_skinning_palette(skeleton, pose, animated_palette));
    const auto head_after = transform_humanoid_point(
        animated_palette.matrices[head_index], head_bind_position);
    assert(std::abs(head_after[0] - head_before[0]) > 0.02F ||
           std::abs(head_after[1] - head_before[1]) > 0.02F);

    pose.local_rotations[chest_index].w = std::numeric_limits<float>::quiet_NaN();
    assert(!build_humanoid_skinning_palette(skeleton, pose, animated_palette));

    pose = make_rest_humanoid_pose(skeleton);
    pose.local_translations[head_index].y =
        std::numeric_limits<float>::infinity();
    assert(!build_humanoid_skinning_palette(skeleton, pose, animated_palette));
    return 0;
}
