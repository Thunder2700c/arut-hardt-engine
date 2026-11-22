#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>
#include "arut/rendering/visibility.hpp"

namespace arut::rendering {

struct alignas(16) GpuVisibilityInstance final {
    // xyz = world-space centre, w = conservative bounding-sphere radius.
    std::array<float, 4> bounds{};
    // Column-major model matrix, matching GLSL mat4 std430 layout.
    std::array<float, 16> model{};
};

struct alignas(16) GpuVisibilityPushConstants final {
    // Six normalized plane equations: dot(normal, centre) + distance >= -radius.
    std::array<std::array<float, 4>, 6> frustum_planes{};
    std::uint32_t instance_count{0};
    std::array<std::uint32_t, 3> padding{};
};

[[nodiscard]] inline bool compact_visible_instances_cpu(
    const ViewFrustum& frustum,
    std::span<const GpuVisibilityInstance> source_instances,
    std::vector<Mat4>& output_models) noexcept {
    output_models.clear();
    try {
        output_models.reserve(source_instances.size());
        for (const auto& source : source_instances) {
            const BoundingSphere bounds{{
                source.bounds[0], source.bounds[1], source.bounds[2]
            }, source.bounds[3]};
            if (!sphere_visible(frustum, bounds)) continue;
            Mat4 model{};
            model.value = source.model;
            output_models.push_back(model);
        }
    } catch (...) {
        output_models.clear();
        return false;
    }
    return true;
}

struct IndexedIndirectArguments final {
    std::uint32_t index_count{0};
    std::uint32_t instance_count{0};
    std::uint32_t first_index{0};
    std::int32_t vertex_offset{0};
    std::uint32_t first_instance{0};
};

inline constexpr std::uint32_t kVisibilityWorkgroupSize = 64U;

[[nodiscard]] constexpr std::uint32_t visibility_workgroup_count(
    std::uint32_t instance_count) noexcept {
    return (instance_count + kVisibilityWorkgroupSize - 1U) / kVisibilityWorkgroupSize;
}

static_assert(sizeof(GpuVisibilityInstance) == 80U);
static_assert(alignof(GpuVisibilityInstance) == 16U);
static_assert(offsetof(GpuVisibilityInstance, model) == 16U);
static_assert(sizeof(GpuVisibilityPushConstants) == 112U);
static_assert(offsetof(GpuVisibilityPushConstants, instance_count) == 96U);
static_assert(offsetof(GpuVisibilityPushConstants, padding) == 100U);
static_assert(sizeof(IndexedIndirectArguments) == 20U);
static_assert(offsetof(IndexedIndirectArguments, instance_count) == 4U);

} // namespace arut::rendering
