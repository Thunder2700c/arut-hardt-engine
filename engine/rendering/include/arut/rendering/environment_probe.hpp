#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <span>
#include <vector>

namespace arut::rendering {

struct EnvironmentProbeAabb final {
    float min_x{0.0F}, min_y{0.0F}, min_z{0.0F};
    float max_x{0.0F}, max_y{0.0F}, max_z{0.0F};
};

struct EnvironmentProbeInfluence final {
    EnvironmentProbeAabb bounds{};
    float blend_distance{0.0F};
    std::int32_t priority{0};
};

inline constexpr std::size_t kMaxEnvironmentProbes = 4U;

// std140-compatible probe bounds and priority block for the PBR fragment shader.
struct alignas(16) EnvironmentProbeGpuData final {
    std::array<std::array<float, 4>, kMaxEnvironmentProbes> bounds_min_blend{};
    std::array<std::array<float, 4>, kMaxEnvironmentProbes> bounds_max{};
    std::array<std::int32_t, 4> priorities{};
    std::array<std::uint32_t, 4> metadata{}; // x = active probe count
};
static_assert(sizeof(EnvironmentProbeGpuData) == 160U);

[[nodiscard]] inline bool build_environment_probe_gpu_data(
    std::span<const EnvironmentProbeInfluence> probes,
    EnvironmentProbeGpuData& output) noexcept {
    output = {};
    if (probes.size() > kMaxEnvironmentProbes) return false;
    for (std::size_t i = 0; i < probes.size(); ++i) {
        const auto& probe = probes[i];
        const auto& b = probe.bounds;
        if (!std::isfinite(b.min_x) || !std::isfinite(b.min_y) ||
            !std::isfinite(b.min_z) || !std::isfinite(b.max_x) ||
            !std::isfinite(b.max_y) || !std::isfinite(b.max_z) ||
            b.min_x > b.max_x || b.min_y > b.max_y || b.min_z > b.max_z ||
            !std::isfinite(probe.blend_distance) || probe.blend_distance < 0.0F) {
            output = {};
            return false;
        }
        output.bounds_min_blend[i] = {
            b.min_x, b.min_y, b.min_z, probe.blend_distance};
        output.bounds_max[i] = {b.max_x, b.max_y, b.max_z, 0.0F};
        output.priorities[i] = probe.priority;
    }
    output.metadata[0] = static_cast<std::uint32_t>(probes.size());
    return true;
}

// Returns normalized influence weights in input order. Only probes at the
// highest priority with non-zero influence participate in the blend.
[[nodiscard]] inline bool evaluate_environment_probe_weights(
    std::span<const EnvironmentProbeInfluence> probes,
    float x, float y, float z, std::vector<float>& weights) noexcept {
    weights.clear();
    if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z)) return false;
    try {
        weights.assign(probes.size(), 0.0F);
        std::int32_t highest_priority = 0;
        bool has_influence = false;
        float total = 0.0F;
        for (std::size_t i = 0; i < probes.size(); ++i) {
            const auto& probe = probes[i];
            const auto& b = probe.bounds;
            if (!std::isfinite(b.min_x) || !std::isfinite(b.min_y) ||
                !std::isfinite(b.min_z) || !std::isfinite(b.max_x) ||
                !std::isfinite(b.max_y) || !std::isfinite(b.max_z) ||
                b.min_x > b.max_x || b.min_y > b.max_y || b.min_z > b.max_z ||
                !std::isfinite(probe.blend_distance)) continue;

            const float dx = std::max({b.min_x - x, 0.0F, x - b.max_x});
            const float dy = std::max({b.min_y - y, 0.0F, y - b.max_y});
            const float dz = std::max({b.min_z - z, 0.0F, z - b.max_z});
            const float distance = std::sqrt(dx * dx + dy * dy + dz * dz);
            float weight = 0.0F;
            if (distance <= 0.0F) {
                weight = 1.0F;
            } else if (probe.blend_distance > 0.0F &&
                       distance < probe.blend_distance) {
                const float t = 1.0F - distance / probe.blend_distance;
                weight = t * t * (3.0F - 2.0F * t);
            }
            if (!(weight > 0.0F)) continue;
            if (!has_influence || probe.priority > highest_priority) {
                highest_priority = probe.priority;
                has_influence = true;
                total = 0.0F;
                std::fill(weights.begin(), weights.end(), 0.0F);
            }
            if (probe.priority == highest_priority) {
                weights[i] = weight;
                total += weight;
            }
        }
        if (total > 0.0F) {
            for (float& weight : weights) weight /= total;
        }
    } catch (...) {
        weights.clear();
        return false;
    }
    return true;
}

} // namespace arut::rendering
