#pragma once

#include "arut/animation/gltf_importer.hpp"
#include "arut/rendering/mesh.hpp"

#include <array>
#include <cmath>
#include <cstddef>

namespace arut::rendering {

struct ImportedSkinningPalette final {
    std::array<Mat4, animation::kMaxImportedJoints> matrices{};
    std::size_t joint_count{0U};
};

namespace detail {

inline Mat4 imported_local_matrix(const animation::Transform& transform) noexcept {
    const auto& q = transform.rotation;
    const float length2 = q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w;
    if (!std::isfinite(length2) || length2 <= 1.0e-12F) return {};
    const float inv = 1.0F / std::sqrt(length2);
    const float x=q.x*inv, y=q.y*inv, z=q.z*inv, w=q.w*inv;
    Mat4 m = Mat4::identity();
    m.value[0]=(1.0F-2.0F*(y*y+z*z))*transform.scale.x;
    m.value[1]=(2.0F*(x*y+z*w))*transform.scale.x;
    m.value[2]=(2.0F*(x*z-y*w))*transform.scale.x;
    m.value[4]=(2.0F*(x*y-z*w))*transform.scale.y;
    m.value[5]=(1.0F-2.0F*(x*x+z*z))*transform.scale.y;
    m.value[6]=(2.0F*(y*z+x*w))*transform.scale.y;
    m.value[8]=(2.0F*(x*z+y*w))*transform.scale.z;
    m.value[9]=(2.0F*(y*z-x*w))*transform.scale.z;
    m.value[10]=(1.0F-2.0F*(x*x+y*y))*transform.scale.z;
    m.value[12]=transform.translation.x;
    m.value[13]=transform.translation.y;
    m.value[14]=transform.translation.z;
    return m;
}

inline bool resolve_imported_joint(
    const animation::SkeletalAsset& asset, const animation::Pose& pose,
    std::size_t index, std::array<Mat4, animation::kMaxImportedJoints>& globals,
    std::array<std::uint8_t, animation::kMaxImportedJoints>& marks) noexcept {
    if (marks[index] == 2U) return true;
    if (marks[index] == 1U) return false;
    marks[index] = 1U;
    const auto parent = asset.joints[index].parent;
    const Mat4 local = imported_local_matrix(pose.joints[index]);
    if (parent >= 0) {
        const auto parent_index = static_cast<std::size_t>(parent);
        if (parent_index >= asset.joint_count ||
            !resolve_imported_joint(asset, pose, parent_index, globals, marks)) return false;
        globals[index] = multiply(globals[parent_index], local);
    } else {
        globals[index] = local;
    }
    marks[index] = 2U;
    return true;
}

} // namespace detail

// Builds a column-major skin palette from imported glTF inverse-bind matrices.
// Parent joints may appear after their children in the source skin array.
[[nodiscard]] inline bool build_imported_skinning_palette(
    const animation::SkeletalAsset& asset, const animation::Pose& pose,
    ImportedSkinningPalette& output) noexcept {
    output = {};
    if (asset.joint_count == 0U ||
        asset.joint_count > animation::kMaxImportedJoints ||
        pose.joint_count != asset.joint_count) return false;

    std::array<Mat4, animation::kMaxImportedJoints> globals{};
    std::array<std::uint8_t, animation::kMaxImportedJoints> marks{};
    for (std::size_t i=0; i<asset.joint_count; ++i) {
        const auto& t=pose.joints[i];
        const auto& q=t.rotation;
        if (!std::isfinite(t.translation.x)||!std::isfinite(t.translation.y)||
            !std::isfinite(t.translation.z)||!std::isfinite(t.scale.x)||
            !std::isfinite(t.scale.y)||!std::isfinite(t.scale.z)||
            !std::isfinite(q.x)||!std::isfinite(q.y)||!std::isfinite(q.z)||
            !std::isfinite(q.w)||!std::isfinite(q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w)||
            q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w<=1.0e-12F ||
            asset.joints[i].parent < -1 ||
            (asset.joints[i].parent >= 0 &&
             static_cast<std::size_t>(asset.joints[i].parent) >= asset.joint_count))
            return false;
        for (float v:asset.joints[i].inverse_bind_matrix) if (!std::isfinite(v)) return false;
    }
    for (std::size_t i=0; i<asset.joint_count; ++i)
        if (!detail::resolve_imported_joint(asset,pose,i,globals,marks)) return false;

    output.joint_count=asset.joint_count;
    for (std::size_t i=0; i<asset.joint_count; ++i) {
        Mat4 inverse_bind{};
        inverse_bind.value=asset.joints[i].inverse_bind_matrix;
        output.matrices[i]=multiply(globals[i],inverse_bind);
        for (float v:output.matrices[i].value) if (!std::isfinite(v)) {
            output={};
            return false;
        }
    }
    return true;
}

} // namespace arut::rendering
