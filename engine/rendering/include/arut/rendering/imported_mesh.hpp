#pragma once

#include "arut/animation/gltf_importer.hpp"
#include "arut/rendering/mesh.hpp"

#include <cmath>
#include <cstdint>
#include <limits>

namespace arut::rendering {

// Converts the importer's compact glTF vertex representation into the exact
// interleaved format consumed by the existing mobile PBR Vulkan pipeline.
[[nodiscard]] inline bool convert_imported_skinned_mesh(
    const animation::SkeletalAsset& asset,
    const animation::ImportedSkinnedMesh& source,
    MeshData& output) {
    output = {};
    if (!asset.valid() || source.vertices.empty() || source.indices.empty() ||
        source.indices.size() % 3U != 0U ||
        source.vertices.size() > std::numeric_limits<std::uint32_t>::max()) return false;

    MeshData converted{};
    converted.vertices.reserve(source.vertices.size());
    converted.indices = source.indices;

    for (const std::uint32_t index : source.indices)
        if (index >= source.vertices.size()) return false;

    for (const auto& input : source.vertices) {
        const float px = input.position.x, py = input.position.y, pz = input.position.z;
        const float nx = input.normal.x, ny = input.normal.y, nz = input.normal.z;
        if (!std::isfinite(px) || !std::isfinite(py) || !std::isfinite(pz) ||
            !std::isfinite(nx) || !std::isfinite(ny) || !std::isfinite(nz) ||
            !std::isfinite(input.uv[0]) || !std::isfinite(input.uv[1])) return false;

        MeshVertex vertex{};
        vertex.position = {px, py, pz};
        vertex.normal = normalize3({nx, ny, nz});
        vertex.tangent = {1.0F, 0.0F, 0.0F, 1.0F};
        vertex.uv = {input.uv[0], input.uv[1]};

        float weight_sum = 0.0F;
        for (std::size_t influence = 0; influence < 4U; ++influence) {
            const float weight = input.weights[influence];
            if (!std::isfinite(weight) || weight < 0.0F ||
                input.joints[influence] >= asset.joint_count) return false;
            vertex.joints[influence] = input.joints[influence];
            weight_sum += weight;
        }
        if (!std::isfinite(weight_sum)) return false;
        if (weight_sum > 1.0e-8F) {
            for (std::size_t influence = 0; influence < 4U; ++influence)
                vertex.weights[influence] = input.weights[influence] / weight_sum;
        } else {
            // Match MeshVertex's documented rigid-vertex convention.
            vertex.weights = {};
            vertex.joints = {};
        }
        converted.vertices.push_back(vertex);
    }

    output = std::move(converted);
    return true;
}

} // namespace arut::rendering
