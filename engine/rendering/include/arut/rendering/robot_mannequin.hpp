#pragma once

#include "arut/core/humanoid_skeleton.hpp"
#include "arut/rendering/mesh.hpp"

#include <array>
#include <cmath>
#include <cstdint>

namespace arut::rendering {

// Builds a deterministic, engine-owned robot mannequin from the reference rig.
// The result is a bind-pose mesh with four-influence joint weights.
[[nodiscard]] inline MeshData make_robot_mannequin_mesh(
    const core::HumanoidSkeleton& skeleton = core::make_robot_mannequin_skeleton(),
    std::uint32_t radial_segments = 10U) {
    MeshData mesh{};
    if (!skeleton.valid() || radial_segments < 6U || radial_segments > 64U) return mesh;

    using V3 = std::array<float, 3>;
    const auto add = [](const V3& a, const V3& b) -> V3 {
        return {a[0] + b[0], a[1] + b[1], a[2] + b[2]};
    };
    const auto sub = [](const V3& a, const V3& b) -> V3 {
        return {a[0] - b[0], a[1] - b[1], a[2] - b[2]};
    };
    const auto mul = [](const V3& a, float s) -> V3 {
        return {a[0] * s, a[1] * s, a[2] * s};
    };
    const auto dot = [](const V3& a, const V3& b) {
        return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
    };
    const auto cross = [](const V3& a, const V3& b) -> V3 {
        return {a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0]};
    };
    const auto norm = [&](const V3& v) -> V3 {
        const float d = dot(v, v);
        if (!(d > 1.0e-12F)) return {0.0F, 1.0F, 0.0F};
        return mul(v, 1.0F / std::sqrt(d));
    };
    std::array<V3, core::kHumanoidBoneCount> positions{};
    for (std::size_t i = 0; i < skeleton.bones.size(); ++i) {
        const auto& bone = skeleton.bones[i];
        const V3 local{bone.rest_translation.x, bone.rest_translation.y, bone.rest_translation.z};
        positions[i] = bone.parent < 0 ? local : add(positions[static_cast<std::size_t>(bone.parent)], local);
    }

    const auto append_ellipsoid = [&](const V3& center, const V3& radii,
                                      std::uint32_t joint_index) {
        constexpr std::uint32_t rings = 6U;
        const std::uint32_t base = static_cast<std::uint32_t>(mesh.vertices.size());
        for (std::uint32_t r = 0; r <= rings; ++r) {
            const float phi = 3.14159265358979323846F * static_cast<float>(r) / static_cast<float>(rings);
            for (std::uint32_t s = 0; s <= radial_segments; ++s) {
                const float theta = 6.28318530717958647692F * static_cast<float>(s) / static_cast<float>(radial_segments);
                const V3 sphere_normal{std::sin(phi)*std::cos(theta), std::cos(phi), std::sin(phi)*std::sin(theta)};
                const V3 p = add(center, {sphere_normal[0]*radii[0],
                    sphere_normal[1]*radii[1], sphere_normal[2]*radii[2]});
                const V3 n = norm({sphere_normal[0]/radii[0],
                    sphere_normal[1]/radii[1], sphere_normal[2]/radii[2]});
                MeshVertex vertex{p, n, {1.0F,0.0F,0.0F,1.0F},
                    {static_cast<float>(s)/static_cast<float>(radial_segments),
                     static_cast<float>(r)/static_cast<float>(rings)}};
                vertex.joints = {joint_index, 0U, 0U, 0U};
                vertex.weights = {1.0F, 0.0F, 0.0F, 0.0F};
                mesh.vertices.push_back(vertex);
            }
        }
        for (std::uint32_t r = 0; r < rings; ++r) for (std::uint32_t s = 0; s < radial_segments; ++s) {
            const std::uint32_t a = base + r*(radial_segments+1U)+s, b=a+radial_segments+1U;
            mesh.indices.insert(mesh.indices.end(), {a,a+1U,b,a+1U,b+1U,b});
        }
    };
    const auto append_sphere = [&](const V3& center, float radius,
                                   std::uint32_t joint_index) {
        append_ellipsoid(center, {radius, radius, radius}, joint_index);
    };

    const auto append_bone = [&](const V3& start, const V3& end, float radius,
                                 std::uint32_t parent_index, std::uint32_t child_index) {
        const V3 axis = norm(sub(end, start));
        V3 tangent = norm(cross(axis, {0.0F,0.0F,1.0F}));
        if (dot(tangent,tangent) < 0.5F) tangent = norm(cross(axis,{0.0F,1.0F,0.0F}));
        const V3 bitangent = norm(cross(axis,tangent));
        constexpr std::uint32_t longitudinal_segments = 5U;
        const std::uint32_t base = static_cast<std::uint32_t>(mesh.vertices.size());
        for (std::uint32_t ring=0; ring<=longitudinal_segments; ++ring) {
            const float t = static_cast<float>(ring) /
                            static_cast<float>(longitudinal_segments);
            const V3 center = add(start, mul(sub(end, start), t));
            for (std::uint32_t s=0; s<=radial_segments; ++s) {
                const float angle=6.28318530717958647692F*static_cast<float>(s)/static_cast<float>(radial_segments);
                const V3 radial=add(mul(tangent,std::cos(angle)),mul(bitangent,std::sin(angle)));
                const V3 p=add(center,mul(radial,radius));
                MeshVertex vertex{p,radial,{1.0F,0.0F,0.0F,1.0F},
                    {static_cast<float>(s)/static_cast<float>(radial_segments),t}};
                vertex.joints = {parent_index, child_index, 0U, 0U};
                vertex.weights = {1.0F-t, t, 0.0F, 0.0F};
                mesh.vertices.push_back(vertex);
            }
        }
        for (std::uint32_t ring=0; ring<longitudinal_segments; ++ring) {
            for (std::uint32_t s=0; s<radial_segments; ++s) {
                const std::uint32_t a=base+ring*(radial_segments+1U)+s;
                const std::uint32_t b=a+radial_segments+1U;
                mesh.indices.insert(mesh.indices.end(),{a,a+1U,b,a+1U,b+1U,b});
            }
        }
    };

    for (std::size_t i=1; i<skeleton.bones.size(); ++i) {
        const auto& bone=skeleton.bones[i];
        const auto parent=static_cast<std::size_t>(bone.parent);
        append_bone(positions[parent],positions[i],bone.visual_radius,
                    static_cast<std::uint32_t>(parent), static_cast<std::uint32_t>(i));
    }
    for (std::size_t i=0; i<skeleton.bones.size(); ++i) {
        append_sphere(positions[i],skeleton.bones[i].visual_radius*1.12F,
            static_cast<std::uint32_t>(i));
    }
    constexpr std::array<core::HumanoidBone, 2> foot_bones{
        core::HumanoidBone::LeftFoot, core::HumanoidBone::RightFoot};
    for (const auto foot : foot_bones) {
        const auto index = static_cast<std::size_t>(foot);
        const V3 foot_center = add(positions[index], {0.0F, -0.035F, -0.105F});
        append_ellipsoid(foot_center, {0.095F, 0.055F, 0.17F},
                         static_cast<std::uint32_t>(index));
    }
    return mesh;
}

} // namespace arut::rendering
