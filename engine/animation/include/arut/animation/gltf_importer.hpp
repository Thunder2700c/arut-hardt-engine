#pragma once

#include "arut/animation/runtime.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace arut::animation {

inline constexpr std::size_t kMaxImportedJoints = kMaxPoseJoints;

struct ImportedJoint final {
    std::string name;
    std::int16_t parent{-1};
    Transform rest_local{};
    std::array<float, 16> inverse_bind_matrix{};
};

struct ImportedSkinnedVertex final {
    core::Vec3 position{};
    core::Vec3 normal{0.0F, 1.0F, 0.0F};
    float uv[2]{0.0F, 0.0F};
    std::array<std::uint16_t, 4> joints{};
    std::array<float, 4> weights{1.0F, 0.0F, 0.0F, 0.0F};
};

struct ImportedSkinnedMesh final {
    std::string name;
    std::vector<ImportedSkinnedVertex> vertices;
    std::vector<std::uint32_t> indices;
};

struct SkeletalAsset final {
    std::array<ImportedJoint, kMaxImportedJoints> joints{};
    std::size_t joint_count{0U};
    std::vector<ImportedSkinnedMesh> meshes;
    std::vector<AnimationClip> animations;

    [[nodiscard]] bool valid() const noexcept;
};

struct GltfImportReport final {
    std::string message;
    std::size_t imported_joints{0U};
    std::size_t imported_meshes{0U};
    std::size_t imported_vertices{0U};
    std::size_t imported_indices{0U};
    std::size_t imported_animations{0U};
};

// Imports a self-contained GLB 2.0 buffer. External .gltf URI resources are
// intentionally excluded from this memory API; use GLB with embedded BIN data.
[[nodiscard]] bool import_glb(
    const void* bytes,
    std::size_t byte_count,
    SkeletalAsset& output,
    GltfImportReport& report);

// Imports animation-only GLBs whose channels target the supplied skinned rig by node name.
[[nodiscard]] bool import_glb_animation_clips(
    const void* bytes, std::size_t byte_count,
    const SkeletalAsset& source_rig,
    std::vector<AnimationClip>& output,
    GltfImportReport& report);

} // namespace arut::animation
